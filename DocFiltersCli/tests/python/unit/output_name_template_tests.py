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
import unittest

from conformance import fixture
from docfilters_cli.planning.output_name_template import OutputNameTemplate
from docfilters_cli.runtime.cli_error import UsageError

class _RenderRow:
    """One "= render source ext page => expected" row."""

    def __init__(self, source, extension, page, expected):
        self.source = source
        self.extension = extension
        self.page = page
        self.expected = expected

    @staticmethod
    def parse(row, test_case):
        halves = row.split("=>")

        if len(halves) != 2:
            raise ValueError(test_case.describe("render row has no '=>': {0}".format(row)))

        # Space and tab only, the two characters the fixtures pad their columns with; a
        # plain split() would also break on any other Unicode space in a source name.
        inputs = [part for part in re.split(r'[ \t]+', halves[0]) if part]

        if len(inputs) != 3:
            raise ValueError(test_case.describe("render row needs 'source ext page => expected': {0}".format(row)))

        return _RenderRow(inputs[0], inputs[1], int(inputs[2]), halves[1].strip())


class OutputNameTemplateTests(unittest.TestCase):
    """
    --out-name, checked against the shared out-names.txt fixture.

    The render cases are tabular -- template, source, extension, page, expected
    filename -- so they live in the fixture and all four implementations run them. What
    stays here is behaviour a row cannot express: the defaults, and the guarantee that a
    rendered name is a filename and never a path.
    """

    def test_template_matches_the_shared_fixture(self):
        for test_case in fixture.load('out-names.txt'):
            with self.subTest(case=str(test_case)):
                if test_case.error is not None:
                    with self.assertRaises(UsageError) as caught:
                        OutputNameTemplate.parse(test_case.subject)

                    self.assertTrue(
                        caught.exception.code == test_case.error,
                        test_case.describe("expected error {0}, got {1}".format(test_case.error, caught.exception.code)))
                    continue

                template = OutputNameTemplate.parse(test_case.subject)

                if test_case.has("uses-page"):
                    expected = test_case.get_bool("uses-page")
                    self.assertTrue(
                        template.uses_page == expected,
                        test_case.describe("uses-page was {0}, expected {1}".format(
                            "yes" if template.uses_page else "no", "yes" if expected else "no")))

                for row in test_case.get_all("render"):
                    expectation = _RenderRow.parse(row, test_case)
                    actual = template.render(expectation.source, expectation.extension, expectation.page)

                    self.assertTrue(
                        actual == expectation.expected,
                        test_case.describe("render {0} {1} {2} produced '{3}', expected '{4}'".format(
                            expectation.source, expectation.extension, expectation.page, actual, expectation.expected)))

    # ------------------------------------------- behaviour the fixture cannot state

    def test_default_for_a_single_file_format_has_no_page_placeholder(self):
        template = OutputNameTemplate.default(is_single_file=True)

        self.assertFalse(template.uses_page)
        self.assertEqual("report.pdf", template.render("report.docx", "pdf", 1))

    def test_default_for_a_one_per_page_format_includes_a_padded_page_number(self):
        # Without this, every page of a PNG conversion would overwrite the last.
        template = OutputNameTemplate.default(is_single_file=False)

        self.assertTrue(template.uses_page)
        self.assertEqual("report-0001.png", template.render("report.docx", "png", 1))

    def test_rendering_never_emits_a_path_separator(self):
        # The template names a file inside --out-dir. Whatever the input path looked
        # like, the result must not be able to escape that directory. This is an
        # assertion about the output rather than an expected value, so it cannot be
        # a fixture row.
        for source in ["../../etc/passwd",
                       "..\\..\\windows\\system32\\config",
                       "/absolute/path/report.docx"]:
            with self.subTest(source=source):
                template = OutputNameTemplate.parse("{stem}.{ext}")

                rendered = template.render(source, "txt", 1)

                self.assertNotIn("/", rendered)
                self.assertNotIn("\\", rendered)
                self.assertNotIn("..", rendered)

    def test_two_pages_of_the_same_document_never_render_to_the_same_name(self):
        # The property that makes a one-per-page format safe at all.
        template = OutputNameTemplate.default(is_single_file=False)

        seen = set()

        for page in range(1, 201):
            name = template.render("report.docx", "png", page)
            self.assertNotIn(name, seen, "duplicate name at page " + str(page))
            seen.add(name)

    def test_the_original_text_is_recoverable_for_the_plan_dump(self):
        text = "{stem}-{page:04}.{ext}"

        self.assertEqual(text, str(OutputNameTemplate.parse(text)))

    def test_only_plain_ascii_digits_are_a_padding_width(self):
        # Python-only, like the matching test in PageRangeTests: int() accepts a sign,
        # surrounding whitespace and digit-group underscores, str.isdigit() accepts
        # non-ASCII digits, and Python's $ matches before a trailing newline. Each would
        # make a template here that the other three ports call malformed.
        for width in [" 4", "4 ", "+4", "0_4", "4.0", "٤", "4\n"]:
            with self.subTest(width=width):
                with self.assertRaises(UsageError):
                    OutputNameTemplate.parse("{stem}-{page:" + width + "}.{ext}")

        # The well-formed spelling of the same width still parses.
        self.assertEqual("{stem}-{page:4}.{ext}", str(OutputNameTemplate.parse("{stem}-{page:4}.{ext}")))


if __name__ == '__main__':
    unittest.main()
