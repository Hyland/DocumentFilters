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

#include "Runtime/JsonWriter.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli::Runtime;

TEST_CASE("an empty object renders on one line", "[JsonWriter]")
{
	std::string json = JsonWriter().beginObject().endObject().toString();
	CHECK(json == "{}\n");
}

TEST_CASE("properties are indented by two spaces", "[JsonWriter]")
{
	std::string json = JsonWriter().beginObject().property("name", "report.pdf").endObject().toString();
	CHECK(json == "{\n  \"name\": \"report.pdf\"\n}\n");
}

TEST_CASE("keys stay in insertion order and are never sorted", "[JsonWriter]")
{
	// Sorting would be a gratuitous difference from the order the engine
	// reports fields in.
	std::string json = JsonWriter().beginObject().property("zebra", 1).property("apple", 2).endObject().toString();
	CHECK(json.find("zebra") < json.find("apple"));
}

TEST_CASE("nested objects indent cumulatively", "[JsonWriter]")
{
	std::string json = JsonWriter().beginObject().name("inner").beginObject().property("a", 1).endObject().endObject().toString();
	CHECK(json == "{\n  \"inner\": {\n    \"a\": 1\n  }\n}\n");
}

TEST_CASE("arrays render one element per line", "[JsonWriter]")
{
	std::string json = JsonWriter().beginArray().value("a").value("b").endArray().toString();
	CHECK(json == "[\n  \"a\",\n  \"b\"\n]\n");
}

TEST_CASE("an empty array renders on one line", "[JsonWriter]")
{
	CHECK(JsonWriter().beginArray().endArray().toString() == "[]\n");
}

TEST_CASE("booleans and numbers are unquoted", "[JsonWriter]")
{
	std::string json = JsonWriter().beginObject().property("count", 42).property("ok", true).endObject().toString();
	CHECK(json.find("\"count\": 42") != std::string::npos);
	CHECK(json.find("\"ok\": true") != std::string::npos);
}

TEST_CASE("a null string renders as null", "[JsonWriter]")
{
	std::string json = JsonWriter().beginObject().property("modified", std::optional<std::string>()).endObject().toString();
	CHECK(json.find("\"modified\": null") != std::string::npos);
}

TEST_CASE("the mandatory escapes are applied", "[JsonWriter]")
{
	CHECK(JsonWriter::escape("a\"b") == "a\\\"b");
	CHECK(JsonWriter::escape("a\\b") == "a\\\\b");
	CHECK(JsonWriter::escape("a\nb") == "a\\nb");
	CHECK(JsonWriter::escape("a\tb") == "a\\tb");
	CHECK(JsonWriter::escape("a\rb") == "a\\rb");
}

TEST_CASE("control characters use the six character form", "[JsonWriter]")
{
	CHECK(JsonWriter::escape(std::string("\0", 1)) == "\\u0000");
}

TEST_CASE("non ascii is written literally rather than escaped", "[JsonWriter]")
{
	// UTF-8 for "Ünïcödé". Each \xNN is its own string literal --- adjacent
	// literals concatenate at compile time --- because \x is not limited to
	// two hex digits: it keeps consuming hex-digit characters, so \xAF
	// directly followed by the letter c would otherwise be parsed as the
	// single (out of range) escape \xAFc.
	std::string text = "\xC3\x9C" "n" "\xC3\xAF" "c" "\xC3\xB6" "d" "\xC3\xA9";

	// Written literally, or the four implementations produce different bytes
	// for the same document.
	CHECK(JsonWriter::escape(text) == text);
}

TEST_CASE("a forward slash is not escaped", "[JsonWriter]")
{
	// Legal either way in JSON, so it has to be pinned down.
	CHECK(JsonWriter::escape("a/b") == "a/b");
}

TEST_CASE("a surrogate pair is written literally", "[JsonWriter]")
{
	// U+1F600 as UTF-8, one four-byte character. The reference holds it as two
	// UTF-16 units and must not mistake them for two unpaired surrogates; here
	// the question is whether the four bytes survive untouched.
	std::string emoji = "\xF0" "\x9F" "\x98" "\x80";
	CHECK(JsonWriter::escape(emoji) == emoji);
}

TEST_CASE("an unpaired high surrogate is escaped", "[JsonWriter]")
{
	// U+D800 as the three bytes a lenient UTF-16 to UTF-8 conversion writes.
	// Reachable from a Windows filename, which NTFS stores as unvalidated
	// UTF-16. Escaped, all four implementations emit the same six ASCII bytes.
	CHECK(JsonWriter::escape("\xED" "\xA0" "\x80") == "\\ud800");
}

TEST_CASE("an unpaired low surrogate is escaped", "[JsonWriter]")
{
	CHECK(JsonWriter::escape("\xED" "\xBF" "\xBF") == "\\udfff");
}

TEST_CASE("an unpaired surrogate beside a pair leaves the pair intact", "[JsonWriter]")
{
	CHECK(JsonWriter::escape("\xED" "\xA0" "\x80" "\xF0" "\x9F" "\x98" "\x80") == "\\ud800\xF0" "\x9F" "\x98" "\x80");
}

TEST_CASE("an escaped document carries no surrogates of its own", "[JsonWriter]")
{
	std::string json = JsonWriter().beginObject().property("title", "a\xED" "\xA0" "\x80" "b").endObject().toString();

	CHECK(json.find("\"title\": \"a\\ud800b\"") != std::string::npos);
	CHECK(json.find('\xED') == std::string::npos);
}

TEST_CASE("the document ends with exactly one newline", "[JsonWriter]")
{
	std::string json = JsonWriter().beginObject().property("a", 1).endObject().toString();
	CHECK(json.size() >= 2);
	CHECK(json.substr(json.size() - 2) == "}\n");
	CHECK(json.find("\n\n") == std::string::npos);
}

TEST_CASE("line endings are always LF even on windows", "[JsonWriter]")
{
	std::string json = JsonWriter().beginObject().property("a", 1).endObject().toString();
	CHECK(json.find('\r') == std::string::npos);
}

TEST_CASE("an unclosed object is a programming error", "[JsonWriter]")
{
	CHECK_THROWS_AS(JsonWriter().beginObject().toString(), std::logic_error);
}
