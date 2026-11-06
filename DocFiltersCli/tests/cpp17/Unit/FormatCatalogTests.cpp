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
//  The --to token table, checked against the shared formats.txt fixture.
//
//  The table is data, and the Python, Java and C++17 implementations need
//  exactly the same data, so it lives in the fixture rather than being
//  transcribed into four test suites. What stays here is the behaviour the
//  fixture cannot express: catalogue-wide invariants, and how lookup
//  failures are reported.
// ---------------------------------------------------------------------------

#include "Conformance/Fixture.h"
#include "Planning/FormatCatalog.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <stdexcept>

using namespace Hyland::DocumentFilters::Cli::Planning;
using namespace Hyland::DocumentFilters::Cli::Runtime;
using namespace Hyland::DocumentFilters::Cli::Tests::Conformance;

namespace
{
	/// <summary>The ASCII-only fold the catalogue matches tokens with.</summary>
	std::string fold(const std::string& token)
	{
		std::string folded = token;
		for (char& c : folded)
		{
			if (c >= 'A' && c <= 'Z')
				c = static_cast<char>(c + 32);
		}
		return folded;
	}

	FormatKind parseKind(const FixtureCase& testCase)
	{
		std::optional<std::string> kind = testCase.get("kind");
		if (kind == "text")
			return FormatKind::Text;
		if (kind == "canvas")
			return FormatKind::Canvas;
		if (kind == "docpack")
			return FormatKind::DocPack;

		throw std::runtime_error(testCase.describe("unknown 'kind'"));
	}
}

TEST_CASE("token matches the shared catalogue", "[FormatCatalog][Fixture]")
{
	for (const FixtureCase& testCase : Fixture::load("formats.txt"))
	{
		const std::string& token = testCase.subject;

		if (testCase.error().has_value())
		{
			bool threw = false;
			try
			{
				FormatCatalog::resolve(token);
			}
			catch (const UsageError& error)
			{
				threw = true;
				CHECK(error.code() == *testCase.error());
			}
			CHECK(threw);
			continue;
		}

		const FormatSpec& spec = FormatCatalog::resolve(token);

		// The canonical token is the folded one: the fixture states PDF and
		// Tiff as their own cases, and both must land on the lower-case
		// entry.
		CHECK(spec.token == fold(token));

		CHECK(spec.kind == parseKind(testCase));
		CHECK(spec.isSingleFile == testCase.getBool("single-file"));
		CHECK(spec.supportsCombine == testCase.getBool("combine"));
		CHECK(spec.isHiDefCapable == testCase.getBool("hidef"));
		CHECK(spec.extension == testCase.get("ext"));

		// 'none' rather than an empty value, so the row is visible in the fixture.
		std::string expectedOptions = (testCase.get("options") == "none") ? "" : *testCase.get("options");
		CHECK(spec.canvasOptions == expectedOptions);
	}
}

// ------------------------------------------- behaviour the fixture cannot state

TEST_CASE("the catalogue has an entry for every resolvable token", "[FormatCatalog]")
{
	// Guards against a token that resolves but is missing from all(), which
	// would leave it out of the help text and the formats listing.
	for (const FixtureCase& testCase : Fixture::load("formats.txt"))
	{
		if (testCase.error().has_value())
			continue;

		bool found = false;
		for (const FormatSpec& spec : FormatCatalog::all())
		{
			if (spec.token == fold(testCase.subject))
			{
				found = true;
				break;
			}
		}
		CHECK(found);
	}
}

TEST_CASE("the catalogue declares nothing the fixture does not list", "[FormatCatalog]")
{
	// The reverse direction: an implementation must not quietly support a
	// format the other three have never heard of.
	std::set<std::string> expected;
	for (const FixtureCase& testCase : Fixture::load("formats.txt"))
	{
		if (!testCase.error().has_value())
			expected.insert(testCase.subject);
	}

	for (const FormatSpec& spec : FormatCatalog::all())
		CHECK(expected.count(spec.token) > 0);
}

TEST_CASE("every catalogue entry has a unique token", "[FormatCatalog]")
{
	std::set<std::string> seen;
	for (const FormatSpec& spec : FormatCatalog::all())
		CHECK(seen.insert(spec.token).second);
}

TEST_CASE("every catalogue entry has a display name for help text", "[FormatCatalog]")
{
	// Not in the fixture because it is prose, and prose is allowed to differ
	// between languages in a way that ids and extensions are not.
	for (const FormatSpec& spec : FormatCatalog::all())
		CHECK_FALSE(spec.displayName.empty());
}

TEST_CASE("tokens are matched case insensitively but canonicalise to lower case", "[FormatCatalog]")
{
	// Users type --to PDF; the plan dump must still say pdf, or the corpus
	// comparison becomes case sensitive by accident.
	CHECK(FormatCatalog::resolve("PDF").token == "pdf");
	CHECK(FormatCatalog::resolve("MD-Text").token == "md-text");
}

TEST_CASE("the error names the token and where to find the valid ones", "[FormatCatalog]")
{
	// The message points at the formats command rather than listing the
	// catalogue inline.
	try
	{
		FormatCatalog::resolve("pdxf");
		FAIL("expected a UsageError");
	}
	catch (const UsageError& error)
	{
		CHECK(std::string(error.what()).find("pdxf") != std::string::npos);
		CHECK(std::string(error.what()).find("formats") != std::string::npos);
	}
}

TEST_CASE("an unknown token exits with the usage code", "[FormatCatalog]")
{
	try
	{
		FormatCatalog::resolve("pdxf");
		FAIL("expected a UsageError");
	}
	catch (const UsageError& error)
	{
		CHECK(error.exitCode() == ExitCode::usageError);
	}
}

TEST_CASE("tryResolve returns null rather than throwing", "[FormatCatalog]")
{
	CHECK(FormatCatalog::tryResolve("nonsense") == nullptr);
	CHECK(FormatCatalog::tryResolve("pdf") != nullptr);
}

TEST_CASE("a text mode format is never high definition capable", "[FormatCatalog]")
{
	// A structural invariant rather than a per-row fact: if these ever
	// disagreed, --fallback would have nothing coherent to fall back to.
	for (const FormatSpec& spec : FormatCatalog::all())
	{
		if (spec.kind == FormatKind::Text)
			CHECK_FALSE(spec.isHiDefCapable);
	}
}
