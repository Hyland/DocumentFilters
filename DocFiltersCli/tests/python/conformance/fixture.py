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

import os
from typing import List, Optional, Tuple


class FixtureCase:
    """One case from a shared fixture: a subject, the world it runs in, and its expectations."""

    def __init__(self, subject: str, file: str, line: int):
        self.subject = subject
        """The text after $ — a command line, a format token, a template."""

        self.given: List[Tuple[str, str]] = []
        """
        The < lines: what the case is given, in fixture order. Keys may repeat.

        Held apart from expectations rather than distinguished by key name, because they
        are the opposite kind of thing: < describes the world before the run and =
        describes what the run must produce. A reader that mixed them would make every
        consumer filter by a list of key names it had to keep in step.
        """

        self.expectations: List[Tuple[str, str]] = []
        """The = lines, in fixture order. Keys may repeat."""

        self.file = file
        """Fixture filename, for failure messages."""

        self.line = line
        """1-based line of the $ line."""

    @property
    def error(self) -> Optional[str]:
        """The expected error code, when the case expects a failure."""
        return self.get("error")

    def get(self, key: str) -> Optional[str]:
        """
        The single value for a key, trimmed, or None when the key is absent.

        Values are stored verbatim and trimmed here, because fixtures pad them into a
        readable column and almost no case cares about that padding. The exceptions use
        get_raw, for a value whose own leading whitespace is part of what it asserts.
        """
        value = self.get_raw(key)
        return None if value is None else value.strip()

    def get_raw(self, key: str) -> Optional[str]:
        """The single value for a key exactly as written, padding included."""
        for pair_key, pair_value in self.expectations:
            if pair_key == key:
                return pair_value

        return None

    def get_all(self, key: str) -> List[str]:
        """Every value for a repeated key, trimmed, in fixture order."""
        return [value.strip() for pair_key, value in self.expectations if pair_key == key]

    def get_all_raw(self, key: str) -> List[str]:
        """
        Every value for a repeated key exactly as written, with one separating space
        removed and everything beyond it kept.

        This is what the golden output cases use. `= json    "a": 1` yields `   "a": 1` —
        the first space after the key separates, the rest is the value's own indentation.
        """
        return [value for pair_key, value in self.expectations if pair_key == key]

    def has(self, key: str) -> bool:
        """True when the fixture states an expectation for this key."""
        return any(pair_key == key for pair_key, _ in self.expectations)

    def given_all(self, key: str) -> List[str]:
        """Every value for a repeated < key, trimmed, in fixture order."""
        return [value.strip() for pair_key, value in self.given if pair_key == key]

    def has_given(self, key: str) -> bool:
        """True when the case is given anything under this key."""
        return any(pair_key == key for pair_key, _ in self.given)

    def get_bool(self, key: str) -> bool:
        """A yes/no expectation. Accepts only "yes" and "no", so the fixtures stay uniform."""
        value = self.get(key)

        if value == "yes":
            return True
        if value == "no":
            return False

        raise ValueError(self.describe("'{0}' must be yes or no, was '{1}'".format(key, value)))

    def describe(self, problem: str) -> str:
        """
        A failure message that names the fixture and line, states the problem, and
        reproduces the whole case, so a failing run does not send the reader off to open
        another file.
        """
        text = ["{0}:{1}  $ {2}\n".format(self.file, self.line, self.subject),
                "  {0}\n".format(problem),
                "  case as written:\n"]

        for key, value in self.given:
            text.append("    < {0} {1}\n".format(key, value))

        for key, value in self.expectations:
            text.append("    = {0} {1}\n".format(key, value))

        return ''.join(text)

    def __str__(self) -> str:
        return "{0}:{1} {2}".format(self.file, self.line, self.subject)


# Reader for the shared conformance fixtures.
#
# Every fixture uses one format, so each language needs exactly one reader rather than one
# per file. A case is a $ subject line, then any < lines saying what the case is given,
# then = lines saying what it must produce. Blank lines separate cases and # starts a
# comment.
#
# Deliberately free of JSON and YAML so the C++17 and Java suites can parse it with
# nothing but their standard library — and, here, so that the Python suite needs no
# dependency either.

_HERE = os.path.dirname(os.path.abspath(__file__))

DIRECTORY = os.path.abspath(os.path.join(_HERE, '..', '..', 'conformance'))
"""
Where the fixtures live.

The C# reads them from AppContext.BaseDirectory, because the build copies them next to
the test assembly. Python has no such copy step, so this reads the one true copy in the
source tree directly — which is better: there is no build output that can go stale
against the fixture a reviewer is editing.
"""


def load(name: str) -> List[FixtureCase]:
    """Loads a fixture by filename, e.g. formats.txt."""
    path = os.path.join(DIRECTORY, name)

    if not os.path.isfile(path):
        raise FileNotFoundError("conformance fixture not found: " + path)

    with open(path, 'r', encoding='utf-8') as handle:
        lines = handle.read().split('\n')

    return parse(name, lines)


def parse(file: str, lines: List[str]) -> List[FixtureCase]:
    """Parses fixture text. Exposed so the reader itself can be tested."""
    cases = []
    current = None

    for index, raw_line in enumerate(lines):
        line = raw_line.strip()

        if line.startswith("#"):
            continue

        if len(line) == 0:
            # A blank line ends the current case, as the format documents. Merely skipping
            # it would silently attach a later '=' line to the case above the gap — which a
            # stricter reader in another language would reject, so the same fixture would
            # mean different things in different ports.
            current = None
            continue

        if line.startswith("$"):
            current = FixtureCase(line[1:].strip(), file, index + 1)
            cases.append(current)
            continue

        if line.startswith("<"):
            if current is None:
                raise ValueError("{0}:{1}: '<' line with no preceding '$' case".format(file, index + 1))

            if len(current.expectations) > 0:
                raise ValueError(
                    "{0}:{1}: '<' describes what the case is given and comes before every '=' line".format(
                        file, index + 1))

            current.given.append(_split_key_value(line))
            continue

        if line.startswith("="):
            if current is None:
                raise ValueError("{0}:{1}: expectation with no preceding '$' case".format(file, index + 1))

            current.expectations.append(_split_key_value(line))
            continue

        raise ValueError("{0}:{1}: unrecognised line: {2}".format(file, index + 1, line))

    return cases


def _split_key_value(line: str) -> Tuple[str, str]:
    """
    Splits a < or = line into its key and its value.

    Exactly one space separates the key from the value; everything after it is stored
    verbatim. Callers that do not care about the column padding use get/get_all, which
    trim. get_raw/get_all_raw exist for a value whose own leading whitespace is part of
    what it asserts, and must survive the reader.
    """
    rest = line[1:].lstrip()
    space = rest.find(' ')
    key = rest if space < 0 else rest[:space]
    value = "" if space < 0 else rest[space + 1:]

    return key, value
