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

package com.hyland.documentfilters.cli.tests.unit;

import static com.hyland.documentfilters.cli.tests.Assert.assertDoesNotContain;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.commands.CommandContext;
import com.hyland.documentfilters.cli.commands.ConvertCommand;
import com.hyland.documentfilters.cli.commands.IdentifyCommand;
import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.IdentifyOptions;
import com.hyland.documentfilters.cli.configuration.OnErrorBehaviour;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.engine.EngineException;
import com.hyland.documentfilters.cli.execution.DocumentFiltersSession;
import com.hyland.documentfilters.cli.planning.FormatCatalog;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.InitialisationError;
import com.hyland.documentfilters.cli.runtime.InterruptedError;
import com.hyland.documentfilters.cli.runtime.Interruption;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocument;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocumentFiltersEngine;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.util.ArrayList;
import java.util.List;

/**
 * What the CLI does with each way Document Filters can fail.
 *
 * <p>The engine is the one dependency the execution paths could not be tested
 * around: it needs a license, a native library, and a document that actually
 * exhibits the outcome under test. Behind {@code IDocumentFiltersEngine} every
 * outcome is scriptable, so the question these tests ask is the one that matters
 * --- not "does the engine do this", which is Hyland's to answer, but "when it does,
 * does the CLI behave".
 *
 * <p>The codes are not invented: {@link EngineError} has one member per
 * {@code IGR_E_*} constant, carrying the constant's own value.
 */
public final class EngineOutcomeTests
{
	/**
	 * Every failure the engine can report of its own accord.
	 *
	 * <p>{@link EngineError#NONE} is not a failure, and {@link EngineError#CANCELLED}
	 * is not the engine's: it is only ever raised because this tool asked, through
	 * the open callback, so the caller that asked translates it into its own error
	 * --- a {@code --timeout} reported as {@code E_LIMIT_EXCEEDED} rather than a
	 * document reported as broken. Sweeping it here would assert the opposite.
	 */
	static List<EngineError> everyEngineError()
	{
		List<EngineError> errors = new ArrayList<>();
		for (EngineError error : EngineError.values())
		{
			if (error != EngineError.NONE && error != EngineError.CANCELLED)
				errors.add(error);
		}
		return errors;
	}

	/**
	 * The inputs have to exist in two places: on the fake filesystem, because the
	 * walk selects them before the engine is ever asked, and in the fake engine,
	 * because that is what opening them consults. A path present in only one of the
	 * two is the setup mistake this helper exists to prevent.
	 */
	private static CommandContext context(FakeDocumentFiltersEngine engine, RecordingConsole console, SharedOptions shared)
	{
		InMemoryFileSystem fileSystem = new InMemoryFileSystem();
		for (String path : shared.input.paths)
			fileSystem.addFile(path);

		CommandContext context = new CommandContext();
		context.sharedOptions = shared;
		context.console = console;
		context.fileSystem = fileSystem;
		context.session = DocumentFiltersSession.forEngine(engine);
		return context;
	}

	private static SharedOptions shared()
	{
		return shared("out");
	}

