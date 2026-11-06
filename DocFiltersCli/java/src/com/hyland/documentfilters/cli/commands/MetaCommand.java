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

import com.hyland.documentfilters.cli.configuration.MetaOptions;
import com.hyland.documentfilters.cli.configuration.OptionsValidator;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.discovery.InputEnumerator;
import com.hyland.documentfilters.cli.discovery.InputSource;
import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.engine.EngineException;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.execution.Diagnostics;
import com.hyland.documentfilters.cli.execution.EngineLog;
import com.hyland.documentfilters.cli.execution.ExtractorOpenSettings;
import com.hyland.documentfilters.cli.execution.MetadataReport;
import com.hyland.documentfilters.cli.execution.PasswordProvider;
import com.hyland.documentfilters.cli.execution.StopSignal;
import com.hyland.documentfilters.cli.planning.OutputCardinality;
import com.hyland.documentfilters.cli.planning.OutputPlanner;
import com.hyland.documentfilters.cli.planning.RunPlan;
import com.hyland.documentfilters.cli.runtime.BatchRunner;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.DocumentError;
import com.hyland.documentfilters.cli.runtime.JsonWriter;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.channels.SeekableByteChannel;
import java.util.Arrays;
import java.util.List;

/** {@code meta} --- report each document's metadata fields. */
public final class MetaCommand implements ICommand
{
	/** The bound per-command options. Readable so the binding itself can be tested. */
	public final MetaOptions options;

	public MetaCommand(MetaOptions options)
	{
		this.options = options;
	}

	@Override
	public String getName()
	{
		return "meta";
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
			OptionsValidator.validate(shared);

			InputEnumerator enumerator = new InputEnumerator(context.fileSystem, context.console);
			List<InputSource> inputs = enumerator.enumerate(shared.input);

			PasswordProvider passwords = new PasswordProvider(shared.passwords, context.console, context.fileSystem);
			Diagnostics diagnostics = new Diagnostics(shared.diagnostics, context.console, context.session.getApi(), context.clock);
			EngineLog log = EngineLog.forOptions(shared.diagnostics, context.console);
			BatchRunner batch = new BatchRunner(shared.batch, context.console, context.interruption);
			MetadataReport report = new MetadataReport();
			JsonWriter json = options.json ? new JsonWriter().beginArray() : null;

			diagnostics.beginRun();

			int[] processed = { 0 };
			int exitCode = batch.run(inputs, input ->
			{
				processed[0]++;
				diagnostics.beginDocument(input.path, processed[0], inputs.size());
				passwords.reset();

				// Opened out here so it outlives the document that reads from it.
				try (SeekableByteChannel debugInput = CommandExecution.debugInput(input.path, shared.diagnostics,
					context.fileSystem, context.console, input.path))
				{
					IDocument opened;
					StopSignal stop = CommandExecution.watchForStop(diagnostics, context.interruption);
					try
					{
						ExtractorOpenSettings open = ExtractorOpenSettings.forMetadataOnly(shared);
						opened = debugInput == null
							? context.session.openDocument(input.path, open, passwords, stop::shouldStop, log)
							: context.session.openDocument(debugInput, input.path, open, passwords, stop::shouldStop, log);
					}
					catch (EngineException error)
					{
						if (error.getError() == EngineError.CANCELLED)
							throw stop.cause(input.path);
						throw new DocumentError(input.path, "could not open document", error);
					}
					catch (CliError error)
					{
						throw error;
					}
					catch (RuntimeException error)
					{
						throw new DocumentError(input.path, "could not open document", error);
					}

					try (IDocument document = opened)
					{
						try
						{
							if (options.json)
								report.writeJson(document, input.path, json);
							else
								report.writeText(document, input.path, context.console);
						}
						catch (CliError error)
						{
							throw error;
						}
						catch (RuntimeException error)
						{
							throw new DocumentError(input.path, CommandExecution.messageOf(error), error);
						}
					}
					diagnostics.endDocument(input.path);
					diagnostics.checkDocumentTimeout(input.path);
					diagnostics.checkLimits();
					diagnostics.pauseIfRequested();
				}
				catch (IOException e)
				{
					throw new UncheckedIOException(e.getMessage(), e);
				}
			});

			if (options.json)
				context.console.write(json.endArray().toString());

			diagnostics.endRun(batch.getSuccessCount(), batch.getFailureCount());
			return exitCode;
		});
	}

	@Override
	public String describePlan(CommandContext context)
	{
		SharedOptions shared = context.sharedOptions;
		OptionsValidator.validate(shared);

		InputEnumerator enumerator = new InputEnumerator(context.fileSystem, context.console);
		List<InputSource> inputs = enumerator.enumerate(shared.input);

		OutputCardinality cardinality = OutputPlanner.resolveCardinality(shared, inputs.size(), false);

		return String.join("\n", Arrays.asList(
			"command meta",
			PlanFormat.inputs(inputs),
			shared.toCanonicalString(false),
			"json " + PlanFormat.yesNo(options.json),
			"cardinality " + RunPlan.cardinalityText(cardinality)));
	}
}
