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

#include "Strings.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				namespace Strings
				{
					namespace
					{
						/// <summary>
						/// Whether the reference would call this code point whitespace.
						/// </summary>
						/// <remarks>
						/// <c>char.IsWhiteSpace</c>, written out: U+0009 to U+000D, U+0085,
						/// and the Unicode space, line and paragraph separator categories.
						/// Java reaches the same set through <c>Character.getType</c> and
						/// Python by subtracting from <c>str.isspace()</c>; C++17 has no
						/// Unicode tables, so the set is listed.
						///
						/// Listing it means it does not follow Unicode on its own. These
						/// code points have been stable for a long time and a new separator
						/// is rare, but if one is ever added this is the line that has to
						/// change, where the other three ports would pick it up with their
						/// runtime.
						/// </remarks>
						bool isWhiteSpace(char32_t codePoint)
						{
							return (codePoint >= 0x0009 && codePoint <= 0x000D)   // tab, LF, VT, FF, CR
								|| codePoint == 0x0020                            // space
								|| codePoint == 0x0085                            // next line
								|| codePoint == 0x00A0                            // no-break space
								|| codePoint == 0x1680                            // ogham space mark
								|| (codePoint >= 0x2000 && codePoint <= 0x200A)   // en quad .. hair space
								|| codePoint == 0x2028                            // line separator
								|| codePoint == 0x2029                            // paragraph separator
								|| codePoint == 0x202F                            // narrow no-break space
								|| codePoint == 0x205F                            // medium mathematical space
								|| codePoint == 0x3000;                           // ideographic space
						}

						/// <summary>
						/// Decodes the UTF-8 sequence beginning at <paramref name="at" />,
						/// returning false for anything that is not well-formed.
						/// </summary>
						/// <remarks>
						/// Overlong forms are rejected rather than decoded. Without that
						/// check <c>C0 A0</c> would read as U+0020 and be trimmed, where
						/// .NET's decoder sees two invalid bytes and nothing to trim.
						/// </remarks>
						bool decodeUtf8(const std::string& text, size_t at, char32_t& codePoint, size_t& length)
						{
							const unsigned char lead = static_cast<unsigned char>(text[at]);
							if (lead < 0x80)
							{
								codePoint = lead;
								length = 1;
								return true;
							}

							size_t continuations;
							char32_t value;
							char32_t smallest;
							if ((lead & 0xE0) == 0xC0)
							{
								continuations = 1;
								value = lead & 0x1Fu;
								smallest = 0x80;
							}
							else if ((lead & 0xF0) == 0xE0)
							{
								continuations = 2;
								value = lead & 0x0Fu;
								smallest = 0x800;
							}
							else if ((lead & 0xF8) == 0xF0)
							{
								continuations = 3;
								value = lead & 0x07u;
								smallest = 0x10000;
							}
							else
							{
								return false;   // a continuation byte, or 0xF8 and above
							}

							if (text.size() - at <= continuations)
								return false;

							for (size_t i = 1; i <= continuations; ++i)
							{
								const unsigned char next = static_cast<unsigned char>(text[at + i]);
								if ((next & 0xC0) != 0x80)
									return false;
								value = (value << 6) | (next & 0x3Fu);
							}

							if (value < smallest)
								return false;

							codePoint = value;
							length = continuations + 1;
							return true;
						}

						/// <summary>
						/// Where the code point ending at <paramref name="end" /> begins, or
						/// <paramref name="end" /> when the bytes before it are not one.
						/// </summary>
						/// <remarks>
						/// UTF-8 is self-synchronising: a continuation byte is 10xxxxxx and
						/// nothing else is, so stepping back over at most three of them
						/// finds the lead. The step limit is what stops a run of stray
						/// continuation bytes walking to the front of the string.
						/// </remarks>
						size_t startOfLast(const std::string& text, size_t begin, size_t end)
						{
							for (size_t steps = 0; steps < 4 && end - steps > begin; ++steps)
							{
								const size_t at = end - steps - 1;
								if ((static_cast<unsigned char>(text[at]) & 0xC0) != 0x80)
									return at;
							}
							return end;
						}

						/// <summary>The whitespace sequence at <paramref name="at" />, or 0.</summary>
						size_t whiteSpaceAt(const std::string& text, size_t at)
						{
							char32_t codePoint = 0;
							size_t length = 0;
							if (!decodeUtf8(text, at, codePoint, length) || !isWhiteSpace(codePoint))
								return 0;
							return length;
						}
					}

					std::string trim(const std::string& text)
					{
						size_t begin = 0;
						while (begin < text.size())
						{
							const size_t length = whiteSpaceAt(text, begin);
							if (length == 0)
								break;
							begin += length;
						}

						size_t end = text.size();
						while (end > begin)
						{
							const size_t at = startOfLast(text, begin, end);
							// at + length != end rejects a lead byte whose sequence runs
							// past the end, which would otherwise trim bytes that are not
							// a character at all.
							if (at == end || whiteSpaceAt(text, at) != end - at)
								break;
							end = at;
						}

						return text.substr(begin, end - begin);
					}

					bool isAllWhiteSpace(const std::string& text)
					{
						size_t at = 0;
						while (at < text.size())
						{
							const size_t length = whiteSpaceAt(text, at);
							if (length == 0)
								return false;
							at += length;
						}
						return true;
					}

					std::vector<std::string> splitLines(const std::string& text)
					{
						// File.ReadAllLines on a zero-byte file returns no lines at all,
						// not one empty line --- the general loop below would produce the
						// latter, so the empty case is handled on its own.
						if (text.empty())
							return {};

						// CR, LF and CRLF are each one terminator, which is what
						// StreamReader.ReadLine does and therefore what File.ReadAllLines does.
						// Splitting on LF alone and merely trimming a preceding CR handled CRLF but
						// not a lone CR, so a CR-only file --- a --files-from list or a password file
						// written by anything that emits bare CRs --- came back as a single line in
						// C++ where C# saw several. Same bytes, different number of inputs.
						std::vector<std::string> lines;
						size_t start = 0;

						while (start <= text.size())
						{
							size_t stop = text.find_first_of("\r\n", start);
							if (stop == std::string::npos)
							{
								lines.push_back(text.substr(start));
								break;
							}

							lines.push_back(text.substr(start, stop - start));

							// CRLF is one terminator, not an empty line between two.
							start = (text[stop] == '\r' && stop + 1 < text.size() && text[stop + 1] == '\n') ? stop + 2 : stop + 1;
						}

						// A trailing terminator ends the last real line rather than starting an empty
						// one after it --- File.ReadAllLines agrees, so a fixture file saved with or
						// without a final newline parses to the same lines.
						if (!lines.empty() && lines.back().empty() && (text.back() == '\r' || text.back() == '\n'))
							lines.pop_back();

						return lines;
					}

					std::string foldAscii(const std::string& text)
					{
						std::string result = text;
						for (char& c : result)
						{
							if (c >= 'A' && c <= 'Z')
								c = static_cast<char>(c - 'A' + 'a');
						}
						return result;
					}
				}
			}
		}
	}
}
