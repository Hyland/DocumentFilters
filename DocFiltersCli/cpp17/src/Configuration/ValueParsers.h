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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_CONFIGURATION_VALUEPARSERS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_CONFIGURATION_VALUEPARSERS_H

#include "CommandOptions.h"
#include "SharedOptions.h"

#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Configuration
			{
				/// <summary>
				/// Turns command line text into typed values.
				/// </summary>
				/// <remarks>
				/// Every conversion that can fail lives here rather than in the CLI
				/// parsing library, so that a bad value produces the same message and the
				/// same exit code in the C#, Python, Java and C++17 builds. The parsers
				/// themselves are only ever asked for <c>bool</c>, <c>std::string</c> and
				/// <c>std::vector&lt;std::string&gt;</c>, none of which can fail to
				/// convert.
				/// </remarks>
				namespace ValueParsers
				{
					/// <summary>Parses a non-negative integer, e.g. for <c>--max-depth</c>.</summary>
					/// <exception cref="Runtime::UsageError">The text is not a non-negative integer.</exception>
					int parseCount(const std::string& argument, const std::string& text);

					/// <summary>Parses a positive integer of seconds, e.g. for <c>--timeout</c>.</summary>
					int parseSeconds(const std::string& argument, const std::string& text);

					/// <summary>Parses a size in megabytes, e.g. for <c>--max-memory</c>.</summary>
					long long parseMegabytes(const std::string& argument, const std::string& text);

					/// <summary>
					/// Splits a semicolon-separated id list, e.g. <c>--include-types "PDF;DOCX"</c>.
					/// Empty entries are dropped; surrounding whitespace is trimmed.
					/// </summary>
					std::vector<std::string> parseIdList(const std::string& text);

					/// <summary>Parses one <c>--redact page,x,y,w,h</c> value.</summary>
					/// <exception cref="Runtime::UsageError">Wrong number of parts, or a part is not an integer.</exception>
					RedactSpec parseRedaction(const std::string& text);

					/// <summary>Parses one <c>--form page,name=value</c> value.</summary>
					/// <remarks>Only the first <c>=</c> separates name from value, so values may contain <c>=</c>.</remarks>
					FormValue parseFormValue(const std::string& text);

					/// <summary>Parses <c>--on-error</c>: "skip" or "stop", after <see cref="foldToken"/>.</summary>
					OnErrorBehaviour parseOnError(const std::string& text);

					/// <summary>
					/// Folds a value drawn from a vocabulary this tool publishes, so that
					/// PDF, Pdf and pdf are one token.
					/// </summary>
					/// <remarks>
					/// Applied by <c>--to</c>, <c>--on-error</c>,
					/// <c>--applies-to</c> and the two log-level arguments --- every
					/// argument whose valid values are enumerated here rather than
					/// supplied by the engine or the user. Paths, globs, regular
					/// expressions, <c>--out-name</c> templates, <c>--filter</c> text,
					/// passwords, <c>--codepage</c> and the <c>--options</c> string are
					/// used exactly as typed.
					///
					/// A thin forward to <see cref="Runtime::Strings::foldAscii"/>, kept
					/// under this name because every call site in the argument table
					/// reads <c>ValueParsers::foldToken</c> --- the same ASCII-only fold,
					/// named the way the design document's vocabulary names it here.
					/// </remarks>
					std::string foldToken(const std::string& text);
				}
			}
		}
	}
}

#endif
