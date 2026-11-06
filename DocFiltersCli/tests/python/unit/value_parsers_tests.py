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

from docfilters_cli.configuration import value_parsers
from docfilters_cli.configuration.shared_options import OnErrorBehaviour
from docfilters_cli.runtime.cli_error import ErrorCodes, UsageError


class ValueParsersTests(unittest.TestCase):
    """
    The conversions that can fail. These live in shared code precisely so that a bad
    value produces the same message and the same exit code in the C#, Python, Java and
    C++17 builds, rather than four different parsing libraries' wording.
    """

    def test_counts_parse(self):
        for text, expected in [("0", 0), ("1", 1), ("42", 42)]:
            with self.subTest(text=text):
                self.assertEqual(expected, value_parsers.parse_count("--max-depth", text))

    def test_a_bad_count_is_a_usage_error(self):
        for text in ["abc", "", "-1", "1.5", "1e3", " 1"]:
            with self.subTest(text=text):
                with self.assertRaises(UsageError) as caught:
                    value_parsers.parse_count("--max-depth", text)

                self.assertEqual(ErrorCodes.BAD_VALUE, caught.exception.code)

    def test_a_trailing_newline_is_a_bad_value_for_every_strict_parser(self):
        # No C# counterpart, because the hazard cannot arise there: int.TryParse with
        # NumberStyles.None has no AllowTrailingWhite, so "5\n" is simply not a number.
        # Python's $ does match immediately before a trailing newline, so the guards are
        # anchored with \Z instead, and this is what fails if one of them regresses to $.
        for parse in [value_parsers.parse_count, value_parsers.parse_seconds, value_parsers.parse_megabytes]:
            with self.subTest(parse=parse.__name__):
                with self.assertRaises(UsageError) as caught:
                    parse("--max-depth", "5\n")

                self.assertEqual(ErrorCodes.BAD_VALUE, caught.exception.code)

    def test_the_error_names_the_argument_that_was_wrong(self):
        # Without the argument name, a batch of options makes the message useless.
        with self.assertRaises(UsageError) as caught:
            value_parsers.parse_count("--max-depth", "abc")

        self.assertIn("--max-depth", caught.exception.message)

    # ------------------------------------------------------------------------ seconds

    def test_durations_parse(self):
        for text, expected in [("1", 1), ("60", 60), ("86400", 86400)]:
            with self.subTest(text=text):
                self.assertEqual(expected, value_parsers.parse_seconds("--timeout", text))

    def test_a_bad_duration_is_a_usage_error(self):
        # Zero is rejected as well as negative: a timeout of nothing is a mistake, not a
        # request to finish instantly.
        for text in ["0", "-1", "abc", "", "1.5", "30s"]:
            with self.subTest(text=text):
                with self.assertRaises(UsageError) as caught:
                    value_parsers.parse_seconds("--timeout", text)

                self.assertEqual(ErrorCodes.BAD_VALUE, caught.exception.code)

    # --------------------------------------------------------------------- megabytes

    def test_memory_limits_parse(self):
        for text, expected in [("1", 1), ("512", 512), ("8192", 8192)]:
            with self.subTest(text=text):
                self.assertEqual(expected, value_parsers.parse_megabytes("--max-memory", text))

    def test_a_memory_limit_beyond_int_range_still_parses(self):
        # The value is megabytes and the field is a long, so a limit larger than
        # int.MaxValue is legitimate rather than an overflow.
        self.assertEqual(4194304, value_parsers.parse_megabytes("--max-memory", "4194304"))

    def test_a_bad_memory_limit_is_a_usage_error(self):
        for text in ["0", "-1", "abc", "", "512MB", "1.5"]:
            with self.subTest(text=text):
                with self.assertRaises(UsageError) as caught:
                    value_parsers.parse_megabytes("--max-memory", text)

                self.assertEqual(ErrorCodes.BAD_VALUE, caught.exception.code)

    def test_both_limit_parsers_name_the_argument_that_was_wrong(self):
        with self.assertRaises(UsageError) as caught:
            value_parsers.parse_seconds("--timeout", "x")

        self.assertIn("--timeout", caught.exception.message)

        with self.assertRaises(UsageError) as caught:
            value_parsers.parse_megabytes("--max-memory", "x")

        self.assertIn("--max-memory", caught.exception.message)

    # ---------------------------------------------------------------------- id lists

    def test_id_lists_split_on_semicolons(self):
        self.assertEqual(["PDF", "DOCX"], value_parsers.parse_id_list("PDF;DOCX"))

    def test_id_lists_trim_whitespace_and_drop_empty_entries(self):
        self.assertEqual(["PDF", "DOCX"], value_parsers.parse_id_list(" PDF ; ; DOCX ;"))

    def test_a_single_id_needs_no_separator(self):
        self.assertEqual(["PDF"], value_parsers.parse_id_list("PDF"))

    # ------------------------------------------------------------------------- trimming

    def test_trim_removes_the_whitespace_the_reference_calls_whitespace(self):
        # Python-only, like the \Z anchors: these are the characters both implementations
        # agree about, so trim has to keep removing all of them.
        for text in [" PDF ", "\tPDF\t", "\r\nPDF\r\n", "\x0bPDF\x0c", "\x85PDF\x85",
                     "\xa0PDF\xa0", " PDF ", "　PDF　"]:
            with self.subTest(text=text):
                self.assertEqual("PDF", value_parsers.trim(text))

    def test_trim_keeps_the_c0_information_separators(self):
        # The one place char.IsWhiteSpace and str.isspace() disagree. .NET's Trim leaves
        # FS, GS, RS and US alone, so an id or a log module name ending in one of them is
        # the same token in all four ports rather than a shorter one here.
        for character in ['\x1c', '\x1d', '\x1e', '\x1f']:
            with self.subTest(character=character):
                self.assertFalse(value_parsers.is_whitespace(character))
                self.assertEqual("PDF" + character, value_parsers.trim("PDF" + character))
                self.assertEqual([character], value_parsers.parse_id_list(character))

        # str.strip() is what this is not, and the contrast is the whole point.
        self.assertEqual("PDF", "PDF\x1c".strip())

    def test_trim_of_nothing_but_whitespace_is_empty(self):
        self.assertEqual("", value_parsers.trim("  \t\r\n "))
        self.assertEqual("", value_parsers.trim(""))

    def test_is_all_whitespace_matches_is_null_or_white_space(self):
        for text in [None, "", " ", "\t\r\n"]:
            with self.subTest(text=text):
                self.assertTrue(value_parsers.is_all_whitespace(text))

        for text in ["a", " a ", "\x1c"]:
            with self.subTest(text=text):
                self.assertFalse(value_parsers.is_all_whitespace(text))

    # -------------------------------------------------------------------- redactions

    def test_a_redaction_parses_as_page_then_rectangle(self):
        spec = value_parsers.parse_redaction("2,10,20,100,50")

        self.assertEqual(2, spec.page)
        self.assertEqual(10, spec.x)
        self.assertEqual(20, spec.y)
        self.assertEqual(100, spec.width)
        self.assertEqual(50, spec.height)

    def test_a_malformed_redaction_is_a_usage_error(self):
        for text in ["2,10,20,100", "2,10,20,100,50,60", "2,10,20,100,abc", ""]:
            with self.subTest(text=text):
                with self.assertRaises(UsageError):
                    value_parsers.parse_redaction(text)

    # ------------------------------------------------------------------- form values

    def test_a_form_value_parses_as_page_then_name_equals_value(self):
        value = value_parsers.parse_form_value("1,surname=Smith")

        self.assertEqual(1, value.page)
        self.assertEqual("surname", value.name)
        self.assertEqual("Smith", value.value)

    def test_only_the_first_equals_separates_name_from_value(self):
        # Form values legitimately contain '=', for instance a base64 payload.
        value = value_parsers.parse_form_value("1,token=abc=def=")

        self.assertEqual("token", value.name)
        self.assertEqual("abc=def=", value.value)

    def test_an_empty_form_value_is_allowed(self):
        # Clearing a field is a legitimate operation.
        value = value_parsers.parse_form_value("1,surname=")

        self.assertEqual("", value.value)

    def test_a_malformed_form_value_is_a_usage_error(self):
        for text in ["1,surname", "surname=Smith", ""]:
            with self.subTest(text=text):
                with self.assertRaises(UsageError):
                    value_parsers.parse_form_value(text)

    # ----------------------------------------------------------------------- on-error

    def test_on_error_accepts_exactly_the_two_documented_values(self):
        for text, expected in [("skip", OnErrorBehaviour.SKIP), ("stop", OnErrorBehaviour.STOP)]:
            with self.subTest(text=text):
                self.assertEqual(expected, value_parsers.parse_on_error(text))

    def test_anything_else_for_on_error_is_a_usage_error(self):
        # Case sensitive on purpose: if one implementation accepted "SKIP" and another
        # did not, the four would diverge on what is a valid command line.
        for text in ["SKIP", "Skip", "continue", ""]:
            with self.subTest(text=text):
                with self.assertRaises(UsageError):
                    value_parsers.parse_on_error(text)


if __name__ == '__main__':
    unittest.main()
