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

namespace Hyland.DocumentFilters.Cli.Planning
{
	/// <summary>How a target format is produced.</summary>
	public enum FormatKind
	{
		/// <summary>Streamed from the extractor with <c>GetText</c> or <c>SaveTo</c>; no canvas involved.</summary>
		Text,

		/// <summary>Rendered page by page onto a canvas from <c>MakeOutputCanvas</c>.</summary>
		Canvas,

		/// <summary>Produced by <c>Extractor.MakeDocPack</c>, which is neither of the above.</summary>
		DocPack
	}

	/// <summary>
	/// Everything the planner needs to know about one <c>--to</c> value.
	/// </summary>
	/// <remarks>
	/// The two properties that drive the output model are <see cref="IsSingleFile"/>
	/// and <see cref="IsHiDefCapable"/>. Both are static properties of the format, so
	/// <c>-o</c> never has to guess and never depends on the document.
	/// </remarks>
	public sealed class FormatSpec
	{
		/// <summary>The token the user writes, e.g. <c>pdf</c>. Always lower case.</summary>
		public string Token { get; set; }

		/// <summary>Which engine path renders it.</summary>
		public FormatKind Kind { get; set; }

		/// <summary>
		/// The canvas device, when <see cref="Kind"/> is
		/// <see cref="FormatKind.Canvas"/>. Null for text and docpack, which use no
		/// canvas — not zero, which is a real device (<c>PNG</c>).
		/// </summary>
		public CanvasType? CanvasType { get; set; }

		/// <summary>
		/// Canvas options the format itself contributes, appended after whatever
		/// <c>--canvas-options</c> supplies. Empty for most.
		/// </summary>
		/// <remarks>
		/// These are defaults, not identity. The engine resolves a repeated key to its
		/// first occurrence, so appending them leaves <c>--canvas-options</c> in front
		/// and the user wins every key they name — which is why there is one property
		/// here and not a second one for overridable settings.
		///
		/// <c>JSON_OUTPUT_SCHEMA=MDAST;</c> is what tells <c>mdast</c> apart from
		/// <c>json</c>: both are the JSON canvas, and an implementation that drops it
		/// writes plain JSON and looks correct.
		///
		/// <c>HTML_INLINE_IMAGES</c> and <c>SVG_INLINE_IMAGES</c> both default to off
		/// in the engine, which for <c>html5</c> and <c>svg</c> is never what this tool
		/// wants: those canvases rasterise each page into a sibling image and lay a
		/// transparent text layer over it, but only the markup is written to
		/// <c>--out-dir</c>, so the sibling never lands and every page renders blank.
		/// Turning them on inlines the raster as a <c>data:</c> URI, which also gives
		/// <c>--out-file</c> and <c>--stdout</c> the single self-contained file they
		/// promise. doc2text forces the same two, in <c>main.cpp</c>, for the same
		/// reason.
		/// </remarks>
		public string CanvasOptions { get; set; } = string.Empty;

		/// <summary>
		/// True when the format writes exactly one file however many pages the
		/// document has (pdf, tiff, text). False when it writes one file per page
		/// (png, jpg, svg), which is what makes <c>--out-file</c> and <c>--stdout</c>
		/// illegal for it.
		/// </summary>
		public bool IsSingleFile { get; set; }

		/// <summary>
		/// True when several inputs may merge into one output under <c>--combine</c>.
		/// </summary>
		/// <remarks>
		/// A separate question from <see cref="IsSingleFile"/>, and the two only look
		/// alike. <c>docpack</c> writes one file per document, like pdf, but
		/// <c>Extractor.MakeDocPack</c> takes one extractor, so a pack cannot hold two
		/// documents. Deriving <c>--combine</c> from <see cref="IsSingleFile"/> lets
		/// <c>--to docpack --combine</c> pass validation and fail at execution.
		/// </remarks>
		public bool SupportsCombine { get; set; }

		/// <summary>True when the format can carry high-definition, paginated output.</summary>
		public bool IsHiDefCapable { get; set; }

		/// <summary>The filename extension, without a dot, used by <c>{ext}</c> in an output template.</summary>
		/// <remarks>
		/// <c>web-safe</c> is the one entry whose extension is a placeholder,
		/// <c>unknown</c>: the engine picks png, gif, jpg or webp from the bit depth it
		/// settles on, which is not knowable until the page has been rendered, so
		/// execution renames the file once the real type is detected. That rename is an
		/// execution concern and needs nothing here — the renamed path goes through
		/// <c>OutputPlanner.VerifyPageTarget</c> like every other per-page output.
        /// </remarks>
		public string Extension { get; set; }

		/// <summary>Human-readable name for help and error text.</summary>
		public string DisplayName { get; set; }

		/// <summary>The canonical text form, used by the conformance corpus.</summary>
		public override string ToString()
		{
			return $"{Token} {Kind} {(IsSingleFile ? "single-file" : "per-page")}{(IsHiDefCapable ? " hidef" : "")}".ToLowerInvariant();
		}
	}
}
