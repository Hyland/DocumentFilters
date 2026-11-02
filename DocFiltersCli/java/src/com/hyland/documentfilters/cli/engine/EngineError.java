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
 * Every way a Document Filters call can fail, as the engine reports them.
 *
 * <p>One member per {@code IGR_E_*} constant, with the constant's own value, so a
 * code coming back from the binding maps across without a lookup table and an
 * unrecognised one is still carried faithfully.
 *
 * <p>This list is the axis the engine tests enumerate: for each outcome, what the
 * CLI is supposed to do with it. Most become a {@code DocumentError} against one
 * input and let the batch continue; a few are worth telling apart, and those are
 * the interesting cases.
 */
public enum EngineError
{
	/** No error. Present so the enum can round-trip any code the engine returns. */
	NONE(0),

	/** {@code IGR_E_OPEN_ERROR}: the file could not be opened at all. */
	OPEN_ERROR(1),

	/** {@code IGR_E_WRONG_TYPE}: the call does not apply to this kind of document. */
	WRONG_TYPE(2),

	/** {@code IGR_E_IN_USE}. */
	IN_USE(3),

	/** {@code IGR_E_NOT_READABLE}: recognised, but nothing can be read out of it. */
	NOT_READABLE(4),

	/** {@code IGR_E_PASSWORD}: a password is needed, or the one supplied was wrong. */
	PASSWORD(5),

	/** {@code IGR_E_NOT_FOUND}: no such file, page, subfile or option. */
	NOT_FOUND(10),

	/** {@code IGR_E_WRITE_ERROR}: the output could not be written. */
	WRITE_ERROR(11),

	/** {@code IGR_E_NOT_VALID_FOR_THIS_CLASS}. */
	NOT_VALID_FOR_THIS_CLASS(12),

	/** {@code IGR_E_ERROR}: the general failure the engine falls back on. */
	ERROR(13),

	/** {@code IGR_E_INVALID_HANDLE}: a handle used after it was closed. */
	INVALID_HANDLE(14),

	/** {@code IGR_E_INVALID_POINTER}. */
	INVALID_POINTER(15),

	/** {@code IGR_E_INVALID_PARAMETER}: an option string or flag the engine rejected. */
	INVALID_PARAMETER(16),

	/** {@code IGR_E_FILE_CORRUPT}: the document is damaged. */
	FILE_CORRUPT(17),

	/** {@code IGR_E_OUT_OF_MEMORY}. */
	OUT_OF_MEMORY(20),

	/** {@code IGR_E_BAD_ERROR}. */
	BAD_ERROR(21),

	/** {@code IGR_E_WRONG_THREAD}. */
	WRONG_THREAD(22),

	/** {@code IGR_E_TOO_MANY_HANDLES}: handles leaked, or a batch too large to hold open. */
	TOO_MANY_HANDLES(23),

	/** {@code IGR_E_FONTS_NOT_FOUND}: rendering needs fonts the install does not have. */
	FONTS_NOT_FOUND(24),

	/** {@code IGR_E_VERSION_MISMATCH}: the binding and the native library disagree. */
	VERSION_MISMATCH(25),

	/** {@code IGR_E_ACTION_NOT_SUPPORTED}: this format cannot do what was asked. */
	ACTION_NOT_SUPPORTED(26),

	/**
	 * The open callback asked the engine to stop --- {@code IGR_CANCELLED}.
	 *
	 * <p>Not a failure of the document. This is the only way to interrupt the engine
	 * at all: the callback is raised during open as
	 * {@code IGR_OPEN_CALLBACK_ACTION_HEARTBEAT}, and nothing in the API offers the
	 * same for rendering --- {@code IGR_Render_Page} takes no callback. So a caller
	 * that asks for this already knows why, and translates it into its own error
	 * rather than reporting the document as broken.
	 */
	CANCELLED(27);

	private final int _value;

	EngineError(int value)
	{
		_value = value;
	}

	/** The engine's own number for this failure. */
	public int getValue()
	{
		return _value;
	}
}
