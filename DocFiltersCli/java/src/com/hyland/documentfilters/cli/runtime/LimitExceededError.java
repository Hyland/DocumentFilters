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

/** A run limit was exceeded. */
public final class LimitExceededError extends CliError
{
	private static final long serialVersionUID = 1L;

	/** For {@code --max-time} and {@code --max-memory}, which bound the whole run. */
	public LimitExceededError(String message)
	{
		this(message, null);
	}

	/**
	 * @param path the input the limit applied to, for {@code --timeout}; null for
	 *        a limit on the whole run.
	 */
	public LimitExceededError(String message, String path)
	{
		super(ErrorCodes.LIMIT_EXCEEDED, message, path, null);
	}

	@Override
	public int getExitCode()
	{
		return ExitCode.LIMIT_EXCEEDED;
	}
}
