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
	/// <c>convert</c> — render documents to a target format, and optionally report
	/// document and page structure while doing so.
	/// </summary>
	public sealed class ConvertCommand : ICommand
	{
		public ConvertCommand(ConvertOptions options)
		{
			Options = options;
		}

		/// <summary>The bound per-command options. Readable so the binding itself can be tested.</summary>
		public ConvertOptions Options { get; }

		public string Name => "convert";

		public bool RequiresEngine => true;

		public int Execute(CommandContext context)
		{
			return CommandExecution.WithSession(context, () =>
			{
				var shared = context.SharedOptions;
				OptionsValidator.ValidateConvert(shared, Options);

				var enumerator = new InputEnumerator(context.FileSystem, context.Console);
				var inputs = enumerator.Enumerate(shared.Input);

				var plan = OutputPlanner.Plan(shared, Options, inputs, context.FileSystem);
				// --fingerprint never writes to plan.Items' resolved targets — see
				// CanvasConversion.MakeCanvas — so a real file already sitting at one
				// of those paths is not a conflict to report.
				if (!Options.Fingerprint)
					OutputPlanner.VerifyTargets(plan, shared, context.FileSystem);

				var diagnostics = new Diagnostics(shared.Diagnostics, context.Console, context.Session.Api, context.Clock);
				var log = EngineLog.For(shared.Diagnostics, context.Console);
				diagnostics.BeginRun();

				// --combine is allowed for text formats too, which have no canvas: their
				// documents concatenate into one writer instead. Keyed off the format's
				// kind rather than the cardinality alone, because reading
				// CanvasType.Value for a text format threw before anything was caught
				// and took the process down with an unhandled exception.
				var combinedFormat = Options.Format;
				bool combining = plan.Cardinality == OutputCardinality.Combined && !shared.Output.UseStdout;
				if (combining)
					EnsureOutputDirectory(context, Path.GetDirectoryName(plan.CombinedTarget));

				// A block rather than a `using` declaration, because the combined output has
				// to be closed before --view below: it is not a finished file until it is, a
				// PDF's trailer being written on close. Declared, both of these lived to the
				// end of the method, so --combine --view handed the viewer a half-written
				// document while the comment under it said the opposite. The scope now says
				// when they close, which is not something a later edit can quietly move.
				int exitCode;
				using (IOutputCanvas combinedCanvas = combining && combinedFormat.Kind == FormatKind.Canvas
						? context.Session.Api.MakeCanvas(plan.CombinedTarget, combinedFormat.CanvasType.Value,
															   CanvasConversion.ComposeCanvasOptions(combinedFormat, Options.CanvasOptions, Options.NoImages))
						: null)
				using (TextWriter combinedText = combining && combinedFormat.Kind == FormatKind.Text
						? new StreamWriter(context.FileSystem.CreateFile(plan.CombinedTarget))
						: null)
				{
					var batch = new BatchRunner(shared.Batch, context.Console, context.Interruption);
					var canvasConversion = new CanvasConversion(context.Session, context.Console, shared, context.FileSystem);
					int processed = 0;
					exitCode = batch.Run(plan.Items, item =>
					{
						processed++;
						diagnostics.BeginDocument(item.DisplayName, processed, plan.Items.Count);

						ProcessDocument(context, shared, item, combinedCanvas, combinedText, canvasConversion, diagnostics, log);

						diagnostics.EndDocument(item.DisplayName);
						diagnostics.CheckDocumentTimeout(item.DisplayName);
						diagnostics.CheckLimits();
						diagnostics.PauseIfRequested();
					});

					diagnostics.EndRun(batch.SuccessCount, batch.FailureCount);
				}

				// --view opens what was produced, once the run is over and the file is
				// closed. Only a run with one output has something unambiguous to open,
				// which is what the spec means by "single output".
				if (Options.View && exitCode == ExitCode.Success)
					(context.OpenViewer ?? Viewer.Open)(SingleOutputOf(plan), context.Console);

				return exitCode;
			});
		}

		/// <summary>
		/// Resolves the format, opens the document once with settings matching that
		/// format, converts, and closes it. Failures become a <see cref="DocumentError"/>
		/// for <see cref="BatchRunner"/> to count rather than ending the run.
		/// </summary>
		/// <remarks>
		/// The format is settled before the document is opened, which it has to be:
		/// <see cref="ExtractorOpenSettings"/> depends on it, and the engine applies
		/// those settings during <c>Open</c>. It comes from <c>--to</c> and nothing
		/// else, so there is nothing to discover from the document and no reason to
		/// open one twice.
		/// </remarks>
		private void ProcessDocument(CommandContext context, SharedOptions shared, PlannedItem item, IOutputCanvas combinedCanvas,
									 TextWriter combinedText, CanvasConversion canvasConversion, Diagnostics diagnostics,
									 EngineLog log)
        {
			var format = Options.Format;
            var passwords = new PasswordProvider(shared.Passwords, context.Console, context.FileSystem);
			var openSettings = ExtractorOpenSettings.ForFormat(format, Options, shared);

			// Once per item, before anything opens an output. --fingerprint is skipped
			// because its target is a plan's guess that is never written to.
			if (!Options.Fingerprint)
				EnsureOutputDirectory(context, item.OutputDirectory);

			// A debug switch that changes how the bytes arrive hands the engine a stream
			// instead of the path; otherwise the engine opens the file itself. Declared
			// out here, not inside the try, so it outlives the document that reads from
			// it — disposing it at the end of the open would close the file mid-read.
			using Stream debugInput = DebugInput.For(item.SourcePath, shared.Diagnostics, context.FileSystem,
													 context.Console, item.DisplayName);

			IDocument document;
			StopSignal stop = CommandExecution.WatchForStop(diagnostics, context.Interruption);
			try
			{
				document = debugInput == null
					? context.Session.OpenDocument(item.SourcePath, openSettings, passwords,
												   stop.ShouldStop, log)
					: context.Session.OpenDocument(debugInput, item.DisplayName, openSettings, passwords,
												   stop.ShouldStop, log);
			}
			catch (EngineException error) when (error.Error == EngineError.Cancelled)
			{
				// We asked for this, so it is the limit or the interrupt being reported, not a bad document.
				throw stop.Cause(item.DisplayName);
			}
			catch (Exception error) when (!(error is CliError))
			{
				throw new DocumentError(item.SourcePath, "could not open document", error);
			}

			using (document)
			{
				try
				{
					var structure = new StructureReport(Options.Reports, context.Console);
					structure.ReportDocument(document, item.DisplayName);

					if (Options.Reports.Any)
					{
						for (int pageIndex = 0; pageIndex < document.GetPageCount(); pageIndex++)
						{
							using IDocumentPage page = document.GetPage(pageIndex);
							structure.ReportPage(page, pageIndex + 1, item.DisplayName);
						}
					}

					switch (format.Kind)
					{
						case FormatKind.Text:
							ConvertToText(context, shared, item, format, document, Options, combinedText);
							break;

						case FormatKind.DocPack:
							ConvertToDocPack(context, shared, item, document, Options);
							break;

						default:
							if (combinedCanvas != null)
								canvasConversion.AppendTo(combinedCanvas, document, Options.Pages ?? PageRange.All, Options, item.DisplayName);
							else if (shared.Output.UseStdout)
								canvasConversion.ConvertToStream(document, context.Console.OpenStandardOutput(), item, format, Options);
							else
								canvasConversion.Convert(document, item, format, Options);
							break;
					}
				}
				catch (Exception error) when (!(error is CliError))
				{
					throw new DocumentError(item.SourcePath, error.Message, error);
				}
			}
		}

		/// <summary>
		/// The one file a run produced, or null when it produced none or several.
		/// </summary>
		/// <remarks>
		/// A per-page format resolves its names during execution, so even a single
		/// input can leave several files and there is no one thing to open.
		/// </remarks>
		public static string SingleOutputOf(RunPlan plan)
		{
			if (plan.Cardinality == OutputCardinality.Combined)
				return plan.CombinedTarget;

			return plan.Items.Count == 1 ? plan.Items[0].Target : null;
		}


		/// <summary>
		/// Creates the directory an output is about to be written into, if it is not
		/// already there.
		/// </summary>
		/// <remarks>
		/// Here, in the orchestrator, rather than at each of the three places that open
		/// an output, because only one of those three is ours to change: the text path
		/// writes through <see cref="IFileSystem"/>, but a canvas and a docpack are
		/// opened by the engine, which will not create a directory and reports a
		/// missing one as <c>FileStream::Create</c> failing — a per-document error that
		/// reads as though the input were missing. Doing it per format meant
		/// <c>--out-dir out/</c> worked for <c>--to text</c> and failed for everything
		/// else.
		/// </remarks>
		private static void EnsureOutputDirectory(CommandContext context, string directory)
		{
			if (!string.IsNullOrEmpty(directory))
				context.FileSystem.CreateDirectory(directory);
		}

		private static void ConvertToText(CommandContext context, SharedOptions shared, PlannedItem item, FormatSpec format,
										  IDocument document, ConvertOptions options, TextWriter combinedText)
		{
			var textConversion = new TextConversion();
			if (combinedText != null)
			{
				// --combine: every document appends to the one writer Execute owns, so
				// it is not disposed here.
				textConversion.Convert(document, format, options, combinedText);
			}
			else if (shared.Output.UseStdout)
			{
				var writer = new StreamWriter(context.Console.OpenStandardOutput()) { AutoFlush = true };
				textConversion.Convert(document, format, options, writer);
			}
			else
			{
				// Looked at again here, not only when the plan was made. The planner checks
				// every target before the run starts, against a directory none of this run's
				// outputs are in yet, so two inputs whose targets the filesystem treats as
				// one name --- which the collision check's ASCII fold cannot see for a pair
				// differing outside ASCII --- both passed, and the second write replaced the
				// first with no --overwrite and exit 0. Measured on NTFS with Å.txt and
				// å.txt. Asking immediately before the write puts the question to the
				// filesystem, which is the authority on whether two names are one file.
				OutputPlanner.VerifyPageTarget(item.Target, shared, context.FileSystem);

				using var destination = new StreamWriter(context.FileSystem.CreateFile(item.Target));
				textConversion.Convert(document, format, options, destination);
			}
		}

		/// <summary>
		/// <c>--stdout</c> for <c>docpack</c>. <c>item.Target</c> is null under
		/// <see cref="OutputCardinality.Stream"/>, the same gap
		/// <see cref="CanvasConversion.ConvertToStream"/> closed for canvas formats.
		/// </summary>
		/// <remarks>
		/// Renders into a <see cref="MemoryStream"/> first rather than handing
		/// <c>--stdout</c>'s own stream to <c>MakeDocPack</c> directly, for the same
		/// reason as the canvas path: a docpack is a binary container whose finished
		/// bytes can only be known once <c>MakeDocPack</c> is done, and
		/// <see cref="IConsole.OpenStandardOutput"/> is forward-only. No sample or
		/// doc2text streams a docpack to confirm whether it truly needs to seek, so
		/// this buffers defensively rather than risk the same "renders fine, nothing
		/// reaches the console" failure found for canvases in D37.
		/// </remarks>
		private static void ConvertToDocPack(CommandContext context, SharedOptions shared, PlannedItem item, IDocument document, ConvertOptions options)
		{
			string canvasOptions = options.CanvasOptions ?? string.Empty;
			if (!shared.Output.UseStdout)
			{
				document.MakeDocPack(item.Target, canvasOptions);
				return;
			}

			using var buffer = new MemoryStream();
			document.MakeDocPack(buffer, canvasOptions);

			buffer.Position = 0;
			Stream destination = context.Console.OpenStandardOutput();
			buffer.CopyTo(destination);
			destination.Flush();
		}

		public string DescribePlan(CommandContext context)
		{
			var shared = context.SharedOptions;
			OptionsValidator.ValidateConvert(shared, Options);

			var enumerator = new InputEnumerator(context.FileSystem, context.Console);
			var inputs = enumerator.Enumerate(shared.Input);

            var plan = OutputPlanner.Plan(shared, Options, inputs, context.FileSystem);
			if (!Options.Fingerprint)
				OutputPlanner.VerifyTargets(plan, shared, context.FileSystem);

			var lines = new List<string>
			{
				"command convert",
				"to " + Options.Format,
				PlanFormat.Inputs(inputs),
				shared.ToCanonicalString(),
				"pages " + (Options.Pages ?? PageRange.All),
				plan.ToCanonicalString()
			};
			return string.Join("\n", lines);
		}
	}
}
