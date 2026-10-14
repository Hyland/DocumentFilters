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
	/// <summary>
	/// <c>extract</c> — pull embedded subfiles, attachments and images out of
	/// container documents.
	/// </summary>
	public sealed class ExtractCommand : ICommand
	{
		public ExtractCommand(ExtractOptions options)
		{
			Options = options;
		}

		/// <summary>The bound per-command options. Readable so the binding itself can be tested.</summary>
		public ExtractOptions Options { get; }

		public string Name => "extract";

		public bool RequiresEngine => true;

		public int Execute(CommandContext context)
		{
			return CommandExecution.WithSession(context, () =>
			{
                var shared = context.SharedOptions;
				OptionsValidator.ValidateExtract(shared, Options);

				var enumerator = new InputEnumerator(context.FileSystem, context.Console);
                var inputs = enumerator.Enumerate(shared.Input);

				var passwords = new PasswordProvider(shared.Passwords, context.Console, context.FileSystem);
				var diagnostics = new Diagnostics(shared.Diagnostics, context.Console, context.Session.Api, context.Clock);
				var log = EngineLog.For(shared.Diagnostics, context.Console);
				var batch = new BatchRunner(shared.Batch, context.Console, context.Interruption);
				var subfiles = new SubfileExtraction(context.FileSystem);

				diagnostics.BeginRun();

				int processed = 0;
				int exitCode = batch.Run(inputs, input =>
				{
					processed++;
					diagnostics.BeginDocument(input.Path, processed, inputs.Count);
					passwords.Reset();

					IDocument container;
					StopSignal stop = CommandExecution.WatchForStop(diagnostics, context.Interruption);
					try
					{
						container = context.Session.OpenDocument(input.Path,
							ExtractorOpenSettings.ForInspection(shared, Options.Images), passwords,
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
						try
						{
							subfiles.ExtractAll(container, input.Path, shared, Options);
						}
						catch (Exception error) when (!(error is CliError))
						{
							throw new DocumentError(input.Path, error.Message, error);
						}
					}

					diagnostics.EndDocument(input.Path);
					diagnostics.CheckDocumentTimeout(input.Path);
					diagnostics.CheckLimits();
					diagnostics.PauseIfRequested();
				});

				diagnostics.EndRun(batch.SuccessCount, batch.FailureCount);
				return exitCode;
			});
		}

		public string DescribePlan(CommandContext context)
		{
            var shared = context.SharedOptions;
			OptionsValidator.ValidateExtract(shared, Options);

			var enumerator = new InputEnumerator(context.FileSystem, context.Console);
            var inputs = enumerator.Enumerate(shared.Input);

			OutputCardinality cardinality = OutputPlanner.ResolveCardinality(shared, inputs.Count, combine: false);

			var lines = new List<string>
			{
				"command extract",
				PlanFormat.Inputs(inputs),
				shared.ToCanonicalString(),
				"include " + PlanFormat.NoneOr(Options.IncludePatterns),
				"exclude " + PlanFormat.NoneOr(Options.ExcludePatterns),
				"images " + PlanFormat.YesNo(Options.Images),
				"make-dirs " + PlanFormat.YesNo(Options.ReplicateTree),
				"sub-id " + PlanFormat.YesNo(Options.AppendSubfileId),
				"cardinality " + RunPlan.CardinalityText(cardinality)
			};
			return string.Join("\n", lines);
		}
	}
}
