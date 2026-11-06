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

from docfilters_cli.execution.text_conversion import TextConversion


class TextConversionTests(unittest.TestCase):
    """
    The ISYS control codes the extractor emits, and what --to text does with them.

    Ported from doc2text's processOutput, which is the only implementation of this table
    there is. It has to live in the tool: a text-mode open resolves to the same reader
    whether IGR_FORMAT_TEXT is named or omitted, so no open flag can switch it.
    """

    def test_ordinary_text_is_untouched(self):
        self.assertEqual("Hello, world.", TextConversion.strip_control_codes("Hello, world."))

    def test_start_of_heading_becomes_a_space(self):
        # \u form throughout: C#'s \x escape is variable length, so "a\x0Eb" is the
        # single character U+0EB, not U+000E followed by 'b'. Python's \x is fixed at
        # exactly two hex digits and so has no such trap, but the \u spelling is kept
        # anyway — it is the one the reference reads with, and both ports should show
        # the same code point at the same place in the fixture.
        self.assertEqual("Title Body", TextConversion.strip_control_codes("Title\u0001Body"))

    def test_styling_marks_are_dropped(self):
        # The reference's [Theory] with one [InlineData] per code. unittest has no
        # parameterised-test attribute, so the cases become a list walked under
        # subTest: each still runs and reports independently, and a failure names the
        # code that failed rather than stopping at the first one.
        codes = [
            ('\u0002', "normal"),
            ('\u0003', "bold"),
            ('\u0004', "italic"),
            ('\u0005', "underline"),
            ('\u0006', "start-of-hit"),
            ('\u000C', "soft break"),
            ('\u0010', "end-of-meta"),
        ]

        for code, description in codes:
            with self.subTest(code=description):
                self.assertEqual("ab", TextConversion.strip_control_codes("a" + code + "b"))

    def test_paragraph_and_start_of_meta_become_newlines(self):
        self.assertEqual("a\nb", TextConversion.strip_control_codes("a\u000Eb"))
        self.assertEqual("a\nb", TextConversion.strip_control_codes("a\u000Fb"))

    def test_flow_html_keeps_its_paragraph_marks_as_nothing(self):
        # The markup already carries the structure, so doc2text suppresses the
        # newline when the open flags say HTML; a newline there would only add
        # whitespace between tags.
        self.assertEqual("<p>a</p><p>b</p>",
                         TextConversion.strip_control_codes("<p>a</p>\u000E<p>b</p>", is_flow_html=True))

    def test_start_of_meta_still_breaks_a_line_in_flow_html(self):
        self.assertEqual("a\nb", TextConversion.strip_control_codes("a\u000Fb", is_flow_html=True))

    def test_real_newlines_and_tabs_survive(self):
        # Only the ISYS codes go; line endings are the document's, not ours.
        self.assertEqual("a\r\nb\tc", TextConversion.strip_control_codes("a\r\nb\tc"))

    def test_every_code_in_one_string_is_handled_together(self):
        chunk = "\u0001H\u0002e\u0003l\u0004l\u0005o\u0006\u000E\u000Fw\u000Co\u0010rld"

        self.assertEqual(" Hello\n\nworld", TextConversion.strip_control_codes(chunk))

    def test_a_chunk_of_nothing_but_codes_strips_to_nothing(self):
        # string.Empty is "" here; Python has no named constant for it, and the
        # reference's use of one is a C# readability habit rather than a distinction.
        self.assertEqual("", TextConversion.strip_control_codes("\u0002\u0003\u0004\u000C\u0010"))
        self.assertEqual("", TextConversion.strip_control_codes(""))

    def test_stripping_is_safe_across_a_chunk_boundary(self):
        # get_text hands back 4096 characters at a time, so a code can land at
        # either edge. Every one is a single character that does not depend on its
        # neighbours, which is what makes stripping per chunk equivalent to
        # stripping the whole stream.
        whole = "one\u000Etwo\u000Ethree"

        # The reference's plain for loop, under subTest: this is one [Fact] there and
        # one test method here, but a bare loop would report only the first split that
        # broke and the split index is the whole diagnosis.
        for split in range(len(whole) + 1):
            with self.subTest(split=split):
                joined = (TextConversion.strip_control_codes(whole[:split])
                          + TextConversion.strip_control_codes(whole[split:]))

                self.assertEqual(TextConversion.strip_control_codes(whole), joined)
