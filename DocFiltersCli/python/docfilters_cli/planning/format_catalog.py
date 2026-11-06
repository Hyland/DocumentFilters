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

from typing import List, Optional

from ..configuration.value_parsers import fold_token
from ..runtime.cli_error import ErrorCodes, UsageError
from .format_spec import CanvasType, FormatKind, FormatSpec


class FormatCatalog:
    """
    The --to token table: what each format name means, and the two static properties
    the output planner needs from it.

    This table is the single source of truth for the format surface in all four
    language implementations, and several entries are not a straight mapping onto a
    single engine enum value:

    - md is high-definition markdown via a canvas; md-text is text-mode markdown
      straight from the extractor.
    - html is flow HTML from the extractor; html5 is paginated high-definition HTML
      via a canvas.
    - docpack is neither — it goes through Extractor.make_doc_pack.
    - mdast has no device of its own: it is the JSON canvas under a different schema,
      told apart only by the JSON_OUTPUT_SCHEMA=MDAST; it contributes through
      FormatSpec.canvas_options (D31). Confirmed against doc2text, which spells it the
      same way.
    """

    _ALL: List[FormatSpec] = [

# === text mode: straight from the extractor ================================

FormatSpec(token="text",     kind=FormatKind.TEXT,    canvas_type=None,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=False, extension="txt",
           display_name="Plain text"),

FormatSpec(token="md-text",  kind=FormatKind.TEXT,    canvas_type=None,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=False, extension="md",
           display_name="Markdown (text mode)"),

FormatSpec(token="html",     kind=FormatKind.TEXT,    canvas_type=None,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=False, extension="html",
           display_name="HTML (flow)"),

# === canvas, single file ===================================================

FormatSpec(token="md",       kind=FormatKind.CANVAS,  canvas_type=CanvasType.MARKDOWN,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="md",
           display_name="Markdown (high definition)"),

FormatSpec(token="html5",    kind=FormatKind.CANVAS,  canvas_type=CanvasType.HTML,
           canvas_options="HTML_INLINE_IMAGES=ON;",
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="html",
           display_name="HTML5 (paginated)"),

FormatSpec(token="json",     kind=FormatKind.CANVAS,  canvas_type=CanvasType.JSON,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="json",
           display_name="JSON"),

FormatSpec(token="xml",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.XML,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="xml",
           display_name="XML"),

FormatSpec(token="pdf",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.PDF,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="pdf",
           display_name="PDF"),

FormatSpec(token="tiff",     kind=FormatKind.CANVAS,  canvas_type=CanvasType.TIF,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="tif",
           display_name="TIFF (multi-page)"),

FormatSpec(token="xps",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.XPS,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="xps",
           display_name="XPS"),

FormatSpec(token="ps",       kind=FormatKind.CANVAS,  canvas_type=CanvasType.PS,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="ps",
           display_name="PostScript"),

FormatSpec(token="dcx",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.DCX,
           is_single_file=True, supports_combine=True,  is_hi_def_capable=True,  extension="dcx",
           display_name="DCX (multi-page PCX)"),

# === canvas, one file per page =============================================

FormatSpec(token="png",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.PNG,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="png",
           display_name="PNG"),

FormatSpec(token="jpg",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.JPG,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="jpg",
           display_name="JPEG"),

FormatSpec(token="svg",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.SVG,
           canvas_options="SVG_INLINE_IMAGES=ON;",
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="svg",
           display_name="SVG"),

FormatSpec(token="gif",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.GIF,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="gif",
           display_name="GIF"),

FormatSpec(token="bmp",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.BMP,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="bmp",
           display_name="Windows bitmap"),

FormatSpec(token="eps",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.EPS,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="eps",
           display_name="Encapsulated PostScript"),

FormatSpec(token="webp",     kind=FormatKind.CANVAS,  canvas_type=CanvasType.WEBP,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="webp",
           display_name="WebP"),

FormatSpec(token="jpeg2000", kind=FormatKind.CANVAS,  canvas_type=CanvasType.JPEG2000,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="jp2",
           display_name="JPEG 2000"),

FormatSpec(token="pcx",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.PCX,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="pcx",
           display_name="PCX"),

FormatSpec(token="tga",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.TGA,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="tga",
           display_name="Targa"),

FormatSpec(token="brk",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.BRK,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="brk",
           display_name="Brooktrout fax"),

FormatSpec(token="pbm",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.PBM,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="pbm",
           display_name="Portable bitmap (monochrome)"),

FormatSpec(token="pgm",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.PGM,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="pgm",
           display_name="Portable greymap"),

FormatSpec(token="ppm",      kind=FormatKind.CANVAS,  canvas_type=CanvasType.PPM,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="ppm",
           display_name="Portable pixmap"),

FormatSpec(token="mdast",    kind=FormatKind.CANVAS, canvas_type=CanvasType.JSON,
           canvas_options="JSON_OUTPUT_SCHEMA=MDAST;",
           is_single_file=True,  supports_combine=True,  is_hi_def_capable=True,  extension="json",
           display_name="Markdown AST (JSON)"),

FormatSpec(token="web-safe", kind=FormatKind.CANVAS, canvas_type=CanvasType.WEB_SAFE,
           is_single_file=False, supports_combine=False, is_hi_def_capable=True,  extension="unknown",
           display_name="Web-safe image"),

# === the third path ========================================================

FormatSpec(token="docpack",  kind=FormatKind.DOC_PACK, canvas_type=None,
           is_single_file=True,  supports_combine=False, is_hi_def_capable=True,  extension="docpack",
           display_name="Document Pack"),

    ]

    # C# keys this dictionary with StringComparer.InvariantCultureIgnoreCase. Python
    # has no comparer hook on dict, so the key is folded on the way in and on the way
    # out instead — with the catalogue's own ASCII fold, not str.lower(), which is
    # Unicode-aware and would fold the Kelvin sign to k and turn a Turkish İ into two
    # characters. Every token below is already ASCII lower case, so folding them is
    # belt and braces; folding the lookup argument is what makes --to PDF resolve.
    _BY_TOKEN = {fold_token(fs.token): fs for fs in _ALL}

    @staticmethod
    def all() -> List[FormatSpec]:
        """Every format token this build accepts, in help-display order."""
        # A list, so the order is the order written above. Nothing here iterates
        # _BY_TOKEN: a dict has no defined order in C#, so any output derived from it
        # would differ between the ports.
        return FormatCatalog._ALL

    @staticmethod
    def resolve(token: str) -> FormatSpec:
        """
        Resolves a --to token.

        Raises:
            UsageError: The token is not in the catalogue. The message lists the valid
                tokens, so that a typo is self-correcting.
        """
        res = FormatCatalog.try_resolve(token)
        if res is not None:
            return res

        raise UsageError(ErrorCodes.UNKNOWN_FORMAT,
                         "unknown format '" + token
                         + "'; run 'docfilters formats' to list what this build can write")

    @staticmethod
    def try_resolve(token: str) -> Optional[FormatSpec]:
        """Resolves a token, returning None instead of raising when it is unknown."""
        return FormatCatalog._BY_TOKEN.get(fold_token(token))
