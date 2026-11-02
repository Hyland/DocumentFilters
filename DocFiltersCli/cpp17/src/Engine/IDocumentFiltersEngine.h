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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_ENGINE_IDOCUMENTFILTERSENGINE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_ENGINE_IDOCUMENTFILTERSENGINE_H

// EngineHyperlink::type, EngineFormElement::type, EngineAnnotation::type and
// EnginePageElement::type name the binding's own enums directly, the same
// exception Planning::FormatSpec::canvasType makes for CanvasType: each is
// public and constructible, so a fake gains nothing by wrapping it.
#include <DocumentFiltersObjects.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <ios>
#include <istream>
#include <memory>
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
			namespace Engine
			{
				// -------------------------------------------------------------------------
				//  THE ENGINE SEAM
				//
				//  One interface method per Document Filters call, with no logic of its
				//  own. Everything above it --- the conversion paths, the reports, the
				//  batch runner --- talks to these interfaces, so all of it can be tested
				//  without the native library, a license, or a real document.
				//
				//  The rule for anything added here: it maps one call, and it decides
				//  nothing. A method that chooses, filters or formats belongs above the
				//  seam, where it can be tested as a pure function.
				// -------------------------------------------------------------------------

				class IDocument;
				class IDocumentPage;
				class IOutputCanvas;
				class ISubFileEntry;

				// ------------------------------------------------------------------ values

				/// <summary>What a file is, without opening it.</summary>
				struct EngineFileType
				{
					/// <summary>The numeric format id; 0 when the engine cannot place the file.</summary>
					int id = 0;

					std::string mimeType;

					/// <summary>The format's long name, e.g. "Microsoft Word 2007".</summary>
					std::string name;

					/// <summary>The text/subfiles/hd flag word.</summary>
					int capabilities = 0;
				};

				/// <summary>One row of <c>getSupportedFormats</c>.</summary>
				struct EngineFormat
				{
					int id = 0;
					std::string shortName;
					std::string displayName;
				};

				/// <summary>One row of <c>getAvailableOptions</c>.</summary>
				struct EngineOption
				{
					std::string displayName;

					/// <summary>The value's type, as the engine names it: "int", "string", "bool".</summary>
					std::string type;

					std::string description;
					int flags = 0;
					std::string defaultValue;
				};

				/// <summary>One node of the document outline.</summary>
				struct EngineBookmark
				{
					std::string title;
					int level = 0;

					/// <summary>Zero-based, as the binding reports it.</summary>
					int pageIndex = 0;

					std::vector<EngineBookmark> children;
				};

				/// <summary>One hyperlink on a page.</summary>
				struct EngineHyperlink
				{
					Hyland::DocFilters::Hyperlink::Type type = Hyland::DocFilters::Hyperlink::Type::Uri;
					std::string uri;
					std::string ref;
					int x = 0;
					int y = 0;
					int width = 0;
					int height = 0;
				};

				/// <summary>One form element on a page.</summary>
				struct EngineFormElement
				{
					Hyland::DocFilters::FormElement::Type type = Hyland::DocFilters::FormElement::Type::SinglelineText;
					std::string name;
					int x = 0;
					int y = 0;
					int width = 0;
					int height = 0;
				};

				/// <summary>One annotation on a page.</summary>
				struct EngineAnnotation
				{
					Hyland::DocFilters::Annotation::Type type = Hyland::DocFilters::Annotation::Type::Unknown;
					std::string title;
					std::string text;
				};

				/// <summary>
				/// A page element's bounds. The binding's own rectangle types are handles
				/// into engine-owned data; this is a plain copy, the C++ equivalent of the
				/// C# port's own choice of a general <c>RectangleF</c> over the binding's
				/// rectangle type.
				/// </summary>
				struct EngineRectF
				{
					float left = 0;
					float top = 0;
					float right = 0;
					float bottom = 0;
				};

				/// <summary>One node of a page's element tree.</summary>
				struct EnginePageElement
				{
					Hyland::DocFilters::PageElementType type = Hyland::DocFilters::PageElementType::None;
					int depth = 0;
					EngineRectF bounds;
					std::vector<EnginePageElement> children;
				};

				/// <summary>One <c>--redact</c> region, in page coordinates.</summary>
				struct EngineRedaction
				{
					int x = 0;
					int y = 0;
					int width = 0;
					int height = 0;
				};

				/// <summary>One <c>--form</c> assignment.</summary>
				struct EngineFormValue
				{
					std::string name;
					std::string value;
				};

				/// <summary>
				/// What <c>--redact</c> and <c>--form</c> add to one page before it renders.
				/// </summary>
				/// <remarks>
				/// Carried as data rather than as the binding's own render-properties type
				/// for the same reason as everything else here: the adapter builds that
				/// object, and a fake can assert on these values without one.
				/// </remarks>
				struct EnginePageDecorations
				{
					std::vector<EngineRedaction> redactions;
					std::vector<EngineFormValue> formValues;

					bool isEmpty() const { return redactions.empty() && formValues.empty(); }
				};

				// ---------------------------------------------------------------- interfaces

				/// <summary>The engine itself: what Document Filters offers before a document exists.</summary>
				class IDocumentFiltersEngine
				{
				public:
					virtual ~IDocumentFiltersEngine() = default;

					/// <summary><c>DocumentFilters.Initialize</c>.</summary>
					/// <exception cref="EngineException">The library was not found, or the license was rejected.</exception>
					virtual void initialize(const std::string& licenseKey, const std::string& libraryPath) = 0;

					/// <summary>
					/// <c>GetExtractor</c> followed by <c>Extractor::Open</c>, which the engine
					/// applies as one step.
					/// </summary>
					/// <remarks>
					/// The two are not separable here on purpose: the open flags take effect
					/// <em>during</em> <c>Open</c>, so handing back an unopened extractor would
					/// invite exactly the mistake this exists to prevent.
					///
					/// <paramref name="flags"/> is the complete <c>IGR_</c> flag set, format bit
					/// included, and reaches the engine unaltered.
					///
					/// <paramref name="password"/> is asked for the document's password when
					/// the engine requests one, and returns nullopt to give up --- which
					/// surfaces as an <see cref="EngineException"/> carrying
					/// <see cref="EngineError::password"/>.
					///
					/// <paramref name="shouldCancel"/> is polled while the engine works, and
					/// returning true stops the open with <see cref="EngineError::cancelled"/>.
					/// </remarks>
					/// <exception cref="EngineException">The document could not be opened.</exception>
					virtual std::unique_ptr<IDocument> openDocument(const std::string& path, int flags, const std::string& options,
						const std::function<std::optional<std::string>()>& password,
						const std::function<bool()>& shouldCancel = nullptr,
						const std::function<int(const std::string&)>& logLevelFor = nullptr,
						const std::function<void(int, const std::string&, const std::string&)>& onLogMessage = nullptr) = 0;

					/// <summary>
					/// The same, reading the document from <paramref name="source"/> rather
					/// than from a path.
					/// </summary>
					/// <remarks>
					/// For the debug switches that have to control how bytes reach the engine:
					/// <c>--debug-use-memory-stream</c>, <c>--debug-truncate</c> and
					/// <c>--debug-measure-io</c>. <paramref name="displayName"/> is what
					/// failures are reported against, since a stream has no name of its own.
					/// </remarks>
					/// <exception cref="EngineException">The document could not be opened.</exception>
					virtual std::unique_ptr<IDocument> openDocument(std::istream& source, const std::string& displayName, int flags,
						const std::string& options, const std::function<std::optional<std::string>()>& password,
						const std::function<bool()>& shouldCancel = nullptr,
						const std::function<int(const std::string&)>& logLevelFor = nullptr,
						const std::function<void(int, const std::string&, const std::string&)>& onLogMessage = nullptr) = 0;

					/// <summary>Points the engine at a directory for its scratch files --- <c>--temp-dir</c>.</summary>
					virtual void setTempPath(const std::string& path) = 0;

					/// <summary>
					/// The engine's own memory report --- <c>-x</c> and <c>--xx</c>. Free-form
					/// text the engine composes, printed verbatim: its content is the
					/// engine's to change and is not contract.
					/// </summary>
					virtual std::string getMemoryStatus() = 0;

					/// <summary>
					/// <c>GetExtractor</c> and <c>getFileType</c>, <em>without</em> opening: what
					/// the file is, without reading it.
					/// </summary>
					/// <exception cref="EngineException">The file could not be reached or recognised.</exception>
					virtual EngineFileType identifyFile(const std::string& path) = 0;

					/// <summary>
					/// The same, from a stream --- so the debug switches that change how bytes
					/// arrive reach <c>identify</c> too.
					/// </summary>
					/// <exception cref="EngineException">The file could not be reached or recognised.</exception>
					virtual EngineFileType identifyFile(std::istream& source, const std::string& displayName) = 0;

					/// <summary><c>MakeOutputCanvas</c> onto a file.</summary>
					virtual std::unique_ptr<IOutputCanvas> makeCanvas(
						const std::string& target, Hyland::DocFilters::CanvasType type, const std::string& options) = 0;

					/// <summary><c>MakeOutputCanvas</c> onto a stream, for <c>--stdout</c> and <c>--fingerprint</c>.</summary>
					virtual std::unique_ptr<IOutputCanvas> makeCanvas(
						std::ostream& target, Hyland::DocFilters::CanvasType type, const std::string& options) = 0;

					/// <summary><c>GetSupportedFormats</c>, for the <c>formats</c> command.</summary>
					virtual std::vector<EngineFormat> getSupportedFormats() = 0;

					/// <summary><c>GetAvailableOptions</c>, for the <c>options</c> command.</summary>
					virtual std::vector<EngineOption> getAvailableOptions() = 0;

					/// <summary>The binding's version, for the <c>version</c> command.</summary>
					virtual std::string describeVersion() = 0;
				};

				/// <summary>One open document: <c>Extractor</c>.</summary>
				class IDocument
				{
				public:
					virtual ~IDocument() = default;

					/// <summary><c>getFileType()</c>: the numeric format id.</summary>
					virtual int getFileType() = 0;

					/// <summary><c>getFileType(IGR_FORMAT_MIMETYPE)</c>.</summary>
					virtual std::string getMimeType() = 0;

					/// <summary><c>getFileType(IGR_FORMAT_LONG_NAME)</c>.</summary>
					virtual std::string getFormatName() = 0;

					/// <summary><c>getFileCapabilities()</c>: the text/subfiles/hd flag word.</summary>
					virtual int getFileCapabilities() = 0;

					/// <summary><c>getEOF()</c>.</summary>
					virtual bool getEOF() = 0;

					/// <summary><c>GetText(maxLength)</c>.</summary>
					virtual std::string getText(int maxLength) = 0;

					/// <summary><c>GetPageCount()</c>.</summary>
					virtual int getPageCount() = 0;

					/// <summary><c>GetPage(index)</c>, zero-based as the binding has it.</summary>
					virtual std::unique_ptr<IDocumentPage> getPage(int pageIndex) = 0;

					/// <summary><c>GetFirstSubFile</c>/<c>GetNextSubFile</c>, drained into a list.</summary>
					virtual std::vector<std::unique_ptr<ISubFileEntry>> subFiles() = 0;

					/// <summary><c>GetFirstImage</c>/<c>GetNextImage</c>, drained into a list.</summary>
					virtual std::vector<std::unique_ptr<ISubFileEntry>> images() = 0;

					/// <summary><c>RootBookmark</c>, or absent when the document has no outline.</summary>
					virtual std::optional<EngineBookmark> rootBookmark() = 0;

					/// <summary><c>MakeDocPack</c> onto a file.</summary>
					virtual void makeDocPack(const std::string& filename, const std::string& options) = 0;

					/// <summary><c>MakeDocPack</c> onto a stream.</summary>
					virtual void makeDocPack(std::ostream& destination, const std::string& options) = 0;
				};

				/// <summary>One page of an open document: <c>Page</c>.</summary>
				class IDocumentPage
				{
				public:
					virtual ~IDocumentPage() = default;

					/// <summary><c>GetWidth()</c>.</summary>
					virtual int getWidth() = 0;

					/// <summary><c>GetHeight()</c>.</summary>
					virtual int getHeight() = 0;

					/// <summary><c>GetAttribute(name)</c>; empty when the page has no such attribute.</summary>
					virtual std::string getAttribute(const std::string& name) = 0;

					virtual std::vector<EngineHyperlink> hyperlinks() = 0;

					virtual std::vector<EngineFormElement> formElements() = 0;

					virtual std::vector<EngineAnnotation> annotations() = 0;

					/// <summary><c>GetRootPageElement()</c>, or absent when the page exposes no element tree.</summary>
					virtual std::optional<EnginePageElement> getRootPageElement() = 0;
				};

				/// <summary>An output canvas: <c>Canvas</c>.</summary>
				class IOutputCanvas
				{
				public:
					virtual ~IOutputCanvas() = default;

					/// <summary><c>RenderPage</c>. <paramref name="decorations"/> is absent when the page needs none.</summary>
					virtual void renderPage(
						IDocumentPage& page, const std::string& options, const std::optional<EnginePageDecorations>& decorations) = 0;

					/// <summary><c>RenderPages(extractor)</c>: the whole document in one call.</summary>
					virtual void renderPages(IDocument& document) = 0;

					/// <summary><c>DrawImage</c>, for <c>--watermark</c>.</summary>
					virtual void drawImage(int x, int y, const std::vector<std::uint8_t>& image, const std::string& options) = 0;
				};

				/// <summary>One entry inside a container: <c>SubFile</c>.</summary>
				class ISubFileEntry
				{
				public:
					virtual ~ISubFileEntry() = default;

					virtual std::string id() = 0;

					/// <summary>The name as the container gives it --- untrusted, and never used as a path unchecked.</summary>
					virtual std::string name() = 0;

					virtual long long size() = 0;

					virtual std::optional<std::chrono::system_clock::time_point> modified() = 0;

					/// <summary><c>SubFile::Open</c>: the entry as a document in its own right.</summary>
					/// <exception cref="EngineException">The entry could not be opened.</exception>
					virtual std::unique_ptr<IDocument> open(int flags, const std::string& options) = 0;

					/// <summary><c>CopyTo(filename)</c>: the entry's bytes, untouched.</summary>
					virtual void copyTo(const std::string& path) = 0;
				};
			}
		}
	}
}

#endif
