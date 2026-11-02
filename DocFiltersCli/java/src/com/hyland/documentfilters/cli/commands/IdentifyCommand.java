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

import com.hyland.documentfilters.cli.configuration.IdentifyOptions;
import com.hyland.documentfilters.cli.configuration.OptionsValidator;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.discovery.InputEnumerator;
import com.hyland.documentfilters.cli.discovery.InputSource;
import com.hyland.documentfilters.cli.engine.EngineFileType;
import com.hyland.documentfilters.cli.execution.Diagnostics;
import com.hyland.documentfilters.cli.runtime.BatchRunner;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.DocumentError;
import com.hyland.documentfilters.cli.runtime.JsonWriter;
import com.perceptive.documentfilters.isys_docfiltersConstants;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.channels.SeekableByteChannel;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * {@code identify} --- report each input's format without opening it for
 * extraction.
 */
public final class IdentifyCommand implements ICommand
{
	/** The bound per-command options. Readable so the binding itself can be tested. */
	public final IdentifyOptions options;

	public IdentifyCommand(IdentifyOptions options)
	{
		this.options = options;
	}

	@Override
	public String getName()
	{
		return "identify";
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

			Diagnostics diagnostics = new Diagnostics(shared.diagnostics, context.console, context.session.getApi(), context.clock);
			BatchRunner batch = new BatchRunner(shared.batch, context.console, context.interruption);
			JsonWriter json = options.json ? new JsonWriter().beginArray() : null;

			diagnostics.beginRun();

			int[] processed = { 0 };
			int exitCode = batch.run(inputs, input ->
			{
				processed[0]++;
				diagnostics.beginDocument(input.path, processed[0], inputs.size());

				// identify never opens the document, so there is no open callback and
				// nothing to cancel or log --- but what the engine makes of a truncated
				// or buffered file is exactly what the debug switches ask about.
				try (SeekableByteChannel debugInput = CommandExecution.debugInput(input.path, shared.diagnostics,
					context.fileSystem, context.console, input.path))
				{
					EngineFileType identified;
					try
					{
						identified = debugInput == null
							? context.session.getApi().identifyFile(input.path)
							: context.session.getApi().identifyFile(debugInput, input.path);
					}
					catch (CliError error)
					{
						throw error;
					}
					catch (RuntimeException error)
					{
						throw new DocumentError(input.path, "could not identify document", error);
					}

					String capabilities = options.capabilities ? capabilitiesText(identified.capabilities) : null;

					if (options.json)
					{
						json.beginObject().property("path", input.path).property("format", identified.id);
						if (capabilities != null)
							json.property("capabilities", capabilities);
						json.property("mime", identified.mimeType).property("name", identified.name).endObject();
					}
					else
					{
						String capabilitiesField = capabilities != null ? "caps=" + capabilities + " " : "";
						context.console.writeLine(input.path + ": format=" + identified.id + " " + capabilitiesField
							+ "mime=" + text(identified.mimeType) + " name=" + text(identified.name));
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

	private static String capabilitiesText(int capabilities)
	{
		List<String> caps = new ArrayList<>();
		if ((capabilities & isys_docfiltersConstants.IGR_FILE_SUPPORTS_TEXT) != 0)
			caps.add("text");
		if ((capabilities & isys_docfiltersConstants.IGR_FILE_SUPPORTS_SUBFILES) != 0)
			caps.add("subfiles");
		if ((capabilities & isys_docfiltersConstants.IGR_FILE_SUPPORTS_HDHTML) != 0)
			caps.add("hd");

		return caps.isEmpty() ? "none" : String.join(",", caps);
	}

	@Override
	public String describePlan(CommandContext context)
	{
		SharedOptions shared = context.sharedOptions;
		OptionsValidator.validate(shared);

		InputEnumerator enumerator = new InputEnumerator(context.fileSystem, context.console);
		List<InputSource> inputs = enumerator.enumerate(shared.input);

		return String.join("\n", Arrays.asList(
			"command identify",
			PlanFormat.inputs(inputs),
			shared.toCanonicalString(false),
			"json " + PlanFormat.yesNo(options.json),
			"capabilities " + PlanFormat.yesNo(options.capabilities)));
	}

	private static String text(String value)
	{
		return value != null ? value : "";
	}
}
