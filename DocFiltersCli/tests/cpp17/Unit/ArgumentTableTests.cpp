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
//  The command line surface, checked against the shared arguments.txt
//  fixture.
//
//  The fixture is written from the CLI design document, not dumped from any
//  implementation. That direction is the point: a file generated from the C#
//  build could only prove the other three agree with C#, whereas this proves
//  all four agree with the specification --- so an argument nobody
//  implemented still fails.
//
//  This asks "does the right set of arguments exist, with the right arity?".
//  Whether each one does what it says is CommandLineApp's job, ported once
//  that class's forward dependencies (Commands/, Execution/, Engine/) exist.
// ---------------------------------------------------------------------------

#include "Cli/ArgumentTable.h"
#include "Conformance/Fixture.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

using namespace Hyland::DocumentFilters::Cli::Cli;
using namespace Hyland::DocumentFilters::Cli::Tests::Conformance;

namespace
{
	/// <summary>One <c>= argument template arity</c> row.</summary>
	struct ArgumentRow
	{
		std::string templateSpec;
		ArgumentArity arity;
		std::string longName;
		std::optional<std::string> shortName;

		static ArgumentArity parseArity(const std::string& text, const FixtureCase& testCase)
		{
			if (text == "none")
				return ArgumentArity::NoValue;
			if (text == "single")
				return ArgumentArity::SingleValue;
			if (text == "multiple")
				return ArgumentArity::MultipleValue;

			throw FixtureFormatError(testCase.describe("unknown arity '" + text + "'"));
		}

		static ArgumentRow parse(const std::string& row, const FixtureCase& testCase)
		{
			std::vector<std::string> parts;
			size_t i = 0;
			while (i < row.size())
			{
				while (i < row.size() && (row[i] == ' ' || row[i] == '\t'))
					++i;
				size_t start = i;
				while (i < row.size() && row[i] != ' ' && row[i] != '\t')
					++i;
				if (i > start)
					parts.push_back(row.substr(start, i - start));
			}

			if (parts.size() != 2)
				throw FixtureFormatError(testCase.describe("argument row needs 'template arity': " + row));

			ArgumentRow result;
			result.templateSpec = parts[0];
			result.arity = parseArity(parts[1], testCase);

			size_t start = 0;
			while (true)
			{
				size_t bar = parts[0].find('|', start);
				std::string form = parts[0].substr(start, bar == std::string::npos ? std::string::npos : bar - start);

				if (form.rfind("--", 0) == 0)
					result.longName = form.substr(2);
				else if (form.rfind("-", 0) == 0)
					result.shortName = form.substr(1);

				if (bar == std::string::npos)
					break;
				start = bar + 1;
			}

			// An argument written only in short form, such as -x, is named by that form.
			if (result.longName.empty())
				result.longName = result.shortName.value();

			return result;
		}
	};

	const std::vector<std::string>& sevenProcessingCommands() { return ArgumentTable::commands(); }
}

TEST_CASE("command declares exactly the arguments in the shared fixture", "[ArgumentTable][Fixture]")
{
	for (const FixtureCase& testCase : Fixture::load("arguments.txt"))
	{
		const std::string& command = testCase.subject;

		std::vector<ArgumentRow> expected;
		for (const std::string& row : testCase.getAll("argument"))
			expected.push_back(ArgumentRow::parse(row, testCase));

		std::map<std::string, std::shared_ptr<const IArgumentDescriptor>> declared;
		for (const std::shared_ptr<const IArgumentDescriptor>& descriptor : ArgumentTable::describeCommand(command))
			declared[descriptor->longName()] = descriptor;

		// Nothing missing.
		for (const ArgumentRow& row : expected)
		{
			CHECK(declared.count(row.longName) > 0);
			if (declared.count(row.longName) == 0)
				continue;

			const IArgumentDescriptor& actual = *declared[row.longName];

			CHECK(actual.arity() == row.arity);
			CHECK((actual.shortName().value_or("")) == (row.shortName.value_or("")));
		}

		// And nothing extra: an undocumented argument is drift just as surely as a
		// missing one, and would never be implemented in the other three.
		std::set<std::string> expectedNames;
		for (const ArgumentRow& row : expected)
			expectedNames.insert(row.longName);

		for (const std::shared_ptr<const IArgumentDescriptor>& descriptor : ArgumentTable::describeCommand(command))
			CHECK(expectedNames.count(descriptor->longName()) > 0);
	}
}

