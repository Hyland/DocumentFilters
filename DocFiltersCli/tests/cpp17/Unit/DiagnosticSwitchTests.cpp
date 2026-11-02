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

// The diagnostic switches, each of which used to parse and then do nothing.
// Nine arguments were accepted, validated, and never read again. That is
// worse than not having them: the help text promises something, the
// validator agrees the command line is sensible, and the run behaves exactly
// as if the argument had been left out. These tests are what stops one
// quietly returning to that state.

#include "Commands/CommandExecution.h"
#include "Commands/ConvertCommand.h"
#include "Configuration/CommandOptions.h"
#include "Configuration/SharedOptions.h"
#include "Execution/CanvasConversion.h"
#include "Execution/DebugInput.h"
#include "Execution/Diagnostics.h"
#include "Execution/DocumentFiltersSession.h"
#include "Execution/EngineLog.h"
#include "Execution/ExtractorOpenSettings.h"
#include "Fakes/FakeDocumentFiltersEngine.h"
#include "Fakes/InMemoryFileSystem.h"
#include "Fakes/RecordingConsole.h"
#include "Planning/FormatCatalog.h"
#include "Planning/RunPlan.h"
#include "Runtime/Interruption.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <thread>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocumentFiltersEngine;
using Hyland::DocumentFilters::Cli::Tests::Fakes::InMemoryFileSystem;
using Hyland::DocumentFilters::Cli::Tests::Fakes::RecordingConsole;

namespace
{
	std::streamsize streamLength(std::istream& stream)
	{
		if (auto* measured = dynamic_cast<Execution::MeasuredStream*>(&stream))
			return measured->length();

		std::streamsize current = stream.tellg();
		stream.seekg(0, std::ios::end);
		std::streamsize length = stream.tellg();
		stream.seekg(current, std::ios::beg);
		return length;
	}
}

TEST_CASE("temp dir is given to the engine", "[DiagnosticSwitches]")
{
	// A library-wide setting, applied once after Initialize rather than per
	// document: IGR_Multiplex(IGR_Multi_Set_Temp_Path), as doc2text does it.
	FakeDocumentFiltersEngine engine;
	Execution::DocumentFiltersSession::forEngine(engine);
	engine.setTempPath("/scratch");

	CHECK(engine.tempPath == std::optional<std::string>("/scratch"));
	CHECK(std::find(engine.calls.begin(), engine.calls.end(), "setTempPath(/scratch)") != engine.calls.end());
}

TEST_CASE("engine memory is reported at the level asked for", "[DiagnosticSwitches]")
{
	struct Case
	{
		int level;
		bool perDocument;
		bool perRun;
	};

	for (Case c : { Case{ 0, false, false }, Case{ 1, false, true }, Case{ 2, true, true } })
	{
		FakeDocumentFiltersEngine engine;
		engine.memoryStatus = "heap: 42 bytes";
		RecordingConsole console;
		Configuration::DiagnosticOptions options;
		options.memoryStatusLevel = c.level;
		Execution::Diagnostics diagnostics(options, console, &engine);

		diagnostics.beginRun();
		diagnostics.beginDocument("a.docx", 1, 1);
		diagnostics.endDocument("a.docx");
		std::size_t afterDocument = console.outLines.size();
		diagnostics.endRun(1, 0);

		CHECK(afterDocument == (c.perDocument ? 1u : 0u));
		CHECK(console.outLines.size() == (c.perDocument ? 1u : 0u) + (c.perRun ? 1u : 0u));

		if (c.perRun)
		{
			bool found = false;
			for (const std::string& line : console.outLines)
			{
				if (line.find("heap: 42 bytes") != std::string::npos)
					found = true;
			}
			CHECK(found);
		}
	}
}

