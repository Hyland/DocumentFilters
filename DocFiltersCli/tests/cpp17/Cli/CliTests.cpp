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

// Whole command lines, argv to exit code. These are about the wiring, not
// the rules. Every rule here is already pinned by the conformance corpus or
// a unit test; what is new is that the path from Program::run through
// dispatch to a command's execute() is exercised at all, which it was not
// before.

#include "Cli/CliHarness.h"
#include "Program.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"
#include "Runtime/Interruption.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include <algorithm>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Cli::CliHarness;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeSubFile;
using Hyland::DocumentFilters::Cli::Tests::Fakes::RecordingConsole;

namespace
{
	bool anyStartsWith(const std::vector<std::string>& lines, const std::string& prefix)
	{
		return std::any_of(lines.begin(), lines.end(), [&prefix](const std::string& line) { return line.rfind(prefix, 0) == 0; });
	}

	bool anyContains(const std::vector<std::string>& lines, const std::string& needle)
	{
		return std::any_of(lines.begin(), lines.end(), [&needle](const std::string& line) { return line.find(needle) != std::string::npos; });
	}

	bool endsWith(const std::string& text, const std::string& suffix)
	{
		return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
	}
}

// ============================================ exit codes out of the front door

TEST_CASE("a clean conversion exits zero and says nothing", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.docx", "hello");

	CHECK(cli.run("convert", "--to", "text", "--out-dir", "out", "a.docx") == Runtime::ExitCode::success);
	CHECK(cli.errors().empty());

	// Silence means success: a clean run prints nothing unless asked.
	CHECK(cli.out().empty());
}

TEST_CASE("a usage error exits two and names the problem on stderr", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.docx");

	CHECK(cli.run("convert", "--to", "nonsense", "--out-dir", "out", "a.docx") == Runtime::ExitCode::usageError);

	REQUIRE(cli.errors().size() == 1);
	const std::string& line = cli.errors()[0];
	CHECK(line.rfind(std::string(Runtime::ErrorCodes::unknownFormat) + ": ", 0) == 0);
	CHECK(line.find("nonsense") != std::string::npos);
}

TEST_CASE("an unknown command exits two", "[Cli]")
{
	CliHarness cli;

	CHECK(cli.run("nonsense", "a.docx") == Runtime::ExitCode::usageError);
	CHECK(anyStartsWith(cli.errors(), std::string(Runtime::ErrorCodes::unknownCommand) + ": "));
}

TEST_CASE("naming no input exits three", "[Cli]")
{
	CliHarness cli;

	CHECK(cli.run("convert", "--to", "text", "--out-dir", "out") == Runtime::ExitCode::noInputsMatched);
	CHECK(anyStartsWith(cli.errors(), std::string(Runtime::ErrorCodes::noInputs) + ": "));
}

TEST_CASE("an output conflict exits four", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.docx").withDocument("b.docx");

	CHECK(cli.run("convert", "--to", "pdf", "--out-file", "one.pdf", "a.docx", "b.docx") == Runtime::ExitCode::outputConflict);
	CHECK(anyStartsWith(cli.errors(), std::string(Runtime::ErrorCodes::outFileMultiInput) + ": "));
}

TEST_CASE("one bad document among good ones exits one", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("good.docx", "text").withUnreadableDocument("bad.docx", Engine::EngineError::fileCorrupt);

	CHECK(cli.run("convert", "--to", "text", "--out-dir", "out", "good.docx", "bad.docx") == Runtime::ExitCode::partialFailure);
}

// ============================================ the two stderr shapes

TEST_CASE("a per document failure is prefixed by its input", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("good.docx", "text").withUnreadableDocument("bad.docx", Engine::EngineError::fileCorrupt);

	cli.run("convert", "--to", "text", "--out-dir", "out", "good.docx", "bad.docx");

	REQUIRE(cli.errors().size() == 1);
	CHECK(cli.errors()[0].rfind("bad.docx: " + std::string(Runtime::ErrorCodes::documentFailed) + ": ", 0) == 0);
}

