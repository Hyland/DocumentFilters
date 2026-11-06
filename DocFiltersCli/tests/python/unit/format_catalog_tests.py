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

from conformance import fixture
from docfilters_cli.planning.format_catalog import FormatCatalog
from docfilters_cli.planning.format_spec import FormatKind
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import UsageError

class FormatCatalogTests(unittest.TestCase):
    """
    The --to token table, checked against the shared formats.txt fixture.

    The table is data, and the C#, Java and C++17 implementations need exactly the
    same data, so it lives in the fixture rather than being transcribed into four test
    suites. What stays here is the behaviour the fixture cannot express:
    catalogue-wide invariants, and how lookup failures are reported.
    """

    def test_token_matches_the_shared_catalogue(self):
        for test_case in fixture.load('formats.txt'):
            with self.subTest(case=str(test_case)):
                token = test_case.subject

                if test_case.error is not None:
                    with self.assertRaises(UsageError) as caught:
                        FormatCatalog.resolve(token)

                    self.assertEqual(
                        test_case.error, caught.exception.code,
                        test_case.describe("expected error " + test_case.error
                                           + ", got " + caught.exception.code))
                    continue

                spec = FormatCatalog.resolve(token)

                self.assertIsNotNone(spec, test_case.describe("token did not resolve"))

                # The canonical token is the folded one: the fixture states PDF and Tiff
                # as their own cases, and both must land on the lower-case entry.
                self.assertEqual(
                    _fold(token), spec.token,
                    test_case.describe("canonical token was '" + spec.token
                                       + "', expected '" + _fold(token) + "'"))

                expected_kind = _parse_kind(test_case)
                self.assertEqual(
                    expected_kind, spec.kind,
                    test_case.describe("kind was " + spec.kind.value + ", expected " + expected_kind.value))

                single_file = test_case.get_bool('single-file')
                self.assertEqual(
                    single_file, spec.is_single_file,
                    test_case.describe("single-file was " + _yes(spec.is_single_file)
                                       + ", expected " + _yes(single_file)))

                combine = test_case.get_bool('combine')
                self.assertEqual(
                    combine, spec.supports_combine,
                    test_case.describe("combine was " + _yes(spec.supports_combine)
                                       + ", expected " + _yes(combine)))

                hidef = test_case.get_bool('hidef')
                self.assertEqual(
                    hidef, spec.is_hi_def_capable,
                    test_case.describe("hidef was " + _yes(spec.is_hi_def_capable)
                                       + ", expected " + _yes(hidef)))

                ext = test_case.get('ext')
                self.assertEqual(
                    ext, spec.extension,
                    test_case.describe("ext was '" + spec.extension + "', expected '" + ext + "'"))

                # 'none' rather than an empty value, so the row is visible in the fixture.
                options = '' if test_case.get('options') == 'none' else test_case.get('options')
                self.assertEqual(
                    options, spec.canvas_options or '',
                    test_case.describe("options were '" + str(spec.canvas_options)
                                       + "', expected '" + options + "'"))

    # ------------------------------------------- behaviour the fixture cannot state

    def test_the_catalogue_has_an_entry_for_every_resolvable_token(self):
        # Guards against a token that resolves but is missing from all(), which would
        # leave it out of the help text and the formats listing.
        for test_case in fixture.load('formats.txt'):
            if test_case.error is not None:
                continue

            self.assertTrue(
                any(f.token == _fold(test_case.subject) for f in FormatCatalog.all()),
                test_case.describe("resolves, but is absent from FormatCatalog.all()"))

    def test_the_catalogue_declares_nothing_the_fixture_does_not_list(self):
        # The reverse direction: an implementation must not quietly support a format the
        # other three have never heard of.
        expected = set(c.subject for c in fixture.load('formats.txt') if c.error is None)

        for spec in FormatCatalog.all():
            self.assertIn(spec.token, expected,
                          "'" + spec.token + "' is in the catalogue but not in formats.txt")

    def test_every_catalogue_entry_has_a_unique_token(self):
        seen = set()

        for spec in FormatCatalog.all():
            self.assertNotIn(spec.token, seen, "duplicate token: " + spec.token)
            seen.add(spec.token)

    def test_every_catalogue_entry_has_a_display_name_for_help_text(self):
        # Not in the fixture because it is prose, and prose is allowed to differ between
        # languages in a way that ids and extensions are not.
        for spec in FormatCatalog.all():
            self.assertTrue(spec.display_name, spec.token + " has no display name")

    def test_tokens_are_matched_case_insensitively_but_canonicalise_to_lower_case(self):
        # Users type --to PDF; the plan dump must still say pdf, or the corpus comparison
        # becomes case sensitive by accident.
        self.assertEqual("pdf", FormatCatalog.resolve("PDF").token)
        self.assertEqual("md-text", FormatCatalog.resolve("MD-Text").token)

    def test_the_error_names_the_token_and_where_to_find_the_valid_ones(self):
        # The message points at the formats command rather than listing the catalogue
        # inline. Two dozen tokens in an error string would have to stay byte-identical
        # across four languages and be edited every time the catalogue grows; a pointer
        # does not.
        with self.assertRaises(UsageError) as caught:
            FormatCatalog.resolve("pdxf")

        self.assertIn("pdxf", caught.exception.message)
        self.assertIn("formats", caught.exception.message)

    def test_an_unknown_token_exits_with_the_usage_code(self):
        with self.assertRaises(UsageError) as caught:
            FormatCatalog.resolve("pdxf")

        self.assertEqual(exit_code.USAGE_ERROR, caught.exception.exit_code)

    def test_tryresolve_returns_null_rather_than_throwing(self):
        self.assertIsNone(FormatCatalog.try_resolve("nonsense"))
        self.assertIsNotNone(FormatCatalog.try_resolve("pdf"))

    def test_a_text_mode_format_is_never_high_definition_capable(self):
        # A structural invariant rather than a per-row fact: if these ever disagreed,
        # --fallback would have nothing coherent to fall back to.
        for spec in FormatCatalog.all():
            if spec.kind == FormatKind.TEXT:
                self.assertFalse(spec.is_hi_def_capable,
                                 spec.token + " is text mode but claims hi-def capability")


def _fold(token):
    """
    The ASCII-only fold the catalogue matches tokens with.

    Written out here rather than imported from value_parsers, exactly as the C# suite
    spells it out rather than calling ValueParsers.FoldToken: a test that reuses the
    implementation's own fold cannot catch the implementation folding wrongly.
    """
    return ''.join(chr(ord(c) - ord('A') + ord('a')) if 'A' <= c <= 'Z' else c for c in token)


def _parse_kind(test_case):
    kind = test_case.get('kind')

    if kind == 'text':
        return FormatKind.TEXT
    if kind == 'canvas':
        return FormatKind.CANVAS
    if kind == 'docpack':
        return FormatKind.DOC_PACK

    raise ValueError(test_case.describe("unknown 'kind'"))


def _yes(value):
    return "yes" if value else "no"


if __name__ == '__main__':
    unittest.main()