TEST_CASE("logmem reports the processes own peak", "[DiagnosticSwitches]")
{
	// The process, not the engine: -x and --xx answer the other question.
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.logMemory = true;
	Execution::Diagnostics diagnostics(options, console);
	diagnostics.endRun(1, 0);

	bool found = false;
	for (const std::string& line : console.outLines)
	{
		if (line.rfind("peak memory: ", 0) == 0 && !line.empty() && line.substr(line.size() - 2) == "MB")
			found = true;
	}
	CHECK(found);
}

namespace
{
	/// <summary>A clock a test moves by hand, so a deadline can pass instantly.</summary>
	class TestClock
	{
	public:
		std::chrono::steady_clock::time_point now() const { return _now; }

		void advance(std::chrono::steady_clock::duration by) { _now += by; }

	private:
		std::chrono::steady_clock::time_point _now {};
	};
}

// ------------------------------------------------------------ the stop predicate

TEST_CASE("nothing stops a run inside its limits that nobody interrupted", "[DiagnosticSwitches]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.timeout = 600;
	Execution::Diagnostics diagnostics(options, console);
	diagnostics.beginDocument("a.docx", 1, 1);

	Runtime::Interruption interruption;
	CHECK_FALSE(Commands::CommandExecution::watchForStop(diagnostics, &interruption).shouldStop());
	CHECK_FALSE(Commands::CommandExecution::watchForStop(diagnostics, nullptr).shouldStop());
}

TEST_CASE("the documents deadline stops it", "[DiagnosticSwitches]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.timeout = 30;
	TestClock clock;
	Execution::Diagnostics diagnostics(options, console, nullptr, [&clock]() { return clock.now(); });
	diagnostics.beginDocument("slow.docx", 1, 1);
	clock.advance(std::chrono::seconds(31));

	Execution::StopSignal stop = Commands::CommandExecution::watchForStop(diagnostics, nullptr);

	CHECK(stop.shouldStop());
	CHECK(stop.reason() == Execution::StopReason::timedOut);
}

TEST_CASE("an interrupt stops it even with time to spare", "[DiagnosticSwitches]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.timeout = 600;
	Execution::Diagnostics diagnostics(options, console);
	diagnostics.beginDocument("a.docx", 1, 1);

	Runtime::Interruption interruption;
	Execution::StopSignal stop = Commands::CommandExecution::watchForStop(diagnostics, &interruption);

	CHECK_FALSE(stop.shouldStop());
	CHECK_FALSE(stop.reason().has_value());

	interruption.request();

	CHECK(stop.shouldStop());
	CHECK(stop.reason() == Execution::StopReason::interrupted);
}

TEST_CASE("an interrupt outranks a deadline that also passed", "[DiagnosticSwitches]")
{
	// The reason is settled by the poll that stopped the document rather than
	// worked out afterwards, which is what makes this answerable at all: by the
	// time a cancelled open is caught both conditions can be true, and asking then
	// is a second guess at a question already answered.
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.timeout = 30;
	TestClock clock;
	Execution::Diagnostics diagnostics(options, console, nullptr, [&clock]() { return clock.now(); });
	diagnostics.beginDocument("slow.docx", 1, 1);
	clock.advance(std::chrono::seconds(31));

	Runtime::Interruption interruption;
	interruption.request();

	Execution::StopSignal stop = Commands::CommandExecution::watchForStop(diagnostics, &interruption);

	CHECK(stop.shouldStop());
	CHECK(stop.reason() == Execution::StopReason::interrupted);

	try
	{
		stop.raise("slow.docx");
		FAIL("expected an InterruptedError");
	}
	catch (const Runtime::CliError& error)
	{
		CHECK(error.code() == Runtime::ErrorCodes::interrupted);
	}
}