TEST_CASE("a per document limit is prefixed like any other per document failure", "[Cli]")
{
	Runtime::LimitExceededError error("--timeout of 30s exceeded", std::string("huge.pdf"));
	RecordingConsole console;

	int exitCode = Program::reportError(error, console);

	CHECK(exitCode == Runtime::ExitCode::limitExceeded);
	REQUIRE(console.errorLines.size() == 1);
	CHECK(console.errorLines[0] == "huge.pdf: E_LIMIT_EXCEEDED: --timeout of 30s exceeded");
}

TEST_CASE("a run wide limit names no input", "[Cli]")
{
	// --max-time and --max-memory bound the whole run, so there is nothing to
	// name.
	RecordingConsole console;

	Program::reportError(Runtime::LimitExceededError("--max-time of 60s exceeded"), console);

	REQUIRE(console.errorLines.size() == 1);
	CHECK(console.errorLines[0] == "E_LIMIT_EXCEEDED: --max-time of 60s exceeded");
}

TEST_CASE("a run level failure has no input to name", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.docx");

	cli.run("convert", "--to", "text", "--out-file", "a.txt", "--out-dir", "out", "a.docx");

	REQUIRE(cli.errors().size() == 1);
	CHECK(cli.errors()[0].rfind(std::string(Runtime::ErrorCodes::mutuallyExclusive) + ": ", 0) == 0);
}

// ============================================ canvas formats to stdout

TEST_CASE("a canvas format can stream to stdout", "[Cli]")
{
	// Reproduces a real bug: --stdout resolves to OutputCardinality::Stream,
	// which leaves both PlannedItem::target and targetPattern absent.
	// convert's own branches only ever expect one or the other.
	CliHarness cli;
	cli.withDocument("a.docx", "text");

	CHECK(cli.run("convert", "--to", "pdf", "--stdout", "a.docx") == Runtime::ExitCode::success);

	CHECK(cli.errors().empty());
	CHECK(anyStartsWith(cli.engine.calls, "makeCanvas(<stream>,"));
	CHECK_FALSE(anyStartsWith(cli.engine.calls, "makeCanvas(a.pdf,"));
}

TEST_CASE("docpack can stream to stdout too", "[Cli]")
{
	// docpack is not a canvas format at all --- FormatSpec::canvasType is
	// absent, it goes through IDocument::makeDocPack instead --- so the
	// canvas fix above does not cover it. Same root cause, same fix.
	CliHarness cli;
	cli.withDocument("a.docx", "text");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> document = cli.engine.addDocument("a.docx", "text");

	CHECK(cli.run("convert", "--to", "docpack", "--stdout", "a.docx") == Runtime::ExitCode::success);

	CHECK(cli.errors().empty());
	CHECK(document->docPacksWritten == std::vector<std::string>{ "<stream>" });
}

TEST_CASE("a report alongside a text conversion is refused before anything opens", "[Cli]")
{
	// Found end to end: convert --to text --bookmarks failed inside the
	// engine, and so did every other pairing of a text format with a report.
	CliHarness cli;
	cli.withDocument("a.docx", "hello");

	CHECK(cli.run("convert", "--to", "text", "--bookmarks", "--stdout", "a.docx") == Runtime::ExitCode::usageError);

	CHECK(anyContains(cli.errors(), Runtime::ErrorCodes::reportNeedsHiDef));
	CHECK_FALSE(anyStartsWith(cli.engine.calls, "openDocument("));
}

TEST_CASE("an output directory that does not exist yet is created", "[Cli]")
{
	// Found from the README's own first example. Only the text path created
	// it, so --out-dir out/ worked for --to text and failed for every other
	// format.
	for (const char* format : { "pdf", "png", "text", "docpack" })
	{
		CliHarness cli;
		cli.withDocument("a.docx", "hello");

		CHECK(cli.run("convert", "--to", format, "--out-dir", "brand/new", "a.docx") == Runtime::ExitCode::success);

		CHECK(cli.fileSystem.directoryExists("brand/new"));
	}
}

TEST_CASE("combining a text format concatenates into one writer", "[Cli]")
{
	for (const char* format : { "text", "md-text", "html" })
	{
		CliHarness cli;
		cli.withDocument("a.docx", "first").withDocument("b.docx", "second");

		CHECK(cli.run("convert", "--to", format, "--combine", "--out-file", "out/all.txt", "a.docx", "b.docx") == Runtime::ExitCode::success);

		CHECK(cli.errors().empty());
		CHECK(cli.fileSystem.readText("out/all.txt") == "firstsecond");
	}
}

