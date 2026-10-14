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

using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Commands
{
	/// <summary>
	/// The one piece every <c>Execute</c> needs and none of them owns:
	/// <see cref="DocumentFiltersSession"/>'s lifetime.
	/// </summary>
	/// <remarks>
	/// A CLI process runs exactly one command, so creating the session at the start
	/// of that one <c>Execute</c> call and disposing it at the end already matches
	/// its documented lifetime — "once for the lifetime of the process" — without
	/// <c>CommandLineApp</c> or <c>Program</c> needing to know about it. A context
	/// hand-wired with a session already — as tests are free to do — is left alone
	/// and not disposed here, since this did not create it.
	/// </remarks>
	public static class CommandExecution
	{
		/// <summary>
		/// A fresh <see cref="StopSignal"/> for one document.
		/// </summary>
		/// <remarks>
		/// Per document, not per run: the signal latches the first reason it sees, and a
		/// deadline belongs to the document it was measured against.
		/// <paramref name="interruption"/> is null for a caller that installed no
		/// handler, which is every test that does not ask for one.
		/// </remarks>
		public static StopSignal WatchForStop(Diagnostics diagnostics, Interruption interruption)
		{
			return new StopSignal(diagnostics, interruption);
		}

		public static int WithSession(CommandContext context, Func<int> action)
		{
			bool ownsSession = context.Session == null;
			if (ownsSession)
				context.Session = DocumentFiltersSession.Create(context.SharedOptions.Library,
														context.SharedOptions.Processing.TempDir);

			try
			{
				return action();
			}
			finally
			{
				if (ownsSession)
				{
					context.Session.Dispose();
					context.Session = null;
				}
			}
		}
	}
}
