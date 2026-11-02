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

from abc import ABC, abstractmethod
from typing import BinaryIO, List, Optional


class IConsole(ABC):
    """
    Everything the tool writes to the terminal, behind one seam so tests can
    capture output without a console.

    --silent, --verbose and --color are honoured by the implementation, not by
    callers: call sites always say what they mean and let this decide whether it
    is shown.
    """

    @abstractmethod
    def write(self, text: str) -> None:
        """Report output — the answer the user asked for. Goes to stdout."""
        pass

    @abstractmethod
    def write_line(self, text: str) -> None:
        """Report output followed by a newline."""
        pass

    @abstractmethod
    def write_error(self, text: str) -> None:
        """An error message. Goes to stderr, and is part of the cross-language contract."""
        pass

    @abstractmethod
    def write_verbose(self, text: str) -> None:
        """Progress or diagnostic chatter. Suppressed unless --verbose."""
        pass

    @abstractmethod
    def open_standard_output(self) -> BinaryIO:
        """
        The raw stdout stream, for --stdout with a binary format. On Windows
        this must be opened in binary mode so that PDF and image bytes survive.
        """
        pass

    @property
    @abstractmethod
    def is_input_interactive(self) -> bool:
        """True when stdin is an interactive terminal, which --password-prompt requires."""
        pass

    @abstractmethod
    def read_all_input_lines(self) -> List[str]:
        """
        Every line of standard input, for --files-from -.

        Reading stdin belongs here rather than on IFileSystem: "-" is not a path,
        and routing it through the filesystem would leave tests unable to supply it.
        """
        pass

    @abstractmethod
    def read_password(self, prompt: str) -> Optional[str]:
        """
        Prompts for a password and reads it without echoing, for --password-prompt.
        Returns None when the user cancels.
        """
        pass

    @abstractmethod
    def wait_for_key(self) -> None:
        """Waits for a keypress between documents, for --pause."""
        pass
