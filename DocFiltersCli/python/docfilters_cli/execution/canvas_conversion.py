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
import itertools
import os
import shutil
from typing import BinaryIO, List, Optional

from ..configuration.command_options import ConvertOptions
from ..engine.i_document_filters_engine import (EngineFormValue, EnginePageDecorations,
                                                EngineRedaction, IDocument, IDocumentPage,
                                                IOutputCanvas)
from ..planning import output_planner
from ..planning.format_spec import CanvasType, FormatSpec
from ..planning.page_range import PageRange
from ..planning.run_plan import PlannedItem


class CanvasConversion:
    """
    The high-definition path: open a canvas, walk the pages, render each one.
    Covers PDF, TIFF, the image formats, HTML5, structured XML, JSON and
    high-definition markdown.
    """

    def __init__(self, session, console, shared=None, file_system=None):
        """
        Takes the session because canvases are made by DocumentFilters.make_output_canvas
        -- an IDocument cannot produce one, so a document alone is not enough to render
        anything.

        session is deliberately unannotated. Annotating it would mean importing
        document_filters_session, and that closes a cycle: extractor_open_settings calls
        image_suppression below, this class holds a session, and the session's
        open_document takes an ExtractorOpenSettings. In Python that is a real import
        cycle where in one C# assembly it is nothing. Nothing here needs the type -- only
        the object, whose methods are reached without importing anything -- so the
        annotation is what goes, following engine_log.py's treatment of console.

        shared is the output policy, for checking each per-page target as it is resolved.
        file_system is None in tests that render nowhere and verify nothing.
        """
        self._session = session
        self._console = console
        self._shared = shared
        self._file_system = file_system

    def convert(self, document: IDocument, item: PlannedItem, format: FormatSpec,
                options: ConvertOptions) -> None:
        """
        Renders one document to its own output. Used for both single-file formats
        (one canvas, many pages) and one-per-page formats (one canvas per page,
        named from PlannedItem.target_pattern as the pages are found).
        """
        canvas_options = self.compose_canvas_options(format, options.canvas_options, options.no_images)

        if item.target is not None:
            with self._make_canvas(item.target, format.canvas_type, canvas_options, options) as canvas:
                self.append_to(canvas, document, item.pages, options, item.display_name)
        else:
            for page_number in self.renderable_pages(item.pages, document.get_page_count()):
                target = item.target_pattern.resolve(item.source_path, page_number)

                # Each page, not just the first. Planning can only check the first target
                # of a per-page item -- how many pages a document has is not known until it
                # is open -- so the rest have to be checked here, as they are resolved.
                # Without this, an existing page 2 was overwritten without --overwrite
                # whenever page 1 happened to be absent: exit 0, nothing reported.
                self._verify_page_target(target, options)

                with self._make_canvas(target, format.canvas_type, canvas_options, options) as canvas:
                    self._render_page(canvas, document, page_number, options, item.display_name)

                # After the canvas is closed, so the bytes are on disk to be identified.
                self._name_by_rendered_type(target, format, options)

    def convert_to_stream(self, document: IDocument, destination: BinaryIO, item: PlannedItem,
                          format: FormatSpec, options: ConvertOptions) -> None:
        """
        Renders one document straight to destination -- for --stdout, where a single-file
        canvas format owns no path at all.

        convert only ever branches on PlannedItem.target versus
        PlannedItem.target_pattern, because those are the two shapes OutputPlanner
        produces for a real file target -- but --stdout resolves to
        OutputCardinality.STREAM, which leaves both None. Convert's own else branch used
        to be reached with a None target_pattern in that case and threw a bare
        NullReferenceException -- reported as a document failure with no useful message.
        ConvertCommand now calls this instead whenever --stdout is set, the same way
        TextConversion already had its own stdout branch for text formats; canvas formats
        never did.

        Renders into a seekable buffer first rather than handing destination to the canvas
        directly. Every published sample that streams a canvas
        (ConvertDocumentToTIFFStream, ConvertDocumentToInteractiveHDHTML) writes to an
        in-memory stream for the same reason: finishing a binary format means seeking back
        to patch a header or table computed only once the whole document has rendered, and
        IConsole.open_standard_output is forward-only. Handing the console stream straight
        to the canvas produced no error and no output -- the render itself succeeded, but
        whatever the canvas needed to seek back and write never reached anywhere.
        """
        canvas_options = self.compose_canvas_options(format, options.canvas_options, options.no_images)

        with io.BytesIO() as buffer:
            with self._session.api.make_canvas_on_stream(buffer, format.canvas_type,
                                                         canvas_options) as canvas:
                self.append_to(canvas, document, item.pages, options, item.display_name)

            # copyfileobj rather than write(getvalue()), which is Stream.CopyTo's own
            # chunked read-and-write and not a second copy of the whole rendering.
            buffer.seek(0)
            shutil.copyfileobj(buffer, destination)
            destination.flush()

    @staticmethod
    def compose_canvas_options(format: FormatSpec, user_options: Optional[str],
                               no_images: bool = False) -> str:
        """
        The option string a canvas for format is opened with: --canvas-options first, then
        the format's own.

        The order is the whole of the rule. The engine resolves a repeated key to its
        first occurrence -- CLayoutOptions::parse appends rather than replaces, and find
        returns the first match -- so putting the user in front means they win every key
        they name, and FormatSpec.canvas_options only fills in what they left alone.

        Used by both canvas paths, here and --combine's shared canvas in ConvertCommand,
        because a combined run produces the same format and needs the same options. Static
        and free of the engine so the ordering can be tested directly.
        """
        # str.strip(';') is a character-set trim, which is what C#'s Trim(';') is, and so
        # is deliberately not value_parsers.trim: an option string ending in a space keeps
        # that space here, exactly as the reference leaves it.
        user = (user_options or '').strip(';')
        own = (format.canvas_options or '').strip(';')

        suppressed = CanvasConversion.image_suppression(format) if no_images else ''
        if len(suppressed) > 0:
            own = suppressed if len(own) == 0 else own + ";" + suppressed

        if len(user) == 0:
            return own
        if len(own) == 0:
            return user

        return user + ";" + own

    @staticmethod
    def image_suppression(format: Optional[FormatSpec]) -> str:
        """
        What --no-images means for format, or nothing when it means nothing.

        Only the markdown formats have an engine switch for this --
        MARKDOWN_INCLUDE_IMAGES. Plain text has no way to carry an image in the first
        place, and the flag is deliberately ignored for html5 and svg: those inline the
        page raster itself (D35), so suppressing images there would not drop embedded
        pictures, it would blank the page. Ignored rather than refused, so the flag can be
        set once for a batch of mixed formats without a run failing on the ones it does not
        touch.
        """
        return "MARKDOWN_INCLUDE_IMAGES=OFF" \
            if format is not None and (format.token == "md" or format.token == "md-text") \
            else ''

    def append_to(self, canvas: IOutputCanvas, document: IDocument, pages: PageRange,
                  options: ConvertOptions, display_name: str) -> None:
        """
        Renders one document onto a canvas someone else owns, for --combine. The caller
        opens the canvas once and passes every document through in turn.

        Takes the fast, single-call render_pages path -- the one the published samples use
        -- whenever nothing needs a per-page hook; an explicit --pages selection or any
        decoration (including --fingerprint, which needs the page open after it renders)
        falls back to rendering one page at a time.
        """
        if pages.is_all and not _has_page_decorations(options) and not options.fingerprint:
            canvas.render_pages(document)
        else:
            for page_number in self.renderable_pages(pages, document.get_page_count()):
                self._render_page(canvas, document, page_number, options, display_name)

    @staticmethod
    def renderable_pages(pages: PageRange, page_count: int) -> List[int]:
        """
        The pages of pages that a document of page_count pages actually has.

        PageRange.enumerate deliberately does not filter an explicit selection by count --
        it has no way to know whether a count it is handed is genuine -- so that filtering
        happens here instead, at the one point that does have a real, just-verified page
        count: after the document is open. Kept as a pure function of the range and the
        count, rather than folded inline, so it is testable without the engine.
        PageRange.ALL never needs the filter -- enumerate already stops at page_count for
        it -- but applying it uniformly is simpler than special-casing which branch needs
        it.

        takewhile and not a filtering comprehension, as the C# uses TakeWhile and not Where:
        the pages arrive ascending, so the first one past the end means every one after it
        is too, and a filter would still walk all two billion of --pages 1-2147483647 to
        find that out.

        A list where the C# returns IEnumerable<int>. Both callers iterate it once and
        immediately, and a generator would only defer the work while making the empty case
        harder to read.
        """
        return list(itertools.takewhile(lambda page_number: page_number <= page_count,
                                        pages.enumerate(page_count)))

    def _name_by_rendered_type(self, target: str, format: FormatSpec, options: ConvertOptions) -> None:
        """
        Renames a web-safe page to match whatever the engine actually wrote.

        web-safe is the one format whose extension cannot be planned: the engine chooses
        PNG, GIF, JPEG or WebP per page, from the page's own content. The planner therefore
        names the file .unknown, and this puts the real name on it afterwards.

        The type comes from the engine identifying the file it just wrote, mapped by MIME
        type: the MIME strings are public API, and identify_file already returns them. The
        destination goes through the same output policy as any other target, rather than
        whatever is in the way being deleted.
        """
        if format.token != "web-safe" or options.fingerprint:
            return

        extension = _extension_for_mime_type(self._session.api.identify_file(target).mime_type)
        if extension is None:
            return

        renamed = _change_extension(target, extension)
        if renamed == target:
            return

        self._verify_page_target(renamed, options)
        self._file_system.move_file(target, renamed)

    def _verify_page_target(self, target: str, options: ConvertOptions) -> None:
        """
        Applies the run's output policy to one resolved per-page target.

        Skipped under --fingerprint, which renders to a null stream and writes no file at
        all, so an existing path of that name is not in anyone's way -- the same exemption
        ConvertCommand makes for verify_targets.
        """
        if options.fingerprint or self._shared is None or self._file_system is None:
            return

        output_planner.verify_page_target(target, self._shared, self._file_system)

    def _make_canvas(self, target: str, canvas_type: CanvasType, canvas_options: str,
                     options: ConvertOptions) -> IOutputCanvas:
        """
        Opens the canvas a page would normally render onto -- or, under --fingerprint, a
        canvas whose bytes are thrown away instead.

        The render still has to happen, because a fingerprint is an attribute of the
        rendered page, not of the document -- but the bytes it produces are never target's
        real content, the same way doc2text's own --fingerprint renders into a temporary
        stream it never keeps (main.cpp, hidecontent). OptionsValidator already rejects
        every argument that would let someone rely on target instead -- --out-dir,
        --out-file, --stdout, --view, --combine -- so it is only ever a plan's
        default-directory guess here, never a path the user asked for by name.
        """
        # The directory is already there: ConvertCommand creates it once per item,
        # because the engine opens this file itself and will not create one.
        if options.fingerprint:
            return self._session.api.make_canvas_on_stream(_NullStream(), canvas_type, canvas_options)

        return self._session.api.make_canvas(target, canvas_type, canvas_options)

    def _render_page(self, canvas: IOutputCanvas, document: IDocument, page_number: int,
                     options: ConvertOptions, display_name: str) -> None:
        with document.get_page(page_number - 1) as page:
            render_opts = options.render_options or ''
            if options.watermark and not os.path.isfile(options.watermark.replace('\\', '/')):
                render_opts += ("" if len(render_opts) == 0 else ";") + "WATERMARK=" + options.watermark
            canvas.render_page(page, render_opts,
                               CanvasConversion.apply_page_decorations(page_number, options))
            self.draw_watermark(canvas, page, options)

            if options.fingerprint:
                self.write_fingerprint(page, page_number, display_name)

    def write_fingerprint(self, page: IDocumentPage, page_number: int, display_name: str) -> None:
        """
        Writes --fingerprint's one line for a page that has just been rendered.

        Confirmed against doc2text's own fingerPrintPage (main.cpp): try the
        IMAGE_FINGERPRINT_V2 page attribute first, fall back to the older
        IMAGE_FINGERPRINT, and print nothing when both are empty -- doc2text only calls
        this after the page has rendered onto a canvas, which is why it lives here rather
        than in StructureReport: the fingerprint is a property of the rendered image, not
        of the document's layout.
        """
        line = self.format_fingerprint(display_name, page_number,
                                       page.get_attribute("IMAGE_FINGERPRINT_V2"),
                                       page.get_attribute("IMAGE_FINGERPRINT"))
        if line is not None:
            self._console.write_line(line)

    @staticmethod
    def format_fingerprint(display_name: str, page_number: int, fingerprint_v2: Optional[str],
                           fingerprint_v1: Optional[str]) -> Optional[str]:
        """
        The formatting half of write_fingerprint, kept as a pure function of the two
        attribute strings so it is testable without the engine.
        """
        # Plain truthiness is string.IsNullOrEmpty for a string: None and '' are the only
        # falsy values an Optional[str] has. Not is_all_whitespace, which is the wider
        # IsNullOrWhiteSpace the reference did not ask for -- a fingerprint of spaces is
        # not something the engine produces, and treating it as absent would be this port
        # inventing a rule.
        fingerprint = fingerprint_v1 if not fingerprint_v2 else fingerprint_v2
        if not fingerprint:
            return None

        return "{0}: page={1} fingerprint={2}".format(display_name, page_number, fingerprint)

    @staticmethod
    def apply_page_decorations(page_number: int,
                              options: ConvertOptions) -> Optional[EnginePageDecorations]:
        """
        Applies --redact and --form to one page before it is rendered.

        None when the page needs neither, so a run using none of them never builds an
        EnginePageDecorations at all. --watermark is handled separately, by draw_watermark.
        """
        decorations = EnginePageDecorations()

        for redaction in [r for r in options.redactions if r.page == page_number]:
            decorations.redactions.append(EngineRedaction(x=redaction.x, y=redaction.y,
                                                          width=redaction.width,
                                                          height=redaction.height))

        for form in [f for f in options.form_values if f.page == page_number]:
            decorations.form_values.append(EngineFormValue(name=form.name, value=form.value))

        return None if decorations.is_empty else decorations

    def draw_watermark(self, canvas: IOutputCanvas, page: IDocumentPage,
                       options: ConvertOptions) -> None:
        if not options.watermark or not os.path.isfile(options.watermark.replace('\\', '/')):
            return

        with open(options.watermark.replace('\\', '/'), 'rb') as source:
            data = source.read()
        canvas.draw_image(0, 0, data, None)