TEST_CASE("a cancellation with no recorded reason is a document error", "[DiagnosticSwitches]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	Execution::Diagnostics diagnostics(options, console);
	diagnostics.beginDocument("a.docx", 1, 1);

	Execution::StopSignal stop = Commands::CommandExecution::watchForStop(diagnostics, nullptr);
	CHECK_FALSE(stop.shouldStop());

	try
	{
		stop.raise("a.docx");
		FAIL("expected a DocumentError");
	}
	catch (const Runtime::CliError& error)
	{
		CHECK(error.code() == Runtime::ErrorCodes::documentFailed);
	}
}

// ------------------------------------------------------------------- --no-images

TEST_CASE("no images suppresses them for the markdown formats", "[DiagnosticSwitches]")
{
	for (const char* token : { "md", "md-text" })
		CHECK(Execution::CanvasConversion::imageSuppression(Planning::FormatCatalog::resolve(token)) == "MARKDOWN_INCLUDE_IMAGES=OFF");
}

TEST_CASE("no images is ignored where it has no meaning", "[DiagnosticSwitches]")
{
	for (const char* token : { "html5", "svg", "text", "pdf" })
		CHECK(Execution::CanvasConversion::imageSuppression(Planning::FormatCatalog::resolve(token)) == "");
}

TEST_CASE("no images reaches the canvas options for hi def markdown", "[DiagnosticSwitches]")
{
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("md"), std::nullopt, true) == "MARKDOWN_INCLUDE_IMAGES=OFF");
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("md"), std::nullopt, false) == "");
}

TEST_CASE("no images reaches the open options for text mode markdown", "[DiagnosticSwitches]")
{
	// md-text never opens a canvas, so the same rule has to arrive at Open.
	Configuration::ConvertOptions options;
	options.noImages = true;
	Execution::ExtractorOpenSettings settings =
		Execution::ExtractorOpenSettings::forFormat(Planning::FormatCatalog::resolve("md-text"), options, Configuration::SharedOptions());

	CHECK(settings.options.find("MARKDOWN_INCLUDE_IMAGES=OFF") != std::string::npos);
}

// ------------------------------------------------------- --log-level/--log-modules

TEST_CASE("no log level means no log at all", "[DiagnosticSwitches]")
{
	// Absent rather than a disabled instance, so the engine is handed no
	// callback and never composes a message nobody prints.
	RecordingConsole console;
	CHECK_FALSE(Execution::EngineLog::for_(Configuration::DiagnosticOptions(), console).has_value());
}

TEST_CASE("every module logs when none are named", "[DiagnosticSwitches]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.logLevel = "debug";
	std::optional<Execution::EngineLog> log = Execution::EngineLog::for_(options, console);
	REQUIRE(log.has_value());

	CHECK(log->levelFor("pdf") == Execution::EngineLog::levelDebug);
	CHECK(log->levelFor("anything") == Execution::EngineLog::levelDebug);
}

TEST_CASE("naming modules silences the rest", "[DiagnosticSwitches]")
{
	// The filter is applied by answering "none" for a module nobody asked
	// about, which is where --log-modules actually takes effect --- the
	// engine has no notion of the user's list.
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.logLevel = "trace";
	options.logModules = "pdf,ole";
	std::optional<Execution::EngineLog> log = Execution::EngineLog::for_(options, console);
	REQUIRE(log.has_value());

	CHECK(log->levelFor("pdf") == Execution::EngineLog::levelTrace);
	CHECK(log->levelFor("OLE") == Execution::EngineLog::levelTrace);
	CHECK(log->levelFor("html") == Execution::EngineLog::levelNone);
}

TEST_CASE("the debug spellings are the same switch", "[DiagnosticSwitches]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.debugLogLevel = "warning";
	options.debugLogModules = "pdf";
	std::optional<Execution::EngineLog> log = Execution::EngineLog::for_(options, console);
	REQUIRE(log.has_value());

	CHECK(log->levelFor("pdf") == Execution::EngineLog::levelWarning);
	CHECK(log->levelFor("xls") == Execution::EngineLog::levelNone);
}

