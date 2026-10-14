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

using System;
using System.Collections.Generic;
using System.Linq;
using Hyland.DocumentFilters.Cli.Cli;
using Hyland.DocumentFilters.Cli.Commands;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// What the CLI does with each way Document Filters can fail.
	/// </summary>
	/// <remarks>
	/// The engine is the one dependency the execution paths could not be tested
	/// around: it needs a license, a native library, and a document that actually
	/// exhibits the outcome under test. Behind <see cref="IDocumentFiltersEngine"/>
	/// every outcome is scriptable, so the question these tests ask is the one that
	/// matters — not "does the engine do this", which is Hyland's to answer, but
	/// "when it does, does the CLI behave".
	///
	/// The codes are not invented: <see cref="EngineError"/> has one member per
	/// <c>IGR_E_*</c> constant in <c>ISYS11dfConstants</c>, carrying the constant's
	/// own value.
	/// </remarks>
	public class EngineOutcomeTests
	{
		/// <summary>Every failure the engine can report, as the axis of a theory.</summary>
		/// <summary>
		/// Every failure the engine can report of its own accord.
		/// </summary>
		/// <remarks>
		/// <see cref="EngineError.None"/> is not a failure, and
		/// <see cref="EngineError.Cancelled"/> is not the engine's: it is only ever
		/// raised because this tool asked, through the open callback, so the caller
		/// that asked translates it into its own error — a <c>--timeout</c> reported as
		/// <c>E_LIMIT_EXCEEDED</c> rather than a document reported as broken. Sweeping
		/// it here would assert the opposite.
		/// </remarks>
		public static IEnumerable<object[]> EveryEngineError()
		{
			foreach (EngineError error in Enum.GetValues(typeof(EngineError)).Cast<EngineError>())
			{
				if (error != EngineError.None && error != EngineError.Cancelled)
					yield return new object[] { error };
			}
		}

		/// <remarks>
		/// The inputs have to exist in two places: on the fake filesystem, because the
		/// walk selects them before the engine is ever asked, and in the fake engine,
		/// because that is what opening them consults. A path present in only one of
		/// the two is the setup mistake this helper exists to prevent.
		/// </remarks>
		private static CommandContext Context(FakeDocumentFiltersEngine engine, RecordingConsole console, SharedOptions shared)
		{
			var fileSystem = new InMemoryFileSystem();
			foreach (string path in shared.Input.Paths)
				fileSystem.AddFile(path);

			return new CommandContext
			{
				SharedOptions = shared,
				Console = console,
				FileSystem = fileSystem,
				Session = DocumentFiltersSession.For(engine)
			};
		}

		private static SharedOptions Shared(string outDir = "out")
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = outDir;
			return shared;
		}

		// ============================================ opening a document

		/// <summary>
		/// Ctrl+C arriving while a document is opening exits 130, not a timeout error.
		/// </summary>
		/// <remarks>
		/// Both causes reach the engine through one predicate and come back as one
		/// <c>EngineError.Cancelled</c>, and every command used to translate that into
		/// <c>DocumentTimedOut</c> unconditionally — which reads <c>Timeout.Value</c>.
		/// With no <c>--timeout</c> given there is no value, so an interrupt during
		/// <c>Open</c> crashed on a nullable rather than reporting anything. The cause
		/// has to be recovered at the catch, because the engine cannot carry it.
		///
		/// The interrupt is requested from inside the open: <c>BatchRunner</c> checks
		/// between documents, so one requested any earlier stops the run before
		/// anything opens and never reaches this path at all.
		/// </remarks>
		[Fact]
		public void An_interrupt_while_a_document_opens_exits_130_and_does_not_report_a_timeout()
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.AddDocument("a.docx", "body");
			var console = new RecordingConsole();
			var interruption = new Interruption();
			engine.DuringOpen = () => interruption.Request();

			SharedOptions shared = Shared();
			shared.Input.Paths.Add("a.docx");

			CommandContext ctx = Context(engine, console, shared);
			ctx.Interruption = interruption;

			var command = new ConvertCommand(new ConvertOptions
			{
				Format = Hyland.DocumentFilters.Cli.Planning.FormatCatalog.Resolve("text")
			});

			// Thrown, not returned: InterruptedError travels past BatchRunner — which
			// only absorbs DocumentError — and Program turns it into the exit code. What
			// matters here is which error it is, and that it is a CliError at all rather
			// than the nullable dereference that used to end the process.
			CliError error = Assert.ThrowsAny<CliError>(() => command.Execute(ctx));

			Assert.IsType<InterruptedError>(error);
			Assert.Equal(ExitCode.Interrupted, error.ExitCode);
			Assert.DoesNotContain("--timeout", error.Message);
		}


		/// <summary>
		/// Whatever the engine says went wrong, one bad input is one failed input.
		/// </summary>
		/// <remarks>
		/// This is the property the whole batch model rests on: an engine failure has
		/// to arrive as a <see cref="DocumentError"/> naming the input, not as an
		/// exception that ends the process. An <c>EngineException</c> escaping a
		/// command would bypass <c>BatchRunner</c> entirely — <c>--on-error skip</c>
		/// would stop skipping, and the exit code would be whatever <c>Program</c>
		/// makes of an unhandled exception rather than 1.
		/// </remarks>
		[Theory]
		[MemberData(nameof(EveryEngineError))]
		public void Any_open_failure_becomes_one_failed_input(EngineError error)
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.AddDocument("good.docx", "hello");
			engine.FailToOpen("bad.docx", error);

			var console = new RecordingConsole();
			int exitCode = RunConvert(engine, console, "good.docx", "bad.docx");

			Assert.Equal(ExitCode.PartialFailure, exitCode);
			Assert.Contains(console.ErrorLines, line => line.Contains("bad.docx"));
			Assert.DoesNotContain(console.ErrorLines, line => line.Contains("good.docx"));
		}

		[Theory]
		[MemberData(nameof(EveryEngineError))]
		public void Any_open_failure_stops_the_run_under_fail_fast(EngineError error)
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.FailToOpen("first.docx", error);
			engine.AddDocument("second.docx", "hello");

			var console = new RecordingConsole();
			SharedOptions shared = Shared();
			shared.Batch.OnError = OnErrorBehaviour.Stop;

			int exitCode = RunConvert(engine, console, shared, "first.docx", "second.docx");

			Assert.Equal(ExitCode.PartialFailure, exitCode);

			// The second document is never opened, which is the whole point of stopping.
			Assert.DoesNotContain(engine.Calls, call => call.Contains("second.docx"));
		}

		[Fact]
		public void A_document_that_opens_is_converted_and_closed()
		{
			var engine = new FakeDocumentFiltersEngine();
			FakeDocument document = engine.AddDocument("a.docx", "the text");

			int exitCode = RunConvert(engine, new RecordingConsole(), "a.docx");

			Assert.Equal(ExitCode.Success, exitCode);
			Assert.True(document.WasOpened);
			Assert.True(document.IsDisposed);
		}

		// ============================================ passwords

		[Fact]
		public void The_right_password_opens_a_protected_document()
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.AddProtectedDocument("locked.docx", "hunter2", "secret text");

			SharedOptions shared = Shared();
			shared.Passwords.Passwords.Add("hunter2");

			Assert.Equal(ExitCode.Success, RunConvert(engine, new RecordingConsole(), shared, "locked.docx"));
		}

		[Fact]
		public void A_wrong_password_fails_that_document_only()
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.AddProtectedDocument("locked.docx", "hunter2", "secret");
			engine.AddDocument("open.docx", "plain");

			SharedOptions shared = Shared();
			shared.Passwords.Passwords.Add("wrong");

			var console = new RecordingConsole();
			int exitCode = RunConvert(engine, console, shared, "locked.docx", "open.docx");

			Assert.Equal(ExitCode.PartialFailure, exitCode);
			Assert.Contains(console.ErrorLines, line => line.Contains("locked.docx"));
		}

		[Fact]
		public void No_password_at_all_fails_the_same_way_a_wrong_one_does()
		{
			// The engine reports both as IGR_E_PASSWORD, so the CLI cannot tell them
			// apart and must not pretend to.
			var engine = new FakeDocumentFiltersEngine();
			engine.AddProtectedDocument("locked.docx", "hunter2", "secret");

			var console = new RecordingConsole();
			int exitCode = RunConvert(engine, console, Shared(), "locked.docx");

			Assert.Equal(ExitCode.PartialFailure, exitCode);
			Assert.Contains(console.ErrorLines, line => line.Contains("locked.docx"));
		}

		// ============================================ failures after opening

		/// <summary>
		/// A failure part-way through reading is still one failed input, not a crash.
		/// </summary>
		/// <remarks>
		/// Worth separating from an open failure because the document is open when it
		/// happens: the handle has to be closed on the way out, and the batch has to
		/// carry on. A corrupt document that opens and then fails on read is the
		/// common shape of it.
		/// </remarks>
		[Theory]
		[MemberData(nameof(EveryEngineError))]
		public void A_read_failure_after_opening_is_still_one_failed_input(EngineError error)
		{
			var engine = new FakeDocumentFiltersEngine();
			FakeDocument broken = engine.AddDocument("broken.docx", "some text");
			broken.ReadFailure = error;
			engine.AddDocument("fine.docx", "text");

			var console = new RecordingConsole();
			int exitCode = RunConvert(engine, console, "broken.docx", "fine.docx");

			Assert.Equal(ExitCode.PartialFailure, exitCode);
			Assert.Contains(console.ErrorLines, line => line.Contains("broken.docx"));
			Assert.True(broken.IsDisposed, "the document must be closed even when reading it failed");
		}

		// ============================================ identify

		[Theory]
		[MemberData(nameof(EveryEngineError))]
		public void Identify_reports_a_failure_per_input_and_keeps_going(EngineError error)
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.FailToOpen("bad.bin", error);
			FakeDocument good = engine.AddDocument("good.bin");
			good.FileType = 254;
			good.MimeType = "application/vnd.ms-word";
			good.FormatName = "Microsoft Word";

			var console = new RecordingConsole();
			var command = new IdentifyCommand(new IdentifyOptions());
			SharedOptions shared = Shared(outDir: null);
			shared.Input.Paths.Add("bad.bin");
			shared.Input.Paths.Add("good.bin");

			int exitCode = command.Execute(Context(engine, console, shared));

			Assert.Equal(ExitCode.PartialFailure, exitCode);
			Assert.Contains(console.OutLines, line => line.Contains("good.bin") && line.Contains("format=254"));
		}

		[Fact]
		public void Identify_never_opens_the_document()
		{
			// identify asks what a file is, not what it says. Opening it would need a
			// password for a protected document and would read bytes it has no use for.
			var engine = new FakeDocumentFiltersEngine();
			engine.AddProtectedDocument("locked.docx", "hunter2");

			var console = new RecordingConsole();
			SharedOptions shared = Shared(outDir: null);
			shared.Input.Paths.Add("locked.docx");

			int exitCode = new IdentifyCommand(new IdentifyOptions()).Execute(Context(engine, console, shared));

			Assert.Equal(ExitCode.Success, exitCode);
			Assert.Contains(engine.Calls, call => call.StartsWith("IdentifyFile(", StringComparison.Ordinal));
			Assert.DoesNotContain(engine.Calls, call => call.StartsWith("OpenDocument(", StringComparison.Ordinal));
		}

		// ============================================ initialisation

		[Theory]
		[MemberData(nameof(EveryEngineError))]
		public void An_initialisation_failure_is_not_a_document_failure(EngineError error)
		{
			// Nothing was processed, so this is the run never starting: exit 5, not the
			// partial-failure 1 that means "some files did not work".
			var engine = new FakeDocumentFiltersEngine { InitialisationFailure = error };

			InitialisationError thrown = Assert.Throws<InitialisationError>(
				() => DocumentFiltersSessionFactory(engine));

			Assert.Equal(ExitCode.InitialisationFailure, thrown.ExitCode);
			Assert.Equal(ErrorCodes.InitFailed, thrown.Code);
		}

		/// <summary>Mirrors what <c>DocumentFiltersSession.Create</c> does, against the fake.</summary>
		private static DocumentFiltersSession DocumentFiltersSessionFactory(FakeDocumentFiltersEngine engine)
		{
			try
			{
				engine.Initialize(null, ".");
			}
			catch (EngineException error)
			{
				throw new InitialisationError("could not initialise Document Filters: " + error.Message);
			}

			return DocumentFiltersSession.For(engine);
		}

		// ============================================ helpers

		private static int RunConvert(FakeDocumentFiltersEngine engine, RecordingConsole console, params string[] paths)
		{
			return RunConvert(engine, console, Shared(), paths);
		}

		private static int RunConvert(FakeDocumentFiltersEngine engine, RecordingConsole console, SharedOptions shared, params string[] paths)
		{
			foreach (string path in paths)
				shared.Input.Paths.Add(path);

			var options = new ConvertOptions { Format = Planning.FormatCatalog.Resolve("text") };
			var command = new ConvertCommand(options);

			return command.Execute(Context(engine, console, shared));
		}
	}
}
