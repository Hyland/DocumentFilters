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

package com.hyland.documentfilters.cli.commands;

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.OptionsValidator;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.discovery.InputEnumerator;
import com.hyland.documentfilters.cli.discovery.InputSource;
import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.engine.EngineException;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.IDocumentPage;
import com.hyland.documentfilters.cli.engine.IOutputCanvas;
import com.hyland.documentfilters.cli.execution.CanvasConversion;
import com.hyland.documentfilters.cli.execution.Diagnostics;
import com.hyland.documentfilters.cli.execution.EngineLog;
import com.hyland.documentfilters.cli.execution.ExtractorOpenSettings;
import com.hyland.documentfilters.cli.execution.PasswordProvider;
import com.hyland.documentfilters.cli.execution.StopSignal;
import com.hyland.documentfilters.cli.execution.StructureReport;
import com.hyland.documentfilters.cli.execution.TextConversion;
import com.hyland.documentfilters.cli.planning.FormatKind;
import com.hyland.documentfilters.cli.planning.FormatSpec;
import com.hyland.documentfilters.cli.planning.OutputCardinality;
import com.hyland.documentfilters.cli.planning.OutputPlanner;
import com.hyland.documentfilters.cli.planning.PageRange;
import com.hyland.documentfilters.cli.planning.PlannedItem;
import com.hyland.documentfilters.cli.planning.RunPlan;
import com.hyland.documentfilters.cli.runtime.BatchRunner;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.DocumentError;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.MemoryChannel;
import com.hyland.documentfilters.cli.runtime.Viewer;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.UncheckedIOException;
import java.io.Writer;
import java.nio.channels.SeekableByteChannel;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;
import java.util.function.BiConsumer;

/**
 * {@code convert} --- render documents to a target format, and optionally report
 * document and page structure while doing so.
 */
public final class ConvertCommand implements ICommand
{
	/** The bound per-command options. Readable so the binding itself can be tested. */
	public final ConvertOptions options;

	public ConvertCommand(ConvertOptions options)
	{
		this.options = options;
	}

	@Override
	public String getName()
	{
		return "convert";
	}

	@Override
	public boolean requiresEngine()
	{
		return true;
	}

	@Override
	public int execute(CommandContext context)
	{
		return CommandExecution.withSession(context, () ->
		{
			SharedOptions shared = context.sharedOptions;
			OptionsValidator.validateConvert(shared, options);

			InputEnumerator enumerator = new InputEnumerator(context.fileSystem, context.console);
			List<InputSource> inputs = enumerator.enumerate(shared.input);

			RunPlan plan = OutputPlanner.plan(shared, options, inputs, context.fileSystem);
			// --fingerprint never writes to plan.items' resolved targets, so a real
			// file already sitting at one of those paths is not a conflict to report.
			if (!options.fingerprint)
				OutputPlanner.verifyTargets(plan, shared, context.fileSystem);

			Diagnostics diagnostics = new Diagnostics(shared.diagnostics, context.console, context.session.getApi(), context.clock);
			EngineLog log = EngineLog.forOptions(shared.diagnostics, context.console);
			diagnostics.beginRun();

			// --combine is allowed for text formats too, which have no canvas: their
			// documents concatenate into one writer instead. Keyed off the format's
			// kind rather than the cardinality alone, because a text format has no
			// canvas type to make a canvas from.
			FormatSpec combinedFormat = options.format;
			boolean combining = plan.cardinality == OutputCardinality.COMBINED && !shared.output.useStdout;
			if (combining)
				ensureOutputDirectory(context, OutputPlanner.directoryOf(plan.combinedTarget));

			// A block rather than resources living to the end of the method, because
			// the combined output has to be closed before --view below: it is not a
			// finished file until it is, a PDF's trailer being written on close.
			int exitCode;
			try (IOutputCanvas combinedCanvas = combining && combinedFormat.getKind() == FormatKind.CANVAS
					? context.session.getApi().makeCanvas(plan.combinedTarget, combinedFormat.getCanvasType(),
						CanvasConversion.composeCanvasOptions(combinedFormat, options.canvasOptions, options.noImages))
					: null;
				Writer combinedText = combining && combinedFormat.getKind() == FormatKind.TEXT
					? documentWriter(context.fileSystem.createFile(plan.combinedTarget))
					: null)
			{
				BatchRunner batch = new BatchRunner(shared.batch, context.console, context.interruption);
				CanvasConversion canvasConversion = new CanvasConversion(context.session, context.console, shared, context.fileSystem);
				int[] processed = { 0 };
				exitCode = batch.run(plan.items, item ->
				{
					processed[0]++;
					diagnostics.beginDocument(item.displayName, processed[0], plan.items.size());

					processDocument(context, shared, item, combinedCanvas, combinedText, canvasConversion, diagnostics, log);

					diagnostics.endDocument(item.displayName);
					diagnostics.checkDocumentTimeout(item.displayName);
					diagnostics.checkLimits();
					diagnostics.pauseIfRequested();
				});

				diagnostics.endRun(batch.getSuccessCount(), batch.getFailureCount());
			}
			catch (IOException e)
			{
				throw new UncheckedIOException(e.getMessage(), e);
			}

			// --view opens what was produced, once the run is over and the file is
			// closed. Only a run with one output has something unambiguous to open.
			if (options.view && exitCode == ExitCode.SUCCESS)
			{
				BiConsumer<String, IConsole> viewer = context.openViewer != null ? context.openViewer : Viewer::open;
				viewer.accept(singleOutputOf(plan), context.console);
			}

			return exitCode;
		});
	}

