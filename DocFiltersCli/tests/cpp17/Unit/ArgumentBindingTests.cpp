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
//  That each argument's setter writes to the field it is supposed to.
//
//  This is the one mapping in the project that is hand-written once per
//  argument, roughly seventy-five times, which makes it the likeliest place
//  for a copy-and-paste mistake. ArgumentTableTests asks "does this argument
//  exist?"; this asks "does it do what it says?".
// ---------------------------------------------------------------------------

#include "Cli/CommandLineApp.h"
#include "Commands/ConvertCommand.h"
#include "Commands/ExtractCommand.h"
#include "Commands/FormatsCommand.h"
#include "Commands/IdentifyCommand.h"
#include "Commands/ListCommand.h"
#include "Commands/MetaCommand.h"
#include "Commands/OptionsCommand.h"
#include "Commands/VersionCommand.h"
#include "Fakes/InMemoryFileSystem.h"
#include "Fakes/RecordingConsole.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli::Cli;
using namespace Hyland::DocumentFilters::Cli::Commands;
using namespace Hyland::DocumentFilters::Cli::Configuration;
using namespace Hyland::DocumentFilters::Cli::Runtime;
using namespace Hyland::DocumentFilters::Cli::Tests::Fakes;

namespace
{
	BoundCommandLine bind(std::vector<std::string> args)
	{
		RecordingConsole console;
		InMemoryFileSystem fileSystem;
		CommandLineApp app(console, fileSystem);
		return app.bind(args);
	}

	/// <summary>A minimal valid convert invocation, with the arguments under test spliced in.</summary>
	SharedOptions sharedOptions(std::vector<std::string> arguments = {})
	{
		std::vector<std::string> args = { "convert", "--to", "text", "--out-dir", "out" };
		args.insert(args.end(), arguments.begin(), arguments.end());
		args.push_back("a.docx");
		return bind(args).sharedOptions();
	}

	ConvertOptions convertOptions(std::vector<std::string> arguments = {})
	{
		std::vector<std::string> args = { "convert", "--to", "text" };
		args.insert(args.end(), arguments.begin(), arguments.end());
		args.push_back("--out-dir");
		args.push_back("out");
		args.push_back("a.docx");
		BoundCommandLine bound = bind(args);
		return dynamic_cast<ConvertCommand&>(bound.command()).options();
	}

	ExtractOptions extractOptions(std::vector<std::string> arguments = {})
	{
		std::vector<std::string> args = { "extract" };
		args.insert(args.end(), arguments.begin(), arguments.end());
		args.push_back("--out-dir");
		args.push_back("out");
		args.push_back("a.zip");
		BoundCommandLine bound = bind(args);
		return dynamic_cast<ExtractCommand&>(bound.command()).options();
	}
}

// ====================================================== shared: output

TEST_CASE("out-dir writes to output out-dir", "[ArgumentBinding]")
{
	REQUIRE(sharedOptions().output.outDir.has_value());
	CHECK(*sharedOptions().output.outDir == "out");
}

TEST_CASE("short o is an alias for out-dir and never out-file", "[ArgumentBinding]")
{
	// The old tool's -o guessed file-or-directory from the format. It must not.
	SharedOptions shared = bind({ "convert", "--to", "text", "-o", "out", "a.docx" }).sharedOptions();

	REQUIRE(shared.output.outDir.has_value());
	CHECK(*shared.output.outDir == "out");
	CHECK_FALSE(shared.output.outFile.has_value());
}

TEST_CASE("out-file writes to output out-file and not out-dir", "[ArgumentBinding]")
{
	SharedOptions shared = bind({ "convert", "--to", "text", "--out-file", "a.txt", "a.docx" }).sharedOptions();

	REQUIRE(shared.output.outFile.has_value());
	CHECK(*shared.output.outFile == "a.txt");
	CHECK_FALSE(shared.output.outDir.has_value());
}

TEST_CASE("out-name compiles to a template", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--out-name", "{stem}-{page:04}.{ext}" });

	REQUIRE(shared.output.nameTemplate.has_value());
	CHECK(shared.output.nameTemplate->usesPage());
}

TEST_CASE("stdout sets the stdout argument", "[ArgumentBinding]")
{
	SharedOptions shared = bind({ "convert", "--to", "text", "--stdout", "a.docx" }).sharedOptions();

	CHECK(shared.output.useStdout);
}

