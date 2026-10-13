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

namespace Hyland.DocumentFilters.Cli.Runtime
{
	/// <summary>
	/// The per-file loop, in one place, so that <c>--on-error</c> behaves identically
	/// on every command.
	/// </summary>
	public sealed class BatchRunner
	{
		private readonly BatchOptions _options;
		private readonly IConsole _console;

		public BatchRunner(BatchOptions options, IConsole console, Interruption interruption = null)
		{
			_options = options;
			_console = console;
			_interruption = interruption;
		}

		private readonly Interruption _interruption;

		/// <summary>
		/// Runs <paramref name="action"/> for each item.
		/// </summary>
		/// <remarks>
		/// Under <c>--on-error skip</c> a failure is reported and the loop continues,
		/// but the run still ends non-zero. Under <c>--on-error stop</c> the first
		/// failure ends the loop. Errors that are not per-document — a usage error, a
		/// limit being exceeded — propagate rather than being counted.
		///
		/// <b>Under <c>--combine</c> this still runs per input.</b> Skipping a failed
		/// document leaves the shared canvas open and carries on, so the run produces
		/// one document containing everything that converted, and exits 1. A partial
		/// output is the point rather than an accident: exit 1 means "some inputs
		/// failed", not "nothing was written".
		/// </remarks>
		/// <returns>A value from <see cref="ExitCode"/>: success, or partial failure.</returns>
		public int Run<T>(IReadOnlyList<T> items, Action<T> action)
		{
			SuccessCount = 0;
			FailureCount = 0;

			foreach (T item in items)
			{
				// Between documents, because that is the only point in a batch where
				// stopping leaves nothing half-written.
				if (_interruption != null && _interruption.Requested)
					throw new InterruptedError();

				try
				{
					action(item);
					SuccessCount++;
				}
				catch (DocumentError error)
				{
					FailureCount++;
					_console.WriteError($"{error.Path}: {error.Code}: {error.Message}");

					if (_options.OnError == OnErrorBehaviour.Stop)
						break;
				}
			}

			return FailureCount > 0 ? ExitCode.PartialFailure : ExitCode.Success;
		}

		/// <summary>How many items failed in the last run.</summary>
		public int FailureCount { get; private set; }

		/// <summary>How many items succeeded in the last run.</summary>
		public int SuccessCount { get; private set; }
	}
}
