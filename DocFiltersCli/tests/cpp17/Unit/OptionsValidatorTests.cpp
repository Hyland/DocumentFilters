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
//  The rules no single argument can enforce, because they are relations
//  between arguments. Everything here is decidable from the command line
//  alone --- rules that need to know how many files exist live in the
//  planner instead.
// ---------------------------------------------------------------------------

#include "Configuration/OptionsValidator.h"
#include "Planning/FormatCatalog.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <stdexcept>

using namespace Hyland::DocumentFilters::Cli::Configuration;
using namespace Hyland::DocumentFilters::Cli::Planning;
using namespace Hyland::DocumentFilters::Cli::Runtime;

namespace
{
	ConvertOptions toFormat(const std::string& token)
	{
		ConvertOptions options;
		options.format = &FormatCatalog::resolve(token);
		return options;
	}

	UsageError expectUsageError(const std::function<void()>& action)
	{
		try
		{
			action();
		}
		catch (const UsageError& error)
		{
			return error;
		}
		throw std::logic_error("expected a UsageError");
	}
}

TEST_CASE("a bare default configuration is valid", "[OptionsValidator]")
{
	OptionsValidator::validate(SharedOptions());
}

TEST_CASE("out-file and out-dir together are a usage error", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outFile = "book.pdf";
	shared.output.outDir = "out";

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("out-name without out-dir is a usage error", "[OptionsValidator]")
{
	// The template names a file inside a directory; without one it means nothing.
	SharedOptions shared;
	shared.output.nameTemplate = OutputNameTemplate::parse("{stem}.{ext}");

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.code() == ErrorCodes::outNameNeedsOutDir);
}

TEST_CASE("subfile depth without subfiles is a usage error", "[OptionsValidator]")
{
	// The depth limits a descent that is not happening, so accepting it would
	// exit 0 having done none of what was asked.
	SharedOptions shared;
	shared.subfiles.maxDepth = 3;

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.code() == ErrorCodes::subfileDepthNeedsSubfiles);
}

TEST_CASE("subfiles without a depth is an unlimited descent", "[OptionsValidator]")
{
	// The rule is one-directional: it is the depth that needs the flag.
	SharedOptions shared;
	shared.subfiles.enabled = true;

	OptionsValidator::validate(shared);

	CHECK_FALSE(shared.subfiles.maxDepth.has_value());
}

TEST_CASE("stdout with out-dir is a usage error", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.useStdout = true;
	shared.output.outDir = "out";

	CHECK_THROWS_AS(OptionsValidator::validate(shared), UsageError);
}

