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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_DIAGNOSTICS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_DIAGNOSTICS_H

#include "Configuration/SharedOptions.h"
#include "Engine/IDocumentFiltersEngine.h"
#include "Runtime/CliError.h"
#include "Runtime/IConsole.h"

#include <chrono>
#include <functional>
#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>
				/// Timing, memory reporting, progress and the run limits.
				/// </summary>
				/// <remarks>
				/// Unlike everything else the tool prints, this output is deliberately
				/// NOT part of the cross-language contract, because elapsed times and
				/// memory figures differ from run to run. The conformance suite ignores
				/// it.
				/// </remarks>
				class Diagnostics
				{
				public:
					/// <param name="engine">
					/// Absent for a run that never opens a document --- <c>--print-plan</c>
					/// --- so <c>-x</c>/<c>--xx</c> have nothing to ask and quietly report
					/// nothing rather than needing a null check at every call site.
					/// </param>
					/// <param name="clock">
					/// A monotonic reading, for tests that need time to pass without waiting
					/// for it.
					/// </param>
					/// <remarks>
					/// <paramref name="clock"/> exists because <c>--timeout</c> is otherwise
					/// only testable by sleeping past it, and the smallest value the validator
					/// accepts is one second --- so the cases covering a document that outran
					/// its limit cost more than the rest of the suite together. A clock the
					/// test advances makes them instant and exact, and removes the flakiness of
					/// asserting on elapsed wall time at all. Every reading goes through it, so
					/// nothing measures time two different ways.
					/// </remarks>
					explicit Diagnostics(const Configuration::DiagnosticOptions& options, Runtime::IConsole& console,
						Engine::IDocumentFiltersEngine* engine = nullptr,
						std::function<std::chrono::steady_clock::time_point()> clock = nullptr)
						: _options(options), _console(console), _engine(engine)
						, _clock(clock ? std::move(clock) : []() { return std::chrono::steady_clock::now(); })
					{
					}

					/// <summary>Marks the start of the run, for <c>--time</c>.</summary>
					void beginRun();

					/// <summary>Marks the start of one document, for <c>--logtime</c> and <c>--timeout</c>.</summary>
					void beginDocument(const std::string& displayName, int index, int total);

					/// <summary>
					/// Ends the document's timing window, emitting its timing if asked and
					/// leaving the elapsed time readable.
					/// </summary>
					/// <remarks>
					/// Stopped rather than discarded. <see cref="checkDocumentTimeout"/>
					/// runs <em>after</em> this at every call site, so clearing the timer
					/// here made its guard always true and <c>--timeout</c> never fired at
					/// all --- a document could run for any length of time and still exit
					/// 0. <see cref="beginDocument"/> replaces the timer for the next
					/// document, so nothing needs clearing here; leaving it running also
					/// means the elapsed time the check reads is the document's own, not
					/// the document plus whatever ran between the two calls.
					/// </remarks>
					void endDocument(const std::string& displayName);

					/// <summary>Emits the run summary for <c>--time</c>, and the closing reports.</summary>
					void endRun(int succeeded, int failed);

					/// <summary>
					/// Checks <c>--max-time</c> and <c>--max-memory</c>.
					/// </summary>
					/// <remarks>
					/// <c>--timeout</c> is per document rather than per run and is checked
					/// separately, at <see cref="endDocument"/>'s caller --- a synchronous
					/// native call cannot be pre-empted mid-flight, so this can only notice
					/// a document that has already finished taking too long, not stop one
					/// early.
					/// </remarks>
					/// <exception cref="Runtime::LimitExceededError">A limit was passed.</exception>
					void checkLimits();

					/// <summary>
					/// Checks the just-finished document against <c>--timeout</c>.
					/// </summary>
					/// <exception cref="Runtime::LimitExceededError">The document took longer than allowed.</exception>
					void checkDocumentTimeout(const std::string& displayName);

					/// <summary>
					/// True once the document being processed has run past <c>--timeout</c>.
					/// </summary>
					/// <remarks>
					/// Polled by the engine's open callback as well as checked after the
					/// document, because those cover different halves and neither covers
					/// both. The callback can stop a document that hangs while the engine
					/// parses it, which is the only point the engine offers; it cannot
					/// touch the page loop, because rendering a page takes no callback.
					/// The check after the document therefore stays --- it is the only
					/// thing that sees rendering time at all, even though by then the work
					/// is already done.
					/// </remarks>
					bool documentDeadlineExceeded() const;

					/// <summary>The error for a document that outran <c>--timeout</c>.</summary>
					/// <remarks>
					/// Built here rather than at each throw site so both routes --- the
					/// cancelled open and the check afterwards --- report the identical
					/// line.
					/// </remarks>
					Runtime::LimitExceededError documentTimedOut(const std::string& displayName) const;

					/// <summary>Waits for a keypress between documents, for <c>--pause</c>.</summary>
					void pauseIfRequested();

				private:
					/// <summary>
					/// Prints the engine's memory report when <c>-x</c> or <c>--xx</c>
					/// asked for it.
					/// </summary>
					/// <remarks>
					/// The two switches are one level: <c>-x</c> is 1 and reports at the
					/// end of the run, <c>--xx</c> is 2 and also reports after each
					/// document, which is how doc2text spells the same pair. The text is
					/// the engine's own and is printed verbatim --- it is diagnostics, not
					/// contract.
					/// </remarks>
					void reportEngineMemory(int atLeast);

					const Configuration::DiagnosticOptions& _options;
					Runtime::IConsole& _console;
					Engine::IDocumentFiltersEngine* _engine;

					std::function<std::chrono::steady_clock::time_point()> _clock;

					std::optional<std::chrono::steady_clock::time_point> _runStart;

					/// <summary>
					/// The current document's start time, set by <see cref="beginDocument"/>
					/// and left in place by <see cref="endDocument"/> --- only the next
					/// <see cref="beginDocument"/> replaces it. Absent before the first
					/// document.
					/// </summary>
					std::optional<std::chrono::steady_clock::time_point> _documentStart;

					/// <summary>
					/// The elapsed time <see cref="endDocument"/> froze it at, once stopped.
					/// Absent while the document is still running, mirroring a live
					/// <c>Stopwatch</c> that has not been stopped yet.
					/// </summary>
					std::optional<std::chrono::steady_clock::duration> _documentElapsed;

					/// <summary>The current elapsed time for the running or just-stopped document.</summary>
					std::chrono::steady_clock::duration currentDocumentElapsed() const;
				};
			}
		}
	}
}

#endif
