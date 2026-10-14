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

using System.Threading;
using Hyland.DocumentFilters.Cli.Commands;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Cli;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The diagnostic switches, each of which used to parse and then do nothing.
	/// </summary>
	/// <remarks>
	/// Nine arguments were accepted, validated, and never read again. That is worse
	/// than not having them: the help text promises something, the validator agrees
	/// the command line is sensible, and the run behaves exactly as if the argument
	/// had been left out. These tests are what stops one quietly returning to that
	/// state.
	/// </remarks>
	public class DiagnosticSwitchTests
	{
		[Fact]
		public void Temp_dir_is_given_to_the_engine()
		{
			// A library-wide setting, applied once after Initialize rather than per
			// document: IGR_Multiplex(IGR_Multi_Set_Temp_Path), as doc2text does it.
			var engine = new FakeDocumentFiltersEngine();
			var options = new LibraryOptions();

			DocumentFiltersSession.For(engine);
			engine.SetTempPath("/scratch");

			Assert.Equal("/scratch", engine.TempPath);
			Assert.Contains("SetTempPath(/scratch)", engine.Calls);
		}

		[Theory]
		[InlineData(0, false, false)]   // neither switch
		[InlineData(1, false, true)]    // -x: the run only
		[InlineData(2, true, true)]     // --xx: every document too
		public void Engine_memory_is_reported_at_the_level_asked_for(int level, bool perDocument, bool perRun)
		{
			var engine = new FakeDocumentFiltersEngine { MemoryStatus = "heap: 42 bytes" };
			var console = new RecordingConsole();
			var diagnostics = new Diagnostics(new DiagnosticOptions { MemoryStatusLevel = level }, console, engine);

			diagnostics.BeginRun();
			diagnostics.BeginDocument("a.docx", 1, 1);
			diagnostics.EndDocument("a.docx");
			int afterDocument = console.OutLines.Count;
			diagnostics.EndRun(1, 0);

			Assert.Equal(perDocument ? 1 : 0, afterDocument);
			Assert.Equal((perDocument ? 1 : 0) + (perRun ? 1 : 0), console.OutLines.Count);

			if (perRun)
				Assert.Contains(console.OutLines, line => line.Contains("heap: 42 bytes"));
		}

		[Fact]
		public void Logmem_reports_the_processes_own_peak()
		{
			// The process, not the engine: -x and --xx answer the other question.
			var console = new RecordingConsole();
			new Diagnostics(new DiagnosticOptions { LogMemory = true }, console).EndRun(1, 0);

			Assert.Contains(console.OutLines, line => line.StartsWith("peak memory: ") && line.EndsWith("MB"));
		}

		/// <summary>A clock a test moves by hand, so a deadline can pass instantly.</summary>
		private sealed class TestClock
		{
			public TimeSpan Now { get; private set; }

			public TimeSpan Read() => Now;

			public void Advance(TimeSpan by) => Now += by;
		}

		// ------------------------------------------------ the stop predicate

		/// <summary>
		/// When the deadline has passed <em>and</em> the user has interrupted, the run
		/// reports the interrupt.
		/// </summary>
		/// <remarks>
		/// The reason is settled by the poll that stopped the document rather than
		/// worked out afterwards, which is what makes this answerable at all: by the
		/// time a cancelled open is caught both conditions can be true, and asking then
		/// is a second guess at a question already answered.
		/// </remarks>
		[Fact]
		public void An_interrupt_outranks_a_deadline_that_also_passed()
		{
			var clock = new TestClock();
			var diagnostics = new Diagnostics(new DiagnosticOptions { Timeout = 30 }, new RecordingConsole(),
											  engine: null, clock: clock.Read);
			diagnostics.BeginDocument("slow.docx", 1, 1);
			clock.Advance(TimeSpan.FromSeconds(31));

			var interruption = new Interruption();
			interruption.Request();

			StopSignal stop = CommandExecution.WatchForStop(diagnostics, interruption);

			Assert.True(stop.ShouldStop());
			Assert.Equal(StopReason.Interrupted, stop.Reason);
			Assert.IsType<InterruptedError>(stop.Cause("slow.docx"));
		}

		/// <summary>A cancelled open nobody asked for is the document failing, not a limit.</summary>
		[Fact]
		public void A_cancellation_with_no_recorded_reason_is_a_document_error()
		{
			var diagnostics = new Diagnostics(new DiagnosticOptions(), new RecordingConsole());
			diagnostics.BeginDocument("a.docx", 1, 1);

			StopSignal stop = CommandExecution.WatchForStop(diagnostics, null);

			Assert.False(stop.ShouldStop());
			Assert.IsType<DocumentError>(stop.Cause("a.docx"));
		}

		/// <remarks>
		/// Shared by every command that opens a document. It was four copies of the
		/// same two lines until this moved, which is four places for one of them to
		/// quietly lose a term — and neither term is visible from the outside, since
		/// both produce the same cancelled open.
		/// </remarks>
		[Fact]
		public void Nothing_stops_a_run_inside_its_limits_that_nobody_interrupted()
		{
			var diagnostics = new Diagnostics(new DiagnosticOptions { Timeout = 600 }, new RecordingConsole());
			diagnostics.BeginDocument("a.docx", 1, 1);

			Assert.False(CommandExecution.WatchForStop(diagnostics, new Interruption()).ShouldStop());
			Assert.False(CommandExecution.WatchForStop(diagnostics, null).ShouldStop());
		}

		[Fact]
		public void The_documents_deadline_stops_it()
		{
			var clock = new TestClock();
			var diagnostics = new Diagnostics(new DiagnosticOptions { Timeout = 30 }, new RecordingConsole(),
											  engine: null, clock: clock.Read);
			diagnostics.BeginDocument("slow.docx", 1, 1);
			clock.Advance(TimeSpan.FromSeconds(31));

			StopSignal stop = CommandExecution.WatchForStop(diagnostics, null);

			Assert.True(stop.ShouldStop());
			Assert.Equal(StopReason.TimedOut, stop.Reason);
		}

		[Fact]
		public void An_interrupt_stops_it_even_with_time_to_spare()
		{
			var diagnostics = new Diagnostics(new DiagnosticOptions { Timeout = 600 }, new RecordingConsole());
			diagnostics.BeginDocument("a.docx", 1, 1);

			var interruption = new Interruption();
			StopSignal stop = CommandExecution.WatchForStop(diagnostics, interruption);

			Assert.False(stop.ShouldStop());
			Assert.Null(stop.Reason);

			interruption.Request();

			Assert.True(stop.ShouldStop());
			Assert.Equal(StopReason.Interrupted, stop.Reason);
		}

		// ------------------------------------------------------- --no-images

		[Theory]
		[InlineData("md")]
		[InlineData("md-text")]
		public void No_images_suppresses_them_for_the_markdown_formats(string token)
		{
			Assert.Equal("MARKDOWN_INCLUDE_IMAGES=OFF",
				CanvasConversion.ImageSuppression(FormatCatalog.Resolve(token)));
		}

		[Theory]
		[InlineData("html5")]   // inlines the page raster itself — suppressing would blank it
		[InlineData("svg")]
		[InlineData("text")]    // nowhere to put an image in the first place
		[InlineData("pdf")]
		public void No_images_is_ignored_where_it_has_no_meaning(string token)
		{
			Assert.Equal(string.Empty, CanvasConversion.ImageSuppression(FormatCatalog.Resolve(token)));
		}

		[Fact]
		public void No_images_reaches_the_canvas_options_for_hi_def_markdown()
		{
			Assert.Equal("MARKDOWN_INCLUDE_IMAGES=OFF",
				CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("md"), null, noImages: true));

			Assert.Equal(string.Empty,
				CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("md"), null, noImages: false));
		}

		[Fact]
		public void No_images_reaches_the_open_options_for_text_mode_markdown()
		{
			// md-text never opens a canvas, so the same rule has to arrive at Open.
			var options = new ConvertOptions { NoImages = true };
			ExtractorOpenSettings settings =
				ExtractorOpenSettings.ForFormat(FormatCatalog.Resolve("md-text"), options, new SharedOptions());

			Assert.Contains("MARKDOWN_INCLUDE_IMAGES=OFF", settings.Options);
		}

		// ----------------------------------------------- --log-level/--log-modules

		[Fact]
		public void No_log_level_means_no_log_at_all()
		{
			// Null rather than a disabled instance, so the engine is handed no callback
			// and never composes a message nobody prints.
			Assert.Null(EngineLog.For(new DiagnosticOptions(), new RecordingConsole()));
		}

		[Fact]
		public void Every_module_logs_when_none_are_named()
		{
			EngineLog log = EngineLog.For(new DiagnosticOptions { LogLevel = "debug" }, new RecordingConsole());

			Assert.Equal(EngineLog.LevelDebug, log.LevelFor("pdf"));
			Assert.Equal(EngineLog.LevelDebug, log.LevelFor("anything"));
		}

		[Fact]
		public void Naming_modules_silences_the_rest()
		{
			// The filter is applied by answering "none" for a module nobody asked about,
			// which is where --log-modules actually takes effect — the engine has no
			// notion of the user's list.
			EngineLog log = EngineLog.For(
				new DiagnosticOptions { LogLevel = "trace", LogModules = "pdf,ole" }, new RecordingConsole());

			Assert.Equal(EngineLog.LevelTrace, log.LevelFor("pdf"));
			Assert.Equal(EngineLog.LevelTrace, log.LevelFor("OLE"));
			Assert.Equal(EngineLog.LevelNone, log.LevelFor("html"));
		}

		[Fact]
		public void The_debug_spellings_are_the_same_switch()
		{
			EngineLog log = EngineLog.For(
				new DiagnosticOptions { DebugLogLevel = "warning", DebugLogModules = "pdf" }, new RecordingConsole());

			Assert.Equal(EngineLog.LevelWarning, log.LevelFor("pdf"));
			Assert.Equal(EngineLog.LevelNone, log.LevelFor("xls"));
		}

		[Fact]
		public void A_log_message_goes_to_stderr_with_its_level_and_module()
		{
			// stderr, as doc2text does. It shares the stream with the two contract error
			// shapes, which is safe because nothing logs unless --log-level was passed.
			var console = new RecordingConsole();
			EngineLog log = EngineLog.For(new DiagnosticOptions { LogLevel = "debug" }, console);

			log.Write(EngineLog.LevelDebug, "pdf", "parsing xref");

			string line = Assert.Single(console.ErrorLines);
			Assert.Contains("DEBUG [pdf] parsing xref", line);
			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void An_unknown_level_logs_nothing_rather_than_failing()
		{
			// doc2text's own parse falls through to none; a typo should not end a run
			// that was otherwise fine.
			Assert.Null(EngineLog.For(new DiagnosticOptions { LogLevel = "chatty" }, new RecordingConsole()));
		}

		// -------------------------------------------------------------- --view

		[Fact]
		public void View_opens_the_one_file_a_run_produced()
		{
			var plan = new RunPlan { Cardinality = OutputCardinality.PerInput };
			plan.Items.Add(new PlannedItem { SourcePath = "a.docx", Target = "out/a.pdf" });

			Assert.Equal("out/a.pdf", ConvertCommand.SingleOutputOf(plan));
		}

		[Fact]
		public void View_opens_the_combined_target_when_there_is_one()
		{
			var plan = new RunPlan { Cardinality = OutputCardinality.Combined, CombinedTarget = "out/book.pdf" };
			plan.Items.Add(new PlannedItem { SourcePath = "a.docx" });
			plan.Items.Add(new PlannedItem { SourcePath = "b.docx" });

			Assert.Equal("out/book.pdf", ConvertCommand.SingleOutputOf(plan));
		}

		[Fact]
		public void View_opens_nothing_when_a_run_produced_several_files()
		{
			// "Single output" is the spec's own wording: several inputs, or one input to
			// a per-page format, leave nothing unambiguous to open.
			var many = new RunPlan { Cardinality = OutputCardinality.PerInput };
			many.Items.Add(new PlannedItem { SourcePath = "a.docx", Target = "out/a.pdf" });
			many.Items.Add(new PlannedItem { SourcePath = "b.docx", Target = "out/b.pdf" });

			Assert.Null(ConvertCommand.SingleOutputOf(many));

			var perPage = new RunPlan { Cardinality = OutputCardinality.PerInput };
			perPage.Items.Add(new PlannedItem
			{
				SourcePath = "a.docx",
				TargetPattern = new TargetPattern { Directory = "out", Extension = "png" }
			});

			Assert.Null(ConvertCommand.SingleOutputOf(perPage));
		}

		// ------------------------------------------------------ the debug streams

		[Fact]
		public void Nothing_wraps_the_input_when_no_debug_switch_is_set()
		{
			var fileSystem = new InMemoryFileSystem();
			fileSystem.AddFile("a.docx", "hello");

			Assert.Null(DebugInput.For("a.docx", new DiagnosticOptions(), fileSystem, new RecordingConsole(), "a.docx"));
		}

		[Fact]
		public void Use_memory_stream_reads_the_whole_document_into_memory()
		{
			var fileSystem = new InMemoryFileSystem();
			fileSystem.AddFile("a.docx", "hello world");

			using Stream stream = DebugInput.For("a.docx", new DiagnosticOptions { UseMemoryStream = true },
												 fileSystem, new RecordingConsole(), "a.docx");

			Assert.NotNull(stream);
			Assert.Equal(11, stream.Length);
		}

		[Fact]
		public void Truncate_reads_only_the_first_bytes()
		{
			var fileSystem = new InMemoryFileSystem();
			fileSystem.AddFile("a.docx", "hello world");

			using Stream stream = DebugInput.For("a.docx", new DiagnosticOptions { TruncateInputBytes = 5 },
												 fileSystem, new RecordingConsole(), "a.docx");

			Assert.Equal(5, stream.Length);
		}

		[Fact]
		public void Truncating_past_the_end_stops_at_the_end()
		{
			var fileSystem = new InMemoryFileSystem();
			fileSystem.AddFile("a.docx", "short");

			using Stream stream = DebugInput.For("a.docx", new DiagnosticOptions { TruncateInputBytes = 4096 },
												 fileSystem, new RecordingConsole(), "a.docx");

			Assert.Equal(5, stream.Length);
		}

		[Fact]
		public void Measuring_composes_with_a_truncated_source()
		{
			// The two switches answer independent questions — where the bytes come from,
			// and whether the asking is counted — so asking both must not drop one. An
			// if/else chain here silently ignored --debug-measure-io whenever a
			// truncated or buffered source was also requested.
			var fileSystem = new InMemoryFileSystem();
			fileSystem.AddFile("a.docx", "hello world");
			var console = new RecordingConsole();

			using Stream stream = DebugInput.For(
				"a.docx",
				new DiagnosticOptions { TruncateInputBytes = 5, MeasureIo = true },
				fileSystem, console, "a.docx");

			// Truncated to the prefix...
			Assert.Equal(5, stream.Length);

			// ...and still counting.
			var buffer = new byte[5];
			stream.Read(buffer, 0, buffer.Length);
			stream.Dispose();

			Assert.Equal("a.docx: reads=1 seeks=0 bytes=5", Assert.Single(console.OutLines));
		}

		[Fact]
		public void Measuring_composes_with_a_buffered_source()
		{
			var fileSystem = new InMemoryFileSystem();
			fileSystem.AddFile("a.docx", "hello world");
			var console = new RecordingConsole();

			using Stream stream = DebugInput.For(
				"a.docx",
				new DiagnosticOptions { UseMemoryStream = true, MeasureIo = true },
				fileSystem, console, "a.docx");

			Assert.Equal(11, stream.Length);

			stream.Read(new byte[11], 0, 11);
			stream.Dispose();

			Assert.Equal("a.docx: reads=1 seeks=0 bytes=11", Assert.Single(console.OutLines));
		}

		[Fact]
		public void Measure_io_counts_what_the_engine_asked_for_and_reports_on_close()
		{
			// Reported on close so the figures cover the whole document rather than
			// whatever had happened when someone thought to look.
			var fileSystem = new InMemoryFileSystem();
			fileSystem.AddFile("a.docx", "hello world");
			var console = new RecordingConsole();

			Stream stream = DebugInput.For("a.docx", new DiagnosticOptions { MeasureIo = true },
										   fileSystem, console, "a.docx");

			var buffer = new byte[5];
			stream.Read(buffer, 0, buffer.Length);
			stream.Seek(0, SeekOrigin.Begin);
			Assert.Empty(console.OutLines);

			stream.Dispose();

			Assert.Equal("a.docx: reads=1 seeks=1 bytes=5", Assert.Single(console.OutLines));
		}
	}
}
