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
//  The cross-language contract, executed.
//
//  Each case runs stages 1 to 3 --- parse, bind, plan --- over the filesystem
//  it is given, and compares the canonical plan text against the shared
//  fixture. No document is opened, so these need neither a license nor the
//  native library.
//
//  The C#, Python and Java suites run the same fixture through their own
//  implementations. A case passing in all four is the definition of the four
//  agreeing.
// ---------------------------------------------------------------------------

#include "Cli/ArgumentTable.h"
#include "Cli/CommandLineApp.h"
#include "Commands/ICommand.h"
#include "Fakes/RecordingConsole.h"
#include "PlanCorpus.h"
#include "Runtime/CliError.h"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <set>

using namespace Hyland::DocumentFilters::Cli::Cli;
using namespace Hyland::DocumentFilters::Cli::Commands;
using namespace Hyland::DocumentFilters::Cli::Runtime;
using namespace Hyland::DocumentFilters::Cli::Tests::Conformance;
using namespace Hyland::DocumentFilters::Cli::Tests::Fakes;

namespace
{
	/// <summary>Collapses runs of spaces so fixture alignment is not significant.</summary>
	std::string normalise(std::string text)
	{
		std::string collapsed;
		bool lastWasSpace = false;
		for (char c : text)
		{
			if (c == ' ')
			{
				if (!lastWasSpace)
					collapsed += ' ';
				lastWasSpace = true;
			}
			else
			{
				collapsed += c;
				lastWasSpace = false;
			}
		}

		size_t start = collapsed.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
			return "";
		size_t end = collapsed.find_last_not_of(" \t\r\n");
		return collapsed.substr(start, end - start + 1);
	}

	/// <summary>
	/// Runs stages 1 to 3 --- parse, bind, plan --- against the filesystem the
	/// case is given, with the input walk feeding the planner.
	/// </summary>
	std::string plan(const std::vector<std::string>& args, IFileSystem& fileSystem)
	{
		RecordingConsole console;
		CommandLineApp app(console, fileSystem);
		BoundCommandLine bound = app.bind(args);

		CommandContext context;
		context.sharedOptions = bound.sharedOptions();
		context.console = &console;
		context.fileSystem = &fileSystem;

		return bound.command().describePlan(context);
	}

	/// <summary>
	/// Keys that are not a fixed plan line: <c>exit</c>, which is the process
	/// result, and the target keys, whose count varies by input and by format.
	/// </summary>
	bool isVariableKey(const std::string& key) { return key == "exit" || key == "target" || key == "pattern"; }
}

TEST_CASE("case matches the shared corpus", "[PlanCorpus][Fixture]")
{
	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		std::vector<std::string> args = PlanCorpus::splitArguments(testCase.subject);
		int expectedExit = PlanCorpus::expectedExit(testCase);
		InMemoryFileSystem fileSystem = PlanCorpus::buildFileSystem(testCase);

		if (testCase.error().has_value())
		{
			bool threw = false;
			try
			{
				plan(args, fileSystem);
			}
			catch (const CliError& error)
			{
				threw = true;

				CHECK(error.code() == *testCase.error());
				CHECK(error.exitCode() == expectedExit);

				// Error text is part of the contract, so four ports cannot each
				// phrase the same failure differently and all pass.
				std::optional<std::string> expectedMessage = testCase.get("message");
				CHECK(normalise(error.what()) == normalise(expectedMessage.value_or("")));
			}
			CHECK(threw);
			continue;
		}

		// The whole document, in order --- not a search for fragments. Substring
		// matching would let extra lines, reordered fields and duplicate targets
		// pass in one implementation and fail in another, which is exactly the
		// drift this corpus exists to catch.
		std::vector<std::string> expected;
		for (const std::pair<std::string, std::string>& expectation : testCase.expectations)
		{
			if (expectation.first != "exit")
				expected.push_back(normalise(expectation.first + " " + expectation.second));
		}

		std::string planText = plan(args, fileSystem);

		std::vector<std::string> actual;
		size_t lineStart = 0;
		while (lineStart <= planText.size())
		{
			size_t lineEnd = planText.find('\n', lineStart);
			std::string line = planText.substr(lineStart, lineEnd == std::string::npos ? std::string::npos : lineEnd - lineStart);
			std::string normalised = normalise(line);
			if (!normalised.empty())
				actual.push_back(normalised);

			if (lineEnd == std::string::npos)
				break;
			lineStart = lineEnd + 1;
		}

		CHECK(expected == actual);
	}
}

