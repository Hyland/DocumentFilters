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
//  --out-name, checked against the shared out-names.txt fixture.
//
//  The render cases are tabular --- template, source, extension, page,
//  expected filename --- so they live in the fixture and all four
//  implementations run them. What stays here is behaviour a row cannot
//  express: the defaults, and the guarantee that a rendered name is a
//  filename and never a path.
// ---------------------------------------------------------------------------

#include "Conformance/Fixture.h"
#include "Planning/OutputNameTemplate.h"
#include "Runtime/CliError.h"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <stdexcept>

using namespace Hyland::DocumentFilters::Cli::Planning;
using namespace Hyland::DocumentFilters::Cli::Runtime;
using namespace Hyland::DocumentFilters::Cli::Tests::Conformance;

namespace
{
	/// <summary>One `= render source ext page => expected` row.</summary>
	struct RenderRow
	{
		std::string source;
		std::string extension;
		int page;
		std::string expected;
	};

	RenderRow parseRenderRow(const std::string& row, const FixtureCase& testCase)
	{
		size_t arrow = row.find("=>");
		if (arrow == std::string::npos)
			throw std::runtime_error(testCase.describe("render row has no '=>': " + row));

		std::string left = row.substr(0, arrow);
		std::string right = row.substr(arrow + 2);

		// Trim leading/trailing whitespace off the right half.
		size_t rightBegin = right.find_first_not_of(" \t");
		right = (rightBegin == std::string::npos) ? "" : right.substr(rightBegin);
		size_t rightEnd = right.find_last_not_of(" \t");
		if (rightEnd != std::string::npos)
			right = right.substr(0, rightEnd + 1);

		std::vector<std::string> inputs;
		std::string current;
		for (char c : left)
		{
			if (c == ' ' || c == '\t')
			{
				if (!current.empty())
				{
					inputs.push_back(current);
					current.clear();
				}
			}
			else
			{
				current += c;
			}
		}
		if (!current.empty())
			inputs.push_back(current);

		if (inputs.size() != 3)
			throw std::runtime_error(testCase.describe("render row needs 'source ext page => expected': " + row));

		return RenderRow{ inputs[0], inputs[1], std::stoi(inputs[2]), right };
	}
}

TEST_CASE("template matches the shared fixture", "[OutputNameTemplate][Fixture]")
{
	for (const FixtureCase& testCase : Fixture::load("out-names.txt"))
	{
		if (testCase.error().has_value())
		{
			bool threw = false;
			try
			{
				OutputNameTemplate::parse(testCase.subject);
			}
			catch (const UsageError& error)
			{
				threw = true;
				CHECK(error.code() == *testCase.error());
			}
			CHECK(threw);
			continue;
		}

		OutputNameTemplate tpl = OutputNameTemplate::parse(testCase.subject);

		if (testCase.has("uses-page"))
			CHECK(tpl.usesPage() == testCase.getBool("uses-page"));

		for (const std::string& row : testCase.getAll("render"))
		{
			RenderRow expectation = parseRenderRow(row, testCase);
			std::string actual = tpl.render(expectation.source, expectation.extension, expectation.page);
			CHECK(actual == expectation.expected);
		}
	}
}

// ------------------------------------------- behaviour the fixture cannot state

TEST_CASE("default for a single file format has no page placeholder", "[OutputNameTemplate]")
{
	OutputNameTemplate tpl = OutputNameTemplate::defaultTemplate(true);

	CHECK_FALSE(tpl.usesPage());
	CHECK(tpl.render("report.docx", "pdf", 1) == "report.pdf");
}

TEST_CASE("default for a one per page format includes a padded page number", "[OutputNameTemplate]")
{
	// Without this, every page of a PNG conversion would overwrite the last.
	OutputNameTemplate tpl = OutputNameTemplate::defaultTemplate(false);

	CHECK(tpl.usesPage());
	CHECK(tpl.render("report.docx", "png", 1) == "report-0001.png");
}

TEST_CASE("rendering never emits a path separator", "[OutputNameTemplate]")
{
	// The template names a file inside --out-dir. Whatever the input path
	// looked like, the result must not be able to escape that directory.
	std::vector<std::string> sources = { "../../etc/passwd", "..\\..\\windows\\system32\\config", "/absolute/path/report.docx" };

	OutputNameTemplate tpl = OutputNameTemplate::parse("{stem}.{ext}");

	for (const std::string& source : sources)
	{
		std::string rendered = tpl.render(source, "txt", 1);
		CHECK(rendered.find('/') == std::string::npos);
		CHECK(rendered.find('\\') == std::string::npos);
		CHECK(rendered.find("..") == std::string::npos);
	}
}

TEST_CASE("two pages of the same document never render to the same name", "[OutputNameTemplate]")
{
	// The property that makes a one-per-page format safe at all.
	OutputNameTemplate tpl = OutputNameTemplate::defaultTemplate(false);

	std::set<std::string> seen;
	for (int page = 1; page <= 200; ++page)
		CHECK(seen.insert(tpl.render("report.docx", "png", page)).second);
}

TEST_CASE("the original text is recoverable for the plan dump", "[OutputNameTemplate]")
{
	std::string text = "{stem}-{page:04}.{ext}";
	CHECK(OutputNameTemplate::parse(text).toCanonicalString() == text);
}
