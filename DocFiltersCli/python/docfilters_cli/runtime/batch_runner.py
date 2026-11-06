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

from ..configuration.shared_options import BatchOptions, OnErrorBehaviour
from . import exit_code
from .cli_error import DocumentError
from .i_console import IConsole
from .interruption import RunInterruptedError


class BatchRunner:
    """
    The per-file loop, in one place, so that --on-error behaves identically on
    every command.
    """

    def __init__(self, options: BatchOptions, console: IConsole, interruption=None):
        self._options = options
        self._console = console
        self._interruption = interruption
        self.success_count = 0
        self.failure_count = 0

    def run(self, items, action) -> int:
        """
        Runs action for each item.

        Under --on-error skip a failure is reported and the loop continues, but the
        run still ends non-zero. Under --on-error stop the first failure ends the
        loop. Errors that are not per-document — a usage error, a limit being
        exceeded — propagate rather than being counted.

        Under --combine this still runs per input. Skipping a failed document leaves
        the shared canvas open and carries on, so the run produces one document
        containing everything that converted, and exits 1. A partial output is the
        point rather than an accident: exit 1 means "some inputs failed", not
        "nothing was written".

        Returns a value from exit_code: SUCCESS, or PARTIAL_FAILURE.
        """
        self.success_count = 0
        self.failure_count = 0

        for item in items:
            # Between documents, because that is the only point in a batch where
            # stopping leaves nothing half-written.
            if self._interruption is not None and self._interruption.requested:
                raise RunInterruptedError()

            try:
                action(item)
                self.success_count += 1
            except DocumentError as error:
                self.failure_count += 1
                self._console.write_error("{0}: {1}: {2}".format(error.path, error.code, error.message))

                if self._options.on_error == OnErrorBehaviour.STOP:
                    break

        return exit_code.PARTIAL_FAILURE if self.failure_count > 0 else exit_code.SUCCESS
