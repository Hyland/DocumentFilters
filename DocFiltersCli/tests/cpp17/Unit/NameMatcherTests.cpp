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

#include "Planning/NameMatcher.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli::Planning;
using namespace Hyland::DocumentFilters::Cli::Runtime;

namespace
{
	NameMatcher glob(const std::string& pattern) { return NameMatcher::fromGlobs({ pattern }); }
	NameMatcher glob(const std::string& a, const std::string& b) { return NameMatcher::fromGlobs({ a, b }); }
	NameMatcher regexOf(const std::string& pattern) { return NameMatcher::fromRegexes({ pattern }); }
}

TEST_CASE("a star matches within one path segment", "[NameMatcher]")
{
	NameMatcher matcher = glob("*.pdf");
	CHECK(matcher.matches("report.pdf"));
	CHECK_FALSE(matcher.matches("reports/report.pdf"));
}

TEST_CASE("a double star crosses path segments", "[NameMatcher]")
{
	NameMatcher matcher = glob("**/*.pdf");
	CHECK(matcher.matches("a/b/c/report.pdf"));
	CHECK_FALSE(matcher.matches("a/b/c/report.docx"));
}

TEST_CASE("a question mark matches exactly one character", "[NameMatcher]")
{
	NameMatcher matcher = glob("report?.pdf");
	CHECK(matcher.matches("report1.pdf"));
	CHECK_FALSE(matcher.matches("report.pdf"));
	CHECK_FALSE(matcher.matches("report12.pdf"));
}

TEST_CASE("a leading bang excludes", "[NameMatcher]")
{
	// The specification names this as the default idiom for exclusion.
	NameMatcher matcher = glob("**/*.pdf", "!**/draft/*");
	CHECK(matcher.matches("final/report.pdf"));
	CHECK_FALSE(matcher.matches("draft/report.pdf"));
}

TEST_CASE("exclusions alone keep everything else", "[NameMatcher]")
{
	NameMatcher matcher = glob("!**/*.tmp");
	CHECK(matcher.matches("report.pdf"));
	CHECK_FALSE(matcher.matches("scratch/a.tmp"));
}

TEST_CASE("an entry matching several includes is kept once", "[NameMatcher]")
{
	NameMatcher matcher = glob("**/*.pdf", "**/report.*");
	CHECK(matcher.matches("report.pdf"));
}

TEST_CASE("an exclusion beats an inclusion", "[NameMatcher]")
{
	NameMatcher matcher = glob("**/*.pdf", "!**/secret.pdf");
	CHECK_FALSE(matcher.matches("a/secret.pdf"));
}

TEST_CASE("backslashes are treated as separators so patterns are portable", "[NameMatcher]")
{
	// A pattern written with forward slashes must work against Windows paths,
	// otherwise every test and every corpus case is platform specific.
	NameMatcher matcher = glob("**/*.pdf");
	CHECK(matcher.matches("a\\b\\report.pdf"));
}

TEST_CASE("regex mode matches the whole path not a substring", "[NameMatcher]")
{
	// Deliberately unanchored. An anchored pattern such as ".*\.pdf$" would
	// pass under substring semantics too, so it could not tell whole-match
	// from search --- and the four languages differ exactly there.
	NameMatcher matcher = regexOf("report\\.pdf");
	CHECK(matcher.matches("report.pdf"));
	CHECK_FALSE(matcher.matches("a/report.pdf"));
	CHECK_FALSE(matcher.matches("old-report.pdf"));
	CHECK_FALSE(matcher.matches("report.pdf.bak"));
}

TEST_CASE("a regex can still match a full path when written to", "[NameMatcher]")
{
	NameMatcher matcher = regexOf("a/b/report\\.pdf");
	CHECK(matcher.matches("a/b/report.pdf"));
	CHECK_FALSE(matcher.matches("x/a/b/report.pdf"));
}

TEST_CASE("regex mode expresses what a glob cannot", "[NameMatcher]")
{
	// Alternation with a bounded repeat: the reason --regex exists at all.
	NameMatcher matcher = regexOf("^report-\\d{4}\\.(pdf|docx)$");
	CHECK(matcher.matches("report-2026.pdf"));
	CHECK(matcher.matches("report-2026.docx"));
	CHECK_FALSE(matcher.matches("report-26.pdf"));
}

TEST_CASE("an invalid regex is a usage error", "[NameMatcher]")
{
	bool threw = false;
	try
	{
		NameMatcher::fromRegexes({ "[unclosed" });
	}
	catch (const UsageError& error)
	{
		threw = true;
		CHECK(error.exitCode() == ExitCode::usageError);
	}
	CHECK(threw);
}

TEST_CASE("canonical text form is stable for the conformance corpus", "[NameMatcher]")
{
	CHECK(glob("**/*.pdf").toCanonicalString() == glob("**/*.pdf").toCanonicalString());
}
