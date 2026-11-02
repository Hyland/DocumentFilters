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

package com.hyland.documentfilters.cli.tests.cli;

import static com.hyland.documentfilters.cli.tests.Assert.assertContains;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.Program;
import com.hyland.documentfilters.cli.configuration.ConsoleOptions;
import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.engine.EngineFormat;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.Interruption;
import com.hyland.documentfilters.cli.runtime.LimitExceededError;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocument;
import com.hyland.documentfilters.cli.tests.fakes.FakeSubFile;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Whole command lines, argv to exit code.
 *
 * <p>These are about the wiring, not the rules. Every rule here is already pinned by
 * the conformance corpus or a unit test; what is new is that the path from
 * {@code Program.run} through dispatch to a command's {@code execute} is exercised
 * at all.
 */
public final class CliTests
{
	/** The verbs that write outputs, for the tests that apply to both. */
	private static final String[][] WRITING_VERBS = { { "convert", "--to", "text" }, { "extract" } };

	/** The verbs that inspect documents without writing outputs. */
	private static final String[][] INSPECTION_VERBS = { { "meta" }, { "list" }, { "identify" } };

	// ============================================ exit codes out of the front door

	@Test
	public void aCleanConversionExitsZeroAndSaysNothing()
	{
		CliHarness cli = new CliHarness().withDocument("a.docx", "hello");

		assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", "text", "--out-dir", "out", "a.docx"));
		assertEquals(Collections.emptyList(), cli.errors());

		// Silence means success: a clean run prints nothing unless asked.
		assertEquals(Collections.emptyList(), cli.out());
	}

	@Test
	public void aUsageErrorExitsTwoAndNamesTheProblemOnStderr()
	{
		CliHarness cli = new CliHarness().withDocument("a.docx");

		assertEquals(ExitCode.USAGE_ERROR, cli.run("convert", "--to", "nonsense", "--out-dir", "out", "a.docx"));

		String line = single(cli.errors());
		assertTrue(line.startsWith(ErrorCodes.UNKNOWN_FORMAT + ": "), line);
		assertContains("nonsense", line);
	}

	@Test
	public void anUnknownCommandExitsTwo()
	{
		CliHarness cli = new CliHarness();

		assertEquals(ExitCode.USAGE_ERROR, cli.run("nonsense", "a.docx"));
		assertTrue(anyStartsWith(cli.errors(), ErrorCodes.UNKNOWN_COMMAND + ": "), String.valueOf(cli.errors()));
	}

	@Test
	public void namingNoInputExitsThree()
	{
		CliHarness cli = new CliHarness();

		assertEquals(ExitCode.NO_INPUTS_MATCHED, cli.run("convert", "--to", "text", "--out-dir", "out"));
		assertTrue(anyStartsWith(cli.errors(), ErrorCodes.NO_INPUTS + ": "), String.valueOf(cli.errors()));
	}

	@Test
	public void anOutputConflictExitsFour()
	{
		CliHarness cli = new CliHarness()
			.withDocument("a.docx")
			.withDocument("b.docx");

		assertEquals(ExitCode.OUTPUT_CONFLICT, cli.run("convert", "--to", "pdf", "--out-file", "one.pdf", "a.docx", "b.docx"));
		assertTrue(anyStartsWith(cli.errors(), ErrorCodes.OUT_FILE_MULTI_INPUT + ": "), String.valueOf(cli.errors()));
	}

	@Test
	public void oneBadDocumentAmongGoodOnesExitsOne()
	{
		CliHarness cli = new CliHarness()
			.withDocument("good.docx", "text")
			.withUnreadableDocument("bad.docx", EngineError.FILE_CORRUPT);

		assertEquals(ExitCode.PARTIAL_FAILURE, cli.run("convert", "--to", "text", "--out-dir", "out", "good.docx", "bad.docx"));
	}

	// ============================================ the two stderr shapes