// ------------------------------------------- behaviour the fixture cannot state

TEST_CASE("the fixture covers every processing command", "[ArgumentTable]")
{
	std::set<std::string> covered;
	for (const FixtureCase& testCase : Fixture::load("arguments.txt"))
		covered.insert(testCase.subject);

	for (const std::string& command : ArgumentTable::commands())
		CHECK(covered.count(command) > 0);
}

TEST_CASE("the fixture lists no command the tool does not have", "[ArgumentTable]")
{
	const std::vector<std::string>& commands = ArgumentTable::commands();
	for (const FixtureCase& testCase : Fixture::load("arguments.txt"))
	{
		bool known = std::find(commands.begin(), commands.end(), testCase.subject) != commands.end();
		CHECK(known);
	}
}

TEST_CASE("the seven processing commands are declared in help order", "[ArgumentTable]")
{
	std::vector<std::string> expected = { "convert", "extract", "list", "identify", "meta", "formats", "options" };
	CHECK(ArgumentTable::commands() == expected);
}

TEST_CASE("help and version are verbs but not processing commands", "[ArgumentTable]")
{
	// They accept no shared options and process no documents, so they are
	// dispatchable without appearing in the applicability table.
	const std::vector<std::string>& verbs = ArgumentTable::allVerbs();
	const std::vector<std::string>& commands = ArgumentTable::commands();

	CHECK(std::find(verbs.begin(), verbs.end(), "help") != verbs.end());
	CHECK(std::find(verbs.begin(), verbs.end(), "version") != verbs.end());
	CHECK(std::find(commands.begin(), commands.end(), "help") == commands.end());
	CHECK(std::find(commands.begin(), commands.end(), "version") == commands.end());
}

TEST_CASE("no command declares the same argument twice", "[ArgumentTable]")
{
	for (const std::string& command : sevenProcessingCommands())
	{
		std::set<std::string> seen;
		for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
			CHECK(seen.insert(argument->longName()).second);
	}
}

TEST_CASE("no command reuses a short name", "[ArgumentTable]")
{
	for (const std::string& command : sevenProcessingCommands())
	{
		std::set<std::string> seen;
		for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
		{
			if (!argument->shortName().has_value())
				continue;

			CHECK(seen.insert(*argument->shortName()).second);
		}
	}
}

TEST_CASE("every argument has help text", "[ArgumentTable]")
{
	// Prose, so it is not in the fixture --- help text may differ in wording
	// between the four implementations. It may not be absent.
	for (const std::string& command : sevenProcessingCommands())
	{
		for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
			CHECK_FALSE(argument->description().empty());
	}
}

TEST_CASE("a shared argument keeps the same arity on every command that accepts it", "[ArgumentTable]")
{
	// The defining property of a shared option, and true without exception:
	// --glob cannot be repeatable on convert and single-valued on list, and
	// --regex cannot be a path pattern on one command and a mode switch on
	// another.
	std::map<std::string, ArgumentArity> arities;

	for (const std::string& command : ArgumentTable::commands())
	{
		for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
		{
			auto existing = arities.find(argument->longName());
			if (existing != arities.end())
				CHECK(existing->second == argument->arity());
			else
				arities[argument->longName()] = argument->arity();
		}
	}
}

TEST_CASE("extract accepts the shared regex like every other walking command", "[ArgumentTable]")
{
	// extract walks directories like the others, so it selects paths the same
	// way: -g/--glob and --regex, with --include/--exclude picking subfiles.
	std::set<std::string> declared;
	for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand("extract"))
		declared.insert(argument->longName());

	CHECK(declared.count("regex") > 0);
	CHECK(declared.count("include") > 0);
	CHECK(declared.count("filter") == 0);
}

TEST_CASE("include and exclude are a matched pair on extract", "[ArgumentTable]")
{
	// A matched pair: same arity, opposite direction.
	std::optional<ArgumentArity> includeArity;
	std::optional<ArgumentArity> excludeArity;

	for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand("extract"))
	{
		if (argument->longName() == "include")
			includeArity = argument->arity();
		else if (argument->longName() == "exclude")
			excludeArity = argument->arity();
	}

	REQUIRE(includeArity.has_value());
	REQUIRE(excludeArity.has_value());
	CHECK(*excludeArity == *includeArity);
}
