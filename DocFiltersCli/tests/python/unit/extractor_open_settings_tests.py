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
from typing import Optional

from docfilters_cli.configuration.command_options import ConvertOptions
from docfilters_cli.configuration.shared_options import SharedOptions
from docfilters_cli.execution.extractor_open_settings import ExtractorOpenSettings
from docfilters_cli.planning.format_catalog import FormatCatalog

# The C#'s private consts. Module-level private names rather than class attributes,
# following the house rule that a `private static` member ports to a module-level one.
# Written as literals on purpose: reading them from OpenType would make the test agree
# with the enum rather than with the ABI the four ports have to reproduce.
_BODY_AND_META = 2
_META_ONLY = 1
_FORMAT_TEXT = 0x00010000
_FORMAT_HTML = 0x00020000
_FORMAT_IMAGE = 0x00050000
_FORMAT_MARKDOWN = 0x00080000


def _bit(token: str, options: Optional[ConvertOptions] = None) -> int:
    # `options if options is not None else ConvertOptions()` is C#'s `options ?? new
    # ConvertOptions()`. `or` would do the same here only because a ConvertOptions has no
    # __bool__ today; `is not None` says what was meant.
    return ExtractorOpenSettings.format_bit(FormatCatalog.resolve(token),
                                            options if options is not None else ConvertOptions())


class ExtractorOpenSettingsTests(unittest.TestCase):
    """
    The open flags each command and format is opened with.

    Worth pinning by literal value rather than by enum name. The IGR_FORMAT_* constants
    are an enumeration packed into bits 16 to 19 (Text 0x1, HTML 0x2, Image 0x5, Markdown
    0x8, each shifted by 16), not independent bits, so two of them OR'd together silently
    make a third — and the numbers are the contract the Python, Java and C++ ports have to
    reproduce.
    """

    def test_a_canvas_format_opens_paginated(self):
        for token in ["png", "pdf", "html5", "tiff", "mdast"]:
            with self.subTest(token=token):
                self.assertEqual(_FORMAT_IMAGE, _bit(token))

    def test_flow_html_asks_for_the_html_reader_not_the_text_one(self):
        # FormatText here means --to html quietly emits plain text, which looks
        # plausible enough to ship.
        self.assertEqual(_FORMAT_HTML, _bit("html"))

    def test_text_mode_markdown_asks_for_the_markdown_reader(self):
        self.assertEqual(_FORMAT_MARKDOWN, _bit("md-text"))

    def test_plain_text_asks_for_the_text_reader(self):
        self.assertEqual(_FORMAT_TEXT, _bit("text"))

    def test_include_control_codes_asks_for_no_reader_at_all(self):
        # Omitting IGR_FORMAT_TEXT is the whole mechanism: it leaves the ISYS
        # control codes in the stream rather than having the engine strip them.
        #
        # Python has no object initialiser, so the field is assigned after construction,
        # as the rest of this port does.
        options = ConvertOptions()
        options.include_control_codes = True

        self.assertEqual(0, _bit("text", options))

    def test_docpack_opens_with_no_format_bit(self):
        # Matches doc2text, which opens --docpack with a bare IGR_BODY_AND_META.
        self.assertEqual(0, _bit("docpack"))

    def test_a_structure_report_does_not_change_the_format_bit(self):
        # Forcing paginated mode here was tried and does not work: it takes the
        # text reader away from the conversion opened in the same call. The pairing
        # is refused by options_validator.check_reports_need_canvas_format instead, so
        # by the time this runs a report implies a canvas format already.
        options = ConvertOptions()
        options.reports.dimensions = True
        options.reports.bookmarks = True

        self.assertEqual(_FORMAT_TEXT, _bit("text", options))
        self.assertEqual(_FORMAT_MARKDOWN, _bit("md-text", options))
        self.assertEqual(_FORMAT_IMAGE, _bit("pdf", options))

    def test_only_one_format_bit_is_ever_set(self):
        # They are an enumeration in bits 16-19, so two OR'd together make a third.
        #
        # The C# names the loop variable `format`; `spec` here, which is what
        # format_catalog_tests.py already calls it, because a loop variable named `format`
        # would shadow the builtin for the rest of the method. The production code accepts
        # that shadowing for a parameter, where the name is part of the signature the four
        # ports agree on; a local has no such claim.
        for spec in FormatCatalog.all():
            bit = ExtractorOpenSettings.format_bit(spec, ConvertOptions())
            self.assertTrue(
                bit == 0 or bit == _FORMAT_TEXT or bit == _FORMAT_HTML or bit == _FORMAT_IMAGE
                or bit == _FORMAT_MARKDOWN,
                "{0} produced an unrecognised format bit 0x{1:X}".format(spec.token, bit))

    def test_the_content_selector_is_carried_alongside_the_format_bit(self):
        settings = ExtractorOpenSettings.for_format(FormatCatalog.resolve("html"),
                                                    ConvertOptions(), SharedOptions())

        self.assertEqual(_BODY_AND_META | _FORMAT_HTML, settings.flags)

    def test_inspection_commands_open_body_and_metadata_as_text(self):
        self.assertEqual(_BODY_AND_META | _FORMAT_TEXT,
                         ExtractorOpenSettings.for_inspection(SharedOptions()).flags)

    def test_extract_images_asks_the_engine_to_enumerate_them(self):
        # ENUMERATE_IMAGES is read at open and nowhere else, so --images iterating
        # IDocument.images without it finds an empty collection every time.
        self.assertEqual(
            "ENUMERATE_IMAGES=ON;",
            ExtractorOpenSettings.for_inspection(SharedOptions(), enumerate_images=True).options)

        self.assertEqual("", ExtractorOpenSettings.for_inspection(SharedOptions()).options)

    def test_the_users_engine_options_come_before_the_one_we_add(self):
        # First occurrence wins in the engine, so --options ENUMERATE_IMAGES=OFF
        # still overrides what --images would otherwise turn on.
        shared = SharedOptions()
        shared.processing.engine_options = "ENUMERATE_IMAGES=OFF"

        self.assertEqual(
            "ENUMERATE_IMAGES=OFF;ENUMERATE_IMAGES=ON;",
            ExtractorOpenSettings.for_inspection(shared, enumerate_images=True).options)

    def test_meta_opens_metadata_only(self):
        # MetaOnly replaces BodyAndMeta rather than adding to it — the two are
        # values of the same low-bit selector, not flags.
        self.assertEqual(_META_ONLY | _FORMAT_TEXT,
                         ExtractorOpenSettings.for_metadata_only(SharedOptions()).flags)


if __name__ == '__main__':
    unittest.main()
