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

import static com.hyland.documentfilters.cli.tests.Assert.assertContains;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertNotNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.commands.CommandExecution;
import com.hyland.documentfilters.cli.commands.ConvertCommand;
import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.DiagnosticOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.execution.CanvasConversion;
import com.hyland.documentfilters.cli.execution.DebugInput;
import com.hyland.documentfilters.cli.execution.Diagnostics;
import com.hyland.documentfilters.cli.execution.DocumentFiltersSession;
import com.hyland.documentfilters.cli.execution.EngineLog;
import com.hyland.documentfilters.cli.execution.ExtractorOpenSettings;
import com.hyland.documentfilters.cli.execution.StopReason;
import com.hyland.documentfilters.cli.execution.StopSignal;
import com.hyland.documentfilters.cli.planning.FormatCatalog;
import com.hyland.documentfilters.cli.planning.OutputCardinality;
import com.hyland.documentfilters.cli.planning.PlannedItem;
import com.hyland.documentfilters.cli.planning.RunPlan;
import com.hyland.documentfilters.cli.planning.TargetPattern;
import com.hyland.documentfilters.cli.runtime.DocumentError;
import com.hyland.documentfilters.cli.runtime.InterruptedError;
import com.hyland.documentfilters.cli.runtime.Interruption;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocumentFiltersEngine;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SeekableByteChannel;
import java.time.Duration;
import java.util.Arrays;
import java.util.Collections;

/**
 * The diagnostic switches, each of which used to parse and then do nothing.
 *
 * <p>Nine arguments were accepted, validated, and never read again. That is worse
 * than not having them: the help text promises something, the validator agrees
 * the command line is sensible, and the run behaves exactly as if the argument
 * had been left out. These tests are what stops one quietly returning to that
 * state.
 */
public final class DiagnosticSwitchTests
{
	@Test
	public void tempDirIsGivenToTheEngine()
	{
		// A library-wide setting, applied once after initialize rather than per
		// document: IGR_Multiplex(IGR_Multi_Set_Temp_Path), as doc2text does it.
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();

		DocumentFiltersSession.forEngine(engine);
		engine.setTempPath("/scratch");

		assertEquals("/scratch", engine.tempPath);
		assertTrue(engine.calls.contains("SetTempPath(/scratch)"), String.valueOf(engine.calls));
	}

	@Test
	public void engineMemoryIsReportedAtTheLevelAskedFor()
	{
		// { level, per document, per run }: neither switch, -x for the run only, and
		// --xx for every document too.
		for (Object[] row : new Object[][] { { 0, false, false }, { 1, false, true }, { 2, true, true } })
		{
			int level = (Integer) row[0];
			boolean perDocument = (Boolean) row[1];
			boolean perRun = (Boolean) row[2];

			FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
			engine.memoryStatus = "heap: 42 bytes";
			RecordingConsole console = new RecordingConsole();
			DiagnosticOptions options = new DiagnosticOptions();
			options.memoryStatusLevel = level;
			Diagnostics diagnostics = new Diagnostics(options, console, engine, null);

			diagnostics.beginRun();
			diagnostics.beginDocument("a.docx", 1, 1);
			diagnostics.endDocument("a.docx");
			int afterDocument = console.outLines.size();
			diagnostics.endRun(1, 0);

			String context = "level " + level;
			assertEquals(perDocument ? 1 : 0, afterDocument, context);
			assertEquals((perDocument ? 1 : 0) + (perRun ? 1 : 0), console.outLines.size(), context);

			if (perRun)
				assertTrue(console.outLines.stream().anyMatch(line -> line.contains("heap: 42 bytes")), context);
		}
	}

	@Test
	public void logmemReportsTheProcessesOwnPeak()
	{
		// The process, not the engine: -x and --xx answer the other question.
		RecordingConsole console = new RecordingConsole();
		DiagnosticOptions options = new DiagnosticOptions();
		options.logMemory = true;
		new Diagnostics(options, console).endRun(1, 0);

		assertTrue(console.outLines.stream().anyMatch(line -> line.startsWith("peak memory: ") && line.endsWith("MB")),
			String.valueOf(console.outLines));
	}

	// ------------------------------------------------------------ the stop predicate

	private static DiagnosticOptions withTimeout(Integer seconds)
	{
		DiagnosticOptions options = new DiagnosticOptions();
		options.timeout = seconds;
		return options;
	}

