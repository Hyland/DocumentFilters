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

import re
from typing import List, Optional

from ..runtime.cli_error import ErrorCodes, UsageError
from .command_options import FormValue, RedactSpec
from .shared_options import OnErrorBehaviour

# Turns command line text into typed values.
#
# Every conversion that can fail lives here rather than in the CLI parsing library, so
# that a bad value produces the same message and the same exit code in the C#, Python,
# Java and C++17 builds. The parsers themselves are only ever asked for bool, str and
# list-of-str, none of which can fail to convert.

# \Z rather than $ for the end anchor. Python's $ also matches immediately before a
# trailing newline, so '5\n' would satisfy a '^[0-9]+$' pattern and be accepted here as 5,
# where int.TryParse with NumberStyles.None rejects it for want of AllowTrailingWhite. \Z
# is the absolute end of the string and nothing else, which is the anchor the C# means.
_DIGITS = re.compile(r'^[0-9]+\Z')

# The C# parses with int.TryParse(..., NumberStyles.None, InvariantCulture, ...), which
# rejects anything past the type's range. Python integers have no range, so the bounds
# have to be stated for the overflow cases to stay usage errors rather than being
# accepted here and overflowing somewhere further in.
_INT_MAX = 2147483647
_LONG_MAX = 9223372036854775807


def _parse_strict(text: Optional[str], maximum: int) -> Optional[int]:
    """
    An integer exactly as int.TryParse with NumberStyles.None reads one, or None.

    Digits and nothing else: no sign, no surrounding whitespace, no group separator, no
    decimal point, and nothing outside the type's range. Neither int() nor str.isdigit()
    is close enough — int() accepts " +5 " and "5_0", isdigit() accepts non-ASCII digits
    like ٣, and neither has an upper bound.
    """
    if text is None or _DIGITS.match(text) is None:
        return None

    value = int(text)
    return None if value > maximum else value


def parse_count(argument: str, text: str) -> int:
    """Parses a non-negative integer, e.g. for --max-depth."""
    value = _parse_strict(text, _INT_MAX)
    if value is None or value < 0:
        _throw(argument, text)
    return value


def parse_seconds(argument: str, text: str) -> int:
    """Parses a positive integer of seconds, e.g. for --timeout."""
    value = _parse_strict(text, _INT_MAX)
    if value is None or value <= 0:
        _throw(argument, text)
    return value


def parse_megabytes(argument: str, text: str) -> int:
    """Parses a size in megabytes, e.g. for --max-memory."""
    value = _parse_strict(text, _LONG_MAX)
    if value is None or value <= 0:
        _throw(argument, text)
    return value


def parse_id_list(text: str) -> List[str]:
    """
    Splits a semicolon-separated id list, e.g. --include-types "PDF;DOCX". Empty
    entries are dropped; surrounding whitespace is trimmed.
    """
    result = []
    for part in text.split(';'):
        trimmed = trim(part)
        if trimmed:
            result.append(trimmed)
    return result


def parse_redaction(text: str) -> RedactSpec:
    """Parses one --redact page,x,y,w,h value."""
    parts = text.split(',')
    if len(parts) != 5:
        _throw("--redact", text)

    values = []
    for part in parts:
        value = _parse_strict(part, _INT_MAX)
        if value is None:
            _throw("--redact", text)
        values.append(value)

    spec = RedactSpec()
    spec.page, spec.x, spec.y, spec.width, spec.height = values
    return spec


def parse_form_value(text: str) -> FormValue:
    """
    Parses one --form page,name=value value.

    Only the first = separates name from value, so values may contain =.
    """
    comma = text.find(',')
    if comma < 0:
        _throw("--form", text)

    page_text = text[:comma]
    rest = text[comma + 1:]

    equals = rest.find('=')
    if equals < 0:
        _throw("--form", text)

    page = _parse_strict(page_text, _INT_MAX)
    if page is None:
        _throw("--form", text)

    value = FormValue()
    value.page = page
    value.name = rest[:equals]
    value.value = rest[equals + 1:]
    return value


def parse_on_error(text: str) -> OnErrorBehaviour:
    """Parses --on-error: "skip" or "stop", after fold_token."""
    if text == "skip":
        return OnErrorBehaviour.SKIP
    if text == "stop":
        return OnErrorBehaviour.STOP

    _throw("--on-error", text)


def fold_token(text: str) -> str:
    """
    Folds a value drawn from a vocabulary this tool publishes, so that PDF, Pdf and pdf
    are one token.

    Applied by --to, --on-error, --applies-to and the two log-level
    arguments — every argument whose valid values are enumerated by this tool rather
    than supplied by the engine or the user. Paths, globs, regular expressions,
    --out-name templates, --filter text, passwords, --codepage and the --options string
    are used exactly as typed.

    ASCII only, and that is the whole point. A locale-aware fold maps I to the dotless ı
    under a Turkish locale, so --to TIFF would stop resolving on a Turkish machine —
    identically in C#, Java and Python, and in C++ where std::tolower is
    locale-sensitive too. This maps A-Z to a-z and leaves every other character alone.
    str.lower() is not that: it is Unicode-aware, so it would fold Kelvin sign to k and
    turn İ into two characters.
    """
    return ''.join(chr(ord(c) - ord('A') + ord('a')) if 'A' <= c <= 'Z' else c for c in text)


# The four characters Python calls whitespace and .NET does not: the C0 information
# separators, FS, GS, RS and US. Every other character the two disagree about is one they
# actually agree about -- U+0085 and U+00A0 included -- so subtracting exactly these from
# Python's own answer reproduces char.IsWhiteSpace without a table of the whole Unicode
# whitespace set, and keeps tracking Unicode as Python's tables are updated.
_NOT_WHITESPACE_IN_DOTNET = '\x1c\x1d\x1e\x1f'


def is_whitespace(character: str) -> bool:
    """
    Whether the reference implementation would call this character whitespace.

    char.IsWhiteSpace, in other words, rather than str.isspace(). PlanCorpus settles the
    same disagreement for the fixture reader by naming the five characters a subject line
    may contain; user input has no such fixed vocabulary, so this subtracts the difference
    from Python's answer instead.
    """
    return character.isspace() and character not in _NOT_WHITESPACE_IN_DOTNET


def trim(text: str) -> str:
    """
    Strips leading and trailing whitespace exactly as string.Trim does.

    str.strip() is not that: it also removes U+001C-U+001F, so a --include-types entry or
    a --log-modules name ending in one of them would be a different token here than in the
    other three ports. Written as two scans rather than str.strip(a_table_of_characters)
    because the predicate, not a character list, is what the reference actually applies.
    """
    start = 0
    end = len(text)

    while start < end and is_whitespace(text[start]):
        start += 1
    while end > start and is_whitespace(text[end - 1]):
        end -= 1

    return text[start:end]


def is_all_whitespace(text: Optional[str]) -> bool:
    """string.IsNullOrWhiteSpace, which is how a blank line in --files-from is recognised."""
    return text is None or len(trim(text)) == 0


def _throw(argument: str, text: str):
    raise UsageError(ErrorCodes.BAD_VALUE, "{0} does not accept the value '{1}'".format(argument, text))
