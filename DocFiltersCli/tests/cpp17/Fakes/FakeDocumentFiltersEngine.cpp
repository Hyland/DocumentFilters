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

#include "FakeDocumentFiltersEngine.h"

#include <algorithm>

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
					namespace
					{
						/// <summary>The length of a stream, leaving its read position unchanged.</summary>
						std::streamoff streamLength(std::istream& source)
						{
							std::istream::pos_type current = source.tellg();
							source.seekg(0, std::ios::end);
							std::istream::pos_type end = source.tellg();
							source.seekg(current);
							return static_cast<std::streamoff>(end);
						}
					}

					Engine::EngineException engineFailure(Engine::EngineError error, const std::string& message)
					{
						return Engine::EngineException(error, static_cast<int>(error), message);
					}

					// ------------------------------------------------------------------ FakePage

					std::string FakePage::getAttribute(const std::string& name)
					{
						auto found = _state->attributes.find(name);
						return found != _state->attributes.end() ? found->second : std::string();
					}

					// -------------------------------------------------------------- FakeDocument

					void FakeDocument::check(const std::string& call) const
					{
						if (_state->readFailure.has_value()
							&& (!_state->readFailureOn.has_value() || *_state->readFailureOn == call))
						{
							throw engineFailure(*_state->readFailure, call + " failed on " + _state->path);
						}
					}

					int FakeDocument::getFileType()
					{
						check("getFileType");
						return _state->fileType;
					}

					std::string FakeDocument::getMimeType()
					{
						check("getMimeType");
						return _state->mimeType;
					}

					std::string FakeDocument::getFormatName()
					{
						check("getFormatName");
						return _state->formatName;
					}

					int FakeDocument::getFileCapabilities()
					{
						check("getFileCapabilities");
						return _state->capabilities;
					}

					bool FakeDocument::getEOF()
					{
						check("getEOF");
						return _state->textOffset >= _state->text.size();
					}

					std::string FakeDocument::getText(int maxLength)
					{
						check("getText");

						size_t remaining = _state->text.size() - _state->textOffset;
						size_t take = std::min(static_cast<size_t>(std::max(maxLength, 0)), remaining);
						if (take == 0)
							return std::string();

						std::string chunk = _state->text.substr(_state->textOffset, take);
						_state->textOffset += take;
						return chunk;
					}

					int FakeDocument::getPageCount()
					{
						check("getPageCount");
						return !_state->pages.empty() ? static_cast<int>(_state->pages.size()) : _state->pageCount;
					}

					std::unique_ptr<Engine::IDocumentPage> FakeDocument::getPage(int pageIndex)
					{
						check("getPage");

						if (pageIndex < 0 || pageIndex >= getPageCount())
							throw engineFailure(Engine::EngineError::notFound, "no page " + std::to_string(pageIndex) + " in " + _state->path);

						// A document given only a page count, not page contents, still renders.
						while (static_cast<int>(_state->pages.size()) <= pageIndex)
							_state->pages.push_back(std::make_shared<FakePageState>());

						return std::make_unique<FakePage>(_state->pages[static_cast<size_t>(pageIndex)]);
					}

					std::vector<std::unique_ptr<Engine::ISubFileEntry>> FakeDocument::subFiles()
					{
						check("subFiles");

						std::vector<std::unique_ptr<Engine::ISubFileEntry>> result;
						for (const std::shared_ptr<FakeSubFileState>& entry : _state->subFileEntries)
							result.push_back(std::make_unique<FakeSubFile>(entry));
						return result;
					}

					std::vector<std::unique_ptr<Engine::ISubFileEntry>> FakeDocument::images()
					{
						check("images");

						std::vector<std::unique_ptr<Engine::ISubFileEntry>> result;
						for (const std::shared_ptr<FakeSubFileState>& entry : _state->imageEntries)
							result.push_back(std::make_unique<FakeSubFile>(entry));
						return result;
					}

					std::optional<Engine::EngineBookmark> FakeDocument::rootBookmark()
					{
						check("rootBookmark");
						return _state->bookmarks;
					}

					void FakeDocument::makeDocPack(const std::string& filename, const std::string&)
					{
						check("makeDocPack");
						_state->docPacksWritten.push_back(filename);
					}

					void FakeDocument::makeDocPack(std::ostream&, const std::string&)
					{
						check("makeDocPack");
						_state->docPacksWritten.push_back("<stream>");
					}

					// --------------------------------------------------------------- FakeSubFile

					std::unique_ptr<Engine::IDocument> FakeSubFile::open(int, const std::string&)
					{
						if (_state->openFailure.has_value())
							throw engineFailure(*_state->openFailure, "could not open subfile " + _state->name);

						std::shared_ptr<FakeDocumentState> content = _state->content;
						if (!content)
						{
							content = std::make_shared<FakeDocumentState>();
							content->path = _state->name;
						}

						return std::make_unique<FakeDocument>(content);
					}

					void FakeSubFile::copyTo(const std::string& path)
					{
						if (_state->copyFailure.has_value())
							throw engineFailure(*_state->copyFailure, "could not copy subfile " + _state->name);

						_state->copiedTo.push_back(path);
					}

					// ---------------------------------------------------------------- FakeCanvas

					void FakeCanvas::renderPage(
						Engine::IDocumentPage& page, const std::string&, const std::optional<Engine::EnginePageDecorations>& decorations)
					{
						if (_state->renderFailure.has_value())
							throw engineFailure(*_state->renderFailure, "render failed");

						auto* fakePage = dynamic_cast<FakePage*>(&page);
						_state->renderedPages.push_back(fakePage != nullptr ? fakePage->state() : nullptr);
						_state->decorations.push_back(decorations);
					}

					void FakeCanvas::renderPages(Engine::IDocument& document)
					{
						if (_state->renderFailure.has_value())
							throw engineFailure(*_state->renderFailure, "render failed");

						auto* fakeDocument = dynamic_cast<FakeDocument*>(&document);
						_state->renderedDocuments.push_back(fakeDocument != nullptr ? fakeDocument->state() : nullptr);
					}

					void FakeCanvas::drawImage(int, int, const std::vector<std::uint8_t>&, const std::string&) { ++_state->imagesDrawn; }

					// -------------------------------------------------------- FakeDocumentFiltersEngine

					FakeDocumentFiltersEngine::~FakeDocumentFiltersEngine() { isDisposed = true; }

					std::shared_ptr<FakeDocumentState> FakeDocumentFiltersEngine::addDocument(const std::string& path, const std::string& text)
					{
						auto document = std::make_shared<FakeDocumentState>();
						document->path = path;
						document->text = text;
						_documents[path] = document;
						return document;
					}

					FakeDocumentFiltersEngine& FakeDocumentFiltersEngine::failToOpen(const std::string& path, Engine::EngineError error)
					{
						_openFailures[path] = error;
						return *this;
					}

					std::shared_ptr<FakeDocumentState> FakeDocumentFiltersEngine::addProtectedDocument(
						const std::string& path, const std::string& password, const std::string& text)
					{
						std::shared_ptr<FakeDocumentState> document = addDocument(path, text);
						document->password = password;
						return document;
					}

					void FakeDocumentFiltersEngine::initialize(const std::string& licenseKey, const std::string& libraryPath)
					{
						calls.push_back("initialize(" + (licenseKey.empty() ? std::string("null") : licenseKey) + ","
							+ (libraryPath.empty() ? std::string("null") : libraryPath) + ")");

						if (initialisationFailure.has_value())
							throw engineFailure(*initialisationFailure, "initialisation failed");
					}

					std::unique_ptr<Engine::IDocument> FakeDocumentFiltersEngine::openDocumentByPath(const std::string& path, int flags,
						const std::string& options, const std::function<std::optional<std::string>()>& password,
						const std::function<bool()>& shouldCancel, const std::function<int(const std::string&)>& logLevelFor,
						const std::function<void(int, const std::string&, const std::string&)>& onLogMessage)
					{
						calls.push_back("openDocument(" + path + "," + std::to_string(flags) + "," + options
							+ (shouldCancel ? ",cancellable)" : ")"));

						// Runs before the cancellation poll, so a test can make something true
						// *during* the open rather than before it. Ctrl+C arriving mid-open is
						// the only way to reach the cancellation path at all --- BatchRunner
						// checks between documents, so one requested earlier stops the run
						// before anything opens.
						if (duringOpen)
							duringOpen();

						// The engine polls this while it parses; the fake polls it once, which
						// is enough to prove a caller wired it up and translates the refusal.
						if (shouldCancel && shouldCancel())
							throw engineFailure(Engine::EngineError::cancelled, "cancelled while opening " + path);

						auto scripted = _openFailures.find(path);
						if (scripted != _openFailures.end())
							throw engineFailure(scripted->second, "could not open " + path);

						auto found = _documents.find(path);
						if (found == _documents.end())
							throw engineFailure(Engine::EngineError::openError, "no such document: " + path);

						std::shared_ptr<FakeDocumentState> document = found->second;

						if (document->password.has_value())
						{
							// The real engine asks through the callback and turns a refusal
							// into IGR_E_PASSWORD out of Open, so a wrong answer and no answer
							// land here the same way.
							std::optional<std::string> offered = password ? password() : std::nullopt;
							if (offered != document->password)
								throw engineFailure(Engine::EngineError::password, "password required for " + path);
						}

						// The engine asks the level per module, then hands back messages.
						// Answering once with a known module is enough to show a caller wired
						// both halves.
						if (logLevelFor && onLogMessage)
						{
							int level = logLevelFor("pdf");
							if (level > 0)
								onLogMessage(level, "pdf", "opened " + path);
						}

						document->openFlags = flags;
						document->openOptions = options;
						document->wasOpened = true;
						return std::make_unique<FakeDocument>(document);
					}

					std::unique_ptr<Engine::IDocument> FakeDocumentFiltersEngine::openDocument(const std::string& path, int flags,
						const std::string& options, const std::function<std::optional<std::string>()>& password,
						const std::function<bool()>& shouldCancel, const std::function<int(const std::string&)>& logLevelFor,
						const std::function<void(int, const std::string&, const std::string&)>& onLogMessage)
					{
						return openDocumentByPath(path, flags, options, password, shouldCancel, logLevelFor, onLogMessage);
					}

					std::unique_ptr<Engine::IDocument> FakeDocumentFiltersEngine::openDocument(std::istream& source,
						const std::string& displayName, int flags, const std::string& options,
						const std::function<std::optional<std::string>()>& password, const std::function<bool()>& shouldCancel,
						const std::function<int(const std::string&)>& logLevelFor,
						const std::function<void(int, const std::string&, const std::string&)>& onLogMessage)
					{
						calls.push_back("openDocument(<stream:" + std::to_string(streamLength(source)) + ">," + displayName + ","
							+ std::to_string(flags) + "," + options + (shouldCancel ? ",cancellable)" : ")"));

						return openDocumentByPath(displayName, flags, options, password, shouldCancel, logLevelFor, onLogMessage);
					}

					void FakeDocumentFiltersEngine::setTempPath(const std::string& path)
					{
						calls.push_back("setTempPath(" + path + ")");
						tempPath = path;
					}

					std::string FakeDocumentFiltersEngine::getMemoryStatus()
					{
						calls.push_back("getMemoryStatus()");
						return memoryStatus;
					}

					Engine::EngineFileType FakeDocumentFiltersEngine::identifyFile(const std::string& path)
					{
						calls.push_back("identifyFile(" + path + ")");

						auto scripted = _openFailures.find(path);
						if (scripted != _openFailures.end())
							throw engineFailure(scripted->second, "could not identify " + path);

						auto found = _documents.find(path);
						if (found == _documents.end())
							throw engineFailure(Engine::EngineError::openError, "no such document: " + path);

						const std::shared_ptr<FakeDocumentState>& document = found->second;

						Engine::EngineFileType result;
						result.id = document->fileType;
						result.mimeType = document->mimeType;
						result.name = document->formatName;
						result.capabilities = document->capabilities;
						return result;
					}

					Engine::EngineFileType FakeDocumentFiltersEngine::identifyFile(std::istream& source, const std::string& displayName)
					{
						calls.push_back("identifyFile(<stream:" + std::to_string(streamLength(source)) + ">," + displayName + ")");
						return identifyFile(displayName);
					}

					std::unique_ptr<Engine::IOutputCanvas> FakeDocumentFiltersEngine::makeCanvas(
						const std::string& target, Hyland::DocFilters::CanvasType type, const std::string& options)
					{
						calls.push_back("makeCanvas(" + target + "," + std::to_string(static_cast<int>(type)) + "," + options + ")");

						if (canvasFailure.has_value())
							throw engineFailure(*canvasFailure, "could not create canvas at " + target);

						auto state = std::make_shared<FakeCanvasState>();
						state->target = target;
						state->type = type;
						state->options = options;
						canvases.push_back(state);
						return std::make_unique<FakeCanvas>(state);
					}

					std::unique_ptr<Engine::IOutputCanvas> FakeDocumentFiltersEngine::makeCanvas(
						std::ostream&, Hyland::DocFilters::CanvasType type, const std::string& options)
					{
						calls.push_back("makeCanvas(<stream>," + std::to_string(static_cast<int>(type)) + "," + options + ")");

						if (canvasFailure.has_value())
							throw engineFailure(*canvasFailure, "could not create canvas on a stream");

						auto state = std::make_shared<FakeCanvasState>();
						state->type = type;
						state->options = options;
						canvases.push_back(state);
						return std::make_unique<FakeCanvas>(state);
					}

					std::vector<Engine::EngineFormat> FakeDocumentFiltersEngine::getSupportedFormats()
					{
						calls.push_back("getSupportedFormats()");

						if (catalogFailure.has_value())
							throw engineFailure(*catalogFailure, "catalogue unavailable");

						return formats;
					}

					std::vector<Engine::EngineOption> FakeDocumentFiltersEngine::getAvailableOptions()
					{
						calls.push_back("getAvailableOptions()");

						if (catalogFailure.has_value())
							throw engineFailure(*catalogFailure, "catalogue unavailable");

						return options;
					}

					std::string FakeDocumentFiltersEngine::describeVersion() { return "0.0.0-fake"; }
				}
			}
		}
	}
}
