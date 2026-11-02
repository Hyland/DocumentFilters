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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_BATCHRUNNER_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_BATCHRUNNER_H

#include "Configuration/SharedOptions.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"
#include "Runtime/IConsole.h"
#include "Runtime/Interruption.h"

#include <functional>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// The per-file loop, in one place, so that <c>--on-error</c> behaves
				/// identically on every command.
				/// </summary>
				class BatchRunner
				{
				public:
					explicit BatchRunner(const Configuration::BatchOptions& options, IConsole& console, Interruption* interruption = nullptr)
						: _options(options), _console(console), _interruption(interruption)
					{
					}

					/// <summary>
					/// Runs <paramref name="action"/> for each item.
					/// </summary>
					/// <remarks>
					/// Under <c>--on-error skip</c> a failure is reported and the loop
					/// continues, but the run still ends non-zero. Under
					/// <c>--on-error stop</c> the first failure ends the loop. Errors that
					/// are not per-document --- a usage error, a limit being exceeded ---
					/// propagate rather than being counted.
					///
					/// <b>Under <c>--combine</c> this still runs per input.</b> Skipping a
					/// failed document leaves the shared canvas open and carries on, so the
					/// run produces one document containing everything that converted, and
					/// exits 1. A partial output is the point rather than an accident: exit
					/// 1 means "some inputs failed", not "nothing was written".
					/// </remarks>
					/// <returns>A value from <see cref="ExitCode"/>: success, or partial failure.</returns>
					template <typename T>
					int run(const std::vector<T>& items, const std::function<void(const T&)>& action)
					{
						_successCount = 0;
						_failureCount = 0;

						for (const T& item : items)
						{
							// Between documents, because that is the only point in a batch
							// where stopping leaves nothing half-written.
							if (_interruption != nullptr && _interruption->requested())
								throw InterruptedError();

							try
							{
								action(item);
								_successCount++;
							}
							catch (const DocumentError& error)
							{
								_failureCount++;
								// DocumentError always carries a path --- its constructor
								// requires one --- so this is always the per-document shape.
								_console.writeError(*error.path() + ": " + error.code() + ": " + error.what());

								if (_options.onError == Configuration::OnErrorBehaviour::Stop)
									break;
							}
						}

						return _failureCount > 0 ? ExitCode::partialFailure : ExitCode::success;
					}

					/// <summary>How many items failed in the last run.</summary>
					int failureCount() const { return _failureCount; }

					/// <summary>How many items succeeded in the last run.</summary>
					int successCount() const { return _successCount; }

				private:
					const Configuration::BatchOptions& _options;
					IConsole& _console;
					Interruption* _interruption;

					int _failureCount = 0;
					int _successCount = 0;
				};
			}
		}
	}
}

#endif
