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

#include "Execution/CanvasConversion.h"
#include "Planning/PageRange.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace Hyland::DocumentFilters::Cli::Planning;
using namespace Hyland::DocumentFilters::Cli::Runtime;

namespace
{
	// The lazy pages as a list, for comparing with an expected one.
	std::vector<int> listed(const PageRange::Pages& pages)
	{
		return std::vector<int>(pages.begin(), pages.end());
	}
}

TEST_CASE("parses the example from the specification", "[PageRange]")
{
	PageRange range = PageRange::parse("1,3-5,8");
	CHECK(listed(range.enumerate(10)) == std::vector<int>{ 1, 3, 4, 5, 8 });
}

TEST_CASE("single page selects only that page", "[PageRange]")
{
	PageRange range = PageRange::parse("4");
	CHECK(range.contains(4));
	CHECK_FALSE(range.contains(3));
	CHECK_FALSE(range.contains(5));
}

TEST_CASE("pages are one based", "[PageRange]")
{
	PageRange range = PageRange::parse("1");
	CHECK(range.contains(1));
	CHECK_FALSE(range.contains(0));
}

TEST_CASE("overlapping ranges collapse and sort", "[PageRange]")
{
	PageRange range = PageRange::parse("3,1,1-2");
	CHECK(listed(range.enumerate(10)) == std::vector<int>{ 1, 2, 3 });
}

TEST_CASE("a span of one page is allowed", "[PageRange]")
{
	PageRange range = PageRange::parse("2-2");
	CHECK(listed(range.enumerate(10)) == std::vector<int>{ 2 });
}

TEST_CASE("an explicit selection is returned regardless of the count argument", "[PageRange]")
{
	PageRange range = PageRange::parse("1,5-9");
	CHECK(listed(range.enumerate(6)) == std::vector<int>{ 1, 5, 6, 7, 8, 9 });
}

TEST_CASE("an explicit selection ignores the count argument entirely", "[PageRange]")
{
	PageRange range = PageRange::parse("10,20");
	CHECK(listed(range.enumerate(1)) == std::vector<int>{ 10, 20 });
}

TEST_CASE("all selects every page", "[PageRange]")
{
	CHECK(PageRange::all().isAll());
	CHECK(listed(PageRange::all().enumerate(3)) == std::vector<int>{ 1, 2, 3 });
}

TEST_CASE("an explicit range is not all", "[PageRange]")
{
	CHECK_FALSE(PageRange::parse("1-3").isAll());
}

TEST_CASE("malformed input is a usage error", "[PageRange]")
{
	std::vector<std::string> cases = { "", "abc", "1,", ",1", "1--3", "1-", "-3", "1,,2" };
	for (const std::string& text : cases)
	{
		UsageError error = [&] {
			try
			{
				PageRange::parse(text);
			}
			catch (const UsageError& e)
			{
				return e;
			}
			throw std::logic_error("expected a UsageError for '" + text + "'");
		}();

		CHECK(error.code() == ErrorCodes::badPageRange);
		CHECK(error.exitCode() == ExitCode::usageError);
	}
}

TEST_CASE("page zero is a usage error", "[PageRange]")
{
	// Pages are one-based as the user writes them, so 0 is meaningless rather
	// than a synonym for the first page.
	CHECK_THROWS_AS(PageRange::parse("0"), UsageError);
}

TEST_CASE("negative pages are a usage error", "[PageRange]")
{
	CHECK_THROWS_AS(PageRange::parse("-1,3"), UsageError);
}

TEST_CASE("a descending span is a usage error", "[PageRange]")
{
	// "5-3" is far more likely to be a typo than a request to reverse the
	// pages, so it is rejected rather than silently normalised.
	CHECK_THROWS_AS(PageRange::parse("5-3"), UsageError);
}

TEST_CASE("canonical text form is stable for the conformance corpus", "[PageRange]")
{
	// The corpus compares plans as text, so two equivalent selections must
	// render identically in all four language implementations.
	CHECK(PageRange::parse("1,2,3").toCanonicalString() == PageRange::parse("1-3").toCanonicalString());
}

TEST_CASE("overlapping and touching spans merge", "[PageRange]")
{
	PageRange range = PageRange::parse("3,1,1-2,5-7,6-9");

	CHECK(range.toCanonicalString() == "1-3,5-9");
	CHECK(range.firstPage() == 1);
}

TEST_CASE("the first page of all is one", "[PageRange]")
{
	CHECK(PageRange::all().firstPage() == 1);
	CHECK(PageRange::parse("8,3-5").firstPage() == 3);
}

TEST_CASE("the widest span costs nothing until it is walked", "[PageRange]")
{
	// Every page used to be expanded into a set while the arguments were read,
	// so this valid value allocated two billion entries before anything ran.
	PageRange range = PageRange::parse("1-2147483647");

	CHECK(range.toCanonicalString() == "1-2147483647");
	CHECK(range.firstPage() == 1);
	CHECK(range.contains(2147483647));
	CHECK_FALSE(range.contains(0));

	std::vector<int> firstThree;
	for (int page : range.enumerate(3))
	{
		if (firstThree.size() == 3)
			break;
		firstThree.push_back(page);
	}
	CHECK(firstThree == std::vector<int>{ 1, 2, 3 });
	CHECK(Hyland::DocumentFilters::Cli::Execution::CanvasConversion::renderablePages(range, 3) == std::vector<int>{ 1, 2, 3 });
}

TEST_CASE("a span ending at the last int does not wrap", "[PageRange]")
{
	CHECK(listed(PageRange::parse("2147483646-2147483647").enumerate(1)) == std::vector<int>{ 2147483646, 2147483647 });
}