TEST_CASE("a log message goes to stderr with its level and module", "[DiagnosticSwitches]")
{
	// stderr, as doc2text does. It shares the stream with the two contract
	// error shapes, which is safe because nothing logs unless --log-level was
	// passed.
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.logLevel = "debug";
	std::optional<Execution::EngineLog> log = Execution::EngineLog::for_(options, console);
	REQUIRE(log.has_value());

	log->write(Execution::EngineLog::levelDebug, "pdf", "parsing xref");

	REQUIRE(console.errorLines.size() == 1);
	CHECK(console.errorLines[0].find("DEBUG [pdf] parsing xref") != std::string::npos);
	CHECK(console.outLines.empty());
}

TEST_CASE("an unknown level logs nothing rather than failing", "[DiagnosticSwitches]")
{
	// doc2text's own parse falls through to none; a typo should not end a run
	// that was otherwise fine.
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.logLevel = "chatty";
	CHECK_FALSE(Execution::EngineLog::for_(options, console).has_value());
}

// -------------------------------------------------------------------- --view

TEST_CASE("view opens the one file a run produced", "[DiagnosticSwitches]")
{
	Planning::RunPlan plan;
	plan.cardinality = Planning::OutputCardinality::PerInput;
	Planning::PlannedItem item;
	item.sourcePath = "a.docx";
	item.target = "out/a.pdf";
	plan.items.push_back(item);

	CHECK(Commands::ConvertCommand::singleOutputOf(plan) == std::optional<std::string>("out/a.pdf"));
}

TEST_CASE("view opens the combined target when there is one", "[DiagnosticSwitches]")
{
	Planning::RunPlan plan;
	plan.cardinality = Planning::OutputCardinality::Combined;
	plan.combinedTarget = "out/book.pdf";

	Planning::PlannedItem a;
	a.sourcePath = "a.docx";
	plan.items.push_back(a);

	Planning::PlannedItem b;
	b.sourcePath = "b.docx";
	plan.items.push_back(b);

	CHECK(Commands::ConvertCommand::singleOutputOf(plan) == std::optional<std::string>("out/book.pdf"));
}

TEST_CASE("view opens nothing when a run produced several files", "[DiagnosticSwitches]")
{
	// "Single output" is the spec's own wording: several inputs, or one input
	// to a per-page format, leave nothing unambiguous to open.
	Planning::RunPlan many;
	many.cardinality = Planning::OutputCardinality::PerInput;

	Planning::PlannedItem a;
	a.sourcePath = "a.docx";
	a.target = "out/a.pdf";
	many.items.push_back(a);

	Planning::PlannedItem b;
	b.sourcePath = "b.docx";
	b.target = "out/b.pdf";
	many.items.push_back(b);

	CHECK_FALSE(Commands::ConvertCommand::singleOutputOf(many).has_value());

	Planning::RunPlan perPage;
	perPage.cardinality = Planning::OutputCardinality::PerInput;

	Planning::PlannedItem c;
	c.sourcePath = "a.docx";
	Planning::TargetPattern pattern;
	pattern.directory = "out";
	pattern.extension = "png";
	c.targetPattern = pattern;
	perPage.items.push_back(c);

	CHECK_FALSE(Commands::ConvertCommand::singleOutputOf(perPage).has_value());
}

// ------------------------------------------------------ the debug streams

TEST_CASE("nothing wraps the input when no debug switch is set", "[DiagnosticSwitches]")
{
	InMemoryFileSystem fileSystem;
	fileSystem.addFile("a.docx", "hello");

	RecordingConsole console;
	CHECK(Execution::DebugInput::for_("a.docx", Configuration::DiagnosticOptions(), fileSystem, console, "a.docx") == nullptr);
}

TEST_CASE("use memory stream reads the whole document into memory", "[DiagnosticSwitches]")
{
	InMemoryFileSystem fileSystem;
	fileSystem.addFile("a.docx", "hello world");

	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.useMemoryStream = true;
	std::unique_ptr<std::istream> stream = Execution::DebugInput::for_("a.docx", options, fileSystem, console, "a.docx");

	REQUIRE(stream != nullptr);
	CHECK(streamLength(*stream) == 11);
}