TEST_CASE("out-file dash is the same as stdout", "[ArgumentBinding]")
{
	// Two spellings, one setting. Nothing downstream should see the difference.
	SharedOptions shared = bind({ "convert", "--to", "text", "--out-file", "-", "a.docx" }).sharedOptions();

	CHECK(shared.output.useStdout);
}

TEST_CASE("overwrite is a switch", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "--overwrite" }).output.overwrite);
	CHECK_FALSE(sharedOptions().output.overwrite);
}

// ====================================================== shared: walk

TEST_CASE("positional paths land in input paths in order", "[ArgumentBinding]")
{
	SharedOptions shared = bind({ "convert", "--to", "text", "--out-dir", "out", "a.docx", "b.docx" }).sharedOptions();

	CHECK(shared.input.paths == std::vector<std::string>{ "a.docx", "b.docx" });
}

// A token that starts with a dash but is not one of this tool's own option
// spellings is a path. looksLikeOption is the whole rule, and these shapes are
// the ones a parser library reads differently: a cluster of short options, the
// end-of-options separator, a long option, and a long option whose name begins
// with a dash. The C# [Theory] is one loop here, as elsewhere in this suite.
TEST_CASE("a dash token that is not an option spelling is a path", "[ArgumentBinding]")
{
	for (const char* token : { "-tsv", "-ab", "--", "---weird", "----" })
	{
		SharedOptions shared = bind({ "convert", "--to", "text", "--out-dir", "out", token }).sharedOptions();

		CHECK(shared.input.paths == std::vector<std::string>{ token });
	}
}

TEST_CASE("a dash shaped path keeps its position among the others", "[ArgumentBinding]")
{
	// Order is load-bearing: --combine writes the inputs in the order given.
	SharedOptions shared =
		bind({ "convert", "--to", "text", "--out-dir", "out", "a.docx", "-tsv", "b.docx" }).sharedOptions();

	CHECK(shared.input.paths == std::vector<std::string>{ "a.docx", "-tsv", "b.docx" });
}

TEST_CASE("an option value may look like a path", "[ArgumentBinding]")
{
	// The partition walks by arity, not by shape: --out-name takes a value, so
	// the next token is that value however it is spelled.
	SharedOptions shared =
		bind({ "convert", "--to", "text", "--out-dir", "out", "--out-name", "-{stem}.txt", "a.docx" }).sharedOptions();

	CHECK(shared.output.nameTemplate.has_value());
	CHECK(shared.input.paths == std::vector<std::string>{ "a.docx" });
}

TEST_CASE("files-from writes its path", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--files-from", "list.txt" });
	REQUIRE(shared.input.filesFrom.has_value());
	CHECK(*shared.input.filesFrom == "list.txt");
}

TEST_CASE("max-depth parses into its own field", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--max-depth", "3" });

	REQUIRE(shared.input.maxDepth.has_value());
	CHECK(*shared.input.maxDepth == 3);
	CHECK_FALSE(shared.input.noRecurseWasGiven);
	REQUIRE(shared.input.effectiveMaxDepth().has_value());
	CHECK(*shared.input.effectiveMaxDepth() == 3);
}

TEST_CASE("max-depth defaults to unlimited", "[ArgumentBinding]")
{
	// Directories are walked recursively by default.
	SharedOptions shared = sharedOptions();

	CHECK_FALSE(shared.input.maxDepth.has_value());
	CHECK_FALSE(shared.input.effectiveMaxDepth().has_value());
}

TEST_CASE("no-recurse does not write the depth field", "[ArgumentBinding]")
{
	// The two arguments must stay distinguishable: if --no-recurse wrote 1 into
	// maxDepth, then "--max-depth 5 --no-recurse" would be indistinguishable
	// from a plain "--max-depth 1", and the contradiction could never be
	// reported. The resolved answer comes from effectiveMaxDepth instead.
	SharedOptions shared = sharedOptions({ "--no-recurse" });

	CHECK(shared.input.noRecurseWasGiven);
	CHECK_FALSE(shared.input.maxDepth.has_value());
	REQUIRE(shared.input.effectiveMaxDepth().has_value());
	CHECK(*shared.input.effectiveMaxDepth() == 1);
}

