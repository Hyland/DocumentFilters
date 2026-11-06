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

from typing import List, Optional


class PasswordProvider:
    """
    Supplies passwords to the engine as it encounters protected documents.

    Candidates come from -p, then --password-file, in order. When none of them work and
    --password-prompt was given, the user is asked.

    Open, and deliberately so: what --password-prompt should do when stdin is not a
    terminal — fail with a usage error, or fall through as if it were not given. §09 of the
    design document carries it as one of the calls to be made while building rather than
    settled in advance, so the four ports must agree on an answer before any of them relies
    on one.
    """

    def __init__(self, options, console, file_system):
        self._options = options
        self._console = console
        self._file_system = file_system

        self._file_candidates: Optional[List[str]] = None
        self._next_index = 0
        self._prompt_exhausted = False

    def get_password(self, document_id: str) -> Optional[str]:
        """
        Returns the next password to try for the given document, or None when the
        candidates are exhausted.

        -p candidates are tried in the order given, then every line of --password-file, and
        finally — once those are exhausted and --password-prompt was given — one
        interactive prompt. The prompt is asked at most once per document: an engine that
        rejects it moves on to the next document rather than asking again forever.
        """
        if self._file_candidates is None:
            self.load_password_file()

        candidates = self._options.passwords
        if self._next_index < len(candidates):
            password = candidates[self._next_index]
            self._next_index += 1
            return password

        file_index = self._next_index - len(candidates)
        if file_index < len(self._file_candidates):
            self._next_index += 1
            return self._file_candidates[file_index]

        if self._options.prompt and not self._prompt_exhausted and self._console.is_input_interactive:
            self._prompt_exhausted = True
            return self._console.read_password("Password for {0}: ".format(document_id))

        return None

    def reset(self):
        """Forgets which candidates have been tried, when moving to a new document."""
        self._next_index = 0
        self._prompt_exhausted = False

    def load_password_file(self):
        """Reads --password-file, one candidate per line."""
        if not self._options.password_file:
            self._file_candidates = []
        else:
            self._file_candidates = [line for line in self._file_system.read_all_lines(self._options.password_file)
                                     if len(line) > 0]
