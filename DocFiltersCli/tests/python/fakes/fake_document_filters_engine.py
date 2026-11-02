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
import io
from typing import BinaryIO, Callable, Dict, List, Optional

from docfilters_cli.engine.engine_exception import EngineError, EngineException
from docfilters_cli.engine.i_document_filters_engine import (EngineAnnotation, EngineBookmark,
                                                             EngineFileType, EngineFormat,
                                                             EngineFormElement, EngineHyperlink,
                                                             EngineOption, EnginePageDecorations,
                                                             EnginePageElement, IDocument,
                                                             IDocumentFiltersEngine, IDocumentPage,
                                                             IOutputCanvas, ISubFileEntry)
from docfilters_cli.planning.format_spec import CanvasType

# ---------------------------------------------------------------------------
#  A NOTE ON THE CALL LOG
#
#  The strings appended to `calls` keep the reference's PascalCase spellings --
#  "OpenDocument(a.pdf,2,)", not "open_document(a.pdf,2,)" -- even though the
#  methods producing them are snake_case here. They are not API names: they are
#  assertion text, and an engine test ported from C# asserts on the same literal in
#  both languages. Keeping them identical is what makes the two call logs diffable,
#  which is the whole point of having one. The house PascalCase-to-snake_case rule is
#  about identifiers, and none of these is one.
#
#  A NOTE ON THE NULLABLE FAILURE SWITCHES
#
#  Every `EngineError?` below is an Optional[EngineError] tested with `is not None`,
#  never for truth. EngineError.NONE is 0 and therefore falsy, so `if self.
#  initialisation_failure:` would silently ignore a test that scripted it -- and NONE
#  is exactly the value a test reaching for "the engine reported no error, loudly"
#  would pick. `is not None` is also what C#'s HasValue means.
# ---------------------------------------------------------------------------