TEST_CASE("the contradictory depth pair stays distinguishable after binding", "[ArgumentBinding]")
{
	// The state the validator has to be able to see. Binding must not collapse
	// it before validation runs.
	SharedOptions conflicting =
		bind({ "convert", "--to", "text", "--out-dir", "out", "--max-depth", "5", "--no-recurse", "a.docx" }).sharedOptions();

	REQUIRE(conflicting.input.maxDepth.has_value());
	CHECK(*conflicting.input.maxDepth == 5);
	CHECK(conflicting.input.noRecurseWasGiven);
}

TEST_CASE("follow uses capital L", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "-L" }).input.followSymlinks);
	CHECK(sharedOptions({ "--follow" }).input.followSymlinks);
}

TEST_CASE("sort and include-hidden are switches", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "--sort" }).input.sort);
	CHECK(sharedOptions({ "--include-hidden" }).input.includeHidden);
	CHECK_FALSE(sharedOptions().input.sort);
	CHECK_FALSE(sharedOptions().input.includeHidden);
}

TEST_CASE("glob accumulates and builds a matcher", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "-g", "**/*.pdf" });

	CHECK(shared.input.globPatterns == std::vector<std::string>{ "**/*.pdf" });
	CHECK(shared.input.regexPatterns.empty());
	REQUIRE(shared.input.matcher.has_value());
	CHECK(shared.input.matcher->matches("a/b.pdf"));
}

TEST_CASE("glob is repeatable and accumulates", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "-g", "**/*.pdf", "-g", "!**/draft/*" });

	CHECK(shared.input.globPatterns == std::vector<std::string>{ "**/*.pdf", "!**/draft/*" });
	CHECK(shared.input.matcher->matches("final/a.pdf"));
	CHECK_FALSE(shared.input.matcher->matches("draft/a.pdf"));
}

TEST_CASE("regex accumulates into its own list", "[ArgumentBinding]")
{
	// The two pattern languages must stay distinguishable after binding, or the
	// validator cannot tell that both were given.
	SharedOptions shared = sharedOptions({ "--regex", R"(.*\.pdf$)" });

	CHECK(shared.input.regexPatterns == std::vector<std::string>{ R"(.*\.pdf$)" });
	CHECK(shared.input.globPatterns.empty());
	REQUIRE(shared.input.matcher.has_value());
	CHECK(shared.input.matcher->matches("a/b.pdf"));
}

TEST_CASE("regex is repeatable and accumulates", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--regex", R"(.*\.pdf$)", "--regex", R"(.*\.docx$)" });

	CHECK(shared.input.regexPatterns.size() == 2);
}

TEST_CASE("neither pattern argument leaves the matcher unset", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions();

	CHECK(shared.input.globPatterns.empty());
	CHECK(shared.input.regexPatterns.empty());
	CHECK_FALSE(shared.input.matcher.has_value());
}

TEST_CASE("a malformed regex is a usage error at bind time", "[ArgumentBinding]")
{
	// Compiled in stage 2, the same as the subfile globs, so a typo is reported
	// before any directory walk or file work starts.
	CHECK_THROWS_AS(sharedOptions({ "--regex", "[unclosed" }), UsageError);
}

TEST_CASE("include and exclude types split on semicolons", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--include-types", "PDF;DOCX", "--exclude-types", "ZIP" });

	CHECK(shared.input.includeTypes == std::vector<std::string>{ "PDF", "DOCX" });
	CHECK(shared.input.excludeTypes == std::vector<std::string>{ "ZIP" });
}

// ================================================ shared: subfiles

TEST_CASE("subfiles short and long forms agree", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "-s" }).subfiles.enabled);
	CHECK(sharedOptions({ "--subfiles" }).subfiles.enabled);
	CHECK_FALSE(sharedOptions().subfiles.enabled);
}

TEST_CASE("subfile-depth is separate from directory depth", "[ArgumentBinding]")
{
	// The single most confusable pair of arguments in the tool.
	SharedOptions shared = sharedOptions({ "--max-depth", "2", "--subfile-depth", "5" });

	REQUIRE(shared.input.maxDepth.has_value());
	CHECK(*shared.input.maxDepth == 2);
	REQUIRE(shared.subfiles.maxDepth.has_value());
	CHECK(*shared.subfiles.maxDepth == 5);
}

// ================================================ shared: passwords

TEST_CASE("password is repeatable and accumulates in order", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "-p", "first", "-p", "second" });

	CHECK(shared.passwords.passwords == std::vector<std::string>{ "first", "second" });
}

TEST_CASE("password-file and prompt write their own fields", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--password-file", "pw.txt", "--password-prompt" });

	REQUIRE(shared.passwords.passwordFile.has_value());
	CHECK(*shared.passwords.passwordFile == "pw.txt");
	CHECK(shared.passwords.prompt);
}

