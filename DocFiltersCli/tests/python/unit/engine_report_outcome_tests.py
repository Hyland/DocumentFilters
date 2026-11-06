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
from typing import Iterable, List, Optional

from docfilters_cli.configuration.command_options import (FormatsOptions, OptionCatalogOptions,
                                                          StructureReportOptions)
from docfilters_cli.engine.engine_exception import EngineError, EngineException
from docfilters_cli.engine.i_document_filters_engine import (AnnotationKind, EngineAnnotation,
                                                             EngineBookmark, EngineFormat,
                                                             EngineFormElement, EngineHyperlink,
                                                             EngineOption, EnginePageElement,
                                                             EngineRectangle, FormElementType,
                                                             HyperlinkActionType, PageElementType)
from docfilters_cli.execution.catalog_reports import CatalogReports
from docfilters_cli.execution.document_filters_session import DocumentFiltersSession
from docfilters_cli.execution.structure_report import StructureReport
from fakes.fake_document_filters_engine import (FakeDocument, FakeDocumentFiltersEngine,
                                                FakePage)
from fakes.recording_console import RecordingConsole
from unit.engine_outcome_tests import _every_engine_error


class EngineReportOutcomeTests(unittest.TestCase):
    """
    The ten structure reports, against what the engine gives them.

    Every one of these reads something only an opened, laid-out document has, so none of
    them could be tested before the engine had a seam. What is checked here is the shape of
    each record -- these tests are the authority on that -- and, as much, what each does
    with nothing: a document with no bookmarks, a page with no hyperlinks, an attribute the
    engine returns empty. A report that printed a header for an empty collection, or
    crashed on a None, would be wrong in a way no conversion test would notice.
    """

    def _single(self, lines: List[str]) -> str:
        """assertEqual lives on the TestCase, so the reference's Assert.Single is a method."""
        self.assertEqual(1, len(lines), "expected exactly one line, got {0!r}".format(lines))
        return lines[0]

    # ============================================ document-level

    def test_fonts_come_from_the_last_page_attribute_and_split_on_semicolons(self):
        """
        Fonts are not a collection in the binding.

        They arrive as the semicolon-separated DOCUMENT_FONTS attribute of the last page,
        which is the same route doc2text takes. That makes the splitting this report's own
        job, and worth pinning.
        """
        document = FakeDocument("sample.docx")
        document.page_count = 2
        document.pages.append(FakePage())
        document.pages.append(FakePage())
        document.pages[1].attributes["DOCUMENT_FONTS"] = "Calibri;Cambria Math"

        console = RecordingConsole()
        _report(console, fonts=True).report_document(document, "sample.docx")

        self.assertEqual(["sample.docx: font=Calibri", "sample.docx: font=Cambria Math"],
                         console.out_lines)

    def test_a_document_with_no_fonts_prints_nothing(self):
        document = FakeDocument("sample.docx")
        document.page_count = 1

        console = RecordingConsole()
        _report(console, fonts=True).report_document(document, "sample.docx")

        self.assertEqual([], console.out_lines)

    def test_a_document_with_no_pages_at_all_reports_no_fonts_rather_than_failing(self):
        # A text-only format has no laid-out pages, so there is no last page to ask.
        document = FakeDocument("plain.txt")
        document.page_count = 0

        console = RecordingConsole()
        _report(console, fonts=True).report_document(document, "plain.txt")

        self.assertEqual([], console.out_lines)

    def test_bookmarks_are_walked_depth_first_with_their_level_and_one_based_page(self):
        document = FakeDocument("sample.docx")

        # A bookmark's page number only exists once the document has been laid out, so
        # write_bookmarks skips a document with no pages at all -- see
        # test_bookmarks_are_skipped_for_a_document_with_no_pages below.
        document.page_count = 4
        document.bookmarks = _bookmark("root", children=[
            _bookmark("Introduction", level=0, page_index=0),
            _bookmark("Methodology", level=0, page_index=2, children=[
                _bookmark("Sampling", level=1, page_index=3)])])

        console = RecordingConsole()
        _report(console, bookmarks=True).report_document(document, "sample.docx")

        self.assertEqual(["sample.docx: bookmark=1 level=0 page=1 title=Introduction",
                          "sample.docx: bookmark=2 level=0 page=3 title=Methodology",
                          "sample.docx: bookmark=3 level=1 page=4 title=Sampling"],
                         console.out_lines)

    def test_a_document_with_no_outline_prints_no_bookmarks(self):
        # bookmarks is None for a document with no outline, which is most of them.
        # page_count is set so this exercises that path specifically, rather than the
        # page-count guard below for the same empty result.
        document = FakeDocument("plain.txt")
        document.page_count = 1

        console = RecordingConsole()
        _report(console, bookmarks=True).report_document(document, "plain.txt")

        self.assertEqual([], console.out_lines)

    def test_bookmarks_are_skipped_for_a_document_with_no_pages(self):
        # Reproduces a real bug: a bookmark's page number only exists once the document
        # has been laid out, so a handle opened without IGR_FORMAT_IMAGE -- --to text
        # --bookmarks, where the text reader needs the format bit bookmarks would
        # otherwise take -- was never laid out at all. IDocument.get_root_bookmark does
        # not degrade to "nothing" on its own for that case the way the last-page
        # attribute does; it throws the binding's own "Invalid DocHandle". This guard
        # applies the same "reports nothing rather than failing" rule the binding does not.
        document = FakeDocument("sample.docx")
        document.page_count = 0
        document.bookmarks = _bookmark("root", children=[
            _bookmark("Introduction", level=0, page_index=0)])

        console = RecordingConsole()
        _report(console, bookmarks=True).report_document(document, "sample.docx")

        self.assertEqual([], console.out_lines)

    def test_annotations_are_reported_with_their_page_kind_author_and_text(self):
        document = FakeDocument("sample.docx")
        document.page_count = 2
        document.pages.append(FakePage())
        document.pages.append(FakePage())

        annotation = EngineAnnotation()
        annotation.type = AnnotationKind.TEXT
        annotation.title = "Jane Smith"
        annotation.text = "Check this figure"
        document.pages[1].annotation_list.append(annotation)

        console = RecordingConsole()
        _report(console, annotations=True).report_document(document, "sample.docx")

        line = self._single(console.out_lines)
        self.assertIn("annotation=1", line)
        self.assertIn("page=2", line)
        self.assertIn("author=Jane Smith", line)
        self.assertIn("text=Check this figure", line)

    # ============================================ page-level

    def test_dimensions_come_from_the_page_itself(self):
        console = RecordingConsole()
        page = FakePage()
        page.width = 612
        page.height = 792

        _report(console, dimensions=True).report_page(page, 1, "sample.docx")

        self.assertEqual("sample.docx: page=1 width=612 height=792",
                         self._single(console.out_lines))

    def test_hyperlinks_report_a_uri_target_or_a_ref_target_but_never_both(self):
        page = FakePage()

        external = EngineHyperlink()
        external.type = HyperlinkActionType.URI
        external.uri = "https://example.com"
        external.ref = "ignored"
        external.x = 100
        external.y = 200
        external.width = 80
        external.height = 12
        page.hyperlink_list.append(external)

        internal = EngineHyperlink()
        internal.type = HyperlinkActionType.GOTO
        internal.uri = "ignored"
        internal.ref = "page-3"
        internal.x = 1
        internal.y = 2
        internal.width = 3
        internal.height = 4
        page.hyperlink_list.append(internal)

        console = RecordingConsole()
        _report(console, hyperlinks=True).report_page(page, 1, "sample.docx")

        self.assertIn("uri=https://example.com", console.out_lines[0])
        self.assertNotIn("ref=", console.out_lines[0])
        self.assertIn("ref=page-3", console.out_lines[1])
        self.assertNotIn("uri=", console.out_lines[1])

    def test_form_elements_report_their_geometry_and_name(self):
        page = FakePage()

        element = EngineFormElement()
        element.type = FormElementType.SINGLELINE_TEXT
        element.name = "surname"
        element.x = 72
        element.y = 144
        element.width = 200
        element.height = 20
        page.form_element_list.append(element)

        console = RecordingConsole()
        _report(console, form_elements=True).report_page(page, 1, "sample.docx")

        line = self._single(console.out_lines)
        self.assertIn("form-element=1", line)
        self.assertIn("x=72 y=144 w=200 h=20", line)
        self.assertIn("name=surname", line)

    def test_page_elements_are_flattened_depth_first_with_their_depth(self):
        """
        The element tree is reported flattened, depth first, with each node's own depth.

        So a nested node appears once, after its parent, not inside it.
        """
        page = FakePage()
        page.root_element = _element(
            PageElementType.PAGE, 0, EngineRectangle(0, 0, 612, 792), children=[
                _element(PageElementType.PARAGRAPH, 1, EngineRectangle(72, 72, 468, 14),
                         children=[
                             _element(PageElementType.RUN, 2,
                                      EngineRectangle(72, 72, 100, 14))])])

        console = RecordingConsole()
        _report(console, page_elements=True).report_page(page, 1, "sample.docx")

        self.assertEqual(2, len(console.out_lines))
        self.assertIn("element=1", console.out_lines[0])
        self.assertIn("depth=1", console.out_lines[0])
        self.assertIn("x=72 y=72 w=468 h=14", console.out_lines[0])
        self.assertIn("element=2", console.out_lines[1])
        self.assertIn("depth=2", console.out_lines[1])

    def test_a_page_that_exposes_no_element_tree_prints_nothing(self):
        # get_root_page_element returns None for a format with no layout tree.
        console = RecordingConsole()
        _report(console, page_elements=True).report_page(FakePage(), 1, "sample.docx")

        self.assertEqual([], console.out_lines)

    def test_an_empty_page_prints_nothing_for_any_page_report(self):
        console = RecordingConsole()
        page = FakePage()

        _report(console, page_fonts=True, page_font_mappings=True, page_elements=True,
                form_elements=True, hyperlinks=True).report_page(page, 1, "sample.docx")

        self.assertEqual([], console.out_lines)

    def test_no_report_requested_reads_nothing_from_the_document(self):
        # The reports are opt-in, so a plain conversion must not pay for them -- asking a
        # page for its hyperlinks is work the engine has to do.
        document = FakeDocument("sample.docx")
        document.read_failure = EngineError.ACTION_NOT_SUPPORTED

        console = RecordingConsole()
        _report(console).report_document(document, "sample.docx")

        self.assertEqual([], console.out_lines)

    # ============================================ the catalogues

    def test_formats_lists_what_the_engine_reports(self):
        engine = FakeDocumentFiltersEngine()
        engine.formats.append(_format(254, "docx", "Microsoft Word 2007"))
        engine.formats.append(_format(112, "doc", "Microsoft Word for Windows"))

        console = RecordingConsole()
        _catalog(engine, console).write_formats(FormatsOptions())

        self.assertEqual(["id=254 ext=docx name=Microsoft Word 2007",
                          "id=112 ext=doc name=Microsoft Word for Windows"],
                         console.out_lines)

    def test_the_formats_filter_matches_on_name_and_short_name(self):
        engine = FakeDocumentFiltersEngine()
        engine.formats.append(_format(254, "docx", "Microsoft Word 2007"))
        engine.formats.append(_format(200, "pdf", "Adobe Portable Document Format"))

        console = RecordingConsole()
        options = FormatsOptions()
        options.filter = "word"
        _catalog(engine, console).write_formats(options)

        self.assertEqual("id=254 ext=docx name=Microsoft Word 2007",
                         self._single(console.out_lines))

    def test_a_filter_matching_nothing_prints_nothing_rather_than_failing(self):
        engine = FakeDocumentFiltersEngine()
        engine.formats.append(_format(254, "docx", "Microsoft Word 2007"))

        console = RecordingConsole()
        options = FormatsOptions()
        options.filter = "nothing-matches-this"
        _catalog(engine, console).write_formats(options)

        self.assertEqual([], console.out_lines)

    def test_a_build_that_reports_no_formats_prints_nothing(self):
        console = RecordingConsole()
        _catalog(FakeDocumentFiltersEngine(), console).write_formats(FormatsOptions())

        self.assertEqual([], console.out_lines)

    def test_options_lists_what_the_engine_reports(self):
        engine = FakeDocumentFiltersEngine()
        engine.options.append(_option("BW_MODE", "int", default_value="0",
                                      description="Black and white rendering mode"))

        console = RecordingConsole()
        _catalog(engine, console).write_options(OptionCatalogOptions())

        line = self._single(console.out_lines)
        self.assertIn("name=BW_MODE", line)
        self.assertIn("type=int", line)
        self.assertIn("default=0", line)
        self.assertIn("description=Black and white rendering mode", line)

    def test_an_options_filter_matching_nothing_prints_nothing(self):
        engine = FakeDocumentFiltersEngine()
        engine.options.append(_option("BW_MODE", "int", description="mode"))

        console = RecordingConsole()
        options = OptionCatalogOptions()
        options.filter = "nothing-matches-this"
        _catalog(engine, console).write_options(options)

        self.assertEqual([], console.out_lines)

    def test_a_catalogue_that_cannot_be_read_surfaces_the_engine_error(self):
        """
        The engine's error reaches the caller intact.

        The catalogues are the two commands that read no document, so a failure in either
        is a failure of the run rather than of an input -- there is no input to blame.
        """
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                engine = FakeDocumentFiltersEngine()
                engine.catalog_failure = error

                with self.assertRaises(EngineException) as from_formats:
                    _catalog(engine, RecordingConsole()).write_formats(FormatsOptions())

                with self.assertRaises(EngineException) as from_options:
                    _catalog(engine, RecordingConsole()).write_options(OptionCatalogOptions())

                self.assertEqual(error, from_formats.exception.error)
                self.assertEqual(error, from_options.exception.error)


