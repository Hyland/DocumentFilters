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

#include "Execution/Diagnostics.h"
#include "Engine/EngineException.h"
#include "Runtime/Strings.h"

#include <cstdio>

#if defined(_WIN32)
#include <psapi.h>
#include <windows.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/resource.h>
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
					/// <summary>The process's peak resident size, in megabytes.</summary>
					/// <remarks>
					/// Returning zero off Windows was not a harmless gap: <c>--logmem</c>
					/// printed "peak memory: 0MB" and <c>--max-memory</c> could never fire, so
					/// a limit the user asked for was accepted and silently ignored --- the
					/// same shape of defect as an argument nobody reads. The C# reference reads
					/// <c>Process.PeakWorkingSet64</c>, which works on every platform it runs
					/// on, so this was a divergence too.
					///
					/// <c>ru_maxrss</c> is the POSIX equivalent, and its unit is the one thing
					/// to get right: kilobytes on Linux, bytes on macOS.
					/// </remarks>
					long long peakWorkingSetMegabytes()
					{
#if defined(_WIN32)
						PROCESS_MEMORY_COUNTERS counters {};
						if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
							return static_cast<long long>(counters.PeakWorkingSetSize) / (1024 * 1024);
						return 0;
#else
						rusage usage {};
						if (::getrusage(RUSAGE_SELF, &usage) != 0)
							return 0;

#if defined(__APPLE__)
						return static_cast<long long>(usage.ru_maxrss) / (1024 * 1024);
#else
						return static_cast<long long>(usage.ru_maxrss) / 1024;
#endif
#endif
					}

					std::string formatSeconds(double seconds)
					{
						char buffer[64];
						std::snprintf(buffer, sizeof(buffer), "%.3f", seconds);
						return buffer;
					}
				}

				void Diagnostics::beginRun() { _runStart = _clock(); }

				void Diagnostics::beginDocument(const std::string& displayName, int index, int total)
				{
					if (_options.progress)
						_console.writeLine("[" + std::to_string(index) + "/" + std::to_string(total) + "] " + displayName);

					_documentStart = _clock();
					_documentElapsed.reset();
				}

				std::chrono::steady_clock::duration Diagnostics::currentDocumentElapsed() const
				{
					if (_documentElapsed.has_value())
						return *_documentElapsed;
					if (_documentStart.has_value())
						return _clock() - *_documentStart;
					return std::chrono::steady_clock::duration::zero();
				}

				void Diagnostics::endDocument(const std::string& displayName)
				{
					if (!_documentStart.has_value())
						return;

					_documentElapsed = _clock() - *_documentStart;

					if (_options.logTime)
					{
						double seconds = std::chrono::duration<double>(*_documentElapsed).count();
						_console.writeLine(displayName + ": " + formatSeconds(seconds) + "s");
					}

					reportEngineMemory(2);
				}

				void Diagnostics::endRun(int succeeded, int failed)
				{
					if (_options.time && _runStart.has_value())
					{
						double seconds = std::chrono::duration<double>(_clock() - *_runStart).count();
						_console.writeLine(
							std::to_string(succeeded) + " succeeded, " + std::to_string(failed) + " failed in " + formatSeconds(seconds) + "s");
					}

					if (_options.logMemory)
						_console.writeLine("peak memory: " + std::to_string(peakWorkingSetMegabytes()) + "MB");

					// -x reports the engine's own view once, at the end; --xx also reports
					// it after every document, which endDocument does.
					reportEngineMemory(1);
				}

				void Diagnostics::reportEngineMemory(int atLeast)
				{
					if (_options.memoryStatusLevel < atLeast || _engine == nullptr)
						return;

					try
					{
						std::string status = _engine->getMemoryStatus();
						std::string trimmed = Runtime::Strings::trim(status);
						if (!trimmed.empty())
						{
							std::string rightTrimmed = status;
							while (!rightTrimmed.empty()
								&& (rightTrimmed.back() == ' ' || rightTrimmed.back() == '\t' || rightTrimmed.back() == '\r'
									|| rightTrimmed.back() == '\n'))
								rightTrimmed.pop_back();
							_console.writeLine(rightTrimmed);
						}
					}
					catch (const Engine::EngineException& error)
					{
						// A diagnostic must not fail a run that otherwise worked. Reported
						// so the silence is explained rather than mistaken for "no memory in
						// use".
						_console.writeError(std::string("-x could not read the engine's memory status: ") + error.what());
					}
				}

				void Diagnostics::checkLimits()
				{
					if (_options.maxTime.has_value() && _runStart.has_value())
					{
						double seconds = std::chrono::duration<double>(_clock() - *_runStart).count();
						if (seconds > *_options.maxTime)
							throw Runtime::LimitExceededError("--max-time of " + std::to_string(*_options.maxTime) + "s exceeded");
					}

					if (_options.maxMemory.has_value())
					{
						long long usedMegabytes = peakWorkingSetMegabytes();
						if (usedMegabytes >= *_options.maxMemory)
							throw Runtime::LimitExceededError("--max-memory of " + std::to_string(*_options.maxMemory) + "MB exceeded");
					}
				}

				void Diagnostics::checkDocumentTimeout(const std::string& displayName)
				{
					if (documentDeadlineExceeded())
						throw documentTimedOut(displayName);
				}

				bool Diagnostics::documentDeadlineExceeded() const
				{
					if (!_options.timeout.has_value() || !_documentStart.has_value())
						return false;

					double seconds = std::chrono::duration<double>(currentDocumentElapsed()).count();
					return seconds > *_options.timeout;
				}

				Runtime::LimitExceededError Diagnostics::documentTimedOut(const std::string& displayName) const
				{
					return Runtime::LimitExceededError("--timeout of " + std::to_string(*_options.timeout) + "s exceeded", displayName);
				}

				void Diagnostics::pauseIfRequested()
				{
					if (_options.pause)
						_console.waitForKey();
				}
			}
		}
	}
}
