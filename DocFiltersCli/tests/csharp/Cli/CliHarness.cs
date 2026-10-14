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

using System.Collections.Generic;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;

namespace Hyland.DocumentFilters.Cli.Tests.Cli
{
	/// <summary>
	/// Runs a whole command line the way the shell does, in process.
	/// </summary>
	/// <remarks>
	/// <c>argv</c> in; exit code, stdout and stderr out. Everything between is the
	/// real thing — the real parser, the real validator, the real walk, the real
	/// planner, the real commands, the real error-to-exit-code mapping — with only
	/// the three edges swapped: the console, the filesystem and the engine.
	///
	/// This covers the one seam the conformance corpus cannot reach. The corpus stops
	/// at <c>Bind</c> and <c>DescribePlan</c>; everything above that —
	/// <c>Program.Run</c>, <c>CommandLineApp.Run</c>'s dispatch,
	/// <c>--print-contract</c>, the dry-run branch, the session's lifetime, and
	/// <c>ReportError</c>'s stderr format — had no test at all before this.
	///
	/// No license and no native library: the engine is a fake, so these run wherever
	/// the unit tests run. What they deliberately do not prove is that the real
	/// engine behaves as the fake claims — that is tier 3's job.
	/// </remarks>
	public sealed class CliHarness
	{
		private readonly InMemoryFileSystem _fileSystem = new InMemoryFileSystem();

		/// <summary>The fake disk, so a test can assert what the run left on it.</summary>
		public InMemoryFileSystem FileSystem => _fileSystem;

		/// <summary>The engine the run sees. Script documents and failures on it before running.</summary>
		public FakeDocumentFiltersEngine Engine { get; } = new FakeDocumentFiltersEngine();

		/// <summary>Everything the run wrote, kept after <see cref="Run"/> returns.</summary>
		public RecordingConsole Console { get; } = new RecordingConsole();

		public int ExitCode { get; private set; }

		public IReadOnlyList<string> Out => Console.OutLines;

		/// <summary>
		/// Everything written to stdout, verbatim.
		/// </summary>
		/// <remarks>
		/// <see cref="Out"/> only records whole lines, so it misses output written
		/// without one — the JSON reports build a document and write it in a single
		/// call, which is exactly the output whose exact shape is worth asserting.
		/// </remarks>
		public string Output => Console.Output;

		public IReadOnlyList<string> Errors => Console.ErrorLines;

		/// <summary>Adds an input that exists on disk and opens in the engine.</summary>
		/// <remarks>
		/// Both halves, always. A path in only one of the two is the setup mistake that
		/// otherwise shows up as a walk selecting nothing, or an open failing for a file
		/// the walk did find — neither of which is what the test meant to say.
		/// </remarks>
		public CliHarness WithDocument(string path, string text = "")
		{
			_fileSystem.AddFile(path);
			Engine.AddDocument(path, text);
			return this;
		}

		/// <summary>Adds a file on disk that the engine will refuse to open.</summary>
		public CliHarness WithUnreadableDocument(string path, EngineError error)
		{
			_fileSystem.AddFile(path);
			Engine.FailToOpen(path, error);
			return this;
		}

		/// <summary>Adds a file the engine opens only with the right password.</summary>
		public CliHarness WithProtectedDocument(string path, string password, string text = "")
		{
			_fileSystem.AddFile(path);
			Engine.AddProtectedDocument(path, password, text);
			return this;
		}

		/// <summary>Adds a file on disk that the engine has never heard of.</summary>
		/// <remarks>
		/// The walk finds it and the open fails — which is what a file of a format this
		/// build cannot read looks like from the outside.
		/// </remarks>
		public CliHarness WithUnopenableFile(string path)
		{
			_fileSystem.AddFile(path);
			return this;
		}

		/// <summary>Puts a file where an output would go, for the overwrite rules.</summary>
		public CliHarness WithExistingOutput(string path)
		{
			_fileSystem.AddFile(path, "already here");
			return this;
		}

		/// <summary>Runs the command line and returns the exit code.</summary>
		/// <summary>
		/// Builds a harness whose runs watch <paramref name="interruption"/>.
		/// </summary>
		public CliHarness(Interruption interruption)
		{
			_interruption = interruption;
		}

		/// <summary>The default: nothing is watching for an interrupt.</summary>
		public CliHarness() { }

		private readonly Interruption _interruption;

		/// <summary>
		/// A clock the test moves by hand, so a run can outlast <c>--timeout</c> without
		/// the suite waiting.
		/// </summary>
		public TestClock Clock { get; } = new TestClock();

		/// <summary>
		/// What <c>--view</c> was handed, and what was in it at that moment.
		/// </summary>
		/// <remarks>
		/// The content is read when the viewer is called rather than afterwards,
		/// because the question <c>--view</c> raises is one of timing: a combined
		/// output is not a finished file until its canvas or its writer is closed, and
		/// reading it after the run would look the same either way.
		/// </remarks>
		public List<string> Viewed { get; } = new List<string>();

		public List<string> ViewedContent { get; } = new List<string>();

		public int Run(params string[] args)
		{
			ExitCode = Program.Run(args, Console, _fileSystem, Engine, _interruption, Clock.Read, RecordView);
			return ExitCode;
		}

		private void RecordView(string path, IConsole console)
		{
			Viewed.Add(path);
			ViewedContent.Add(_fileSystem.FileExists(path) ? _fileSystem.ReadText(path) : null);
		}

		/// <summary>The files the run created, for asserting on what it wrote.</summary>
		public IReadOnlyList<string> Created => _fileSystem.Created;

		/// <summary>A monotonic reading a test advances itself.</summary>
		/// <remarks>
		/// Starts at zero and only moves when a test says so, so a run takes no real
		/// time however long it claims to have taken. Every harness run uses one, which
		/// also means timings printed by <c>--time</c> are reproducible rather than
		/// whatever the machine happened to do.
		/// </remarks>
		public sealed class TestClock
		{
			public TimeSpan Now { get; private set; }

			public TimeSpan Read() => Now;

			public void Advance(TimeSpan by) => Now += by;
		}
	}
}