class FakeDocumentFiltersEngine(IDocumentFiltersEngine):
    """
    A Document Filters engine that answers from a script instead of a library.

    The engine is the one dependency the execution paths cannot be tested around: it
    needs a license, a native library, and a real document that actually exhibits
    whatever is being tested. Every failure this thing can produce is a failure the
    real engine produces too -- the codes come from EngineError, which mirrors
    ISYS11dfConstants -- so a test here is a statement about how the CLI handles a
    real outcome.

    Documents are added by path. Anything not added fails the way the engine fails for
    a file it cannot read, so "the document is not there" needs no setup.
    """

    def __init__(self):
        # Ordinal string comparison is what the C# dictionaries are built with, and it
        # is what a Python dict does with str keys already -- there is no
        # culture-sensitive alternative to opt out of here.
        self._documents: Dict[str, 'FakeDocument'] = {}
        self._open_failures: Dict[str, EngineError] = {}

        self.calls: List[str] = []
        """Every call made, in order, for asserting on what the CLI asked the engine to do."""

        self.canvases: List['FakeCanvas'] = []
        """Canvases handed out, in order of creation."""

        self.initialisation_failure: Optional[EngineError] = None
        """Set to fail initialize, the way a bad license or a missing library does."""

        self.canvas_failure: Optional[EngineError] = None
        """Set to fail every make_canvas."""

        self.during_open: Optional[Callable[[], None]] = None
        """Run at the start of an open, before the cancellation poll."""


        self.catalog_failure: Optional[EngineError] = None
        """
        Set to fail get_supported_formats and get_available_options.

        The catalogues are the two commands that read no document, so there is no
        per-input failure to script -- the whole call either answers or does not.
        """

        self.formats: List[EngineFormat] = []
        self.options: List[EngineOption] = []

        self.is_disposed = False
        """
        C#'s IsDisposed, whose setter is private.

        A plain attribute rather than a property with no setter: nothing here is
        protecting an invariant, and a test that wanted to lie to itself about whether
        the engine was closed could do so just as easily through the C#'s private
        field. Every fake in this package settles it the same way.
        """

        self.temp_path: Optional[str] = None
        """The last path set_temp_path was given, or None."""

        self.memory_status = "heap: 0 bytes"
        """What get_memory_status answers."""

    # ------------------------------------------------------------- building

    def add_document(self, path: str, text: str = "") -> 'FakeDocument':
        """Adds a document the engine can open."""
        document = FakeDocument(path)
        document.text = text
        self._documents[path] = document
        return document

    def fail_to_open(self, path: str, error: EngineError) -> 'FakeDocumentFiltersEngine':
        """
        Makes opening path fail with a given engine error.

        EngineError.PASSWORD is the one worth calling out: the real engine reports a
        wrong password and a refused prompt identically, so a test using it covers
        both.
        """
        self._open_failures[path] = error
        return self

    def add_protected_document(self, path: str, password: str, text: str = "") -> 'FakeDocument':
        """Adds a document that opens only when the right password is supplied."""
        document = self.add_document(path, text)
        document.password = password
        return document

    # -------------------------------------------------------------- engine

    def initialize(self, license_key: str, library_path: str) -> None:
        self.calls.append("Initialize({0},{1})".format(
            license_key if license_key is not None else "null",
            library_path if library_path is not None else "null"))

        if self.initialisation_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.initialisation_failure,
                                                    "initialisation failed")

    def open_document(self, path: str, flags: int, options: str,
                      password: Optional[Callable[[], Optional[str]]] = None,
                      should_cancel: Optional[Callable[[], bool]] = None,
                      log_level_for: Optional[Callable[[str], int]] = None,
                      on_log_message: Optional[Callable[[int, str, str], None]] = None) -> IDocument:
        self.calls.append("OpenDocument({0},{1},{2}{3})".format(
            path, flags, options if options is not None else "",
            ",cancellable" if should_cancel is not None else ""))

        # Runs before the cancellation poll, so a test can make something true *during*
        # the open rather than before it. Ctrl+C arriving mid-open is the only way to
        # reach the cancellation path at all -- the batch runner checks for an interrupt
        # between documents, so one requested earlier stops the run before anything opens.
        if self.during_open is not None:
            self.during_open()

        # The engine polls this while it parses; the fake polls it once, which is
        # enough to prove a caller wired it up and translates the refusal.
        if should_cancel is not None and should_cancel():
            raise FakeDocumentFiltersEngine.failure(EngineError.CANCELLED,
                                                    "cancelled while opening " + path)

        if path in self._open_failures:
            raise FakeDocumentFiltersEngine.failure(self._open_failures[path],
                                                    "could not open " + path)

        if path not in self._documents:
            raise FakeDocumentFiltersEngine.failure(EngineError.OPEN_ERROR,
                                                    "no such document: " + path)

        document = self._documents[path]

        if document.password is not None:
            # The real engine asks through the callback and turns a refusal into
            # IGR_E_PASSWORD out of Open, so a wrong answer and no answer land here the
            # same way.
            offered = password() if password is not None else None
            if offered != document.password:
                raise FakeDocumentFiltersEngine.failure(EngineError.PASSWORD,
                                                        "password required for " + path)

        # The engine asks the level per module, then hands back messages. Answering
        # once with a known module is enough to show a caller wired both halves.
        if log_level_for is not None and on_log_message is not None:
            level = log_level_for("pdf")
            if level > 0:
                on_log_message(level, "pdf", "opened " + path)

        document.open_flags = flags
        document.open_options = options
        document.was_opened = True
        return document

    def open_document_from_stream(self, source: BinaryIO, display_name: str, flags: int,
                                  options: str,
                                  password: Optional[Callable[[], Optional[str]]] = None,
                                  should_cancel: Optional[Callable[[], bool]] = None,
                                  log_level_for: Optional[Callable[[str], int]] = None,
                                  on_log_message: Optional[Callable[[int, str, str], None]] = None
                                  ) -> IDocument:
        """
        The stream form. Records the stream's length, which is what the debug switches
        that use it are actually about.
        """
        self.calls.append("OpenDocument(<stream:{0}>,{1},{2},{3}{4}".format(
            _stream_length(source), display_name, flags,
            options if options is not None else "",
            ",cancellable)" if should_cancel is not None else ")"))

        return self.open_document(display_name, flags, options, password, should_cancel,
                                  log_level_for, on_log_message)

    def set_temp_path(self, path: str) -> None:
        self.calls.append("SetTempPath({0})".format(path))
        self.temp_path = path

    def get_memory_status(self) -> str:
        self.calls.append("GetMemoryStatus()")
        return self.memory_status

    def identify_file_from_stream(self, source: BinaryIO, display_name: str) -> EngineFileType:
        """The stream form, for the debug switches that reach identify too."""
        self.calls.append("IdentifyFile(<stream:{0}>,{1})".format(_stream_length(source),
                                                                  display_name))
        return self.identify_file(display_name)

    def identify_file(self, path: str) -> EngineFileType:
        self.calls.append("IdentifyFile({0})".format(path))

        if path in self._open_failures:
            raise FakeDocumentFiltersEngine.failure(self._open_failures[path],
                                                    "could not identify " + path)

        if path not in self._documents:
            raise FakeDocumentFiltersEngine.failure(EngineError.OPEN_ERROR,
                                                    "no such document: " + path)

        document = self._documents[path]

        # Built and then assigned, where the C# uses an object initialiser: EngineFileType
        # lives above the seam and takes no constructor arguments, and adding some for the
        # tests' convenience would be a test changing production API.
        file_type = EngineFileType()
        file_type.id = document.file_type
        file_type.mime_type = document.mime_type
        file_type.name = document.format_name
        file_type.capabilities = document.capabilities
        return file_type

    def make_canvas(self, target: str, canvas_type: CanvasType, options: str) -> IOutputCanvas:
        self.calls.append("MakeCanvas({0},{1},{2})".format(
            target, _canvas_type_name(canvas_type), options if options is not None else ""))

        if self.canvas_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.canvas_failure,
                                                    "could not create canvas at " + target)

        canvas = FakeCanvas(target, canvas_type, options)
        self.canvases.append(canvas)
        return canvas

    def make_canvas_on_stream(self, target: BinaryIO, canvas_type: CanvasType,
                              options: str) -> IOutputCanvas:
        self.calls.append("MakeCanvas(<stream>,{0},{1})".format(
            _canvas_type_name(canvas_type), options if options is not None else ""))

        if self.canvas_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.canvas_failure,
                                                    "could not create canvas on a stream")

        canvas = FakeCanvas(None, canvas_type, options)
        self.canvases.append(canvas)
        return canvas

    def get_supported_formats(self) -> List[EngineFormat]:
        self.calls.append("GetSupportedFormats()")

        if self.catalog_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.catalog_failure,
                                                    "catalogue unavailable")

        return self.formats

    def get_available_options(self) -> List[EngineOption]:
        self.calls.append("GetAvailableOptions()")

        if self.catalog_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.catalog_failure,
                                                    "catalogue unavailable")

        return self.options

    def describe_version(self) -> str:
        return "0.0.0-fake"

    def close(self) -> None:
        self.is_disposed = True

    @staticmethod
    def failure(error: EngineError, message: str) -> EngineException:
        """
        An engine failure with the code the real library would report.

        A staticmethod on the class rather than a module-level function: the C# is
        `internal static` on a sealed instance class, which this port keeps on the
        class (the rule execution/subfile_extraction.py and
        execution/canvas_conversion.py already follow). Only a `private static` member
        becomes a module-level function.
        """
        return EngineException(error, int(error), message)