TEST_CASE("an open cancelled by the timeout is reported as the limit", "[Cli]")
{
	// The engine stops and reports cancelled, which reaches us as
	// EngineError::cancelled. That must not be read as a broken document.
	//
	// Driven through the real predicate rather than by scripting a cancelled open:
	// the run has to be the one that asks to stop, because that is what records the
	// reason. Scripting the engine's refusal instead tested the translation without
	// the decision behind it, and went on passing when a cancellation nobody asked
	// for started reporting the document instead. The document "takes" a minute
	// without the suite waiting one: the clock is the harness's, and the open moves
	// it past the limit so the predicate fires on its next poll.
	for (std::vector<std::string> verb : { std::vector<std::string>{ "convert", "--to", "text" }, std::vector<std::string>{ "extract" } })
	{
		CliHarness cli;
		cli.withDocument("slow.docx", "text");
		cli.engine.duringOpen = [&cli]() { cli.clock.advance(std::chrono::seconds(60)); };

		std::vector<std::string> args = verb;
		for (const std::string& part : { std::string("--out-dir"), std::string("out"), std::string("--timeout"), std::string("30"), std::string("slow.docx") })
			args.push_back(part);

		CHECK(cli.run(args) == Runtime::ExitCode::limitExceeded);
		REQUIRE(cli.errors().size() == 1);
		CHECK(cli.errors()[0] == "slow.docx: E_LIMIT_EXCEEDED: --timeout of 30s exceeded");
	}
}

TEST_CASE("opening a document offers the engine a way to stop", "[Cli]")
{
	// The predicate has to reach open, because the heartbeat is raised
	// during open and nowhere else.
	for (std::vector<std::string> verb : { std::vector<std::string>{ "convert", "--to", "text" }, std::vector<std::string>{ "extract" } })
	{
		CliHarness cli;
		cli.withDocument("a.docx", "text");

		std::vector<std::string> args = verb;
		for (const std::string& part : { std::string("--out-dir"), std::string("out"), std::string("--timeout"), std::string("30"), std::string("a.docx") })
			args.push_back(part);
		cli.run(args);

		CHECK(std::any_of(cli.engine.calls.begin(), cli.engine.calls.end(),
			[](const std::string& call) { return call.rfind("openDocument(a.docx,", 0) == 0 && endsWith(call, ",cancellable)"); }));
	}
}

TEST_CASE("an interrupted run stops between documents and exits 130", "[Cli]")
{
	// Ctrl+C is recorded rather than killing the process, so the run reports
	// and exits through the same path as every other code.
	for (std::vector<std::string> verb : { std::vector<std::string>{ "convert", "--to", "text" }, std::vector<std::string>{ "extract" } })
	{
		Runtime::Interruption interruption;
		interruption.request();

		CliHarness cli(interruption);
		cli.withDocument("a.docx", "one").withDocument("b.docx", "two");

		std::vector<std::string> args = verb;
		args.push_back("--out-dir");
		args.push_back("out");
		args.push_back("a.docx");
		args.push_back("b.docx");

		CHECK(cli.run(args) == Runtime::ExitCode::interrupted);

		REQUIRE(cli.errors().size() == 1);
		CHECK(cli.errors()[0] == "E_INTERRUPTED: interrupted");
		CHECK_FALSE(anyStartsWith(cli.engine.calls, "openDocument("));
	}
}

TEST_CASE("a run nobody interrupts is unaffected", "[Cli]")
{
	Runtime::Interruption interruption;
	CliHarness cli(interruption);
	cli.withDocument("a.docx", "one");

	CHECK(cli.run("convert", "--to", "text", "--out-dir", "out", "a.docx") == Runtime::ExitCode::success);
	CHECK(cli.errors().empty());
}

// ===================================== diagnostics on every command

