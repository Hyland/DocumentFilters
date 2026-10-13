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

/****************************************************************************
 * Document Filters Example - a command line tool.
 *
 * Convert, extract, list, identify and inspect documents from the shell.
 *
 *   docfilters convert --to pdf --out-dir out/ ./corpus
 *   docfilters extract -i --out-dir dump/ bundle.pst
 *   docfilters identify --json *.bin
 *
 * The same command line is implemented in C#, Python, Java and C++17. See
 * ../README.md for how the pieces fit together and why they are arranged this
 * way; if you are here to learn the API, read the Execution folder and ignore
 * the rest.
 ****************************************************************************/

using Hyland.DocumentFilters.Cli.Cli;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli
{
	/// <summary>
	/// Entry point. Wires up the console and filesystem, runs the command line, and
	/// turns whatever happens into a single exit code.
	/// </summary>
	public static class Program
	{
		public static int Main(string[] args)
		{
			// Ctrl+C is recorded rather than allowed to kill the process, so the run
			// reports and exits 130 through the same path as every other exit code.
			return Run(args, new SystemConsole(PreScanConsoleOptions(args)), new SystemFileSystem(), engine: null,
					   interruption: Interruption.Install());
		}

		/// <summary>
		/// Everything <see cref="Main"/> does once its three dependencies exist.
		/// </summary>
		/// <remarks>
		/// Split out so a test can drive a whole command line — argv in, exit code and
		/// console output out — without the real console, the real filesystem or the
		/// native library. That path is otherwise reachable only by starting a process,
		/// which needs a licensed machine to tell a wiring mistake from an engine one.
		///
		/// <paramref name="engine"/> null means the real engine, built on first use by
		/// whichever command needs it.
		/// </remarks>
		public static int Run(string[] args, IConsole console, IFileSystem fileSystem, IDocumentFiltersEngine engine,
							  Interruption interruption = null, Func<TimeSpan> clock = null, Action<string, IConsole> openViewer = null)
		{
			var app = new CommandLineApp(console, fileSystem, engine, interruption, clock, openViewer);

			try
			{
				return app.Run(args);
			}
			catch (CliError error)
			{
				return ReportError(error, console);
			}
		}

		/// <summary>
		/// Reads <c>--silent</c>, <c>--verbose</c> and <c>--color</c> directly out of
		/// <paramref name="args"/>, before anything is bound.
		/// </summary>
		/// <remarks>
		/// <see cref="SystemConsole"/> needs these <em>before</em> the first line is
		/// written, but they only exist on the <see cref="Configuration.SharedOptions"/>
		/// that <see cref="CommandLineApp.Bind"/> produces, and <see cref="CommandLineApp.Run"/>
		/// starts writing — <c>--print-contract</c>, <c>--dry-run</c>, the command
		/// itself — in the same call that does the binding, with no point in between
		/// for <c>Main</c> to step in. A raw scan sidesteps the ordering problem
		/// entirely: this tool disables prefix abbreviation and none of these three
		/// takes a value, so <c>--silent</c> has exactly one spelling, and a plain
		/// array search for it is exact, not a heuristic. Re-validating them properly
		/// — an unknown argument, a value stuck to <c>--silent</c> — is still
		/// <see cref="CommandLineApp.Bind"/>'s job; this only ever needs to get the
		/// three booleans right for a well-formed command line.
		/// </remarks>
		public static ConsoleOptions PreScanConsoleOptions(string[] args)
		{
			return new ConsoleOptions
			{
				Silent = args.Contains("--silent"),
				Verbose = args.Contains("--verbose"),
				Color = args.Contains("--color")
			};
		}

		/// <summary>
		/// Maps an error to its exit code and prints its message to stderr.
		/// </summary>
		/// <remarks>
		/// Error text is part of the cross-language contract, so it is composed here
		/// from <see cref="CliError.Code"/> and the message rather than being left to
		/// whatever the CLI parsing library would have said. The run-level shape is
		/// code first, e.g.
		/// <c>E_INIT_FAILED: could not initialise Document Filters</c> — code first,
		/// then the message, with no other punctuation.
		/// </remarks>
		public static int ReportError(CliError error, IConsole console)
		{
			console.WriteError(error.Path == null
				? $"{error.Code}: {error.Message}"
				: $"{error.Path}: {error.Code}: {error.Message}");
			return error.ExitCode;
		}
	}
}