// ================================================ shared: processing

TEST_CASE("engine options are stored verbatim and not parsed", "[ArgumentBinding]")
{
	// The CLI does not interpret this string; the engine does.
	SharedOptions shared = sharedOptions({ "--options", "BW_MODE=3;CODEPAGE=1252" });

	REQUIRE(shared.processing.engineOptions.has_value());
	CHECK(*shared.processing.engineOptions == "BW_MODE=3;CODEPAGE=1252");
}

TEST_CASE("codepage and temp-dir write their own fields", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--codepage", "1252", "--temp-dir", "/tmp/df" });

	REQUIRE(shared.processing.codepage.has_value());
	CHECK(*shared.processing.codepage == "1252");
	REQUIRE(shared.processing.tempDir.has_value());
	CHECK(*shared.processing.tempDir == "/tmp/df");
}

// ================================================ shared: batch

TEST_CASE("on-error defaults to skip", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions();

	CHECK(shared.batch.onError == OnErrorBehaviour::Skip);
	CHECK_FALSE(shared.batch.onErrorWasExplicit);
}

TEST_CASE("on-error stop is recorded as explicit", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--on-error", "stop" });

	CHECK(shared.batch.onError == OnErrorBehaviour::Stop);
	CHECK(shared.batch.onErrorWasExplicit);
}

TEST_CASE("fail-fast writes stop through the same field and flags itself", "[ArgumentBinding]")
{
	// Recorded separately so a contradictory pair can be rejected rather than
	// silently resolved.
	SharedOptions shared = sharedOptions({ "--fail-fast" });

	CHECK(shared.batch.onError == OnErrorBehaviour::Stop);
	CHECK(shared.batch.failFastWasGiven);
	CHECK_FALSE(shared.batch.onErrorWasExplicit);
}

// ================================================ shared: console, library

TEST_CASE("console switches write their own fields", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "--color" }).console.color);
	CHECK(sharedOptions({ "--verbose" }).console.verbose);
	CHECK(sharedOptions({ "--silent" }).console.silent);
}

TEST_CASE("license-key and library have no short forms", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--license-key", "KEY", "--library", "/opt/df" });

	REQUIRE(shared.library.licenseKey.has_value());
	CHECK(*shared.library.licenseKey == "KEY");
	REQUIRE(shared.library.libraryPath.has_value());
	CHECK(*shared.library.libraryPath == "/opt/df");
}

// ================================================ shared: diagnostics

TEST_CASE("timing switches write their own fields", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "--time" }).diagnostics.time);
	CHECK(sharedOptions({ "--logtime" }).diagnostics.logTime);
	CHECK(sharedOptions({ "--logmem" }).diagnostics.logMemory);
	CHECK(sharedOptions({ "--progress" }).diagnostics.progress);
	CHECK(sharedOptions({ "--pause" }).diagnostics.pause);
}

TEST_CASE("limits parse as numbers", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions({ "--max-time", "60", "--timeout", "10", "--max-memory", "512" });

	REQUIRE(shared.diagnostics.maxTime.has_value());
	CHECK(*shared.diagnostics.maxTime == 60);
	REQUIRE(shared.diagnostics.timeout.has_value());
	CHECK(*shared.diagnostics.timeout == 10);
	REQUIRE(shared.diagnostics.maxMemory.has_value());
	CHECK(*shared.diagnostics.maxMemory == 512);
}

TEST_CASE("memory status levels escalate", "[ArgumentBinding]")
{
	// -x reports once at the end of the run, --xx after every document.
	CHECK(sharedOptions().diagnostics.memoryStatusLevel == 0);
	CHECK(sharedOptions({ "-x" }).diagnostics.memoryStatusLevel == 1);
	CHECK(sharedOptions({ "--xx" }).diagnostics.memoryStatusLevel == 2);
}

TEST_CASE("the higher memory status level wins whichever order they are given", "[ArgumentBinding]")
{
	// A level, not two independent switches: --xx already includes what -x does,
	// so the pair is redundant rather than contradictory.
	CHECK(sharedOptions({ "-x", "--xx" }).diagnostics.memoryStatusLevel == 2);
	CHECK(sharedOptions({ "--xx", "-x" }).diagnostics.memoryStatusLevel == 2);
}