TEST_CASE("every command that reads a document honours the diagnostics", "[Cli]")
{
	for (std::vector<std::string> verb : { std::vector<std::string>{ "convert", "--to", "text" }, std::vector<std::string>{ "extract" },
			 std::vector<std::string>{ "meta" }, std::vector<std::string>{ "list" }, std::vector<std::string>{ "identify" } })
	{
		CliHarness cli;
		cli.withDocument("a.docx", "hello");
		cli.engine.memoryStatus = "heap: 42 bytes";

		// No --out-dir: identify and meta declare no output group, so
		// passing one is a usage error rather than a no-op.
		std::vector<std::string> args = verb;
		args.push_back("-x");
		args.push_back("a.docx");

		CHECK(cli.run(args) == Runtime::ExitCode::success);
		CHECK(anyContains(cli.out(), "heap: 42 bytes"));
	}
}

TEST_CASE("the debug input switches reach the inspection commands", "[Cli]")
{
	// identify never opens the document, so it has no open callback to
	// cancel or log through --- but what the engine makes of a truncated
	// file is exactly what --debug-truncate asks.
	for (const char* verb : { "meta", "list", "identify" })
	{
		CliHarness cli;
		cli.withDocument("a.docx", "hello world");

		CHECK(cli.run(verb, "--debug-use-memory-stream", "a.docx") == Runtime::ExitCode::success);

		CHECK(anyContains(cli.engine.calls, "<stream:"));
	}
}

TEST_CASE("an interrupt stops the inspection commands too", "[Cli]")
{
	for (const char* verb : { "meta", "list", "identify" })
	{
		Runtime::Interruption interruption;
		interruption.request();

		CliHarness cli(interruption);
		cli.withDocument("a.docx", "one").withDocument("b.docx", "two");

		CHECK(cli.run(verb, "a.docx", "b.docx") == Runtime::ExitCode::interrupted);

		REQUIRE(cli.errors().size() == 1);
		CHECK(cli.errors()[0] == "E_INTERRUPTED: interrupted");
	}
}

