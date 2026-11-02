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

import datetime
from enum import IntEnum
from typing import BinaryIO, Callable, List, Optional

from ..planning.format_spec import CanvasType

# -------------------------------------------------------------------------
#  THE ENGINE SEAM
#
#  One interface method per Document Filters call, with no logic of its own.
#  Everything above it — the conversion paths, the reports, the batch runner —
#  talks to these interfaces, so all of it can be tested without the native
#  library, a license, or a real document.
#
#  WHY THE RETURN TYPES ARE OURS AND NOT THE BINDING'S
#
#  A thinner wrapper would hand back Extractor, Canvas, Page and SubFile
#  directly. In the C# that cannot be tested: all four have internal-only
#  constructors, so no fake outside Hyland.DocumentFilters.dll can produce one,
#  and their members are not virtual, so none of them can be subclassed either.
#  The binding is built to be used, not stood in for.
#
#  That particular argument does not survive the port: nothing in the published
#  Python binding is sealed, and a fake could in principle subclass
#  DocumentFilters.Extractor. Three reasons keep the seam anyway, and they are the
#  ones that actually matter here:
#
#    * Importing the binding at all requires the DocumentFilters package to be
#      installed. This module and engine_exception import nothing from it, which is
#      what lets the whole pure suite — planning, discovery, validation, the CLI
#      contract — run from a clone with no native library present.
#    * Every one of those four binding classes is a live native handle wearing a
#      Python object, and each opens its handle lazily on first use. A fake that
#      subclassed one would inherit __del__, _needHandle and Close, and would try to
#      talk to a library that is not there.
#    * The four ports of this tool have to agree byte for byte. A value type
#      declared here is the same in all four; a binding type is whatever each
#      language's binding happens to expose, and those differ — see below.
#
#  So each of those four gets an interface here and a one-line adapter in
#  document_filters_engine, and the small value types they expose — a format row, a
#  bookmark, a hyperlink — get a plain class.
#
#  WHY THE ENUMS ARE OURS TOO
#
#  The C# reuses the binding's enums as they are, on the grounds that an enum is
#  public and constructible so it costs a fake nothing. The Python binding does not
#  publish them: a hyperlink's action, a form element's kind and a page element's
#  kind are bare IGR_* module constants there, and only annotations get an enum
#  (Annotations.Type). Declaring all four here as IntEnum is therefore the only way
#  to name them at the seam, and it keeps this module importable without the
#  binding. planning/format_spec.py already sets that precedent with CanvasType,
#  for the same two reasons. The values are IGR_* numbers, which are ABI and so
#  cannot drift.
#
#  DISPOSAL
#
#  C# marks the four handle-owning interfaces IDisposable. Here each grows close()
#  plus __enter__/__exit__, so `using (x)` ports to `with x:` and `x.Dispose()` to
#  `x.close()`. Never left to the garbage collector: the binding's own __del__ will
#  eventually close a handle, but "eventually" is not a guarantee a batch of ten
#  thousand documents can spend, and IGR_E_TOO_MANY_HANDLES is what running out
#  looks like.
#
#  THE RULE FOR ANYTHING ADDED HERE
#
#  It maps one call, and it decides nothing. A method that chooses, filters or
#  formats belongs above the seam, where it can be tested as a pure function.
#
#  An engine call stays a call. Where the binding exposes something as a method —
#  get_width, get_eof, get_file_type — so does this, even where the binding also
#  offers a property wrapping it. A reader tracing a conversion should be able to
#  read each of these as "this is where the engine is asked", and a property reads
#  as a field that is simply there.
#
#  OVERLOADS
#
#  Python has none, so each C# overload pair becomes two named methods. The suffix
#  follows the binding's own vocabulary: _from_stream where bytes are read
#  (Extractor.FromStream), _on_stream where they are written (Canvas.OnStream).
# -------------------------------------------------------------------------