class FakeDocument(IDocument):
    """A document the fake engine can hand out."""

    def __init__(self, path: str):
        self._text_offset = 0

        self.path = path

        self.text = ""
        """What get_text streams out, in 4k-ish chunks as the real one does."""

        self.password: Optional[str] = None
        """When set, open_document demands exactly this password."""

        self.file_type = 254
        """Scripted answers. The interface members below are the calls over them."""

        self.mime_type = "application/octet-stream"
        self.format_name = "Fake Format"
        self.capabilities = 0

        self.page_count = 0
        self.pages: List['FakePage'] = []
        self.sub_file_entries: List['FakeSubFile'] = []
        self.image_entries: List['FakeSubFile'] = []
        self.bookmarks: Optional[EngineBookmark] = None

        self.read_failure: Optional[EngineError] = None
        """Set to make a call on the open document fail."""

        self.read_failure_on: Optional[str] = None
        """Which call read_failure applies to; None means all of them."""

        self.was_opened = False
        self.is_disposed = False
        self.open_flags = 0
        self.open_options: Optional[str] = None
        self.doc_packs_written: List[str] = []

    def get_file_type(self) -> int:
        self._check("GetFileType")
        return self.file_type

    def get_mime_type(self) -> str:
        self._check("GetMimeType")
        return self.mime_type

    def get_format_name(self) -> str:
        self._check("GetFormatName")
        return self.format_name

    def get_file_capabilities(self) -> int:
        self._check("GetFileCapabilities")
        return self.capabilities

    def get_eof(self) -> bool:
        self._check("GetEOF")
        return self._text_offset >= len(self.text)

    def get_text(self, max_length: int) -> str:
        self._check("GetText")

        take = min(max_length, len(self.text) - self._text_offset)
        if take <= 0:
            return ""

        chunk = self.text[self._text_offset:self._text_offset + take]
        self._text_offset += take
        return chunk

    def get_page_count(self) -> int:
        self._check("GetPageCount")
        return len(self.pages) if len(self.pages) > 0 else self.page_count

    def get_page(self, page_index: int) -> IDocumentPage:
        self._check("GetPage")

        if page_index < 0 or page_index >= self.get_page_count():
            raise FakeDocumentFiltersEngine.failure(
                EngineError.NOT_FOUND, "no page {0} in {1}".format(page_index, self.path))

        # A document given only a page count, not page contents, still renders.
        while len(self.pages) <= page_index:
            self.pages.append(FakePage())

        return self.pages[page_index]

    @property
    def sub_files(self) -> List[ISubFileEntry]:
        self._check("SubFiles")
        # list(...) is Cast<ISubFileEntry>().ToList(): the copy is the point, since the
        # caller must not be able to mutate the scripted list through the interface.
        # Python needs no cast, because a FakeSubFile already is an ISubFileEntry.
        return list(self.sub_file_entries)

    @property
    def images(self) -> List[ISubFileEntry]:
        self._check("Images")
        return list(self.image_entries)

    @property
    def root_bookmark(self) -> Optional[EngineBookmark]:
        self._check("RootBookmark")
        return self.bookmarks

    def make_doc_pack(self, filename: str, options: str) -> None:
        self._check("MakeDocPack")
        self.doc_packs_written.append(filename)

    def make_doc_pack_on_stream(self, destination: BinaryIO, options: str) -> None:
        self._check("MakeDocPack")
        self.doc_packs_written.append("<stream>")

    def close(self) -> None:
        self.is_disposed = True

    def _check(self, call: str) -> None:
        if (self.read_failure is not None
                and (self.read_failure_on is None or self.read_failure_on == call)):
            raise FakeDocumentFiltersEngine.failure(
                self.read_failure, "{0} failed on {1}".format(call, self.path))