TEST_CASE("memory status and logmem are separate settings", "[ArgumentBinding]")
{
	// --logmem samples this process's peak memory while working; -x/--xx ask the
	// engine what it has allocated.
	SharedOptions shared = sharedOptions({ "--xx", "--logmem" });

	CHECK(shared.diagnostics.memoryStatusLevel == 2);
	CHECK(shared.diagnostics.logMemory);
}

TEST_CASE("the log aliases write their own fields and resolve to one value", "[ArgumentBinding]")
{
	// Each alias keeps its own field so the validator can see that both were
	// given; effectiveLogLevel is what everything downstream reads.
	SharedOptions viaShort = sharedOptions({ "--log-level", "trace" });
	REQUIRE(viaShort.diagnostics.logLevel.has_value());
	CHECK(*viaShort.diagnostics.logLevel == "trace");
	CHECK_FALSE(viaShort.diagnostics.debugLogLevel.has_value());
	REQUIRE(viaShort.diagnostics.effectiveLogLevel().has_value());
	CHECK(*viaShort.diagnostics.effectiveLogLevel() == "trace");

	SharedOptions viaDebug = sharedOptions({ "--debug-log-level", "trace" });
	CHECK_FALSE(viaDebug.diagnostics.logLevel.has_value());
	REQUIRE(viaDebug.diagnostics.debugLogLevel.has_value());
	CHECK(*viaDebug.diagnostics.debugLogLevel == "trace");
	REQUIRE(viaDebug.diagnostics.effectiveLogLevel().has_value());
	CHECK(*viaDebug.diagnostics.effectiveLogLevel() == "trace");
}

TEST_CASE("the module aliases write their own fields too", "[ArgumentBinding]")
{
	REQUIRE(sharedOptions({ "--log-modules", "pdf" }).diagnostics.effectiveLogModules().has_value());
	CHECK(*sharedOptions({ "--log-modules", "pdf" }).diagnostics.effectiveLogModules() == "pdf");
	CHECK(*sharedOptions({ "--debug-log-modules", "pdf" }).diagnostics.effectiveLogModules() == "pdf");
	CHECK_FALSE(sharedOptions({ "--log-modules", "pdf" }).diagnostics.debugLogModules.has_value());
}

TEST_CASE("the remaining debug switches write their own fields", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "--debug-use-memory-stream" }).diagnostics.useMemoryStream);
	CHECK(sharedOptions({ "--debug-measure-io" }).diagnostics.measureIo);
	REQUIRE(sharedOptions({ "--debug-truncate", "4096" }).diagnostics.truncateInputBytes.has_value());
	CHECK(*sharedOptions({ "--debug-truncate", "4096" }).diagnostics.truncateInputBytes == 4096);
}

TEST_CASE("dry-run and print-plan write the same field", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "--dry-run" }).dryRun);
	CHECK(sharedOptions({ "--print-plan" }).dryRun);
	CHECK_FALSE(sharedOptions().dryRun);
}

TEST_CASE("print-contract writes its own field", "[ArgumentBinding]")
{
	CHECK(sharedOptions({ "--print-contract" }).printContract);
}

// ====================================================== convert

TEST_CASE("to resolves through the format catalogue", "[ArgumentBinding]")
{
	ConvertOptions options = convertOptions();

	REQUIRE(options.format != nullptr);
	CHECK(options.format->token == "text");
}

TEST_CASE("fallback is no longer an argument", "[ArgumentBinding]")
{
	// Kept as a test rather than deleted with the option: it was accepted for long
	// enough to appear in scripts, and "unknown argument" is the answer those should
	// get --- not silent acceptance, and not a crash.
	try
	{
		convertOptions({ "--fallback", "text" });
		FAIL("expected a UsageError");
	}
	catch (const CliError& error)
	{
		CHECK(error.code() == ErrorCodes::unknownArgument);
		CHECK(error.exitCode() == ExitCode::usageError);
	}
}

TEST_CASE("combine and the simple switches write their own fields", "[ArgumentBinding]")
{
	CHECK(convertOptions({ "--combine" }).combine);
	CHECK(convertOptions({ "--no-images" }).noImages);
	CHECK(convertOptions({ "--include-control-codes" }).includeControlCodes);
	CHECK(convertOptions({ "--fingerprint" }).fingerprint);
	CHECK(convertOptions({ "--view" }).view);
}