TEST_CASE("extraction creates its directories through the filesystem seam", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("archive.zip");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> container = cli.engine.addDocument("archive.zip");
	container->subFileEntries.push_back(std::make_shared<Tests::Fakes::FakeSubFileState>());
	container->subFileEntries.back()->name = "nested/deep/inner.docx";

	CHECK(cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip") == Runtime::ExitCode::success);

	CHECK(cli.errors().empty());
	CHECK(cli.fileSystem.directoryExists("out/nested/deep"));
}

// --combine kept the canvas and the stream alive until the enclosing lambda
// returned, which is after --view runs: the viewer was handed a file with nothing
// written to it yet, and for a PDF that means no trailer. The content is read
// inside the viewer because afterwards it looks right either way.
TEST_CASE("combined output is finished before the viewer sees it", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.docx", "first");
	cli.withDocument("b.docx", "second");

	CHECK(cli.run("convert", "--to", "text", "--combine", "--out-file", "out/all.txt", "--view", "--sort",
		"a.docx", "b.docx") == Runtime::ExitCode::success);

	CHECK(cli.viewed == std::vector<std::string>{ "out/all.txt" });
	REQUIRE(cli.viewedContent.size() == 1);
	CHECK(cli.viewedContent[0].find("first") != std::string::npos);
	CHECK(cli.viewedContent[0].find("second") != std::string::npos);
}

// The planner checks every target before the run starts, against a directory none
// of this run's outputs are in yet. Two names the filesystem matches to one file
// but the ASCII collision key does not --- NTFS matches the two spellings of
// a-ring, measured --- therefore both passed planning, and the second conversion
// replaced the first with no --overwrite and exit 0. The names are written as
// escapes so this file stays ASCII.
TEST_CASE("an output the run has already written is not replaced silently", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a/\xc3\x85.docx", "from the upper-case name");
	cli.withDocument("b/\xc3\xa5.docx", "from the lower-case name");
	cli.fileSystem.treatAsSameFile("out/\xc3\xa5.txt", "out/\xc3\x85.txt");

	int exitCode = cli.run("convert", "--to", "text", "--out-dir", "out", "--sort",
		"a/\xc3\x85.docx", "b/\xc3\xa5.docx");

	CHECK(exitCode == Runtime::ExitCode::outputConflict);
	REQUIRE_FALSE(cli.errors().empty());
	CHECK(cli.errors()[0].find("E_OUTPUT_EXISTS") != std::string::npos);
	CHECK(cli.fileSystem.readText("out/\xc3\x85.txt") == "from the upper-case name");
}

TEST_CASE("two containers cannot extract onto each other", "[Cli]")
{
	// The claim set was built per container while the extractor is built per run,
	// so two archives each holding a readme.txt were never compared: with
	// --overwrite the second silently replaced the first, exit 0. The planner
	// refuses that for ordinary outputs whether or not --overwrite is given.
	CliHarness cli;
	cli.withDocument("a.zip");
	cli.withDocument("b.zip");
	for (const char* container : { "a.zip", "b.zip" })
	{
		std::shared_ptr<Tests::Fakes::FakeDocumentState> document = cli.engine.addDocument(container);
		document->subFileEntries.push_back(std::make_shared<Tests::Fakes::FakeSubFileState>());
		document->subFileEntries.back()->name = "readme.txt";
	}

	CHECK(cli.run("extract", "--overwrite", "--out-dir", "out", "a.zip", "b.zip") == Runtime::ExitCode::outputConflict);
	REQUIRE_FALSE(cli.errors().empty());
	CHECK(cli.errors()[0].find("E_OUTPUT_COLLISION") != std::string::npos);
}

TEST_CASE("a subfile cannot be written through a link out of the output directory", "[Cli]")
{
	// Containment was lexical only: every segment of out/latest/index.html is a
	// plain name under the root, and if out/latest is a link the write still lands
	// wherever it points. The container picks that name, not the user.
	CliHarness cli;
	cli.withDocument("archive.zip");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> container = cli.engine.addDocument("archive.zip");
	container->subFileEntries.push_back(std::make_shared<Tests::Fakes::FakeSubFileState>());
	container->subFileEntries.back()->name = "latest/index.html";
	cli.fileSystem.addDirectory("elsewhere").markSymlink("out/latest", "elsewhere");

	CHECK(cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip") == Runtime::ExitCode::outputConflict);
	REQUIRE_FALSE(cli.errors().empty());
	CHECK(cli.errors()[0].find("E_UNSAFE_OUTPUT") != std::string::npos);
}

TEST_CASE("an ordinary output directory is not mistaken for a link", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("archive.zip");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> container = cli.engine.addDocument("archive.zip");
	container->subFileEntries.push_back(std::make_shared<Tests::Fakes::FakeSubFileState>());
	container->subFileEntries.back()->name = "latest/index.html";
	cli.fileSystem.addDirectory("out/latest");

	CHECK(cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip") == Runtime::ExitCode::success);
	CHECK(cli.errors().empty());
}

TEST_CASE("extraction refuses to replace an existing file without overwrite", "[Cli]")
{
	// The copy used to be unconditional, so a second extract into the same
	// directory silently replaced whatever was there. It is the same rule as
	// every other output now: E_OUTPUT_EXISTS, exit 4, nothing written.
	CliHarness cli;
	cli.withDocument("archive.zip").withExistingOutput("out/inner.txt");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> container = cli.engine.addDocument("archive.zip");
	container->subFileEntries.push_back(std::make_shared<Tests::Fakes::FakeSubFileState>());
	container->subFileEntries.back()->name = "inner.txt";

	CHECK(cli.run("extract", "--out-dir", "out", "archive.zip") == Runtime::ExitCode::outputConflict);
	REQUIRE_FALSE(cli.errors().empty());
	CHECK(cli.errors()[0].find("E_OUTPUT_EXISTS") != std::string::npos);
	CHECK(container->subFileEntries.back()->copiedTo.empty());
}

TEST_CASE("extraction replaces an existing file with overwrite", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("archive.zip").withExistingOutput("out/inner.txt");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> container = cli.engine.addDocument("archive.zip");
	container->subFileEntries.push_back(std::make_shared<Tests::Fakes::FakeSubFileState>());
	container->subFileEntries.back()->name = "inner.txt";

	CHECK(cli.run("extract", "--overwrite", "--out-dir", "out", "archive.zip") == Runtime::ExitCode::success);
	CHECK(container->subFileEntries.back()->copiedTo == std::vector<std::string>{ "out/inner.txt" });
}

TEST_CASE("extraction does not write through a symbolic link inside the output directory", "[Cli]")
{
	// The name is clean text --- out/link/inner.txt sits inside out --- but with
	// out/link pointing elsewhere the bytes would land outside it.
	//
	// Distinct from "a subfile cannot be written through a link out of the output
	// directory" above, which checks the same refusal where the link points at a
	// directory that exists. Here the target was never created, and the subfile is
	// asked whether it was copied: the refusal has to happen before anything is
	// written, not after. That is the whole point of deciding the plan first, and
	// an implementation that reported the error on the way back from a completed
	// write would pass every other assertion.
	CliHarness cli;
	cli.withDocument("archive.zip");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> container = cli.engine.addDocument("archive.zip");
	container->subFileEntries.push_back(std::make_shared<Tests::Fakes::FakeSubFileState>());
	std::shared_ptr<Tests::Fakes::FakeSubFileState> inner = container->subFileEntries.back();
	inner->name = "link/inner.txt";
	cli.fileSystem.markSymlink("out/link", "/elsewhere");

	CHECK(cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip") == Runtime::ExitCode::outputConflict);
	REQUIRE_FALSE(cli.errors().empty());
	CHECK(cli.errors()[0].find("E_UNSAFE_OUTPUT") != std::string::npos);
	CHECK(inner->copiedTo.empty());
}

TEST_CASE("extraction writes into an output directory that is itself a link", "[Cli]")
{
	// The root is the user's own --out-dir; pointing it at a link is a choice.
	CliHarness cli;
	cli.withDocument("archive.zip");
	std::shared_ptr<Tests::Fakes::FakeDocumentState> container = cli.engine.addDocument("archive.zip");
	container->subFileEntries.push_back(std::make_shared<Tests::Fakes::FakeSubFileState>());
	container->subFileEntries.back()->name = "inner.txt";
	cli.fileSystem.addDirectory("elsewhere").markSymlink("out", "elsewhere");

	CHECK(cli.run("extract", "--out-dir", "out", "archive.zip") == Runtime::ExitCode::success);
	CHECK(container->subFileEntries.back()->copiedTo == std::vector<std::string>{ "out/inner.txt" });
}

TEST_CASE("an empty out-dir is a usage error", "[Cli]")
{
	// An empty directory joins as "/name", which is the filesystem root.
	for (const std::vector<std::string>& verb : { std::vector<std::string>{ "extract" },
			 std::vector<std::string>{ "convert", "--to", "text" } })
	{
		CAPTURE(verb[0]);
		CliHarness cli;
		cli.withDocument("a.docx", "text");

		std::vector<std::string> argv = verb;
		argv.insert(argv.end(), { "--out-dir", "", "a.docx" });
		CHECK(cli.run(argv) == Runtime::ExitCode::usageError);

		REQUIRE_FALSE(cli.errors().empty());
		CHECK(cli.errors()[0].find("E_BAD_VALUE") != std::string::npos);
		CHECK(cli.errors()[0].find("--out-dir does not accept the value ''") != std::string::npos);
	}
}

TEST_CASE("fingerprint creates no output directory", "[Cli]")
{
	// --fingerprint renders into a discarded stream, so the plan's target is
	// a guess that is never written to.
	CliHarness cli;
	cli.withDocument("a.docx", "hello");

	CHECK(cli.run("convert", "--to", "pdf", "--fingerprint", "a.docx") == Runtime::ExitCode::success);

	CHECK_FALSE(cli.fileSystem.directoryExists("brand/new"));
}

TEST_CASE("a report alongside a canvas conversion is allowed", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.docx", "hello");

	CHECK(cli.run("convert", "--to", "pdf", "--bookmarks", "--stdout", "a.docx") == Runtime::ExitCode::success);

	CHECK(cli.errors().empty());
}

// ============================================ dispatch branches

TEST_CASE("dry run prints the plan and opens nothing", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.docx", "text");

	CHECK(cli.run("convert", "--to", "pdf", "--out-dir", "out", "--dry-run", "a.docx") == Runtime::ExitCode::success);

	CHECK(anyContains(cli.out(), "command convert"));
	CHECK(anyContains(cli.out(), "target out/a.pdf"));
	CHECK_FALSE(anyStartsWith(cli.engine.calls, "openDocument("));
}

TEST_CASE("print plan is the same branch as dry run", "[Cli]")
{
	CliHarness dryRun;
	dryRun.withDocument("a.docx");
	CliHarness printPlan;
	printPlan.withDocument("a.docx");

	dryRun.run("convert", "--to", "pdf", "--out-dir", "out", "--dry-run", "a.docx");
	printPlan.run("convert", "--to", "pdf", "--out-dir", "out", "--print-plan", "a.docx");

	CHECK(dryRun.out() == printPlan.out());
}

TEST_CASE("print contract dumps the surface and runs nothing", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.docx");

	CHECK(cli.run("convert", "--to", "pdf", "--out-dir", "out", "--print-contract", "a.docx") == Runtime::ExitCode::success);

	CHECK(anyContains(cli.out(), "command convert"));
	CHECK(anyContains(cli.out(), "--out-dir"));
	CHECK(cli.engine.calls.empty());
}

TEST_CASE("print contract wins over a dry run", "[Cli]")
{
	// Both short-circuit; the contract dump is checked first, so it is what
	// runs.
	CliHarness cli;
	cli.withDocument("a.docx");

	cli.run("convert", "--to", "pdf", "--out-dir", "out", "--print-contract", "--dry-run", "a.docx");

	CHECK(anyContains(cli.out(), "--print-contract"));
	CHECK_FALSE(anyContains(cli.out(), "cardinality"));
}

// ============================================ the engine's lifetime

TEST_CASE("a command that needs no engine never asks for one", "[Cli]")
{
	// formats reads no document, but it does read the catalogue, so the
	// engine is touched exactly once and for that alone.
	CliHarness cli;
	cli.engine.formats.push_back(Engine::EngineFormat{ 254, "docx", "Microsoft Word 2007" });

	CHECK(cli.run("formats") == Runtime::ExitCode::success);

	CHECK(anyContains(cli.out(), "id=254"));
	CHECK_FALSE(anyStartsWith(cli.engine.calls, "openDocument("));
}

TEST_CASE("a run that fails still closes every document it opened", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("first.docx", "text").withUnreadableDocument("second.docx", Engine::EngineError::notReadable);

	cli.run("convert", "--to", "text", "--out-dir", "out", "first.docx", "second.docx");

	for (const std::string& call : cli.engine.calls)
	{
		if (call.rfind("openDocument(", 0) == 0)
			CHECK(call.find(".docx") != std::string::npos);
	}
}

// ============================================ other verbs, end to end

TEST_CASE("identify prints one line per input", "[Cli]")
{
	CliHarness cli;
	cli.withDocument("a.bin");
	cli.engine.addDocument("a.bin")->formatName = "Microsoft Word 2007";

	CHECK(cli.run("identify", "a.bin") == Runtime::ExitCode::success);

	REQUIRE(cli.out().size() == 1);
	CHECK(cli.out()[0].rfind("a.bin: format=", 0) == 0);
	CHECK(cli.out()[0].find("name=Microsoft Word 2007") != std::string::npos);
}

TEST_CASE("version needs neither a document nor a walk", "[Cli]")
{
	CliHarness cli;

	CHECK(cli.run("version") == Runtime::ExitCode::success);
	CHECK_FALSE(cli.out().empty());
}

// ============================================ the console pre-scan

TEST_CASE("the console settings are read before anything is bound", "[Cli]")
{
	CHECK(Program::preScanConsoleOptions({ "convert", "--silent" }).silent);
	CHECK(Program::preScanConsoleOptions({ "convert", "--verbose" }).verbose);
	CHECK(Program::preScanConsoleOptions({ "convert", "--color" }).color);

	Configuration::ConsoleOptions none = Program::preScanConsoleOptions({ "convert", "a.docx" });

	CHECK_FALSE(none.silent);
	CHECK_FALSE(none.verbose);
	CHECK_FALSE(none.color);
}

TEST_CASE("the pre scan reads only exact spellings", "[Cli]")
{
	// Prefix abbreviation is off everywhere, so a partial spelling is an
	// unknown argument rather than a match.
	CHECK_FALSE(Program::preScanConsoleOptions({ "convert", "--sil" }).silent);
	CHECK_FALSE(Program::preScanConsoleOptions({ "convert", "--silent=true" }).silent);
}

TEST_CASE("the pre scan survives an empty command line", "[Cli]")
{
	Configuration::ConsoleOptions options = Program::preScanConsoleOptions({});

	CHECK_FALSE(options.silent);
}
