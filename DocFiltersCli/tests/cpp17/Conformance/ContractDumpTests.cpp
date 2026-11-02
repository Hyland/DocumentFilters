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
//  --print-contract: the tool's own account of its command line surface.
//
//  What is tested here is that it faithfully reflects the argument table. The
//  surface itself is checked against arguments.txt in ArgumentTableTests.
//
//  The C# suite proves the flat dump is not JSON by handing it to a JSON
//  parser and expecting a parse failure. This build adds no JSON parsing
//  library for the tool itself, so there is none available to a test either;
//  the flat renderer's own contract (see ContractDump.h) is that it is
//  brace-free, so that is what is checked directly instead.
// ---------------------------------------------------------------------------

#include "Cli/ArgumentTable.h"
#include "Cli/ContractDump.h"
#include "Conformance/Fixture.h"

#include <catch2/catch_test_macros.hpp>

#include <set>

using namespace Hyland::DocumentFilters::Cli::Cli;
using namespace Hyland::DocumentFilters::Cli::Tests::Conformance;

namespace
{
	/// <summary>
	/// Pulls the long argument names back out of a rendered section by reading
	/// only the template column, never the free-form description --- a line's
	/// tab-separated fields are <c>argument</c>, template, arity, description, and
	/// only the template is safe to search for <c>--name</c> tokens. A word split
	/// over the whole line would just as happily find one inside prose that
	/// happens to mention another flag by name.
	/// </summary>
	std::vector<std::string> longNamesIn(const std::string& section)
	{
		std::vector<std::string> result;

		size_t lineStart = 0;
		while (lineStart <= section.size())
		{
			size_t lineEnd = section.find('\n', lineStart);
			std::string line = section.substr(lineStart, lineEnd == std::string::npos ? std::string::npos : lineEnd - lineStart);

			if (line.rfind("argument", 0) == 0)
			{
				std::vector<std::string> fields;
				size_t fieldStart = 0;
				while (true)
				{
					size_t tab = line.find('\t', fieldStart);
					fields.push_back(line.substr(fieldStart, tab == std::string::npos ? std::string::npos : tab - fieldStart));
					if (tab == std::string::npos)
						break;
					fieldStart = tab + 1;
				}

				if (fields.size() >= 2)
				{
					size_t start = 0;
					const std::string& templateSpec = fields[1];
					while (true)
					{
						size_t bar = templateSpec.find('|', start);
						std::string form = templateSpec.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
						if (form.rfind("--", 0) == 0)
							result.push_back(form.substr(2));
						if (bar == std::string::npos)
							break;
						start = bar + 1;
					}
				}
			}

			if (lineEnd == std::string::npos)
				break;
			lineStart = lineEnd + 1;
		}

		return result;
	}
}

TEST_CASE("every verb appears in the dump", "[ContractDump]")
{
	std::string dump = ContractDump::render();

	for (const std::string& command : ArgumentTable::commands())
		CHECK(dump.find("command " + command) != std::string::npos);
}

TEST_CASE("every argument on every command appears in the dump", "[ContractDump]")
{
	for (const std::string& command : ArgumentTable::commands())
	{
		std::string section = ContractDump::renderCommand(command);

		for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
			CHECK(section.find("--" + argument->longName()) != std::string::npos);
	}
}

TEST_CASE("the dump declares nothing the argument table does not have", "[ContractDump]")
{
	// Without this a stale line in the dump would send the other three chasing an
	// argument that no longer exists.
	for (const std::string& command : ArgumentTable::commands())
	{
		std::set<std::string> declared;
		for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
			declared.insert(argument->longName());

		for (const std::string& name : longNamesIn(ContractDump::renderCommand(command)))
			CHECK(declared.count(name) > 0);
	}
}

