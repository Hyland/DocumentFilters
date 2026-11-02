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

from ..configuration.command_options import ConvertOptions
from ..configuration.shared_options import SharedOptions
from ..engine.i_document_filters_engine import OpenType
from ..planning.format_spec import FormatKind, FormatSpec


class ExtractorOpenSettings:
    """
    How a document should be opened: the engine flags and the options string passed to
    IDocument.open.

    These have to be decided *before* the document is opened, because the engine applies
    them during open -- a flag worked out afterwards has no effect. That is why
    DocumentFiltersSession.open_document takes this rather than each conversion path
    computing flags for a document that is already open.
    """

    def __init__(self):
        self.flags = 0
        """
        The complete IGR_ open flags: the content selector, plus exactly one Format*
        bit -- or none, where the engine's default is what is wanted.

        Complete because it has to be: these reach the engine exactly as built here, and
        nothing downstream supplies a format bit. Pairing a partial set with an OpenMode
        does not work -- that binding overload masks out bits 16 to 31 and substitutes the
        format its mode implies -- see IDocumentFiltersEngine.open_document.

        The Format* values are an enumeration packed into bits 16 to 19, not independent
        bits (Text 0x1, HTML 0x2, Image 0x5, Markdown 0x8, each shifted by 16), so at most
        one may ever be set.
        """

        self.options = ''
        """The engine options string, from --options and --codepage."""

    @staticmethod
    def for_format(format: FormatSpec, options: ConvertOptions,
                   shared: SharedOptions) -> 'ExtractorOpenSettings':
        """
        The settings for converting to a given target format.

        --no-images reaches a text-mode format here rather than through the canvas options,
        because md-text never opens a canvas. md, which does, is handled by
        CanvasConversion.compose_canvas_options; both read the same rule from
        CanvasConversion.image_suppression so it is stated once.
        """
        # Imported here rather than at the top of the module, which is the one place this
        # port has to depart from the reference's shape. The three types form a cycle --
        # this file calls CanvasConversion, which holds a DocumentFiltersSession, whose
        # open_document takes an ExtractorOpenSettings -- and a cycle inside one assembly
        # costs C# nothing, where in Python a module-level import of canvas_conversion
        # would run it before this module finished defining the class it needs.
        #
        # This is the edge to break because it is the only one of the three that is a live
        # call: the other two are type annotations, which cost nothing at import. Deferring
        # the import to the one function that uses it also keeps the break local and
        # obvious, rather than making every future import of this module depend on how the
        # other two files happen to spell their annotations.
        from . import canvas_conversion

        engine_options = _build_options_string(shared)
        if options.no_images and format.kind == FormatKind.TEXT:
            suppressed = canvas_conversion.CanvasConversion.image_suppression(format)
            if len(suppressed) > 0:
                engine_options = _append(engine_options, suppressed)

        settings = ExtractorOpenSettings()
        settings.flags = int(OpenType.BODY_AND_META) | ExtractorOpenSettings.format_bit(format, options)
        settings.options = engine_options
        return settings

    @staticmethod
    def format_bit(format: FormatSpec, options: ConvertOptions) -> int:
        """
        The one IGR_FORMAT_* bit a conversion opens with, or zero for none.

        A canvas format needs FormatImage: that is what makes pages exist. A text format
        names the reader it wants, and text asks for none at all under
        --include-control-codes -- omitting IGR_FORMAT_TEXT is what leaves the ISYS control
        codes in the stream instead of having the engine clean them out. docpack takes none
        either, matching doc2text, which opens it with a bare IGR_BODY_AND_META.

        Structure reports need no case of their own. They require a paginated handle, and
        OptionsValidator.check_reports_need_canvas_format refuses them against anything but
        a canvas format, which already opens paginated -- so by the time this runs, a run
        with reports has a canvas format. Forcing the bit here instead was tried and does
        not work: it leaves a text conversion with no text reader.

        Pure, and separate from for_format, so the whole table can be checked without an
        engine.
        """
        if format.kind == FormatKind.CANVAS:
            return int(OpenType.FORMAT_IMAGE)

        if format.kind == FormatKind.DOC_PACK:
            return 0

        if format.token == "text":
            return 0 if options.include_control_codes else int(OpenType.FORMAT_TEXT)
        if format.token == "md-text":
            return int(OpenType.FORMAT_MARKDOWN)
        if format.token == "html":
            return int(OpenType.FORMAT_HTML)
        return int(OpenType.FORMAT_TEXT)

    @staticmethod
    def for_inspection(shared: SharedOptions, enumerate_images: bool = False) -> 'ExtractorOpenSettings':
        """
        The settings for a command that reads a document without converting it --
        identify, meta, list and extract.

        FormatText: these commands enumerate subfiles and read metadata rather than
        rendering, so they want the text reader and no pages.

        ENUMERATE_IMAGES has to be set here, at open, because that is the only place the
        engine reads it -- IDocument.images is empty without it, and so is the subfile list
        for a document whose only subfiles are images. Setting it afterwards, or not at all
        and iterating container.images anyway, finds nothing. Appended after the user's
        --options so an explicit ENUMERATE_IMAGES=OFF still wins, the engine resolving a
        repeated key to its first occurrence.

        Args:
            enumerate_images: extract --images: ask the engine to surface a document's
                embedded images as well as its subfiles.
        """
        options = _build_options_string(shared)
        if enumerate_images:
            if len(options) > 0 and not options.endswith(';'):
                options += ';'
            options += "ENUMERATE_IMAGES=ON;"

        settings = ExtractorOpenSettings()
        settings.flags = int(OpenType.BODY_AND_META) | int(OpenType.FORMAT_TEXT)
        settings.options = options
        return settings

    @staticmethod
    def for_metadata_only(shared: SharedOptions) -> 'ExtractorOpenSettings':
        """
        The settings for meta specifically: OpenType.META_ONLY rather than the
        BODY_AND_META every other inspection command uses.

        Confirmed against doc2text's own --meta flag (main.cpp: flags = IGR_META_ONLY; --
        an assignment that replaces the default entirely, not an addition to it).
        MetadataReport parses get_text's output as one "Label: value" line per field;
        opening with body text included as well would mix document content into that same
        stream.
        """
        settings = ExtractorOpenSettings()
        settings.flags = int(OpenType.META_ONLY) | int(OpenType.FORMAT_TEXT)
        settings.options = _build_options_string(shared)
        return settings


def _append(existing: str, addition: str) -> str:
    """Joins two engine option strings, inserting a separator where needed."""
    if len(existing) > 0 and not existing.endswith(';'):
        existing += ';'
    return existing + addition + ';'


def _build_options_string(shared: SharedOptions) -> str:
    """
    Builds the semicolon-separated engine options string from --options and --codepage, in
    the form doc2text itself uses: ...;CODEPAGE=1252;.
    """
    options = shared.processing.engine_options or ''

    # Plain truthiness is string.IsNullOrEmpty exactly, for a string: None and '' are the
    # only falsy values an Optional[str] has. Deliberately not is_all_whitespace, which is
    # the wider IsNullOrWhiteSpace the reference did not ask for here -- a codepage of
    # spaces is a bad value the engine should reject, not one this silently drops.
    if shared.processing.codepage:
        if len(options) > 0 and not options.endswith(';'):
            options += ';'
        options += "CODEPAGE=" + shared.processing.codepage + ";"
    return options
