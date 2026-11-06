/*
   (c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_CLI_ARGUMENTDEFINITION_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_CLI_ARGUMENTDEFINITION_H

#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Cli
			{
				/// <summary>How many values an argument takes.</summary>
				enum class ArgumentArity
				{
					/// <summary>A switch: <c>--overwrite</c>. The setter receives no value.</summary>
					NoValue,

					/// <summary>One value: <c>--out-dir out/</c>. Later occurrences replace earlier ones.</summary>
					SingleValue,

					/// <summary>Repeatable: <c>--redact</c>, <c>-p</c>. The setter is called once per occurrence.</summary>
					MultipleValue
				};

				/// <summary>
				/// The parts of an argument that do not depend on what it writes into. This is
				/// the view <c>--print-contract</c> and the coverage tests use.
				/// </summary>
				class IArgumentDescriptor
				{
				public:
					virtual ~IArgumentDescriptor() = default;

					/// <summary>
					/// Every spelling of the argument, pipe-separated, e.g. <c>-o|--out-dir</c>.
					/// </summary>
					/// <remarks>
					/// The shape is McMaster's, which this tool was once built on, and is kept
					/// because the four implementations spell their tables identically and it is
					/// the one string a reader can compare against <c>arguments.txt</c> by eye.
					/// It is now read only here, to derive <see cref="longName"/> and
					/// <see cref="shortName"/>.
					/// </remarks>
					virtual const std::string& templateText() const = 0;

					/// <summary>Help text.</summary>
					virtual const std::string& description() const = 0;

					/// <summary>How many values it takes.</summary>
					virtual ArgumentArity arity() const = 0;

					/// <summary>The long name without dashes, e.g. <c>out-dir</c>. Used to sort the contract dump.</summary>
					virtual const std::string& longName() const = 0;

					/// <summary>The short name without its dash, e.g. <c>o</c>, or absent when there is none.</summary>
					virtual const std::optional<std::string>& shortName() const = 0;
				};

				/// <summary>
				/// One argument, paired with the setter that writes its value into <c>T</c>.
				/// </summary>
				/// <remarks>
				/// <c>T</c> is <c>Configuration::SharedOptions</c> for a shared argument, or a
				/// per-command options struct such as <c>Configuration::ConvertOptions</c>.
				/// </remarks>
				template <typename T>
				class ArgumentDefinition : public IArgumentDescriptor
				{
				public:
					/// <summary>
					/// Writes the parsed value into the target. For <see cref="ArgumentArity::NoValue"/>
					/// the value argument is absent.
					/// </summary>
					using Setter = std::function<void(T&, const std::optional<std::string>&)>;

					ArgumentDefinition(std::string templateSpec, std::string helpText, ArgumentArity arity, Setter setter)
						: _template(std::move(templateSpec)), _description(std::move(helpText)), _arity(arity), _setter(std::move(setter))
					{
						std::string::size_type start = 0;
						while (true)
						{
							std::string::size_type bar = _template.find('|', start);
							std::string form = _template.substr(start, bar == std::string::npos ? std::string::npos : bar - start);

							if (form.rfind("--", 0) == 0)
								_longName = form.substr(2);
							else if (form.rfind("-", 0) == 0)
								_shortName = form.substr(1);

							if (bar == std::string::npos)
								break;
							start = bar + 1;
						}
					}

					const std::string& templateText() const override { return _template; }

					const std::string& description() const override { return _description; }

					ArgumentArity arity() const override { return _arity; }

					/// <remarks>
					/// An argument written only in short form, such as <c>-x</c>, is named by
					/// that form --- there is no long spelling to prefer.
					/// </remarks>
					const std::string& longName() const override { return _longName.empty() ? _shortName.value() : _longName; }

					const std::optional<std::string>& shortName() const override { return _shortName; }

					/// <summary>Writes <paramref name="value"/> into <paramref name="target"/>.</summary>
					const Setter& setter() const { return _setter; }

				private:
					std::string _template;
					std::string _description;
					ArgumentArity _arity;
					Setter _setter;
					std::string _longName;
					std::optional<std::string> _shortName;
				};
			}
		}
	}
}

#endif
