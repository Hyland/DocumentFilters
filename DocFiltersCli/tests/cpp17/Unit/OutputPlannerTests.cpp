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
//  The output model. This is the highest-risk logic in the tool and the
//  reason planning is a separate, pure stage.
//
//  The rule underneath every test here: the meaning of -o never depends on
//  --to.
// ---------------------------------------------------------------------------

#include "Fakes/InMemoryFileSystem.h"
#include "Planning/FormatCatalog.h"
#include "Planning/OutputPlanner.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli::Configuration;
using namespace Hyland::DocumentFilters::Cli::Discovery;
using namespace Hyland::DocumentFilters::Cli::Planning;
using namespace Hyland::DocumentFilters::Cli::Runtime;
using namespace Hyland::DocumentFilters::Cli::Tests::Fakes;

namespace
{
	std::vector<InputSource> inputs(std::initializer_list<std::string> paths)
	{
		std::vector<InputSource> list;
		for (const std::string& path : paths)
		{
			size_t slash = path.rfind('/');
			InputSource source;
			source.path = path;
			source.relativePath = (slash == std::string::npos) ? path : path.substr(slash + 1);
			list.push_back(source);
		}
		return list;
	}

	/// <summary>
	/// OutputPlanner::plan against a filesystem that tells names apart by case,
	/// which is what these cases assume unless one says otherwise.
	/// </summary>
	RunPlan planFor(const SharedOptions& shared, const ConvertOptions& convert,
		const std::vector<InputSource>& sources, bool caseSensitive = true)
	{
		InMemoryFileSystem fileSystem;
		fileSystem.caseSensitive = caseSensitive;
		return OutputPlanner::plan(shared, convert, sources, fileSystem);
	}

	ConvertOptions toFormat(const std::string& token, bool combine = false)
	{
		ConvertOptions options;
		options.combine = combine;
		options.format = &FormatCatalog::resolve(token);
		return options;
	}
}

// ---------------------------------------------------------------- cardinality

TEST_CASE("out-dir with many inputs gives each its own output", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outDir = "out";

	RunPlan plan = planFor(shared, toFormat("pdf"), inputs({ "a.docx", "b.docx" }));

	CHECK(plan.cardinality == OutputCardinality::PerInput);
	REQUIRE(plan.items.size() == 2);
	REQUIRE(plan.items[0].target.has_value());
	CHECK(*plan.items[0].target == "out/a.pdf");
	REQUIRE(plan.items[1].target.has_value());
	CHECK(*plan.items[1].target == "out/b.pdf");
}

TEST_CASE("out-file with one input is a single target", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outFile = "book.pdf";

	RunPlan plan = planFor(shared, toFormat("pdf"), inputs({ "a.docx" }));

	CHECK(plan.cardinality == OutputCardinality::SingleTarget);
	REQUIRE(plan.items[0].target.has_value());
	CHECK(*plan.items[0].target == "book.pdf");
}

TEST_CASE("combine with out-file merges every input into one document", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outFile = "book.pdf";

	RunPlan plan = planFor(shared, toFormat("pdf", true), inputs({ "a.docx", "b.docx", "c.docx" }));

	CHECK(plan.cardinality == OutputCardinality::Combined);
	REQUIRE(plan.combinedTarget.has_value());
	CHECK(*plan.combinedTarget == "book.pdf");
	REQUIRE(plan.items.size() == 3);

	// Items contribute to the shared target rather than owning one.
	for (const PlannedItem& item : plan.items)
		CHECK_FALSE(item.target.has_value());
}

TEST_CASE("stdout is its own cardinality", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.useStdout = true;

	RunPlan plan = planFor(shared, toFormat("pdf"), inputs({ "a.docx" }));

	CHECK(plan.cardinality == OutputCardinality::Stream);
}

// ------------------------------------------------------------- the hard error

TEST_CASE("out-file with several inputs and no combine is a hard error", "[OutputPlanner]")
{
	// The specification is explicit that this is an error and never a
	// guess: the tool must not silently pick one input or overwrite
	// repeatedly.
	SharedOptions shared;
	shared.output.outFile = "one.pdf";

	try
	{
		planFor(shared, toFormat("pdf"), inputs({ "a.docx", "b.docx" }));
		FAIL("expected an OutputConflictError");
	}
	catch (const OutputConflictError& error)
	{
		CHECK(error.code() == ErrorCodes::outFileMultiInput);
		CHECK(error.exitCode() == ExitCode::outputConflict);
	}
}

TEST_CASE("no inputs is its own error and exit code", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outDir = "out";

	try
	{
		planFor(shared, toFormat("pdf"), inputs({}));
		FAIL("expected a NoInputsError");
	}
	catch (const NoInputsError& error)
	{
		CHECK(error.exitCode() == ExitCode::noInputsMatched);
	}
}

