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

import com.hyland.documentfilters.cli.configuration.DiagnosticOptions;
import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.engine.EngineException;
import com.hyland.documentfilters.cli.engine.IDocumentFiltersEngine;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.LimitExceededError;
import java.time.Duration;
import java.util.Locale;
import java.util.function.Supplier;

/**
 * Timing, memory reporting, progress and the run limits.
 *
 * <p>Unlike everything else the tool prints, this output is deliberately NOT part
 * of the cross-language contract, because elapsed times and memory figures differ
 * from run to run. The conformance suite ignores it.
 */
public final class Diagnostics
{
	/** A monotonic reading shared by the whole process. */
	private static final long UPTIME_ORIGIN = System.nanoTime();

	private final DiagnosticOptions _options;
	private final IConsole _console;
	private final IDocumentFiltersEngine _engine;
	private final Supplier<Duration> _clock;

	private Duration _runStart;
	private Duration _documentStart;
	private Duration _documentElapsed;

	public Diagnostics(DiagnosticOptions options, IConsole console)
	{
		this(options, console, null, null);
	}

	/**
	 * @param clock a monotonic reading, for tests that need time to pass without
	 *        waiting for it; null for the real one. {@code --timeout} is otherwise
	 *        only testable by sleeping past it.
	 */
	public Diagnostics(DiagnosticOptions options, IConsole console, IDocumentFiltersEngine engine, Supplier<Duration> clock)
	{
		_options = options;
		_console = console;
		_engine = engine;
		_clock = clock != null ? clock : () -> Duration.ofNanos(System.nanoTime() - UPTIME_ORIGIN);
	}

	/** Marks the start of the run, for {@code --time}. */
	public void beginRun()
	{
		_runStart = _clock.get();
	}

	/** Marks the start of one document, for {@code --logtime} and {@code --timeout}. */
	public void beginDocument(String displayName, int index, int total)
	{
		if (_options.progress)
			_console.writeLine("[" + index + "/" + total + "] " + displayName);

		_documentStart = _clock.get();
		_documentElapsed = null;
	}

	/** How long the current document has taken: frozen once it has ended, still running before that. */
	private Duration currentDocumentElapsed()
	{
		if (_documentElapsed != null)
			return _documentElapsed;

		return _documentStart != null ? _clock.get().minus(_documentStart) : Duration.ZERO;
	}

	/**
	 * Ends the document's timing window, emitting its timing if asked and leaving
	 * the elapsed time readable.
	 *
	 * <p>Stopped rather than discarded: {@link #checkDocumentTimeout} runs after this
	 * at every call site and reads the frozen value.
	 */
	public void endDocument(String displayName)
	{
		if (_documentStart == null)
			return;

		_documentElapsed = _clock.get().minus(_documentStart);

		if (_options.logTime)
			_console.writeLine(displayName + ": " + seconds(_documentElapsed) + "s");

		reportEngineMemory(2);
	}

	/** Emits the run summary for {@code --time}, and the closing reports. */
	public void endRun(int succeeded, int failed)
	{
		if (_options.time && _runStart != null)
			_console.writeLine(succeeded + " succeeded, " + failed + " failed in " + seconds(_clock.get().minus(_runStart)) + "s");

		if (_options.logMemory)
			_console.writeLine("peak memory: " + PeakMemory.bytes() / (1024 * 1024) + "MB");

		// -x reports the engine's own view once, at the end; --xx also reports it
		// after every document, which endDocument does.
		reportEngineMemory(1);
	}

	/**
	 * Prints the engine's memory report when {@code -x} or {@code --xx} asked for it.
	 *
	 * <p>{@code -x} is 1 and reports at the end of the run, {@code --xx} is 2 and also
	 * reports after each document. The text is the engine's own and is printed
	 * verbatim --- it is diagnostics, not contract.
	 */
	private void reportEngineMemory(int atLeast)
	{
		if (_options.memoryStatusLevel < atLeast || _engine == null)
			return;

		try
		{
			String status = _engine.getMemoryStatus();
			if (!ValueParsers.isNullOrWhiteSpace(status))
				_console.writeLine(trimEnd(status));
		}
		catch (EngineException error)
		{
			// A diagnostic must not fail a run that otherwise worked. Reported so
			// the silence is explained rather than mistaken for "no memory in use".
			_console.writeError("-x could not read the engine's memory status: " + error.getMessage());
		}
	}

	/**
	 * Checks {@code --max-time} and {@code --max-memory}.
	 *
	 * <p>{@code --timeout} is per document rather than per run and is checked
	 * separately: a synchronous native call cannot be pre-empted mid-flight.
	 *
	 * @throws LimitExceededError a limit was passed.
	 */
	public void checkLimits()
	{
		if (_options.maxTime != null && _runStart != null && toSeconds(_clock.get().minus(_runStart)) > _options.maxTime)
			throw new LimitExceededError("--max-time of " + _options.maxTime + "s exceeded");

		if (_options.maxMemory != null)
		{
			long usedMegabytes = PeakMemory.bytes() / (1024 * 1024);
			if (usedMegabytes >= _options.maxMemory)
				throw new LimitExceededError("--max-memory of " + _options.maxMemory + "MB exceeded");
		}
	}

	/**
	 * Checks the just-finished document against {@code --timeout}.
	 *
	 * @throws LimitExceededError the document took longer than allowed.
	 */
	public void checkDocumentTimeout(String displayName)
	{
		if (documentDeadlineExceeded())
			throw documentTimedOut(displayName);
	}

	/**
	 * True once the document being processed has run past {@code --timeout}.
	 *
	 * <p>Polled by the engine's open callback as well as checked after the document,
	 * because those cover different halves and neither covers both: the callback can
	 * stop a document that hangs while the engine parses it, and only the check
	 * afterwards sees rendering time at all.
	 */
	public boolean documentDeadlineExceeded()
	{
		return _options.timeout != null
			&& _documentStart != null
			&& toSeconds(currentDocumentElapsed()) > _options.timeout;
	}

	/** The error for a document that outran {@code --timeout}, identical from both routes. */
	public LimitExceededError documentTimedOut(String displayName)
	{
		return new LimitExceededError("--timeout of " + _options.timeout + "s exceeded", displayName);
	}

	/** Waits for a keypress between documents, for {@code --pause}. */
	public void pauseIfRequested()
	{
		if (_options.pause)
			_console.waitForKey();
	}

	private static double toSeconds(Duration duration)
	{
		return duration.toNanos() / 1e9;
	}

	/** Three decimals with a dot whatever the locale, as the reference's {@code F3} under the invariant culture. */
	private static String seconds(Duration duration)
	{
		return String.format(Locale.ROOT, "%.3f", toSeconds(duration));
	}

	private static String trimEnd(String text)
	{
		int end = text.length();
		while (end > 0 && ValueParsers.isWhiteSpace(text.charAt(end - 1)))
			--end;
		return text.substring(0, end);
	}
}
