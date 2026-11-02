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

from docfilters_cli.planning.name_matcher import NameMatcher
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import UsageError


class NameMatcherTests(unittest.TestCase):
    """
    -g/--glob and --regex. Glob is the default idiom; regex is the power mode for
    patterns a glob cannot express.
    """

    @staticmethod
    def _glob(*patterns):
        return NameMatcher.from_globs(list(patterns))

    @staticmethod
    def _regex(*patterns):
        return NameMatcher.from_regexes(list(patterns))

    def test_a_star_matches_within_one_path_segment(self):
        matcher = self._glob("*.pdf")

        self.assertTrue(matcher.matches("report.pdf"))
        self.assertFalse(matcher.matches("reports/report.pdf"))

    def test_a_double_star_crosses_path_segments(self):
        matcher = self._glob("**/*.pdf")

        self.assertTrue(matcher.matches("a/b/c/report.pdf"))
        self.assertFalse(matcher.matches("a/b/c/report.docx"))

    def test_a_question_mark_matches_exactly_one_character(self):
        matcher = self._glob("report?.pdf")

        self.assertTrue(matcher.matches("report1.pdf"))
        self.assertFalse(matcher.matches("report.pdf"))
        self.assertFalse(matcher.matches("report12.pdf"))

    def test_a_leading_bang_excludes(self):
        # The specification names this as the default idiom for exclusion.
        matcher = self._glob("**/*.pdf", "!**/draft/*")

        self.assertTrue(matcher.matches("final/report.pdf"))
        self.assertFalse(matcher.matches("draft/report.pdf"))

    def test_exclusions_alone_keep_everything_else(self):
        matcher = self._glob("!**/*.tmp")

        self.assertTrue(matcher.matches("report.pdf"))
        self.assertFalse(matcher.matches("scratch/a.tmp"))

    def test_an_entry_matching_several_includes_is_kept_once(self):
        matcher = self._glob("**/*.pdf", "**/report.*")

        self.assertTrue(matcher.matches("report.pdf"))

    def test_an_exclusion_beats_an_inclusion(self):
        matcher = self._glob("**/*.pdf", "!**/secret.pdf")

        self.assertFalse(matcher.matches("a/secret.pdf"))

    def test_backslashes_are_treated_as_separators_so_patterns_are_portable(self):
        # A pattern written with forward slashes must work against Windows paths,
        # otherwise every test and every corpus case is platform specific.
        matcher = self._glob("**/*.pdf")

        self.assertTrue(matcher.matches("a\\b\\report.pdf"))

    def test_regex_mode_matches_the_whole_path_not_a_substring(self):
        # Deliberately unanchored. An anchored pattern such as ".*\.pdf$" would
        # pass under substring semantics too, so it could not tell whole-match from
        # search — and the four languages differ exactly there: Python has match,
        # search and fullmatch; .NET's IsMatch searches; Java's matches() does not.
        matcher = self._regex(r"report\.pdf")

        self.assertTrue(matcher.matches("report.pdf"))
        self.assertFalse(matcher.matches("a/report.pdf"))
        self.assertFalse(matcher.matches("old-report.pdf"))
        self.assertFalse(matcher.matches("report.pdf.bak"))

    def test_a_regex_can_still_match_a_full_path_when_written_to(self):
        matcher = self._regex(r"a/b/report\.pdf")

        self.assertTrue(matcher.matches("a/b/report.pdf"))
        self.assertFalse(matcher.matches("x/a/b/report.pdf"))

    def test_regex_mode_expresses_what_a_glob_cannot(self):
        # Alternation with a bounded repeat: the reason --regex exists at all.
        matcher = self._regex(r"^report-\d{4}\.(pdf|docx)$")

        self.assertTrue(matcher.matches("report-2026.pdf"))
        self.assertTrue(matcher.matches("report-2026.docx"))
        self.assertFalse(matcher.matches("report-26.pdf"))

    def test_an_invalid_regex_is_a_usage_error(self):
        with self.assertRaises(UsageError) as context:
            self._regex("[unclosed")

        self.assertEqual(exit_code.USAGE_ERROR, context.exception.exit_code)

    def test_canonical_text_form_is_stable_for_the_conformance_corpus(self):
        self.assertEqual(str(self._glob("**/*.pdf")), str(self._glob("**/*.pdf")))

    def test_a_trailing_newline_does_not_satisfy_the_end_anchor(self):
        # Not in the C# suite, and needed only here: the reference anchors with \z,
        # which Python spells \Z and does not have under its own name. Reaching for $
        # instead — the obvious Python substitution — would let a path ending in a
        # newline match, because Python's $ also matches just before a final newline.
        self.assertFalse(self._glob("*.pdf").matches("report.pdf\n"))
        self.assertFalse(self._regex(r"report\.pdf").matches("report.pdf\n"))


if __name__ == '__main__':
    unittest.main()