TEST_CASE("canvas and render options do not share a field", "[ArgumentBinding]")
{
	ConvertOptions options = convertOptions({ "--canvas-options", "A=1", "--render-options", "B=2" });

	REQUIRE(options.canvasOptions.has_value());
	CHECK(*options.canvasOptions == "A=1");
	REQUIRE(options.renderOptions.has_value());
	CHECK(*options.renderOptions == "B=2");
}

TEST_CASE("pages compiles to a range", "[ArgumentBinding]")
{
	ConvertOptions options = convertOptions({ "--pages", "1,3-5" });

	REQUIRE(options.pages.has_value());
	Hyland::DocumentFilters::Cli::Planning::PageRange::Pages pages = options.pages->enumerate(10);
	CHECK(std::vector<int>(pages.begin(), pages.end()) == std::vector<int>{ 1, 3, 4, 5 });
}

TEST_CASE("watermark writes its own field", "[ArgumentBinding]")
{
	ConvertOptions options = convertOptions({ "--watermark", "DRAFT" });
	REQUIRE(options.watermark.has_value());
	CHECK(*options.watermark == "DRAFT");
}

TEST_CASE("redact is repeatable and each occurrence is parsed", "[ArgumentBinding]")
{
	ConvertOptions options = convertOptions({ "--redact", "2,10,20,100,50", "--redact", "3,0,0,50,50" });

	REQUIRE(options.redactions.size() == 2);
	CHECK(options.redactions[0].page == 2);
	CHECK(options.redactions[0].width == 100);
	CHECK(options.redactions[1].page == 3);
}

TEST_CASE("form is repeatable and each occurrence is parsed", "[ArgumentBinding]")
{
	ConvertOptions options = convertOptions({ "--form", "1,a=x", "--form", "2,b=y" });

	REQUIRE(options.formValues.size() == 2);
	CHECK(options.formValues[0].name == "a");
	CHECK(options.formValues[1].value == "y");
}

TEST_CASE("each structure report argument sets exactly one report", "[ArgumentBinding]")
{
	// Ten near-identical booleans: the highest copy-and-paste risk in the table.
	// Exactly one must flip, and any() has to notice.
	for (const std::string& argument :
		{ "--fonts", "--font-mappings", "--page-fonts", "--page-font-mappings", "--dimensions", "--page-elements", "--form-elements",
			"--hyperlinks", "--bookmarks", "--annotations" })
	{
		ConvertOptions options = convertOptions({ argument });

		CHECK(options.reports.any());

		int enabled = 0;
		for (bool flag : { options.reports.fonts, options.reports.fontMappings, options.reports.pageFonts,
				 options.reports.pageFontMappings, options.reports.dimensions, options.reports.pageElements,
				 options.reports.formElements, options.reports.hyperlinks, options.reports.bookmarks, options.reports.annotations })
		{
			if (flag)
				++enabled;
		}
		CHECK(enabled == 1);
	}
}

TEST_CASE("no structure reports are on by default", "[ArgumentBinding]")
{
	CHECK_FALSE(convertOptions().reports.any());
}

// ====================================================== extract

TEST_CASE("images short and long forms agree", "[ArgumentBinding]")
{
	CHECK(extractOptions({ "-i" }).images);
	CHECK(extractOptions({ "--images" }).images);
	CHECK_FALSE(extractOptions().images);
}

TEST_CASE("subfile name filters do not share a field", "[ArgumentBinding]")
{
	ExtractOptions options = extractOptions({ "--include", "*invoice*", "--exclude", "*draft*" });

	REQUIRE(options.subfileInclude.has_value());
	REQUIRE(options.subfileExclude.has_value());
	CHECK(options.subfileInclude->matches("2026-invoice-01.pdf"));
	CHECK(options.subfileExclude->matches("draft-notes.txt"));
}

TEST_CASE("subfile filters are globs compiled during binding", "[ArgumentBinding]")
{
	// The same pattern language as -g/--glob, compiled in stage 2 so a
	// malformed pattern is a usage error before any archive is opened. A
	// different axis though: -g picks files on disk, this picks subfiles
	// inside a container.
	ExtractOptions options = extractOptions({ "--include", "**/*.docx" });

	CHECK(options.subfileInclude->matches("reports/q1.docx"));
	CHECK_FALSE(options.subfileInclude->matches("reports/q1.pdf"));
}

