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

from typing import Iterable, List, Optional

from ..configuration.value_parsers import fold_token
from ..engine.i_document_filters_engine import (EngineBookmark, FormElementType,
                                                HyperlinkActionType, IDocument, IDocumentPage)


class StructureReport:
    """
    The document and page structure aspects convert reports while it works: fonts,
    dimensions, page elements, form elements, hyperlinks, bookmarks and annotations.

    These are emitted during a conversion run rather than by a separate command, matters
    most for high-definition output, where the structure is only known once the page has
    been laid out.

    Output is minimal and line-oriented, one fact per line, and is compared as golden
    files by the conformance suite. Every line goes out through IConsole.write_line, so
    nothing here writes document bytes and nothing here has to worry about newline
    translation: that concern belongs to the conversion paths that write a document, not
    to a report.
    """

    def __init__(self, options, console):
        self._options = options
        self._console = console

    def report_document(self, document: IDocument, display_name: str) -> None:
        """Reports the document-level aspects: fonts, font mappings, bookmarks, annotations."""
        if self._options.fonts:
            self.write_fonts(document, display_name)

        if self._options.font_mappings:
            self.write_font_mappings(document, display_name)

        if self._options.bookmarks:
            self.write_bookmarks(document, display_name)

        if self._options.annotations:
            self.write_annotations(document, display_name)

    def report_page(self, page: IDocumentPage, page_number: int, display_name: str) -> None:
        """Reports the page-level aspects for one page."""
        if self._options.dimensions:
            self.write_dimensions(page, page_number, display_name)

        if self._options.page_fonts:
            self.write_page_fonts(page, page_number, display_name)

        if self._options.page_font_mappings:
            self.write_page_font_mappings(page, page_number, display_name)

        if self._options.page_elements:
            self.write_page_elements(page, page_number, display_name)

        if self._options.form_elements:
            self.write_form_elements(page, page_number, display_name)

        if self._options.hyperlinks:
            self.write_hyperlinks(page, page_number, display_name)

    def write_fonts(self, document: IDocument, display_name: str) -> None:
        """
        Fonts are not a first-class collection anywhere in the binding; doc2text reads
        them the same way, as the semicolon-separated DOCUMENT_FONTS page attribute (see
        LayoutAPI.cpp's IGR_Get_Page_Attribute handling), queried on the last page once
        every page has been laid out.

        Public rather than private, as are the other nine write_* methods: they are
        `internal` in the C#, which exists so the test assembly can call them, and
        Python's equivalent of that visibility is a name without a leading underscore.
        """
        self._write_font_list(_get_last_page_attribute(document, "DOCUMENT_FONTS"),
                              display_name, page_number=None)

    def write_font_mappings(self, document: IDocument, display_name: str) -> None:
        """Same source as write_fonts, via DOCUMENT_FONT_MAPPINGS."""
        self._write_font_mapping_list(_get_last_page_attribute(document, "DOCUMENT_FONT_MAPPINGS"),
                                      display_name, page_number=None)

    def write_bookmarks(self, document: IDocument, display_name: str) -> None:
        """
        Guarded on IDocument.get_page_count for the same reason _get_last_page_attribute
        is: a bookmark's page number only exists once the document has been laid out into
        pages, and a handle opened without IGR_FORMAT_IMAGE — --to text --bookmarks, where
        the text reader needs the format bit bookmarks would otherwise take — was never
        laid out at all. Unlike _get_last_page_attribute, root_bookmark does not degrade to
        "nothing" on its own for that case; it throws the binding's own Invalid DocHandle.
        This is that same "reports nothing rather than failing" rule, applied where the
        binding does not already apply it.
        """
        if document.get_page_count() == 0:
            return

        root = document.root_bookmark
        if root is None:
            return

        index = 0
        for child in _each(root.children):
            index = self._write_bookmarks_recursive(child, display_name, index)

    def _write_bookmarks_recursive(self, bookmark: EngineBookmark, display_name: str, index: int) -> int:
        """
        Writes one bookmark and everything beneath it, and hands back the running count.

        The C# passes the counter as `ref int index`, which Python has no equivalent for —
        an int argument is rebound inside the callee and the caller never sees it. Returning
        the new value is the port of that, rather than wrapping the counter in a
        single-element list or hanging it off self: the first is a ref parameter in
        disguise and reads worse, and the second would turn a local into instance state
        that outlives the walk, so a second --bookmarks report on the same instance would
        resume numbering where the first left off instead of starting at 1.
        """
        index += 1
        self._write_record(display_name, "bookmark={0} level={1} page={2} title={3}".format(
            index, bookmark.level, bookmark.page_index + 1, _text(bookmark.title)))

        for child in _each(bookmark.children):
            index = self._write_bookmarks_recursive(child, display_name, index)

        return index

    def write_annotations(self, document: IDocument, display_name: str) -> None:
        """
        A document-level report even though every annotation belongs to a page: numbering
        runs across the whole document, so it has to be gathered in one pass rather than
        reset per report_page call.
        """
        index = 0
        page_count = document.get_page_count()
        for page_index in range(page_count):
            # C#'s `using IDocumentPage page = ...`, whose scope is the rest of the
            # enclosing block; `with` closes the page at the end of one iteration, which is
            # the same handle lifetime here because the declaration is the first statement
            # of the loop body.
            with document.get_page(page_index) as page:
                for annotation in page.annotations:
                    index += 1
                    self._write_record(
                        display_name,
                        "annotation={0} page={1} kind={2} author={3} text={4}".format(
                            index, page_index + 1, _kind_of(annotation.type),
                            _text(annotation.title), _text(annotation.text)))

    def write_dimensions(self, page: IDocumentPage, page_number: int, display_name: str) -> None:
        self._write_record(display_name, "page={0} width={1} height={2}".format(
            page_number, page.get_width(), page.get_height()))

    def write_page_fonts(self, page: IDocumentPage, page_number: int, display_name: str) -> None:
        self._write_font_list(page.get_attribute("FONTS"), display_name, page_number)

    def write_page_font_mappings(self, page: IDocumentPage, page_number: int, display_name: str) -> None:
        """--page-font-mappings: how each page's fonts were substituted."""
        self._write_font_mapping_list(page.get_attribute("FONT_MAPPINGS"), display_name, page_number)

    def write_page_elements(self, page: IDocumentPage, page_number: int, display_name: str) -> None:
        root = page.get_root_page_element()
        if root is None:
            return

        index = 0
        for element in root.all_children():
            index += 1
            bounds = element.bounds
            self._write_record(
                display_name,
                "page={0} element={1} kind={2} depth={3} x={4} y={5} w={6} h={7}".format(
                    page_number, index, _kind_of(element.type), element.depth,
                    _truncate(bounds.x), _truncate(bounds.y),
                    _truncate(bounds.width), _truncate(bounds.height)))

    def write_form_elements(self, page: IDocumentPage, page_number: int, display_name: str) -> None:
        index = 0
        for element in page.form_elements:
            index += 1
            self._write_record(
                display_name,
                "page={0} form-element={1} kind={2} x={3} y={4} w={5} h={6} name={7}".format(
                    page_number, index, _form_element_kind(element.type),
                    element.x, element.y, element.width, element.height, _text(element.name)))

    def write_hyperlinks(self, page: IDocumentPage, page_number: int, display_name: str) -> None:
        index = 0
        for link in page.hyperlinks:
            index += 1
            target = ("uri=" + _text(link.uri)) if link.type == HyperlinkActionType.URI \
                else ("ref=" + _text(link.ref))
            self._write_record(
                display_name,
                "page={0} hyperlink={1} kind={2} x={3} y={4} w={5} h={6} {7}".format(
                    page_number, index, _kind_of(link.type),
                    link.x, link.y, link.width, link.height, target))

    def _write_record(self, display_name: str, fields: str) -> None:
        self._console.write_line(display_name + ": " + fields)

    def _write_font_list(self, attribute_value: Optional[str], display_name: str,
                         page_number: Optional[int]) -> None:
        for font in _split_attribute(attribute_value):
            fields = ("page={0} ".format(page_number) if page_number is not None else "") \
                + "font={0}".format(font)
            self._write_record(display_name, fields)

    def _write_font_mapping_list(self, attribute_value: Optional[str], display_name: str,
                                 page_number: Optional[int]) -> None:
        """Each entry is original=mapped, per LayoutAPI.cpp's own construction of the attribute text."""
        for entry in _split_attribute(attribute_value):
            split = entry.find('=')
            original = entry if split < 0 else entry[:split]
            mapped = entry if split < 0 else entry[split + 1:]

            fields = ("page={0} ".format(page_number) if page_number is not None else "") \
                + "font={0} mapped={1}".format(original, mapped)
            self._write_record(display_name, fields)


