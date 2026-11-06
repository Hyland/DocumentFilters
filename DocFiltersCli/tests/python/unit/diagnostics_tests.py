"""
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
"""

import unittest
from typing import Optional

from docfilters_cli.configuration.shared_options import DiagnosticOptions
from docfilters_cli.execution.diagnostics import Diagnostics
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import ErrorCodes, LimitExceededError
from docfilters_cli.runtime.i_console import IConsole
from cli.cli_harness import TestClock
from fakes.recording_console import RecordingConsole


class DiagnosticsTests(unittest.TestCase):
    """
    The per-document timing window, and --timeout on top of it.

    Pinned in the order the commands actually call these: begin_document, work,
    end_document, then check_document_timeout. That order is the whole point --
    end_document used to discard the timer, so the check that ran immediately after
    always saw nothing and --timeout never fired for any document, however long it took.
    """

    def setUp(self):
        self._clock = TestClock()

    def _for_timeout(self, seconds: Optional[int], console: Optional[IConsole] = None) -> Diagnostics:
        # `console ?? new RecordingConsole()` is an is-not-None test, not `or`: C#'s ??
        # falls through on null alone, and a console object is always truthy anyway, but
        # writing it as `or` here would be the habit that breaks the next one of these.
        options = DiagnosticOptions()
        options.timeout = seconds

        return Diagnostics(options, console if console is not None else RecordingConsole(),
                           engine=None, clock=self._clock.read)

    def test_a_document_that_runs_past_the_timeout_is_reported_after_it_ends(self):
        diagnostics = self._for_timeout(seconds=30)

        diagnostics.begin_document("slow.pdf", 1, 1)
        self._clock.advance(31)
        diagnostics.end_document("slow.pdf")

        # Assert.Throws<T> requires the exact type; assertRaises accepts a subclass.
        # Nothing derives from LimitExceededError, so the two are the same assertion
        # today -- and there is no unittest form that insists on the exact class.
        with self.assertRaises(LimitExceededError) as raised:
            diagnostics.check_document_timeout("slow.pdf")

        error = raised.exception

        self.assertEqual(ErrorCodes.LIMIT_EXCEEDED, error.code)
        self.assertEqual(exit_code.LIMIT_EXCEEDED, error.exit_code)
        self.assertEqual("slow.pdf", error.path)
        self.assertEqual("--timeout of 30s exceeded", error.message)

    def test_a_document_inside_the_timeout_passes(self):
        diagnostics = self._for_timeout(seconds=600)

        diagnostics.begin_document("quick.pdf", 1, 1)
        diagnostics.end_document("quick.pdf")

        diagnostics.check_document_timeout("quick.pdf")

    def test_without_the_option_no_amount_of_time_is_a_failure(self):
        diagnostics = self._for_timeout(seconds=None)

        diagnostics.begin_document("slow.pdf", 1, 1)
        self._clock.advance(60 * 60)
        diagnostics.end_document("slow.pdf")

        diagnostics.check_document_timeout("slow.pdf")

    def test_the_check_is_harmless_before_any_document_has_begun(self):
        self._for_timeout(seconds=0).check_document_timeout("nothing.pdf")

    def test_each_document_is_timed_from_its_own_start(self):
        # The timer outlives end_document now, so the guard against it carrying over
        # is begin_document replacing it -- not end_document clearing it.
        diagnostics = self._for_timeout(seconds=600)

        diagnostics.begin_document("first.pdf", 1, 2)
        self._clock.advance(1)
        diagnostics.end_document("first.pdf")
        diagnostics.check_document_timeout("first.pdf")

        diagnostics.begin_document("second.pdf", 2, 2)
        diagnostics.end_document("second.pdf")
        diagnostics.check_document_timeout("second.pdf")

    def test_log_time_still_reports_the_documents_own_elapsed_time(self):
        # end_document does two things; keeping the timer must not cost the other.
        console = RecordingConsole()
        options = DiagnosticOptions()
        options.log_time = True
        diagnostics = Diagnostics(options, console)

        diagnostics.begin_document("a.pdf", 1, 1)
        diagnostics.end_document("a.pdf")

        # Assert.Contains with a predicate becomes any() under assertTrue: assertIn
        # tests membership of a value, and what is asserted here is the shape of a line
        # rather than its exact text, because the elapsed time in it varies per run.
        self.assertTrue(any(line.startswith("a.pdf: ") and line.endswith("s")
                            for line in console.out_lines),
                        "no line of the form 'a.pdf: <n>s' was written")
