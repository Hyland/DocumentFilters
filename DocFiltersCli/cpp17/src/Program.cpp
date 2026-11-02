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

#include "Program.h"

#include "Cli/CommandLineApp.h"

#include <algorithm>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Program
			{
				int run(const std::vector<std::string>& args, Runtime::IConsole& console, Runtime::IFileSystem& fileSystem,
					Engine::IDocumentFiltersEngine* engine, Runtime::Interruption* interruption,
					std::function<std::chrono::steady_clock::time_point()> clock,
					std::function<void(const std::optional<std::string>&, Runtime::IConsole&)> openViewer)
				{
					Cli::CommandLineApp app(console, fileSystem, engine, interruption, std::move(clock), std::move(openViewer));

					try
					{
						return app.run(args);
					}
					catch (const Runtime::CliError& error)
					{
						return reportError(error, console);
					}
				}

				Configuration::ConsoleOptions preScanConsoleOptions(const std::vector<std::string>& args)
				{
					auto contains = [&args](const char* token) { return std::find(args.begin(), args.end(), token) != args.end(); };

					Configuration::ConsoleOptions options;
					options.silent = contains("--silent");
					options.verbose = contains("--verbose");
					options.color = contains("--color");
					return options;
				}

				int reportError(const Runtime::CliError& error, Runtime::IConsole& console)
				{
					console.writeError(
						error.path().has_value() ? *error.path() + ": " + error.code() + ": " + error.what() : error.code() + ": " + error.what());
					return error.exitCode();
				}
			}
		}
	}
}