class FakePage(IDocumentPage):
    """One page of a FakeDocument."""

    def __init__(self):
        self.width = 612
        """Scripted answers, read back through get_width and get_height."""

        self.height = 792

        self.attributes: Dict[str, str] = {}
        self.hyperlink_list: List[EngineHyperlink] = []
        self.form_element_list: List[EngineFormElement] = []
        self.annotation_list: List[EngineAnnotation] = []
        self.root_element: Optional[EnginePageElement] = None
        self.is_disposed = False

    def get_width(self) -> int:
        return self.width

    def get_height(self) -> int:
        return self.height

    def get_attribute(self, name: str) -> Optional[str]:
        return self.attributes.get(name, "")

    @property
    def hyperlinks(self) -> List[EngineHyperlink]:
        return self.hyperlink_list

    @property
    def form_elements(self) -> List[EngineFormElement]:
        return self.form_element_list

    @property
    def annotations(self) -> List[EngineAnnotation]:
        return self.annotation_list

    def get_root_page_element(self) -> Optional[EnginePageElement]:
        return self.root_element

    def close(self) -> None:
        self.is_disposed = True


class FakeCanvas(IOutputCanvas):
    """A canvas that records what was drawn on it instead of writing bytes."""

    def __init__(self, target: Optional[str], canvas_type: CanvasType, options: Optional[str]):
        self.target = target
        """The path it was opened on, or None for a stream canvas."""

        # canvas_type, not type: the C# property is Type, but a parameter named `type`
        # would shadow the builtin for the whole body, and the seam already calls this
        # argument canvas_type in make_canvas. The attribute keeps the reference's name,
        # which costs nothing -- self.type resolves the attribute, never the builtin --
        # and is what EngineAnnotation and EnginePageElement already do.
        self.type = canvas_type
        self.options = options

        self.rendered_pages: List[IDocumentPage] = []
        """Pages rendered onto it, in order."""

        self.decorations: List[Optional[EnginePageDecorations]] = []
        """Decorations passed with each rendered page; None where none were given."""

        self.rendered_documents: List[IDocument] = []
        """Documents rendered whole, through the one-call path."""

        self.images_drawn = 0
        self.is_disposed = False

        self.render_failure: Optional[EngineError] = None
        """Set to fail rendering, the way a full disk or a missing font does."""

    def render_page(self, page: IDocumentPage, options: str,
                    decorations: Optional[EnginePageDecorations]) -> None:
        if self.render_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.render_failure, "render failed")

        self.rendered_pages.append(page)
        self.decorations.append(decorations)

    def render_pages(self, document: IDocument) -> None:
        if self.render_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.render_failure, "render failed")

        self.rendered_documents.append(document)

    def draw_image(self, x: int, y: int, image: bytes, mime_type: str) -> None:
        self.images_drawn += 1

    def close(self) -> None:
        self.is_disposed = True


