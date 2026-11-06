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

package com.hyland.documentfilters.cli.runtime;

/**
 * Base class for every error the CLI reports to the user. Thrown from stages 2
 * to 4 and caught once, in {@code Program}.
 *
 * <p>Unchecked, as the C# exception it translates is: it crosses every layer
 * between where a value is found wrong and the one place that reports it, and a
 * checked exception would be in the signature of each.
 */
public abstract class CliError extends RuntimeException
{
	private static final long serialVersionUID = 1L;

	private final String _code;
	private final String _path;

	protected CliError(String code, String message)
	{
		this(code, message, null, null);
	}

	protected CliError(String code, String message, String path, Throwable cause)
	{
		super(message, cause);
		_code = code;
		_path = path;
	}

	/** Stable identifier from {@link ErrorCodes}. */
	public String getCode()
	{
		return _code;
	}

	/**
	 * The input this failure belongs to, or null when it belongs to the run.
	 *
	 * <p>What decides which of the two stderr shapes the failure gets:
	 * {@code path: CODE: message} when an input is named, {@code CODE: message}
	 * when none is. On the base rather than on {@link DocumentError} alone because
	 * a per-document failure is not always a document failure --- {@code --timeout}
	 * applies to one input but exits 6, not 1, so it could never have been a
	 * {@link DocumentError} and would otherwise have had to fold the filename into
	 * its own message to say which input it meant.
	 */
	public String getPath()
	{
		return _path;
	}

	/** The process exit code this error produces. */
	public abstract int getExitCode();
}
