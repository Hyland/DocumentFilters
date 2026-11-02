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

import unittest
from typing import List

from docfilters_cli.execution.metadata_report import MetadataReport
from fakes.fake_document_filters_engine import FakeDocumentFiltersEngine
from fakes.recording_console import RecordingConsole


class MetadataReportTests(unittest.TestCase):
    """
    How meta turns the engine's metadata blob into one record per field.

    There is no structured metadata API — get_text returns the fields as one run of
    text, and the record separator is not a newline. The engine emits 0x0F before the
    block and 0x0E between records, which is why these fixtures are built from control
    codes rather than \\n: a parser that splits on newlines finds no boundary at all and
    reports every field of a document as one.
    """

    # The reference's `private const char` trio, as class attributes: Python has no
    # const, and a module-level name would put them further from the only tests that
    # use them than the C# puts them.
    _START_OF_META = '\u000F'
    _SEPARATOR = '\u000E'
    _END_OF_META = '\u0010'

    def _report(self, metadata: str) -> List[str]:
        engine = FakeDocumentFiltersEngine()
        engine.add_document("a.docx", metadata)
        console = RecordingConsole()

        # `using IDocument document = ...` is `with ... as document:`; IDocument declares
        # __enter__/__exit__ for exactly this. The password callback is a lambda
        # returning None, which is the reference's `() => null`.
        with engine.open_document("a.docx", 0, "", lambda: None) as document:
            MetadataReport().write_text(document, "a.docx", console)

        return console.out_lines

    def test_records_separated_by_control_codes_each_become_one_line(self):
        # The real shape, from a PDF: 0x0F opens the block, 0x0E divides it, 0x10
        # closes it. Splitting this on '\n' yields a single bogus field whose label
        # still carries the 0x0F.
        lines = self._report(self._START_OF_META + "Title: Report" + self._SEPARATOR + "Author: Ada"
                             + self._SEPARATOR + "Producer: Quartz" + self._END_OF_META)

        self.assertEqual([
            "a.docx: field=Title value=Report",
            "a.docx: field=Author value=Ada",
            "a.docx: field=Producer value=Quartz",
        ], lines)

    def test_a_value_carrying_a_newline_stays_one_record(self):
        # A Word Title or
        # Comments field routinely has one; the continuation used to be dropped,
        # silently truncating the value.
        lines = self._report(self._START_OF_META + "Title: Quarterly Report\nsecond line"
                             + self._SEPARATOR + "Author: Ada")

        self.assertEqual([
            "a.docx: field=Title value=Quarterly Report second line",
            "a.docx: field=Author value=Ada",
        ], lines)

    def test_carriage_returns_and_tabs_inside_a_value_become_single_spaces(self):
        lines = self._report(self._START_OF_META + "Comments: one\ttwo\r\nthree")

        self.assertEqual(["a.docx: field=Comments value=one two three"], lines)

    def test_a_label_containing_colons_is_not_cut_apart(self):
        # Splitting on the first ": " rather than the first ':' is what keeps an
        # XMP label whole — none of its internal colons is followed by a space.
        lines = self._report(self._START_OF_META + "xmp:dc:creator: slop001")

        self.assertEqual(["a.docx: field=xmp:dc:creator value=slop001"], lines)

    def test_a_document_with_no_metadata_reports_nothing(self):
        # Assert.Empty has no unittest counterpart that also says what the subject is,
        # so it becomes a comparison against the empty list: out_lines is always a list,
        # and assertFalse on it would pass just as happily for None.
        self.assertEqual([], self._report(""))

        # The C# needs StartOfMeta.ToString() here because char + char is integer
        # addition there. Python has no such promotion — these are one-character str
        # values and + concatenates — so no conversion has a counterpart.
        self.assertEqual([], self._report(self._START_OF_META + self._END_OF_META))

    def test_a_continuation_with_no_field_before_it_is_ignored(self):
        # Nothing to attach it to, so it is dropped rather than inventing a field.
        self.assertEqual([], self._report("loose text with no label"))
