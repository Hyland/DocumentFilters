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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_CANVASCONVERSION_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_CANVASCONVERSION_H

#include "Configuration/CommandOptions.h"
#include "Execution/DocumentFiltersSession.h"
#include "Planning/FormatSpec.h"
#include "Planning/PageRange.h"
#include "Planning/RunPlan.h"
#include "Runtime/IConsole.h"
#include "Runtime/IFileSystem.h"

#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				namespace Detail
				{
					/// <summary>
					/// A streambuf that accepts and discards everything written to it ---
					/// the equivalent of C#'s <c>Stream.Null</c>, which
					/// <c>--fingerprint</c> renders onto instead of a real target since
					/// the render has to happen but its bytes never matter.
					/// </summary>
					class DiscardingStreambuf : public std::streambuf
					{
					protected:
						std::streamsize xsputn(const char*, std::streamsize n) override { return n; }
						int_type overflow(int_type c) override { return c; }
					};
				}

				/// <summary>
				/// The high-definition path: open a canvas, walk the pages, render each
				/// one. Covers PDF, TIFF, the image formats, HTML5, structured XML, JSON
				/// and high-definition markdown.
				/// </summary>
				class CanvasConversion
				{
				public:
					/// <summary>
					/// Takes the session because canvases are made by
					/// <c>DocumentFilters::MakeOutputCanvas</c> --- an <c>IDocument</c>
					/// cannot produce one, so a document alone is not enough to render
					/// anything.
					/// </summary>
					/// <param name="shared">
					/// The output policy, for checking each per-page target as it is resolved.
					/// Null in tests that render nowhere and verify nothing.
					/// </param>
					/// <param name="fileSystem">Null for the same reason.</param>
					CanvasConversion(DocumentFiltersSession& session, Runtime::IConsole& console,
						const Configuration::SharedOptions* shared = nullptr, Runtime::IFileSystem* fileSystem = nullptr)
						: _session(session), _console(console), _shared(shared), _fileSystem(fileSystem), _nullStream(&_nullBuffer)
					{
					}

					/// <summary>
					/// Renders one document to its own output. Used for both single-file
					/// formats (one canvas, many pages) and one-per-page formats (one
					/// canvas per page, named from <c>PlannedItem::targetPattern</c> as
					/// the pages are found).
					/// </summary>
					void convert(Engine::IDocument& document, const Planning::PlannedItem& item, const Planning::FormatSpec& format,
						const Configuration::ConvertOptions& options);

					/// <summary>
					/// Renders one document straight to <paramref name="destination"/>
					/// --- for <c>--stdout</c>, where a single-file canvas format owns no
					/// path at all.
					/// </summary>
					/// <remarks>
					/// <c>convert</c> only ever branches on <c>PlannedItem::target</c>
					/// versus <c>PlannedItem::targetPattern</c>, because those are the two
					/// shapes <c>Planning::OutputPlanner</c> produces for a real file
					/// target --- but <c>--stdout</c> resolves to
					/// <c>Planning::OutputCardinality::Stream</c>, which leaves both
					/// absent. <c>Commands::ConvertCommand</c> calls this instead whenever
					/// <c>--stdout</c> is set, the same way <c>TextConversion</c> already
					/// has its own stdout branch for text formats; canvas formats never
					/// did.
					///
					/// Renders into a seekable buffer first rather than handing
					/// <paramref name="destination"/> to the canvas directly. Every
					/// published sample that streams a canvas writes to an in-memory
					/// buffer for the same reason: finishing a binary format means
					/// seeking back to patch a header or table computed only once the
					/// whole document has rendered, and the console's own stream is
					/// forward-only. Handing the console stream straight to the canvas
					/// produces no error and no output --- the render itself succeeds,
					/// but whatever the canvas needed to seek back and write never
					/// reaches anywhere.
					/// </remarks>
					void convertToStream(Engine::IDocument& document, std::ostream& destination, const Planning::PlannedItem& item,
						const Planning::FormatSpec& format, const Configuration::ConvertOptions& options);

					/// <summary>
					/// The option string a canvas for <paramref name="format"/> is opened
					/// with: <c>--canvas-options</c> first, then the format's own.
					/// </summary>
					/// <remarks>
					/// The order is the whole of the rule. The engine resolves a repeated
					/// key to its first occurrence, so putting the user in front means
					/// they win every key they name, and <c>FormatSpec::canvasOptions</c>
					/// only fills in what they left alone.
					///
					/// Used by both canvas paths, here and <c>--combine</c>'s shared
					/// canvas in <c>Commands::ConvertCommand</c>, because a combined run
					/// produces the same format and needs the same options. Static and
					/// free of the engine so the ordering can be tested directly.
					/// </remarks>
					static std::string composeCanvasOptions(
						const Planning::FormatSpec& format, const std::optional<std::string>& userOptions, bool noImages = false);

					/// <summary>
					/// What <c>--no-images</c> means for <paramref name="format"/>, or
					/// nothing when it means nothing.
					/// </summary>
					/// <remarks>
					/// Only the markdown formats have an engine switch for this ---
					/// <c>MARKDOWN_INCLUDE_IMAGES</c>. Plain text has no way to carry an
					/// image in the first place, and the flag is deliberately ignored for
					/// <c>html5</c> and <c>svg</c>: those inline the page raster itself,
					/// so suppressing images there would not drop embedded pictures, it
					/// would blank the page. Ignored rather than refused, so the flag can
					/// be set once for a batch of mixed formats without a run failing on
					/// the ones it does not touch.
					/// </remarks>
					static std::string imageSuppression(const Planning::FormatSpec& format);

					/// <summary>
					/// Renders one document onto a canvas someone else owns, for
					/// <c>--combine</c>. The caller opens the canvas once and passes every
					/// document through in turn.
					/// </summary>
					/// <remarks>
					/// Takes the fast, single-call <c>renderPages</c> path --- the one the
					/// published samples use --- whenever nothing needs a per-page hook;
					/// an explicit <c>--pages</c> selection or any decoration (including
					/// <c>--fingerprint</c>, which needs the page open after it renders)
					/// falls back to rendering one page at a time.
					/// </remarks>
					void appendTo(Engine::IOutputCanvas& canvas, Engine::IDocument& document, const Planning::PageRange& pages,
						const Configuration::ConvertOptions& options, const std::string& displayName);

					/// <summary>
					/// The pages of <paramref name="pages"/> that a document of
					/// <paramref name="pageCount"/> pages actually has.
					/// </summary>
					/// <remarks>
					/// <c>PageRange::enumerate</c> deliberately does not filter an
					/// explicit selection by count --- it has no way to know whether a
					/// count it is handed is genuine --- so that filtering happens here
					/// instead, at the one point that does have a real, just-verified page
					/// count: after the document is open. Kept as a pure function of the
					/// range and the count, rather than folded inline, so it is testable
					/// without the engine.
					/// </remarks>
					static std::vector<int> renderablePages(const Planning::PageRange& pages, int pageCount);

					/// <summary>
					/// The formatting half of <c>writeFingerprint</c>, kept as a pure
					/// function of the two attribute strings so it is testable without the
					/// engine.
					/// </summary>
					static std::optional<std::string> formatFingerprint(
						const std::string& displayName, int pageNumber, const std::string& fingerprintV2, const std::string& fingerprintV1);

				private:
					/// <summary>
					/// Opens the canvas a page would normally render onto --- or, under
					/// <c>--fingerprint</c>, a canvas whose bytes are thrown away instead.
					/// </summary>
					/// <remarks>
					/// The render still has to happen, because a fingerprint is an
					/// attribute of the rendered page, not of the document --- but the
					/// bytes it produces are never <paramref name="target"/>'s real
					/// content, the same way doc2text's own <c>--fingerprint</c> renders
					/// into a temporary stream it never keeps.
					/// <c>Configuration::OptionsValidator</c> already rejects every
					/// argument that would let someone rely on <paramref name="target"/>
					/// instead --- <c>--out-dir</c>, <c>--out-file</c>, <c>--stdout</c>,
					/// <c>--view</c>, <c>--combine</c> --- so it is only ever a plan's
					/// default-directory guess here, never a path the user asked for by
					/// name.
					/// </remarks>
					/// <summary>Applies the run's output policy to one resolved per-page target.</summary>
					void verifyPageTarget(const std::string& target, const Configuration::ConvertOptions& options);

					/// <summary>Renames a web-safe page to match whatever the engine actually wrote.</summary>
					void nameByRenderedType(const std::string& target, const Planning::FormatSpec& format,
						const Configuration::ConvertOptions& options);

					std::unique_ptr<Engine::IOutputCanvas> makeCanvas(const std::string& target, Hyland::DocFilters::CanvasType canvasType,
						const std::string& canvasOptions, const Configuration::ConvertOptions& options);

					void renderPage(Engine::IOutputCanvas& canvas, Engine::IDocument& document, int pageNumber,
						const Configuration::ConvertOptions& options, const std::string& displayName);

					/// <summary>
					/// Writes <c>--fingerprint</c>'s one line for a page that has just
					/// been rendered.
					/// </summary>
					/// <remarks>
					/// Confirmed against doc2text's own <c>fingerPrintPage</c>: try the
					/// <c>IMAGE_FINGERPRINT_V2</c> page attribute first, fall back to the
					/// older <c>IMAGE_FINGERPRINT</c>, and print nothing when both are
					/// empty --- doc2text only calls this after the page has rendered onto
					/// a canvas, which is why it lives here rather than in
					/// <c>StructureReport</c>: the fingerprint is a property of the
					/// rendered image, not of the document's layout.
					/// </remarks>
					void writeFingerprint(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName);

					static bool hasPageDecorations(const Configuration::ConvertOptions& options);

					/// <summary>Applies <c>--redact</c> and <c>--form</c> to one page before it is rendered.</summary>
					/// <remarks>
					/// Absent when the page needs neither, so a run using none of them
					/// never builds decorations at all. <c>--watermark</c> is handled
					/// separately, by <c>drawWatermark</c>.
					/// </remarks>
					static std::optional<Engine::EnginePageDecorations> applyPageDecorations(int pageNumber, const Configuration::ConvertOptions& options);

					void drawWatermark(Engine::IOutputCanvas& canvas, Engine::IDocumentPage& page, const Configuration::ConvertOptions& options);

					DocumentFiltersSession& _session;
					Runtime::IConsole& _console;
					const Configuration::SharedOptions* _shared;
					Runtime::IFileSystem* _fileSystem;

					Detail::DiscardingStreambuf _nullBuffer;
					std::ostream _nullStream;
				};
			}
		}
	}
}

#endif
