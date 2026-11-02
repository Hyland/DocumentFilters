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

#include "FormatCatalog.h"

#include "Runtime/CliError.h"
#include "Runtime/Strings.h"

#include <unordered_map>

namespace DF = Hyland::DocFilters;

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Planning
			{
				namespace FormatCatalog
				{
					namespace
					{
						// Field order for every row below: token, kind, canvasType,
						// canvasOptions, isSingleFile, supportsCombine, isHiDefCapable,
						// extension, displayName. C++17 has no designated initialisers, so
						// this comment is the header row a reader would otherwise get from
						// FormatCatalog.cs's named object initialisers.
						const std::vector<FormatSpec> catalog = {

							// === text mode: straight from the extractor ========================

							{ "text", FormatKind::Text, std::nullopt, "",
							  true, true, false, "txt", "Plain text" },

							{ "md-text", FormatKind::Text, std::nullopt, "",
							  true, true, false, "md", "Markdown (text mode)" },

							{ "html", FormatKind::Text, std::nullopt, "",
							  true, true, false, "html", "HTML (flow)" },

							// === canvas, single file ============================================

							{ "md", FormatKind::Canvas, DF::CanvasType::MARKDOWN, "",
							  true, true, true, "md", "Markdown (high definition)" },

							{ "html5", FormatKind::Canvas, DF::CanvasType::HTML, "HTML_INLINE_IMAGES=ON;",
							  true, true, true, "html", "HTML5 (paginated)" },

							{ "json", FormatKind::Canvas, DF::CanvasType::JSON, "",
							  true, true, true, "json", "JSON" },

							{ "mdast", FormatKind::Canvas, DF::CanvasType::JSON, "JSON_OUTPUT_SCHEMA=MDAST;",
							  true, true, true, "json", "Markdown AST (JSON)" },

							{ "xml", FormatKind::Canvas, DF::CanvasType::XML, "",
							  true, true, true, "xml", "XML" },

							{ "pdf", FormatKind::Canvas, DF::CanvasType::PDF, "",
							  true, true, true, "pdf", "PDF" },

							{ "tiff", FormatKind::Canvas, DF::CanvasType::TIF, "",
							  true, true, true, "tif", "TIFF (multi-page)" },

							{ "xps", FormatKind::Canvas, DF::CanvasType::XPS, "",
							  true, true, true, "xps", "XPS" },

							{ "ps", FormatKind::Canvas, DF::CanvasType::PS, "",
							  true, true, true, "ps", "PostScript" },

							{ "dcx", FormatKind::Canvas, DF::CanvasType::DCX, "",
							  true, true, true, "dcx", "DCX (multi-page PCX)" },

							// === canvas, one file per page =======================================

							{ "png", FormatKind::Canvas, DF::CanvasType::PNG, "",
							  false, false, true, "png", "PNG" },

							{ "jpg", FormatKind::Canvas, DF::CanvasType::JPG, "",
							  false, false, true, "jpg", "JPEG" },

							{ "svg", FormatKind::Canvas, DF::CanvasType::SVG, "SVG_INLINE_IMAGES=ON;",
							  false, false, true, "svg", "SVG" },

							{ "gif", FormatKind::Canvas, DF::CanvasType::GIF, "",
							  false, false, true, "gif", "GIF" },

							{ "bmp", FormatKind::Canvas, DF::CanvasType::BMP, "",
							  false, false, true, "bmp", "Windows bitmap" },

							{ "eps", FormatKind::Canvas, DF::CanvasType::EPS, "",
							  false, false, true, "eps", "Encapsulated PostScript" },

							{ "webp", FormatKind::Canvas, DF::CanvasType::WEBP, "",
							  false, false, true, "webp", "WebP" },

							{ "jpeg2000", FormatKind::Canvas, DF::CanvasType::JPEG2000, "",
							  false, false, true, "jp2", "JPEG 2000" },

							{ "pcx", FormatKind::Canvas, DF::CanvasType::PCX, "",
							  false, false, true, "pcx", "PCX" },

							{ "tga", FormatKind::Canvas, DF::CanvasType::TGA, "",
							  false, false, true, "tga", "Targa" },

							// Brooktrout fax. One file per page despite fax formats usually
							// being multi-page: the engine encodes it per page rather than
							// through one of the accumulating page containers pdf, tiff, xps
							// and dcx use.
							{ "brk", FormatKind::Canvas, DF::CanvasType::BRK, "",
							  false, false, true, "brk", "Brooktrout fax" },

							// The netpbm trio, distinguished only by bit depth.
							{ "pbm", FormatKind::Canvas, DF::CanvasType::PBM, "",
							  false, false, true, "pbm", "Portable bitmap (monochrome)" },

							{ "pgm", FormatKind::Canvas, DF::CanvasType::PGM, "",
							  false, false, true, "pgm", "Portable greymap" },

							{ "ppm", FormatKind::Canvas, DF::CanvasType::PPM, "",
							  false, false, true, "ppm", "Portable pixmap" },

							// The one format whose extension the engine picks: png, gif, jpg
							// or webp depending on the bit depth it settles on, which is not
							// knowable until the page has been rendered. Execution renames the
							// file once the real type is detected.
							{ "web-safe", FormatKind::Canvas, DF::CanvasType::WEBSAFE, "",
							  false, false, true, "unknown", "Web-safe image" },

							// === the third path ===================================================

							{ "docpack", FormatKind::DocPack, std::nullopt, "",
							  true, false, true, "docpack", "Document Pack" },
						};

						const std::unordered_map<std::string, const FormatSpec*>& byToken()
						{
							static const std::unordered_map<std::string, const FormatSpec*> map = []
							{
								std::unordered_map<std::string, const FormatSpec*> result;
								for (const FormatSpec& spec : catalog)
									result[spec.token] = &spec;
								return result;
							}();
							return map;
						}
					}

					const std::vector<FormatSpec>& all() { return catalog; }

					const FormatSpec* tryResolve(const std::string& token)
					{
						const std::unordered_map<std::string, const FormatSpec*>& map = byToken();
						auto found = map.find(Runtime::Strings::foldAscii(token));
						return found == map.end() ? nullptr : found->second;
					}

					const FormatSpec& resolve(const std::string& token)
					{
						const FormatSpec* found = tryResolve(token);
						if (found != nullptr)
							return *found;

						throw Runtime::UsageError(Runtime::ErrorCodes::unknownFormat,
							"unknown format '" + token + "'; run 'docfilters formats' to list what this build can write");
					}
				}
			}
		}
	}
}