TEST_CASE("subfile filters are repeatable like the path globs", "[ArgumentBinding]")
{
	// Repeatable for the same reason -g is: one glob per idea reads better
	// than one glob doing several jobs.
	ExtractOptions options = extractOptions({ "--include", "*.docx", "--include", "*.xlsx" });

	CHECK(options.includePatterns == std::vector<std::string>{ "*.docx", "*.xlsx" });
	CHECK(options.subfileInclude->matches("q1.docx"));
	CHECK(options.subfileInclude->matches("q1.xlsx"));
	CHECK_FALSE(options.subfileInclude->matches("q1.pdf"));
}

TEST_CASE("include and exclude may both be given", "[ArgumentBinding]")
{
	// Unlike -g and --regex, which are two spellings of one idea and
	// contradict, these are complementary: keep these, then drop those.
	ExtractOptions options = extractOptions({ "--include", "*.docx", "--exclude", "draft-*" });

	CHECK(options.subfileInclude->matches("q1.docx"));
	CHECK(options.subfileExclude->matches("draft-q1.docx"));
}

TEST_CASE("a malformed subfile glob is a usage error", "[ArgumentBinding]")
{
	CHECK_THROWS_AS(extractOptions({ "--include", "[" }), UsageError);
}

TEST_CASE("extract regex selects paths like it does everywhere else", "[ArgumentBinding]")
{
	// --regex means one thing everywhere: select paths by regular expression.
	// extract accepts it like any other verb that walks a directory.
	SharedOptions shared = bind({ "extract", "--regex", R"(.*\.zip$)", "-o", "out", "archives" }).sharedOptions();

	REQUIRE(shared.input.matcher.has_value());
	CHECK(shared.input.matcher->matches("a/b.zip"));
}

TEST_CASE("extract has no way to make the subfile filters regular expressions", "[ArgumentBinding]")
{
	// The subfile filters are globs, with no mode switch to make them regular
	// expressions. --regex on extract selects paths, as it does everywhere.
	CHECK_THROWS_AS(bind({ "extract", "--include", "inv.*", "--content-regex", "-o", "out", "a.zip" }), UsageError);
}

TEST_CASE("make-dirs is the default and flat turns it off", "[ArgumentBinding]")
{
	CHECK(extractOptions().replicateTree());
	CHECK(extractOptions({ "--make-dirs" }).replicateTree());
	CHECK_FALSE(extractOptions({ "--flat" }).replicateTree());
}

TEST_CASE("the two layout arguments each record their own presence", "[ArgumentBinding]")
{
	// So that --make-dirs --flat can be rejected rather than silently
	// resolved, for the same reason --max-depth and --no-recurse stay apart.
	CHECK(extractOptions({ "--make-dirs" }).makeDirsWasGiven);
	CHECK_FALSE(extractOptions({ "--make-dirs" }).flatWasGiven);
	CHECK(extractOptions({ "--flat" }).flatWasGiven);
	CHECK_FALSE(extractOptions({ "--flat" }).makeDirsWasGiven);
	CHECK_FALSE(extractOptions().makeDirsWasGiven);
	CHECK_FALSE(extractOptions().flatWasGiven);
}

TEST_CASE("sub-id writes its own field", "[ArgumentBinding]")
{
	CHECK(extractOptions({ "--sub-id" }).appendSubfileId);
}

// ====================================================== other verbs

TEST_CASE("json binds on each listing command", "[ArgumentBinding]")
{
	CHECK(dynamic_cast<ListCommand&>(bind({ "list", "--json", "a.zip" }).command()).options().json);
	CHECK(dynamic_cast<IdentifyCommand&>(bind({ "identify", "--json", "a.bin" }).command()).options().json);
	CHECK(dynamic_cast<MetaCommand&>(bind({ "meta", "--json", "a.pdf" }).command()).options().json);
	CHECK(dynamic_cast<FormatsCommand&>(bind({ "formats", "--json" }).command()).options().json);
	CHECK(dynamic_cast<OptionsCommand&>(bind({ "options", "--json" }).command()).options().json);
}

TEST_CASE("identify capabilities binds", "[ArgumentBinding]")
{
	CHECK(dynamic_cast<IdentifyCommand&>(bind({ "identify", "--capabilities", "a.bin" }).command()).options().capabilities);
}

TEST_CASE("formats filter binds", "[ArgumentBinding]")
{
	// bound must be a named local, not a temporary: options below is a
	// reference into it, and a temporary's lifetime would end at this
	// statement's semicolon, leaving that reference dangling.
	BoundCommandLine bound = bind({ "formats", "--filter", "pdf" });
	const FormatsOptions& options = dynamic_cast<FormatsCommand&>(bound.command()).options();

	REQUIRE(options.filter.has_value());
	CHECK(*options.filter == "pdf");
	CHECK_FALSE(options.json);
}