// ------------------------------------------------------ the corpus itself

TEST_CASE("the corpus is present and not empty", "[PlanCorpus]")
{
	// A missing fixture would otherwise make every case above vanish silently
	// rather than fail.
	CHECK_FALSE(PlanCorpus::load().empty());
}

TEST_CASE("every case states an expected exit code", "[PlanCorpus]")
{
	for (const FixtureCase& testCase : PlanCorpus::load())
		CHECK(testCase.has("exit"));
}

TEST_CASE("every failing case expects a non-zero exit", "[PlanCorpus]")
{
	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (testCase.error().has_value())
			CHECK(PlanCorpus::expectedExit(testCase) != 0);
	}
}

TEST_CASE("every expected error code is one the tool can raise", "[PlanCorpus]")
{
	// Catches a typo in the fixture, which would otherwise look like an
	// implementation bug in whichever language ran it next.
	const std::vector<std::string>& known = ErrorCodes::all();

	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (testCase.error().has_value())
			CHECK(std::find(known.begin(), known.end(), *testCase.error()) != known.end());
	}
}

TEST_CASE("every failing case states its message", "[PlanCorpus]")
{
	// stderr is contract. A case that gives only a code would let the four
	// implementations phrase the same failure four ways and all pass.
	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (testCase.error().has_value())
			CHECK(testCase.has("message"));
	}
}

TEST_CASE("every succeeding case states a complete plan", "[PlanCorpus]")
{
	// A case listing only '= exit 0' would pass against any plan whatsoever.
	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (!testCase.error().has_value())
			CHECK(testCase.has("command"));
	}
}

TEST_CASE("only commands with an output group state a cardinality", "[PlanCorpus]")
{
	// Cardinality answers "who owns the output target". identify declares no
	// output group at all, and formats and options read no inputs and write
	// no files, so for those three the question has no answer and printing
	// one would be inventing it.
	std::set<std::string> owns = { "convert", "extract", "list", "meta" };

	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (testCase.error().has_value())
			continue;

		std::optional<std::string> command = testCase.get("command");
		if (!command.has_value())
			continue;

		CHECK(testCase.has("cardinality") == (owns.count(*command) > 0));
	}
}

TEST_CASE("all cases for one command emit the same line set", "[PlanCorpus]")
{
	// The corpus is compared as a whole document, so which lines appear has to
	// be a property of the command and not of the arguments. Two cases with
	// the same argv and different line sets cannot both be satisfied, in any
	// language.
	std::map<std::string, std::string> shapes;

	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (testCase.error().has_value())
			continue;

		std::optional<std::string> command = testCase.get("command");
		if (!command.has_value())
			continue;

		std::string shape;
		for (const std::pair<std::string, std::string>& expectation : testCase.expectations)
		{
			// target and pattern are the one part of the plan that legitimately
			// varies: one per input for a resolved path, one per input for a
			// per-page pattern, and none at all when the output is streamed to
			// stdout. Their placement is checked separately below.
			if (isVariableKey(expectation.first))
				continue;

			if (!shape.empty())
				shape += ' ';
			shape += expectation.first;
		}

		auto existing = shapes.find(*command);
		if (existing != shapes.end())
			CHECK(existing->second == shape);
		else
			shapes[*command] = shape;
	}
}

TEST_CASE("planner errors declare the inputs they need", "[PlanCorpus]")
{
	// The walk runs before planning, so a case meaning to provoke
	// E_OUTFILE_MULTI_INPUT with no tree gets E_NO_INPUTS instead: it passes
	// for the wrong reason and tests nothing.
	std::set<std::string> afterDiscovery = { ErrorCodes::outFileMultiInput, ErrorCodes::outputCollision, ErrorCodes::outputExists };

	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (testCase.error().has_value() && afterDiscovery.count(*testCase.error()) > 0)
			CHECK(testCase.hasGiven("tree"));
	}
}

