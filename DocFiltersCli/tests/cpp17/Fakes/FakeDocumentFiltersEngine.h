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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_FAKES_FAKEDOCUMENTFILTERSENGINE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_FAKES_FAKEDOCUMENTFILTERSENGINE_H

#include "Engine/EngineException.h"
#include "Engine/IDocumentFiltersEngine.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Tests
			{
				namespace Fakes
				{
					namespace Engine = Hyland::DocumentFilters::Cli::Engine;

					// -------------------------------------------------------------------------
					//  Every "Fake*" class below is a thin wrapper the CLI code is free to
					//  destroy the moment it is done with it --- matching the real adapter's
					//  own ownership shape, unique_ptr per call. What a test configures and
					//  inspects afterwards lives in the paired "Fake*State" struct instead,
					//  held by shared_ptr both here and inside whatever wrapper is currently
					//  handed to the CLI. C#'s reference needs no such split: a fake there is
					//  a garbage-collected object, so the same instance survives being
					//  "disposed" and stays inspectable for as long as the test holds a
					//  reference to it. A C++ unique_ptr's destructor genuinely deletes what
					//  it owns, so the state a test wants to inspect after the CLI is done
					//  with a document, page or canvas has to live somewhere that deletion
					//  does not reach.
					// -------------------------------------------------------------------------

					struct FakeDocumentState;

					/// <summary>The shared state behind one <see cref="FakePage"/>.</summary>
					struct FakePageState
					{
						int width = 612;
						int height = 792;
						std::map<std::string, std::string> attributes;
						std::vector<Engine::EngineHyperlink> hyperlinks;
						std::vector<Engine::EngineFormElement> formElements;
						std::vector<Engine::EngineAnnotation> annotations;
						std::optional<Engine::EnginePageElement> rootElement;
						bool isDisposed = false;
					};

					/// <summary>The shared state behind one <see cref="FakeSubFile"/>.</summary>
					struct FakeSubFileState
					{
						std::string id = "0001";
						std::string name;
						long long size = 0;
						std::optional<std::chrono::system_clock::time_point> modified;

						/// <summary>What this entry is when opened, for nested containers.</summary>
						std::shared_ptr<FakeDocumentState> content;

						/// <summary>Set to fail <c>open</c>.</summary>
						std::optional<Engine::EngineError> openFailure;

						/// <summary>Set to fail <c>copyTo</c>, the way an unreadable entry does.</summary>
						std::optional<Engine::EngineError> copyFailure;

						/// <summary>Paths this entry was copied to.</summary>
						std::vector<std::string> copiedTo;

						bool isDisposed = false;
					};

					/// <summary>The shared state behind one <see cref="FakeDocument"/>.</summary>
					struct FakeDocumentState
					{
						std::string path;

						/// <summary>What <c>getText</c> streams out, in caller-sized chunks as the real one does.</summary>
						std::string text;

						/// <summary>When set, opening this document demands exactly this password.</summary>
						std::optional<std::string> password;

						int fileType = 254;
						std::string mimeType = "application/octet-stream";
						std::string formatName = "Fake Format";
						int capabilities = 0;
						int pageCount = 0;

						std::vector<std::shared_ptr<FakePageState>> pages;
						std::vector<std::shared_ptr<FakeSubFileState>> subFileEntries;
						std::vector<std::shared_ptr<FakeSubFileState>> imageEntries;
						std::optional<Engine::EngineBookmark> bookmarks;

						/// <summary>Set to make a call on the open document fail.</summary>
						std::optional<Engine::EngineError> readFailure;

						/// <summary>Which call <see cref="readFailure"/> applies to; absent means all of them.</summary>
						std::optional<std::string> readFailureOn;

						bool wasOpened = false;
						bool isDisposed = false;
						int openFlags = 0;
						std::string openOptions;
						std::vector<std::string> docPacksWritten;

						/// <summary>Position <c>getText</c> has streamed out to so far.</summary>
						size_t textOffset = 0;
					};

					/// <summary>The shared state behind one <see cref="FakeCanvas"/>.</summary>
					struct FakeCanvasState
					{
						/// <summary>The path it was opened on, or absent for a stream canvas.</summary>
						std::optional<std::string> target;

						Hyland::DocFilters::CanvasType type = Hyland::DocFilters::CanvasType::PNG;
						std::string options;

						/// <summary>Pages rendered onto it, in order.</summary>
						std::vector<std::shared_ptr<FakePageState>> renderedPages;

						/// <summary>Decorations passed with each rendered page; absent where none were given.</summary>
						std::vector<std::optional<Engine::EnginePageDecorations>> decorations;

						/// <summary>Documents rendered whole, through the one-call path.</summary>
						std::vector<std::shared_ptr<FakeDocumentState>> renderedDocuments;

						int imagesDrawn = 0;
						bool isDisposed = false;

						/// <summary>Set to fail rendering, the way a full disk or a missing font does.</summary>
						std::optional<Engine::EngineError> renderFailure;
					};

					/// <summary>An engine failure with the code the real library would report.</summary>
					Engine::EngineException engineFailure(Engine::EngineError error, const std::string& message);

					/// <summary>One page of a <see cref="FakeDocumentState"/>.</summary>
					class FakePage : public Engine::IDocumentPage
					{
					public:
						FakePage() : _state(std::make_shared<FakePageState>()) {}

						explicit FakePage(std::shared_ptr<FakePageState> state) : _state(std::move(state)) {}

						~FakePage() override { _state->isDisposed = true; }

						int getWidth() override { return _state->width; }

						int getHeight() override { return _state->height; }

						std::string getAttribute(const std::string& name) override;

						std::vector<Engine::EngineHyperlink> hyperlinks() override { return _state->hyperlinks; }

						std::vector<Engine::EngineFormElement> formElements() override { return _state->formElements; }

						std::vector<Engine::EngineAnnotation> annotations() override { return _state->annotations; }

						std::optional<Engine::EnginePageElement> getRootPageElement() override { return _state->rootElement; }

						const std::shared_ptr<FakePageState>& state() const { return _state; }

					private:
						std::shared_ptr<FakePageState> _state;
					};

					/// <summary>A document the fake engine can hand out.</summary>
					class FakeDocument : public Engine::IDocument
					{
					public:
						/// <summary>
						/// A document not registered with any engine, for tests that hand one
						/// straight to the unit under test --- <c>SubfileExtraction</c>,
						/// <c>StructureReport</c>, <c>MetadataReport</c> and the like --- rather
						/// than opening it through <see cref="FakeDocumentFiltersEngine"/>.
						/// </summary>
						explicit FakeDocument(std::string path) : _state(std::make_shared<FakeDocumentState>())
						{
							_state->path = std::move(path);
						}

						explicit FakeDocument(std::shared_ptr<FakeDocumentState> state) : _state(std::move(state)) {}

						~FakeDocument() override { _state->isDisposed = true; }

						int getFileType() override;

						std::string getMimeType() override;

						std::string getFormatName() override;

						int getFileCapabilities() override;

						bool getEOF() override;

						std::string getText(int maxLength) override;

						int getPageCount() override;

						std::unique_ptr<Engine::IDocumentPage> getPage(int pageIndex) override;

						std::vector<std::unique_ptr<Engine::ISubFileEntry>> subFiles() override;

						std::vector<std::unique_ptr<Engine::ISubFileEntry>> images() override;

						std::optional<Engine::EngineBookmark> rootBookmark() override;

						void makeDocPack(const std::string& filename, const std::string& options) override;

						void makeDocPack(std::ostream& destination, const std::string& options) override;

						const std::shared_ptr<FakeDocumentState>& state() const { return _state; }

					private:
						void check(const std::string& call) const;

						std::shared_ptr<FakeDocumentState> _state;
					};

					/// <summary>One entry inside a fake container.</summary>
					class FakeSubFile : public Engine::ISubFileEntry
					{
					public:
						/// <summary>An entry not yet attached to any container's entry list.</summary>
						explicit FakeSubFile(std::string name, std::string id = "0001") : _state(std::make_shared<FakeSubFileState>())
						{
							_state->name = std::move(name);
							_state->id = std::move(id);
						}

						explicit FakeSubFile(std::shared_ptr<FakeSubFileState> state) : _state(std::move(state)) {}

						~FakeSubFile() override { _state->isDisposed = true; }

						std::string id() override { return _state->id; }

						std::string name() override { return _state->name; }

						long long size() override { return _state->size; }

						std::optional<std::chrono::system_clock::time_point> modified() override { return _state->modified; }

						std::unique_ptr<Engine::IDocument> open(int flags, const std::string& options) override;

						void copyTo(const std::string& path) override;

						const std::shared_ptr<FakeSubFileState>& state() const { return _state; }

					private:
						std::shared_ptr<FakeSubFileState> _state;
					};

					/// <summary>A canvas that records what was drawn on it instead of writing bytes.</summary>
					class FakeCanvas : public Engine::IOutputCanvas
					{
					public:
						explicit FakeCanvas(std::shared_ptr<FakeCanvasState> state) : _state(std::move(state)) {}

						~FakeCanvas() override { _state->isDisposed = true; }

						void renderPage(Engine::IDocumentPage& page, const std::string& options,
							const std::optional<Engine::EnginePageDecorations>& decorations) override;

						void renderPages(Engine::IDocument& document) override;

						void drawImage(int x, int y, const std::vector<std::uint8_t>& image, const std::string& options) override;

						const std::shared_ptr<FakeCanvasState>& state() const { return _state; }

					private:
						std::shared_ptr<FakeCanvasState> _state;
					};

					/// <summary>
					/// A Document Filters engine that answers from a script instead of a
					/// library.
					/// </summary>
					/// <remarks>
					/// The engine is the one dependency the execution paths cannot be tested
					/// around: it needs a license, a native library, and a real document that
					/// actually exhibits whatever is being tested. Every failure this thing
					/// can produce is a failure the real engine produces too --- the codes
					/// come from <see cref="Engine::EngineError"/>, which mirrors
					/// <c>ISYS11dfConstants</c> --- so a test here is a statement about how
					/// the CLI handles a real outcome.
					///
					/// Documents are added by path. Anything not added fails the way the
					/// engine fails for a file it cannot read, so "the document is not there"
					/// needs no setup.
					/// </remarks>
					class FakeDocumentFiltersEngine : public Engine::IDocumentFiltersEngine
					{
					public:
						/// <summary>
						/// Sets <see cref="isDisposed"/>, the same as the real engine's own
						/// destructor releasing the library --- there being no separate
						/// dispose step in this interface is what makes that the whole of
						/// this override.
						/// </summary>
						~FakeDocumentFiltersEngine() override;

						// ------------------------------------------------------------- building

						/// <summary>Adds a document the engine can open.</summary>
						std::shared_ptr<FakeDocumentState> addDocument(const std::string& path, const std::string& text = "");

						/// <summary>
						/// Makes opening <paramref name="path"/> fail with a given engine
						/// error.
						/// </summary>
						/// <remarks>
						/// <see cref="Engine::EngineError::password"/> is the one worth calling
						/// out: the real engine reports a wrong password and a refused prompt
						/// identically, so a test using it covers both.
						/// </remarks>
						FakeDocumentFiltersEngine& failToOpen(const std::string& path, Engine::EngineError error);

						/// <summary>Adds a document that opens only when the right password is supplied.</summary>
						std::shared_ptr<FakeDocumentState> addProtectedDocument(
							const std::string& path, const std::string& password, const std::string& text = "");

						// -------------------------------------------------------------- inspection

						/// <summary>Every call made, in order, for asserting on what the CLI asked the engine to do.</summary>
						std::vector<std::string> calls;

						/// <summary>Run at the start of an open, before the cancellation poll.</summary>
						std::function<void()> duringOpen;

						/// <summary>Canvases handed out, in order of creation.</summary>
						std::vector<std::shared_ptr<FakeCanvasState>> canvases;

						/// <summary>Set to fail <c>initialize</c>, the way a bad license or a missing library does.</summary>
						std::optional<Engine::EngineError> initialisationFailure;

						/// <summary>Set to fail every <c>makeCanvas</c>.</summary>
						std::optional<Engine::EngineError> canvasFailure;

						/// <summary>
						/// Set to fail <see cref="getSupportedFormats"/> and
						/// <see cref="getAvailableOptions"/>.
						/// </summary>
						std::optional<Engine::EngineError> catalogFailure;

						std::vector<Engine::EngineFormat> formats;

						std::vector<Engine::EngineOption> options;

						bool isDisposed = false;

						/// <summary>The last path <c>setTempPath</c> was given, or absent.</summary>
						std::optional<std::string> tempPath;

						/// <summary>What <c>getMemoryStatus</c> answers.</summary>
						std::string memoryStatus = "heap: 0 bytes";

						// -------------------------------------------------------------- engine

						void initialize(const std::string& licenseKey, const std::string& libraryPath) override;

						std::unique_ptr<Engine::IDocument> openDocument(const std::string& path, int flags, const std::string& options,
							const std::function<std::optional<std::string>()>& password, const std::function<bool()>& shouldCancel,
							const std::function<int(const std::string&)>& logLevelFor,
							const std::function<void(int, const std::string&, const std::string&)>& onLogMessage) override;

						std::unique_ptr<Engine::IDocument> openDocument(std::istream& source, const std::string& displayName, int flags,
							const std::string& options, const std::function<std::optional<std::string>()>& password,
							const std::function<bool()>& shouldCancel, const std::function<int(const std::string&)>& logLevelFor,
							const std::function<void(int, const std::string&, const std::string&)>& onLogMessage) override;

						void setTempPath(const std::string& path) override;

						std::string getMemoryStatus() override;

						Engine::EngineFileType identifyFile(const std::string& path) override;

						Engine::EngineFileType identifyFile(std::istream& source, const std::string& displayName) override;

						std::unique_ptr<Engine::IOutputCanvas> makeCanvas(
							const std::string& target, Hyland::DocFilters::CanvasType type, const std::string& options) override;

						std::unique_ptr<Engine::IOutputCanvas> makeCanvas(
							std::ostream& target, Hyland::DocFilters::CanvasType type, const std::string& options) override;

						std::vector<Engine::EngineFormat> getSupportedFormats() override;

						std::vector<Engine::EngineOption> getAvailableOptions() override;

						std::string describeVersion() override;

					private:
						std::unique_ptr<Engine::IDocument> openDocumentByPath(const std::string& path, int flags,
							const std::string& options, const std::function<std::optional<std::string>()>& password,
							const std::function<bool()>& shouldCancel, const std::function<int(const std::string&)>& logLevelFor,
							const std::function<void(int, const std::string&, const std::string&)>& onLogMessage);

						std::map<std::string, std::shared_ptr<FakeDocumentState>> _documents;
						std::map<std::string, Engine::EngineError> _openFailures;
					};
				}
			}
		}
	}
}

#endif
