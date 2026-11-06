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

import io
from collections import deque
from typing import List, Optional

from docfilters_cli.runtime.i_console import IConsole


class RecordingConsole(IConsole):
    """
    Captures everything the tool would have printed, so that output can be asserted
    on without a terminal.

    Standard output and standard error are kept apart deliberately: error text is
    part of the cross-language contract and is asserted precisely, while progress
    chatter is not.
    """

    # A plain class attribute rather than an assignment in __init__: IConsole declares
    # this an abstract property, and only something in the class body counts as an
    # override — set in __init__ it would still be abstract and the class would refuse
    # to instantiate.
    is_input_interactive = False
    """Set this to exercise the --password-prompt paths."""

    def __init__(self):
        self._out: List[str] = []
        self._binary = io.BytesIO()

        self.out_lines: List[str] = []
        """Every line written to standard output, in order."""

        self.error_lines: List[str] = []
        """Every line written to standard error, in order. Part of the contract."""

        self.verbose_lines: List[str] = []
        """Every verbose line. Not part of the contract."""

        self.input_lines: List[str] = []
        """Standard input, for --files-from -."""

        self.password_responses = deque()
        """Answers handed to successive read_password calls, in order."""

        self.password_prompts: List[str] = []
        """The prompts read_password was called with."""

        self.key_waits = 0
        """How many times wait_for_key was called, for --pause."""

    def read_all_input_lines(self) -> List[str]:
        return self.input_lines

    def read_password(self, prompt: str) -> Optional[str]:
        self.password_prompts.append(prompt)
        return self.password_responses.popleft() if self.password_responses else None

    def wait_for_key(self) -> None:
        self.key_waits += 1

    @property
    def binary_output(self) -> bytes:
        """Everything written to the binary stdout stream."""
        return self._binary.getvalue()

    @property
    def output(self) -> str:
        """Standard output as one string."""
        return ''.join(self._out)

    def write(self, text: str) -> None:
        self._out.append(text)

    def write_line(self, text: str) -> None:
        self._out.append(text)
        self._out.append('\n')
        self.out_lines.append(text)

    def write_error(self, text: str) -> None:
        self.error_lines.append(text)

    def write_verbose(self, text: str) -> None:
        self.verbose_lines.append(text)

    def open_standard_output(self):
        return self._binary