TEST_CASE("truncate reads only the first bytes", "[DiagnosticSwitches]")
{
	InMemoryFileSystem fileSystem;
	fileSystem.addFile("a.docx", "hello world");

	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.truncateInputBytes = 5;
	std::unique_ptr<std::istream> stream = Execution::DebugInput::for_("a.docx", options, fileSystem, console, "a.docx");

	REQUIRE(stream != nullptr);
	CHECK(streamLength(*stream) == 5);
}

TEST_CASE("truncating past the end stops at the end", "[DiagnosticSwitches]")
{
	InMemoryFileSystem fileSystem;
	fileSystem.addFile("a.docx", "short");

	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.truncateInputBytes = 4096;
	std::unique_ptr<std::istream> stream = Execution::DebugInput::for_("a.docx", options, fileSystem, console, "a.docx");

	REQUIRE(stream != nullptr);
	CHECK(streamLength(*stream) == 5);
}

TEST_CASE("measuring composes with a truncated source", "[DiagnosticSwitches]")
{
	// The two switches answer independent questions --- where the bytes come
	// from, and whether the asking is counted --- so asking both must not
	// drop one.
	InMemoryFileSystem fileSystem;
	fileSystem.addFile("a.docx", "hello world");
	RecordingConsole console;

	Configuration::DiagnosticOptions options;
	options.truncateInputBytes = 5;
	options.measureIo = true;
	std::unique_ptr<std::istream> stream = Execution::DebugInput::for_("a.docx", options, fileSystem, console, "a.docx");
	REQUIRE(stream != nullptr);

	// Truncated to the prefix...
	CHECK(streamLength(*stream) == 5);

	// ...and still counting.
	char buffer[5];
	stream->read(buffer, sizeof(buffer));
	stream.reset();

	REQUIRE(console.outLines.size() == 1);
	CHECK(console.outLines[0] == "a.docx: reads=1 seeks=0 bytes=5");
}

TEST_CASE("measuring composes with a buffered source", "[DiagnosticSwitches]")
{
	InMemoryFileSystem fileSystem;
	fileSystem.addFile("a.docx", "hello world");
	RecordingConsole console;

	Configuration::DiagnosticOptions options;
	options.useMemoryStream = true;
	options.measureIo = true;
	std::unique_ptr<std::istream> stream = Execution::DebugInput::for_("a.docx", options, fileSystem, console, "a.docx");
	REQUIRE(stream != nullptr);

	CHECK(streamLength(*stream) == 11);

	char buffer[11];
	stream->read(buffer, sizeof(buffer));
	stream.reset();

	REQUIRE(console.outLines.size() == 1);
	CHECK(console.outLines[0] == "a.docx: reads=1 seeks=0 bytes=11");
}

TEST_CASE("measure io counts what the engine asked for and reports on close", "[DiagnosticSwitches]")
{
	// Reported on close so the figures cover the whole document rather than
	// whatever had happened when someone thought to look.
	InMemoryFileSystem fileSystem;
	fileSystem.addFile("a.docx", "hello world");
	RecordingConsole console;

	Configuration::DiagnosticOptions options;
	options.measureIo = true;
	std::unique_ptr<std::istream> stream = Execution::DebugInput::for_("a.docx", options, fileSystem, console, "a.docx");
	REQUIRE(stream != nullptr);

	char buffer[5];
	stream->read(buffer, sizeof(buffer));
	stream->seekg(0, std::ios::beg);
	CHECK(console.outLines.empty());

	stream.reset();

	REQUIRE(console.outLines.size() == 1);
	CHECK(console.outLines[0] == "a.docx: reads=1 seeks=1 bytes=5");
}
