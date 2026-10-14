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

namespace Hyland.DocumentFilters.Cli.Runtime
{
	/// <summary>
	/// Whether the user has asked the run to stop — Ctrl+C.
	/// </summary>
	/// <remarks>
	/// An object rather than a static flag so a test can raise one without touching
	/// the process, and so two runs in the same process cannot see each other's.
	///
	/// Read in two places, which between them are every point the run can be stopped
	/// at all: <see cref="BatchRunner"/> checks it between documents, and the engine's
	/// open callback polls it so a document that hangs while parsing does not have to
	/// finish first. Nothing can interrupt a page loop — <c>IGR_Render_Page</c> takes
	/// no callback — so a long render still runs to completion, the same limitation
	/// <c>--timeout</c> has.
	/// </remarks>
	public sealed class Interruption
	{
		private volatile bool _requested;

		/// <summary>True once the user has interrupted the run.</summary>
		/// <remarks>
		/// Volatile because the handler runs on another thread: .NET raises
		/// <c>Console.CancelKeyPress</c> on a thread-pool thread, not the one doing the
		/// work.
		/// </remarks>
		public bool Requested => _requested;

		/// <summary>Records that the user asked to stop.</summary>
		public void Request()
		{
			_requested = true;
		}

		/// <summary>
		/// Installs a Ctrl+C handler that records the interrupt instead of killing the
		/// process, and returns the object the run should read.
		/// </summary>
		/// <remarks>
		/// <c>Cancel = true</c> is what stops the runtime terminating immediately, which
		/// is the whole point: the run gets to finish what it is holding, report, and
		/// exit 130 through the same path as every other exit code rather than being
		/// killed with whatever the shell decides.
		/// </remarks>
		public static Interruption Install()
		{
			var interruption = new Interruption();

			Console.CancelKeyPress += (sender, args) =>
			{
				args.Cancel = true;
				interruption.Request();
			};

			return interruption;
		}
	}

	/// <summary>The run was interrupted by the user.</summary>
	/// <remarks>
	/// Not a failure of any document, so it names none — the same shape as a run-level
	/// limit.
	/// </remarks>
	public sealed class InterruptedError : CliError
	{
		public InterruptedError() : base(ErrorCodes.Interrupted, "interrupted") { }

		public override int ExitCode => Runtime.ExitCode.Interrupted;
	}
}
