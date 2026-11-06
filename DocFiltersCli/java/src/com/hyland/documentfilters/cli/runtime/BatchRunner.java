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

import com.hyland.documentfilters.cli.configuration.BatchOptions;
import com.hyland.documentfilters.cli.configuration.OnErrorBehaviour;
import java.util.List;
import java.util.function.Consumer;

/**
 * Runs one action per input, turning a {@link DocumentError} into a reported
 * failure rather than the end of the run.
 *
 * <p>{@code --on-error stop} ends the batch at the first failure;
 * {@code --on-error skip} (the default) reports it and carries on. Any other
 * {@link CliError} still ends the run, because it is about the run, not the
 * document.
 */
public final class BatchRunner
{
	private final BatchOptions _options;
	private final IConsole _console;
	private final Interruption _interruption;

	private int _successCount;
	private int _failureCount;

	public BatchRunner(BatchOptions options, IConsole console)
	{
		this(options, console, null);
	}

	public BatchRunner(BatchOptions options, IConsole console, Interruption interruption)
	{
		_options = options;
		_console = console;
		_interruption = interruption;
	}

	/**
	 * Runs {@code action} for each item, returning {@link ExitCode#PARTIAL_FAILURE}
	 * when any of them failed and {@link ExitCode#SUCCESS} otherwise.
	 *
	 * @throws InterruptedError the user pressed Ctrl+C between two items.
	 */
	public <T> int run(List<T> items, Consumer<T> action)
	{
		_successCount = 0;
		_failureCount = 0;

		for (T item : items)
		{
			// Between documents, because that is the only point in a batch where
			// stopping leaves nothing half-written.
			if (_interruption != null && _interruption.isRequested())
				throw new InterruptedError();

			try
			{
				action.accept(item);
				_successCount++;
			}
			catch (DocumentError error)
			{
				_failureCount++;
				_console.writeError(text(error.getPath()) + ": " + error.getCode() + ": " + error.getMessage());

				if (_options.onError == OnErrorBehaviour.STOP)
					break;
			}
		}

		return _failureCount > 0 ? ExitCode.PARTIAL_FAILURE : ExitCode.SUCCESS;
	}

	public int getFailureCount()
	{
		return _failureCount;
	}

	public int getSuccessCount()
	{
		return _successCount;
	}

	private static String text(String value)
	{
		return value != null ? value : "";
	}
}
