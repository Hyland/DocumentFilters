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

import com.hyland.documentfilters.cli.configuration.ExtractOptions;
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
import com.hyland.documentfilters.cli.execution.PasswordProvider;
import com.hyland.documentfilters.cli.execution.StopSignal;
import com.hyland.documentfilters.cli.execution.SubfileExtraction;
import com.hyland.documentfilters.cli.planning.OutputCardinality;
import com.hyland.documentfilters.cli.planning.OutputPlanner;
import com.hyland.documentfilters.cli.planning.RunPlan;
import com.hyland.documentfilters.cli.runtime.BatchRunner;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.DocumentError;
import java.util.Arrays;
import java.util.List;

/**
 * {@code extract} --- pull embedded subfiles, attachments and images out of
 * container documents.
 */
public final class ExtractCommand implements ICommand
{
	/** The bound per-command options. Readable so the binding itself can be tested. */
	public final ExtractOptions options;

	public ExtractCommand(ExtractOptions options)
	{
		this.options = options;
	}

	@Override
	public String getName()
	{
		return "extract";
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
			OptionsValidator.validateExtract(shared, options);

			InputEnumerator enumerator = new InputEnumerator(context.fileSystem, context.console);
			List<InputSource> inputs = enumerator.enumerate(shared.input);

			PasswordProvider passwords = new PasswordProvider(shared.passwords, context.console, context.fileSystem);
			Diagnostics diagnostics = new Diagnostics(shared.diagnostics, context.console, context.session.getApi(), context.clock);
			EngineLog log = EngineLog.forOptions(shared.diagnostics, context.console);
			BatchRunner batch = new BatchRunner(shared.batch, context.console, context.interruption);
			SubfileExtraction subfiles = new SubfileExtraction(context.fileSystem);

			diagnostics.beginRun();

			int[] processed = { 0 };
			int exitCode = batch.run(inputs, input ->
			{
				processed[0]++;
				diagnostics.beginDocument(input.path, processed[0], inputs.size());
				passwords.reset();

				IDocument opened;
				StopSignal stop = CommandExecution.watchForStop(diagnostics, context.interruption);
				try
				{
					opened = context.session.openDocument(input.path,
						ExtractorOpenSettings.forInspection(shared, options.images), passwords, stop::shouldStop, log);
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

				try (IDocument container = opened)
				{
					try
					{
						subfiles.extractAll(container, input.path, shared, options);
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
			});

			diagnostics.endRun(batch.getSuccessCount(), batch.getFailureCount());
			return exitCode;
		});
	}

	@Override
	public String describePlan(CommandContext context)
	{
		SharedOptions shared = context.sharedOptions;
		OptionsValidator.validateExtract(shared, options);

		InputEnumerator enumerator = new InputEnumerator(context.fileSystem, context.console);
		List<InputSource> inputs = enumerator.enumerate(shared.input);

		OutputCardinality cardinality = OutputPlanner.resolveCardinality(shared, inputs.size(), false);

		return String.join("\n", Arrays.asList(
			"command extract",
			PlanFormat.inputs(inputs),
			shared.toCanonicalString(),
			"include " + PlanFormat.noneOr(options.includePatterns),
			"exclude " + PlanFormat.noneOr(options.excludePatterns),
			"images " + PlanFormat.yesNo(options.images),
			"make-dirs " + PlanFormat.yesNo(options.replicateTree()),
			"sub-id " + PlanFormat.yesNo(options.appendSubfileId),
			"cardinality " + RunPlan.cardinalityText(cardinality)));
	}
}
