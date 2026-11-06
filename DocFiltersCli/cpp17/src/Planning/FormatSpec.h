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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_FORMATSPEC_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_FORMATSPEC_H

// FormatSpec::canvasType names the binding's own CanvasType directly, the one
// deliberate exception to Engine/ being the only folder that names a Document
// Filters type: an enum is public and constructible, so a fake gains nothing
// by wrapping it, and FormatCatalog's whole job is mapping a --to token onto
// one of these values.
#include <DocumentFiltersObjects.h>

#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Planning
			{
				/// <summary>How a target format is produced.</summary>
				enum class FormatKind
				{
					/// <summary>Streamed from the extractor with getText or SaveTo; no canvas involved.</summary>
					Text,

					/// <summary>Rendered page by page onto a canvas from MakeOutputCanvas.</summary>
					Canvas,

					/// <summary>Produced by Extractor::MakeDocPack, which is neither of the above.</summary>
					DocPack
				};

				/// <summary>
				/// Everything the planner needs to know about one <c>--to</c> value.
				/// </summary>
				/// <remarks>
				/// The two fields that drive the output model are <c>isSingleFile</c> and
				/// <c>isHiDefCapable</c>. Both are static properties of the format, so
				/// <c>-o</c> never has to guess and never depends on the document.
				/// </remarks>
				struct FormatSpec
				{
					/// <summary>The token the user writes, e.g. "pdf". Always lower case.</summary>
					std::string token;

					/// <summary>Which engine path renders it.</summary>
					FormatKind kind;

					/// <summary>
					/// The canvas device, when <c>kind</c> is <see cref="FormatKind::Canvas"/>.
					/// Absent for text and docpack, which use no canvas --- not zero, which is
					/// a real device (PNG).
					/// </summary>
					std::optional<Hyland::DocFilters::CanvasType> canvasType;

					/// <summary>
					/// Canvas options the format itself contributes, appended after whatever
					/// <c>--canvas-options</c> supplies. Empty for most.
					/// </summary>
					/// <remarks>
					/// These are defaults, not identity. The engine resolves a repeated key
					/// to its first occurrence, so appending them leaves <c>--canvas-options</c>
					/// in front and the user wins every key they name.
					///
					/// <c>JSON_OUTPUT_SCHEMA=MDAST;</c> is what tells <c>mdast</c> apart from
					/// <c>json</c>: both are the JSON canvas, and an implementation that drops
					/// it writes plain JSON and looks correct.
					///
					/// <c>HTML_INLINE_IMAGES</c> and <c>SVG_INLINE_IMAGES</c> both default to
					/// off in the engine, which for <c>html5</c> and <c>svg</c> is never what
					/// this tool wants: those canvases rasterise each page into a sibling
					/// image and lay a transparent text layer over it, but only the markup is
					/// written to <c>--out-dir</c>, so the sibling never lands and every page
					/// renders blank. Turning them on inlines the raster as a <c>data:</c>
					/// URI, which also gives <c>--out-file</c> and <c>--stdout</c> the single
					/// self-contained file they promise.
					/// </remarks>
					std::string canvasOptions;

					/// <summary>
					/// True when the format writes exactly one file however many pages the
					/// document has (pdf, tiff, text). False when it writes one file per
					/// page (png, jpg, svg), which is what makes <c>--out-file</c> and
					/// <c>--stdout</c> illegal for it.
					/// </summary>
					bool isSingleFile;

					/// <summary>
					/// True when several inputs may merge into one output under
					/// <c>--combine</c>.
					/// </summary>
					/// <remarks>
					/// A separate question from <c>isSingleFile</c>, and the two only look
					/// alike. <c>docpack</c> writes one file per document, like pdf, but
					/// <c>Extractor::MakeDocPack</c> takes one extractor, so a pack cannot
					/// hold two documents.
					/// </remarks>
					bool supportsCombine;

					/// <summary>True when the format can carry high-definition, paginated output.</summary>
					bool isHiDefCapable;

					/// <summary>The filename extension, without a dot, used by <c>{ext}</c> in an output template.</summary>
					/// <remarks>
					/// <c>web-safe</c> is the one entry whose extension is a placeholder,
					/// <c>unknown</c>: the engine picks png, gif, jpg or webp from the bit
					/// depth it settles on, which is not knowable until the page has been
					/// rendered, so execution renames the file once the real type is
					/// detected.
					/// </remarks>
					std::string extension;

					/// <summary>Human-readable name for help and error text.</summary>
					std::string displayName;

					/// <summary>The canonical text form, used by the conformance corpus.</summary>
					std::string toCanonicalString() const;
				};
			}
		}
	}
}

#endif
