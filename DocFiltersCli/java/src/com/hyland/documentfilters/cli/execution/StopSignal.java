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

package com.hyland.documentfilters.cli.execution;

import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.DocumentError;
import com.hyland.documentfilters.cli.runtime.InterruptedError;
import com.hyland.documentfilters.cli.runtime.Interruption;

/**
 * The predicate the engine polls during {@code Open}, and the reason it gave when
 * it said to stop.
 *
 * <p>One object rather than a bare {@code BooleanSupplier} because the answer and
 * the reason for it are the same fact, decided at the same instant. The engine
 * can only be told <em>whether</em> to stop --- {@code IGR_CANCELLED} carries
 * nothing else --- so a cause worked out afterwards from the same two conditions
 * is a second guess at a question already answered.
 *
 * <p>Recorded on the first poll that returns true and never revised, so the run
 * reports the condition that actually stopped it.
 */
public final class StopSignal
{
	private final Diagnostics _diagnostics;
	private final Interruption _interruption;
	private StopReason _reason;

	public StopSignal(Diagnostics diagnostics, Interruption interruption)
	{
		_diagnostics = diagnostics;
		_interruption = interruption;
	}

	/**
	 * The reason recorded by the poll that stopped the document, if one did.
	 *
	 * <p>Null after a cancellation means the engine stopped for a reason of its own
	 * and nobody asked it to.
	 */
	public StopReason getReason()
	{
		return _reason;
	}

	/**
	 * What the engine polls. True once, then true forever.
	 *
	 * <p>The interrupt is tested first: if the user pressed Ctrl+C, that is what they
	 * are owed an answer about, whatever else may also have come true.
	 */
	public boolean shouldStop()
	{
		if (_reason != null)
			return true;

		if (_interruption != null && _interruption.isRequested())
			_reason = StopReason.INTERRUPTED;
		else if (_diagnostics.documentDeadlineExceeded())
			_reason = StopReason.TIMED_OUT;

		return _reason != null;
	}

	/** The error for a cancelled open, given what this signal recorded. */
	public CliError cause(String displayName)
	{
		if (_reason == StopReason.INTERRUPTED)
			return new InterruptedError();

		if (_reason == StopReason.TIMED_OUT)
			return _diagnostics.documentTimedOut(displayName);

		return new DocumentError(displayName, "the engine cancelled the operation");
	}
}
