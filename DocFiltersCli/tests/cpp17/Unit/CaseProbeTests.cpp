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

#include "Runtime/CaseProbe.h"

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Hyland::DocumentFilters::Cli::Runtime;

// Where the case question is asked, which is the part a real filesystem cannot
// check: on one volume every directory answers the same, so a probe aimed at the
// parent passes on every machine until someone converts into a mounted volume.

// A listing that records what it was asked for, so a test can see how far the
// probe walked.
namespace
{
	std::function<std::optional<std::string>()> listing(std::vector<std::string> entries, std::vector<std::string>* seen = nullptr)
	{
		auto next = std::make_shared<std::size_t>(0);
		auto items = std::make_shared<std::vector<std::string>>(std::move(entries));
		return [next, items, seen]() -> std::optional<std::string> {
			if (*next >= items->size())
				return std::nullopt;

			const std::string& entry = (*items)[(*next)++];
			if (seen != nullptr)
				seen->push_back(entry);

			return entry;
		};
	}
}

// The whole of the finding this answers: every path the probe asks about lies
// inside the directory being measured. Aimed at the parent --- which is what it
// used to do --- an output under a mounted volume is measured against the
// filesystem holding the mount point, and two outputs differing only by case are
// planned as two files that turn out to be one.
TEST_CASE("asks only about names inside the directory", "[CaseProbe]")
{
	std::vector<std::string> asked;
	CaseProbe::measure("/mnt/ntfs", listing({ "/mnt/ntfs/Report.txt" }), [&asked](const std::string& candidate) {
		asked.push_back(candidate);
		return false;
	});

	CHECK(asked == std::vector<std::string>{ "/mnt/ntfs/rEPORT.TXT" });
}

TEST_CASE("finding the flipped spelling means case-insensitive", "[CaseProbe]")
{
	CHECK(CaseProbe::measure("out", listing({ "out/Report.txt" }), [](const std::string&) { return true; }) == false);
}

TEST_CASE("not finding it means case-sensitive", "[CaseProbe]")
{
	CHECK(CaseProbe::measure("out", listing({ "out/Report.txt" }), [](const std::string&) { return false; }) == true);
}

// Lazily, and only as far as it has to be: the first name with a letter in it
// settles the question, and a directory of a hundred thousand files costs one
// entry.
TEST_CASE("stops at the first entry that can answer", "[CaseProbe]")
{
	std::vector<std::string> walked;
	CaseProbe::measure("out", listing({ "out/2026", "out/Report.txt", "out/Other.txt" }, &walked),
		[](const std::string&) { return false; });

	CHECK(walked == std::vector<std::string>{ "out/2026", "out/Report.txt" });
}

TEST_CASE("a directory with nothing to say answers nothing", "[CaseProbe]")
{
	CHECK_FALSE(CaseProbe::measure("out", listing({}), [](const std::string&) { return true; }).has_value());
	CHECK_FALSE(CaseProbe::measure("out", listing({ "out/2026", "out/4.1" }), [](const std::string&) { return true; }).has_value());
}

TEST_CASE("probes inside the directory being measured", "[CaseProbe]")
{
	CHECK(CaseProbe::flippedSibling("/mnt/ntfs/out", "/mnt/ntfs/out/Report.txt") == "/mnt/ntfs/out/rEPORT.TXT");
}

// The defect this replaced: it flipped the directory's own name and looked for it
// in the parent, so an output under /mnt/ntfs measured /mnt --- a different
// filesystem with a different answer.
TEST_CASE("never probes the parent", "[CaseProbe]")
{
	std::optional<std::string> probe = CaseProbe::flippedSibling("/mnt/ntfs", "/mnt/ntfs/Report.txt");

	REQUIRE(probe.has_value());
	CHECK(probe->rfind("/mnt/ntfs/", 0) == 0);
	CHECK(*probe != "/mnt/NTFS");
}

TEST_CASE("accepts a bare name as well as a full path", "[CaseProbe]")
{
	CHECK(CaseProbe::flippedSibling("out", "Report.txt") == "out/rEPORT.TXT");
}

TEST_CASE("flips every letter rather than lowercasing", "[CaseProbe]")
{
	CHECK(CaseProbe::flippedSibling("out", "Report") == "out/rEPORT");
	CHECK(CaseProbe::flippedSibling("out", "REPORT") == "out/report");
}

// Written with escapes rather than the characters themselves so the test says the
// same thing whatever encoding reads this file.
TEST_CASE("leaves everything that is not an ascii letter alone", "[CaseProbe]")
{
	CHECK(CaseProbe::flippedSibling("out", "Q4-2026_r\xc3\xa9sum\xc3\xa9.PDF") == "out/q4-2026_R\xc3\xa9SUM\xc3\xa9.pdf");
}

// A name with no letter to flip is the same name, which would make the probe ask
// whether the entry it already has exists --- always yes, and the directory would
// read as case-insensitive whatever it is. The caller moves on to the next entry.
TEST_CASE("declines a name with no letter in it", "[CaseProbe]")
{
	CHECK_FALSE(CaseProbe::flippedSibling("out", "2026-01-31.4").has_value());
	CHECK_FALSE(CaseProbe::flippedSibling("out", "_").has_value());
}

TEST_CASE("reads a trailing separator as part of neither name", "[CaseProbe]")
{
	CHECK(CaseProbe::flippedSibling("out/", "out/Sub/") == "out/sUB");
}

TEST_CASE("reads a windows separator the same way", "[CaseProbe]")
{
	CHECK(CaseProbe::flippedSibling("C:/work/out", "C:\\work\\out\\Report.txt") == "C:/work/out/rEPORT.TXT");
}
