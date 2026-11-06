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

import io
import unittest

from docfilters_cli.configuration.command_options import ConvertOptions
from docfilters_cli.execution.canvas_conversion import CanvasConversion
from docfilters_cli.execution.document_filters_session import DocumentFiltersSession
from docfilters_cli.planning.format_catalog import FormatCatalog
from docfilters_cli.planning.page_range import PageRange
from docfilters_cli.planning.run_plan import PlannedItem
from fakes.fake_document_filters_engine import FakeDocumentFiltersEngine
from fakes.recording_console import RecordingConsole


class _ForwardOnlySink:
    """
    A write-only, forward-only sink, the way IConsole.open_standard_output actually
    behaves: real console output cannot be seeked or read back, unlike io.BytesIO, which
    would let a bug that seeks the destination directly pass silently.

    The C# subclasses System.IO.Stream and overrides CanSeek to false. Python has no such
    base class to inherit the refusal from — io.RawIOBase would supply defaults but also a
    close() that flushes, which would corrupt the very counter this exists to read — so the
    refusals are spelled out here instead. io.UnsupportedOperation is the standard-library
    analogue of NotSupportedException for a stream, and is what a real non-seekable Python
    file object raises.
    """

    def __init__(self):
        self.flush_count = 0
        self.bytes_written = 0

    def write(self, data) -> int:
        self.bytes_written += len(data)
        return len(data)

    def flush(self) -> None:
        self.flush_count += 1

    def seekable(self) -> bool:
        return False

    def readable(self) -> bool:
        return False

    def writable(self) -> bool:
        return True

    def seek(self, offset, whence=io.SEEK_SET):
        raise io.UnsupportedOperation("seek")

    def tell(self):
        # Not in the C#, which has no equivalent: there the trap is the Position property,
        # and tell() is how Python code asks the same question. Left refusing for the same
        # reason — a caller that can ask where it is will sooner or later seek there.
        raise io.UnsupportedOperation("tell")

    def truncate(self, size=None):
        raise io.UnsupportedOperation("truncate")

    def read(self, size=-1):
        raise io.UnsupportedOperation("read")