class FakeSubFile(ISubFileEntry):
    """One entry inside a fake container."""

    # These four shadow read-only properties on ISubFileEntry, and have to be declared
    # in the class body to do it. A property is a data descriptor on the type, and a
    # data descriptor wins over the instance dictionary, so `self.id = id` in __init__
    # would raise AttributeError against the inherited getter rather than setting
    # anything. A plain class attribute here replaces the descriptor in the MRO, which
    # makes ordinary instance assignment work again -- the Python equivalent of the C#
    # writing `{ get; set; }` over the interface's getter. The values are only the
    # defaults __init__ overwrites; nothing reads them.
    id = None
    name = None
    size = 0
    modified = None

    def __init__(self, name: str, id: str = "0001"):
        # `id` keeps the reference's parameter name, shadowing the builtin for this body
        # only -- which does not call it. A parameter name is visible in a keyword call,
        # so the four ports agree on it.
        self.name = name
        self.id = id
        self.size = 0
        self.modified: Optional[datetime.datetime] = None

        self.content: Optional[FakeDocument] = None
        """What this entry is when opened, for nested containers."""

        self.open_failure: Optional[EngineError] = None
        """Set to fail open."""

        self.copy_failure: Optional[EngineError] = None
        """Set to fail copy_to, the way an unreadable entry does."""

        self.copied_to: List[str] = []
        """Paths this entry was copied to."""

        self.is_disposed = False

    def open(self, flags: int, options: str) -> IDocument:
        if self.open_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.open_failure,
                                                    "could not open subfile " + self.name)

        # `is not None` rather than `or`, which is C#'s ?? exactly: a FakeDocument is
        # always truthy today, but a fake that grew a __len__ or __bool__ would start
        # silently handing back a fresh empty document instead of the scripted one.
        return self.content if self.content is not None else FakeDocument(self.name)

    def copy_to(self, path: str) -> None:
        if self.copy_failure is not None:
            raise FakeDocumentFiltersEngine.failure(self.copy_failure,
                                                    "could not copy subfile " + self.name)

        self.copied_to.append(path)

    def close(self) -> None:
        self.is_disposed = True


def _stream_length(source: BinaryIO) -> int:
    """
    How many bytes source holds, without disturbing where it is reading from.

    Not in the C#, which asks a Stream for its Length and gets an answer that costs
    nothing and moves nothing. A Python file object has no length, so the only way to
    ask is to seek to the end and look -- and the position has to be put back
    afterwards, because the caller is about to hand this same stream to the engine.
    Forgetting that would make every stream-form call appear to open an empty
    document, and the debug switches this exists for are precisely the ones whose
    whole subject is which bytes arrive.
    """
    position = source.tell()
    try:
        return source.seek(0, io.SEEK_END)
    finally:
        source.seek(position, io.SEEK_SET)


def _canvas_type_name(canvas_type: CanvasType) -> str:
    """
    A canvas device as the call log names it.

    C# interpolates the enum, which writes the member's name. .name is that, and for
    every device but one the two ports spell it identically, because the .NET
    binding's CanvasType members are already upper case: only WebSafe there is
    WEB_SAFE here. Call-log text rather than output, so the difference is confined to
    a test assertion for a device no format in the catalogue selects -- and the enum
    rule that bans .name for printing does not apply, because nothing here is printed.
    """
    return canvas_type.name
