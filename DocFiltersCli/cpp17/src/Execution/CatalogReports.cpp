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

#include "Execution/CatalogReports.h"
#include "Runtime/JsonWriter.h"
#include "Runtime/Strings.h"

#include <vector>

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
					std::vector<std::string> appliesTo(int flags)
					{
						std::vector<std::string> scopes;
						if ((flags & 0x1) != 0)
							scopes.push_back("document");
						if ((flags & 0x2) != 0)
							scopes.push_back("canvas");
						return scopes;
					}

					std::string joinComma(const std::vector<std::string>& values)
					{
						std::string result;
						for (std::size_t i = 0; i < values.size(); i++)
						{
							if (i > 0)
								result += ",";
							result += values[i];
						}
						return result;
					}

					bool containsIgnoreCase(const std::vector<std::string>& values, const std::string& value)
					{
						std::string folded = Runtime::Strings::foldAscii(value);
						for (const std::string& candidate : values)
						{
							if (Runtime::Strings::foldAscii(candidate) == folded)
								return true;
						}
						return false;
					}

					bool matchesFilter(const std::optional<std::string>& filter, const std::vector<std::string>& fields)
					{
						if (!filter.has_value() || filter->empty())
							return true;

						std::string foldedFilter = Runtime::Strings::foldAscii(*filter);
						for (const std::string& field : fields)
						{
							if (Runtime::Strings::foldAscii(field).find(foldedFilter) != std::string::npos)
								return true;
						}
						return false;
					}
				}

				void CatalogReports::writeFormats(const Configuration::FormatsOptions& options)
				{
					std::vector<Engine::EngineFormat> formats;
					for (const Engine::EngineFormat& format : _session.api().getSupportedFormats())
					{
						if (matchesFilter(options.filter, { format.displayName, format.shortName }))
							formats.push_back(format);
					}

					if (options.json)
					{
						Runtime::JsonWriter json;
						json.beginArray();
						for (const Engine::EngineFormat& format : formats)
						{
							json.beginObject().property("id", format.id).property("ext", format.shortName).property("name", format.displayName).endObject();
						}
						_console.write(json.endArray().toString());
					}
					else
					{
						for (const Engine::EngineFormat& format : formats)
							_console.writeLine("id=" + std::to_string(format.id) + " ext=" + format.shortName + " name=" + format.displayName);
					}
				}

				void CatalogReports::writeOptions(const Configuration::OptionCatalogOptions& options)
				{
					std::vector<std::pair<Engine::EngineOption, std::string>> matches;
					for (const Engine::EngineOption& option : _session.api().getAvailableOptions())
					{
						if (!matchesFilter(options.filter, { option.displayName, option.description }))
							continue;

						std::vector<std::string> scopes = appliesTo(option.flags);
						if (options.appliesTo.has_value() && !containsIgnoreCase(scopes, *options.appliesTo))
							continue;

						matches.emplace_back(option, scopes.empty() ? "none" : joinComma(scopes));
					}

					if (options.json)
					{
						Runtime::JsonWriter json;
						json.beginArray();
						for (const auto& match : matches)
						{
							json.beginObject()
								.property("name", match.first.displayName)
								.property("type", match.first.type)
								.property("default", match.first.defaultValue)
								.property("appliesTo", match.second)
								.property("description", match.first.description)
								.endObject();
						}
						_console.write(json.endArray().toString());
					}
					else
					{
						for (const auto& match : matches)
						{
							_console.writeLine("name=" + match.first.displayName + " type=" + match.first.type
								+ " default=" + match.first.defaultValue + " applies-to=" + match.second
								+ " description=" + match.first.description);
						}
					}
				}
			}
		}
	}
}