// ------------------------------------------------- page fan-out is a separate axis

TEST_CASE("a one per page format is still per input", "[OutputPlanner]")
{
	// One document producing five PNGs is not a different cardinality.
	// Target ownership and page count are independent questions.
	SharedOptions shared;
	shared.output.outDir = "out";

	RunPlan plan = planFor(shared, toFormat("png"), inputs({ "a.docx", "b.docx" }));

	CHECK(plan.cardinality == OutputCardinality::PerInput);
}

TEST_CASE("a one per page format without pages defers its targets", "[OutputPlanner]")
{
	// The filenames depend on the document's page count, which is unknown
	// until it is opened, so the planner records a pattern instead of
	// paths.
	SharedOptions shared;
	shared.output.outDir = "out";

	RunPlan plan = planFor(shared, toFormat("png"), inputs({ "a.docx" }));

	CHECK(plan.items[0].isPageCountDependent());
	CHECK_FALSE(plan.items[0].target.has_value());
	CHECK(plan.items[0].targetPattern.has_value());
}

TEST_CASE("a one per page format defers its targets even with explicit pages", "[OutputPlanner]")
{
	// --pages does not make the filenames knowable: the range is clamped to
	// the document's real page count, so --pages 1-2 on a one-page
	// document writes one file. Resolving both up front would let the
	// planner reject a run because a page-2 output already exists when
	// page 2 is never written.
	SharedOptions shared;
	shared.output.outDir = "out";
	ConvertOptions options = toFormat("png");
	options.pages = PageRange::parse("1-2");

	RunPlan plan = planFor(shared, options, inputs({ "a.docx" }));

	CHECK(plan.items[0].isPageCountDependent());
	CHECK_FALSE(plan.items[0].target.has_value());
	CHECK(plan.items[0].targetPattern.has_value());
}

TEST_CASE("the first checked target is the first selected page not page one", "[OutputPlanner]")
{
	// With --pages 5-6 the first output is page 5, so that is what the
	// fail-fast existence check has to look at.
	TargetPattern pattern;
	pattern.directory = "out";
	pattern.templateName = OutputNameTemplate::parse("{stem}-{page:04}.{ext}");
	pattern.extension = "png";

	CHECK(pattern.firstTarget("a.docx", PageRange::parse("5-6")) == "out/a-0005.png");
	CHECK(pattern.firstTarget("a.docx", PageRange::all()) == "out/a-0001.png");
}

TEST_CASE("a single file format resolves its target even without pages", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outDir = "out";

	RunPlan plan = planFor(shared, toFormat("pdf"), inputs({ "a.docx" }));

	CHECK_FALSE(plan.items[0].isPageCountDependent());
}

// ------------------------------------------------------------------ collisions

TEST_CASE("two inputs resolving to the same output is an error", "[OutputPlanner]")
{
	// A flattened walk of a/report.docx and b/report.docx would silently
	// overwrite. Only visible because the whole batch is planned at once.
	SharedOptions shared;
	shared.output.outDir = "out";

	try
	{
		planFor(shared, toFormat("pdf"), inputs({ "a/report.docx", "b/report.docx" }));
		FAIL("expected an OutputConflictError");
	}
	catch (const OutputConflictError& error)
	{
		CHECK(error.code() == ErrorCodes::outputCollision);
	}
}

TEST_CASE("names differing only in case collide where the filesystem folds case", "[OutputPlanner]")
{
	// This was silent data loss: the run exited 0, printed nothing, and left one
	// file holding the second document's text. Nothing caught it because the
	// planner compared paths exactly, which is right on the filesystem the suite
	// happened to run against and wrong on the one most users have.
	//
	// The corpus cannot hold this case -- it states the other answer, since its
	// filesystem is case-sensitive and a shared corpus has to read the same
	// everywhere. This is that half.
	SharedOptions shared;
	shared.output.outDir = "out";

	try
	{
		planFor(shared, toFormat("text"), inputs({ "a/Report.docx", "b/report.docx" }), false);
		FAIL("expected an OutputConflictError");
	}
	catch (const OutputConflictError& error)
	{
		CHECK(error.code() == ErrorCodes::outputCollision);
	}
}

TEST_CASE("names differing only in case are two outputs where the filesystem is case sensitive", "[OutputPlanner]")
{
	// The other direction, and the reason the rule is not simply "always fold":
	// refusing this declines work the tool can do correctly.
	SharedOptions shared;
	shared.output.outDir = "out";

	RunPlan result = planFor(shared, toFormat("text"), inputs({ "a/Report.docx", "b/report.docx" }), true);

	REQUIRE(result.items.size() == 2);
	CHECK(result.items[0].target == std::optional<std::string>("out/Report.txt"));
	CHECK(result.items[1].target == std::optional<std::string>("out/report.txt"));
}

