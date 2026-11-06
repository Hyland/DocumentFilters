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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_STRINGS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_STRINGS_H

#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// Small text helpers shared by every stage, kept dependency-free so the
				/// pure stages (Configuration, Planning) can use them without pulling in
				/// the engine or the filesystem.
				/// </summary>
				/// <remarks>
				/// Ported from the same-named behaviour scattered across the C# reference
				/// --- <c>string.Trim()</c>, <c>ValueParsers.FoldToken</c>,
				/// <c>File.ReadAllLines</c>'s cross-platform line splitting --- rather than
				/// left to <c>&lt;cctype&gt;</c>/<c>&lt;locale&gt;</c> or a new dependency, so
				/// all four ports agree on what "whitespace" and "case-fold" mean.
				/// </remarks>
				namespace Strings
				{
					/// <summary>
					/// Trims from both ends the whitespace <c>string.Trim()</c> trims, which
					/// is <c>char.IsWhiteSpace</c>: U+0009 to U+000D, U+0085, and the space,
					/// line and paragraph separator categories.
					/// </summary>
					/// <remarks>
					/// Not the six ASCII characters <c>&lt;cctype&gt;</c> would offer, and not
					/// <c>std::isspace</c>, which is locale-sensitive on top of that. The
					/// no-break spaces U+00A0, U+2007 and U+202F are whitespace to the
					/// reference and so are trimmed here; the information separators U+001C
					/// to U+001F are not whitespace to it and so are kept.
					///
					/// The text is UTF-8, so the ends are decoded rather than compared byte
					/// by byte. A byte sequence that is not well-formed UTF-8 is not
					/// whitespace, which stops the trim rather than failing the call.
					/// </remarks>
					std::string trim(const std::string& text);

					/// <summary>
					/// Whether every character is whitespace by the same definition, an
					/// empty string included. This is <c>string.IsNullOrWhiteSpace</c>; C++
					/// has no null string, so only the second half of the name survives.
					/// </summary>
					bool isAllWhiteSpace(const std::string& text);

					/// <summary>
					/// Splits text into lines the way <c>File.ReadAllLines</c> does: CR, LF
					/// and CRLF each end one line, so <c>a\rb</c> is two lines and not one,
					/// and a CRLF file and an LF file of the same content give the same lines.
					/// A terminator at the very end closes the last line rather than starting
					/// an empty one after it, and an empty string yields no lines at all,
					/// matching <c>File.ReadAllLines</c> on a zero-byte file.
					/// </summary>
					std::vector<std::string> splitLines(const std::string& text);

					/// <summary>
					/// ASCII-only case fold: 'A'-'Z' to 'a'-'z', nothing else. Matches
					/// <c>ValueParsers.FoldToken</c> exactly, character for character.
					/// </summary>
					/// <remarks>
					/// Deliberately not <c>std::tolower</c> or any locale-aware fold: a
					/// Turkish locale folds 'I' to a dotless 'ı', which would stop
					/// <c>--to TIFF</c> resolving --- in C++ as much as in C#, Java or
					/// Python. See tests/conformance/README.md, "Values are folded, or they
					/// are verbatim".
					/// </remarks>
					std::string foldAscii(const std::string& text);
				}
			}
		}
	}
}

#endif
