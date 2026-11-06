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

// The ISYS control codes the extractor emits, and what --to text does with
// them. Ported from doc2text's processOutput, which is the only
// implementation of this table there is. It has to live in the tool: a
// text-mode open resolves to the same reader whether IGR_FORMAT_TEXT is named
// or omitted, so no open flag can switch it.

#include "Execution/TextConversion.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli::Execution;

TEST_CASE("ordinary text is untouched", "[TextConversion]")
{
	CHECK(TextConversion::stripControlCodes("Hello, world.") == "Hello, world.");
}

TEST_CASE("start of heading becomes a space", "[TextConversion]")
{
	CHECK(TextConversion::stripControlCodes("Title\x01" "Body") == "Title Body");
}

TEST_CASE("styling marks are dropped", "[TextConversion]")
{
	for (char code : { '\x02', '\x03', '\x04', '\x05', '\x06', '\x0C', '\x10' })
		CHECK(TextConversion::stripControlCodes(std::string("a") + code + "b") == "ab");
}

TEST_CASE("paragraph and start of meta become newlines", "[TextConversion]")
{
	CHECK(TextConversion::stripControlCodes("a\x0E" "b") == "a\nb");
	CHECK(TextConversion::stripControlCodes("a\x0F" "b") == "a\nb");
}

TEST_CASE("flow html keeps its paragraph marks as nothing", "[TextConversion]")
{
	// The markup already carries the structure, so doc2text suppresses the
	// newline when the open flags say HTML; a newline there would only add
	// whitespace between tags.
	CHECK(TextConversion::stripControlCodes("<p>a</p>\x0E<p>b</p>", true) == "<p>a</p><p>b</p>");
}

TEST_CASE("start of meta still breaks a line in flow html", "[TextConversion]")
{
	CHECK(TextConversion::stripControlCodes("a\x0F" "b", true) == "a\nb");
}

TEST_CASE("real newlines and tabs survive", "[TextConversion]")
{
	// Only the ISYS codes go; line endings are the document's, not ours.
	CHECK(TextConversion::stripControlCodes("a\r\nb\tc") == "a\r\nb\tc");
}

TEST_CASE("every code in one string is handled together", "[TextConversion]")
{
	std::string chunk = "\x01H\x02" "e\x03l\x04l\x05o\x06\x0E\x0Fw\x0Co\x10rld";
	CHECK(TextConversion::stripControlCodes(chunk) == " Hello\n\nworld");
}

TEST_CASE("a chunk of nothing but codes strips to nothing", "[TextConversion]")
{
	CHECK(TextConversion::stripControlCodes("\x02\x03\x04\x0C\x10") == "");
	CHECK(TextConversion::stripControlCodes("") == "");
}

TEST_CASE("stripping is safe across a chunk boundary", "[TextConversion]")
{
	// GetText hands back 4096 characters at a time, so a code can land at
	// either edge. Every one is a single character that does not depend on
	// its neighbours, which is what makes stripping per chunk equivalent to
	// stripping the whole stream.
	const std::string whole = "one\x0Etwo\x0Ethree";

	for (std::size_t split = 0; split <= whole.size(); split++)
	{
		std::string joined = TextConversion::stripControlCodes(whole.substr(0, split)) + TextConversion::stripControlCodes(whole.substr(split));
		CHECK(joined == TextConversion::stripControlCodes(whole));
	}
}