	/**
	 * When the deadline has passed <em>and</em> the user has interrupted, the run
	 * reports the interrupt.
	 *
	 * <p>The reason is settled by the poll that stopped the document rather than
	 * worked out afterwards, which is what makes this answerable at all: by the time
	 * a cancelled open is caught both conditions can be true, and asking then is a
	 * second guess at a question already answered.
	 */
	@Test
	public void anInterruptOutranksADeadlineThatAlsoPassed()
	{
		DiagnosticsTests.TestClock clock = new DiagnosticsTests.TestClock();
		Diagnostics diagnostics = new Diagnostics(withTimeout(30), new RecordingConsole(), null, clock::read);
		diagnostics.beginDocument("slow.docx", 1, 1);
		clock.advance(Duration.ofSeconds(31));

		Interruption interruption = new Interruption();
		interruption.request();

		StopSignal stop = CommandExecution.watchForStop(diagnostics, interruption);

		assertTrue(stop.shouldStop());
		assertEquals(StopReason.INTERRUPTED, stop.getReason());
		assertEquals(InterruptedError.class, stop.cause("slow.docx").getClass());
	}

	/** A cancelled open nobody asked for is the document failing, not a limit. */
	@Test
	public void aCancellationWithNoRecordedReasonIsADocumentError()
	{
		Diagnostics diagnostics = new Diagnostics(new DiagnosticOptions(), new RecordingConsole());
		diagnostics.beginDocument("a.docx", 1, 1);

		StopSignal stop = CommandExecution.watchForStop(diagnostics, null);

		assertFalse(stop.shouldStop());
		assertEquals(DocumentError.class, stop.cause("a.docx").getClass());
	}

	/**
	 * Shared by every command that opens a document. It was four copies of the same
	 * two lines until this moved, which is four places for one of them to quietly
	 * lose a term --- and neither term is visible from the outside, since both
	 * produce the same cancelled open.
	 */
	@Test
	public void nothingStopsARunInsideItsLimitsThatNobodyInterrupted()
	{
		Diagnostics diagnostics = new Diagnostics(withTimeout(600), new RecordingConsole());
		diagnostics.beginDocument("a.docx", 1, 1);

		assertFalse(CommandExecution.watchForStop(diagnostics, new Interruption()).shouldStop());
		assertFalse(CommandExecution.watchForStop(diagnostics, null).shouldStop());
	}

	@Test
	public void theDocumentsDeadlineStopsIt()
	{
		DiagnosticsTests.TestClock clock = new DiagnosticsTests.TestClock();
		Diagnostics diagnostics = new Diagnostics(withTimeout(30), new RecordingConsole(), null, clock::read);
		diagnostics.beginDocument("slow.docx", 1, 1);
		clock.advance(Duration.ofSeconds(31));

		StopSignal stop = CommandExecution.watchForStop(diagnostics, null);

		assertTrue(stop.shouldStop());
		assertEquals(StopReason.TIMED_OUT, stop.getReason());
	}

	@Test
	public void anInterruptStopsItEvenWithTimeToSpare()
	{
		Diagnostics diagnostics = new Diagnostics(withTimeout(600), new RecordingConsole());
		diagnostics.beginDocument("a.docx", 1, 1);

		Interruption interruption = new Interruption();
		StopSignal stop = CommandExecution.watchForStop(diagnostics, interruption);

		assertFalse(stop.shouldStop());
		assertNull(stop.getReason());

		interruption.request();

		assertTrue(stop.shouldStop());
		assertEquals(StopReason.INTERRUPTED, stop.getReason());
	}

	// -------------------------------------------------------------------- --no-images

	@Test
	public void noImagesSuppressesThemForTheMarkdownFormats()
	{
		for (String token : new String[] { "md", "md-text" })
			assertEquals("MARKDOWN_INCLUDE_IMAGES=OFF", CanvasConversion.imageSuppression(FormatCatalog.resolve(token)), token);
	}

	@Test
	public void noImagesIsIgnoredWhereItHasNoMeaning()
	{
		// html5 inlines the page raster itself, so suppressing would blank it; text
		// has nowhere to put an image in the first place.
		for (String token : new String[] { "html5", "svg", "text", "pdf" })
			assertEquals("", CanvasConversion.imageSuppression(FormatCatalog.resolve(token)), token);
	}

	@Test
	public void noImagesReachesTheCanvasOptionsForHiDefMarkdown()
	{
		assertEquals("MARKDOWN_INCLUDE_IMAGES=OFF", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("md"), null, true));

