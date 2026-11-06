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

from typing import List

from fakes.in_memory_file_system import InMemoryFileSystem

from . import fixture
from .fixture import FixtureCase

# The plan corpus: the shared fixture reader plus the one thing specific to it, splitting a
# subject line into command line arguments.
#
# A module of functions rather than a class: the C# is a static class, which is what C# has
# instead of a module.

FILE_NAME = "plan-corpus.txt"


def load() -> List[FixtureCase]:
    """Every case in the corpus."""
    return fixture.load(FILE_NAME)


def expected_exit(test_case: FixtureCase) -> int:
    """The expected exit code for a case, defaulting to success."""
    value = test_case.get("exit")
    return 0 if value is None else int(value)


def build_file_system(test_case: FixtureCase) -> InMemoryFileSystem:
    """
    Builds the filesystem a case is given by its < lines.

    Never the real one. A real directory listing has no defined order and differs between
    ext4, NTFS and APFS, so a corpus backed by real files could not be byte-identical
    across four languages and three platforms. The fake also models symlinks and hidden
    entries as plain sets, which is what lets --follow and --include-hidden be contract at
    all: creating a symlink on Windows needs a privilege most checkouts do not have, and
    "hidden" is a dot-prefix on POSIX and a file attribute on Windows.
    """
    file_system = InMemoryFileSystem()

    for row in test_case.given_all("tree"):
        for path in _split_paths(row):
            if path.endswith("/"):
                file_system.add_directory(path[:-1])
            else:
                file_system.add_file(path)

    # Links after the tree, so a link may point at a directory the tree created.
    for row in test_case.given_all("link"):
        parts = _split_paths(row)

        if len(parts) != 2:
            raise ValueError(test_case.describe("'< link' needs '<path> <target>': " + row))

        file_system.mark_symlink(parts[0], parts[1])

    # Hidden last: it marks an entry the two above have already created.
    for row in test_case.given_all("hidden"):
        for path in _split_paths(row):
            file_system.mark_hidden(path)

    return file_system


def _split_paths(row: str) -> List[str]:
    return [part for part in row.replace('\t', ' ').split(' ') if part]


def split_arguments(command_line: str) -> List[str]:
    """
    Splits a corpus subject into arguments on whitespace, honouring double quotes so a
    value may contain spaces.
    """
    args = []
    current = []
    quoted = False

    for character in command_line:
        if character == '"':
            quoted = not quoted
        elif _is_whitespace(character) and not quoted:
            if len(current) > 0:
                args.append(''.join(current))
                current = []
        else:
            current.append(character)

    if len(current) > 0:
        args.append(''.join(current))

    return args


def _is_whitespace(character: str) -> bool:
    """
    The separator test, restricted to the characters the corpus actually uses.

    char.IsWhiteSpace in the C#, which is Unicode-aware; str.isspace() is too, but the two
    disagree on the C0 information separators U+001C-U+001F, which Python calls whitespace
    and .NET does not. Naming the five characters a subject line can contain settles it
    identically for all four ports instead of inheriting either standard library's answer.
    """
    return character in " \t\r\n\v\f"