class OpenType(IntEnum):
    """
    The flags a document is opened with: one content selector, plus at most one
    Format* value.

    Not in the C# file either -- there this is Hyland.DocumentFilters.OpenType, reached
    through the binding's own namespace, which is why ExtractorOpenSettings can name it
    without appearing to cross the seam. The Python binding publishes these only as bare
    IGR_* module constants, and importing that module runs the binding's package __init__,
    which is exactly what the lazy import in document_filters_engine.py exists to avoid.
    Naming them here keeps execution/ importable with no native library present.

    The Format* values are an enumeration packed into bits 16 to 19, not independent
    bits, so at most one may ever be set. Only the members this tool uses are declared: a
    partial copy of a constant table is worse than a minimal one, and the binding carries
    five more Format* values no format in the catalogue asks for.
    """

    BODY_ONLY = 0
    """Document text without metadata."""

    META_ONLY = 1
    """Metadata alone, which is what the meta command opens with."""

    BODY_AND_META = 2
    """Both, and the default every other command wants."""

    FORMAT_TEXT = 0x00010000
    """The plain-text reader, which strips the ISYS control codes."""

    FORMAT_HTML = 0x00020000
    """The HTML reader."""

    FORMAT_IMAGE = 0x00050000
    """The paginated reader. This is what makes pages exist, so every canvas format needs it."""

    FORMAT_MARKDOWN = 0x00080000
    """The Markdown reader, for md-text."""


class HyperlinkActionType(IntEnum):
    """
    What a hyperlink does when followed.

    Not in the C# file: there this is Hyperlink.ActionType from the
    Hyland.DocumentFilters .NET binding. The Python binding reports
    Hyperlink.GetActionType() as a bare int and publishes the alternatives only as
    IGR_HYPERLINK_ACTION_* module constants, so there is no enum to reuse. The
    values are those constants, which are ABI and therefore cannot drift.
    """

    GOTO = 0x1
    """A jump within the same document."""

    URI = 0x2
    """An external address."""


class FormElementType(IntEnum):
    """
    The kind of one interactive form field.

    FormElement.ElementType in the .NET binding; the Python binding reports
    FormElement.GetElementType() as a bare int against the
    IGR_PAGE_FORM_ELEMENT_TYPE_* constants, so it is declared here for the same
    reason as HyperlinkActionType.

    The member names are the constants' own suffixes rather than the .NET enum's
    spelling of them — RADIOBUTTON, not RadioButton — because the constants are the
    one naming all four ports can derive identically. The .NET binding's names differ
    from the ABI's in five of the ten (RadioButton, CheckBox, FileSelect,
    MultiLineText, ComboBox, ListBox), and following them would leave each port
    guessing where the word break goes.
    """

    BUTTON = 0
    RADIOBUTTON = 1
    CHECKBOX = 2
    FILESELECT = 3
    MULTILINE_TEXT = 4
    SINGLELINE_TEXT = 5
    COMBOBOX = 6
    LISTBOX = 7
    SIGNATURE = 8
    PASSWORD = 9


class PageElementType(IntEnum):
    """
    One node's kind in a page's element tree, as the structure report names them.

    PageElementType in the .NET binding; the Python binding reports
    PageElement.Type as a bare int against the IGR_PAGE_ELEMENT_TYPE_* constants,
    so it is declared here for the same reason as HyperlinkActionType.

    NOTES is in the .NET enum and in the C header but not in the Python binding,
    which stops its IGR_PAGE_ELEMENT_TYPE_* constants at TEXT_BOX. Declared here
    anyway, from the header's value, so that a document containing one does not come
    back unclassifiable in this port alone.
    """

    PAGE = 1
    SECTION = 2
    COLUMN = 3
    PARAGRAPH = 4
    LINE = 5
    RUN = 6
    WORD = 7
    HEADER = 8
    FOOTER = 9
    IMAGE = 10
    TABLE = 11
    TABLE_ROW = 12
    TABLE_CELL = 13
    FORM = 14
    FORM_ELEMENT = 15
    FLOAT = 16
    GRAPHIC = 17
    TEXT_BOX = 18
    NOTES = 19


