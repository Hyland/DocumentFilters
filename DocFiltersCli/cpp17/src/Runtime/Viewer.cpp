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

#include "Runtime/Viewer.h"

#include "Runtime/IFileSystem.h"

#include <stdexcept>

#if defined(_WIN32)
#include <windows.h>

#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#else
#include <spawn.h>
#include <sys/wait.h>

#include <cerrno>
#include <cstring>

extern char** environ;
#endif

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				namespace
				{
#if !defined(_WIN32)
					/// <summary>
					/// Runs <paramref name="launcher"/> with <paramref name="path"/> as its one
					/// argument, without a shell.
					/// </summary>
					/// <remarks>
					/// <c>std::system</c> was the obvious way to write this and the wrong one:
					/// it hands the string to <c>/bin/sh</c>, so every shell metacharacter in
					/// the path is live. Double quotes stop none of the ones that matter ---
					/// <c>$(...)</c>, backticks and <c>\</c> all still expand inside them, and
					/// an embedded <c>"</c> closes the quoting outright. Output names come from
					/// input names, so a file called <c>x$(cmd).pdf</c> sitting in a walked
					/// directory was enough to run <c>cmd</c>; this is a sample people copy, so
					/// it has no business demonstrating that.
					///
					/// <c>posix_spawnp</c> takes an argv array. There is no string for a shell
					/// to re-read, so there is nothing to quote and nothing to escape.
					/// </remarks>
					void spawnLauncher(const char* launcher, const std::string& path)
					{
						// execv's signature predates const-correctness; the strings outlive the
						// call and neither is modified.
						char* argv[] = { const_cast<char*>(launcher), const_cast<char*>(path.c_str()), nullptr };

						pid_t pid = 0;
						int spawned = ::posix_spawnp(&pid, launcher, nullptr, nullptr, argv, environ);
						if (spawned != 0)
							throw std::runtime_error(std::string(launcher) + " could not be started: " + std::strerror(spawned));

						int status = 0;
						if (::waitpid(pid, &status, 0) < 0)
							throw std::runtime_error(std::string(launcher) + " could not be waited for");

						if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
							throw std::runtime_error(std::string(launcher) + " failed");
					}
#endif

					void start(const std::string& path)
					{
#if defined(_WIN32)
						// The only case that needs the shell: ShellExecute is what makes the file
						// association apply. It takes the path as the verb's target rather than
						// as a command line, so there is nothing to quote here either.
						//
						// Wide, because path is UTF-8 and the ANSI call reads it through the active
						// code page: --view could not open a name the run had just written, now
						// that output names survive whatever the code page is.
						std::wstring wide = FilePath::of(path).wstring();
						HINSTANCE result = ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
						if (reinterpret_cast<INT_PTR>(result) <= 32)
							throw std::runtime_error("ShellExecute failed");
#elif defined(__APPLE__)
						spawnLauncher("open", path);
#else
						spawnLauncher("xdg-open", path);
#endif
					}
				}

				namespace Viewer
				{
					void open(const std::optional<std::string>& path, IConsole& console)
					{
						if (!path.has_value() || path->empty())
							return;

						try
						{
							start(*path);
						}
						catch (const std::exception& error)
						{
							// Not a DocumentError: nothing about the document failed, and the
							// output is on disk either way.
							console.writeError("--view could not open " + *path + ": " + error.what());
						}
					}
				}
			}
		}
	}
}