	/**
	 * Opens the document once with settings matching the format, converts, and
	 * closes it. Failures become a {@link DocumentError} for {@link BatchRunner} to
	 * count rather than ending the run.
	 *
	 * <p>The format is settled before the document is opened, which it has to be:
	 * {@link ExtractorOpenSettings} depends on it, and the engine applies those
	 * settings during {@code Open}.
	 */
	private void processDocument(CommandContext context, SharedOptions shared, PlannedItem item, IOutputCanvas combinedCanvas,
		Writer combinedText, CanvasConversion canvasConversion, Diagnostics diagnostics, EngineLog log)
	{
		FormatSpec format = options.format;
		PasswordProvider passwords = new PasswordProvider(shared.passwords, context.console, context.fileSystem);
		ExtractorOpenSettings openSettings = ExtractorOpenSettings.forFormat(format, options, shared);

		// Once per item, before anything opens an output. --fingerprint is skipped
		// because its target is a plan's guess that is never written to.
		if (!options.fingerprint)
			ensureOutputDirectory(context, item.outputDirectory());

		// A debug switch that changes how the bytes arrive hands the engine a channel
		// instead of the path. Opened out here, not inside the open's try, so it
		// outlives the document that reads from it.
		try (SeekableByteChannel debugInput = CommandExecution.debugInput(item.sourcePath, shared.diagnostics,
			context.fileSystem, context.console, item.displayName))
		{
			IDocument opened;
			StopSignal stop = CommandExecution.watchForStop(diagnostics, context.interruption);
			try
			{
				opened = debugInput == null
					? context.session.openDocument(item.sourcePath, openSettings, passwords, stop::shouldStop, log)
					: context.session.openDocument(debugInput, item.displayName, openSettings, passwords, stop::shouldStop, log);
			}
			catch (EngineException error)
			{
				// We asked for this, so it is the limit or the interrupt being reported, not a bad document.
				if (error.getError() == EngineError.CANCELLED)
					throw stop.cause(item.displayName);
				throw new DocumentError(item.sourcePath, "could not open document", error);
			}
			catch (CliError error)
			{
				throw error;
			}
			catch (RuntimeException error)
			{
				throw new DocumentError(item.sourcePath, "could not open document", error);
			}

			try (IDocument document = opened)
			{
				try
				{
					convertDocument(context, shared, item, format, document, combinedCanvas, combinedText, canvasConversion);
				}
				catch (CliError error)
				{
					throw error;
				}
				catch (IOException | RuntimeException error)
				{
					throw new DocumentError(item.sourcePath, CommandExecution.messageOf(error), error);
				}
			}
		}
		catch (IOException e)
		{
			throw new UncheckedIOException(e.getMessage(), e);
		}
	}

	private void convertDocument(CommandContext context, SharedOptions shared, PlannedItem item, FormatSpec format,
		IDocument document, IOutputCanvas combinedCanvas, Writer combinedText, CanvasConversion canvasConversion)
		throws IOException
	{
		StructureReport structure = new StructureReport(options.reports, context.console);
		structure.reportDocument(document, item.displayName);

		if (options.reports.any())
		{
			for (int pageIndex = 0; pageIndex < document.getPageCount(); pageIndex++)
			{
				try (IDocumentPage page = document.getPage(pageIndex))
				{
					structure.reportPage(page, pageIndex + 1, item.displayName);
				}
			}
		}

		switch (format.getKind())
		{
			case TEXT:
				convertToText(context, shared, item, format, document, options, combinedText);
				break;

			case DOCPACK:
				convertToDocPack(context, shared, item, document, options);
				break;

			default:
				if (combinedCanvas != null)
					canvasConversion.appendTo(combinedCanvas, document, options.pages != null ? options.pages : PageRange.ALL,
						options, item.displayName);
				else if (shared.output.useStdout)
					canvasConversion.convertToStream(document, context.console.openStandardOutput(), item, format, options);
				else
					canvasConversion.convert(document, item, format, options);
				break;
		}
	}