TEST_CASE("cases that walk a directory ask for a defined order", "[PlanCorpus]")
{
	// A case that walks a directory sorts, because directory order is
	// otherwise undefined.
	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (testCase.error().has_value())
			continue;

		// More than one file under a common root means the walk chose an order.
		int pathsWithASlash = 0;
		for (const std::string& row : testCase.givenAll("tree"))
		{
			size_t start = 0;
			while (start < row.size())
			{
				size_t end = row.find_first_of(" \t", start);
				std::string path = row.substr(start, end == std::string::npos ? std::string::npos : end - start);
				if (path.find('/') != std::string::npos)
					++pathsWithASlash;

				if (end == std::string::npos)
					break;
				start = end + 1;
				while (start < row.size() && (row[start] == ' ' || row[start] == '\t'))
					++start;
			}
		}

		if (pathsWithASlash <= 1)
			continue;

		std::optional<std::string> walk = testCase.get("walk");
		CHECK(walk.has_value());
		if (walk.has_value())
			CHECK(walk->find("sort=yes") != std::string::npos);
	}
}

TEST_CASE("target lines are contiguous last and of one kind", "[PlanCorpus]")
{
	// The line set test above ignores them, because a streamed run has no
	// target at all. This is their guard: wherever they appear they are
	// contiguous, they sit after every fixed line, and an item carries either
	// a resolved target or a deferred pattern, never both.
	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		if (testCase.error().has_value())
			continue;

		std::vector<std::string> keys;
		for (const std::pair<std::string, std::string>& expectation : testCase.expectations)
		{
			if (expectation.first != "exit")
				keys.push_back(expectation.first);
		}

		auto first = std::find_if(keys.begin(), keys.end(), [](const std::string& k) { return k == "target" || k == "pattern"; });
		if (first == keys.end())
			continue;

		bool allTail = std::all_of(first, keys.end(), [](const std::string& k) { return k == "target" || k == "pattern"; });
		CHECK(allTail);

		std::set<std::string> distinctTail(first, keys.end());
		CHECK(distinctTail.size() == 1);
	}
}

TEST_CASE("no two cases share a subject and a filesystem", "[PlanCorpus]")
{
	// Two cases with the same input and different expectations cannot both
	// pass. The input is argv *and* the tree it runs against: several cases
	// reuse one command line to show what a hidden entry or a symlink
	// changes, and those are different runs rather than a contradiction.
	std::set<std::string> seen;

	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		std::string identity = testCase.subject;
		for (const std::pair<std::string, std::string>& given : testCase.given)
		{
			identity += '\n';
			identity += given.first;
			identity += ' ';
			identity += normalise(given.second);
		}

		CHECK(seen.count(identity) == 0);
		seen.insert(identity);
	}
}

TEST_CASE("the dry-run switch itself is exercised", "[PlanCorpus]")
{
	// Every other case reaches the planner directly, so without this the
	// advertised switch could be missing entirely and the corpus still pass.
	std::vector<FixtureCase> cases = PlanCorpus::load();

	CHECK(std::any_of(
		cases.begin(), cases.end(), [](const FixtureCase& c) { return c.subject.find("--dry-run") != std::string::npos; }));
	CHECK(std::any_of(
		cases.begin(), cases.end(), [](const FixtureCase& c) { return c.subject.find("--print-plan") != std::string::npos; }));
}

TEST_CASE("the corpus covers every command", "[PlanCorpus]")
{
	// Coverage guard: a new verb must arrive with at least one case.
	std::set<std::string> covered;
	for (const FixtureCase& testCase : PlanCorpus::load())
	{
		std::vector<std::string> args = PlanCorpus::splitArguments(testCase.subject);
		if (!args.empty())
			covered.insert(args[0]);
	}

	for (const std::string& command : ArgumentTable::commands())
		CHECK(covered.count(command) > 0);
}

// ----------------------------------------------------- PlanCorpus itself

TEST_CASE("quoted arguments may contain spaces", "[PlanCorpus]")
{
	CHECK(PlanCorpus::splitArguments(R"(convert --options "BW_MODE=3;X=1" a.docx)")
		== std::vector<std::string>{ "convert", "--options", "BW_MODE=3;X=1", "a.docx" });
}
