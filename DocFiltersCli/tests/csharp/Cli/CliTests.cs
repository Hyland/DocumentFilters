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

using System.Linq;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Cli
{
	/// <summary>
	/// Whole command lines, argv to exit code.
	/// </summary>
	/// <remarks>
	/// These are about the wiring, not the rules. Every rule here is already pinned by
	/// the conformance corpus or a unit test; what is new is that the path from
	/// <c>Program.Run</c> through dispatch to a command's <c>Execute</c> is exercised
	/// at all, which it was not before.
	/// </remarks>
	public class CliTests
	{
		// ============================================ exit codes out of the front door

		[Fact]
		public void A_clean_conversion_exits_zero_and_says_nothing()
		{
			var cli = new CliHarness().WithDocument("a.docx", "hello");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", "text", "--out-dir", "out", "a.docx"));
			Assert.Empty(cli.Errors);

			// Silence means success: a clean run prints nothing unless asked.
			Assert.Empty(cli.Out);
		}

		[Fact]
		public void A_usage_error_exits_two_and_names_the_problem_on_stderr()
		{
			var cli = new CliHarness().WithDocument("a.docx");

			Assert.Equal(ExitCode.UsageError, cli.Run("convert", "--to", "nonsense", "--out-dir", "out", "a.docx"));

			string line = Assert.Single(cli.Errors);
			Assert.StartsWith(ErrorCodes.UnknownFormat + ": ", line);
			Assert.Contains("nonsense", line);
		}

		[Fact]
		public void An_unknown_command_exits_two()
		{
			var cli = new CliHarness();

			Assert.Equal(ExitCode.UsageError, cli.Run("nonsense", "a.docx"));
			Assert.Contains(cli.Errors, line => line.StartsWith(ErrorCodes.UnknownCommand + ": "));
		}

		[Fact]
		public void Naming_no_input_exits_three()
		{
			var cli = new CliHarness();

			Assert.Equal(ExitCode.NoInputsMatched, cli.Run("convert", "--to", "text", "--out-dir", "out"));
			Assert.Contains(cli.Errors, line => line.StartsWith(ErrorCodes.NoInputs + ": "));
		}

		[Fact]
		public void An_output_conflict_exits_four()
		{
			var cli = new CliHarness()
				.WithDocument("a.docx")
				.WithDocument("b.docx");

			Assert.Equal(ExitCode.OutputConflict, cli.Run("convert", "--to", "pdf", "--out-file", "one.pdf", "a.docx", "b.docx"));
			Assert.Contains(cli.Errors, line => line.StartsWith(ErrorCodes.OutFileMultiInput + ": "));
		}

		[Fact]
		public void One_bad_document_among_good_ones_exits_one()
		{
			var cli = new CliHarness()
				.WithDocument("good.docx", "text")
				.WithUnreadableDocument("bad.docx", EngineError.FileCorrupt);

			Assert.Equal(ExitCode.PartialFailure, cli.Run("convert", "--to", "text", "--out-dir", "out", "good.docx", "bad.docx"));
		}

		// ============================================ the two stderr shapes

		/// <remarks>
		/// A per-document failure carries the input it belongs to; a run-level failure
		/// has no input to name. Neither shape was checked anywhere before, because
		/// nothing drove the whole path; these two tests are now their only authority.
		/// </remarks>
		[Fact]
		public void A_per_document_failure_is_prefixed_by_its_input()
		{
			var cli = new CliHarness()
				.WithDocument("good.docx", "text")
				.WithUnreadableDocument("bad.docx", EngineError.FileCorrupt);

			cli.Run("convert", "--to", "text", "--out-dir", "out", "good.docx", "bad.docx");

			string line = Assert.Single(cli.Errors);
			Assert.StartsWith("bad.docx: " + ErrorCodes.DocumentFailed + ": ", line);
		}

		/// <remarks>
		/// A limit that applies to one document is per-document even though it is not a
		/// <c>DocumentError</c> — it exits 6, not 1. It names its input through
		/// <c>CliError.Path</c> rather than by folding the filename into its own
		/// message, so the reporter puts it in the same place as every other
		/// per-document failure.
		/// </remarks>
		[Fact]
		public void A_per_document_limit_is_prefixed_like_any_other_per_document_failure()
		{
			var error = new LimitExceededError("--timeout of 1s exceeded", "huge.pdf");
			var console = new RecordingConsole();

			int exitCode = Program.ReportError(error, console);

			Assert.Equal(ExitCode.LimitExceeded, exitCode);
			Assert.Equal("huge.pdf: E_LIMIT_EXCEEDED: --timeout of 1s exceeded", Assert.Single(console.ErrorLines));
		}

		[Fact]
		public void A_run_wide_limit_names_no_input()
		{
			// --max-time and --max-memory bound the whole run, so there is nothing to name.
			var console = new RecordingConsole();

			Program.ReportError(new LimitExceededError("--max-time of 60s exceeded"), console);

			Assert.Equal("E_LIMIT_EXCEEDED: --max-time of 60s exceeded", Assert.Single(console.ErrorLines));
		}

		[Fact]
		public void A_run_level_failure_has_no_input_to_name()
		{
			var cli = new CliHarness().WithDocument("a.docx");

			cli.Run("convert", "--to", "text", "--out-file", "a.txt", "--out-dir", "out", "a.docx");

			string line = Assert.Single(cli.Errors);
			Assert.StartsWith(ErrorCodes.MutuallyExclusive + ": ", line);
		}

		// ============================================ canvas formats to stdout

		[Fact]
		public void A_canvas_format_can_stream_to_stdout()
		{
			// Reproduces a real bug: --stdout resolves to OutputCardinality.Stream,
			// which leaves both PlannedItem.Target and TargetPattern null. Convert's
			// own branches only ever expect one or the other, so the per-page branch
			// ran with a null TargetPattern and threw a bare NullReferenceException,
			// reported as "E_DOCUMENT_FAILED: Object reference not set to an instance
			// of an object." — the exact symptom, with none of the detail.
			var cli = new CliHarness().WithDocument("a.docx", "text");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", "pdf", "--stdout", "a.docx"));

			Assert.Empty(cli.Errors);
			Assert.Contains(cli.Engine.Calls, call => call.StartsWith("MakeCanvas(<stream>,"));
			Assert.DoesNotContain(cli.Engine.Calls, call => call.StartsWith("MakeCanvas(a.pdf,"));
		}

		[Fact]
		public void Docpack_can_stream_to_stdout_too()
		{
			// docpack is not a canvas format at all — FormatSpec.CanvasType is null,
			// it goes through IDocument.MakeDocPack instead — so the canvas fix above
			// does not cover it. Same root cause, same fix: item.Target is null under
			// OutputCardinality.Stream, and MakeDocPack was called with it unconditionally.
			var cli = new CliHarness().WithDocument("a.docx", "text");
			FakeDocument document = cli.Engine.AddDocument("a.docx", "text");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", "docpack", "--stdout", "a.docx"));

			Assert.Empty(cli.Errors);
			Assert.Equal(new[] { "<stream>" }, document.DocPacksWritten);
		}

		[Fact]
		public void A_report_alongside_a_text_conversion_is_refused_before_anything_opens()
		{
			// Found end to end: convert --to text --bookmarks failed inside the engine,
			// and so did every other pairing of a text format with a report. One open
			// cannot serve both — text extraction needs the text reader, a report needs
			// a paginated handle — so whichever format bit is chosen, the other half
			// fails. Refused up front instead, and nothing is opened.
			var cli = new CliHarness().WithDocument("a.docx", "hello");

			Assert.Equal(ExitCode.UsageError, cli.Run("convert", "--to", "text", "--bookmarks", "--stdout", "a.docx"));

			Assert.Contains(cli.Errors, line => line.Contains(ErrorCodes.ReportNeedsHiDef));
			Assert.DoesNotContain(cli.Engine.Calls, call => call.StartsWith("OpenDocument("));
		}

		[Theory]
		[InlineData("pdf")]      // single-file canvas: item.Target
		[InlineData("png")]      // per-page canvas: item.TargetPattern
		[InlineData("text")]     // written through IFileSystem, not the engine
		[InlineData("docpack")]  // a third writer again
		public void An_output_directory_that_does_not_exist_yet_is_created(string format)
		{
			// Found from the README's own first example. Only the text path created it,
			// so --out-dir out/ worked for --to text and failed for every other format
			// with the engine's "FileStream::Create" error, which reads as though the
			// input were missing. One rule now, in ConvertCommand.
			var cli = new CliHarness().WithDocument("a.docx", "hello");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", format, "--out-dir", "brand/new", "a.docx"));

			Assert.True(cli.FileSystem.DirectoryExists("brand/new"),
				$"--to {format} did not create its output directory");
		}

		[Theory]
		[InlineData("text")]
		[InlineData("md-text")]
		[InlineData("html")]
		public void Combining_a_text_format_concatenates_into_one_writer(string format)
		{
			// These formats have SupportsCombine = true and no canvas, so the combined
			// path read Format.CanvasType.Value on a null and threw — not a CliError, so
			// it escaped Program's handler and took the process down with an unhandled
			// exception rather than any documented exit code.
			var cli = new CliHarness()
				.WithDocument("a.docx", "first")
				.WithDocument("b.docx", "second");

			Assert.Equal(ExitCode.Success,
				cli.Run("convert", "--to", format, "--combine", "--out-file", "out/all.txt", "a.docx", "b.docx"));

			Assert.Empty(cli.Errors);
			Assert.Equal("firstsecond", cli.FileSystem.ReadText("out/all.txt"));
		}

		[Theory]
		[InlineData("convert", "--to", "text")]
		[InlineData("extract")]
		public void An_open_cancelled_by_the_timeout_is_reported_as_the_limit(params string[] verb)
		{
			// The engine stops and reports IGR_CANCELLED, which reaches us as
			// EngineError.Cancelled. That must not be read as a broken document: the
			// cancellation was ours to ask for, so the limit is the cause and
			// E_LIMIT_EXCEEDED with exit 6 is the report — the same line the check
			// after the document produces, so the two routes are indistinguishable.
			//
			// Driven through the real predicate rather than by scripting a cancelled
			// open: the run has to be the one that asks to stop, because that is what
			// records the reason. Scripting the engine's refusal instead tested the
			// translation without the decision behind it, and went on passing when a
			// cancellation nobody asked for started reporting the document instead.
			var cli = new CliHarness().WithDocument("slow.docx", "text");
			// The document "takes" a minute without the suite waiting one: the clock is
			// the harness's, and the open moves it past the limit so the predicate fires
			// on its next poll. Sleeping instead cost more than the whole rest of the
			// suite, since the smallest --timeout the validator accepts is one second.
			cli.Engine.DuringOpen = () => cli.Clock.Advance(TimeSpan.FromSeconds(60));

			var argv = new List<string>(verb) { "--out-dir", "out", "--timeout", "30", "slow.docx" };
			Assert.Equal(ExitCode.LimitExceeded, cli.Run(argv.ToArray()));

			Assert.Equal("slow.docx: E_LIMIT_EXCEEDED: --timeout of 30s exceeded", Assert.Single(cli.Errors));
		}

		[Theory]
		[InlineData("convert", "--to", "text")]
		[InlineData("extract")]
		public void Opening_a_document_offers_the_engine_a_way_to_stop(params string[] verb)
		{
			// The predicate has to reach Open, because the heartbeat is raised during
			// Open and nowhere else — IGR_Render_Page takes no callback. Passing null
			// here would leave a document that hangs while parsing hanging forever,
			// which is exactly what --timeout is reached for.
			var cli = new CliHarness().WithDocument("a.docx", "text");

			var argv = new List<string>(verb) { "--out-dir", "out", "--timeout", "30", "a.docx" };
			cli.Run(argv.ToArray());

			Assert.Contains(cli.Engine.Calls, call => call.StartsWith("OpenDocument(a.docx,") && call.EndsWith(",cancellable)"));
		}

		[Theory]
		[InlineData("convert", "--to", "text")]
		[InlineData("extract")]
		public void An_interrupted_run_stops_between_documents_and_exits_130(params string[] verb)
		{
			// Ctrl+C is recorded rather than killing the process, so the run reports and
			// exits through the same path as every other code. Between documents is the
			// only point in a batch where stopping leaves nothing half-written; a render
			// already in flight cannot be interrupted at all, because IGR_Render_Page
			// takes no callback.
			var interruption = new Interruption();
			interruption.Request();

			var cli = new CliHarness(interruption)
				.WithDocument("a.docx", "one")
				.WithDocument("b.docx", "two");

			var argv = new List<string>(verb) { "--out-dir", "out", "a.docx", "b.docx" };
			Assert.Equal(ExitCode.Interrupted, cli.Run(argv.ToArray()));

			Assert.Equal("E_INTERRUPTED: interrupted", Assert.Single(cli.Errors));
			Assert.DoesNotContain(cli.Engine.Calls, call => call.StartsWith("OpenDocument("));
		}

		[Fact]
		public void A_run_nobody_interrupts_is_unaffected()
		{
			var cli = new CliHarness(new Interruption()).WithDocument("a.docx", "one");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", "text", "--out-dir", "out", "a.docx"));
			Assert.Empty(cli.Errors);
		}

		// ===================================== diagnostics on every command

		[Theory]
		[InlineData("convert", "--to", "text")]
		[InlineData("extract")]
		[InlineData("meta")]
		[InlineData("list")]
		[InlineData("identify")]
		public void Every_command_that_reads_a_document_honours_the_diagnostics(params string[] verb)
		{
			// These are shared options, so they parse on all five — and used to do
			// nothing on any of them. convert and extract were wired first; meta, list
			// and identify had no Diagnostics at all, so the switches stayed inert there
			// even after the others worked.
			var cli = new CliHarness().WithDocument("a.docx", "hello");
			cli.Engine.MemoryStatus = "heap: 42 bytes";

			// No --out-dir: identify and meta declare no output group, so passing one
			// is a usage error rather than a no-op.
			var argv = new List<string>(verb) { "-x", "a.docx" };
			Assert.Equal(ExitCode.Success, cli.Run(argv.ToArray()));

			Assert.Contains(cli.Out, line => line.Contains("heap: 42 bytes"));
		}

		[Theory]
		[InlineData("meta")]
		[InlineData("list")]
		[InlineData("identify")]
		public void The_debug_input_switches_reach_the_inspection_commands(params string[] verb)
		{
			// identify never opens the document, so it has no open callback to cancel or
			// log through — but what the engine makes of a truncated file is exactly what
			// --debug-truncate asks, so it gets the stream form too.
			var cli = new CliHarness().WithDocument("a.docx", "hello world");

			var argv = new List<string>(verb) { "--debug-use-memory-stream", "a.docx" };
			Assert.Equal(ExitCode.Success, cli.Run(argv.ToArray()));

			Assert.Contains(cli.Engine.Calls, call => call.Contains("<stream:"));
		}

		[Theory]
		[InlineData("meta")]
		[InlineData("list")]
		[InlineData("identify")]
		public void An_interrupt_stops_the_inspection_commands_too(params string[] verb)
		{
			var interruption = new Interruption();
			interruption.Request();

			var cli = new CliHarness(interruption)
				.WithDocument("a.docx", "one")
				.WithDocument("b.docx", "two");

			var argv = new List<string>(verb) { "a.docx", "b.docx" };
			Assert.Equal(ExitCode.Interrupted, cli.Run(argv.ToArray()));

			Assert.Equal("E_INTERRUPTED: interrupted", Assert.Single(cli.Errors));
		}

		[Fact]
		public void Extraction_creates_its_directories_through_the_filesystem_seam()
		{
			// SubfileExtraction called System.IO.Directory.CreateDirectory, so a subfile
			// whose name carries a path made real directories wherever the unit suite
			// happened to run — and nothing about the extraction could be asserted
			// without touching the disk.
			var cli = new CliHarness().WithDocument("archive.zip");
			FakeDocument container = cli.Engine.AddDocument("archive.zip");
			container.SubFileEntries.Add(new FakeSubFile("nested/deep/inner.docx"));

			Assert.Equal(ExitCode.Success, cli.Run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"));

			Assert.Empty(cli.Errors);
			Assert.True(cli.FileSystem.DirectoryExists("out/nested/deep"),
				"extraction did not create its directory through IFileSystem");
		}

		[Fact]
		public void Combined_output_is_finished_before_the_viewer_sees_it()
		{
			// --combine kept the canvas and the writer alive until the enclosing scope
			// ended, which is after --view runs: the viewer was handed a file with
			// nothing written to it yet, and for a PDF that means no trailer. The
			// content is read inside the viewer because afterwards it looks right
			// either way.
			var cli = new CliHarness()
				.WithDocument("a.docx", "first")
				.WithDocument("b.docx", "second");

			Assert.Equal(ExitCode.Success,
				cli.Run("convert", "--to", "text", "--combine", "--out-file", "out/all.txt", "--view", "--sort", "a.docx", "b.docx"));

			Assert.Equal(new[] { "out/all.txt" }, cli.Viewed);
			Assert.Contains("first", Assert.Single(cli.ViewedContent));
			Assert.Contains("second", Assert.Single(cli.ViewedContent));
		}

		[Fact]
		public void An_output_the_run_has_already_written_is_not_replaced_silently()
		{
			// The planner checks every target before the run starts, against a directory
			// none of this run's outputs are in yet. Two names the filesystem matches to
			// one file but the ASCII collision key does not --- NTFS matches the two
			// spellings of a-ring, measured --- therefore both passed planning, and the
			// second conversion replaced the first with no --overwrite and exit 0.
			var cli = new CliHarness()
				.WithDocument("a/\u00c5.docx", "from the upper-case name")
				.WithDocument("b/\u00e5.docx", "from the lower-case name");
			cli.FileSystem.TreatAsSameFile("out/\u00e5.txt", "out/\u00c5.txt");

			int exitCode = cli.Run("convert", "--to", "text", "--out-dir", "out", "--sort",
				"a/\u00c5.docx", "b/\u00e5.docx");

			Assert.Equal(ExitCode.OutputConflict, exitCode);
			Assert.Contains(cli.Errors, line => line.Contains("E_OUTPUT_EXISTS"));
			Assert.Equal("from the upper-case name", cli.FileSystem.ReadText("out/\u00c5.txt"));
		}

		[Fact]
		public void Two_containers_cannot_extract_onto_each_other()
		{
			// The claim set was built per container while the extractor is built per run,
			// so two archives each holding a readme.txt were never compared: with
			// --overwrite the second silently replaced the first, exit 0. The planner
			// refuses that for ordinary outputs whether or not --overwrite is given.
			var cli = new CliHarness().WithDocument("a.zip").WithDocument("b.zip");
			cli.Engine.AddDocument("a.zip").SubFileEntries.Add(new FakeSubFile("readme.txt"));
			cli.Engine.AddDocument("b.zip").SubFileEntries.Add(new FakeSubFile("readme.txt"));

			int exitCode = cli.Run("extract", "--overwrite", "--out-dir", "out", "a.zip", "b.zip");

			Assert.Equal(ExitCode.OutputConflict, exitCode);
			Assert.Contains(cli.Errors, line => line.Contains("E_OUTPUT_COLLISION"));
		}

		[Fact]
		public void A_subfile_cannot_be_written_through_a_link_out_of_the_output_directory()
		{
			// Containment was lexical only: every segment of out/latest/index.html is a
			// plain name under the root, and if out/latest is a link the write still lands
			// wherever it points. The container picks that name, not the user.
			var cli = new CliHarness().WithDocument("archive.zip");
			cli.Engine.AddDocument("archive.zip").SubFileEntries.Add(new FakeSubFile("latest/index.html"));
			cli.FileSystem.AddDirectory("elsewhere").MarkSymlink("out/latest", "elsewhere");

			int exitCode = cli.Run("extract", "--make-dirs", "--out-dir", "out", "archive.zip");

			Assert.Equal(ExitCode.OutputConflict, exitCode);
			Assert.Contains(cli.Errors, line => line.Contains("E_UNSAFE_OUTPUT"));
		}

		[Fact]
		public void An_ordinary_output_directory_is_not_mistaken_for_a_link()
		{
			var cli = new CliHarness().WithDocument("archive.zip");
			cli.Engine.AddDocument("archive.zip").SubFileEntries.Add(new FakeSubFile("latest/index.html"));
			cli.FileSystem.AddDirectory("out/latest");

			Assert.Equal(ExitCode.Success, cli.Run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"));
			Assert.Empty(cli.Errors);
		}

		[Fact]
		public void Extraction_refuses_to_replace_an_existing_file_without_overwrite()
		{
			// The copy used to be unconditional, so a second extract into the same
			// directory silently replaced whatever was there. It is the same rule as
			// every other output now: E_OUTPUT_EXISTS, exit 4, nothing written.
			var cli = new CliHarness().WithDocument("archive.zip").WithExistingOutput("out/inner.txt");
			var inner = new FakeSubFile("inner.txt");
			cli.Engine.AddDocument("archive.zip").SubFileEntries.Add(inner);

			Assert.Equal(ExitCode.OutputConflict, cli.Run("extract", "--out-dir", "out", "archive.zip"));

			Assert.Contains(cli.Errors, line => line.Contains("E_OUTPUT_EXISTS"));
			Assert.Empty(inner.CopiedTo);
		}

		[Fact]
		public void Extraction_replaces_an_existing_file_with_overwrite()
		{
			var cli = new CliHarness().WithDocument("archive.zip").WithExistingOutput("out/inner.txt");
			var inner = new FakeSubFile("inner.txt");
			cli.Engine.AddDocument("archive.zip").SubFileEntries.Add(inner);

			Assert.Equal(ExitCode.Success, cli.Run("extract", "--overwrite", "--out-dir", "out", "archive.zip"));

			Assert.Equal("out/inner.txt", Assert.Single(inner.CopiedTo));
		}

		[Fact]
		public void Extraction_does_not_write_through_a_symbolic_link_inside_the_output_directory()
		{
			// The name is clean text — out/link/inner.txt sits inside out — but with
			// out/link pointing elsewhere the bytes would land outside it.
			var cli = new CliHarness().WithDocument("archive.zip");
			cli.FileSystem.MarkSymlink("out/link", "/elsewhere");
			var inner = new FakeSubFile("link/inner.txt");
			cli.Engine.AddDocument("archive.zip").SubFileEntries.Add(inner);

			Assert.Equal(ExitCode.OutputConflict, cli.Run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"));

			Assert.Contains(cli.Errors, line => line.Contains(ErrorCodes.UnsafeOutput));
			Assert.Empty(inner.CopiedTo);
		}

		[Fact]
		public void Extraction_writes_into_an_output_directory_that_is_itself_a_link()
		{
			// The root is the user's own --out-dir; pointing it at a link is a choice.
			var cli = new CliHarness().WithDocument("archive.zip");
			cli.FileSystem.MarkSymlink("out", "/elsewhere");
			var inner = new FakeSubFile("inner.txt");
			cli.Engine.AddDocument("archive.zip").SubFileEntries.Add(inner);

			Assert.Equal(ExitCode.Success, cli.Run("extract", "--out-dir", "out", "archive.zip"));

			Assert.Equal("out/inner.txt", Assert.Single(inner.CopiedTo));
		}

		[Theory]
		[InlineData("extract")]
		[InlineData("convert", "--to", "text")]
		public void An_empty_out_dir_is_a_usage_error(params string[] verb)
		{
			// An empty directory joins as "/name", which is the filesystem root.
			var cli = new CliHarness().WithDocument("a.docx", "text");

			var argv = new List<string>(verb) { "--out-dir", "", "a.docx" };
			Assert.Equal(ExitCode.UsageError, cli.Run(argv.ToArray()));

			Assert.Contains(cli.Errors, line => line.Contains("E_BAD_VALUE") && line.Contains("--out-dir does not accept the value ''"));
		}

		[Fact]
		public void Fingerprint_creates_no_output_directory()
		{
			// --fingerprint renders into a discarded stream, so the plan's target is a
			// guess that is never written to; creating a directory for it would be a
			// side effect of a run that produces no files.
			var cli = new CliHarness().WithDocument("a.docx", "hello");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", "pdf", "--fingerprint", "a.docx"));

			Assert.False(cli.FileSystem.DirectoryExists("brand/new"));
		}

		[Fact]
		public void A_report_alongside_a_canvas_conversion_is_allowed()
		{
			var cli = new CliHarness().WithDocument("a.docx", "hello");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", "pdf", "--bookmarks", "--stdout", "a.docx"));

			Assert.Empty(cli.Errors);
		}

		// ============================================ dispatch branches

		[Fact]
		public void Dry_run_prints_the_plan_and_opens_nothing()
		{
			var cli = new CliHarness().WithDocument("a.docx", "text");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", "pdf", "--out-dir", "out", "--dry-run", "a.docx"));

			Assert.Contains(cli.Out, line => line.Contains("command convert"));
			Assert.Contains(cli.Out, line => line.Contains("target out/a.pdf"));
			Assert.DoesNotContain(cli.Engine.Calls, call => call.StartsWith("OpenDocument("));
		}

		[Fact]
		public void Print_plan_is_the_same_branch_as_dry_run()
		{
			var dryRun = new CliHarness().WithDocument("a.docx");
			var printPlan = new CliHarness().WithDocument("a.docx");

			dryRun.Run("convert", "--to", "pdf", "--out-dir", "out", "--dry-run", "a.docx");
			printPlan.Run("convert", "--to", "pdf", "--out-dir", "out", "--print-plan", "a.docx");

			Assert.Equal(dryRun.Out, printPlan.Out);
		}

		[Fact]
		public void Print_contract_dumps_the_surface_and_runs_nothing()
		{
			var cli = new CliHarness().WithDocument("a.docx");

			Assert.Equal(ExitCode.Success, cli.Run("convert", "--to", "pdf", "--out-dir", "out", "--print-contract", "a.docx"));

			Assert.Contains(cli.Out, line => line.Contains("command convert"));
			Assert.Contains(cli.Out, line => line.Contains("--out-dir"));
			Assert.Empty(cli.Engine.Calls);
		}

		[Fact]
		public void Print_contract_wins_over_a_dry_run()
		{
			// Both short-circuit; the contract dump is checked first, so it is what runs.
			var cli = new CliHarness().WithDocument("a.docx");

			cli.Run("convert", "--to", "pdf", "--out-dir", "out", "--print-contract", "--dry-run", "a.docx");

			Assert.Contains(cli.Out, line => line.Contains("--print-contract"));
			Assert.DoesNotContain(cli.Out, line => line.Contains("cardinality"));
		}

		// ============================================ the engine's lifetime

		[Fact]
		public void A_command_that_needs_no_engine_never_asks_for_one()
		{
			// formats reads no document, but it does read the catalogue, so the engine is
			// touched exactly once and for that alone.
			var cli = new CliHarness();
			cli.Engine.Formats.Add(new EngineFormat { Id = 254, ShortName = "docx", DisplayName = "Microsoft Word 2007" });

			Assert.Equal(ExitCode.Success, cli.Run("formats"));

			Assert.Contains(cli.Out, line => line.Contains("id=254"));
			Assert.DoesNotContain(cli.Engine.Calls, call => call.StartsWith("OpenDocument("));
		}

		[Fact]
		public void A_run_that_fails_still_closes_every_document_it_opened()
		{
			var cli = new CliHarness()
				.WithDocument("first.docx", "text")
				.WithUnreadableDocument("second.docx", EngineError.NotReadable);

			cli.Run("convert", "--to", "text", "--out-dir", "out", "first.docx", "second.docx");

			Assert.All(
				cli.Engine.Calls.Where(call => call.StartsWith("OpenDocument(")),
				call => Assert.Contains(".docx", call));
		}

		// ============================================ other verbs, end to end

		[Fact]
		public void Identify_prints_one_line_per_input()
		{
			var cli = new CliHarness();
			cli.WithDocument("a.bin");
			cli.Engine.AddDocument("a.bin").FormatName = "Microsoft Word 2007";

			Assert.Equal(ExitCode.Success, cli.Run("identify", "a.bin"));

			string line = Assert.Single(cli.Out);
			Assert.StartsWith("a.bin: format=", line);
			Assert.Contains("name=Microsoft Word 2007", line);
		}

		[Fact]
		public void Version_needs_neither_a_document_nor_a_walk()
		{
			var cli = new CliHarness();

			Assert.Equal(ExitCode.Success, cli.Run("version"));
			Assert.NotEmpty(cli.Out);
		}

		// ============================================ the console pre-scan

		/// <remarks>
		/// Read straight out of argv before anything is bound, because the console has
		/// to exist before the first line is written and binding has not happened yet.
		/// A raw scan is only safe because none of the three takes a value and prefix
		/// abbreviation is off.
		/// </remarks>
		[Fact]
		public void The_console_settings_are_read_before_anything_is_bound()
		{
			Assert.True(Program.PreScanConsoleOptions(new[] { "convert", "--silent" }).Silent);
			Assert.True(Program.PreScanConsoleOptions(new[] { "convert", "--verbose" }).Verbose);
			Assert.True(Program.PreScanConsoleOptions(new[] { "convert", "--color" }).Color);

			Hyland.DocumentFilters.Cli.Configuration.ConsoleOptions none =
				Program.PreScanConsoleOptions(new[] { "convert", "a.docx" });

			Assert.False(none.Silent);
			Assert.False(none.Verbose);
			Assert.False(none.Color);
		}

		[Fact]
		public void The_pre_scan_reads_only_exact_spellings()
		{
			// Prefix abbreviation is off everywhere, so a partial spelling is an unknown
			// argument rather than a match — the scan must not be more generous than the
			// parser it front-runs.
			Assert.False(Program.PreScanConsoleOptions(new[] { "convert", "--sil" }).Silent);
			Assert.False(Program.PreScanConsoleOptions(new[] { "convert", "--silent=true" }).Silent);
		}

		[Fact]
		public void The_pre_scan_survives_an_empty_command_line()
		{
			Hyland.DocumentFilters.Cli.Configuration.ConsoleOptions options =
				Program.PreScanConsoleOptions(new string[0]);

			Assert.False(options.Silent);
		}
	}
}