class AnnotationKind(IntEnum):
    """
    The kind of one annotation.

    'Type' is the binding's own name for this in both languages, which in C#
    collides with System.Type wherever it is not qualified and is aliased to
    AnnotationKind there; the same name is kept here so the two files read alike.

    This is the one of the four the Python binding does publish as an enum
    (Annotations.Type), but it is still declared here rather than imported, because
    importing it would pull the binding into this module and take the pure suite
    down with it. The member names are the binding's own, in SCREAMING_SNAKE; the
    values are the IGR_ANNOTATION_* constants, so the two agree by number as well
    as by name. Note the three the binding renames: RECTANGLE is IGR_ANNOTATION_
    SQUARE, ELLIPSE is IGR_ANNOTATION_CIRCLE, and SQUIGGLY is
    IGR_ANNOTATION_SQUIGGLE.

    The full table is declared, all twenty-nine members, and not the nineteen the
    Python binding's own Annotations.Type happens to carry: that class stops at POPUP
    and resumes at BARCODE, leaving out 17 to 25. The C# aliases the .NET binding's
    enum, which is complete, so a widget or a watermark annotation reports its real
    kind there. Copying the Python binding's shorter list would have made those nine
    report kind=unknown in this port alone -- a divergence in report output caused by
    a gap in a binding rather than by anything the tool decided. The values are the
    IGR_ANNOTATION_* constants either way.
    """

    UNKNOWN = 0
    TEXT = 1
    LINK = 2
    FREETEXT = 3
    LINE = 4
    RECTANGLE = 5
    ELLIPSE = 6
    POLYGON = 7
    POLYLINE = 8
    HIGHLIGHT = 9
    UNDERLINE = 10
    SQUIGGLY = 11
    STRIKEOUT = 12
    STAMP = 13
    CARET = 14
    INK = 15
    POPUP = 16
    FILE_ATTACHMENT = 17
    SOUND = 18
    MOVIE = 19
    WIDGET = 20
    SCREEN = 21
    PRINTERMARK = 22
    TRAPNET = 23
    WATERMARK = 24
    THREE_D = 25
    BARCODE = 26
    NAMED_DESTINATION = 27