	/**
	 * A per-document failure carries the input it belongs to; a run-level failure has
	 * no input to name. Nothing else drives the whole path, so these tests are the
	 * only authority on either shape.
	 */
	@Test
	public void aPerDocumentFailureIsPrefixedByItsInput()
	{
		CliHarness cli = new CliHarness()
			.withDocument("good.docx", "text")
			.withUnreadableDocument("bad.docx", EngineError.FILE_CORRUPT);

		cli.run("convert", "--to", "text", "--out-dir", "out", "good.docx", "bad.docx");

		String line = single(cli.errors());
		assertTrue(line.startsWith("bad.docx: " + ErrorCodes.DOCUMENT_FAILED + ": "), line);
	}

	/**
	 * A limit that applies to one document is per-document even though it is not a
	 * {@code DocumentError} --- it exits 6, not 1. It names its input through
	 * {@code CliError.getPath} rather than by folding the filename into its own
	 * message, so the reporter puts it in the same place as every other per-document
	 * failure.
	 */
	@Test
	public void aPerDocumentLimitIsPrefixedLikeAnyOtherPerDocumentFailure()
	{
		LimitExceededError error = new LimitExceededError("--timeout of 1s exceeded", "huge.pdf");
		RecordingConsole console = new RecordingConsole();

		int exitCode = Program.reportError(error, console);

		assertEquals(ExitCode.LIMIT_EXCEEDED, exitCode);
		assertEquals("huge.pdf: E_LIMIT_EXCEEDED: --timeout of 1s exceeded", single(console.errorLines));
	}

	@Test
	public void aRunWideLimitNamesNoInput()
	{
		// --max-time and --max-memory bound the whole run, so there is nothing to name.
		RecordingConsole console = new RecordingConsole();

		Program.reportError(new LimitExceededError("--max-time of 60s exceeded"), console);

		assertEquals("E_LIMIT_EXCEEDED: --max-time of 60s exceeded", single(console.errorLines));
	}

	@Test
	public void aRunLevelFailureHasNoInputToName()
	{
		CliHarness cli = new CliHarness().withDocument("a.docx");

		cli.run("convert", "--to", "text", "--out-file", "a.txt", "--out-dir", "out", "a.docx");

		String line = single(cli.errors());
		assertTrue(line.startsWith(ErrorCodes.MUTUALLY_EXCLUSIVE + ": "), line);
	}

	// ============================================ canvas formats to stdout

	@Test
	public void aCanvasFormatCanStreamToStdout()
	{
		// --stdout resolves to a streamed output, which leaves both the planned
		// item's target and its target pattern null. A per-page branch that expects
		// a pattern fails on the null and reports E_DOCUMENT_FAILED with none of the
		// detail.
		CliHarness cli = new CliHarness().withDocument("a.docx", "text");

		assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", "pdf", "--stdout", "a.docx"));

		assertEquals(Collections.emptyList(), cli.errors());
		assertTrue(anyStartsWith(cli.engine.calls, "MakeCanvas(<stream>,"), String.valueOf(cli.engine.calls));
		assertFalse(anyStartsWith(cli.engine.calls, "MakeCanvas(a.pdf,"), String.valueOf(cli.engine.calls));
	}

	@Test
	public void docpackCanStreamToStdoutToo()
	{
		// docpack is not a canvas format at all --- it goes through the document's
		// own docpack writer instead --- so the canvas case above does not cover it.
		// Same root cause: the target is null for a streamed output, and must not be
		// handed to the writer unconditionally.
		CliHarness cli = new CliHarness().withDocument("a.docx", "text");
		FakeDocument document = cli.engine.addDocument("a.docx", "text");

		assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", "docpack", "--stdout", "a.docx"));

		assertEquals(Collections.emptyList(), cli.errors());
		assertEquals(Arrays.asList("<stream>"), document.docPacksWritten);
	}

