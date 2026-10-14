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
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;

using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Commands
{
	/// <summary><c>list</c> — list a document's subfile contents without extracting.</summary>
	public sealed class ListCommand : ICommand
	{
		public ListCommand(ListOptions options)
		{
			Options = options;
		}

		/// <summary>The bound per-command options. Readable so the binding itself can be tested.</summary>
		public ListOptions Options { get; }

		public string Name => "list";

		public bool RequiresEngine => true;

		public int Execute(CommandContext context)
		{
			return CommandExecution.WithSession(context, () =>
			{
                var shared = context.SharedOptions;
				OptionsValidator.Validate(shared);

				var enumerator = new InputEnumerator(context.FileSystem, context.Console);
                var inputs = enumerator.Enumerate(shared.Input);

				var passwords = new PasswordProvider(shared.Passwords, context.Console, context.FileSystem);
				var diagnostics = new Diagnostics(shared.Diagnostics, context.Console, context.Session.Api, context.Clock);
				var log = EngineLog.For(shared.Diagnostics, context.Console);
				var batch = new BatchRunner(shared.Batch, context.Console, context.Interruption);
				var subfiles = new SubfileExtraction(context.FileSystem);
				JsonWriter json = Options.Json ? new JsonWriter().BeginArray() : null;

				diagnostics.BeginRun();

				int processed = 0;
				int exitCode = batch.Run(inputs, input =>
				{
					processed++;
					diagnostics.BeginDocument(input.Path, processed, inputs.Count);
					passwords.Reset();

					// Declared out here so it outlives the document that reads from it.
					using Stream debugInput = DebugInput.For(input.Path, shared.Diagnostics, context.FileSystem,
															 context.Console, input.Path);

					IDocument container;
					StopSignal stop = CommandExecution.WatchForStop(diagnostics, context.Interruption);
					try
					{
						ExtractorOpenSettings open = ExtractorOpenSettings.ForInspection(shared);
						container = debugInput == null
							? context.Session.OpenDocument(input.Path, open, passwords,
														   stop.ShouldStop, log)
							: context.Session.OpenDocument(debugInput, input.Path, open, passwords,
														   stop.ShouldStop, log);
					}
					catch (EngineException error) when (error.Error == EngineError.Cancelled)
					{
						throw stop.Cause(input.Path);
					}
					catch (Exception error) when (!(error is CliError))
					{
						throw new DocumentError(input.Path, "could not open document", error);
					}

					using (container)
					{
						IReadOnlyList<SubfileEntry> entries;
						try
						{
							entries = subfiles.ListAll(container, input.Path, shared);
						}
						catch (Exception error) when (!(error is CliError))
						{
							throw new DocumentError(input.Path, error.Message, error);
						}

						if (Options.Json)
						{
							json.BeginObject().Property("path", input.Path).Name("subfiles").BeginArray();
							foreach (SubfileEntry entry in entries)
							{
								json.BeginObject()
									.Property("id", entry.Id)
									.Property("name", entry.Name)
									.Property("size", entry.Size)
									.Property("modified", entry.Modified)
									.Property("depth", entry.Depth)
									.EndObject();
							}
							json.EndArray().EndObject();
						}
						else
						{
							foreach (SubfileEntry entry in entries)
							{
								context.Console.WriteLine(
									$"{entry.DisplayName}: id={entry.Id} size={entry.Size} " +
									$"modified={entry.Modified} depth={entry.Depth} name={entry.Name}");
							}
						}
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


		public string DescribePlan(CommandContext context)
		{
            var shared = context.SharedOptions;
			OptionsValidator.Validate(shared);

			var enumerator = new InputEnumerator(context.FileSystem, context.Console);
            var inputs = enumerator.Enumerate(shared.Input);

			OutputCardinality cardinality = OutputPlanner.ResolveCardinality(shared, inputs.Count, combine: false);

			var lines = new List<string>
			{
				"command list",
				PlanFormat.Inputs(inputs),
				shared.ToCanonicalString(),
				"json " + PlanFormat.YesNo(Options.Json),
				"cardinality " + RunPlan.CardinalityText(cardinality)
			};
			return string.Join("\n", lines);
		}
	}
}
