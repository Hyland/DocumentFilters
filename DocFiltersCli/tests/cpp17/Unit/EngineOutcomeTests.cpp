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

// What the CLI does with each way Document Filters can fail. The engine is
// the one dependency the execution paths could not be tested around: it
// needs a license, a native library, and a document that actually exhibits
// the outcome under test. Behind IDocumentFiltersEngine every outcome is
// scriptable, so the question these tests ask is the one that matters ---
// not "does the engine do this", which is Hyland's to answer, but "when it
// does, does the CLI behave".

#include "Commands/ConvertCommand.h"
#include "Commands/IdentifyCommand.h"
#include "Configuration/CommandOptions.h"
#include "Configuration/SharedOptions.h"
#include "Execution/DocumentFiltersSession.h"
#include "Fakes/FakeDocumentFiltersEngine.h"
#include "Fakes/InMemoryFileSystem.h"
#include "Fakes/RecordingConsole.h"
#include "Planning/FormatCatalog.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocument;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocumentFiltersEngine;
using Hyland::DocumentFilters::Cli::Tests::Fakes::InMemoryFileSystem;
using Hyland::DocumentFilters::Cli::Tests::Fakes::RecordingConsole;

namespace
{
	/// <summary>Every failure the engine can report of its own accord.</summary>
	/// <remarks>
	/// <c>EngineError::none</c> is not a failure, and <c>EngineError::cancelled</c>
	/// is not the engine's: it is only ever raised because this tool asked,
	/// through the open callback, so the caller that asked translates it into
	/// its own error --- a <c>--timeout</c> reported as <c>E_LIMIT_EXCEEDED</c>
	/// rather than a document reported as broken. Sweeping it here would
	/// assert the opposite.
	/// </remarks>
	std::vector<Engine::EngineError> everyEngineError()
	{
		return {
			Engine::EngineError::openError,
			Engine::EngineError::wrongType,
			Engine::EngineError::inUse,
			Engine::EngineError::notReadable,
			Engine::EngineError::password,
			Engine::EngineError::notFound,
			Engine::EngineError::writeError,
			Engine::EngineError::notValidForThisClass,
			Engine::EngineError::error,
			Engine::EngineError::invalidHandle,
			Engine::EngineError::invalidPointer,
			Engine::EngineError::invalidParameter,
			Engine::EngineError::fileCorrupt,
			Engine::EngineError::outOfMemory,
			Engine::EngineError::badError,
			Engine::EngineError::wrongThread,
			Engine::EngineError::tooManyHandles,
			Engine::EngineError::fontsNotFound,
			Engine::EngineError::versionMismatch,
			Engine::EngineError::actionNotSupported,
		};
	}

	/// <remarks>
	/// The inputs have to exist in two places: on the fake filesystem,
	/// because the walk selects them before the engine is ever asked, and in
	/// the fake engine, because that is what opening them consults. A path
	/// present in only one of the two is the setup mistake this helper exists
	/// to prevent.
	/// </remarks>
	Commands::CommandContext context(FakeDocumentFiltersEngine& engine, RecordingConsole& console, InMemoryFileSystem& fileSystem,
		Configuration::SharedOptions shared, std::optional<Execution::DocumentFiltersSession>& sessionSlot)
	{
		for (const std::string& path : shared.input.paths)
			fileSystem.addFile(path);

		sessionSlot.emplace(Execution::DocumentFiltersSession::forEngine(engine));

		Commands::CommandContext ctx;
		ctx.sharedOptions = std::move(shared);
		ctx.console = &console;
		ctx.fileSystem = &fileSystem;
		ctx.session = &*sessionSlot;
		return ctx;
	}

	Configuration::SharedOptions sharedWithOutDir(std::optional<std::string> outDir = std::string("out"))
	{
		Configuration::SharedOptions shared;
		shared.output.outDir = outDir;
		return shared;
	}

	int runConvert(FakeDocumentFiltersEngine& engine, RecordingConsole& console, InMemoryFileSystem& fileSystem,
		Configuration::SharedOptions shared, const std::vector<std::string>& paths)
	{
		for (const std::string& path : paths)
			shared.input.paths.push_back(path);

		Configuration::ConvertOptions options;
		options.format = &Planning::FormatCatalog::resolve("text");
		Commands::ConvertCommand command(options);

		// Named, not passed as a temporary: execute() takes CommandContext&, and
		// binding a temporary to a non-const lvalue reference is a Microsoft
		// extension that GCC and Clang reject outright.
		std::optional<Execution::DocumentFiltersSession> sessionSlot;
		Commands::CommandContext ctx = context(engine, console, fileSystem, std::move(shared), sessionSlot);
		return command.execute(ctx);
	}

	int runConvert(FakeDocumentFiltersEngine& engine, RecordingConsole& console, InMemoryFileSystem& fileSystem, const std::vector<std::string>& paths)
	{
		return runConvert(engine, console, fileSystem, sharedWithOutDir(), paths);
	}

	bool anyContains(const std::vector<std::string>& lines, const std::string& needle)
	{
		return std::any_of(lines.begin(), lines.end(), [&needle](const std::string& line) { return line.find(needle) != std::string::npos; });
	}
}