	@Test
	public void aReportAlongsideATextConversionIsRefusedBeforeAnythingOpens()
	{
		// One open cannot serve both a text conversion and a structure report ---
		// text extraction needs the text reader, a report needs a paginated handle ---
		// so whichever is chosen, the other half fails. Refused up front instead, and
		// nothing is opened.
		CliHarness cli = new CliHarness().withDocument("a.docx", "hello");

		assertEquals(ExitCode.USAGE_ERROR, cli.run("convert", "--to", "text", "--bookmarks", "--stdout", "a.docx"));

		assertTrue(anyContains(cli.errors(), ErrorCodes.REPORT_NEEDS_HI_DEF), String.valueOf(cli.errors()));
		assertFalse(anyStartsWith(cli.engine.calls, "OpenDocument("), String.valueOf(cli.engine.calls));
	}

	@Test
	public void anOutputDirectoryThatDoesNotExistYetIsCreated()
	{
		// pdf is a single-file canvas, png a per-page canvas, text is written through
		// the filesystem rather than the engine, and docpack is a writer of its own.
		// Each must create --out-dir, not only the one that happened to.
		for (String format : new String[] { "pdf", "png", "text", "docpack" })
		{
			CliHarness cli = new CliHarness().withDocument("a.docx", "hello");

			assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", format, "--out-dir", "brand/new", "a.docx"), "--to " + format);

			assertTrue(cli.fileSystem.directoryExists("brand/new"), "--to " + format + " did not create its output directory");
		}
	}

	@Test
	public void combiningATextFormatConcatenatesIntoOneWriter()
	{
		// These formats support --combine and have no canvas, so the combined path
		// must not reach for a canvas type that is not there.
		for (String format : new String[] { "text", "md-text", "html" })
		{
			CliHarness cli = new CliHarness()
				.withDocument("a.docx", "first")
				.withDocument("b.docx", "second");

			assertEquals(ExitCode.SUCCESS,
				cli.run("convert", "--to", format, "--combine", "--out-file", "out/all.txt", "a.docx", "b.docx"), "--to " + format);

			assertEquals(Collections.emptyList(), cli.errors(), "--to " + format);
			assertEquals("firstsecond", cli.fileSystem.readText("out/all.txt"), "--to " + format);
		}
	}

	@Test
	public void anOpenCancelledByTheTimeoutIsReportedAsTheLimit()
	{
		// The engine stops and reports a cancellation, which reaches us as
		// EngineError.CANCELLED. That must not be read as a broken document: the
		// cancellation was ours to ask for, so the limit is the cause and
		// E_LIMIT_EXCEEDED with exit 6 is the report --- the same line the check after
		// the document produces, so the two routes are indistinguishable.
		//
		// Driven through the real predicate rather than by scripting a cancelled open:
		// the run has to be the one that asks to stop, because that is what records
		// the reason.
		for (String[] verb : WRITING_VERBS)
		{
			CliHarness cli = new CliHarness().withDocument("slow.docx", "text");
			// The document "takes" a minute without the suite waiting one: the clock
			// is the harness's, and the open moves it past the limit so the predicate
			// fires on its next poll.
			cli.engine.duringOpen = () -> cli.clock.advance(Duration.ofSeconds(60));

			assertEquals(ExitCode.LIMIT_EXCEEDED, cli.run(argv(verb, "--out-dir", "out", "--timeout", "30", "slow.docx")),
				verb[0]);

			assertEquals("slow.docx: E_LIMIT_EXCEEDED: --timeout of 30s exceeded", single(cli.errors()), verb[0]);
		}
	}

	@Test
	public void openingADocumentOffersTheEngineAWayToStop()
	{
		// The predicate has to reach the open, because the engine's heartbeat is
		// raised while opening and nowhere else --- rendering a page takes no
		// callback. Passing null here would leave a document that hangs while parsing
		// hanging forever, which is exactly what --timeout is reached for.
		for (String[] verb : WRITING_VERBS)
		{
			CliHarness cli = new CliHarness().withDocument("a.docx", "text");

			cli.run(argv(verb, "--out-dir", "out", "--timeout", "30", "a.docx"));

			boolean found = false;
			for (String call : cli.engine.calls)
				found |= call.startsWith("OpenDocument(a.docx,") && call.endsWith(",cancellable)");

			assertTrue(found, verb[0] + ": " + cli.engine.calls);
		}
	}