class IDocumentFiltersEngine:
    """
    The engine itself: what DocumentFilters offers before a document exists.

    A plain class whose methods raise NotImplementedError rather than an
    abc.ABC of @abstractmethod, unlike runtime/i_console.py and
    runtime/i_file_system.py. Those two are small enough that a fake implements all
    of them; this seam is thirty-odd methods across five interfaces, and almost
    every fake wants a handful. ABC would force each one to stub the rest, and a
    stub written only to satisfy the metaclass is a stub nobody checks.
    """

    def initialize(self, license_key: str, library_path: str) -> None:
        """
        DocumentFilters.Initialize.

        library_path names the directory holding the engine binaries. The .NET
        binding takes one path and uses it for both jobs, because the managed
        assembly sits beside the native library it loads; the Python binding splits
        them — Initialize(license, resource_path, dll_path) — so this one value is
        handed to both. See document_filters_engine for the detail.

        Raises:
            EngineException: The library was not found, or the license was rejected.
        """
        raise NotImplementedError

    def open_document(self, path: str, flags: int, options: str,
                      password: Optional[Callable[[], Optional[str]]] = None,
                      should_cancel: Optional[Callable[[], bool]] = None,
                      log_level_for: Optional[Callable[[str], int]] = None,
                      on_log_message: Optional[Callable[[int, str, str], None]] = None) -> 'IDocument':
        """
        get_extractor followed by Extractor.open, which the engine applies as one
        step.

        The two are not separable here on purpose: the open flags take effect
        *during* open, so handing back an unopened extractor would invite exactly
        the mistake D18 exists to prevent. The Python binding makes that trap
        sharper still — its Extractor opens itself on first use of any member, at
        IGR_BODY_AND_META, so an extractor handed out unopened does not stay
        unopened, it silently opens itself with the wrong flags.

        flags is the complete IGR_ flag set, format bit included, and reaches the
        engine unaltered. Deliberately not an open mode plus flags: the .NET
        binding's Open(OpenMode, OpenType, ...) overload masks out bits 16 to 31 and
        substitutes the format bit its mode implies, and every IGR_FORMAT_* value
        lives in that range, so a format passed that way is discarded rather than
        honoured. Whoever decides the format has to be the one that names it — see
        execution's ExtractorOpenSettings. The Python binding has only the
        flags-only form, so it cannot go wrong that way, but the contract is stated
        the same in all four ports.

        password is asked for the document's password when the engine requests one,
        and returns None to give up — which surfaces as an EngineException carrying
        EngineError.PASSWORD.

        should_cancel is polled while the engine works, and returning True stops the
        open with EngineError.CANCELLED. It covers *this call only*: the engine
        raises it as IGR_OPEN_CALLBACK_ACTION_HEARTBEAT during open, and there is no
        equivalent anywhere else — IGR_Render_Page and IGR_Make_Output_Canvas take
        no callback, so a page loop cannot be interrupted once the document is open.

        Raises:
            EngineException: The document could not be opened.
        """
        raise NotImplementedError

    def open_document_from_stream(self, source: BinaryIO, display_name: str, flags: int, options: str,
                                  password: Optional[Callable[[], Optional[str]]] = None,
                                  should_cancel: Optional[Callable[[], bool]] = None,
                                  log_level_for: Optional[Callable[[str], int]] = None,
                                  on_log_message: Optional[Callable[[int, str, str], None]] = None) -> 'IDocument':
        """
        The same, reading the document from source rather than from a path.

        For the debug switches that have to control how bytes reach the engine:
        --debug-use-memory-stream, --debug-truncate and --debug-measure-io. Each of
        those is the stream itself doing something different, so none of them can be
        expressed as an option on the path form. display_name is what failures are
        reported against, since a stream has no name of its own.

        Raises:
            EngineException: The document could not be opened.
        """
        raise NotImplementedError

    def set_temp_path(self, path: str) -> None:
        """
        Points the engine at a directory for its scratch files — --temp-dir.

        IGR_Multiplex(IGR_Multi_Set_Temp_Path, ...), which is how doc2text does it.
        A library-wide setting rather than a per-document one, so it is applied once,
        immediately after initialize.
        """
        raise NotImplementedError

    def get_memory_status(self) -> str:
        """
        The engine's own memory report — -x and --xx.

        IGR_Get_Memory_Status. Free-form text the engine composes, printed verbatim:
        its content is the engine's to change and is not contract.
        """
        raise NotImplementedError

    def identify_file(self, path: str) -> 'EngineFileType':
        """
        get_extractor and get_file_type, *without* opening: what the file is,
        without reading it.

        Its own call rather than a mode of open_document because it is a different
        operation on the engine — identify never opens a document, so it never needs
        a password, never applies open flags, and reports a file it could not have
        read. Returning plain data rather than an unopened document also stops a
        caller asking one for text it cannot give, which in Python would not even
        fail cleanly: an unopened Extractor opens itself on first use.

        Raises:
            EngineException: The file could not be reached or recognised.
        """
        raise NotImplementedError

    def identify_file_from_stream(self, source: BinaryIO, display_name: str) -> 'EngineFileType':
        """
        The same, from a stream — so the debug switches that change how bytes arrive
        reach identify too.

        --debug-truncate is the one that earns this: what the engine makes of a
        prefix is exactly the question that switch exists to ask, and without a
        stream form it would be silently inert for this command.

        Raises:
            EngineException: The file could not be reached or recognised.
        """
        raise NotImplementedError

    def make_canvas(self, target: str, canvas_type: CanvasType, options: str) -> 'IOutputCanvas':
        """make_output_canvas onto a file."""
        raise NotImplementedError

    def make_canvas_on_stream(self, target: BinaryIO, canvas_type: CanvasType, options: str) -> 'IOutputCanvas':
        """make_output_canvas onto a stream, for --stdout and --fingerprint."""
        raise NotImplementedError

    def get_supported_formats(self) -> List['EngineFormat']:
        """get_supported_formats, for the formats command."""
        raise NotImplementedError

    def get_available_options(self) -> List['EngineOption']:
        """get_available_options, for the options command."""
        raise NotImplementedError

    def describe_version(self) -> str:
        """The binding's version, for the version command."""
        raise NotImplementedError

    def close(self) -> None:
        """Releases the engine instance. C#'s Dispose."""
        raise NotImplementedError

    def __enter__(self) -> 'IDocumentFiltersEngine':
        return self

    def __exit__(self, exception_type, exception_value, traceback) -> None:
        self.close()