class CanvasConversionTests(unittest.TestCase):
    """
    --fingerprint's line formatting, the one part of it that does not need a real page:
    CanvasConversion.write_fingerprint reads the two page attributes and hands them to
    CanvasConversion.format_fingerprint, which is a pure function of the two strings it
    already read.

    Confirmed against a real isys_doc2text.exe --fingerprint run: IMAGE_FINGERPRINT_V2 is
    tried first, IMAGE_FINGERPRINT is the fallback, and the value itself (e.g.
    c:QlpoOTFB...) is opaque to this tool — it is printed exactly as the engine returns
    it.
    """

    def test_the_v2_fingerprint_is_used_when_present(self):
        line = CanvasConversion.format_fingerprint("sample.docx", 1, "c:v2value", "c:v1value")

        self.assertEqual("sample.docx: page=1 fingerprint=c:v2value", line)

    def test_the_v1_fingerprint_is_the_fallback(self):
        line = CanvasConversion.format_fingerprint("sample.docx", 2, "", "c:v1value")

        self.assertEqual("sample.docx: page=2 fingerprint=c:v1value", line)

    def test_a_null_v2_falls_back_to_v1_the_same_way_as_empty(self):
        line = CanvasConversion.format_fingerprint("sample.docx", 1, None, "c:v1value")

        self.assertEqual("sample.docx: page=1 fingerprint=c:v1value", line)

    def test_nothing_is_written_when_neither_attribute_has_a_value(self):
        self.assertIsNone(CanvasConversion.format_fingerprint("sample.docx", 1, "", ""))
        self.assertIsNone(CanvasConversion.format_fingerprint("sample.docx", 1, None, None))

    def test_the_page_number_is_one_based_and_the_display_name_is_used_verbatim(self):
        # A subfile's display name already carries its container, e.g.
        # "bundle.zip>inner.docx" — this only ever echoes it, never rebuilds it.
        line = CanvasConversion.format_fingerprint("bundle.zip>inner.docx", 3, "c:abc", None)

        self.assertEqual("bundle.zip>inner.docx: page=3 fingerprint=c:abc", line)

    # === canvas option composition =========================================
    #
    # The engine resolves a repeated key to its first occurrence, so the order
    # here is behaviour, not formatting: --canvas-options leads and the format's
    # own options fill in behind it.

    def test_a_format_with_no_options_of_its_own_passes_the_users_through_unchanged(self):
        png = FormatCatalog.resolve("png")

        self.assertEqual("GRAPHIC_DPI=300",
                         CanvasConversion.compose_canvas_options(png, "GRAPHIC_DPI=300"))

    def test_a_format_with_no_options_and_no_user_options_composes_to_nothing(self):
        self.assertEqual(
            "", CanvasConversion.compose_canvas_options(FormatCatalog.resolve("png"), None))
        self.assertEqual(
            "", CanvasConversion.compose_canvas_options(FormatCatalog.resolve("png"), ""))

    def test_a_formats_own_options_apply_when_the_user_gives_none(self):
        self.assertEqual(
            "HTML_INLINE_IMAGES=ON",
            CanvasConversion.compose_canvas_options(FormatCatalog.resolve("html5"), None))

    def test_the_users_options_come_first_so_they_win_the_keys_they_name(self):
        # Not cosmetic: first occurrence wins in the engine, so this is what makes
        # --canvas-options HTML_INLINE_IMAGES=OFF actually take effect.
        composed = CanvasConversion.compose_canvas_options(FormatCatalog.resolve("html5"),
                                                           "HTML_INLINE_IMAGES=OFF")

        self.assertEqual("HTML_INLINE_IMAGES=OFF;HTML_INLINE_IMAGES=ON", composed)
        # Assert.StartsWith. str.startswith is ordinal, which is what the C# default for
        # StartsWith(string) is not — it is culture-sensitive there — but every character
        # involved is ASCII, so the two agree.
        self.assertTrue(composed.startswith("HTML_INLINE_IMAGES=OFF"))

    def test_an_unrelated_user_option_leaves_the_formats_own_in_place(self):
        self.assertEqual(
            "GRAPHIC_DPI=300;HTML_INLINE_IMAGES=ON",
            CanvasConversion.compose_canvas_options(FormatCatalog.resolve("html5"),
                                                   "GRAPHIC_DPI=300"))

    def test_trailing_separators_do_not_produce_an_empty_setting(self):
        # The catalogue writes its options with a trailing ';' and users often do
        # too; an empty setting between two ';' is not worth relying on the engine
        # to ignore.
        self.assertEqual(
            "GRAPHIC_DPI=300;HTML_INLINE_IMAGES=ON",
            CanvasConversion.compose_canvas_options(FormatCatalog.resolve("html5"),
                                                   "GRAPHIC_DPI=300;"))

    def test_the_inlining_formats_carry_their_switch(self):
        # Both canvases otherwise write the page raster to a sibling file that this
        # tool never emits, leaving a transparent text layer over nothing. Confirmed
        # against isys_doc2text.exe, which forces the same two and reproduces the
        # blank output exactly when told HTML_INLINE_IMAGES=OFF.
        for token, expected in [("html5", "HTML_INLINE_IMAGES=ON"),
                                ("svg", "SVG_INLINE_IMAGES=ON")]:
            with self.subTest(token=token):
                self.assertEqual(
                    expected,
                    CanvasConversion.compose_canvas_options(FormatCatalog.resolve(token), None))

    def test_mdast_still_carries_the_schema_that_tells_it_apart_from_json(self):
        self.assertEqual(
            "JSON_OUTPUT_SCHEMA=MDAST",
            CanvasConversion.compose_canvas_options(FormatCatalog.resolve("mdast"), None))
        self.assertEqual(
            "", CanvasConversion.compose_canvas_options(FormatCatalog.resolve("json"), None))

    # -------------------------------------------------------------- --stdout

    def test_streaming_to_stdout_never_seeks_the_destination_and_flushes_it_once_done(self):
        # Reproduces a real bug found right after the --stdout NRE fix: handing
        # IConsole.open_standard_output() straight to make_canvas_on_stream produced no
        # error and no output. Every published sample that streams a canvas
        # (ConvertDocumentToTIFFStream, ConvertDocumentToInteractiveHDHTML) renders
        # into an in-memory stream instead, because finishing a binary format means
        # seeking back to patch a header computed only once rendering is done — and
        # the console's own stream is forward-only. convert_to_stream now renders into
        # its own seekable buffer and only copies the finished bytes across, so a
        # destination that would throw on any seek attempt must still work.
        engine = FakeDocumentFiltersEngine()
        engine.add_document("a.docx", "text")
        conversion = CanvasConversion(DocumentFiltersSession.for_engine(engine),
                                      RecordingConsole())
        destination = _ForwardOnlySink()
        item = PlannedItem()
        item.source_path = "a.docx"
        item.display_name = "a.docx"
        item.pages = PageRange.ALL

        # C#'s try/finally around document.Dispose(); `with` is the Python spelling of the
        # same thing, and IDocument declares __enter__/__exit__ for exactly that.
        with engine.open_document("a.docx", 0, "", lambda: None) as document:
            conversion.convert_to_stream(document, destination, item,
                                         FormatCatalog.resolve("html5"), ConvertOptions())

        self.assertTrue(destination.flush_count > 0)


if __name__ == '__main__':
    unittest.main()