		assertEquals("", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("md"), null, false));
	}

	@Test
	public void noImagesReachesTheOpenOptionsForTextModeMarkdown()
	{
		// md-text never opens a canvas, so the same rule has to arrive at open.
		ConvertOptions options = new ConvertOptions();
		options.noImages = true;
		ExtractorOpenSettings settings = ExtractorOpenSettings.forFormat(FormatCatalog.resolve("md-text"), options, new SharedOptions());

		assertContains("MARKDOWN_INCLUDE_IMAGES=OFF", settings.options);
	}

	// ------------------------------------------------------ --log-level/--log-modules

	@Test
	public void noLogLevelMeansNoLogAtAll()
	{
		// Null rather than a disabled instance, so the engine is handed no callback
		// and never composes a message nobody prints.
		assertNull(EngineLog.forOptions(new DiagnosticOptions(), new RecordingConsole()));
	}

	@Test
	public void everyModuleLogsWhenNoneAreNamed()
	{
		DiagnosticOptions options = new DiagnosticOptions();
		options.logLevel = "debug";
		EngineLog log = EngineLog.forOptions(options, new RecordingConsole());

		assertEquals(EngineLog.LEVEL_DEBUG, log.levelFor("pdf"));
		assertEquals(EngineLog.LEVEL_DEBUG, log.levelFor("anything"));
	}

	@Test
	public void namingModulesSilencesTheRest()
	{
		// The filter is applied by answering "none" for a module nobody asked about,
		// which is where --log-modules actually takes effect --- the engine has no
		// notion of the user's list.
		DiagnosticOptions options = new DiagnosticOptions();
		options.logLevel = "trace";
		options.logModules = "pdf,ole";
		EngineLog log = EngineLog.forOptions(options, new RecordingConsole());

		assertEquals(EngineLog.LEVEL_TRACE, log.levelFor("pdf"));
		assertEquals(EngineLog.LEVEL_TRACE, log.levelFor("OLE"));
		assertEquals(EngineLog.LEVEL_NONE, log.levelFor("html"));
	}

	@Test
	public void theDebugSpellingsAreTheSameSwitch()
	{
		DiagnosticOptions options = new DiagnosticOptions();
		options.debugLogLevel = "warning";
		options.debugLogModules = "pdf";
		EngineLog log = EngineLog.forOptions(options, new RecordingConsole());

		assertEquals(EngineLog.LEVEL_WARNING, log.levelFor("pdf"));
		assertEquals(EngineLog.LEVEL_NONE, log.levelFor("xls"));
	}

	@Test
	public void aLogMessageGoesToStderrWithItsLevelAndModule()
	{
		// stderr, as doc2text does. It shares the stream with the two contract error
		// shapes, which is safe because nothing logs unless --log-level was passed.
		RecordingConsole console = new RecordingConsole();
		DiagnosticOptions options = new DiagnosticOptions();
		options.logLevel = "debug";
		EngineLog log = EngineLog.forOptions(options, console);

		log.write(EngineLog.LEVEL_DEBUG, "pdf", "parsing xref");

		assertEquals(1, console.errorLines.size());
		assertContains("DEBUG [pdf] parsing xref", console.errorLines.get(0));
		assertTrue(console.outLines.isEmpty());
	}

	@Test
	public void anUnknownLevelLogsNothingRatherThanFailing()
	{
		// doc2text's own parse falls through to none; a typo should not end a run
		// that was otherwise fine.
		DiagnosticOptions options = new DiagnosticOptions();
		options.logLevel = "chatty";

		assertNull(EngineLog.forOptions(options, new RecordingConsole()));
	}

	// -------------------------------------------------------------------------- --view

	private static PlannedItem item(String sourcePath, String target)
	{
		PlannedItem item = new PlannedItem();
		item.sourcePath = sourcePath;
		item.target = target;
		return item;
	}

	@Test
	public void viewOpensTheOneFileARunProduced()
	{
		RunPlan plan = new RunPlan();
		plan.cardinality = OutputCardinality.PER_INPUT;
		plan.items.add(item("a.docx", "out/a.pdf"));

		assertEquals("out/a.pdf", ConvertCommand.singleOutputOf(plan));
	}

	@Test
	public void viewOpensTheCombinedTargetWhenThereIsOne()
	{
		RunPlan plan = new RunPlan();
		plan.cardinality = OutputCardinality.COMBINED;
		plan.combinedTarget = "out/book.pdf";
		plan.items.add(item("a.docx", null));
		plan.items.add(item("b.docx", null));

		assertEquals("out/book.pdf", ConvertCommand.singleOutputOf(plan));
	}

	@Test
	public void viewOpensNothingWhenARunProducedSeveralFiles()
	{
		// "Single output" is the spec's own wording: several inputs, or one input to
		// a per-page format, leave nothing unambiguous to open.
		RunPlan many = new RunPlan();
		many.cardinality = OutputCardinality.PER_INPUT;
		many.items.add(item("a.docx", "out/a.pdf"));
		many.items.add(item("b.docx", "out/b.pdf"));

		assertNull(ConvertCommand.singleOutputOf(many));

		RunPlan perPage = new RunPlan();
		perPage.cardinality = OutputCardinality.PER_INPUT;
		PlannedItem pages = item("a.docx", null);
		pages.targetPattern = new TargetPattern();
		pages.targetPattern.directory = "out";
		pages.targetPattern.extension = "png";
		perPage.items.add(pages);

		assertNull(ConvertCommand.singleOutputOf(perPage));
	}

	// ---------------------------------------------------------------- the debug streams

	private static InMemoryFileSystem fileHolding(String contents)
	{
		return new InMemoryFileSystem().addFile("a.docx", contents);
	}

	@Test
	public void nothingWrapsTheInputWhenNoDebugSwitchIsSet() throws IOException
	{
		assertNull(DebugInput.forInput("a.docx", new DiagnosticOptions(), fileHolding("hello"), new RecordingConsole(), "a.docx"));
	}

	@Test
	public void useMemoryStreamReadsTheWholeDocumentIntoMemory() throws IOException
	{
		DiagnosticOptions options = new DiagnosticOptions();
		options.useMemoryStream = true;

		try (SeekableByteChannel stream = DebugInput.forInput("a.docx", options, fileHolding("hello world"), new RecordingConsole(),
			"a.docx"))
		{
			assertNotNull(stream);
			assertEquals(11L, stream.size());
		}
	}

	@Test
	public void truncateReadsOnlyTheFirstBytes() throws IOException
	{
		DiagnosticOptions options = new DiagnosticOptions();
		options.truncateInputBytes = 5L;

		try (SeekableByteChannel stream = DebugInput.forInput("a.docx", options, fileHolding("hello world"), new RecordingConsole(),
			"a.docx"))
		{
			assertEquals(5L, stream.size());
		}
	}

	@Test
	public void truncatingPastTheEndStopsAtTheEnd() throws IOException
	{
		DiagnosticOptions options = new DiagnosticOptions();
		options.truncateInputBytes = 4096L;

		try (SeekableByteChannel stream = DebugInput.forInput("a.docx", options, fileHolding("short"), new RecordingConsole(), "a.docx"))
		{
			assertEquals(5L, stream.size());
		}
	}

	@Test
	public void measuringComposesWithATruncatedSource() throws IOException
	{
		// The two switches answer independent questions --- where the bytes come from,
		// and whether the asking is counted --- so asking both must not drop one. An
		// if/else chain here silently ignored --debug-measure-io whenever a truncated
		// or buffered source was also requested.
		RecordingConsole console = new RecordingConsole();
		DiagnosticOptions options = new DiagnosticOptions();
		options.truncateInputBytes = 5L;
		options.measureIo = true;

		SeekableByteChannel stream = DebugInput.forInput("a.docx", options, fileHolding("hello world"), console, "a.docx");

		// Truncated to the prefix...
		assertEquals(5L, stream.size());

		// ...and still counting.
		stream.read(ByteBuffer.allocate(5));
		stream.close();

		assertEquals(Collections.singletonList("a.docx: reads=1 seeks=0 bytes=5"), console.outLines);
	}

	@Test
	public void measuringComposesWithABufferedSource() throws IOException
	{
		RecordingConsole console = new RecordingConsole();
		DiagnosticOptions options = new DiagnosticOptions();
		options.useMemoryStream = true;
		options.measureIo = true;

		SeekableByteChannel stream = DebugInput.forInput("a.docx", options, fileHolding("hello world"), console, "a.docx");

		assertEquals(11L, stream.size());

		stream.read(ByteBuffer.allocate(11));
		stream.close();

		assertEquals(Arrays.asList("a.docx: reads=1 seeks=0 bytes=11"), console.outLines);
	}

	@Test
	public void measureIoCountsWhatTheEngineAskedForAndReportsOnClose() throws IOException
	{
		// Reported on close so the figures cover the whole document rather than
		// whatever had happened when someone thought to look.
		RecordingConsole console = new RecordingConsole();
		DiagnosticOptions options = new DiagnosticOptions();
		options.measureIo = true;

		SeekableByteChannel stream = DebugInput.forInput("a.docx", options, fileHolding("hello world"), console, "a.docx");

		stream.read(ByteBuffer.allocate(5));
		stream.position(0);
		assertTrue(console.outLines.isEmpty());

		stream.close();

		assertEquals(Arrays.asList("a.docx: reads=1 seeks=1 bytes=5"), console.outLines);
	}
}