TEST_CASE("stdout with a file target is a usage error", "[OptionsValidator]")
{
	// Output cannot go to a file and to stdout. Neither the out-file/out-dir
	// rule nor the stdout/out-dir rule covers this pair.
	SharedOptions shared;
	shared.output.useStdout = true;
	shared.output.outFile = "book.pdf";

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("out-file dash is not a conflict because it is how stdout is spelled", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.useStdout = true;

	OptionsValidator::validate(shared);
}

TEST_CASE("the log aliases disagreeing is a usage error", "[OptionsValidator]")
{
	// --log-level and --debug-log-level are one setting. Two arguments
	// carrying values into one field would silently drop whichever lost.
	SharedOptions shared;
	shared.diagnostics.logLevel = "trace";
	shared.diagnostics.debugLogLevel = "info";

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("the log aliases agreeing is merely redundant", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.diagnostics.logLevel = "trace";
	shared.diagnostics.debugLogLevel = "trace";

	OptionsValidator::validate(shared);
	CHECK(*shared.diagnostics.effectiveLogLevel() == "trace");
}

TEST_CASE("either log alias alone resolves", "[OptionsValidator]")
{
	SharedOptions viaShort;
	viaShort.diagnostics.logLevel = "trace";
	CHECK(*viaShort.diagnostics.effectiveLogLevel() == "trace");

	SharedOptions viaDebug;
	viaDebug.diagnostics.debugLogLevel = "trace";
	CHECK(*viaDebug.diagnostics.effectiveLogLevel() == "trace");

	CHECK_FALSE(SharedOptions().diagnostics.effectiveLogLevel().has_value());
}

TEST_CASE("the module aliases behave the same way", "[OptionsValidator]")
{
	SharedOptions conflicting;
	conflicting.diagnostics.logModules = "pdf";
	conflicting.diagnostics.debugLogModules = "ocr";

	CHECK_THROWS_AS(OptionsValidator::validate(conflicting), UsageError);

	SharedOptions agreeing;
	agreeing.diagnostics.logModules = "pdf";
	agreeing.diagnostics.debugLogModules = "pdf";

	OptionsValidator::validate(agreeing);
	CHECK(*agreeing.diagnostics.effectiveLogModules() == "pdf");
}

TEST_CASE("either pattern language alone is valid", "[OptionsValidator]")
{
	// Compiling the matcher lives in binding (a later phase); this only
	// exercises the exclusivity rule below, which is still validate's job.
	SharedOptions globs;
	globs.input.globPatterns.push_back("**/*.pdf");
	OptionsValidator::validate(globs);

	SharedOptions regexes;
	regexes.input.regexPatterns.push_back("a/b\\.pdf");
	OptionsValidator::validate(regexes);
}

TEST_CASE("glob and regex together are a usage error", "[OptionsValidator]")
{
	// Two ways of saying the same thing. Combining them silently would make
	// the selection depend on argument order, and neither language subsumes
	// the other, so there is no sensible merge.
	SharedOptions shared;
	shared.input.globPatterns.push_back("**/*.pdf");
	shared.input.regexPatterns.push_back(".*\\.docx$");

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("verbose and silent together are a usage error", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.console.verbose = true;
	shared.console.silent = true;

	CHECK_THROWS_AS(OptionsValidator::validate(shared), UsageError);
}

TEST_CASE("no-recurse with an explicit deeper max-depth is a usage error", "[OptionsValidator]")
{
	// --no-recurse means depth 1. Combined with --max-depth 5 the user has
	// asked for two different things.
	SharedOptions shared;
	shared.input.maxDepth = 5;
	shared.input.noRecurseWasGiven = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("no-recurse with max-depth one is consistent", "[OptionsValidator]")
{
	// Redundant rather than contradictory: both ask for the same thing.
	SharedOptions shared;
	shared.input.maxDepth = 1;
	shared.input.noRecurseWasGiven = true;

	OptionsValidator::validate(shared);
}

TEST_CASE("no-recurse alone is fine", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.input.noRecurseWasGiven = true;

	OptionsValidator::validate(shared);
	CHECK(*shared.input.effectiveMaxDepth() == 1);
}

TEST_CASE("max-depth alone is fine", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.input.maxDepth = 5;

	OptionsValidator::validate(shared);
	CHECK(*shared.input.effectiveMaxDepth() == 5);
}

TEST_CASE("neither means unlimited depth", "[OptionsValidator]")
{
	CHECK_FALSE(SharedOptions().input.effectiveMaxDepth().has_value());
}

TEST_CASE("make-dirs with flat is a usage error", "[OptionsValidator]")
{
	// Same class of contradiction as the depth pair, and detectable for the
	// same reason: each argument records its own presence.
	ExtractOptions options;
	options.makeDirsWasGiven = true;
	options.flatWasGiven = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validateExtract(SharedOptions(), options); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("either directory layout argument alone is fine", "[OptionsValidator]")
{
	ExtractOptions makeDirs;
	makeDirs.makeDirsWasGiven = true;
	OptionsValidator::validateExtract(SharedOptions(), makeDirs);

	ExtractOptions flat;
	flat.flatWasGiven = true;
	OptionsValidator::validateExtract(SharedOptions(), flat);

	CHECK(makeDirs.replicateTree());
	CHECK_FALSE(flat.replicateTree());
	CHECK(ExtractOptions().replicateTree());
}

TEST_CASE("a per-page format with a template lacking page is a usage error", "[OptionsValidator]")
{
	// Every page would resolve to the same path: without --overwrite the run
	// fails after page one, and with it only the last page survives.
	SharedOptions shared;
	shared.output.outDir = "out";
	shared.output.nameTemplate = OutputNameTemplate::parse("{stem}.{ext}");

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, toFormat("png")); });
	CHECK(error.code() == ErrorCodes::templateNeedsPage);
}

TEST_CASE("a per-page format with a page bearing template is valid", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outDir = "out";
	shared.output.nameTemplate = OutputNameTemplate::parse("{stem}-{page:04}.{ext}");

	OptionsValidator::validateConvert(shared, toFormat("png"));
}

TEST_CASE("a single file format needs no page in its template", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outDir = "out";
	shared.output.nameTemplate = OutputNameTemplate::parse("{stem}.{ext}");

	OptionsValidator::validateConvert(shared, toFormat("pdf"));
}

TEST_CASE("a per-page format with no template is valid because the default has a page", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outDir = "out";

	OptionsValidator::validateConvert(shared, toFormat("png"));
}

TEST_CASE("fail-fast with an explicit on-error skip is a usage error", "[OptionsValidator]")
{
	// The two arguments write the same setting and contradict each other.
	// Rejected rather than resolved by precedence, because none of the four
	// parsing libraries reliably records which option came first.
	SharedOptions shared;
	shared.batch.onError = OnErrorBehaviour::Skip;
	shared.batch.onErrorWasExplicit = true;
	shared.batch.failFastWasGiven = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("fail-fast with an explicit on-error stop is consistent", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.batch.onError = OnErrorBehaviour::Stop;
	shared.batch.onErrorWasExplicit = true;
	shared.batch.failFastWasGiven = true;

	OptionsValidator::validate(shared);
}

TEST_CASE("fail-fast alone is fine", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.batch.onError = OnErrorBehaviour::Stop;
	shared.batch.failFastWasGiven = true;

	OptionsValidator::validate(shared);
}

// -------------------------------------------------------------------- convert

TEST_CASE("missing --to is a usage error", "[OptionsValidator]")
{
	// Nothing at the parser level enforces a required argument, so an
	// omitted --to leaves ConvertOptions::format null; every other convert
	// rule reads it unconditionally, so this has to be caught first.
	SharedOptions shared;
	shared.output.outDir = "out";

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, ConvertOptions()); });
	CHECK(error.code() == ErrorCodes::missingArgument);
	CHECK(error.exitCode() == ExitCode::usageError);
}

