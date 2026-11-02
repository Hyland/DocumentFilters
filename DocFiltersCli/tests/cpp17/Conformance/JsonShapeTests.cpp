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

// The --json shape of every command that has one. These five shapes are the
// tool's machine-readable interface, so a renamed key or a restructured
// array breaks whatever is parsing it.
//
// Driven through CliHarness rather than the report classes, so the
// assertion covers the path a user actually takes --- argv, dispatch, the
// command, JsonWriter --- and the engine answers from a script, which is
// what lets the expected document be written out in full rather than
// described.

#include "Cli/CliHarness.h"
#include "Engine/IDocumentFiltersEngine.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <ctime>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Cli::CliHarness;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeSubFile;

namespace
{
	std::string json(CliHarness& cli, const std::vector<std::string>& args)
	{
		REQUIRE(cli.run(args) == Runtime::ExitCode::success);
		REQUIRE(cli.errors().empty());

		// Raw, not the line list: a JSON report is built as one document and
		// written in a single call, so line capture misses it entirely.
		std::string output = cli.output();
		std::string normalised;
		for (char ch : output)
		{
			if (ch != '\r')
				normalised += ch;
		}
		while (!normalised.empty() && normalised.back() == '\n')
			normalised.pop_back();
		return normalised;
	}

	std::chrono::system_clock::time_point utc(int year, int month, int day, int hour, int minute, int second)
	{
		std::tm tm {};
		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = minute;
		tm.tm_sec = second;
#if defined(_WIN32)
		return std::chrono::system_clock::from_time_t(_mkgmtime(&tm));
#else
		return std::chrono::system_clock::from_time_t(timegm(&tm));
#endif
	}
}

TEST_CASE("identify reports an array of path format mime and name", "[JsonShape]")
{
	CliHarness cli;
	cli.withDocument("sample.docx");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> document = cli.engine.addDocument("sample.docx");
	document->fileType = 254;
	document->mimeType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	document->formatName = "Microsoft Word 2007";

	std::string expected = "[\n"
							"  {\n"
							"    \"path\": \"sample.docx\",\n"
							"    \"format\": 254,\n"
							"    \"mime\": \"application/vnd.openxmlformats-officedocument.wordprocessingml.document\",\n"
							"    \"name\": \"Microsoft Word 2007\"\n"
							"  }\n"
							"]";

	CHECK(json(cli, { "identify", "--json", "sample.docx" }) == expected);
}

TEST_CASE("meta reports metadata as a nested object keyed by field name", "[JsonShape]")
{
	// Nested rather than flat, and every value a string: the engine reports
	// metadata as text, so a port that guessed at numbers would disagree
	// about "Pages".
	CliHarness cli;
	cli.withDocument("sample.docx", "\x0F" "Author: Jane Smith\x0E" "Title: Quarterly Report\x0E" "Pages: 12\x10");

	std::string expected = "[\n"
							"  {\n"
							"    \"path\": \"sample.docx\",\n"
							"    \"metadata\": {\n"
							"      \"Author\": \"Jane Smith\",\n"
							"      \"Title\": \"Quarterly Report\",\n"
							"      \"Pages\": \"12\"\n"
							"    }\n"
							"  }\n"
							"]";

	CHECK(json(cli, { "meta", "--json", "sample.docx" }) == expected);
}

TEST_CASE("list reports subfiles as an array inside each container", "[JsonShape]")
{
	CliHarness cli;
	cli.withDocument("bundle.zip");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> container = cli.engine.addDocument("bundle.zip");
	auto readme = std::make_shared<Tests::Fakes::FakeSubFileState>();
	readme->name = "readme.txt";
	readme->id = "0001";
	readme->size = 4096;
	readme->modified = utc(2026, 1, 15, 9, 30, 0);
	container->subFileEntries.push_back(readme);

	std::string expected = "[\n"
							"  {\n"
							"    \"path\": \"bundle.zip\",\n"
							"    \"subfiles\": [\n"
							"      {\n"
							"        \"id\": \"0001\",\n"
							"        \"name\": \"readme.txt\",\n"
							"        \"size\": 4096,\n"
							"        \"modified\": \"2026-01-15T09:30:00Z\",\n"
							"        \"depth\": 1\n"
							"      }\n"
							"    ]\n"
							"  }\n"
							"]";

	CHECK(json(cli, { "list", "--json", "bundle.zip" }) == expected);
}

TEST_CASE("formats reports a flat array of id ext and name", "[JsonShape]")
{
	CliHarness cli;
	cli.engine.formats.push_back(Engine::EngineFormat{ 254, "docx", "Microsoft Word 2007" });
	cli.engine.formats.push_back(Engine::EngineFormat{ 112, "doc", "Microsoft Word for Windows" });

	std::string expected = "[\n"
							"  {\n"
							"    \"id\": 254,\n"
							"    \"ext\": \"docx\",\n"
							"    \"name\": \"Microsoft Word 2007\"\n"
							"  },\n"
							"  {\n"
							"    \"id\": 112,\n"
							"    \"ext\": \"doc\",\n"
							"    \"name\": \"Microsoft Word for Windows\"\n"
							"  }\n"
							"]";

	CHECK(json(cli, { "formats", "--json", "--filter", "word" }) == expected);
}

TEST_CASE("options reports name type default appliesTo and description", "[JsonShape]")
{
	// appliesTo is the one camelCase key in the whole surface --- everything
	// else is a single lowercase word, so it is easy to port as "applies-to"
	// or "appliesto" by accident.
	CliHarness cli;
	Engine::EngineOption option;
	option.displayName = "BW_MODE";
	option.type = "int";
	option.defaultValue = "0";
	option.flags = 0x2; // canvas; 0x1 is document, and both join as "document,canvas"
	option.description = "Black and white rendering mode";
	cli.engine.options.push_back(option);

	std::string expected = "[\n"
							"  {\n"
							"    \"name\": \"BW_MODE\",\n"
							"    \"type\": \"int\",\n"
							"    \"default\": \"0\",\n"
							"    \"appliesTo\": \"canvas\",\n"
							"    \"description\": \"Black and white rendering mode\"\n"
							"  }\n"
							"]";

	CHECK(json(cli, { "options", "--json", "--filter", "BW_MODE" }) == expected);
}

TEST_CASE("every json command emits one document even with nothing to report", "[JsonShape]")
{
	// An empty array, not empty output: whatever is parsing this should not
	// have to special-case "no results" as a parse failure.
	CliHarness cli;
	CHECK(json(cli, { "formats", "--json", "--filter", "nothing-matches-this" }) == "[]");

	CliHarness other;
	CHECK(json(other, { "options", "--json", "--filter", "nothing-matches-this" }) == "[]");
}
