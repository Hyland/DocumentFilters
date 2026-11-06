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

#include "Execution/DocumentFiltersSession.h"

#include "Engine/DocumentFiltersEngine.h"
#include "Engine/EngineException.h"
#include "Execution/EngineLog.h"
#include "Execution/PasswordProvider.h"
#include "Runtime/CliError.h"
#include "Runtime/IFileSystem.h"

#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <climits>
#else
#include <climits>
#include <unistd.h>
#endif

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
					std::optional<std::string> environmentVariable(const char* name)
					{
#if defined(_WIN32)
						// Wide, and converted here: the value is handed on as UTF-8, so reading it
						// through the active code page corrupted a DF_PATH the code page cannot
						// spell --- the one way the library's location is documented to be given.
						// FilePath does the widening for the variable's name as well; it holds a
						// string without altering it, and the name is ASCII either way.
						wchar_t buffer[32768];
						DWORD length = GetEnvironmentVariableW(Runtime::FilePath::of(name).c_str(), buffer, static_cast<DWORD>(std::size(buffer)));
						if (length == 0 || length >= std::size(buffer))
							return std::nullopt;
						return Runtime::FilePath::toUtf8(std::wstring(buffer, length));
#else
						const char* value = std::getenv(name);
						return value != nullptr ? std::optional<std::string>(value) : std::nullopt;
#endif
					}

					/// <summary>
					/// The directory this tool's own executable lives in --- never the
					/// process's current directory. See <see cref="DocumentFiltersSession::resolveLibraryPath"/>.
					/// </summary>
					std::string currentExecutableDirectory()
					{
						std::string path;
#if defined(_WIN32)
						// Wide, and grown until it fits: the ANSI call mangles an install path the
						// active code page cannot spell, and MAX_PATH is not a limit on Windows ---
						// a long path truncates, and the default library directory then points at
						// nothing.
						std::vector<wchar_t> buffer(MAX_PATH);
						DWORD length = 0;
						while (true)
						{
							length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
							if (length == 0)
								return ".";

							if (length < buffer.size())
								break;

							buffer.resize(buffer.size() * 2);
						}

						path = Runtime::FilePath::toUtf8(std::wstring(buffer.data(), length));
#elif defined(__APPLE__)
						char buffer[PATH_MAX];
						uint32_t size = sizeof(buffer);
						if (_NSGetExecutablePath(buffer, &size) != 0)
							return ".";
						path.assign(buffer);
#else
						char buffer[PATH_MAX];
						ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
						if (length <= 0)
							return ".";
						path.assign(buffer, static_cast<std::size_t>(length));
#endif
						std::size_t slash = path.find_last_of("/\\");
						return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
					}
				}

				DocumentFiltersSession DocumentFiltersSession::forEngine(Engine::IDocumentFiltersEngine& engine)
				{
					return DocumentFiltersSession(engine, nullptr);
				}

				DocumentFiltersSession DocumentFiltersSession::create(
					const Configuration::LibraryOptions& options, const std::optional<std::string>& tempDir)
				{
					std::string licenseKey = options.licenseKey.has_value() ? *options.licenseKey : environmentVariable("DF_LICENSE_KEY").value_or(std::string());
					std::string libraryPath = resolveLibraryPath(options, environmentVariable("DF_PATH"), currentExecutableDirectory());

					auto engine = std::make_unique<Engine::DocumentFiltersEngine>();
					try
					{
						engine->initialize(licenseKey, libraryPath);
					}
					catch (const Engine::EngineException& error)
					{
						throw Runtime::InitialisationError(std::string("could not initialise Document Filters: ") + error.what());
					}

					// --temp-dir, after initialize because it is a setting on an
					// initialised library rather than an argument to starting one.
					if (tempDir.has_value() && !tempDir->empty())
						engine->setTempPath(*tempDir);

					Engine::IDocumentFiltersEngine& reference = *engine;
					return DocumentFiltersSession(reference, std::move(engine));
				}

				std::string DocumentFiltersSession::resolveLibraryPath(
					const Configuration::LibraryOptions& options, const std::optional<std::string>& environmentValue, const std::string& baseDirectory)
				{
					if (options.libraryPath.has_value())
						return *options.libraryPath;
					if (environmentValue.has_value())
						return *environmentValue;
					return baseDirectory;
				}

				std::unique_ptr<Engine::IDocument> DocumentFiltersSession::openDocument(const std::string& path,
					const ExtractorOpenSettings& open, PasswordProvider& passwords, const std::function<bool()>& shouldCancel, const EngineLog* log)
				{
					auto password = [&passwords, path]() { return passwords.getPassword(path); };

					std::function<int(const std::string&)> logLevelFor;
					std::function<void(int, const std::string&, const std::string&)> onLogMessage;
					if (log != nullptr)
					{
						logLevelFor = [log](const std::string& module) { return log->levelFor(module); };
						onLogMessage = [log](int level, const std::string& module, const std::string& message) { log->write(level, module, message); };
					}

					return _engine.openDocument(path, open.flags, open.options, password, shouldCancel, logLevelFor, onLogMessage);
				}

				std::unique_ptr<Engine::IDocument> DocumentFiltersSession::openDocument(std::istream& source, const std::string& displayName,
					const ExtractorOpenSettings& open, PasswordProvider& passwords, const std::function<bool()>& shouldCancel, const EngineLog* log)
				{
					auto password = [&passwords, displayName]() { return passwords.getPassword(displayName); };

					std::function<int(const std::string&)> logLevelFor;
					std::function<void(int, const std::string&, const std::string&)> onLogMessage;
					if (log != nullptr)
					{
						logLevelFor = [log](const std::string& module) { return log->levelFor(module); };
						onLogMessage = [log](int level, const std::string& module, const std::string& message) { log->write(level, module, message); };
					}

					return _engine.openDocument(source, displayName, open.flags, open.options, password, shouldCancel, logLevelFor, onLogMessage);
				}

				DocumentFiltersSession::~DocumentFiltersSession() = default;
			}
		}
	}
}
