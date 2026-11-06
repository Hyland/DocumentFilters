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
from typing import List

from ..runtime.cli_error import ErrorCodes, UsageError


class NameMatcher:
    """
    Path selection from -g/--glob or --regex.

    Glob is the default idiom. Patterns are matched against the path as written,
    using forward slashes on every platform so that a pattern is portable. A single
    star matches within one path segment, a double star crosses segments, and a
    leading ! turns a pattern into an exclusion.

    Multiple patterns combine as: an entry is kept when it matches at least one
    include pattern (or there are none) and matches no exclude pattern.
    """

    def __init__(self):
        self._glob_patterns = None
        self._regex_patterns = None

        self._includes = []
        self._excludes = []

    @staticmethod
    def from_globs(patterns: List[str]) -> 'NameMatcher':
        """
        Builds a matcher from one or more -g/--glob patterns.

        Raises:
            UsageError: A pattern is malformed.
        """
        matcher = NameMatcher()
        matcher._glob_patterns = list(patterns)
        for raw in patterns:
            is_exclude = raw.startswith("!")
            body = raw[1:] if is_exclude else raw
            regex = NameMatcher._compile_glob(body, raw)
            (matcher._excludes if is_exclude else matcher._includes).append(regex)
        return matcher

    @staticmethod
    def from_regexes(patterns: List[str]) -> 'NameMatcher':
        """
        Builds a matcher from one or more --regex patterns.

        Raises:
            UsageError: A pattern is not a valid regular expression.
        """
        matcher = NameMatcher()
        matcher._regex_patterns = list(patterns)
        for raw in patterns:
            try:
                # wrap the regex in ^(?: ... )\Z so that it is anchored to the beginning and end of the string,
                # and it's in a non-capturing group, which is useful if the user has a regex with alternation (|) at the top level.
                #
                # The C# writes \z; Python spells that absolute-end anchor \Z, and has
                # no \z at all. Python's \Z is exactly .NET's \z — not .NET's \Z — and
                # $ would be wrong in either dialect, because it also matches before a
                # trailing newline.
                matcher._includes.append(re.compile("^(?:" + raw + ")\\Z"))
            except re.error:
                raise UsageError(ErrorCodes.BAD_VALUE, "invalid regular expression '{0}'".format(raw))
        return matcher

    def matches(self, path: str) -> bool:
        """True when the path should be processed."""
        # Plain string replacement rather than os.path or pathlib, which would emit a
        # backslash on Windows and make every pattern platform specific.
        normalized = path.replace('\\', '/')

        if any(regex.search(normalized) for regex in self._excludes):
            return False

        return len(self._includes) == 0 or any(regex.search(normalized) for regex in self._includes)

    @staticmethod
    def _compile_glob(pattern: str, original: str):
        """Translates one -g/--glob pattern (minus any leading !) into an anchored regex."""
        raw_segments = pattern.split('/')
        segments = []
        for segment in raw_segments:
            if segment == "**" and len(segments) > 0 and segments[-1] == "**":
                continue
            segments.append(segment)

        parts = ["^"]
        for i in range(len(segments)):
            segment = segments[i]
            is_first = i == 0
            is_last = i == len(segments) - 1
            previous_is_double_star = not is_first and segments[i - 1] == "**"

            if segment == "**":
                if not is_first:
                    parts.append('/')
                parts.append(".*" if is_last else "(?:.*/)?")
            else:
                if not is_first and not previous_is_double_star:
                    parts.append('/')
                parts.append(NameMatcher._translate_glob_segment(segment, original))
        # The C# ends the pattern "\z$". Only the \z does any work there — $ after an
        # absolute-end anchor can never match anything more — so the Python spelling of
        # that anchor, \Z, is used alone.
        parts.append("\\Z")

        try:
            return re.compile(''.join(parts))
        except re.error:
            raise UsageError(ErrorCodes.BAD_VALUE, "invalid glob pattern '{0}'".format(original))

    @staticmethod
    def _translate_glob_segment(segment: str, original: str) -> str:
        """
        Translates one path segment: * and ? stay within the segment, everything
        else is literal — including a bracket pair, which this tool's glob dialect
        gives no matching meaning to, so [abc] matches only that literal
        five-character text. An unterminated [ is still rejected as malformed rather
        than silently taken as a literal character: it is almost certainly a typo,
        not an intentional five-character filename fragment.
        """
        parts = []
        i = 0
        while i < len(segment):
            c = segment[i]
            if c == '*':
                parts.append("[^/]*")
                i += 1
            elif c == '?':
                parts.append("[^/]")
                i += 1
            elif c == '[':
                close = segment.find(']', i + 1)
                if close < 0:
                    raise UsageError(ErrorCodes.BAD_VALUE, "invalid glob pattern '{0}'".format(original))
                # re.escape and .NET's Regex.Escape do not escape the same set of
                # characters — Python escapes the closing bracket, .NET leaves it bare —
                # but both produce a pattern that matches only this literal text, which
                # is all that is observable.
                parts.append(re.escape(segment[i:close + 1]))
                i = close + 1
            else:
                parts.append(re.escape(c))
                i += 1
        return ''.join(parts)

    def __str__(self) -> str:
        """The canonical text form, used by the conformance corpus."""
        if self._glob_patterns is not None:
            return "glob " + ", ".join(self._glob_patterns)
        if self._regex_patterns is not None:
            return "regex " + ", ".join(self._regex_patterns)
        return ""
