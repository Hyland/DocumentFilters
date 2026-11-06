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

#include "Execution/EngineLog.h"
#include "Runtime/Strings.h"

#include <chrono>
#include <cstdio>
#include <ctime>

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
					std::string timestamp()
					{
						using namespace std::chrono;

						auto now = system_clock::now();
						auto milliseconds = duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
						std::time_t seconds = system_clock::to_time_t(now);
						std::tm local {};
#if defined(_WIN32)
						localtime_s(&local, &seconds);
#else
						localtime_r(&seconds, &local);
#endif

						char buffer[32];
						std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03d", local.tm_year + 1900, local.tm_mon + 1,
							local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec, static_cast<int>(milliseconds.count()));
						return buffer;
					}
				}

				std::optional<EngineLog> EngineLog::for_(const Configuration::DiagnosticOptions& options, Runtime::IConsole& console)
				{
					std::optional<std::string> requested = options.effectiveLogLevel();
					int level = parseLevel(requested);
					if (level == levelNone)
						return std::nullopt;

					std::optional<std::string> modules = options.effectiveLogModules();
					std::set<std::string> named;
					if (modules.has_value() && !modules->empty() && *modules != "*")
					{
						std::string current;
						for (char ch : *modules + ",")
						{
							if (ch == ',' || ch == ';')
							{
								std::string trimmed = Runtime::Strings::trim(current);
								if (!trimmed.empty())
									named.insert(Runtime::Strings::foldAscii(trimmed));
								current.clear();
							}
							else
							{
								current += ch;
							}
						}
					}

					return EngineLog(level, std::move(named), console);
				}

				int EngineLog::levelFor(const std::string& module) const
				{
					return _modules.empty() || _modules.count(Runtime::Strings::foldAscii(module)) > 0 ? _level : levelNone;
				}

				void EngineLog::write(int level, const std::string& module, const std::string& message) const
				{
					_console.writeError(timestamp() + " " + name(level) + " [" + module + "] " + message);
				}

				std::string EngineLog::name(int level)
				{
					switch (level)
					{
					case levelCritical:
					case levelError:
						return "ERROR";
					case levelWarning:
						return "WARN ";
					case levelInformation:
						return "INFO ";
					case levelDebug:
						return "DEBUG";
					case levelTrace:
						return "TRACE";
					default:
						return "     ";
					}
				}

				int EngineLog::parseLevel(const std::optional<std::string>& value)
				{
					if (!value.has_value())
						return levelNone;

					if (*value == "critical")
						return levelCritical;
					if (*value == "error")
						return levelError;
					if (*value == "warning")
						return levelWarning;
					if (*value == "information")
						return levelInformation;
					if (*value == "debug")
						return levelDebug;
					if (*value == "trace")
						return levelTrace;
					return levelNone;
				}
			}
		}
	}
}
