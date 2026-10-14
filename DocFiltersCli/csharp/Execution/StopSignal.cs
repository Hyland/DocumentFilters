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

using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>Why a document was abandoned.</summary>
	public enum StopReason
	{
		/// <summary>The user pressed Ctrl+C.</summary>
		Interrupted,

		/// <summary><c>--timeout</c> elapsed for this document.</summary>
		TimedOut
	}

	/// <summary>
	/// The predicate the engine polls during <c>Open</c>, and the reason it gave when
	/// it said to stop.
	/// </summary>
	/// <remarks>
	/// One object rather than a bare <c>Func&lt;bool&gt;</c> because the answer and
	/// the reason for it are the same fact, decided at the same instant. The engine
	/// can only be told <em>whether</em> to stop — <c>IGR_CANCELLED</c> carries
	/// nothing else — so a cause worked out afterwards from the same two conditions is
	/// a second guess at a question already answered: both can be true by then, either
	/// can have become true after the decision, and neither need still be true at all.
	/// That guess is what made an interrupt during <c>Open</c> report a
	/// <c>--timeout</c> that had never been set, and crash reading it.
	///
	/// Recorded on the first poll that returns true and never revised, so the run
	/// reports the condition that actually stopped it.
	/// </remarks>
	public sealed class StopSignal
	{
		private readonly Diagnostics _diagnostics;
		private readonly Interruption _interruption;

		public StopSignal(Diagnostics diagnostics, Interruption interruption)
		{
			_diagnostics = diagnostics;
			_interruption = interruption;
		}

		/// <summary>The reason recorded by the poll that stopped the document, if one did.</summary>
		/// <remarks>
		/// Null after a cancellation means the engine stopped for a reason of its own
		/// and nobody asked it to — a real possibility, and one the old guesswork could
		/// not distinguish from a limit firing.
		/// </remarks>
		public StopReason? Reason { get; private set; }

		/// <summary>
		/// What the engine polls. True once, then true forever.
		/// </summary>
		/// <remarks>
		/// The interrupt is tested first: if the user pressed Ctrl+C, that is what they
		/// are owed an answer about, whatever else may also have come true. Testing it
		/// here rather than later is the point — only the poll knows which condition was
		/// the one that fired.
		/// </remarks>
		public bool ShouldStop()
		{
			if (Reason.HasValue)
				return true;

			if (_interruption != null && _interruption.Requested)
				Reason = StopReason.Interrupted;
			else if (_diagnostics.DocumentDeadlineExceeded())
				Reason = StopReason.TimedOut;

			return Reason.HasValue;
		}

		/// <summary>
		/// The error for a cancelled open, given what this signal recorded.
		/// </summary>
		public CliError Cause(string displayName)
		{
			if (Reason == StopReason.Interrupted)
				return new InterruptedError();

			if (Reason == StopReason.TimedOut)
				return _diagnostics.DocumentTimedOut(displayName);

			return new DocumentError(displayName, "the engine cancelled the operation");
		}
	}
}
