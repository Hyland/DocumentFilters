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

import signal

from . import exit_code
from .cli_error import CliError, ErrorCodes


class Interruption:
    """
    Whether the user has asked the run to stop — Ctrl+C.

    An object rather than a module-level flag so a test can raise one without
    touching the process, and so two runs in the same process cannot see each
    other's.

    Read in two places, which between them are every point the run can be stopped
    at all: BatchRunner checks it between documents, and the engine's open callback
    polls it so a document that hangs while parsing does not have to finish first.
    Nothing can interrupt a page loop — IGR_Render_Page takes no callback — so a
    long render still runs to completion, the same limitation --timeout has.
    """

    def __init__(self):
        self._requested = False

    @property
    def requested(self) -> bool:
        """True once the user has interrupted the run."""
        return self._requested

    def request(self) -> None:
        """Records that the user asked to stop."""
        self._requested = True

    @staticmethod
    def install() -> 'Interruption':
        """
        Installs a Ctrl+C handler that records the interrupt instead of killing the
        process, and returns the object the run should read.

        Replacing the default handler is what stops the interpreter unwinding
        immediately, which is the whole point: the run gets to finish what it is
        holding, report, and exit 130 through the same path as every other exit code
        rather than dying with a traceback.
        """
        interruption = Interruption()

        signal.signal(signal.SIGINT, lambda signal_number, frame: interruption.request())

        return interruption


class RunInterruptedError(CliError):
    """
    The run was interrupted by the user. C#'s InterruptedError, renamed because that
    name is already a builtin here and means something else entirely.

    Not a failure of any document, so it names none — the same shape as a run-level
    limit.
    """

    def __init__(self):
        super().__init__(ErrorCodes.INTERRUPTED, "interrupted")

    @property
    def exit_code(self):
        return exit_code.INTERRUPTED