// ============================================ opening a document

TEST_CASE("an interrupt while a document opens exits 130 and does not report a timeout", "[EngineOutcome]")
{
	// Both causes reach the engine through one predicate and come back as one
	// cancelled error, and every command used to translate that into
	// documentTimedOut unconditionally -- which dereferences the timeout optional.
	// With no --timeout given there is nothing in it, so an interrupt during open
	// was undefined behaviour rather than exit 130.
	//
	// The interrupt is requested from inside the open: BatchRunner checks between
	// documents, so one requested any earlier stops the run before anything opens
	// and never reaches this path.
	FakeDocumentFiltersEngine engine;
	engine.addDocument("a.docx", "body");
	RecordingConsole console;
	InMemoryFileSystem fileSystem;
	Runtime::Interruption interruption;
	engine.duringOpen = [&interruption]() { interruption.request(); };

	Configuration::SharedOptions shared = sharedWithOutDir();
	shared.input.paths.push_back("a.docx");

	Configuration::ConvertOptions options;
	options.format = &Planning::FormatCatalog::resolve("text");
	Commands::ConvertCommand command(options);

	std::optional<Execution::DocumentFiltersSession> sessionSlot;
	Commands::CommandContext ctx = context(engine, console, fileSystem, shared, sessionSlot);
	ctx.interruption = &interruption;

	try
	{
		command.execute(ctx);
		FAIL("expected an InterruptedError");
	}
	catch (const Runtime::CliError& error)
	{
		CHECK(error.code() == Runtime::ErrorCodes::interrupted);
		CHECK(error.exitCode() == Runtime::ExitCode::interrupted);
		CHECK(std::string(error.what()).find("--timeout") == std::string::npos);
	}
}


TEST_CASE("any open failure becomes one failed input", "[EngineOutcome]")
{
	// This is the property the whole batch model rests on: an engine failure
	// has to arrive as a DocumentError naming the input, not as an exception
	// that ends the process.
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocumentFiltersEngine engine;
		engine.addDocument("good.docx", "hello");
		engine.failToOpen("bad.docx", error);

		RecordingConsole console;
		InMemoryFileSystem fileSystem;
		int exitCode = runConvert(engine, console, fileSystem, { "good.docx", "bad.docx" });

		CHECK(exitCode == Runtime::ExitCode::partialFailure);
		CHECK(anyContains(console.errorLines, "bad.docx"));
		CHECK_FALSE(anyContains(console.errorLines, "good.docx"));
	}
}

TEST_CASE("any open failure stops the run under fail fast", "[EngineOutcome]")
{
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocumentFiltersEngine engine;
		engine.failToOpen("first.docx", error);
		engine.addDocument("second.docx", "hello");

		RecordingConsole console;
		InMemoryFileSystem fileSystem;
		Configuration::SharedOptions shared = sharedWithOutDir();
		shared.batch.onError = Configuration::OnErrorBehaviour::Stop;

		int exitCode = runConvert(engine, console, fileSystem, shared, { "first.docx", "second.docx" });

		CHECK(exitCode == Runtime::ExitCode::partialFailure);

		// The second document is never opened, which is the whole point of
		// stopping.
		CHECK_FALSE(anyContains(engine.calls, "second.docx"));
	}
}

TEST_CASE("a document that opens is converted and closed", "[EngineOutcome]")
{
	FakeDocumentFiltersEngine engine;
	std::shared_ptr<Tests::Fakes::FakeDocumentState> document = engine.addDocument("a.docx", "the text");

	RecordingConsole console;
	InMemoryFileSystem fileSystem;
	int exitCode = runConvert(engine, console, fileSystem, { "a.docx" });

	CHECK(exitCode == Runtime::ExitCode::success);
	CHECK(document->wasOpened);
	CHECK(document->isDisposed);
}

// ============================================ passwords

TEST_CASE("the right password opens a protected document", "[EngineOutcome]")
{
	FakeDocumentFiltersEngine engine;
	engine.addProtectedDocument("locked.docx", "hunter2", "secret text");

	Configuration::SharedOptions shared = sharedWithOutDir();
	shared.passwords.passwords.push_back("hunter2");

	RecordingConsole console;
	InMemoryFileSystem fileSystem;
	CHECK(runConvert(engine, console, fileSystem, shared, { "locked.docx" }) == Runtime::ExitCode::success);
}

TEST_CASE("a wrong password fails that document only", "[EngineOutcome]")
{
	FakeDocumentFiltersEngine engine;
	engine.addProtectedDocument("locked.docx", "hunter2", "secret");
	engine.addDocument("open.docx", "plain");

	Configuration::SharedOptions shared = sharedWithOutDir();
	shared.passwords.passwords.push_back("wrong");

	RecordingConsole console;
	InMemoryFileSystem fileSystem;
	int exitCode = runConvert(engine, console, fileSystem, shared, { "locked.docx", "open.docx" });

	CHECK(exitCode == Runtime::ExitCode::partialFailure);
	CHECK(anyContains(console.errorLines, "locked.docx"));
}

