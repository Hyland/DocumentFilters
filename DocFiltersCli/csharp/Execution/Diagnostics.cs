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

using System.Diagnostics;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// Timing, memory reporting, progress and the run limits.
	/// </summary>
	/// <remarks>
	/// Unlike everything else the tool prints, this output is deliberately 
	/// NOT part of the cross-language contract, because elapsed times and 
	/// memory figures differ from run to run. The conformance suite ignores it.
	/// </remarks>
	public sealed class Diagnostics
	{
		private readonly DiagnosticOptions _options;
		private readonly IConsole _console;

		private readonly IDocumentFiltersEngine _engine;

		private readonly Func<TimeSpan> _clock;

		private TimeSpan? _runStart;
		private TimeSpan? _documentStart;
		private TimeSpan? _documentElapsed;
		private Process _proc;

		/// <summary>A monotonic reading shared by the whole process.</summary>
		private static readonly Stopwatch Uptime = Stopwatch.StartNew();

		/// <param name="clock">
		/// A monotonic reading, for tests that need time to pass without waiting for it.
		/// </param>
		/// <remarks>
		/// <paramref name="clock"/> exists because <c>--timeout</c> is otherwise only
		/// testable by sleeping past it, and the smallest value the validator accepts is
		/// one second — so the two cases covering a document that outran its limit cost
		/// more than the entire rest of the suite. A clock the test advances makes them
		/// instant and exact, and removes the flakiness of asserting on elapsed wall
		/// time at all. Every reading below goes through it, so nothing measures time
		/// two different ways.
		/// </remarks>
        public Diagnostics(DiagnosticOptions options, IConsole console, IDocumentFiltersEngine engine = null,
						   Func<TimeSpan> clock = null)
		{
			_options = options;
			_console = console;
			_engine = engine;
			_clock = clock ?? (() => Uptime.Elapsed);
            _proc = Process.GetCurrentProcess();
        }

		/// <summary>Marks the start of the run, for <c>--time</c>.</summary>
		public void BeginRun()
		{
			_runStart = _clock();
		}

		/// <summary>Marks the start of one document, for <c>--logtime</c> and <c>--timeout</c>.</summary>
		public void BeginDocument(string displayName, int index, int total)
		{
			if (_options.Progress)
				_console.WriteLine($"[{index}/{total}] {displayName}");

			_documentStart = _clock();
			_documentElapsed = null;
		}

		/// <summary>
		/// How long the current document has taken: frozen once it has ended, still
		/// running before that.
		/// </summary>
		private TimeSpan CurrentDocumentElapsed()
		{
			if (_documentElapsed.HasValue)
				return _documentElapsed.Value;

			return _documentStart.HasValue ? _clock() - _documentStart.Value : TimeSpan.Zero;
		}

		/// <summary>
		/// Ends the document's timing window, emitting its timing if asked and leaving
		/// the elapsed time readable.
		/// </summary>
		/// <remarks>
		/// Stopped rather than discarded. <see cref="CheckDocumentTimeout"/> runs
		/// <em>after</em> this at every call site, so clearing the timer here made its
		/// null guard always true and <c>--timeout</c> never fired at all — a document
		/// could run for any length of time and still exit 0.
		/// <see cref="BeginDocument"/> replaces the timer for the next document, so
		/// nothing needs clearing here; stopping it also means the elapsed time the
		/// check reads is the document's own, not the document plus whatever ran
		/// between the two calls.
		/// </remarks>
		public void EndDocument(string displayName)
		{
			if (!_documentStart.HasValue)
				return;

			_documentElapsed = _clock() - _documentStart.Value;

			if (_options.LogTime)
				_console.WriteLine($"{displayName}: {_documentElapsed.Value.TotalSeconds:F3}s");

			ReportEngineMemory(atLeast: 2);
		}

		/// <summary>Emits the run summary for <c>--time</c>, and the closing reports.</summary>
		public void EndRun(int succeeded, int failed)
		{
			if (_options.Time && _runStart.HasValue)
				_console.WriteLine($"{succeeded} succeeded, {failed} failed in {(_clock() - _runStart.Value).TotalSeconds:F3}s");

			if (_options.LogMemory)
				_console.WriteLine($"peak memory: {_proc.PeakWorkingSet64 / (1024 * 1024)}MB");

			// -x reports the engine's own view once, at the end; --xx also reports it
			// after every document, which EndDocument does.
			ReportEngineMemory(atLeast: 1);
		}

		/// <summary>
		/// Prints the engine's memory report when <c>-x</c> or <c>--xx</c> asked for it.
		/// </summary>
		/// <remarks>
		/// The two switches are one level: <c>-x</c> is 1 and reports at the end of the
		/// run, <c>--xx</c> is 2 and also reports after each document, which is how
		/// doc2text spells the same pair. The text is the engine's own and is printed
		/// verbatim — it is diagnostics, not contract.
		/// </remarks>
		private void ReportEngineMemory(int atLeast)
		{
			if (_options.MemoryStatusLevel < atLeast || _engine == null)
				return;

			try
			{
				string status = _engine.GetMemoryStatus();
				if (!string.IsNullOrWhiteSpace(status))
					_console.WriteLine(status.TrimEnd());
			}
			catch (EngineException error)
			{
				// A diagnostic must not fail a run that otherwise worked. Reported so
				// the silence is explained rather than mistaken for "no memory in use".
				_console.WriteError($"-x could not read the engine's memory status: {error.Message}");
			}
		}

		/// <summary>
		/// Checks <c>--max-time</c> and <c>--max-memory</c>.
		/// </summary>
		/// <remarks>
		/// <c>--timeout</c> is per document rather than per run and is checked
		/// separately, in <see cref="EndDocument"/>'s caller — a synchronous native call
		/// cannot be pre-empted mid-flight, so this can only notice a document that has
		/// already finished taking too long, not stop one early.
		/// </remarks>
		/// <exception cref="LimitExceededError">A limit was passed.</exception>
		public void CheckLimits()
		{
			if (_options.MaxTime.HasValue && _runStart.HasValue && (_clock() - _runStart.Value).TotalSeconds > _options.MaxTime.Value)
				throw new LimitExceededError($"--max-time of {_options.MaxTime.Value}s exceeded");

			if (_options.MaxMemory.HasValue)
			{
                long usedMegabytes = _proc.PeakWorkingSet64 / (1024 * 1024);
				if (usedMegabytes >= _options.MaxMemory.Value)
					throw new LimitExceededError($"--max-memory of {_options.MaxMemory.Value}MB exceeded");
			}
		}

		/// <summary>
		/// Checks the just-finished document against <c>--timeout</c>.
		/// </summary>
		/// <exception cref="LimitExceededError">The document took longer than allowed.</exception>
		public void CheckDocumentTimeout(string displayName)
		{
			if (DocumentDeadlineExceeded())
				throw DocumentTimedOut(displayName);
		}

		/// <summary>
		/// True once the document being processed has run past <c>--timeout</c>.
		/// </summary>
		/// <remarks>
		/// Polled by the engine's open callback as well as checked after the document,
		/// because those cover different halves and neither covers both. The callback
		/// can stop a document that hangs while the engine parses it, which is the only
		/// point the engine offers; it cannot touch the page loop, because
		/// <c>IGR_Render_Page</c> takes no callback. The check after the document
		/// therefore stays — it is the only thing that sees rendering time at all, even
		/// though by then the work is already done.
		/// </remarks>
		public bool DocumentDeadlineExceeded()
		{
			return _options.Timeout.HasValue
				&& _documentStart.HasValue
				&& CurrentDocumentElapsed().TotalSeconds > _options.Timeout.Value;
		}

		/// <summary>The error for a document that outran <c>--timeout</c>.</summary>
		/// <remarks>
		/// Built here rather than at each throw site so both routes — the cancelled
		/// open and the check afterwards — report the identical line, which
		/// <c>CliTests</c> pins for both.
		/// </remarks>
		public LimitExceededError DocumentTimedOut(string displayName)
		{
			return new LimitExceededError($"--timeout of {_options.Timeout.Value}s exceeded", displayName);
		}

		/// <summary>Waits for a keypress between documents, for <c>--pause</c>.</summary>
		public void PauseIfRequested()
		{
			if (_options.Pause)
				_console.WaitForKey();
		}
	}
}