// Named without the leading dashes, unlike the flag it is about: Catch2 parses a
// test name beginning with "-" as one of its own options, so ctest -- which
// selects each case by name -- could never run it. The C# counterpart is
// To_alone_is_valid, so this also matches it more closely than "--to" did.
TEST_CASE("to alone is valid", "[OptionsValidator]")
{
	OptionsValidator::validateConvert(SharedOptions(), toFormat("pdf"));
}

TEST_CASE("combine with a one per page format is a usage error", "[OptionsValidator]")
{
	// You cannot merge documents into a format that emits a file per page.
	SharedOptions shared;
	shared.output.outFile = "out.png";
	ConvertOptions options = toFormat("png");
	options.combine = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, options); });
	CHECK(error.code() == ErrorCodes::combineUnsupported);
}

TEST_CASE("combine with a single file format is valid", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outFile = "book.pdf";
	ConvertOptions options = toFormat("pdf");
	options.combine = true;

	OptionsValidator::validateConvert(shared, options);
}

TEST_CASE("combine with text is valid", "[OptionsValidator]")
{
	// Text is a single-file format, so several documents can be concatenated.
	SharedOptions shared;
	shared.output.outFile = "all.txt";
	ConvertOptions options = toFormat("text");
	options.combine = true;

	OptionsValidator::validateConvert(shared, options);
}

TEST_CASE("stdout with a one per page format is a usage error", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.useStdout = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, toFormat("png")); });
	CHECK(error.code() == ErrorCodes::stdoutNeedsSingleFile);
}

TEST_CASE("stdout with a single file format is valid", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.useStdout = true;

	OptionsValidator::validateConvert(shared, toFormat("pdf"));
}

// ---------------------------------------------------------------- fingerprint

TEST_CASE("fingerprint with stdout is a usage error", "[OptionsValidator]")
{
	// --fingerprint renders a page only to read a fingerprint attribute off
	// it and never keeps the render, the same way doc2text's own
	// --fingerprint discards its render through a temporary stream --- so
	// streaming that now-discarded content is a contradiction, not a silent
	// no-op.
	SharedOptions shared;
	shared.output.useStdout = true;
	ConvertOptions options = toFormat("pdf");
	options.fingerprint = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, options); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("fingerprint with view is a usage error", "[OptionsValidator]")
{
	SharedOptions shared;
	ConvertOptions options = toFormat("pdf");
	options.fingerprint = true;
	options.view = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, options); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("fingerprint with combine is a usage error", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outFile = "book.pdf";
	ConvertOptions options = toFormat("pdf");
	options.fingerprint = true;
	options.combine = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, options); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("fingerprint with out-file is a usage error", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outFile = "book.pdf";
	ConvertOptions options = toFormat("pdf");
	options.fingerprint = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, options); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("fingerprint with out-dir is a usage error", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outDir = "out";
	ConvertOptions options = toFormat("pdf");
	options.fingerprint = true;

	UsageError error = expectUsageError([&] { OptionsValidator::validateConvert(shared, options); });
	CHECK(error.code() == ErrorCodes::mutuallyExclusive);
}

TEST_CASE("fingerprint with no output target at all is valid", "[OptionsValidator]")
{
	// The common case: no --out-dir, --out-file or --stdout given, so the
	// planner's own default ('.') is never something the user named.
	ConvertOptions options;
	options.format = &FormatCatalog::resolve("pdf");
	options.fingerprint = true;

	OptionsValidator::validateConvert(SharedOptions(), options);
}

TEST_CASE("fingerprint with page decorations is valid", "[OptionsValidator]")
{
	// --redact, --form and --watermark decorate the same throwaway render
	// fingerprinting already needs, so there is nothing for them to
	// contradict.
	ConvertOptions options = toFormat("pdf");
	options.fingerprint = true;
	options.watermark = "stamp.png";
	options.redactions.push_back(RedactSpec{ 1, 0, 0, 10, 10 });
	options.formValues.push_back(FormValue{ 1, "field", "value" });

	OptionsValidator::validateConvert(SharedOptions(), options);
}

TEST_CASE("every validation failure exits with the usage code", "[OptionsValidator]")
{
	SharedOptions shared;
	shared.output.outFile = "a";
	shared.output.outDir = "b";

	UsageError error = expectUsageError([&] { OptionsValidator::validate(shared); });
	CHECK(error.exitCode() == ExitCode::usageError);
}