TEST_CASE("no password at all fails the same way a wrong one does", "[EngineOutcome]")
{
	// The engine reports both as IGR_E_PASSWORD, so the CLI cannot tell them
	// apart and must not pretend to.
	FakeDocumentFiltersEngine engine;
	engine.addProtectedDocument("locked.docx", "hunter2", "secret");

	RecordingConsole console;
	InMemoryFileSystem fileSystem;
	int exitCode = runConvert(engine, console, fileSystem, sharedWithOutDir(), { "locked.docx" });

	CHECK(exitCode == Runtime::ExitCode::partialFailure);
	CHECK(anyContains(console.errorLines, "locked.docx"));
}

// ============================================ failures after opening

/// <summary>
/// A failure part-way through reading is still one failed input, not a
/// crash.
/// </summary>
/// <remarks>
/// Worth separating from an open failure because the document is open when
/// it happens: the handle has to be closed on the way out, and the batch
/// has to carry on. A corrupt document that opens and then fails on read is
/// the common shape of it.
/// </remarks>
TEST_CASE("a read failure after opening is still one failed input", "[EngineOutcome]")
{
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocumentFiltersEngine engine;
		std::shared_ptr<Tests::Fakes::FakeDocumentState> broken = engine.addDocument("broken.docx", "some text");
		broken->readFailure = error;
		engine.addDocument("fine.docx", "text");

		RecordingConsole console;
		InMemoryFileSystem fileSystem;
		int exitCode = runConvert(engine, console, fileSystem, { "broken.docx", "fine.docx" });

		CHECK(exitCode == Runtime::ExitCode::partialFailure);
		CHECK(anyContains(console.errorLines, "broken.docx"));
		CHECK(broken->isDisposed);
	}
}

// ============================================ identify

TEST_CASE("identify reports a failure per input and keeps going", "[EngineOutcome]")
{
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocumentFiltersEngine engine;
		engine.failToOpen("bad.bin", error);
		std::shared_ptr<Tests::Fakes::FakeDocumentState> good = engine.addDocument("good.bin");
		good->fileType = 254;
		good->mimeType = "application/vnd.ms-word";
		good->formatName = "Microsoft Word";

		RecordingConsole console;
		InMemoryFileSystem fileSystem;
		Commands::IdentifyCommand command{ Configuration::IdentifyOptions() };
		Configuration::SharedOptions shared = sharedWithOutDir(std::nullopt);
		shared.input.paths.push_back("bad.bin");
		shared.input.paths.push_back("good.bin");

		std::optional<Execution::DocumentFiltersSession> sessionSlot;
		Commands::CommandContext ctx = context(engine, console, fileSystem, shared, sessionSlot);
		int exitCode = command.execute(ctx);

		CHECK(exitCode == Runtime::ExitCode::partialFailure);
		CHECK(std::any_of(console.outLines.begin(), console.outLines.end(),
			[](const std::string& line) { return line.find("good.bin") != std::string::npos && line.find("format=254") != std::string::npos; }));
	}
}

TEST_CASE("identify never opens the document", "[EngineOutcome]")
{
	// identify asks what a file is, not what it says. Opening it would need
	// a password for a protected document and would read bytes it has no use
	// for.
	FakeDocumentFiltersEngine engine;
	engine.addProtectedDocument("locked.docx", "hunter2");

	RecordingConsole console;
	InMemoryFileSystem fileSystem;
	Configuration::SharedOptions shared = sharedWithOutDir(std::nullopt);
	shared.input.paths.push_back("locked.docx");

	std::optional<Execution::DocumentFiltersSession> sessionSlot;
	Commands::CommandContext ctx = context(engine, console, fileSystem, shared, sessionSlot);
	Commands::IdentifyCommand command{ Configuration::IdentifyOptions() };
	int exitCode = command.execute(ctx);

	CHECK(exitCode == Runtime::ExitCode::success);
	CHECK(std::any_of(engine.calls.begin(), engine.calls.end(), [](const std::string& call) { return call.rfind("identifyFile(", 0) == 0; }));
	CHECK_FALSE(std::any_of(engine.calls.begin(), engine.calls.end(), [](const std::string& call) { return call.rfind("openDocument(", 0) == 0; }));
}

// ============================================ initialisation

TEST_CASE("an initialisation failure is not a document failure", "[EngineOutcome]")
{
	// Nothing was processed, so this is the run never starting: exit 5, not
	// the partial-failure 1 that means "some files did not work".
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocumentFiltersEngine engine;
		engine.initialisationFailure = error;

		bool threw = false;
		try
		{
			// Mirrors what DocumentFiltersSession::create does, against the
			// fake.
			try
			{
				engine.initialize(std::string(), ".");
			}
			catch (const Engine::EngineException& inner)
			{
				throw Runtime::InitialisationError("could not initialise Document Filters: " + std::string(inner.what()));
			}
		}
		catch (const Runtime::InitialisationError& thrown)
		{
			threw = true;
			CHECK(thrown.exitCode() == Runtime::ExitCode::initialisationFailure);
			CHECK(thrown.code() == Runtime::ErrorCodes::initFailed);
		}
		CHECK(threw);
	}
}
