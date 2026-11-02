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

from enum import Enum
from typing import Optional

from ..runtime.cli_error import CliError, DocumentError
from ..runtime.interruption import Interruption, RunInterruptedError


class StopReason(Enum):
    """Why a document was abandoned."""

    INTERRUPTED = 1
    """The user pressed Ctrl+C."""

    TIMED_OUT = 2
    """--timeout elapsed for this document."""


class StopSignal:
    """
    The predicate the engine polls during open, and the reason it gave when it said to
    stop.

    One object rather than a bare callable because the answer and the reason for it are
    the same fact, decided at the same instant. The engine can only be told *whether* to
    stop -- IGR_CANCELLED carries nothing else -- so a cause worked out afterwards from the
    same two conditions is a second guess at a question already answered: both can be true
    by then, either can have become true after the decision, and neither need still be
    true at all.

    Recorded on the first poll that returns true and never revised, so the run reports the
    condition that actually stopped it.
    """

    def __init__(self, diagnostics, interruption: Optional[Interruption]):
        # diagnostics is unannotated for the reason canvas_conversion gives for session:
        # nothing here needs the type, only the object.
        self._diagnostics = diagnostics
        self._interruption = interruption
        self.reason: Optional[StopReason] = None
        """
        The reason recorded by the poll that stopped the document, if one did. None after
        a cancellation means the engine stopped for a reason of its own and nobody asked it
        to.
        """

    def should_stop(self) -> bool:
        """
        What the engine polls. True once, then true forever.

        The interrupt is tested first: if the user pressed Ctrl+C, that is what they are
        owed an answer about, whatever else may also have come true.
        """
        if self.reason is not None:
            return True

        if self._interruption is not None and self._interruption.requested:
            self.reason = StopReason.INTERRUPTED
        elif self._diagnostics.document_deadline_exceeded():
            self.reason = StopReason.TIMED_OUT

        return self.reason is not None

    def cause(self, display_name: str) -> CliError:
        """The error for a cancelled open, given what this signal recorded."""
        if self.reason == StopReason.INTERRUPTED:
            return RunInterruptedError()

        if self.reason == StopReason.TIMED_OUT:
            return self._diagnostics.document_timed_out(display_name)

        return DocumentError(display_name, "the engine cancelled the operation")
