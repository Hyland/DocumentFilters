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
from typing import Iterator, List, Optional, Tuple

from ..runtime.cli_error import ErrorCodes, UsageError

# The largest value the C# reference accepts, because it parses with int.TryParse
# and a 32-bit int overflows past this. Python integers have no such ceiling, so
# the limit has to be applied by hand or the ports disagree on "99999999999".
_MAX_PAGE_NUMBER = 2147483647

# Only ASCII digits, and nothing else: no sign, no surrounding whitespace, no
# thousands separator, no decimal point. int() would accept " +5 " and "5_0", and
# str.isdigit() would accept non-ASCII digits such as U+0663, none of which the
# other three implementations parse.
# \Z ends the pattern rather than $, which in Python also matches before a trailing
# newline and would let --pages "3\n" parse as page 3 instead of being the bad value the
# other three ports call it.
_PAGE_NUMBER_PATTERN = re.compile(r'^[0-9]+\Z')


class PageRange:
    """
    A --pages selection such as 1,3-5,8. Pages are one-based, as the user writes
    them.
    """

    def __init__(self, spans: Optional[List[Tuple[int, int]]], canonical_text: str):
        # None means "every page" (PageRange.ALL); otherwise the selected one-based pages
        # as inclusive (from, to) spans, ascending, and merged so that no two overlap or
        # touch.
        #
        # Spans and not the pages themselves, because a span costs the same however wide
        # it is. Expanding one into a set made --pages 1-2147483647 -- a valid value --
        # allocate two billion entries while the arguments were still being read.
        self._spans = spans
        self._canonical_text = canonical_text

    @property
    def is_all(self) -> bool:
        """True when no restriction was given."""
        return self._spans is None

    @property
    def first_page(self) -> int:
        """The lowest page selected: 1 for PageRange.ALL."""
        return 1 if self.is_all else self._spans[0][0]

    @staticmethod
    def parse(text: str) -> 'PageRange':
        """
        Parses a range expression. Accepts comma-separated single pages and
        from-to spans. Duplicates collapse and the result is ascending, so
        3,1,1-2 and 1,2,3 are the same selection.

        Raises:
            UsageError: Malformed syntax, a page below 1, or a descending span
                such as 5-3.
        """
        if not text:
            PageRange._throw()

        given = []  # type: List[Tuple[int, int]]
        for token in text.split(','):
            if len(token) == 0:
                PageRange._throw()  # a leading, trailing or doubled comma

            dash = token.find('-')
            if dash < 0:
                page = PageRange._parse_page_number(token)
                given.append((page, page))
            else:
                start = PageRange._parse_page_number(token[:dash])
                end = PageRange._parse_page_number(token[dash + 1:])

                if end < start:
                    PageRange._throw()  # "5-3": far more likely a typo than a request to reverse

                given.append((start, end))

        spans = PageRange._merge(given)
        return PageRange(spans, PageRange._build_canonical_text(spans))

    @staticmethod
    def _merge(spans: List[Tuple[int, int]]) -> List[Tuple[int, int]]:
        """
        Sorts spans and joins every pair that overlaps or touches, so 3,1,1-2 and 1-3
        become the same single span.
        """
        merged = []  # type: List[Tuple[int, int]]
        for start, end in sorted(spans):
            if merged and start <= merged[-1][1] + 1:
                merged[-1] = (merged[-1][0], max(merged[-1][1], end))
            else:
                merged.append((start, end))

        return merged

    @staticmethod
    def _parse_page_number(text: str) -> int:
        if not text:
            PageRange._throw()

        if not _PAGE_NUMBER_PATTERN.match(text):
            PageRange._throw()

        value = int(text)
        if value < 1 or value > _MAX_PAGE_NUMBER:
            PageRange._throw()

        return value

    @staticmethod
    def _throw():
        raise UsageError(ErrorCodes.BAD_PAGE_RANGE,
                         "--pages must be a list of pages or ranges, such as 1,3-5,8")

    @staticmethod
    def _build_canonical_text(spans: List[Tuple[int, int]]) -> str:
        """
        Writes merged spans in their canonical form, e.g. 1,3,4,5,8 becomes 1,3-5,8: a
        run of three or more pages as from-to, and a run of two as the two pages.
        """
        parts = []
        for start, end in spans:
            if end - start <= 1:
                parts.append(str(start))
                if end != start:
                    parts.append(str(end))
            else:
                parts.append("{0}-{1}".format(start, end))

        return ','.join(parts)

    def contains(self, page_number: int) -> bool:
        """True when this selection includes the given one-based page number."""
        if self.is_all:
            return page_number >= 1

        return any(start <= page_number <= end for start, end in self._spans)

    def enumerate(self, page_count: int) -> Iterator[int]:
        """
        The selected page numbers, ascending: every page of a document with
        page_count pages for PageRange.ALL, or the explicit selection exactly as given
        otherwise.

        Lazy, so a caller that stops early pays only for the pages it took: a span can be
        two billion pages wide, and a document is rarely that long. An iterator where the
        C# returns IEnumerable<int>, for the same reason.

        An explicit selection is never filtered against page_count here — a count
        is all this class is ever given, not a verified list of which page numbers
        are real, and not every caller even has a genuine one yet: a plan built
        before the document is open has nothing better than a guess. Deciding
        whether a requested page actually exists belongs to whoever opens it, once
        it has a count it can trust; this only ever answers "what did the user ask
        for".
        """
        if self.is_all:
            return iter(range(1, max(0, page_count) + 1))

        return (page for start, end in self._spans for page in range(start, end + 1))

    def __str__(self) -> str:
        """The canonical text form, used by the conformance corpus."""
        return self._canonical_text


# The C# declares this as a static property on the class. Python cannot name the
# class inside its own body, so the single shared instance is attached here
# instead; it is immutable, so one instance for the whole process is safe.
PageRange.ALL = PageRange(None, "all")
"""Selects every page."""
