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

from enum import Enum, IntEnum
from typing import Optional


class FormatKind(Enum):
    """How a target format is produced."""

    TEXT = 'text'
    """Streamed from the extractor with get_text or save_to; no canvas involved."""

    CANVAS = 'canvas'
    """Rendered page by page onto a canvas from make_output_canvas."""

    DOC_PACK = 'docpack'
    """Produced by Extractor.make_doc_pack, which is neither of the above."""


class CanvasType(IntEnum):
    """
    The engine's output canvas devices, as accepted by make_output_canvas.

    Not in the C# file: there the CanvasType enum comes from the
    Hyland.DocumentFilters .NET binding. The Python binding publishes only some of
    these devices as IGR_DEVICE_* integers — gif, dcx, tga, brk, pcx, jpeg2000 and
    web-safe are missing from it — so the catalogue could not name every device it
    needs through the binding, and importing the binding here would also make this
    pure-contract module unusable without the native library on the path. The values
    are the IGR_DEVICE_* numbers, which are ABI and therefore cannot drift.
    """

    PNG = 0
    JPG = 1
    PDF = 2
    TIF = 3
    BMP = 4
    XML = 5
    HTML = 6
    PBM = 7
    PGM = 8
    PPM = 9
    WEBP = 10
    XPS = 11
    SVG = 12
    EPS = 13
    PS = 14
    WEB_SAFE = 15
    TGA = 16
    BRK = 17
    PCX = 18
    DCX = 19
    JPEG2000 = 20
    GIF = 21
    JSON = 22
    MARKDOWN = 23


class FormatSpec:
    """
    Everything the planner needs to know about one --to value.

    The two attributes that drive the output model are is_single_file and
    is_hi_def_capable. Both are static properties of the format, so -o never has to
    guess and never depends on the document.
    """

    def __init__(self, token: str, kind: FormatKind, canvas_type: Optional[CanvasType],
                 is_single_file: bool, supports_combine: bool, is_hi_def_capable: bool,
                 extension: str, display_name: str, canvas_options: str = ''):
        self.token = token
        """The token the user writes, e.g. pdf. Always lower case."""

        self.kind = kind
        """Which engine path renders it."""

        # None for text and docpack, which use no canvas — not zero, which is a real
        # device (PNG).
        self.canvas_type = canvas_type
        """The canvas device, when kind is FormatKind.CANVAS."""

        # These are defaults, not identity. The engine resolves a repeated key to its
        # first occurrence, so appending them leaves --canvas-options in front and the
        # user wins every key they name — which is why there is one attribute here and
        # not a second one for overridable settings.
        #
        # JSON_OUTPUT_SCHEMA=MDAST; is what tells mdast apart from json: both are the
        # JSON canvas, and an implementation that drops it writes plain JSON and looks
        # correct.
        #
        # HTML_INLINE_IMAGES and SVG_INLINE_IMAGES both default to off in the engine,
        # which for html5 and svg is never what this tool wants: those canvases
        # rasterise each page into a sibling image and lay a transparent text layer
        # over it, but only the markup is written to --out-dir, so the sibling never
        # lands and every page renders blank. Turning them on inlines the raster as a
        # data: URI, which also gives --out-file and --stdout the single
        # self-contained file they promise. doc2text forces the same two, in main.cpp,
        # for the same reason.
        self.canvas_options = canvas_options
        """Canvas options the format itself contributes, appended after whatever
        --canvas-options supplies. Empty for most."""

        self.is_single_file = is_single_file
        """True when the format writes exactly one file however many pages the
        document has (pdf, tiff, text). False when it writes one file per page
        (png, jpg, svg), which is what makes --out-file and --stdout illegal for it."""

        # A separate question from is_single_file, and the two only look alike. docpack
        # writes one file per document, like pdf, but Extractor.make_doc_pack takes one
        # extractor, so a pack cannot hold two documents. Deriving --combine from
        # is_single_file lets --to docpack --combine pass validation and fail at
        # execution.
        self.supports_combine = supports_combine
        """True when several inputs may merge into one output under --combine."""

        self.is_hi_def_capable = is_hi_def_capable
        """True when the format can carry high-definition, paginated output."""

        # web-safe is the one entry whose extension is a placeholder, unknown: the
        # engine picks png, gif, jpg or webp from the bit depth it settles on, which is
        # not knowable until the page has been rendered, so execution renames the file
        # once the real type is detected. That rename is an execution concern and needs
        # nothing here — the renamed path goes through
        # OutputPlanner.verify_page_target like every other per-page output.
        self.extension = extension
        """The filename extension, without a dot, used by {ext} in an output template."""

        self.display_name = display_name
        """Human-readable name for help and error text."""

    def __str__(self) -> str:
        """The canonical text form, used by the conformance corpus."""
        # C# derives the kind word by lower-casing the enum member name, which works
        # there because DocPack folds to docpack. The Python members are UPPER_SNAKE,
        # so DOC_PACK would fold to doc_pack; the canonical spelling is the enum's
        # value instead. Nothing else here needs folding: token is lower case by
        # construction, and the rest of the words are literals.
        parts = [self.token, ' ', self.kind.value, ' ',
                 'single-file' if self.is_single_file else 'per-page']

        if self.is_hi_def_capable:
            parts.append(' hidef')

        return ''.join(parts)