def _report(console: RecordingConsole, **switches: bool) -> StructureReport:
    """
    A report with the named switches on and every other one off.

    The reference passes an Action<StructureReportOptions>; keyword arguments say the same
    thing without a lambda, since the only thing any caller's action does is turn switches
    on. getattr before setattr so that a misspelled switch fails here rather than quietly
    setting an attribute no report ever reads.
    """
    options = StructureReportOptions()

    for name, value in switches.items():
        getattr(options, name)
        setattr(options, name, value)

    return StructureReport(options, console)


def _catalog(engine: FakeDocumentFiltersEngine, console: RecordingConsole) -> CatalogReports:
    return CatalogReports(DocumentFiltersSession.for_engine(engine), console)


def _bookmark(title: str, level: int = 0, page_index: int = 0,
              children: Iterable[EngineBookmark] = ()) -> EngineBookmark:
    """
    One outline node, as the engine would report it.

    Python has no object initialiser, so the reference's nested `new EngineBookmark { ... }`
    tree would otherwise become a paragraph of assignments per node with the shape of the
    tree lost among them. children is extended rather than assigned, so the list the
    constructor made is the one that ends up populated.
    """
    bookmark = EngineBookmark()
    bookmark.title = title
    bookmark.level = level
    bookmark.page_index = page_index
    bookmark.children.extend(children)
    return bookmark


def _element(element_type: PageElementType, depth: int, bounds: EngineRectangle,
             children: Iterable[EnginePageElement] = ()) -> EnginePageElement:
    """One node of a page's element tree, for the same reason as _bookmark."""
    element = EnginePageElement()
    element.type = element_type
    element.depth = depth
    element.bounds = bounds
    element.children.extend(children)
    return element


def _format(format_id: int, short_name: str, display_name: str) -> EngineFormat:
    """`format` is a builtin and `id` is one too, hence format_id."""
    engine_format = EngineFormat()
    engine_format.id = format_id
    engine_format.short_name = short_name
    engine_format.display_name = display_name
    return engine_format


def _option(display_name: str, option_type: str, default_value: Optional[str] = None,
            description: Optional[str] = None) -> EngineOption:
    """`type` is a builtin, hence option_type; the attribute keeps the binding's name."""
    option = EngineOption()
    option.display_name = display_name
    option.type = option_type
    option.default_value = default_value
    option.description = description
    return option
