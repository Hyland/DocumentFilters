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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_PROGRAM_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_PROGRAM_H

#include "Configuration/SharedOptions.h"
#include "Engine/IDocumentFiltersEngine.h"
#include "Runtime/CliError.h"
#include "Runtime/IConsole.h"
#include "Runtime/IFileSystem.h"
#include "Runtime/Interruption.h"

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			/// <summary>
			/// Entry point. Wires up the console and filesystem, runs the command
			/// line, and turns whatever happens into a single exit code.
			/// </summary>
			namespace Program
			{
				/// <summary>
				/// Everything <c>main()</c> does once its three dependencies exist.
				/// </summary>
				/// <remarks>
				/// Split out so a test can drive a whole command line --- argv in,
				/// exit code and console output out --- without the real console, the
				/// real filesystem or the native library. That path is otherwise
				/// reachable only by starting a process, which needs a licensed
				/// machine to tell a wiring mistake from an engine one.
				///
				/// <paramref name="engine"/> nullptr means the real engine, built on
				/// first use by whichever command needs it.
				/// </remarks>
				int run(const std::vector<std::string>& args, Runtime::IConsole& console, Runtime::IFileSystem& fileSystem,
					Engine::IDocumentFiltersEngine* engine, Runtime::Interruption* interruption = nullptr,
					std::function<std::chrono::steady_clock::time_point()> clock = nullptr,
				std::function<void(const std::optional<std::string>&, Runtime::IConsole&)> openViewer = nullptr);

				/// <summary>
				/// Reads <c>--silent</c>, <c>--verbose</c> and <c>--color</c> directly
				/// out of <paramref name="args"/>, before anything is bound.
				/// </summary>
				/// <remarks>
				/// <c>SystemConsole</c> needs these <em>before</em> the first line is
				/// written, but they only exist on the <c>Configuration::SharedOptions</c>
				/// that <c>CommandLineApp::bind</c> produces, and <c>CommandLineApp::run</c>
				/// starts writing --- <c>--print-contract</c>, <c>--dry-run</c>, the
				/// command itself --- in the same call that does the binding, with no
				/// point in between for this to step in. A raw scan sidesteps the
				/// ordering problem entirely: this tool disables prefix abbreviation
				/// and none of these three takes a value, so <c>--silent</c> has
				/// exactly one spelling, and a plain array search for it is exact, not
				/// a heuristic.
				/// </remarks>
				Configuration::ConsoleOptions preScanConsoleOptions(const std::vector<std::string>& args);

				/// <summary>
				/// Maps an error to its exit code and prints its message to stderr.
				/// </summary>
				/// <remarks>
				/// Error text is part of the cross-language contract, so it is
				/// composed here from <c>CliError::code()</c> and the message rather
				/// than being left to whatever the CLI parsing library would have
				/// said. The run-level shape is code first, e.g.
				/// <c>E_INIT_FAILED: could not initialise Document Filters</c> --- code
				/// first, then the message, with no other punctuation.
				/// </remarks>
				int reportError(const Runtime::CliError& error, Runtime::IConsole& console);
			}
		}
	}
}

#endif
