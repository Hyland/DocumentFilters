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
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Commands
{
	/// <summary>
	/// Everything a command needs that is not its own options. Per-command options
	/// are passed to the command's constructor instead, so there is no object here
	/// holding seven verbs' worth of options with six of them unused.
	/// </summary>
	public sealed class CommandContext
	{
		public SharedOptions SharedOptions { get; set; }

		public IConsole Console { get; set; }

		public IFileSystem FileSystem { get; set; }

		/// <summary>
		/// The initialised engine. Null for commands that do not open documents, and
		/// created lazily so that <c>--print-plan</c> never needs a license.
		/// </summary>
		public DocumentFiltersSession Session { get; set; }

		/// <summary>
		/// Whether the user has interrupted the run, or null when nothing is watching.
		/// </summary>
		/// <remarks>
		/// Null for a caller that installed no handler — every test, and
		/// <c>--dry-run</c>, which finishes long before anyone could press anything.
		/// </remarks>
		public Interruption Interruption { get; set; }

		/// <summary>
		/// A monotonic reading for <see cref="Execution.Diagnostics"/>, or null for the
		/// real one.
		/// </summary>
		/// <remarks>
		/// Here rather than inside each command because the commands build their own
		/// <c>Diagnostics</c> and a test driving the whole CLI has no other way in. It
		/// is what lets the limit cases assert on a document that outran
		/// <c>--timeout</c> without the suite waiting out the smallest value the
		/// validator accepts.
		/// </remarks>
		public Func<TimeSpan> Clock { get; set; }

		/// <summary>
		/// What <c>--view</c> hands the finished output to, or null for the desktop's
		/// own handler.
		/// </summary>
		/// <remarks>
		/// Here for the same reason <see cref="Clock"/> is: a test driving the whole
		/// CLI has no other way in, and what <c>--view</c> has to get right is not what
		/// opens — A7 settled that it is the machine's business — but <em>when</em>. The
		/// output has to be finished before anything is asked to open it, and under
		/// <c>--combine</c> it was not: the canvas and the writer were still alive, so a
		/// PDF whose trailer is written on close was handed over half-written.
		/// </remarks>
		public Action<string, IConsole> OpenViewer { get; set; }
	}

	/// <summary>One verb.</summary>
	public interface ICommand
	{
		/// <summary>The verb as typed, e.g. <c>convert</c>.</summary>
		string Name { get; }

		/// <summary>True when this command opens documents and therefore needs the engine.</summary>
		bool RequiresEngine { get; }

		/// <summary>Runs the command.</summary>
		/// <returns>A value from <see cref="ExitCode"/>.</returns>
		int Execute(CommandContext context);

		/// <summary>
		/// Runs stages 1 to 3 only and returns the canonical plan text, for
		/// <c>--print-plan</c>. Never opens a document.
		/// </summary>
		string DescribePlan(CommandContext context);
	}
}
