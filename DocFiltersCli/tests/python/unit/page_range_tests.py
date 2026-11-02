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

import itertools
import unittest

from docfilters_cli.execution.canvas_conversion import CanvasConversion
from docfilters_cli.planning.page_range import PageRange
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import ErrorCodes, UsageError


class PageRangeTests(unittest.TestCase):
    """
    --pages. The specification gives one example, 1,3-5,8; everything else here is
    a decision this suite pins down.
    """

    def test_parses_the_example_from_the_specification(self):
        page_range = PageRange.parse("1,3-5,8")

        self.assertEqual([1, 3, 4, 5, 8], list(page_range.enumerate(10)))

    def test_single_page_selects_only_that_page(self):
        page_range = PageRange.parse("4")

        self.assertTrue(page_range.contains(4))
        self.assertFalse(page_range.contains(3))
        self.assertFalse(page_range.contains(5))

    def test_pages_are_one_based(self):
        page_range = PageRange.parse("1")

        self.assertTrue(page_range.contains(1))
        self.assertFalse(page_range.contains(0))

    def test_overlapping_ranges_collapse_and_sort(self):
        # The user may write ranges in any order, with overlaps. The selection is
        # a set, so this is the same as "1,2,3".
        page_range = PageRange.parse("3,1,1-2")

        self.assertEqual([1, 2, 3], list(page_range.enumerate(10)))

    def test_a_span_of_one_page_is_allowed(self):
        page_range = PageRange.parse("2-2")

        self.assertEqual([2], list(page_range.enumerate(10)))

    def test_an_explicit_selection_is_returned_regardless_of_the_count_argument(self):
        # enumerate's argument is not a verified fact about the document — callers
        # that do not have one yet pass a placeholder — so an explicit selection is
        # never filtered by it here. Whichever of these pages a document of 6 pages
        # does not actually have is discovered when execution opens it, not guessed
        # at from a count handed to this method.
        page_range = PageRange.parse("1,5-9")

        self.assertEqual([1, 5, 6, 7, 8, 9], list(page_range.enumerate(6)))

    def test_an_explicit_selection_ignores_the_count_argument_entirely(self):
        # Not just "not clamped to 6" — genuinely unaffected by it, including a
        # count too small to hold any of the selection at all.
        page_range = PageRange.parse("10,20")

        self.assertEqual([10, 20], list(page_range.enumerate(1)))

    def test_all_selects_every_page(self):
        self.assertTrue(PageRange.ALL.is_all)
        self.assertEqual([1, 2, 3], list(PageRange.ALL.enumerate(3)))

    def test_an_explicit_range_is_not_all(self):
        self.assertFalse(PageRange.parse("1-3").is_all)

    def test_malformed_input_is_a_usage_error(self):
        for text in ["", "abc", "1,", ",1", "1--3", "1-", "-3", "1,,2"]:
            with self.subTest(text=text):
                with self.assertRaises(UsageError) as context:
                    PageRange.parse(text)

                self.assertEqual(ErrorCodes.BAD_PAGE_RANGE, context.exception.code)
                self.assertEqual(exit_code.USAGE_ERROR, context.exception.exit_code)

    def test_page_zero_is_a_usage_error(self):
        # Pages are one-based as the user writes them, so 0 is meaningless rather
        # than a synonym for the first page.
        with self.assertRaises(UsageError):
            PageRange.parse("0")

    def test_negative_pages_are_a_usage_error(self):
        with self.assertRaises(UsageError):
            PageRange.parse("-1,3")

    def test_a_descending_span_is_a_usage_error(self):
        # "5-3" is far more likely to be a typo than a request to reverse the pages,
        # so it is rejected rather than silently normalised.
        with self.assertRaises(UsageError):
            PageRange.parse("5-3")

    def test_canonical_text_form_is_stable_for_the_conformance_corpus(self):
        # The corpus compares plans as text, so two equivalent selections must
        # render identically in all four language implementations.
        self.assertEqual(str(PageRange.parse("1,2,3")), str(PageRange.parse("1-3")))

    def test_only_plain_ascii_digits_are_a_page_number(self):
        # Not in the C# suite, and needed only here: int() accepts a sign, surrounding
        # whitespace and digit-group underscores, and str.isdigit() accepts non-ASCII
        # digits, none of which the other three implementations parse. Accepting any of
        # them would let a --pages value that is a usage error everywhere else succeed
        # in Python.
        #
        # "5\n" is the same hazard one level down, in the regex rather than the conversion:
        # Python's $ matches before a trailing newline, so the pattern is anchored with \Z.
        for text in [" 5", "5 ", "+5", "5_0", "5.0", "٣", "٥-٦", "5\n", "1-3\n"]:
            with self.subTest(text=text):
                with self.assertRaises(UsageError):
                    PageRange.parse(text)

    def test_a_page_number_beyond_the_32_bit_range_is_a_usage_error(self):
        # Also Python-only: the reference parses into a 32-bit int, which overflows and
        # fails, while a Python int has no ceiling at all and would silently accept this.
        with self.assertRaises(UsageError):
            PageRange.parse("2147483648")

        self.assertEqual([2147483647], list(PageRange.parse("2147483647").enumerate(1)))

    def test_overlapping_and_touching_spans_merge(self):
        page_range = PageRange.parse("3,1,1-2,5-7,6-9")

        self.assertEqual("1-3,5-9", str(page_range))
        self.assertEqual(1, page_range.first_page)

    def test_the_widest_span_costs_nothing_until_it_is_walked(self):
        # Every page used to be expanded into a list while the arguments were read, so
        # this valid value allocated two billion entries before anything ran.
        page_range = PageRange.parse("1-2147483647")

        self.assertEqual("1-2147483647", str(page_range))
        self.assertEqual(1, page_range.first_page)
        self.assertTrue(page_range.contains(2147483647))
        self.assertFalse(page_range.contains(0))
        self.assertEqual([1, 2, 3], list(itertools.islice(page_range.enumerate(3), 3)))
        self.assertEqual([1, 2, 3], CanvasConversion.renderable_pages(page_range, 3))

    def test_a_span_ending_at_the_last_int_does_not_wrap(self):
        self.assertEqual([2147483646, 2147483647],
                         list(PageRange.parse("2147483646-2147483647").enumerate(1)))


if __name__ == '__main__':
    unittest.main()