class IDocument:
    """One open document: Extractor."""

    def get_file_type(self) -> int:
        """get_file_type(): the numeric format id."""
        raise NotImplementedError

    def get_mime_type(self) -> str:
        """get_file_type(IGR_FORMAT_MIME_TYPE)."""
        raise NotImplementedError

    def get_format_name(self) -> str:
        """get_file_type(IGR_FORMAT_LONG_NAME)."""
        raise NotImplementedError

    def get_file_capabilities(self) -> int:
        """get_file_capabilities(): the text/subfiles/hd flag word."""
        raise NotImplementedError

    def get_eof(self) -> bool:
        """get_eof()."""
        raise NotImplementedError

    def get_text(self, max_length: int) -> str:
        """get_text(max_length)."""
        raise NotImplementedError

    def get_page_count(self) -> int:
        """get_page_count()."""
        raise NotImplementedError

    def get_page(self, page_index: int) -> 'IDocumentPage':
        """get_page(index), zero-based as the binding has it."""
        raise NotImplementedError

    @property
    def sub_files(self) -> List['ISubFileEntry']:
        """
        get_first_sub_file/get_next_sub_file, drained into a list.

        Drained rather than streamed because the binding's cursor is positional
        state on the extractor: interleaving a walk with anything else that touches
        the same handle silently loses entries. A list makes that impossible.
        """
        raise NotImplementedError

    @property
    def images(self) -> List['ISubFileEntry']:
        """get_first_image/get_next_image, drained into a list."""
        raise NotImplementedError

    @property
    def root_bookmark(self) -> Optional['EngineBookmark']:
        """root_bookmark, or None when the document has no outline."""
        raise NotImplementedError

    def make_doc_pack(self, filename: str, options: str) -> None:
        """make_doc_pack onto a file."""
        raise NotImplementedError

    def make_doc_pack_on_stream(self, destination: BinaryIO, options: str) -> None:
        """make_doc_pack onto a stream."""
        raise NotImplementedError

    def close(self) -> None:
        """Releases the document handle. C#'s Dispose."""
        raise NotImplementedError

    def __enter__(self) -> 'IDocument':
        return self

    def __exit__(self, exception_type, exception_value, traceback) -> None:
        self.close()


class IDocumentPage:
    """One page of an open document: Page."""

    def get_width(self) -> int:
        """get_width()."""
        raise NotImplementedError

    def get_height(self) -> int:
        """get_height()."""
        raise NotImplementedError

    def get_attribute(self, name: str) -> Optional[str]:
        """get_attribute(name); empty or None when the page has no such attribute."""
        raise NotImplementedError

    @property
    def hyperlinks(self) -> List['EngineHyperlink']:
        """Every hyperlink on the page, drained into a list."""
        raise NotImplementedError

    @property
    def form_elements(self) -> List['EngineFormElement']:
        """Every interactive form field on the page, drained into a list."""
        raise NotImplementedError

    @property
    def annotations(self) -> List['EngineAnnotation']:
        """Every annotation on the page, drained into a list."""
        raise NotImplementedError

    def get_root_page_element(self) -> Optional['EnginePageElement']:
        """get_root_page_element(), or None when the page exposes no element tree."""
        raise NotImplementedError

    def close(self) -> None:
        """Releases the page handle. C#'s Dispose."""
        raise NotImplementedError

    def __enter__(self) -> 'IDocumentPage':
        return self

    def __exit__(self, exception_type, exception_value, traceback) -> None:
        self.close()


