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

#include "Runtime/Strings.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli::Runtime;

TEST_CASE("trim removes the whitespace the reference calls whitespace", "[Strings]")
{
	CHECK(Strings::trim("  hello  ") == "hello");
	CHECK(Strings::trim("\t\r\nhello\n\r\t") == "hello");
	CHECK(Strings::trim("hello") == "hello");
	CHECK(Strings::trim("") == "");
	CHECK(Strings::trim("no trailing") == "no trailing");

	// Beyond ASCII, which is the half a byte-wise trim misses. Written as
	// escapes rather than literals: a non-ASCII character in a source file is a
	// build-configuration bet on MSVC, as this list says elsewhere.
	CHECK(Strings::trim("\xC2\xA0hello\xC2\xA0") == "hello");           // U+00A0 no-break space
	CHECK(Strings::trim("\xE3\x80\x80hello\xE3\x80\x80") == "hello");   // U+3000 ideographic space
	CHECK(Strings::trim("\xE2\x80\x87hello") == "hello");               // U+2007 figure space
	CHECK(Strings::trim("hello\xE2\x80\xA8") == "hello");               // U+2028 line separator
	CHECK(Strings::trim("\xC2\x85hello") == "hello");                   // U+0085 next line
}

TEST_CASE("trim keeps the C0 information separators", "[Strings]")
{
	// U+001C to U+001F are whitespace to Java and Python and not to the
	// reference, so they stay. The other three ports each pin this too.
	CHECK(Strings::trim("\x1C\x1D\x1E\x1F") == "\x1C\x1D\x1E\x1F");
	CHECK(Strings::trim(" \x1Chello\x1F ") == "\x1Chello\x1F");
}

TEST_CASE("trim of nothing but whitespace is empty", "[Strings]")
{
	CHECK(Strings::trim("   ") == "");
	CHECK(Strings::trim("\xC2\xA0\xE3\x80\x80 \t") == "");
}

TEST_CASE("trim leaves interior whitespace alone", "[Strings]")
{
	CHECK(Strings::trim("  a  b  ") == "a  b");
	CHECK(Strings::trim(" a\xC2\xA0\x62 ") == "a\xC2\xA0\x62");
}

TEST_CASE("trim leaves malformed UTF-8 alone", "[Strings]")
{
	// Not whitespace, so the trim stops rather than eating bytes that are not a
	// character. The reference never sees these: its strings are UTF-16 and
	// already decoded.
	CHECK(Strings::trim("\xFFhello\xFF") == "\xFFhello\xFF");
	CHECK(Strings::trim("\xC2") == "\xC2");                 // a lead byte with nothing after it
	CHECK(Strings::trim("\xA0\xA0") == "\xA0\xA0");         // continuation bytes with no lead
	CHECK(Strings::trim("\xC0\xA0") == "\xC0\xA0");         // an overlong U+0020
	CHECK(Strings::trim(" \xC2") == "\xC2");
}

TEST_CASE("isAllWhiteSpace matches IsNullOrWhiteSpace", "[Strings]")
{
	CHECK(Strings::isAllWhiteSpace(""));
	CHECK(Strings::isAllWhiteSpace(" \t\r\n"));
	CHECK(Strings::isAllWhiteSpace("\xC2\xA0"));            // U+00A0 alone is a blank line
	CHECK(Strings::isAllWhiteSpace("\xE3\x80\x80 \xC2\x85"));
	CHECK_FALSE(Strings::isAllWhiteSpace("a"));
	CHECK_FALSE(Strings::isAllWhiteSpace(" a "));
	CHECK_FALSE(Strings::isAllWhiteSpace("\x1C"));          // an information separator is not
	CHECK_FALSE(Strings::isAllWhiteSpace("\xFF"));
}

TEST_CASE("splitLines on an empty string yields no lines", "[Strings]")
{
	CHECK(Strings::splitLines("").empty());
}

TEST_CASE("splitLines matches File.ReadAllLines on LF text", "[Strings]")
{
	CHECK(Strings::splitLines("a\nb") == std::vector<std::string>{ "a", "b" });
	CHECK(Strings::splitLines("a\nb\n") == std::vector<std::string>{ "a", "b" });
	CHECK(Strings::splitLines("a\nb\n\n") == std::vector<std::string>{ "a", "b", "" });
	CHECK(Strings::splitLines("\n") == std::vector<std::string>{ "" });
}

TEST_CASE("splitLines matches File.ReadAllLines on CRLF text", "[Strings]")
{
	CHECK(Strings::splitLines("a\r\nb\r\n") == std::vector<std::string>{ "a", "b" });
	CHECK(Strings::splitLines("a\r\nb") == std::vector<std::string>{ "a", "b" });
}

TEST_CASE("splitLines matches File.ReadAllLines on a bare CR", "[Strings]")
{
	// Every expectation here was measured against File.ReadAllLines rather than
	// assumed: StreamReader.ReadLine ends a line on CR, LF or CRLF, so a lone CR is
	// a terminator like any other. Splitting on LF alone made a CR-only file one
	// line in C++ and several in C# -- the same bytes producing a different number
	// of inputs from --files-from, or a different set of passwords.
	CHECK(Strings::splitLines("a\rb") == std::vector<std::string>{ "a", "b" });
	CHECK(Strings::splitLines("a\rb\r") == std::vector<std::string>{ "a", "b" });
	CHECK(Strings::splitLines("\r") == std::vector<std::string>{ "" });

	// A CR immediately followed by another CR is two terminators, so the line
	// between them is empty and kept -- unlike CRLF, which is one.
	CHECK(Strings::splitLines("a\r\rb") == std::vector<std::string>{ "a", "", "b" });

	// Mixed endings in one file, which is what a hand-edited list often is.
	CHECK(Strings::splitLines("a\r\nb\rc") == std::vector<std::string>{ "a", "b", "c" });
}

TEST_CASE("splitLines never drops a genuinely blank interior line", "[Strings]")
{
	CHECK(Strings::splitLines("a\n\nb") == std::vector<std::string>{ "a", "", "b" });
}

TEST_CASE("foldAscii lowercases only ASCII letters", "[Strings]")
{
	CHECK(Strings::foldAscii("TIFF") == "tiff");
	CHECK(Strings::foldAscii("Pdf") == "pdf");
	CHECK(Strings::foldAscii("already-lower") == "already-lower");
	CHECK(Strings::foldAscii("MiXeD_123") == "mixed_123");
}

TEST_CASE("foldAscii leaves non-ASCII bytes untouched, unlike a locale-aware fold", "[Strings]")
{
	// U+0130 LATIN CAPITAL LETTER I WITH DOT ABOVE, UTF-8 encoded: the character
	// a Turkish-locale std::tolower would map 'I' to. foldAscii must pass its
	// bytes through unchanged rather than transform them, which is exactly the
	// guarantee that keeps --to TIFF resolving under any locale --- see
	// tests/conformance/README.md.
	const std::string turkishDottedI = "\xC4\xB0";
	CHECK(Strings::foldAscii(turkishDottedI) == turkishDottedI);
}
