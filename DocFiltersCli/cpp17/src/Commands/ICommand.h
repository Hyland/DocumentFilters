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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_ICOMMAND_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_ICOMMAND_H

#include "Configuration/SharedOptions.h"
#include "Runtime/IConsole.h"
#include "Runtime/IFileSystem.h"
#include "Runtime/Interruption.h"

#include <chrono>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				// Forward declared rather than included: a command only ever holds a
				// pointer to the session, and the session's own definition belongs to
				// whichever phase implements Execution/.
				class DocumentFiltersSession;
			}

			namespace Commands
			{
				/// <summary>
				/// Everything a command needs that is not its own options. Per-command
				/// options are passed to the command's constructor instead, so there is no
				/// object here holding seven verbs' worth of options with six of them unused.
				/// </summary>
				struct CommandContext
				{
					Configuration::SharedOptions sharedOptions;

					Runtime::IConsole* console = nullptr;

					Runtime::IFileSystem* fileSystem = nullptr;

					/// <summary>
					/// The initialised engine session. Null for commands that do not open
					/// documents, and created lazily so that <c>--print-plan</c> never needs
					/// a license.
					/// </summary>
					Execution::DocumentFiltersSession* session = nullptr;

					/// <summary>
					/// Whether the user has interrupted the run, or null when nothing is
					/// watching.
					/// </summary>
					/// <remarks>
					/// Null for a caller that installed no handler --- every test, and
					/// <c>--dry-run</c>, which finishes long before anyone could press
					/// anything.
					/// </remarks>
					Runtime::Interruption* interruption = nullptr;

					/// <summary>
					/// A monotonic reading for <c>Diagnostics</c>, or empty for the real one.
					/// </summary>
					/// <remarks>
					/// Here rather than inside each command because the commands build their
					/// own <c>Diagnostics</c> and a test driving the whole CLI has no other way
					/// in. It is what lets the limit cases assert on a document that outran
					/// <c>--timeout</c> without the suite waiting out the smallest value the
					/// validator accepts.
					/// </remarks>
					std::function<std::chrono::steady_clock::time_point()> clock;

					/// <summary>
					/// What <c>--view</c> hands the finished output to, or empty for the
					/// desktop's own handler.
					/// </summary>
					/// <remarks>
					/// Here for the same reason <c>clock</c> is: a test driving the whole CLI
					/// has no other way in, and what <c>--view</c> has to get right is not what
					/// opens --- A7 settled that it is the machine's business --- but
					/// <em>when</em>. The output has to be finished before anything is asked to
					/// open it, and under <c>--combine</c> it was not: the canvas and the
					/// writer were still alive, so a PDF whose trailer is written on close was
					/// handed over half-written.
					/// </remarks>
					std::function<void(const std::optional<std::string>&, Runtime::IConsole&)> openViewer;
				};

				/// <summary>One verb.</summary>
				class ICommand
				{
				public:
					virtual ~ICommand() = default;

					/// <summary>The verb as typed, e.g. "convert".</summary>
					virtual std::string name() const = 0;

					/// <summary>True when this command opens documents and therefore needs the engine.</summary>
					virtual bool requiresEngine() const = 0;

					/// <summary>Runs the command.</summary>
					/// <returns>A value from Runtime::ExitCode.</returns>
					virtual int execute(CommandContext& context) const = 0;

					/// <summary>
					/// Runs stages 1 to 3 only and returns the canonical plan text, for
					/// <c>--print-plan</c>. Never opens a document.
					/// </summary>
					virtual std::string describePlan(CommandContext& context) const = 0;
				};

				/// <summary>
				/// What every command's <c>execute()</c> throws until Engine/ and Execution/
				/// exist to give it a real body.
				/// </summary>
				/// <remarks>
				/// Only <c>describePlan()</c> is real in this phase --- see the phase
				/// write-up for why <c>--dry-run</c>/<c>plan-corpus.txt</c> never needed the
				/// engine at all, unlike a real run.
				/// </remarks>
				[[noreturn]] inline void executionNotYetPorted(const std::string& command)
				{
					throw std::logic_error(command + "::execute() is not yet ported (needs Engine/ and Execution/)");
				}
			}
		}
	}
}

#endif