class IOutputCanvas:
    """An output canvas: Canvas."""

    def render_page(self, page: IDocumentPage, options: str,
                    decorations: Optional['EnginePageDecorations']) -> None:
        """render_page. decorations is None when the page needs none."""
        raise NotImplementedError

    def render_pages(self, document: IDocument) -> None:
        """render_pages(extractor): the whole document in one call."""
        raise NotImplementedError

    def draw_image(self, x: int, y: int, image: bytes, mime_type: str) -> None:
        """
        draw_image, for --watermark.

        The last argument is the image's MIME type, not an options string. The C#
        seam calls it `options`, which is a misnomer there too: both bindings name
        the parameter `mimetype` and pass it straight to IGR_Canvas_DrawImage, which
        uses it to pick a decoder. Renamed here so the call site cannot read it as
        somewhere to put canvas options.
        """
        raise NotImplementedError

    def close(self) -> None:
        """Closes the canvas, which is what flushes and finishes the output. C#'s Dispose."""
        raise NotImplementedError

    def __enter__(self) -> 'IOutputCanvas':
        return self

    def __exit__(self, exception_type, exception_value, traceback) -> None:
        self.close()


class ISubFileEntry:
    """
    One entry inside a container: SubFile.

    Closable because the binding's SubFile is a handle, not a row of data. The entry
    owns that handle for its whole life, including the IDocument that open returns —
    in the binding those are the same object, so the document must not be closed
    separately.
    """

    @property
    def id(self) -> str:
        """The container's own identifier for the entry, which open takes."""
        raise NotImplementedError

    @property
    def name(self) -> str:
        """The name as the container gives it — untrusted, and never used as a path unchecked."""
        raise NotImplementedError

    @property
    def size(self) -> int:
        """The entry's length in bytes as the container records it."""
        raise NotImplementedError

    @property
    def modified(self) -> Optional[datetime.datetime]:
        """The entry's timestamp, or None when the container records none."""
        raise NotImplementedError

    def open(self, flags: int, options: str) -> IDocument:
        """
        SubFile.open: the entry as a document in its own right.

        Raises:
            EngineException: The entry could not be opened.
        """
        raise NotImplementedError

    def copy_to(self, path: str) -> None:
        """copy_to(filename): the entry's bytes, untouched."""
        raise NotImplementedError

    def close(self) -> None:
        """Releases the entry's handle. C#'s Dispose."""
        raise NotImplementedError

    def __enter__(self) -> 'ISubFileEntry':
        return self

    def __exit__(self, exception_type, exception_value, traceback) -> None:
        self.close()


# ------------------------------------------------------------------ values


class EngineFileType:
    """What a file is, without opening it."""

    def __init__(self):
        self.id = 0
        """The numeric format id; 0 when the engine cannot place the file."""

        self.mime_type: Optional[str] = None
        """The format's MIME type, e.g. application/pdf."""

        self.name: Optional[str] = None
        """The format's long name, e.g. Microsoft Word 2007."""

        self.capabilities = 0
        """The text/subfiles/hd flag word."""


class EngineFormat:
    """One row of get_supported_formats."""

    def __init__(self):
        self.id = 0
        """The numeric format id, which is the row's index."""

        self.short_name: Optional[str] = None
        """The engine's terse name for the format."""

        self.display_name: Optional[str] = None
        """The engine's long name for the format."""


class EngineOption:
    """One row of get_available_options."""

    def __init__(self):
        self.display_name: Optional[str] = None
        """The option's name, as it is written in an --options string."""

        self.type: Optional[str] = None
        """The value's type, as the engine names it: int, string, bool."""

        self.description: Optional[str] = None
        """What the option does, in the engine's own words."""

        self.flags = 0
        """The engine's flag word for the option."""

        self.default_value: Optional[str] = None
        """What the engine does when the option is not named."""


