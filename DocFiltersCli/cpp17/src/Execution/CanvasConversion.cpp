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

#include "Execution/CanvasConversion.h"

#include "Planning/OutputPlanner.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				namespace
				{
					std::string trimChar(const std::string& text, char c)
					{
						std::size_t begin = 0;
						std::size_t end = text.size();
						while (begin < end && text[begin] == c)
							begin++;
						while (end > begin && text[end - 1] == c)
							end--;
						return text.substr(begin, end - begin);
					}
				}

				void CanvasConversion::convert(Engine::IDocument& document, const Planning::PlannedItem& item, const Planning::FormatSpec& format,
					const Configuration::ConvertOptions& options)
				{
					std::string canvasOptions = composeCanvasOptions(format, options.canvasOptions, options.noImages);

					if (item.target.has_value())
					{
						std::unique_ptr<Engine::IOutputCanvas> canvas = makeCanvas(*item.target, *format.canvasType, canvasOptions, options);
						appendTo(*canvas, document, item.pages, options, item.displayName);
					}
					else
					{
						for (int pageNumber : renderablePages(item.pages, document.getPageCount()))
						{
							std::string target = item.targetPattern->resolve(item.sourcePath, pageNumber);

							// Each page, not just the first. Planning can only check the first
							// target of a per-page item --- how many pages a document has is not
							// known until it is open --- so the rest are checked here, as they are
							// resolved. Without this, an existing page 2 was overwritten without
							// --overwrite whenever page 1 happened to be absent: exit 0, nothing
							// reported.
							verifyPageTarget(target, options);

							{
								std::unique_ptr<Engine::IOutputCanvas> canvas
									= makeCanvas(target, *format.canvasType, canvasOptions, options);
								renderPage(*canvas, document, pageNumber, options, item.displayName);
							}

							// After the canvas is closed, so the bytes are on disk to be identified.
							nameByRenderedType(target, format, options);
						}
					}
				}

				void CanvasConversion::convertToStream(Engine::IDocument& document, std::ostream& destination, const Planning::PlannedItem& item,
					const Planning::FormatSpec& format, const Configuration::ConvertOptions& options)
				{
					std::string canvasOptions = composeCanvasOptions(format, options.canvasOptions, options.noImages);

					std::ostringstream buffer;
					{
						std::unique_ptr<Engine::IOutputCanvas> canvas = _session.api().makeCanvas(buffer, *format.canvasType, canvasOptions);
						appendTo(*canvas, document, item.pages, options, item.displayName);
					}

					std::string bytes = buffer.str();
					destination.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
					destination.flush();
				}

				std::string CanvasConversion::composeCanvasOptions(
					const Planning::FormatSpec& format, const std::optional<std::string>& userOptions, bool noImages)
				{
					std::string user = trimChar(userOptions.value_or(std::string()), ';');
					std::string own = trimChar(format.canvasOptions, ';');

					std::string suppressed = noImages ? imageSuppression(format) : std::string();
					if (!suppressed.empty())
						own = own.empty() ? suppressed : own + ";" + suppressed;

					if (user.empty())
						return own;
					if (own.empty())
						return user;

					return user + ";" + own;
				}

				std::string CanvasConversion::imageSuppression(const Planning::FormatSpec& format)
				{
					return (format.token == "md" || format.token == "md-text") ? "MARKDOWN_INCLUDE_IMAGES=OFF" : std::string();
				}

				void CanvasConversion::appendTo(Engine::IOutputCanvas& canvas, Engine::IDocument& document, const Planning::PageRange& pages,
					const Configuration::ConvertOptions& options, const std::string& displayName)
				{
					if (pages.isAll() && !hasPageDecorations(options) && !options.fingerprint)
					{
						canvas.renderPages(document);
					}
					else
					{
						for (int pageNumber : renderablePages(pages, document.getPageCount()))
							renderPage(canvas, document, pageNumber, options, displayName);
					}
				}

				std::vector<int> CanvasConversion::renderablePages(const Planning::PageRange& pages, int pageCount)
				{
					std::vector<int> result;
					for (int pageNumber : pages.enumerate(pageCount))
					{
						// Stop, not skip: the pages arrive ascending, so the first one past the
						// end means every one after it is too, and skipping would still walk all
						// two billion of --pages 1-2147483647 to find that out.
						if (pageNumber > pageCount)
							break;
						result.push_back(pageNumber);
					}
					return result;
				}

				namespace
				{
					/// <summary>The extension for an image the engine identified, or empty if unknown.</summary>
					std::string extensionForMimeType(const std::string& mimeType)
					{
						if (mimeType == "image/png")
							return "png";
						if (mimeType == "image/gif")
							return "gif";
						if (mimeType == "image/jpeg")
							return "jpg";
						if (mimeType == "image/webp")
							return "webp";
						return std::string();
					}
				}

				/// <summary>
				/// Renames a <c>web-safe</c> page to match whatever the engine actually wrote.
				/// </summary>
				/// <remarks>
				/// <c>web-safe</c> is the one format whose extension cannot be planned: the
				/// engine chooses PNG, GIF, JPEG or WebP per page, from the page's own content
				/// --- one document here renders page 1 as JPEG, pages 2 and 3 as PNG and page
				/// 4 as GIF. The planner therefore names the file <c>.unknown</c>, and this
				/// puts the real name on it afterwards. Until now nothing did, so every page
				/// was left as <c>.unknown</c> and the format's whole point produced files
				/// nothing would open by extension.
				///
				/// The type comes from the engine identifying the file it just wrote, which is
				/// what doc2text does. Mapped by MIME type rather than by the numeric format id
				/// doc2text switches on: the ids are internal, the MIME strings are public API.
				///
				/// The destination goes through the same output policy as any other target.
				/// doc2text instead deletes whatever is in the way, which is exactly the
				/// overwrite bypass this round of review was about.
				/// </remarks>
				void CanvasConversion::nameByRenderedType(
					const std::string& target, const Planning::FormatSpec& format, const Configuration::ConvertOptions& options)
				{
					if (format.token != "web-safe" || options.fingerprint || _fileSystem == nullptr)
						return;

					std::string extension = extensionForMimeType(_session.api().identifyFile(target).mimeType);
					if (extension.empty())
						return;

					std::size_t dot = target.find_last_of('.');
					std::string renamed = (dot == std::string::npos ? target : target.substr(0, dot)) + "." + extension;
					if (renamed == target)
						return;

					verifyPageTarget(renamed, options);
					_fileSystem->moveFile(target, renamed);
				}

				/// <remarks>
				/// Skipped under <c>--fingerprint</c>, which renders to a null stream and
				/// writes no file at all, so an existing path of that name is not in anyone's
				/// way --- the same exemption <c>ConvertCommand</c> makes for
				/// <c>verifyTargets</c>.
				/// </remarks>
				void CanvasConversion::verifyPageTarget(const std::string& target, const Configuration::ConvertOptions& options)
				{
					if (options.fingerprint || _shared == nullptr || _fileSystem == nullptr)
						return;

					Planning::OutputPlanner::verifyPageTarget(target, *_shared, *_fileSystem);
				}

				std::unique_ptr<Engine::IOutputCanvas> CanvasConversion::makeCanvas(const std::string& target,
					Hyland::DocFilters::CanvasType canvasType, const std::string& canvasOptions, const Configuration::ConvertOptions& options)
				{
					// The directory is already there: ConvertCommand creates it once per
					// item, because the engine opens this file itself and will not
					// create one.
					return options.fingerprint ? _session.api().makeCanvas(_nullStream, canvasType, canvasOptions)
												: _session.api().makeCanvas(target, canvasType, canvasOptions);
				}

				void CanvasConversion::renderPage(Engine::IOutputCanvas& canvas, Engine::IDocument& document, int pageNumber,
					const Configuration::ConvertOptions& options, const std::string& displayName)
				{
					std::unique_ptr<Engine::IDocumentPage> page = document.getPage(pageNumber - 1);

					std::string renderOpts = options.renderOptions.value_or(std::string());
					if (options.watermark.has_value() && !options.watermark->empty())
					{
						std::string normalisedWatermark = *options.watermark;
						for (char& ch : normalisedWatermark)
						{
							if (ch == '\\')
								ch = '/';
						}
						if (!std::ifstream(Runtime::FilePath::of(normalisedWatermark)).good())
							renderOpts += (renderOpts.empty() ? "" : ";") + std::string("WATERMARK=") + *options.watermark;
					}

					canvas.renderPage(*page, renderOpts, applyPageDecorations(pageNumber, options));
					drawWatermark(canvas, *page, options);

					if (options.fingerprint)
						writeFingerprint(*page, pageNumber, displayName);
				}

				void CanvasConversion::writeFingerprint(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName)
				{
					std::optional<std::string> line = formatFingerprint(
						displayName, pageNumber, page.getAttribute("IMAGE_FINGERPRINT_V2"), page.getAttribute("IMAGE_FINGERPRINT"));
					if (line.has_value())
						_console.writeLine(*line);
				}

				std::optional<std::string> CanvasConversion::formatFingerprint(
					const std::string& displayName, int pageNumber, const std::string& fingerprintV2, const std::string& fingerprintV1)
				{
					const std::string& fingerprint = fingerprintV2.empty() ? fingerprintV1 : fingerprintV2;
					if (fingerprint.empty())
						return std::nullopt;

					return displayName + ": page=" + std::to_string(pageNumber) + " fingerprint=" + fingerprint;
				}

				bool CanvasConversion::hasPageDecorations(const Configuration::ConvertOptions& options)
				{
					return (options.watermark.has_value() && !options.watermark->empty()) || !options.redactions.empty()
						|| !options.formValues.empty();
				}

				std::optional<Engine::EnginePageDecorations> CanvasConversion::applyPageDecorations(
					int pageNumber, const Configuration::ConvertOptions& options)
				{
					Engine::EnginePageDecorations decorations;

					for (const Configuration::RedactSpec& redaction : options.redactions)
					{
						if (redaction.page != pageNumber)
							continue;

						Engine::EngineRedaction converted;
						converted.x = redaction.x;
						converted.y = redaction.y;
						converted.width = redaction.width;
						converted.height = redaction.height;
						decorations.redactions.push_back(converted);
					}

					for (const Configuration::FormValue& form : options.formValues)
					{
						if (form.page != pageNumber)
							continue;

						Engine::EngineFormValue converted;
						converted.name = form.name;
						converted.value = form.value;
						decorations.formValues.push_back(converted);
					}

					return decorations.isEmpty() ? std::nullopt : std::optional<Engine::EnginePageDecorations>(std::move(decorations));
				}

				void CanvasConversion::drawWatermark(Engine::IOutputCanvas& canvas, Engine::IDocumentPage& page, const Configuration::ConvertOptions& options)
				{
					if (!options.watermark.has_value() || options.watermark->empty())
						return;

					std::string path = *options.watermark;
					for (char& ch : path)
					{
						if (ch == '\\')
							ch = '/';
					}

					std::ifstream file(Runtime::FilePath::of(path), std::ios::binary);
					if (!file.good())
						return;

					std::vector<std::uint8_t> bytes(
						(std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
					canvas.drawImage(0, 0, bytes, std::string());
				}
			}
		}
	}
}
