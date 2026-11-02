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

package com.hyland.documentfilters.cli.engine;

/**
 * A Document Filters call failed. The one exception type above the seam.
 *
 * <p>Everything the binding can throw is translated into this, so nothing above the
 * seam catches {@code IGRException} --- which is what lets a fake engine reproduce
 * any failure the real one can, and what stops a binding type from leaking into the
 * conversion paths.
 *
 * <p>Deliberately not a {@code CliError}, matching the reference. It carries no exit
 * code of its own because it is not something the user is ever shown: execution
 * catches it and restates it as a {@code DocumentError} against one input, or as an
 * {@code InitialisationError}, or --- for {@link EngineError#CANCELLED} --- as an
 * {@code InterruptedError}. Unchecked, so that it crosses the functional interfaces
 * the execution layer passes around without every one of them declaring it.
 */
public final class EngineException extends RuntimeException
{
	private static final long serialVersionUID = 1L;

	private final EngineError _error;
	private final int _code;

	public EngineException(EngineError error, int code, String message)
	{
		this(error, code, message, null);
	}

	public EngineException(EngineError error, int code, String message, Throwable inner)
	{
		super(message, inner);
		_error = error;
		_code = code;
	}

	/** The failure, named. */
	public EngineError getError()
	{
		return _error;
	}

	/**
	 * The raw {@code IGR_E_*} value, kept even when {@link #getError} could not
	 * name it, so a code this enum does not know about is still reportable.
	 */
	public int getCode()
	{
		return _code;
	}

	/**
	 * Maps a raw engine code onto {@link EngineError}.
	 *
	 * <p>An unknown code becomes {@link EngineError#ERROR} --- the engine's own
	 * catch-all --- rather than throwing, because a newer native library returning a
	 * code this build has never heard of is not a reason to crash.
	 * {@link #getCode} still carries the real value.
	 */
	public static EngineError classify(int code)
	{
		for (EngineError error : EngineError.values())
		{
			if (error.getValue() == code)
				return error;
		}
		return EngineError.ERROR;
	}
}
