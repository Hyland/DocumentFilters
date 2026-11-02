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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_CLI_COMMANDLINEAPP_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_CLI_COMMANDLINEAPP_H

#include "BoundCommandLine.h"
#include "Engine/IDocumentFiltersEngine.h"
#include "Runtime/IConsole.h"
#include "Runtime/IFileSystem.h"
#include "Runtime/Interruption.h"

#include <optional>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Cli
			{
				/// <summary>
				/// Builds the CLI11 command line application from <see cref="ArgumentTable"/>
				/// and dispatches to the chosen command.
				/// </summary>
				/// <remarks>
				/// This is the only class in the project that knows which CLI parsing library
				/// is in use, and the whole of the stage 1 to stage 2 boundary. The Python,
				/// Java and C# ports use an equivalent over argparse, picocli and McMaster;
				/// everything below it is shared logic.
				/// </remarks>
				class CommandLineApp
				{
				public:
					/// <param name="engine">
					/// The Document Filters engine, or nullptr to build the real one on
					/// first use.
					/// </param>
					/// <remarks>
					/// Passed in by hand, like the console and the filesystem, rather
					/// than resolved from anywhere: that is the rule the whole sample
					/// follows, and it is what lets a test drive a real command line
					/// from argv to exit code without a license or the native library.
					/// </remarks>
					explicit CommandLineApp(Runtime::IConsole& console, Runtime::IFileSystem& fileSystem,
						Engine::IDocumentFiltersEngine* engine = nullptr, Runtime::Interruption* interruption = nullptr,
						std::function<std::chrono::steady_clock::time_point()> clock = nullptr,
						std::function<void(const std::optional<std::string>&, Runtime::IConsole&)> openViewer = nullptr)
						: _console(console), _fileSystem(fileSystem), _engine(engine), _interruption(interruption), _clock(std::move(clock)),
							_openViewer(std::move(openViewer))
					{
					}

					/// <summary>
					/// Selects the command and binds every argument on it, without executing
					/// anything.
					/// </summary>
					/// <exception cref="Runtime::UsageError">
					/// Unknown verb or argument, a missing value, or a value that failed to
					/// parse.
					/// </exception>
					BoundCommandLine bind(const std::vector<std::string>& args);

					/// <summary>
					/// Parses <paramref name="args"/> and runs the selected command.
					/// </summary>
					/// <remarks>
					/// Any <c>Runtime::CliError</c> propagates rather than being caught
					/// here: <c>main()</c> is the one place that turns an error into
					/// stderr text and an exit code, via <c>reportError</c> --- and it
					/// needs the console bound to the run's own
					/// <c>--silent</c>/<c>--verbose</c>/<c>--color</c> settings, which
					/// <c>bind</c> has not resolved yet when this method is entered.
					/// </remarks>
					/// <returns>A value from <see cref="Runtime::ExitCode"/>.</returns>
					/// <exception cref="Runtime::CliError">
					/// Unknown verb or argument, a missing value, or anything a command's
					/// <c>execute</c> itself throws.
					/// </exception>
					int run(const std::vector<std::string>& args);

				private:
					Runtime::IConsole& _console;
					Runtime::IFileSystem& _fileSystem;
					Engine::IDocumentFiltersEngine* _engine;
					Runtime::Interruption* _interruption;
					std::function<std::chrono::steady_clock::time_point()> _clock;

					std::function<void(const std::optional<std::string>&, Runtime::IConsole&)> _openViewer;
				};
			}
		}
	}
}

#endif
