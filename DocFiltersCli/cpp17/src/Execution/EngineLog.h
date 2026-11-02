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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_ENGINELOG_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_ENGINELOG_H

#include "Configuration/SharedOptions.h"
#include "Runtime/IConsole.h"

#include <optional>
#include <set>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>
				/// <c>--log-level</c> and <c>--log-modules</c>: what the engine's own readers
				/// log, and where it goes.
				/// </summary>
				/// <remarks>
				/// The engine asks, per module, how much it should log, then hands back each
				/// message. Both arrive as callbacks on the open call, so this only reaches
				/// documents being opened --- there is no logging of a page loop, for the
				/// same reason there is no cancelling one.
				///
				/// The filtering happens here rather than being asked of the engine, exactly
				/// as doc2text does it: the engine has no notion of which modules the user
				/// named, so the answer to "what level for module X" is where the
				/// <c>--log-modules</c> set is applied.
				///
				/// Output goes to <b>stderr</b>, which doc2text also does. It shares that
				/// stream with the two contract error shapes, which is safe because nothing
				/// is logged unless <c>--log-level</c> was passed: a run that says nothing
				/// about logging produces the same stderr it always did.
				/// </remarks>
				class EngineLog
				{
				public:
					static constexpr int levelNone = 0;
					static constexpr int levelCritical = 1;
					static constexpr int levelError = 2;
					static constexpr int levelWarning = 3;
					static constexpr int levelInformation = 4;
					static constexpr int levelDebug = 5;
					static constexpr int levelTrace = 6;

					/// <summary>
					/// The log for these options, or nullopt when nothing was asked for.
					/// </summary>
					/// <remarks>
					/// Absent rather than a disabled instance, so a run with no logging
					/// hands the engine no callback at all and the engine can skip composing
					/// messages nobody will print.
					/// </remarks>
					static std::optional<EngineLog> for_(const Configuration::DiagnosticOptions& options, Runtime::IConsole& console);

					/// <summary>
					/// Answers the engine's "how much should <paramref name="module"/> log".
					/// </summary>
					/// <remarks>
					/// <see cref="levelNone"/> for a module the user did not name, which is
					/// how <c>--log-modules</c> is enforced --- the engine then composes
					/// nothing for it rather than composing messages this side throws away.
					/// </remarks>
					int levelFor(const std::string& module) const;

					/// <summary>Writes one message the engine produced.</summary>
					void write(int level, const std::string& module, const std::string& message) const;

					/// <summary>
					/// The fixed-width level name, so the module and message columns line
					/// up.
					/// </summary>
					/// <remarks>
					/// Same five names and the same padding doc2text uses, minus the colour:
					/// this tool writes no escape codes of its own anywhere else.
					/// </remarks>
					static std::string name(int level);

					/// <summary>
					/// <c>--log-level</c>'s vocabulary, mapped onto the engine's scale.
					/// </summary>
					/// <remarks>
					/// Anything unrecognised is <see cref="levelNone"/> rather than an error,
					/// matching doc2text, whose own parse falls through to none. The value
					/// arrives already folded --- argument parsing folds every token before
					/// this is reached --- so this compares it verbatim, exactly as the C#
					/// reference does.
					/// </remarks>
					static int parseLevel(const std::optional<std::string>& value);

				private:
					EngineLog(int level, std::set<std::string> modules, Runtime::IConsole& console)
						: _level(level), _modules(std::move(modules)), _console(console)
					{
					}

					int _level;

					/// <summary>Every named module, already ASCII-folded, so lookup folds only its argument.</summary>
					std::set<std::string> _modules;

					Runtime::IConsole& _console;
				};
			}
		}
	}
}

#endif
