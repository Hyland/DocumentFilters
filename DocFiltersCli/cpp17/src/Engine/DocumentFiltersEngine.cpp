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

#include "DocumentFiltersEngine.h"

#include "EngineException.h"
#include "Runtime/IFileSystem.h"

#include <cstring>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>

#include <winver.h>
#pragma comment(lib, "version.lib")
#endif

// ---------------------------------------------------------------------------
//  IGR_Make_DocPack / IGR_Make_DocPack_On are not declared by the shipped
//  C++17 binding at all --- confirmed absent from both DocumentFilters.h and
//  DocumentFiltersObjects.h. They are genuinely exported by ISYS11df (the
//  .NET binding calls them, DllImport-style, against the same native
//  library), so redeclaring them here with the native ABI and linking
//  against the same import library that already ships with this binding
//  reaches them --- the one place this file goes around the C++ wrapper
//  rather than through it, the same way DocumentFiltersEngine.cs's
//  IGR_Get_Memory_Status redeclaration goes around a binding defect. Here
//  the binding has no declaration at all to go around, rather than a broken
//  one.
// ---------------------------------------------------------------------------
extern "C"
{
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_DocPack(IGR_LONG handle, const IGR_UCS2* dest_filename, const IGR_UCS2* options,
		Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Make_DocPack_On(
		IGR_LONG handle, IGR_Writable_Stream* dest_stream, const IGR_UCS2* options, Error_Control_Block* error);
}

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Engine
			{
				namespace
				{
					namespace DF = Hyland::DocFilters;

					/// <summary>
					/// Turns anything the binding throws into an <see cref="EngineException"/>.
					/// </summary>
					/// <remarks>
					/// One place, so no caller above the seam ever catches
					/// <c>DocumentFilters::Error</c> and no binding type reaches the
					/// conversion paths.
					/// </remarks>
					template <typename Func>
					auto engineCall(Func&& call) -> decltype(call())
					{
						try
						{
							return call();
						}
						catch (const DF::DocumentFilters::Error& error)
						{
							int code = static_cast<int>(error.code());
							throw EngineException(EngineException::classify(code), code, error.what(), std::make_exception_ptr(error));
						}
						catch (const EngineException&)
						{
							// Already translated -- a nested call inside this one.
							throw;
						}
						catch (const std::exception& error)
						{
							throw EngineException(EngineError::error, static_cast<int>(EngineError::error), error.what(),
								std::make_exception_ptr(error));
						}
					}

					/// <summary>
					/// A write-only <c>IGR_Writable_Stream</c> over a <c>std::ostream</c>.
					/// </summary>
					/// <remarks>
					/// Needed because the C++17 binding's convenience stream bridges
					/// (<c>Stream::bridge_iostream</c> and the <c>MakeOutputCanvas</c>
					/// overloads that take a <c>std::iostream&amp;</c>) all assume a
					/// bidirectional stream --- there is no bridge for a write-only
					/// <c>std::ostream</c> such as standard output, unlike the managed
					/// binding's <c>System.IO.Stream</c>, which this tool's C# reference
					/// hands to <c>MakeOutputCanvas</c> directly. Every canvas write ends up
					/// here regardless of format, because <c>IGR_Writable_Stream*</c> is the
					/// one canvas-construction overload that asks for nothing but a write
					/// callback.
					///
					/// <c>IGR_Writable_Stream</c> is a plain C struct with <c>base</c> ---
					/// an <c>IGR_Stream</c> --- as its first member, so a pointer to this
					/// type is safely reinterpretable as a pointer to either.
					/// </remarks>
					struct WritableStreamBridge : IGR_Writable_Stream
					{
						std::ostream* target;

						explicit WritableStreamBridge(std::ostream& stream) : target(&stream)
						{
							base.Open = &openImpl;
							base.Read = &readImpl;
							base.Seek = &seekImpl;
							base.Close = &closeImpl;
							Write = &writeImpl;
						}

						static ULONG CALLBACK openImpl(IGR_Stream*) { return IGR_OK; }

						static ULONG CALLBACK readImpl(IGR_Stream*, void*, ULONG) { return 0; }

						static IGR_LONGLONG CALLBACK seekImpl(IGR_Stream* stream, IGR_LONGLONG offset, ULONG whence)
						{
							auto* self = reinterpret_cast<WritableStreamBridge*>(stream);

							std::ios_base::seekdir direction =
								whence == 1 ? std::ios_base::cur : (whence == 2 ? std::ios_base::end : std::ios_base::beg);
							self->target->seekp(static_cast<std::streamoff>(offset), direction);
							return static_cast<IGR_LONGLONG>(self->target->tellp());
						}

						static void CALLBACK closeImpl(IGR_Stream*) {}

						static ULONG CALLBACK writeImpl(IGR_Writable_Stream* stream, void* buffer, ULONG bufferSize)
						{
							auto* self = static_cast<WritableStreamBridge*>(stream);
							self->target->write(static_cast<const char*>(buffer), static_cast<std::streamsize>(bufferSize));
							return self->target->good() ? bufferSize : 0;
						}
					};

					void writeDocPack(IGR_HDOC handle, const std::u16string& options, const std::string& filename)
					{
						std::u16string wideFilename = DF::u8_to_u16(filename);
						Error_Control_Block error{};

						IGR_RETURN_CODE code = ::IGR_Make_DocPack(static_cast<IGR_LONG>(handle),
							reinterpret_cast<const IGR_UCS2*>(wideFilename.c_str()), reinterpret_cast<const IGR_UCS2*>(options.c_str()),
							&error);

						if (code != IGR_OK)
							throw DF::DocumentFilters::Error(code, "IGR_Make_DocPack failed");
					}

					void writeDocPack(IGR_HDOC handle, const std::u16string& options, std::ostream& destination)
					{
						WritableStreamBridge bridge(destination);
						Error_Control_Block error{};

						IGR_RETURN_CODE code = ::IGR_Make_DocPack_On(
							static_cast<IGR_LONG>(handle), &bridge, reinterpret_cast<const IGR_UCS2*>(options.c_str()), &error);

						if (code != IGR_OK)
							throw DF::DocumentFilters::Error(code, "IGR_Make_DocPack_On failed");
					}

					std::optional<EngineBookmark> convert(const DF::Bookmark& bookmark)
					{
						if (!bookmark.ok())
							return std::nullopt;

						EngineBookmark result;
						result.title = DF::w_to_u8(bookmark.getTitle());
						result.level = static_cast<int>(bookmark.getLevel());
						result.pageIndex = static_cast<int>(bookmark.getPageIndex());

						for (const DF::Bookmark& child : bookmark.children())
						{
							std::optional<EngineBookmark> converted = convert(child);
							if (converted.has_value())
								result.children.push_back(std::move(*converted));
						}

						return result;
					}

					std::optional<EnginePageElement> convert(const DF::PageElement& element)
					{
						if (!element.ok())
							return std::nullopt;

						EnginePageElement result;
						result.type = element.getType();
						result.depth = static_cast<int>(element.getDepth());

						IGR_FRect bounds = element.getBounds();
						result.bounds = EngineRectF{ bounds.left, bounds.top, bounds.right, bounds.bottom };

						for (const DF::PageElement& child : element.children())
						{
							std::optional<EnginePageElement> converted = convert(child);
							if (converted.has_value())
								result.children.push_back(std::move(*converted));
						}

						return result;
					}

					EngineFileType identify(DF::Extractor& extractor)
					{
						EngineFileType result;
						result.id = static_cast<int>(extractor.getFileType());
						result.mimeType = DF::w_to_u8(extractor.getFileType(DF::Format::What::MimeType));
						result.name = DF::w_to_u8(extractor.getFileType(DF::Format::What::LongName));
						result.capabilities = static_cast<int>(extractor.getFileCapabilities());
						return result;
					}

					/// <summary>
					/// Wires this tool's plain callbacks onto <c>Extractor</c>'s typed
					/// setters.
					/// </summary>
					/// <remarks>
					/// The C++17 binding has no single "open callback with an action code"
					/// the way the managed binding does; it decomposes that into one typed
					/// <c>std::function</c> setter per action instead, so there is no
					/// <c>request.Action</c> switch to translate here at all --- each of
					/// this tool's own callbacks maps onto exactly one setter.
					/// </remarks>
					void wireOpenCallbacks(DF::Extractor& extractor, const std::function<std::optional<std::string>()>& password,
						const std::function<bool()>& shouldCancel, const std::function<int(const std::string&)>& logLevelFor,
						const std::function<void(int, const std::string&, const std::string&)>& onLogMessage)
					{
						if (password)
						{
							extractor.setPasswordCallback([password](const std::wstring&) -> std::wstring
								{
									std::optional<std::string> candidate = password();
									if (!candidate.has_value())
										throw DF::DocumentFilters::Error(IGR_E_PASSWORD, "password refused");

									return DF::u8_to_w(*candidate);
								});
						}

						if (shouldCancel)
						{
							extractor.setHeartbeatCallback([shouldCancel]() -> int { return shouldCancel() ? IGR_CANCELLED : IGR_OK; });
						}

						if (logLevelFor)
						{
							extractor.setLogLevelCallback([logLevelFor](const std::string& module) -> IGR_LOG_LEVEL_TYPE
								{ return static_cast<IGR_LOG_LEVEL_TYPE>(logLevelFor(module)); });
						}

						if (onLogMessage)
						{
							extractor.setLogMessageCallback(
								[onLogMessage](IGR_LOG_LEVEL_TYPE level, const std::string& module, const std::string& message)
								{ onLogMessage(static_cast<int>(level), module, message); });
						}
					}

#ifdef _WIN32
					/// <summary>
					/// Reads <c>ISYS11df</c>'s own file version resource, the way
					/// doc2text's <c>showVersion</c> reports every native component's
					/// version --- by reading each DLL's file version directly, not
					/// through any Document Filters call.
					/// </summary>
					/// <remarks>
					/// This was once the divergence from the C# reference, which reported
					/// its own binding assembly's version under the engine's label; that
					/// turned out to be a defect there and is fixed, so both now answer
					/// with the native library's version or with "unknown".
					///
					/// The mechanism still differs deliberately. C# asks which module the
					/// process actually loaded; this passes a bare filename to
					/// <c>GetFileVersionInfoSize</c>, which resolves it the way
					/// <c>LoadLibrary</c> would and so finds the same file. Enumerating
					/// loaded modules portably is more machinery than the answer is
					/// worth, and the two agree wherever a version exists to read.
					///
					/// A file version is a PE resource, so the non-Windows build below
					/// reports "unknown" --- there is nothing to read from an ELF or
					/// Mach-O image. <c>end2end.py</c>'s version check skips rather than
					/// fails on those platforms for the same reason.
					/// </remarks>
					std::string readNativeLibraryVersion(const std::string& libraryPath)
					{
						std::string path = libraryPath.empty() ? std::string("ISYS11df.dll") : libraryPath + "\\ISYS11df.dll";

						DWORD handle = 0;
						// The wide calls, because libraryPath is UTF-8: the ANSI ones would read
						// the name through the active code page and report "unknown" for a
						// library the engine had just loaded successfully from a --library-path
						// the code page cannot spell.
						std::wstring wide = Runtime::FilePath::of(path).wstring();

						DWORD size = ::GetFileVersionInfoSizeW(wide.c_str(), &handle);
						if (size == 0)
							return "unknown";

						std::vector<char> buffer(size);
						if (!::GetFileVersionInfoW(wide.c_str(), handle, size, buffer.data()))
							return "unknown";

						VS_FIXEDFILEINFO* info = nullptr;
						UINT infoSize = 0;
						if (!::VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<LPVOID*>(&info), &infoSize) || info == nullptr ||
							info->dwSignature != 0xfeef04bd)
						{
							return "unknown";
						}

						return std::to_string(HIWORD(info->dwFileVersionMS)) + "." + std::to_string(LOWORD(info->dwFileVersionMS)) + "."
							+ std::to_string(HIWORD(info->dwFileVersionLS)) + "." + std::to_string(LOWORD(info->dwFileVersionLS));
					}
#else
					std::string readNativeLibraryVersion(const std::string&) { return "unknown"; }
#endif

					class DocumentAdapter;
					class PageAdapter;

					/// <summary><see cref="ISubFileEntry"/> over <c>Subfile</c>.</summary>
					class SubFileAdapter : public ISubFileEntry
					{
					public:
						explicit SubFileAdapter(DF::Subfile subfile) : _subfile(std::move(subfile)) {}

						std::string id() override { return DF::w_to_u8(_subfile.getId()); }

						std::string name() override { return DF::w_to_u8(_subfile.getName()); }

						long long size() override { return static_cast<long long>(_subfile.getSize()); }

						std::optional<std::chrono::system_clock::time_point> modified() override
						{
							return engineCall([&]() -> std::optional<std::chrono::system_clock::time_point>
								{
									DF::DateTime value = _subfile.getFileDate();
									return value.empty() ? std::nullopt : std::optional(value.ToTimePoint());
								});
						}

						std::unique_ptr<IDocument> open(int flags, const std::string& options) override;

						void copyTo(const std::string& path) override
						{
							engineCall([&] { _subfile.CopyTo(DF::u8_to_w(path)); });
						}

					private:
						DF::Subfile _subfile;
					};

					std::vector<std::unique_ptr<ISubFileEntry>> drain(const DF::Extractor::subfiles_t& entries)
					{
						std::vector<std::unique_ptr<ISubFileEntry>> result;
						for (const DF::Subfile& entry : entries)
							result.push_back(std::make_unique<SubFileAdapter>(entry));
						return result;
					}

					/// <summary><see cref="IDocument"/> over <c>Extractor</c>.</summary>
					class DocumentAdapter : public IDocument
					{
					public:
						/// <param name="extractor">The handle to wrap.</param>
						/// <param name="ownsHandle">
						/// False when the handle belongs to something else --- a subfile,
						/// which the binding models as an <c>Extractor</c> in its own right,
						/// so opening it yields a document that shares the entry's handle
						/// and must not close it.
						/// </param>
						explicit DocumentAdapter(DF::Extractor extractor, bool ownsHandle = true)
							: _extractor(std::move(extractor)), _ownsHandle(ownsHandle)
						{
						}

						~DocumentAdapter() override
						{
							if (_ownsHandle)
								_extractor.Close();
						}

						int getFileType() override
						{
							return engineCall([&] { return static_cast<int>(_extractor.getFileType()); });
						}

						std::string getMimeType() override
						{
							return engineCall([&] { return DF::w_to_u8(_extractor.getFileType(DF::Format::What::MimeType)); });
						}

						std::string getFormatName() override
						{
							return engineCall([&] { return DF::w_to_u8(_extractor.getFileType(DF::Format::What::LongName)); });
						}

						int getFileCapabilities() override
						{
							return engineCall([&] { return static_cast<int>(_extractor.getFileCapabilities()); });
						}

						bool getEOF() override
						{
							return engineCall([&] { return _extractor.getEOF(); });
						}

						std::string getText(int maxLength) override
						{
							return engineCall([&] { return DF::w_to_u8(_extractor.getText(static_cast<size_t>(maxLength))); });
						}

						int getPageCount() override
						{
							return engineCall([&] { return static_cast<int>(_extractor.getPageCount()); });
						}

						std::unique_ptr<IDocumentPage> getPage(int pageIndex) override;

						std::vector<std::unique_ptr<ISubFileEntry>> subFiles() override
						{
							return engineCall([&] { return drain(_extractor.subfiles()); });
						}

						std::vector<std::unique_ptr<ISubFileEntry>> images() override
						{
							return engineCall([&] { return drain(_extractor.images()); });
						}

						std::optional<EngineBookmark> rootBookmark() override
						{
							return engineCall([&] { return convert(_extractor.getRootBookmark()); });
						}

						void makeDocPack(const std::string& filename, const std::string& options) override
						{
							engineCall([&] { writeDocPack(_extractor.getHandle(), DF::u8_to_u16(options), filename); });
						}

						void makeDocPack(std::ostream& destination, const std::string& options) override
						{
							engineCall([&] { writeDocPack(_extractor.getHandle(), DF::u8_to_u16(options), destination); });
						}

						DF::Extractor& inner() { return _extractor; }

					private:
						DF::Extractor _extractor;
						bool _ownsHandle;
					};

					/// <summary><see cref="IDocumentPage"/> over <c>Page</c>.</summary>
					class PageAdapter : public IDocumentPage
					{
					public:
						explicit PageAdapter(DF::Page page) : _page(std::move(page)) {}

						int getWidth() override
						{
							return engineCall([&] { return static_cast<int>(_page.getWidth()); });
						}

						int getHeight() override
						{
							return engineCall([&] { return static_cast<int>(_page.getHeight()); });
						}

						std::string getAttribute(const std::string& name) override
						{
							return engineCall([&] { return DF::w_to_u8(_page.getAttribute(DF::u8_to_w(name))); });
						}

						std::vector<EngineHyperlink> hyperlinks() override
						{
							return engineCall([&]
								{
									std::vector<EngineHyperlink> result;
									for (const DF::Hyperlink& link : _page.hyperlinks())
									{
										EngineHyperlink converted;
										converted.type = link.get_type();
										converted.uri = DF::w_to_u8(link.getUri());
										converted.ref = DF::w_to_u8(link.getRef());
										converted.x = link.getX();
										converted.y = link.getY();
										converted.width = link.getWidth();
										converted.height = link.getHeight();
										result.push_back(std::move(converted));
									}
									return result;
								});
						}

						std::vector<EngineFormElement> formElements() override
						{
							return engineCall([&]
								{
									std::vector<EngineFormElement> result;
									for (const DF::FormElement& element : _page.formElements())
									{
										EngineFormElement converted;
										converted.type = element.get_type();
										converted.name = DF::w_to_u8(element.getName());
										converted.x = element.getX();
										converted.y = element.getY();
										converted.width = element.getWidth();
										converted.height = element.getHeight();
										result.push_back(std::move(converted));
									}
									return result;
								});
						}

						std::vector<EngineAnnotation> annotations() override
						{
							return engineCall([&]
								{
									std::vector<EngineAnnotation> result;
									for (const std::shared_ptr<DF::Annotation>& annotation : _page.annotations())
									{
										EngineAnnotation converted;
										converted.type = annotation->getType();
										converted.title = DF::w_to_u8(annotation->getTitle());
										converted.text = DF::w_to_u8(annotation->getText());
										result.push_back(std::move(converted));
									}
									return result;
								});
						}

						std::optional<EnginePageElement> getRootPageElement() override
						{
							return engineCall([&] { return convert(_page.getRootPageElement()); });
						}

						DF::Page& inner() { return _page; }

					private:
						DF::Page _page;
					};

					std::unique_ptr<IDocumentPage> DocumentAdapter::getPage(int pageIndex)
					{
						return engineCall([&]() -> std::unique_ptr<IDocumentPage>
							{ return std::make_unique<PageAdapter>(_extractor.getPage(static_cast<size_t>(pageIndex))); });
					}

					std::unique_ptr<IDocument> SubFileAdapter::open(int flags, const std::string& options)
					{
						return engineCall([&]() -> std::unique_ptr<IDocument>
							{
								_subfile.Open(static_cast<uint32_t>(flags), DF::u8_to_w(options), nullptr);
								// Shares this entry's handle rather than opening a second one,
								// so it must not be the one that closes it.
								return std::make_unique<DocumentAdapter>(_subfile, false);
							});
					}

					/// <summary>A real canvas was handed something that did not come from the real engine.</summary>
					EngineException mismatch(const std::string& what)
					{
						return EngineException(EngineError::invalidParameter, static_cast<int>(EngineError::invalidParameter),
							"this canvas can only render a " + what + " that came from the same engine");
					}

					/// <summary><see cref="IOutputCanvas"/> over <c>Canvas</c>.</summary>
					class CanvasAdapter : public IOutputCanvas
					{
					public:
						explicit CanvasAdapter(DF::Canvas canvas, std::unique_ptr<WritableStreamBridge> streamBridge = nullptr)
							: _canvas(std::move(canvas)), _streamBridge(std::move(streamBridge))
						{
						}

						~CanvasAdapter() override { _canvas.Close(); }

						void renderPage(
							IDocumentPage& page, const std::string& options, const std::optional<EnginePageDecorations>& decorations) override
						{
							auto* adapter = dynamic_cast<PageAdapter*>(&page);
							if (adapter == nullptr)
								throw mismatch("page");

							engineCall([&]
								{
									if (!decorations.has_value() || decorations->isEmpty())
									{
										_canvas.RenderPage(adapter->inner(), DF::u8_to_w(options));
										return;
									}

									DF::RenderPageProperties properties;
									for (const EngineRedaction& redaction : decorations->redactions)
									{
										properties.AddRedaction(
											redaction.x, redaction.y, redaction.x + redaction.width, redaction.y + redaction.height);
									}

									for (const EngineFormValue& value : decorations->formValues)
										properties.AddFormValue(DF::u8_to_w(value.name), DF::u8_to_w(value.value), false);

									_canvas.RenderPage(adapter->inner(), DF::u8_to_w(options), properties);
								});
						}

						void renderPages(IDocument& document) override
						{
							auto* adapter = dynamic_cast<DocumentAdapter*>(&document);
							if (adapter == nullptr)
								throw mismatch("document");

							engineCall([&] { _canvas.RenderPages(adapter->inner()); });
						}

						void drawImage(int x, int y, const std::vector<std::uint8_t>& image, const std::string& options) override
						{
							engineCall([&] { _canvas.DrawImage(x, y, image.data(), image.size(), DF::u8_to_w(options)); });
						}

					private:
						DF::Canvas _canvas;

						/// <summary>
						/// Kept alive alongside the canvas when it was constructed over a
						/// stream: the canvas only holds a raw <c>IGR_Writable_Stream*</c>
						/// into this, never taking ownership of it.
						/// </summary>
						std::unique_ptr<WritableStreamBridge> _streamBridge;
					};
				}

				DocumentFiltersEngine::DocumentFiltersEngine() = default;

				DocumentFiltersEngine::~DocumentFiltersEngine() = default;

				void DocumentFiltersEngine::initialize(const std::string& licenseKey, const std::string& libraryPath)
				{
					engineCall([&] { _api.Initialize(licenseKey, libraryPath); });
					_libraryPath = libraryPath;
				}

				std::unique_ptr<IDocument> DocumentFiltersEngine::openDocument(const std::string& path, int flags,
					const std::string& options, const std::function<std::optional<std::string>()>& password,
					const std::function<bool()>& shouldCancel, const std::function<int(const std::string&)>& logLevelFor,
					const std::function<void(int, const std::string&, const std::string&)>& onLogMessage)
				{
					return engineCall([&]() -> std::unique_ptr<IDocument>
						{
							DF::Extractor extractor = _api.GetExtractor(path);
							wireOpenCallbacks(extractor, password, shouldCancel, logLevelFor, onLogMessage);
							extractor.Open(static_cast<uint32_t>(flags), DF::u8_to_w(options), nullptr);
							return std::make_unique<DocumentAdapter>(std::move(extractor), true);
						});
				}

				std::unique_ptr<IDocument> DocumentFiltersEngine::openDocument(std::istream& source, const std::string&, int flags,
					const std::string& options, const std::function<std::optional<std::string>()>& password,
					const std::function<bool()>& shouldCancel, const std::function<int(const std::string&)>& logLevelFor,
					const std::function<void(int, const std::string&, const std::string&)>& onLogMessage)
				{
					return engineCall([&]() -> std::unique_ptr<IDocument>
						{
							DF::Extractor extractor = _api.GetExtractor(source);
							wireOpenCallbacks(extractor, password, shouldCancel, logLevelFor, onLogMessage);
							extractor.Open(static_cast<uint32_t>(flags), DF::u8_to_w(options), nullptr);
							return std::make_unique<DocumentAdapter>(std::move(extractor), true);
						});
				}

				void DocumentFiltersEngine::setTempPath(const std::string& path)
				{
					// IGR_Multiplex's own parameters are pointer-sized (size_t*), unlike
					// the managed binding's uint-typed P/Invoke declaration that
					// DocumentFiltersEngine.cs works around by truncating a pointer into
					// one --- there is nothing to work around here.
					//
					// The address is handed to the engine as ANSI text, matching the
					// managed adapter's own Marshal.StringToHGlobalAnsi: a path outside
					// the current code page is an existing limitation of this specific
					// call, not one introduced here.
					engineCall([&]
						{
							Error_Control_Block error{};
							size_t address = reinterpret_cast<size_t>(path.c_str());
							size_t unused = 0;
							::IGR_Multiplex(IGR_Multi_Set_Temp_Path, &address, &unused, &error);
						});
				}

				std::string DocumentFiltersEngine::getMemoryStatus()
				{
					// Declared directly on DocumentFilters in the managed binding this
					// project ports from, but only reachable there via a hand redeclared
					// P/Invoke: its own wrapper marshals the length as SysUInt, which is
					// not a legal pairing for a plain uint and throws on every call. The
					// C++17 binding does not wrap this call at all, wrapped or not, so the
					// raw ISYS11df entry point --- already visible through
					// DocumentFiltersObjects.h's include of the C header, and already
					// linked --- is called directly; nothing here needs working around.
					return engineCall([&]
						{
							std::vector<char> buffer(4096, '\0');
							size_t written = ::IGR_Get_Memory_Status(buffer.data(), buffer.size());
							return written == 0 ? std::string() : std::string(buffer.data(), written);
						});
				}

				EngineFileType DocumentFiltersEngine::identifyFile(const std::string& path)
				{
					return engineCall([&]
						{
							DF::Extractor extractor = _api.GetExtractor(path);
							return identify(extractor);
						});
				}

				EngineFileType DocumentFiltersEngine::identifyFile(std::istream& source, const std::string&)
				{
					return engineCall([&]
						{
							DF::Extractor extractor = _api.GetExtractor(source);
							return identify(extractor);
						});
				}

				std::unique_ptr<IOutputCanvas> DocumentFiltersEngine::makeCanvas(
					const std::string& target, Hyland::DocFilters::CanvasType type, const std::string& options)
				{
					return engineCall([&]() -> std::unique_ptr<IOutputCanvas>
						{ return std::make_unique<CanvasAdapter>(_api.MakeOutputCanvas(target, type, DF::u8_to_w(options))); });
				}

				std::unique_ptr<IOutputCanvas> DocumentFiltersEngine::makeCanvas(
					std::ostream& target, Hyland::DocFilters::CanvasType type, const std::string& options)
				{
					return engineCall([&]() -> std::unique_ptr<IOutputCanvas>
						{
							auto bridge = std::make_unique<WritableStreamBridge>(target);
							DF::Canvas canvas = _api.MakeOutputCanvas(bridge.get(), type, DF::u8_to_w(options));
							return std::make_unique<CanvasAdapter>(std::move(canvas), std::move(bridge));
						});
				}

				std::vector<EngineFormat> DocumentFiltersEngine::getSupportedFormats()
				{
					return engineCall([&]
						{
							std::vector<EngineFormat> result;
							for (const DF::Format& format : _api.getSupportedFormats())
							{
								EngineFormat converted;
								converted.id = static_cast<int>(format.id());
								converted.shortName = DF::w_to_u8(format.getShortName());
								converted.displayName = DF::w_to_u8(format.getDisplayName());
								result.push_back(std::move(converted));
							}
							return result;
						});
				}

				std::vector<EngineOption> DocumentFiltersEngine::getAvailableOptions()
				{
					return engineCall([&]
						{
							std::vector<EngineOption> result;
							for (const DF::Option& option : _api.getAvailableOptions())
							{
								EngineOption converted;
								converted.displayName = DF::w_to_u8(option.getDisplayName());
								converted.type = DF::w_to_u8(option.getType());
								converted.description = DF::w_to_u8(option.getDescription());
								converted.flags = static_cast<int>(option.getFlags());
								converted.defaultValue = DF::w_to_u8(option.getDefaultValue());
								result.push_back(std::move(converted));
							}
							return result;
						});
				}

				std::string DocumentFiltersEngine::describeVersion()
				{
					return engineCall([&] { return readNativeLibraryVersion(_libraryPath); });
				}
			}
		}
	}
}
