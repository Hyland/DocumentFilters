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

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Discovery;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Runtime;

using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Commands
{
	/// <summary><c>identify</c> — detect and print each input's format. No conversion.</summary>
	public sealed class IdentifyCommand : ICommand
	{
		public IdentifyCommand(IdentifyOptions options)
		{
			Options = options;
		}

		/// <summary>The bound per-command options. Readable so the binding itself can be tested.</summary>
		public IdentifyOptions Options { get; }

		public string Name => "identify";

		public bool RequiresEngine => true;

		public int Execute(CommandContext context)
		{
			return CommandExecution.WithSession(context, () =>
			{
                var shared = context.SharedOptions;
				OptionsValidator.Validate(shared);

				var enumerator = new InputEnumerator(context.FileSystem, context.Console);
                var inputs = enumerator.Enumerate(shared.Input);

				var diagnostics = new Diagnostics(shared.Diagnostics, context.Console, context.Session.Api, context.Clock);
				var batch = new BatchRunner(shared.Batch, context.Console, context.Interruption);
				JsonWriter json = Options.Json ? new JsonWriter().BeginArray() : null;

				diagnostics.BeginRun();

				int processed = 0;
				int exitCode = batch.Run(inputs, input =>
				{
					processed++;
					diagnostics.BeginDocument(input.Path, processed, inputs.Count);

					// identify never opens the document, so there is no open callback and
					// nothing to cancel or log — but what the engine makes of a truncated
					// or buffered file is exactly what the debug switches ask about.
					using Stream debugInput = DebugInput.For(input.Path, shared.Diagnostics, context.FileSystem,
															 context.Console, input.Path);

					EngineFileType identified;
					try
					{
						identified = debugInput == null
							? context.Session.Api.IdentifyFile(input.Path)
							: context.Session.Api.IdentifyFile(debugInput, input.Path);
					}
					catch (Exception error) when (!(error is CliError))
					{
						throw new DocumentError(input.Path, "could not identify document", error);
					}

					string capabilities = Options.Capabilities ? CapabilitiesText(identified.Capabilities) : null;

					if (Options.Json)
					{
						json.BeginObject().Property("path", input.Path).Property("format", identified.Id);
						if (capabilities != null)
							json.Property("capabilities", capabilities);
						json.Property("mime", identified.MimeType).Property("name", identified.Name).EndObject();
					}
					else
					{
						string capabilitiesField = capabilities != null ? $"caps={capabilities} " : string.Empty;
						context.Console.WriteLine(
							$"{input.Path}: format={identified.Id} {capabilitiesField}mime={identified.MimeType} name={identified.Name}");
					}
					diagnostics.EndDocument(input.Path);
					diagnostics.CheckDocumentTimeout(input.Path);
					diagnostics.CheckLimits();
					diagnostics.PauseIfRequested();
				});

				if (Options.Json)
					context.Console.Write(json.EndArray().ToString());

				diagnostics.EndRun(batch.SuccessCount, batch.FailureCount);
				return exitCode;
			});
		}

		/// <remarks>
		/// The three bits doc2text itself reports (<c>main.cpp</c>'s
		/// <c>IGR_Get_Stream_Type</c> handling): <c>IGR_FILE_SUPPORTS_TEXT</c>,
		/// <c>IGR_FILE_SUPPORTS_SUBFILES</c>, <c>IGR_FILE_SUPPORTS_HDHTML</c>.
		/// </remarks>
		private static string CapabilitiesText(int capabilities)
		{
			var caps = new List<string>();
			if ((capabilities & ISYS11dfConstants.IGR_FILE_SUPPORTS_TEXT) != 0)
				caps.Add("text");
			if ((capabilities & ISYS11dfConstants.IGR_FILE_SUPPORTS_SUBFILES) != 0)
				caps.Add("subfiles");
			if ((capabilities & ISYS11dfConstants.IGR_FILE_SUPPORTS_HDHTML) != 0)
				caps.Add("hd");

			return caps.Count == 0 ? "none" : string.Join(",", caps);
		}

		public string DescribePlan(CommandContext context)
		{
            var shared = context.SharedOptions;
			OptionsValidator.Validate(shared);

			var enumerator = new InputEnumerator(context.FileSystem, context.Console);
			var inputs = enumerator.Enumerate(shared.Input);

			var lines = new List<string>
			{
				"command identify",
				PlanFormat.Inputs(inputs),
				shared.ToCanonicalString(false),
				"json " + PlanFormat.YesNo(Options.Json),
				"capabilities " + PlanFormat.YesNo(Options.Capabilities)
			};
			return string.Join("\n", lines);
		}
	}
}
