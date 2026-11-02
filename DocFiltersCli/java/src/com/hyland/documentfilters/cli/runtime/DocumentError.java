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
 * A single document failed. Collected by {@link BatchRunner} rather than ending
 * the run, unless {@code --on-error stop} is in effect.
 */
public final class DocumentError extends CliError
{
	private static final long serialVersionUID = 1L;

	public DocumentError(String path, String message)
	{
		this(path, message, null);
	}

	/** @param inner the underlying exception, when the failure came from the engine. */
	public DocumentError(String path, String message, Throwable inner)
	{
		super(ErrorCodes.DOCUMENT_FAILED, message, path, inner);
	}

	/** The underlying exception, when the failure came from the engine. */
	public Throwable getInnerError()
	{
		return getCause();
	}

	@Override
	public int getExitCode()
	{
		return ExitCode.PARTIAL_FAILURE;
	}
}