TEST_CASE("the dump agrees with the shared fixture", "[ContractDump][Fixture]")
{
	// Transitively true given the two tests above and ArgumentTableTests, but
	// worth asserting directly: this is the pair of files a reader will diff by
	// hand when something looks wrong.
	for (const FixtureCase& testCase : Fixture::load("arguments.txt"))
	{
		std::string section = ContractDump::renderCommand(testCase.subject);

		for (const std::string& row : testCase.getAll("argument"))
		{
			size_t space = row.find_first_of(" \t");
			std::string templateSpec = row.substr(0, space);
			std::string longForm = templateSpec.substr(templateSpec.rfind('|') == std::string::npos ? 0 : templateSpec.rfind('|') + 1);

			CHECK(section.find(longForm) != std::string::npos);
		}
	}
}

TEST_CASE("each argument records its arity", "[ContractDump]")
{
	// The other three need this to register an argument correctly: a switch
	// declared as taking a value silently swallows the next argument.
	std::string section = ContractDump::renderCommand("convert");

	CHECK(section.find("none") != std::string::npos);
	CHECK(section.find("single") != std::string::npos);
	CHECK(section.find("multiple") != std::string::npos);
}

TEST_CASE("the dump is deterministic", "[ContractDump]")
{
	CHECK(ContractDump::render() == ContractDump::render());
}

TEST_CASE("the dump is line oriented and free of json", "[ContractDump]")
{
	// Not "contains no brace": a description is free to mention template
	// placeholders like {stem}, and --out-name's does. What actually keeps
	// this from parsing as JSON is that a JSON document's one top-level value
	// must begin, after only whitespace, with one of a handful of characters
	// --- '{', '[', a quote, a digit, '-', or the start of true/false/null ---
	// and this dump instead begins with the bare word "command".
	std::string dump = ContractDump::render();

	CHECK(dump.rfind("command ", 0) == 0);
}

TEST_CASE("the dump uses lf line endings", "[ContractDump]")
{
	// Otherwise it differs between a Windows and a Linux build.
	CHECK(ContractDump::render().find('\r') == std::string::npos);
}

// ------------------------------------------------------------- help --json

TEST_CASE("the json rendering is actually json", "[ContractDump]")
{
	// The whole reason it is a separate renderer: the flat dump is asserted
	// brace-free just above, so an argument named --json could not share it.
	std::string json = ContractDump::renderJson();

	CHECK(json.rfind('[', 0) == 0);
	CHECK(json.back() == '\n');
	CHECK(json.find('\r') == std::string::npos);
}

TEST_CASE("the json rendering names every command", "[ContractDump]")
{
	std::string json = ContractDump::renderJson();

	for (const std::string& command : ArgumentTable::commands())
		CHECK(json.find("\"" + command + "\"") != std::string::npos);
}

TEST_CASE("the json rendering names every argument on every command", "[ContractDump]")
{
	std::string json = ContractDump::renderJson();

	for (const std::string& command : ArgumentTable::commands())
	{
		for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
			CHECK(json.find("--" + argument->longName()) != std::string::npos);
	}
}

TEST_CASE("the json rendering carries arity and help text", "[ContractDump]")
{
	// A port registering arguments from this needs the arity; a person reading
	// it wants the description. Both have to survive the rendering.
	std::string json = ContractDump::renderJson();

	CHECK(json.find("\"arity\"") != std::string::npos);
	CHECK(json.find("\"description\"") != std::string::npos);
	CHECK(json.find("\"template\"") != std::string::npos);
}

TEST_CASE("the json rendering is deterministic", "[ContractDump]")
{
	CHECK(ContractDump::renderJson() == ContractDump::renderJson());
}

TEST_CASE("the two renderings describe the same surface", "[ContractDump]")
{
	// They share the argument table rather than the renderer, so nothing but a
	// test keeps them agreeing.
	std::string json = ContractDump::renderJson();

	for (const std::string& command : ArgumentTable::commands())
	{
		for (const std::string& name : longNamesIn(ContractDump::renderCommand(command)))
			CHECK(json.find("--" + name) != std::string::npos);
	}
}