class EngineBookmark:
    """One node of the document outline."""

    def __init__(self):
        self.title: Optional[str] = None
        """The text shown for the node."""

        self.level = 0
        """Depth in the outline, as the binding reports it."""

        self.page_index = 0
        """Zero-based, as the binding reports it."""

        self.children: List['EngineBookmark'] = []
        """The nodes beneath this one, in outline order."""


class EngineHyperlink:
    """One hyperlink on a page."""

    def __init__(self):
        self.type = HyperlinkActionType.GOTO
        """What following the link does."""

        self.uri: Optional[str] = None
        """The external address, for URI."""

        self.ref: Optional[str] = None
        """The in-document destination, for GOTO."""

        self.x = 0
        self.y = 0
        self.width = 0
        self.height = 0


class EngineFormElement:
    """One form element on a page."""

    def __init__(self):
        self.type = FormElementType.BUTTON
        """The kind of field."""

        self.name: Optional[str] = None
        """The field's name, which --form assigns by."""

        self.x = 0
        self.y = 0
        self.width = 0
        self.height = 0


class EngineAnnotation:
    """One annotation on a page."""

    def __init__(self):
        self.type = AnnotationKind.UNKNOWN
        """The kind of annotation."""

        self.title: Optional[str] = None
        """Who made it, as the document records them."""

        self.text: Optional[str] = None
        """Its contents."""


class EngineRectangle:
    """
    A rectangle in page coordinates, as floats.

    Not in the C# file: there this is System.Drawing.RectangleF, which the .NET
    binding returns from PageElement.Bounds. Python has no equivalent in the
    standard library, and the Python binding returns its own ctypes IGR_FRect —
    which cannot cross the seam, both because it would drag the binding into this
    module and because it is a view onto a struct the page handle owns.

    Stated as x/y/width/height to match RectangleF rather than as the
    left/top/right/bottom IGR_FRect carries, so all four ports report the same four
    numbers; the adapter converts.
    """

    def __init__(self, x: float = 0.0, y: float = 0.0, width: float = 0.0, height: float = 0.0):
        self.x = x
        self.y = y
        self.width = width
        self.height = height


class EnginePageElement:
    """One node of a page's element tree."""

    def __init__(self):
        self.type = PageElementType.PAGE
        """The kind of node."""

        self.depth = 0
        """Depth in the tree, as the binding reports it."""

        self.bounds = EngineRectangle()
        """The binding reports element geometry as a float rectangle, not four ints."""

        self.children: List['EnginePageElement'] = []
        """The nodes beneath this one, in document order."""

    def all_children(self):
        """Every node beneath this one, depth first."""
        for child in self.children:
            yield child

            for descendant in child.all_children():
                yield descendant


class EnginePageDecorations:
    """
    What --redact and --form add to one page before it renders.

    Carried as data rather than as the binding's RenderPageProperties for the same
    reason as everything else here: the adapter builds that object, and a fake can
    assert on these values without one.
    """

    def __init__(self):
        self.redactions: List['EngineRedaction'] = []
        """The regions to black out, in page coordinates."""

        self.form_values: List['EngineFormValue'] = []
        """The field assignments to apply before rendering."""

    @property
    def is_empty(self) -> bool:
        """True when this page needs no decoration at all, which is the common case."""
        return len(self.redactions) == 0 and len(self.form_values) == 0


class EngineRedaction:
    """One --redact region, in page coordinates."""

    def __init__(self, x: int = 0, y: int = 0, width: int = 0, height: int = 0):
        self.x = x
        self.y = y
        self.width = width
        self.height = height


class EngineFormValue:
    """One --form assignment."""

    def __init__(self, name: Optional[str] = None, value: Optional[str] = None):
        self.name = name
        self.value = value
