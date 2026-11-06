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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_STRUCTUREREPORT_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_STRUCTUREREPORT_H

#include "Configuration/CommandOptions.h"
#include "Engine/IDocumentFiltersEngine.h"
#include "Runtime/IConsole.h"

#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>
				/// The document and page structure aspects <c>convert</c> reports while
				/// it works: fonts, dimensions, page elements, form elements,
				/// hyperlinks, bookmarks and annotations.
				/// </summary>
				/// <remarks>
				/// These are emitted during a conversion run rather than by a separate
				/// command, matters most for high-definition output, where the structure
				/// is only known once the page has been laid out.
				///
				/// Output is minimal and line-oriented, one fact per line, and is
				/// compared as golden files by the conformance suite.
				/// </remarks>
				class StructureReport
				{
				public:
					StructureReport(const Configuration::StructureReportOptions& options, Runtime::IConsole& console)
						: _options(options), _console(console)
					{
					}

					/// <summary>Reports the document-level aspects: fonts, font mappings, bookmarks, annotations.</summary>
					void reportDocument(Engine::IDocument& document, const std::string& displayName);

					/// <summary>Reports the page-level aspects for one page.</summary>
					void reportPage(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName);

					/// <remarks>
					/// Fonts are not a first-class collection anywhere in the binding;
					/// doc2text reads them the same way, as the semicolon-separated
					/// <c>DOCUMENT_FONTS</c> page attribute, queried on the last page once
					/// every page has been laid out.
					/// </remarks>
					void writeFonts(Engine::IDocument& document, const std::string& displayName);

					/// <remarks>Same source as <see cref="writeFonts"/>, via <c>DOCUMENT_FONT_MAPPINGS</c>.</remarks>
					void writeFontMappings(Engine::IDocument& document, const std::string& displayName);

					/// <remarks>
					/// Guarded on the page count for the same reason
					/// <c>getLastPageAttribute</c> is: a bookmark's page number only
					/// exists once the document has been laid out into pages, and a
					/// handle opened without <c>IGR_FORMAT_IMAGE</c> --- <c>--to text
					/// --bookmarks</c>, where the text reader needs the format bit
					/// bookmarks would otherwise take --- was never laid out at all.
					/// This is the same "reports nothing rather than failing" rule.
					/// </remarks>
					void writeBookmarks(Engine::IDocument& document, const std::string& displayName);

					/// <remarks>
					/// A document-level report even though every annotation belongs to a
					/// page: numbering runs across the whole document, so it has to be
					/// gathered in one pass rather than reset per <see cref="reportPage"/>
					/// call.
					/// </remarks>
					void writeAnnotations(Engine::IDocument& document, const std::string& displayName);

					void writeDimensions(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName);

					void writePageFonts(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName);

					/// <summary><c>--page-font-mappings</c>: how each page's fonts were substituted.</summary>
					void writePageFontMappings(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName);

					void writePageElements(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName);

					void writeFormElements(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName);

					void writeHyperlinks(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName);

				private:
					void writeRecord(const std::string& displayName, const std::string& fields);
					void writeFontList(const std::string& attributeValue, const std::string& displayName, std::optional<int> pageNumber);

					/// <remarks>Each entry is <c>original=mapped</c>, per the attribute text's own construction.</remarks>
					void writeFontMappingList(const std::string& attributeValue, const std::string& displayName, std::optional<int> pageNumber);

					const Configuration::StructureReportOptions& _options;
					Runtime::IConsole& _console;
				};
			}
		}
	}
}

#endif