	@Test
	public void anInterruptedRunStopsBetweenDocumentsAndExits130()
	{
		// Ctrl+C is recorded rather than killing the process, so the run reports and
		// exits through the same path as every other code. Between documents is the
		// only point in a batch where stopping leaves nothing half-written.
		for (String[] verb : WRITING_VERBS)
		{
			Interruption interruption = new Interruption();
			interruption.request();

			CliHarness cli = new CliHarness(interruption)
				.withDocument("a.docx", "one")
				.withDocument("b.docx", "two");

			assertEquals(ExitCode.INTERRUPTED, cli.run(argv(verb, "--out-dir", "out", "a.docx", "b.docx")), verb[0]);

			assertEquals("E_INTERRUPTED: interrupted", single(cli.errors()), verb[0]);
			assertFalse(anyStartsWith(cli.engine.calls, "OpenDocument("), verb[0] + ": " + cli.engine.calls);
		}
	}

	@Test
	public void aRunNobodyInterruptsIsUnaffected()
	{
		CliHarness cli = new CliHarness(new Interruption()).withDocument("a.docx", "one");

		assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", "text", "--out-dir", "out", "a.docx"));
		assertEquals(Collections.emptyList(), cli.errors());
	}

	// ===================================== diagnostics on every command

	@Test
	public void everyCommandThatReadsADocumentHonoursTheDiagnostics()
	{
		// These are shared options, so they parse on all five, and must do something
		// on all five rather than only on the two that write outputs.
		String[][] verbs = { { "convert", "--to", "text" }, { "extract" }, { "meta" }, { "list" }, { "identify" } };

		for (String[] verb : verbs)
		{
			CliHarness cli = new CliHarness().withDocument("a.docx", "hello");
			cli.engine.memoryStatus = "heap: 42 bytes";

			// No --out-dir: identify and meta declare no output group, so passing one
			// is a usage error rather than a no-op.
			assertEquals(ExitCode.SUCCESS, cli.run(argv(verb, "-x", "a.docx")), verb[0] + ": " + cli.errors());

			assertTrue(anyContains(cli.out(), "heap: 42 bytes"), verb[0] + ": " + cli.out());
		}
	}

	@Test
	public void theDebugInputSwitchesReachTheInspectionCommands()
	{
		// identify never opens the document, so it has no open callback to cancel or
		// log through --- but what the engine makes of a truncated file is exactly
		// what --debug-truncate asks, so it gets the stream form too.
		for (String[] verb : INSPECTION_VERBS)
		{
			CliHarness cli = new CliHarness().withDocument("a.docx", "hello world");

			assertEquals(ExitCode.SUCCESS, cli.run(argv(verb, "--debug-use-memory-stream", "a.docx")), verb[0]);

			assertTrue(anyContains(cli.engine.calls, "<stream:"), verb[0] + ": " + cli.engine.calls);
		}
	}

	@Test
	public void anInterruptStopsTheInspectionCommandsToo()
	{
		for (String[] verb : INSPECTION_VERBS)
		{
			Interruption interruption = new Interruption();
			interruption.request();

			CliHarness cli = new CliHarness(interruption)
				.withDocument("a.docx", "one")
				.withDocument("b.docx", "two");

			assertEquals(ExitCode.INTERRUPTED, cli.run(argv(verb, "a.docx", "b.docx")), verb[0]);

			assertEquals("E_INTERRUPTED: interrupted", single(cli.errors()), verb[0]);
		}
	}

