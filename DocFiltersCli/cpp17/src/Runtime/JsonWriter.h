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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_JSONWRITER_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_JSONWRITER_H

#include <optional>
#include <stack>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// A minimal JSON writer, hand-rolled so that no library's escaping policy
				/// reaches the contract.
				/// </summary>
				/// <remarks>
				/// Only used for the <c>--json</c> listings on <c>list</c>, <c>identify</c>,
				/// <c>meta</c>, <c>formats</c> and <c>options</c>. Note that
				/// <c>convert --to json</c> is a completely different thing: that is the
				/// engine's own JSON canvas, and its bytes are streamed through untouched.
				///
				/// Output rules: two-space indent, one key per line; keys in insertion
				/// order, never sorted; UTF-8 with no byte order mark, LF line endings;
				/// non-ASCII written literally, not escaped as <c>\uXXXX</c>; a trailing
				/// newline at the end of the document.
				///
				/// <c>value(const char*)</c> and <c>value(int)</c> exist only so a string
				/// literal or a plain int literal --- both exact matches for one overload
				/// here --- cannot be resolved through a standard conversion to
				/// <c>bool</c> instead: without them, <c>value("x")</c> and <c>value(42)</c>
				/// would each compete between a standard conversion (to <c>bool</c>) and a
				/// user-defined or integral one, which C++ either resolves to the wrong
				/// overload silently or rejects outright as ambiguous.
				/// </remarks>
				class JsonWriter
				{
				public:
					/// <summary>Starts an object. Must be matched by <see cref="endObject"/>.</summary>
					JsonWriter& beginObject();

					JsonWriter& endObject();

					/// <summary>Starts an array. Must be matched by <see cref="endArray"/>.</summary>
					JsonWriter& beginArray();

					JsonWriter& endArray();

					/// <summary>Writes a property name, which the next value belongs to.</summary>
					JsonWriter& name(const std::string& propertyName);

					/// <summary>Writes a string value, or null.</summary>
					JsonWriter& value(const std::optional<std::string>& text);
					JsonWriter& value(const char* text);

					JsonWriter& value(long long number);
					JsonWriter& value(int number);

					JsonWriter& value(bool flag);

					/// <summary>Convenience for a name and string value in one call.</summary>
					JsonWriter& property(const std::string& propertyName, const std::optional<std::string>& text);
					JsonWriter& property(const std::string& propertyName, const char* text);

					JsonWriter& property(const std::string& propertyName, long long number);
					JsonWriter& property(const std::string& propertyName, int number);

					JsonWriter& property(const std::string& propertyName, bool flag);

					/// <summary>The finished document, ending with a newline.</summary>
					/// <exception cref="std::logic_error">An object or array was left open.</exception>
					std::string toString() const;

					/// <summary>Escapes a string the way <see cref="value(const std::optional&lt;std::string&gt;&amp;)"/> does. Exposed for tests.</summary>
					static std::string escape(const std::string& text);

				private:
					enum class ContainerKind
					{
						Object,
						Array
					};

					struct Frame
					{
						ContainerKind kind;
						bool hasChildren = false;
					};

					JsonWriter& beginContainer(ContainerKind kind, char openToken);
					JsonWriter& endContainer(char closeToken);
					void beforeValue();
					void beginItem();

					std::string _text;
					std::stack<Frame> _open;

					/// <summary>
					/// True right after <see cref="name"/>, so the next token written is that
					/// property's value rather than a new array element or sibling property
					/// --- its comma and indent were already written by <see cref="name"/>
					/// itself.
					/// </summary>
					bool _atPropertyValue = false;
				};
			}
		}
	}
}

#endif