	private static SharedOptions shared(String outDir)
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = outDir;
		return shared;
	}

	private static boolean anyContains(List<String> lines, String fragment)
	{
		for (String line : lines)
		{
			if (line.contains(fragment))
				return true;
		}
		return false;
	}

	private static boolean anyStartsWith(List<String> lines, String prefix)
	{
		for (String line : lines)
		{
			if (line.startsWith(prefix))
				return true;
		}
		return false;
	}

	// ============================================ opening a document

	/**
	 * Ctrl+C arriving while a document is opening exits 130, not a timeout error.
	 *
	 * <p>Both causes reach the engine through one predicate and come back as one
	 * {@code EngineError.CANCELLED}, and every command used to translate that into
	 * a timeout unconditionally --- which reads the timeout's value. With no
	 * {@code --timeout} given there is no value, so an interrupt during open crashed
	 * rather than reporting anything. The cause has to be recovered at the catch,
	 * because the engine cannot carry it.
	 *
	 * <p>The interrupt is requested from inside the open: the batch runner checks
	 * between documents, so one requested any earlier stops the run before anything
	 * opens and never reaches this path at all.
	 */
	@Test
	public void anInterruptWhileADocumentOpensExits130AndDoesNotReportATimeout()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.addDocument("a.docx", "body");
		RecordingConsole console = new RecordingConsole();
		Interruption interruption = new Interruption();
		engine.duringOpen = interruption::request;

		SharedOptions shared = shared();
		shared.input.paths.add("a.docx");

		CommandContext ctx = context(engine, console, shared);
		ctx.interruption = interruption;

		ConvertOptions options = new ConvertOptions();
		options.format = FormatCatalog.resolve("text");
		ConvertCommand command = new ConvertCommand(options);

		// Thrown, not returned: InterruptedError travels past the batch runner ---
		// which only absorbs DocumentError --- and the entry point turns it into the
		// exit code. What matters here is which error it is, and that it is a
		// CliError at all rather than a crash.
		CliError error = assertThrows(InterruptedError.class, () -> command.execute(ctx));

		assertEquals(ExitCode.INTERRUPTED, error.getExitCode());
		assertDoesNotContain("--timeout", error.getMessage());
	}

	/**
	 * Whatever the engine says went wrong, one bad input is one failed input.
	 *
	 * <p>This is the property the whole batch model rests on: an engine failure has
	 * to arrive as a {@code DocumentError} naming the input, not as an exception
	 * that ends the process. An {@link EngineException} escaping a command would
	 * bypass the batch runner entirely --- {@code --on-error skip} would stop
	 * skipping, and the exit code would be whatever the entry point makes of an
	 * unhandled exception rather than 1.
	 */
	@Test
	public void anyOpenFailureBecomesOneFailedInput()
	{
		for (EngineError error : everyEngineError())
		{
			FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
			engine.addDocument("good.docx", "hello");
			engine.failToOpen("bad.docx", error);

			RecordingConsole console = new RecordingConsole();
			int exitCode = runConvert(engine, console, "good.docx", "bad.docx");

			assertEquals(ExitCode.PARTIAL_FAILURE, exitCode, error.name());
			assertTrue(anyContains(console.errorLines, "bad.docx"), error.name());
			assertTrue(!anyContains(console.errorLines, "good.docx"), error.name());
		}
	}

	@Test
	public void anyOpenFailureStopsTheRunUnderFailFast()
	{
		for (EngineError error : everyEngineError())
		{
			FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
			engine.failToOpen("first.docx", error);
			engine.addDocument("second.docx", "hello");

			RecordingConsole console = new RecordingConsole();
			SharedOptions shared = shared();
			shared.batch.onError = OnErrorBehaviour.STOP;

			int exitCode = runConvert(engine, console, shared, "first.docx", "second.docx");

			assertEquals(ExitCode.PARTIAL_FAILURE, exitCode, error.name());

			// The second document is never opened, which is the whole point of stopping.
			assertTrue(!anyContains(engine.calls, "second.docx"), error.name());
		}
	}

	@Test
	public void aDocumentThatOpensIsConvertedAndClosed()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		FakeDocument document = engine.addDocument("a.docx", "the text");

		int exitCode = runConvert(engine, new RecordingConsole(), "a.docx");

		assertEquals(ExitCode.SUCCESS, exitCode);
		assertTrue(document.wasOpened);
		assertTrue(document.isDisposed);
	}

	// ============================================ passwords

	@Test
	public void theRightPasswordOpensAProtectedDocument()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.addProtectedDocument("locked.docx", "hunter2", "secret text");

		SharedOptions shared = shared();
		shared.passwords.passwords.add("hunter2");

		assertEquals(ExitCode.SUCCESS, runConvert(engine, new RecordingConsole(), shared, "locked.docx"));
	}

	@Test
	public void aWrongPasswordFailsThatDocumentOnly()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.addProtectedDocument("locked.docx", "hunter2", "secret");
		engine.addDocument("open.docx", "plain");

		SharedOptions shared = shared();
		shared.passwords.passwords.add("wrong");

		RecordingConsole console = new RecordingConsole();
		int exitCode = runConvert(engine, console, shared, "locked.docx", "open.docx");

		assertEquals(ExitCode.PARTIAL_FAILURE, exitCode);
		assertTrue(anyContains(console.errorLines, "locked.docx"));
	}

	@Test
	public void noPasswordAtAllFailsTheSameWayAWrongOneDoes()
	{
		// The engine reports both as IGR_E_PASSWORD, so the CLI cannot tell them
		// apart and must not pretend to.
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.addProtectedDocument("locked.docx", "hunter2", "secret");

		RecordingConsole console = new RecordingConsole();
		int exitCode = runConvert(engine, console, shared(), "locked.docx");

		assertEquals(ExitCode.PARTIAL_FAILURE, exitCode);
		assertTrue(anyContains(console.errorLines, "locked.docx"));
	}

	// ============================================ failures after opening

	/**
	 * A failure part-way through reading is still one failed input, not a crash.
	 *
	 * <p>Worth separating from an open failure because the document is open when it
	 * happens: the handle has to be closed on the way out, and the batch has to
	 * carry on. A corrupt document that opens and then fails on read is the common
	 * shape of it.
	 */
	@Test
	public void aReadFailureAfterOpeningIsStillOneFailedInput()
	{
		for (EngineError error : everyEngineError())
		{
			FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
			FakeDocument broken = engine.addDocument("broken.docx", "some text");
			broken.readFailure = error;
			engine.addDocument("fine.docx", "text");

			RecordingConsole console = new RecordingConsole();
			int exitCode = runConvert(engine, console, "broken.docx", "fine.docx");

			assertEquals(ExitCode.PARTIAL_FAILURE, exitCode, error.name());
			assertTrue(anyContains(console.errorLines, "broken.docx"), error.name());
			assertTrue(broken.isDisposed, "the document must be closed even when reading it failed: " + error.name());
		}
	}

	// ============================================ identify

	@Test
	public void identifyReportsAFailurePerInputAndKeepsGoing()
	{
		for (EngineError error : everyEngineError())
		{
			FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
			engine.failToOpen("bad.bin", error);
			FakeDocument good = engine.addDocument("good.bin");
			good.fileType = 254;
			good.mimeType = "application/vnd.ms-word";
			good.formatName = "Microsoft Word";

			RecordingConsole console = new RecordingConsole();
			IdentifyCommand command = new IdentifyCommand(new IdentifyOptions());
			SharedOptions shared = shared(null);
			shared.input.paths.add("bad.bin");
			shared.input.paths.add("good.bin");

			int exitCode = command.execute(context(engine, console, shared));

			assertEquals(ExitCode.PARTIAL_FAILURE, exitCode, error.name());

			boolean reported = false;
			for (String line : console.outLines)
				reported |= line.contains("good.bin") && line.contains("format=254");
			assertTrue(reported, error.name());
		}
	}

	@Test
	public void identifyNeverOpensTheDocument()
	{
		// identify asks what a file is, not what it says. Opening it would need a
		// password for a protected document and would read bytes it has no use for.
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.addProtectedDocument("locked.docx", "hunter2");

		RecordingConsole console = new RecordingConsole();
		SharedOptions shared = shared(null);
		shared.input.paths.add("locked.docx");

		int exitCode = new IdentifyCommand(new IdentifyOptions()).execute(context(engine, console, shared));

		assertEquals(ExitCode.SUCCESS, exitCode);
		assertTrue(anyStartsWith(engine.calls, "IdentifyFile("));
		assertTrue(!anyStartsWith(engine.calls, "OpenDocument("));
	}

	// ============================================ initialisation

	@Test
	public void anInitialisationFailureIsNotADocumentFailure()
	{
		// Nothing was processed, so this is the run never starting: exit 5, not the
		// partial-failure 1 that means "some files did not work".
		for (EngineError error : everyEngineError())
		{
			FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
			engine.initialisationFailure = error;

			InitialisationError thrown = assertThrows(InitialisationError.class, () -> documentFiltersSessionFactory(engine));

			assertEquals(ExitCode.INITIALISATION_FAILURE, thrown.getExitCode());
			assertEquals(ErrorCodes.INIT_FAILED, thrown.getCode());
		}
	}

	/** Mirrors what {@code DocumentFiltersSession.create} does, against the fake. */
	private static DocumentFiltersSession documentFiltersSessionFactory(FakeDocumentFiltersEngine engine)
	{
		try
		{
			engine.initialize(null, ".");
		}
		catch (EngineException error)
		{
			throw new InitialisationError("could not initialise Document Filters: " + error.getMessage());
		}

		return DocumentFiltersSession.forEngine(engine);
	}

	// ============================================ helpers

	private static int runConvert(FakeDocumentFiltersEngine engine, RecordingConsole console, String... paths)
	{
		return runConvert(engine, console, shared(), paths);
	}

	private static int runConvert(FakeDocumentFiltersEngine engine, RecordingConsole console, SharedOptions shared, String... paths)
	{
		for (String path : paths)
			shared.input.paths.add(path);

		ConvertOptions options = new ConvertOptions();
		options.format = FormatCatalog.resolve("text");
		ConvertCommand command = new ConvertCommand(options);

		return command.execute(context(engine, console, shared));
	}
}