_EXTENSIONS_BY_MIME_TYPE = {
    "image/png": "png",
    "image/gif": "gif",
    "image/jpeg": "jpg",
    "image/webp": "webp",
}


def _extension_for_mime_type(mime_type: Optional[str]) -> Optional[str]:
    """The extension for an image the engine identified, or None if unknown."""
    return _EXTENSIONS_BY_MIME_TYPE.get(mime_type)


def _change_extension(path: str, extension: str) -> str:
    """
    Path.ChangeExtension: whatever follows the last dot of the file name is replaced, and a
    name with no dot gains one.

    Not os.path.splitext, which treats a leading dot as part of the name rather than as an
    extension -- .unknown has no extension to splitext and would become .unknown.png, where
    the reference makes it .png.
    """
    separator = max(path.rfind('/'), path.rfind('\\'))
    dot = path.rfind('.')
    stem = path[:dot] if dot > separator else path
    return stem + "." + extension


def _has_page_decorations(options: ConvertOptions) -> bool:
    return bool(options.watermark) or len(options.redactions) > 0 or len(options.form_values) > 0


class _NullStream:
    """
    The write-only sink --fingerprint renders into, standing in for Stream.Null.

    Not in the C#, which names the framework's own Stream.Null. Python has no equivalent:
    os.devnull is a path, not a stream, and opening it would put a real file handle and a
    syscall per write in the path of every rendered page.

    The position is tracked rather than pinned at zero, which is the one way this differs
    from Stream.Null. It has to be: the binding's stream bridge answers every engine seek
    with tell(), so a stream that reported zero after seeking to 40000 would be telling
    the engine its own seek had failed. .NET's canvas never learns that, because the
    engine reaches Stream.Null through a bridge of the .NET binding's own. Nothing
    observable depends on the choice -- the bytes are discarded either way, which is the
    whole point of the class -- so the safer of the two is taken.

    Reads return nothing, as Stream.Null's do.
    """

    def __init__(self):
        self._position = 0
        self._length = 0

    def write(self, data) -> int:
        self._position += len(data)
        self._length = max(self._length, self._position)
        return len(data)

    def read(self, size: int = -1) -> bytes:
        return b''

    def seek(self, offset: int, whence: int = io.SEEK_SET) -> int:
        if whence == io.SEEK_CUR:
            self._position += offset
        elif whence == io.SEEK_END:
            self._position = self._length + offset
        else:
            self._position = offset
        return self._position

    def tell(self) -> int:
        return self._position

    def flush(self) -> None:
        pass

    def close(self) -> None:
        pass
