/*
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
 */

using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Planning
{
	/// <summary>
	/// The <c>--to</c> token table: what each format name means, and the two static
	/// properties the output planner needs from it.
	/// </summary>
	/// <remarks>
	/// This table is the single source of truth for the format surface in all four
	/// language implementations, and several entries are not a straight mapping onto
	/// a single engine enum value:
	/// <list type="bullet">
	/// <item><c>md</c> is high-definition markdown via a canvas; <c>md-text</c> is
	/// text-mode markdown straight from the extractor.</item>
	/// <item><c>html</c> is flow HTML from the extractor; <c>html5</c> is paginated
	/// high-definition HTML via a canvas.</item>
	/// <item><c>docpack</c> is neither — it goes through
	/// <c>Extractor.MakeDocPack</c>.</item>
	/// <item><c>mdast</c> has no device of its own: it is the JSON canvas under a
	/// different schema, told apart only by the <c>JSON_OUTPUT_SCHEMA=MDAST;</c> it
	/// contributes through <see cref="FormatSpec.CanvasOptions"/> (D31). Confirmed
	/// against doc2text, which spells it the same way.</item>
	/// </list>
	/// </remarks>
	public static class FormatCatalog
	{
		private static readonly IReadOnlyList<FormatSpec> _all = new List<FormatSpec>
		{
// === text mode: straight from the extractor ================================

new FormatSpec { Token = "text",     Kind = FormatKind.Text,    CanvasType = null,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = false, Extension = "txt",
                 DisplayName = "Plain text" },

new FormatSpec { Token = "md-text",  Kind = FormatKind.Text,    CanvasType = null,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = false, Extension = "md",
                 DisplayName = "Markdown (text mode)" },

new FormatSpec { Token = "html",     Kind = FormatKind.Text,    CanvasType = null,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = false, Extension = "html",
                 DisplayName = "HTML (flow)" },

// === canvas, single file ===================================================

new FormatSpec { Token = "md",       Kind = FormatKind.Canvas,  CanvasType = CanvasType.MARKDOWN,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "md",
                 DisplayName = "Markdown (high definition)" },

new FormatSpec { Token = "html5",    Kind = FormatKind.Canvas,  CanvasType = CanvasType.HTML,
                 CanvasOptions = "HTML_INLINE_IMAGES=ON;",
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "html",
                 DisplayName = "HTML5 (paginated)" },

new FormatSpec { Token = "json",     Kind = FormatKind.Canvas,  CanvasType = CanvasType.JSON,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "json",
                 DisplayName = "JSON" },

new FormatSpec { Token = "xml",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.XML,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "xml",
                 DisplayName = "XML" },

new FormatSpec { Token = "pdf",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.PDF,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "pdf",
                 DisplayName = "PDF" },

new FormatSpec { Token = "tiff",     Kind = FormatKind.Canvas,  CanvasType = CanvasType.TIF,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "tif",
                 DisplayName = "TIFF (multi-page)" },

new FormatSpec { Token = "xps",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.XPS,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "xps",
                 DisplayName = "XPS" },

new FormatSpec { Token = "ps",       Kind = FormatKind.Canvas,  CanvasType = CanvasType.PS,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "ps",
                 DisplayName = "PostScript" },

new FormatSpec { Token = "dcx",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.DCX,
                 IsSingleFile = true, SupportsCombine = true,  IsHiDefCapable = true,  Extension = "dcx",
                 DisplayName = "DCX (multi-page PCX)" },

// === canvas, one file per page =============================================

new FormatSpec { Token = "png",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.PNG,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "png",
                 DisplayName = "PNG" },

new FormatSpec { Token = "jpg",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.JPG,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "jpg",
                 DisplayName = "JPEG" },

new FormatSpec { Token = "svg",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.SVG,
                 CanvasOptions = "SVG_INLINE_IMAGES=ON;",
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "svg",
                 DisplayName = "SVG" },

new FormatSpec { Token = "gif",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.GIF,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "gif",
                 DisplayName = "GIF" },

new FormatSpec { Token = "bmp",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.BMP,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "bmp",
                 DisplayName = "Windows bitmap" },

new FormatSpec { Token = "eps",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.EPS,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "eps",
                 DisplayName = "Encapsulated PostScript" },

new FormatSpec { Token = "webp",     Kind = FormatKind.Canvas,  CanvasType = CanvasType.WEBP,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "webp",
                 DisplayName = "WebP" },

new FormatSpec { Token = "jpeg2000", Kind = FormatKind.Canvas,  CanvasType = CanvasType.JPEG2000,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "jp2",
                 DisplayName = "JPEG 2000" },

new FormatSpec { Token = "pcx",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.PCX,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "pcx",
                 DisplayName = "PCX" },

new FormatSpec { Token = "tga",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.TGA,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "tga",
                 DisplayName = "Targa" },

new FormatSpec { Token = "brk",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.BRK,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "brk",
                 DisplayName = "Brooktrout fax" },

new FormatSpec { Token = "pbm",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.PBM,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "pbm",
                 DisplayName = "Portable bitmap (monochrome)" },

new FormatSpec { Token = "pgm",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.PGM,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "pgm",
                 DisplayName = "Portable greymap" },

new FormatSpec { Token = "ppm",      Kind = FormatKind.Canvas,  CanvasType = CanvasType.PPM,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "ppm",
                 DisplayName = "Portable pixmap" },

new FormatSpec { Token = "mdast",    Kind = FormatKind.Canvas, CanvasType = CanvasType.JSON,
                 CanvasOptions = "JSON_OUTPUT_SCHEMA=MDAST;",
                 IsSingleFile = true,  SupportsCombine = true,  IsHiDefCapable = true,  Extension = "json",
                 DisplayName = "Markdown AST (JSON)" },

new FormatSpec { Token = "web-safe", Kind = FormatKind.Canvas, CanvasType = CanvasType.WebSafe,
                 IsSingleFile = false, SupportsCombine = false, IsHiDefCapable = true,  Extension = "unknown",
                 DisplayName = "Web-safe image" },

// === the third path ========================================================

new FormatSpec { Token = "docpack",  Kind = FormatKind.DocPack, CanvasType = null,
                 IsSingleFile = true,  SupportsCombine = false, IsHiDefCapable = true,  Extension = "docpack",
                 DisplayName = "Document Pack" },

        };

        private static readonly IReadOnlyDictionary<string, FormatSpec> _byToken =
            _all.ToDictionary(fs => fs.Token, StringComparer.InvariantCultureIgnoreCase);

        /// <summary>Every format token this build accepts, in help-display order.</summary>
        public static IReadOnlyList<FormatSpec> All()
		{
			return _all;
		}

		/// <summary>Resolves a <c>--to</c> token.</summary>
		/// <exception cref="Runtime.UsageError">
		/// The token is not in the catalogue. The message lists the valid tokens, so
		/// that a typo is self-correcting.
		/// </exception>
		public static FormatSpec Resolve(string token)
		{
            var res = TryResolve(token);
            if (res != null)
                return res;

            throw new UsageError(ErrorCodes.UnknownFormat, 
                $"unknown format '{token}'; run 'docfilters formats' to list what this build can write");
		}

		/// <summary>Resolves a token, returning null instead of throwing when it is unknown.</summary>
		public static FormatSpec TryResolve(string token)
        {
            if (_byToken.TryGetValue(token, out var formatSpec))
                return formatSpec;
            return null;
        }
	}
}
