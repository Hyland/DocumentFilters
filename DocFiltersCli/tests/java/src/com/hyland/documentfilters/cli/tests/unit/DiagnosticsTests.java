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

package com.hyland.documentfilters.cli.tests.unit;

import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.configuration.DiagnosticOptions;
import com.hyland.documentfilters.cli.execution.Diagnostics;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.LimitExceededError;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.time.Duration;

/**
 * The per-document timing window, and {@code --timeout} on top of it.
 *
 * <p>Pinned in the order the commands actually call these: {@code beginDocument},
 * work, {@code endDocument}, then {@code checkDocumentTimeout}. That order is the
 * whole point --- {@code endDocument} used to discard the timer, so the check that
 * ran immediately after always saw nothing and {@code --timeout} never fired for
 * any document, however long it took.
 */
public final class DiagnosticsTests
{
	/** A clock a test moves by hand, so a deadline can pass instantly. */
	static final class TestClock
	{
		private Duration _now = Duration.ZERO;

		Duration read()
		{
			return _now;
		}

		void advance(Duration by)
		{
			_now = _now.plus(by);
		}
	}

	private final TestClock _clock = new TestClock();

	/**
	 * Every case here drives the clock rather than sleeping. Asserting that a
	 * deadline passed used to mean waiting for it, which made the assertions both
	 * slow and approximate --- a loaded machine could stretch a five-millisecond
	 * sleep past any limit, and a fast one could leave it short.
	 */
	private Diagnostics forTimeout(Integer seconds)
	{
		DiagnosticOptions options = new DiagnosticOptions();
		options.timeout = seconds;
		return new Diagnostics(options, new RecordingConsole(), null, _clock::read);
	}

	@Test
	public void aDocumentThatRunsPastTheTimeoutIsReportedAfterItEnds()
	{
		Diagnostics diagnostics = forTimeout(30);

		diagnostics.beginDocument("slow.pdf", 1, 1);
		_clock.advance(Duration.ofSeconds(31));
		diagnostics.endDocument("slow.pdf");

		LimitExceededError error = assertThrows(LimitExceededError.class, () -> diagnostics.checkDocumentTimeout("slow.pdf"));

		assertEquals(ErrorCodes.LIMIT_EXCEEDED, error.getCode());
		assertEquals(ExitCode.LIMIT_EXCEEDED, error.getExitCode());
		assertEquals("slow.pdf", error.getPath());
		assertEquals("--timeout of 30s exceeded", error.getMessage());
	}

	@Test
	public void aDocumentInsideTheTimeoutPasses()
	{
		Diagnostics diagnostics = forTimeout(600);

		diagnostics.beginDocument("quick.pdf", 1, 1);
		diagnostics.endDocument("quick.pdf");

		diagnostics.checkDocumentTimeout("quick.pdf");
	}

	@Test
	public void withoutTheOptionNoAmountOfTimeIsAFailure()
	{
		Diagnostics diagnostics = forTimeout(null);

		diagnostics.beginDocument("slow.pdf", 1, 1);
		_clock.advance(Duration.ofHours(1));
		diagnostics.endDocument("slow.pdf");

		diagnostics.checkDocumentTimeout("slow.pdf");
	}

	@Test
	public void theCheckIsHarmlessBeforeAnyDocumentHasBegun()
	{
		forTimeout(0).checkDocumentTimeout("nothing.pdf");
	}

	@Test
	public void eachDocumentIsTimedFromItsOwnStart()
	{
		// The timer outlives endDocument now, so the guard against it carrying over
		// is beginDocument replacing it --- not endDocument clearing it.
		Diagnostics diagnostics = forTimeout(600);

		diagnostics.beginDocument("first.pdf", 1, 2);
		_clock.advance(Duration.ofSeconds(1));
		diagnostics.endDocument("first.pdf");
		diagnostics.checkDocumentTimeout("first.pdf");

		diagnostics.beginDocument("second.pdf", 2, 2);
		diagnostics.endDocument("second.pdf");
		diagnostics.checkDocumentTimeout("second.pdf");
	}

	@Test
	public void logTimeStillReportsTheDocumentsOwnElapsedTime()
	{
		// endDocument does two things; keeping the timer must not cost the other.
		RecordingConsole console = new RecordingConsole();
		DiagnosticOptions options = new DiagnosticOptions();
		options.logTime = true;
		Diagnostics diagnostics = new Diagnostics(options, console);

		diagnostics.beginDocument("a.pdf", 1, 1);
		diagnostics.endDocument("a.pdf");

		assertTrue(console.outLines.stream().anyMatch(line -> line.startsWith("a.pdf: ") && line.endsWith("s")),
			String.valueOf(console.outLines));
	}
}