TEST_CASE("two inputs with the same name always collide", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outDir = "out";

	for (bool caseSensitive : { true, false })
	{
		try
		{
			planFor(shared, toFormat("text"), inputs({ "a/report.docx", "b/report.docx" }), caseSensitive);
			FAIL("expected an OutputConflictError");
		}
		catch (const OutputConflictError& error)
		{
			CHECK(error.code() == ErrorCodes::outputCollision);
		}
	}
}

TEST_CASE("collisions are detected for deferred targets too", "[OutputPlanner]")
{
	// Page count is unknown for PNG, but the stems already collide, so
	// this is decidable from the pattern alone.
	SharedOptions shared;
	shared.output.outDir = "out";

	CHECK_THROWS_AS(planFor(shared, toFormat("png"), inputs({ "a/report.docx", "b/report.docx" })), OutputConflictError);
}

TEST_CASE("same stem with different source extensions still collides", "[OutputPlanner]")
{
	// report.docx and report.pdf both become report.txt.
	SharedOptions shared;
	shared.output.outDir = "out";

	CHECK_THROWS_AS(planFor(shared, toFormat("text"), inputs({ "report.docx", "report.pdf" })), OutputConflictError);
}

TEST_CASE("an out-name template can resolve a collision", "[OutputPlanner]")
{
	// {srcext} distinguishes the two, so this must not error.
	SharedOptions shared;
	shared.output.outDir = "out";
	shared.output.nameTemplate = OutputNameTemplate::parse("{stem}-{srcext}.{ext}");

	RunPlan plan = planFor(shared, toFormat("text"), inputs({ "report.docx", "report.pdf" }));

	REQUIRE(plan.items[0].target.has_value());
	CHECK(*plan.items[0].target == "out/report-docx.txt");
	REQUIRE(plan.items[1].target.has_value());
	CHECK(*plan.items[1].target == "out/report-pdf.txt");
}

// -------------------------------------------------------------- existing files

TEST_CASE("an existing target without overwrite is an error", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outDir = "out";
	InMemoryFileSystem fs;
	fs.addFile("out/a.pdf", "old");

	RunPlan plan = planFor(shared, toFormat("pdf"), inputs({ "a.docx" }));

	try
	{
		OutputPlanner::verifyTargets(plan, shared, fs);
		FAIL("expected an OutputConflictError");
	}
	catch (const OutputConflictError& error)
	{
		CHECK(error.code() == ErrorCodes::outputExists);
	}
}

TEST_CASE("overwrite permits an existing target", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outDir = "out";
	shared.output.overwrite = true;
	InMemoryFileSystem fs;
	fs.addFile("out/a.pdf", "old");

	RunPlan plan = planFor(shared, toFormat("pdf"), inputs({ "a.docx" }));

	OutputPlanner::verifyTargets(plan, shared, fs);
}

TEST_CASE("verification checks the first page of a deferred item", "[OutputPlanner]")
{
	// Fail fast on the common case: re-running a conversion into a
	// directory that already holds its output.
	SharedOptions shared;
	shared.output.outDir = "out";
	InMemoryFileSystem fs;
	fs.addFile("out/a-0001.png", "old");

	RunPlan plan = planFor(shared, toFormat("png"), inputs({ "a.docx" }));

	CHECK_THROWS_AS(OutputPlanner::verifyTargets(plan, shared, fs), OutputConflictError);
}

TEST_CASE("verification does not check later pages of a deferred item", "[OutputPlanner]")
{
	// Page 2 may never be written, so its presence must not fail the run
	// here. Execution checks each page as it produces it.
	SharedOptions shared;
	shared.output.outDir = "out";
	InMemoryFileSystem fs;
	fs.addFile("out/a-0002.png", "old");

	RunPlan plan = planFor(shared, toFormat("png"), inputs({ "a.docx" }));

	OutputPlanner::verifyTargets(plan, shared, fs);
}

// --------------------------------------------------------------- tree layout

TEST_CASE("the source tree is replicated under out-dir", "[OutputPlanner]")
{
	SharedOptions shared;
	shared.output.outDir = "out";

	InputSource source;
	source.path = "corpus/reports/q1.docx";
	source.relativePath = "reports/q1.docx";
	source.root = "corpus";

	RunPlan plan = planFor(shared, toFormat("pdf"), { source });

	REQUIRE(plan.items[0].target.has_value());
	CHECK(*plan.items[0].target == "out/reports/q1.pdf");
}