# The four helpers below are the C#'s `private static` members. Module-level private
# functions rather than @staticmethod, because a static method is what C# has instead of a
# free function, and debug_input.py already settles that direction for this package.


def _each(children: Optional[List]) -> Iterable:
    """
    The C#'s `children ?? Enumerable.Empty<...>()`, which guards a null collection.

    The seam declares EngineBookmark.children as a list and initialises it to empty, so
    None should never arrive; the guard is carried across anyway because the C# carries it,
    and because a fake is free to assign None to a plain attribute where C# would at least
    have had to say so.
    """
    return children if children is not None else []


def _text(value: Optional[str]) -> str:
    """
    A field value as string interpolation would render it in C#.

    Not in the C#, and it exists for one difference: C#'s $"title={null}" writes an empty
    string, where "title={0}".format(None) writes the four characters None. Every field
    this report prints that the engine may have no value for — a bookmark title, an
    annotation's author and text, a form element's name, a hyperlink's uri or ref, all
    Optional[str] at the seam — goes through here, so a document that omits one produces
    the same line in all four ports.
    """
    return value if value is not None else ""


def _truncate(value: float) -> int:
    """
    A page-element bound as the C#'s (int) cast renders it.

    int() truncates toward zero and so does a C# float-to-int conversion, including for
    negatives, so -0.9 is 0 in both. The one case the two do not agree on is a bound that
    is NaN or outside the int range: C# yields an unspecified value (int.MinValue in
    practice) and Python raises ValueError or OverflowError. Left as it is rather than
    clamped, because a guard here would be behaviour this port invented and the reference
    does not have; the engine reports laid-out page geometry, which is finite by
    construction.
    """
    return int(value)