	/**
	 * The one file a run produced, or null when it produced none or several.
	 *
	 * <p>A per-page format resolves its names during execution, so even a single
	 * input can leave several files and there is no one thing to open.
	 */
	public static String singleOutputOf(RunPlan plan)
	{
		if (plan.cardinality == OutputCardinality.COMBINED)
			return plan.combinedTarget;

		return plan.items.size() == 1 ? plan.items.get(0).target : null;
	}

	/**
	 * Creates the directory an output is about to be written into, if it is not
	 * already there.
	 *
	 * <p>Here, in the orchestrator, rather than at each of the three places that
	 * open an output, because a canvas and a docpack are opened by the engine, which
	 * will not create a directory and reports a missing one as a per-document error
	 * that reads as though the input were missing.
	 */
	private static void ensureOutputDirectory(CommandContext context, String directory)
	{
		if (directory == null || directory.isEmpty())
			return;

		try
		{
			context.fileSystem.createDirectory(directory);
		}
		catch (IOException e)
		{
			throw new UncheckedIOException(e.getMessage(), e);
		}
	}

	private static void convertToText(CommandContext context, SharedOptions shared, PlannedItem item, FormatSpec format,
		IDocument document, ConvertOptions options, Writer combinedText) throws IOException
	{
		TextConversion textConversion = new TextConversion();
		if (combinedText != null)
		{
			// --combine: every document appends to the one writer execute owns, so it
			// is not closed here.
			textConversion.convert(document, format, options, combinedText);
		}
		else if (shared.output.useStdout)
		{
			Writer writer = documentWriter(context.console.openStandardOutput());
			textConversion.convert(document, format, options, writer);
			writer.flush();
		}
		else
		{
			// Looked at again here, not only when the plan was made: asking immediately
			// before the write puts the question to the filesystem, which is the
			// authority on whether two names are one file.
			OutputPlanner.verifyPageTarget(item.target, shared, context.fileSystem);

			try (Writer destination = documentWriter(context.fileSystem.createFile(item.target)))
			{
				textConversion.convert(document, format, options, destination);
			}
		}
	}

	/**
	 * {@code --stdout} for {@code docpack}. {@code item.target} is null when
	 * streaming, so the pack is rendered into memory first: a docpack is a binary
	 * container whose finished bytes can only be known once {@code makeDocPack} is
	 * done, and stdout is forward-only.
	 */
	private static void convertToDocPack(CommandContext context, SharedOptions shared, PlannedItem item, IDocument document,
		ConvertOptions options) throws IOException
	{
		String canvasOptions = options.canvasOptions != null ? options.canvasOptions : "";
		if (!shared.output.useStdout)
		{
			document.makeDocPack(item.target, canvasOptions);
			return;
		}

		try (MemoryChannel buffer = new MemoryChannel())
		{
			document.makeDocPack(buffer, canvasOptions);

			OutputStream destination = context.console.openStandardOutput();
			buffer.writeTo(destination);
			destination.flush();
		}
	}

	/**
	 * A writer for document text, standing in for the reference's
	 * {@code new StreamWriter(stream)}: UTF-8 with no byte order mark, and no
	 * newline translation.
	 *
	 * <p>Strict, as that writer's encoding is: a character UTF-8 cannot carry fails
	 * the document rather than being silently replaced, because this writes the
	 * document itself and it must not be altered. The console replaces instead,
	 * because it writes reports.
	 */
	private static Writer documentWriter(OutputStream destination)
	{
		return new BufferedWriter(new OutputStreamWriter(destination, StandardCharsets.UTF_8.newEncoder()));
	}

	@Override
	public String describePlan(CommandContext context)
	{
		SharedOptions shared = context.sharedOptions;
		OptionsValidator.validateConvert(shared, options);

		InputEnumerator enumerator = new InputEnumerator(context.fileSystem, context.console);
		List<InputSource> inputs = enumerator.enumerate(shared.input);

		RunPlan plan = OutputPlanner.plan(shared, options, inputs, context.fileSystem);
		if (!options.fingerprint)
			OutputPlanner.verifyTargets(plan, shared, context.fileSystem);

		return String.join("\n", Arrays.asList(
			"command convert",
			"to " + options.format,
			PlanFormat.inputs(inputs),
			shared.toCanonicalString(),
			"pages " + (options.pages != null ? options.pages : PageRange.ALL),
			plan.toCanonicalString()));
	}
}