TEST_CASE("options filter and applies-to do not share a field", "[ArgumentBinding]")
{
	BoundCommandLine bound = bind({ "options", "--filter", "BW", "--applies-to", "document" });
	const OptionCatalogOptions& options = dynamic_cast<OptionsCommand&>(bound.command()).options();

	REQUIRE(options.filter.has_value());
	CHECK(*options.filter == "BW");
	REQUIRE(options.appliesTo.has_value());
	CHECK(*options.appliesTo == "document");
}

// ====================================================== arity behaviour

TEST_CASE("a single value argument given twice keeps the last", "[ArgumentBinding]")
{
	SharedOptions shared =
		bind({ "convert", "--to", "text", "-o", "first", "-o", "second", "a.docx" }).sharedOptions();

	REQUIRE(shared.output.outDir.has_value());
	CHECK(*shared.output.outDir == "second");
}

TEST_CASE("an absent argument leaves its default untouched", "[ArgumentBinding]")
{
	SharedOptions shared = sharedOptions();

	CHECK_FALSE(shared.processing.codepage.has_value());
	CHECK_FALSE(shared.output.outFile.has_value());
	CHECK(shared.passwords.passwords.empty());
	CHECK(shared.batch.onError == OnErrorBehaviour::Skip);
}

TEST_CASE("verb selection produces the right command type", "[ArgumentBinding]")
{
	CHECK(dynamic_cast<ConvertCommand*>(&bind({ "convert", "--to", "text", "-o", "out", "a.docx" }).command()) != nullptr);
	CHECK(dynamic_cast<ExtractCommand*>(&bind({ "extract", "-o", "out", "a.zip" }).command()) != nullptr);
	CHECK(dynamic_cast<ListCommand*>(&bind({ "list", "a.zip" }).command()) != nullptr);
	CHECK(dynamic_cast<IdentifyCommand*>(&bind({ "identify", "a.bin" }).command()) != nullptr);
	CHECK(dynamic_cast<MetaCommand*>(&bind({ "meta", "a.pdf" }).command()) != nullptr);
	CHECK(dynamic_cast<FormatsCommand*>(&bind({ "formats" }).command()) != nullptr);
	CHECK(dynamic_cast<OptionsCommand*>(&bind({ "options" }).command()) != nullptr);
	CHECK(dynamic_cast<VersionCommand*>(&bind({ "version" }).command()) != nullptr);
}

// ====================================================== rejection

TEST_CASE("an unknown verb is a usage error", "[ArgumentBinding]")
{
	try
	{
		bind({ "nonsense", "a.docx" });
		FAIL("expected a UsageError");
	}
	catch (const UsageError& error)
	{
		CHECK(error.code() == ErrorCodes::unknownCommand);
	}
}

TEST_CASE("an unknown argument is a usage error", "[ArgumentBinding]")
{
	try
	{
		bind({ "convert", "--to", "text", "--nonsense", "a.docx" });
		FAIL("expected a UsageError");
	}
	catch (const UsageError& error)
	{
		CHECK(error.code() == ErrorCodes::unknownArgument);
	}
}

TEST_CASE("prefix abbreviation does not resolve", "[ArgumentBinding]")
{
	// --out-d must not resolve to --out-dir. Prefix abbreviation is a common
	// parsing-library default, and none of the four allows it.
	CHECK_THROWS_AS(bind({ "convert", "--to", "text", "--out-d", "out", "a.docx" }), UsageError);
}

TEST_CASE("an argument from another verb is rejected", "[ArgumentBinding]")
{
	// --combine belongs to convert; extract must not quietly accept it.
	CHECK_THROWS_AS(bind({ "extract", "--combine", "-o", "out", "a.zip" }), UsageError);
}

TEST_CASE("a bad value is reported by us and not by the parsing library", "[ArgumentBinding]")
{
	// The error code proves the message came from ValueParsers, which is what
	// keeps stderr identical across the four ports.
	try
	{
		bind({ "convert", "--to", "text", "--max-depth", "abc", "-o", "out", "a.docx" });
		FAIL("expected a UsageError");
	}
	catch (const UsageError& error)
	{
		CHECK(error.code() == ErrorCodes::badValue);
	}
}