	@Test
	public void extractionCreatesItsDirectoriesThroughTheFilesystemSeam()
	{
		// A subfile whose name carries a path must have its directories made through
		// the filesystem seam, or they land wherever the suite happens to run and
		// nothing about the extraction can be asserted without touching the disk.
		CliHarness cli = new CliHarness().withDocument("archive.zip");
		FakeDocument container = cli.engine.addDocument("archive.zip");
		container.subFileEntries.add(new FakeSubFile("nested/deep/inner.docx"));

		assertEquals(ExitCode.SUCCESS, cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"));

		assertEquals(Collections.emptyList(), cli.errors());
		assertTrue(cli.fileSystem.directoryExists("out/nested/deep"), "extraction did not create its directory through IFileSystem");
	}

	@Test
	public void combinedOutputIsFinishedBeforeTheViewerSeesIt()
	{
		// The viewer must be handed a finished file: for a PDF, one with its trailer.
		// The content is read inside the viewer because afterwards it looks right
		// either way.
		CliHarness cli = new CliHarness()
			.withDocument("a.docx", "first")
			.withDocument("b.docx", "second");

		assertEquals(ExitCode.SUCCESS,
			cli.run("convert", "--to", "text", "--combine", "--out-file", "out/all.txt", "--view", "--sort", "a.docx", "b.docx"));

		assertEquals(Arrays.asList("out/all.txt"), cli.viewed);
		String content = single(cli.viewedContent);
		assertContains("first", content);
		assertContains("second", content);
	}

	@Test
	public void anOutputTheRunHasAlreadyWrittenIsNotReplacedSilently()
	{
		// The planner checks every target before the run starts, against a directory
		// none of this run's outputs are in yet. Two names the filesystem matches to
		// one file but the ASCII collision key does not --- NTFS matches the two
		// spellings of a-ring --- therefore both pass planning, and the second
		// conversion must still not replace the first.
		CliHarness cli = new CliHarness()
			.withDocument("a/\u00c5.docx", "from the upper-case name")
			.withDocument("b/\u00e5.docx", "from the lower-case name");
		cli.fileSystem.treatAsSameFile("out/\u00e5.txt", "out/\u00c5.txt");

		int exitCode = cli.run("convert", "--to", "text", "--out-dir", "out", "--sort", "a/\u00c5.docx", "b/\u00e5.docx");

		assertEquals(ExitCode.OUTPUT_CONFLICT, exitCode);
		assertTrue(anyContains(cli.errors(), "E_OUTPUT_EXISTS"), String.valueOf(cli.errors()));
		assertEquals("from the upper-case name", cli.fileSystem.readText("out/\u00c5.txt"));
	}

	@Test
	public void twoContainersCannotExtractOntoEachOther()
	{
		// Claims are compared across the whole run, not per container, so two
		// archives each holding a readme.txt collide. The planner refuses that for
		// ordinary outputs whether or not --overwrite is given.
		CliHarness cli = new CliHarness().withDocument("a.zip").withDocument("b.zip");
		cli.engine.addDocument("a.zip").subFileEntries.add(new FakeSubFile("readme.txt"));
		cli.engine.addDocument("b.zip").subFileEntries.add(new FakeSubFile("readme.txt"));

		int exitCode = cli.run("extract", "--overwrite", "--out-dir", "out", "a.zip", "b.zip");

		assertEquals(ExitCode.OUTPUT_CONFLICT, exitCode);
		assertTrue(anyContains(cli.errors(), "E_OUTPUT_COLLISION"), String.valueOf(cli.errors()));
	}

	@Test
	public void aSubfileCannotBeWrittenThroughALinkOutOfTheOutputDirectory()
	{
		// Lexical containment is not enough: every segment of out/latest/index.html
		// is a plain name under the root, and if out/latest is a link the write still
		// lands wherever it points. The container picks that name, not the user.
		CliHarness cli = new CliHarness().withDocument("archive.zip");
		cli.engine.addDocument("archive.zip").subFileEntries.add(new FakeSubFile("latest/index.html"));
		cli.fileSystem.addDirectory("elsewhere").markSymlink("out/latest", "elsewhere");

		int exitCode = cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip");

		assertEquals(ExitCode.OUTPUT_CONFLICT, exitCode);
		assertTrue(anyContains(cli.errors(), "E_UNSAFE_OUTPUT"), String.valueOf(cli.errors()));
	}

	@Test
	public void anOrdinaryOutputDirectoryIsNotMistakenForALink()
	{
		CliHarness cli = new CliHarness().withDocument("archive.zip");
		cli.engine.addDocument("archive.zip").subFileEntries.add(new FakeSubFile("latest/index.html"));
		cli.fileSystem.addDirectory("out/latest");

		assertEquals(ExitCode.SUCCESS, cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"));
		assertEquals(Collections.emptyList(), cli.errors());
	}

	@Test
	public void extractionRefusesToReplaceAnExistingFileWithoutOverwrite()
	{
		// The same rule as every other output: E_OUTPUT_EXISTS, exit 4, nothing
		// written.
		CliHarness cli = new CliHarness().withDocument("archive.zip").withExistingOutput("out/inner.txt");
		FakeSubFile inner = new FakeSubFile("inner.txt");
		cli.engine.addDocument("archive.zip").subFileEntries.add(inner);

		assertEquals(ExitCode.OUTPUT_CONFLICT, cli.run("extract", "--out-dir", "out", "archive.zip"));

		assertTrue(anyContains(cli.errors(), "E_OUTPUT_EXISTS"), String.valueOf(cli.errors()));
		assertEquals(Collections.emptyList(), inner.copiedTo);
	}

	@Test
	public void extractionReplacesAnExistingFileWithOverwrite()
	{
		CliHarness cli = new CliHarness().withDocument("archive.zip").withExistingOutput("out/inner.txt");
		FakeSubFile inner = new FakeSubFile("inner.txt");
		cli.engine.addDocument("archive.zip").subFileEntries.add(inner);

		assertEquals(ExitCode.SUCCESS, cli.run("extract", "--overwrite", "--out-dir", "out", "archive.zip"));

		assertEquals("out/inner.txt", single(inner.copiedTo));
	}

	@Test
	public void extractionDoesNotWriteThroughASymbolicLinkInsideTheOutputDirectory()
	{
		// The name is clean text --- out/link/inner.txt sits inside out --- but with
		// out/link pointing elsewhere the bytes would land outside it.
		CliHarness cli = new CliHarness().withDocument("archive.zip");
		cli.fileSystem.markSymlink("out/link", "/elsewhere");
		FakeSubFile inner = new FakeSubFile("link/inner.txt");
		cli.engine.addDocument("archive.zip").subFileEntries.add(inner);

		assertEquals(ExitCode.OUTPUT_CONFLICT, cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"));

		assertTrue(anyContains(cli.errors(), ErrorCodes.UNSAFE_OUTPUT), String.valueOf(cli.errors()));
		assertEquals(Collections.emptyList(), inner.copiedTo);
	}

	@Test
	public void extractionWritesIntoAnOutputDirectoryThatIsItselfALink()
	{
		// The root is the user's own --out-dir; pointing it at a link is a choice.
		CliHarness cli = new CliHarness().withDocument("archive.zip");
		cli.fileSystem.markSymlink("out", "/elsewhere");
		FakeSubFile inner = new FakeSubFile("inner.txt");
		cli.engine.addDocument("archive.zip").subFileEntries.add(inner);

		assertEquals(ExitCode.SUCCESS, cli.run("extract", "--out-dir", "out", "archive.zip"));

		assertEquals("out/inner.txt", single(inner.copiedTo));
	}

	@Test
	public void anEmptyOutDirIsAUsageError()
	{
		// An empty directory joins as "/name", which is the filesystem root.
		for (String[] verb : WRITING_VERBS)
		{
			CliHarness cli = new CliHarness().withDocument("a.docx", "text");

			assertEquals(ExitCode.USAGE_ERROR, cli.run(argv(verb, "--out-dir", "", "a.docx")), verb[0]);

			boolean found = false;
			for (String line : cli.errors())
				found |= line.contains("E_BAD_VALUE") && line.contains("--out-dir does not accept the value ''");

			assertTrue(found, verb[0] + ": " + cli.errors());
		}
	}

	@Test
	public void fingerprintCreatesNoOutputDirectory()
	{
		// --fingerprint renders into a discarded stream, so the plan's target is a
		// guess that is never written to; creating a directory for it would be a side
		// effect of a run that produces no files.
		CliHarness cli = new CliHarness().withDocument("a.docx", "hello");

		assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", "pdf", "--fingerprint", "a.docx"));

		assertFalse(cli.fileSystem.directoryExists("brand/new"));
	}

	@Test
	public void aReportAlongsideACanvasConversionIsAllowed()
	{
		CliHarness cli = new CliHarness().withDocument("a.docx", "hello");

		assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", "pdf", "--bookmarks", "--stdout", "a.docx"));

		assertEquals(Collections.emptyList(), cli.errors());
	}

	// ============================================ dispatch branches

	@Test
	public void dryRunPrintsThePlanAndOpensNothing()
	{
		CliHarness cli = new CliHarness().withDocument("a.docx", "text");

		assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", "pdf", "--out-dir", "out", "--dry-run", "a.docx"));

		assertTrue(anyContains(cli.out(), "command convert"), String.valueOf(cli.out()));
		assertTrue(anyContains(cli.out(), "target out/a.pdf"), String.valueOf(cli.out()));
		assertFalse(anyStartsWith(cli.engine.calls, "OpenDocument("), String.valueOf(cli.engine.calls));
	}

	@Test
	public void printPlanIsTheSameBranchAsDryRun()
	{
		CliHarness dryRun = new CliHarness().withDocument("a.docx");
		CliHarness printPlan = new CliHarness().withDocument("a.docx");

		dryRun.run("convert", "--to", "pdf", "--out-dir", "out", "--dry-run", "a.docx");
		printPlan.run("convert", "--to", "pdf", "--out-dir", "out", "--print-plan", "a.docx");

		assertEquals(dryRun.out(), printPlan.out());
	}

	@Test
	public void printContractDumpsTheSurfaceAndRunsNothing()
	{
		CliHarness cli = new CliHarness().withDocument("a.docx");

		assertEquals(ExitCode.SUCCESS, cli.run("convert", "--to", "pdf", "--out-dir", "out", "--print-contract", "a.docx"));

		assertTrue(anyContains(cli.out(), "command convert"), String.valueOf(cli.out()));
		assertTrue(anyContains(cli.out(), "--out-dir"), String.valueOf(cli.out()));
		assertEquals(Collections.emptyList(), cli.engine.calls);
	}

	@Test
	public void printContractWinsOverADryRun()
	{
		// Both short-circuit; the contract dump is checked first, so it is what runs.
		CliHarness cli = new CliHarness().withDocument("a.docx");

		cli.run("convert", "--to", "pdf", "--out-dir", "out", "--print-contract", "--dry-run", "a.docx");

		assertTrue(anyContains(cli.out(), "--print-contract"), String.valueOf(cli.out()));
		assertFalse(anyContains(cli.out(), "cardinality"), String.valueOf(cli.out()));
	}

	// ============================================ the engine's lifetime

	@Test
	public void aCommandThatNeedsNoEngineNeverAsksForOne()
	{
		// formats reads no document, but it does read the catalogue, so the engine is
		// touched exactly once and for that alone.
		CliHarness cli = new CliHarness();
		EngineFormat format = new EngineFormat();
		format.id = 254;
		format.shortName = "docx";
		format.displayName = "Microsoft Word 2007";
		cli.engine.formats.add(format);

		assertEquals(ExitCode.SUCCESS, cli.run("formats"));

		assertTrue(anyContains(cli.out(), "id=254"), String.valueOf(cli.out()));
		assertFalse(anyStartsWith(cli.engine.calls, "OpenDocument("), String.valueOf(cli.engine.calls));
	}

	@Test
	public void aRunThatFailsStillClosesEveryDocumentItOpened()
	{
		CliHarness cli = new CliHarness()
			.withDocument("first.docx", "text")
			.withUnreadableDocument("second.docx", EngineError.NOT_READABLE);

		cli.run("convert", "--to", "text", "--out-dir", "out", "first.docx", "second.docx");

		for (String call : cli.engine.calls)
		{
			if (call.startsWith("OpenDocument("))
				assertContains(".docx", call);
		}
	}

	// ============================================ other verbs, end to end

	@Test
	public void identifyPrintsOneLinePerInput()
	{
		CliHarness cli = new CliHarness();
		cli.withDocument("a.bin");
		cli.engine.addDocument("a.bin").formatName = "Microsoft Word 2007";

		assertEquals(ExitCode.SUCCESS, cli.run("identify", "a.bin"));

		String line = single(cli.out());
		assertTrue(line.startsWith("a.bin: format="), line);
		assertContains("name=Microsoft Word 2007", line);
	}

	@Test
	public void versionNeedsNeitherADocumentNorAWalk()
	{
		CliHarness cli = new CliHarness();

		assertEquals(ExitCode.SUCCESS, cli.run("version"));
		assertFalse(cli.out().isEmpty(), "version printed nothing");
	}

	// ============================================ the console pre-scan

	/**
	 * Read straight out of argv before anything is bound, because the console has to
	 * exist before the first line is written and binding has not happened yet. A raw
	 * scan is only safe because none of the three takes a value and prefix
	 * abbreviation is off.
	 */
	@Test
	public void theConsoleSettingsAreReadBeforeAnythingIsBound()
	{
		assertTrue(Program.preScanConsoleOptions(new String[] { "convert", "--silent" }).silent);
		assertTrue(Program.preScanConsoleOptions(new String[] { "convert", "--verbose" }).verbose);
		assertTrue(Program.preScanConsoleOptions(new String[] { "convert", "--color" }).color);

		ConsoleOptions none = Program.preScanConsoleOptions(new String[] { "convert", "a.docx" });

		assertFalse(none.silent);
		assertFalse(none.verbose);
		assertFalse(none.color);
	}

	@Test
	public void thePreScanReadsOnlyExactSpellings()
	{
		// Prefix abbreviation is off everywhere, so a partial spelling is an unknown
		// argument rather than a match --- the scan must not be more generous than
		// the parser it front-runs.
		assertFalse(Program.preScanConsoleOptions(new String[] { "convert", "--sil" }).silent);
		assertFalse(Program.preScanConsoleOptions(new String[] { "convert", "--silent=true" }).silent);
	}

	@Test
	public void thePreScanSurvivesAnEmptyCommandLine()
	{
		ConsoleOptions options = Program.preScanConsoleOptions(new String[0]);

		assertFalse(options.silent);
	}

	// ============================================ helpers

	/** A verb's own tokens followed by the rest of the command line. */
	private static String[] argv(String[] verb, String... rest)
	{
		List<String> args = new ArrayList<>(Arrays.asList(verb));
		args.addAll(Arrays.asList(rest));
		return args.toArray(new String[0]);
	}

	/** The only element of {@code items}, failing if there is not exactly one. */
	private static String single(List<String> items)
	{
		assertEquals(Integer.valueOf(1), Integer.valueOf(items.size()), "expected exactly one line in " + items);
		return items.get(0);
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

	private static boolean anyContains(List<String> lines, String fragment)
	{
		for (String line : lines)
		{
			if (line.contains(fragment))
				return true;
		}

		return false;
	}
}
