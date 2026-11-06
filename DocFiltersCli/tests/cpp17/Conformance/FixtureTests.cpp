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

// ---------------------------------------------------------------------------
//  Direct tests of the reader itself. The C# reference has no equivalent
//  file --- Fixture.cs is exercised only indirectly, through
//  PlanCorpusTests --- but every later phase of this port depends on this
//  reader agreeing with tests/conformance/README.md's own description of the
//  format, so it is worth pinning here before anything is built on top of it.
// ---------------------------------------------------------------------------

#include "Fixture.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli::Tests::Conformance;

namespace
{
	std::vector<FixtureCase> parseText(std::initializer_list<std::string> lines)
	{
		return Fixture::parse("test.txt", std::vector<std::string>(lines));
	}
}

TEST_CASE("a case collects its expectations in order", "[Fixture]")
{
	auto cases = parseText({ "$ convert --to pdf a.docx", "= command   convert", "= exit      0" });

	REQUIRE(cases.size() == 1);
	CHECK(cases[0].subject == "convert --to pdf a.docx");
	CHECK(cases[0].get("command") == "convert");
	CHECK(cases[0].get("exit") == "0");
	CHECK(cases[0].file == "test.txt");
	CHECK(cases[0].line == 1);
}

TEST_CASE("comments and blank lines are ignored", "[Fixture]")
{
	auto cases = parseText({ "# a leading comment", "", "$ formats", "# a comment inside a case", "= exit  0" });

	REQUIRE(cases.size() == 1);
	CHECK(cases[0].get("exit") == "0");
}

TEST_CASE("a blank line ends the current case", "[Fixture]")
{
	// Without this, a stray '=' after the gap would silently attach to the
	// case above it rather than being rejected --- see Fixture.h's remarks.
	CHECK_THROWS_AS(parseText({ "$ formats", "= exit  0", "", "= exit  1" }), FixtureFormatError);
}

TEST_CASE("multiple cases are separated cleanly", "[Fixture]")
{
	auto cases = parseText({ "$ one", "= exit  0", "", "$ two", "= exit  1" });

	REQUIRE(cases.size() == 2);
	CHECK(cases[0].subject == "one");
	CHECK(cases[0].get("exit") == "0");
	CHECK(cases[1].subject == "two");
	CHECK(cases[1].get("exit") == "1");
	CHECK(cases[1].line == 4);
}

TEST_CASE("an empty subject is allowed", "[Fixture]")
{
	// formats.txt uses one to assert that an empty --to is rejected.
	auto cases = parseText({ "$", "= error E_UNKNOWN_FORMAT" });

	REQUIRE(cases.size() == 1);
	CHECK(cases[0].subject.empty());
}

TEST_CASE("a case keeps what it is given apart from what it expects", "[Fixture]")
{
	auto cases = parseText({ "$ convert corpus", "< tree  a.docx b.docx", "< tree  c.docx", "= inputs a.docx, b.docx, c.docx" });

	REQUIRE(cases.size() == 1);
	CHECK(cases[0].givenAll("tree") == std::vector<std::string>{ "a.docx b.docx", "c.docx" });
	CHECK(cases[0].get("inputs") == "a.docx, b.docx, c.docx");
	CHECK(cases[0].hasGiven("tree"));
	CHECK_FALSE(cases[0].hasGiven("link"));
}

TEST_CASE("what a case is given comes before what it expects", "[Fixture]")
{
	CHECK_THROWS_AS(parseText({ "$ convert corpus", "= exit  0", "< tree  a.docx" }), FixtureFormatError);
}

TEST_CASE("a given line before any case is an error", "[Fixture]")
{
	CHECK_THROWS_AS(parseText({ "< tree  a.docx" }), FixtureFormatError);
}

TEST_CASE("an expectation before any case is an error", "[Fixture]")
{
	CHECK_THROWS_AS(parseText({ "= exit  0" }), FixtureFormatError);
}

TEST_CASE("an unrecognised line is an error", "[Fixture]")
{
	CHECK_THROWS_AS(parseText({ "$ formats", "? not a real prefix" }), FixtureFormatError);
}

TEST_CASE("get trims, getRaw keeps the value's own padding", "[Fixture]")
{
	auto cases = parseText({ "$ x", "= json    \"a\": 1" });

	REQUIRE(cases.size() == 1);
	CHECK(cases[0].get("json") == "\"a\": 1");
	CHECK(cases[0].getRaw("json") == "   \"a\": 1");
}

TEST_CASE("an absent key returns nullopt, not an empty string", "[Fixture]")
{
	auto cases = parseText({ "$ x", "= exit  0" });

	REQUIRE(cases.size() == 1);
	CHECK_FALSE(cases[0].get("error").has_value());
	CHECK_FALSE(cases[0].error().has_value());
	CHECK_FALSE(cases[0].has("error"));
}

TEST_CASE("an error expectation is recognised", "[Fixture]")
{
	auto cases = parseText({ "$ x", "= error E_UNKNOWN_FORMAT", "= exit  2" });

	REQUIRE(cases.size() == 1);
	REQUIRE(cases[0].error().has_value());
	CHECK(*cases[0].error() == "E_UNKNOWN_FORMAT");
}

TEST_CASE("a repeated key keeps every value", "[Fixture]")
{
	auto cases = parseText({ "$ x", "= target out/a.pdf", "= target out/b.pdf" });

	REQUIRE(cases.size() == 1);
	CHECK(cases[0].getAll("target") == std::vector<std::string>{ "out/a.pdf", "out/b.pdf" });
}

TEST_CASE("yes and no expectations parse as booleans", "[Fixture]")
{
	auto cases = parseText({ "$ x", "= single-file yes", "= combine no" });

	REQUIRE(cases.size() == 1);
	CHECK(cases[0].getBool("single-file") == true);
	CHECK(cases[0].getBool("combine") == false);
}

TEST_CASE("anything other than yes or no is rejected", "[Fixture]")
{
	auto cases = parseText({ "$ x", "= hidef maybe" });

	REQUIRE(cases.size() == 1);
	CHECK_THROWS_AS(cases[0].getBool("hidef"), FixtureFormatError);
	CHECK_THROWS_AS(cases[0].getBool("missing"), FixtureFormatError);
}

TEST_CASE("a failure message names the file the line and the whole case", "[Fixture]")
{
	auto cases = parseText({ "$ convert --to pdf a.docx", "< tree  a.docx", "= command convert" });

	REQUIRE(cases.size() == 1);
	std::string description = cases[0].describe("something went wrong");

	CHECK(description.find("test.txt:1") != std::string::npos);
	CHECK(description.find("$ convert --to pdf a.docx") != std::string::npos);
	CHECK(description.find("something went wrong") != std::string::npos);
	CHECK(description.find("< tree  a.docx") != std::string::npos);
	CHECK(description.find("= command convert") != std::string::npos);
}

TEST_CASE("load reads a real fixture from tests/conformance", "[Fixture]")
{
	auto cases = Fixture::load("formats.txt");

	CHECK_FALSE(cases.empty());

	bool foundDocpack = false;
	for (const auto& fixtureCase : cases)
	{
		if (fixtureCase.subject == "docpack")
		{
			foundDocpack = true;
			CHECK(fixtureCase.get("kind") == "docpack");
			CHECK(fixtureCase.getBool("single-file"));
		}
	}
	CHECK(foundDocpack);
}

TEST_CASE("load throws FixtureNotFoundError on a fixture that does not exist", "[Fixture]")
{
	CHECK_THROWS_AS(Fixture::load("no-such-fixture.txt"), FixtureNotFoundError);
}