def _split_attribute(value: Optional[str]) -> List[str]:
    """
    The semicolon-separated form the engine packs a font list into, as entries.

    string.IsNullOrEmpty, not IsNullOrWhiteSpace, so the test is exactly "None or length
    zero" and a value of all spaces still splits — which matters, since an entry is a font
    name and a font name may be spaces as far as this is concerned.

    A list rather than a generator, where the C# returns IEnumerable<string>: the only two
    callers iterate it once and immediately, and a generator would defer the work to no
    benefit while making the empty case harder to read.
    """
    if value is None or len(value) == 0:
        return []

    return [entry for entry in value.split(';') if len(entry) > 0]


def _get_last_page_attribute(document: IDocument, attribute_name: str) -> str:
    """The named attribute of the last page, or empty when the document has no pages."""
    page_count = document.get_page_count()
    if page_count == 0:
        return ""

    with document.get_page(page_count - 1) as page:
        value = page.get_attribute(attribute_name)
        return value if value is not None else ""


def _kind_of(value) -> str:
    """
    One of the engine seam's enums as this report names it.

    The C# is `value.ToString().ToLowerInvariant()` over any enum, and there that reads the
    .NET binding's own member names: PageElementType.TableRow becomes tablerow,
    Annotations.Type.NamedDestination becomes nameddestination, Hyperlink.ActionType.GoTo
    becomes goto. Those strings are golden-file contract, so this has to produce the same
    ones, and neither obvious Python spelling does:

      * value.value is the wrong answer here. It is the right one in
        planning/format_spec.py, whose FormatKind carries the canonical spelling as its
        value; the four enums this function is called with — PageElementType,
        AnnotationKind, HyperlinkActionType, FormElementType — are IntEnum over the ABI's
        IGR_* numbers, so .value is an integer and would render kind=12.
      * value.name.lower() is wrong twice over. The Python members are SCREAMING_SNAKE
        where .NET's are PascalCase, so every member whose name is more than one word
        would gain an underscore the C# does not write: TABLE_ROW, TABLE_CELL,
        FORM_ELEMENT and TEXT_BOX in PageElementType, NAMED_DESTINATION in AnnotationKind.
        And .lower() is the case-folding this codebase does not use — see fold_token.

    So the name has its word breaks removed and is then folded with fold_token. That is a
    derivation rather than a table of forty-odd labels on purpose: it is exact for every
    member of all four enums today (the underscore marks precisely where .NET has a capital
    and nowhere else, FormElementType's ABI-derived MULTILINE_TEXT and SINGLELINE_TEXT
    included), and it stays exact for a member added later, where a hand-written table
    would silently start reporting a number.

    A value the enums do not name can still arrive as a bare int, since the engine adapter
    maps an unrecognised kind to a fallback member rather than failing; that case renders
    the number, which is also what C#'s ToString() does for an undefined enum value.
    """
    name = getattr(value, 'name', None)
    if name is not None:
        return fold_token(name.replace('_', ''))

    # bool before int: bool subclasses int, so an isinstance(value, int) test on its own
    # would accept True and quietly report kind=1.
    if isinstance(value, bool) or not isinstance(value, int):
        return fold_token(str(value))

    return str(int(value))


def _form_element_kind(element_type) -> str:
    """
    Short, CLI-facing labels rather than the binding's own enum names — only SingleLineText
    -> text is confirmed against a real fixture; the rest follow the same short-label
    convention.

    The C# switch's ten cases cover every member of FormElement.ElementType, and the ten
    members of the seam's FormElementType are the same ten, so the fall-through is
    unreachable for any value the enums name in either port. It is kept because the C#
    keeps it, and because a bare int can reach here the same way it can reach _kind_of.

    Written as an if/elif chain rather than a dict: Python 3.8 has no match, and a dict
    literal would be the same ten pairs with the fall-through moved somewhere less obvious.
    The member names are the seam's — RADIOBUTTON, MULTILINE_TEXT — which are the ABI
    constants' own suffixes and differ from the .NET binding's spelling the C# uses; see
    FormElementType's docstring for why the seam chose those.
    """
    if element_type == FormElementType.SINGLELINE_TEXT:
        return "text"
    if element_type == FormElementType.MULTILINE_TEXT:
        return "multiline"
    if element_type == FormElementType.CHECKBOX:
        return "checkbox"
    if element_type == FormElementType.RADIOBUTTON:
        return "radio"
    if element_type == FormElementType.COMBOBOX:
        return "combo"
    if element_type == FormElementType.LISTBOX:
        return "list"
    if element_type == FormElementType.FILESELECT:
        return "file"
    if element_type == FormElementType.SIGNATURE:
        return "signature"
    if element_type == FormElementType.PASSWORD:
        return "password"
    if element_type == FormElementType.BUTTON:
        return "button"

    return _kind_of(element_type)
