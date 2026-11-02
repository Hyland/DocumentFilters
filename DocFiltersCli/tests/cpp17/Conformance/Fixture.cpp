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

#include "Fixture.h"

// Resolved via docfilters_lib's public include directory (DocFiltersCli/cpp17/src),
// which tests/cpp17/CMakeLists.txt links this target against --- not a relative
// path, so it survives either tree moving independently of the other.
#include "Runtime/Strings.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Tests
			{
				namespace Conformance
				{
					namespace
					{
						using Runtime::Strings::trim;

						/// <summary>
						/// Splits a '&lt;' or '=' line into its key and its value.
						/// </summary>
						/// <remarks>
						/// Exactly one space separates the key from the value; everything
						/// after it is stored verbatim. Callers that do not care about the
						/// column padding use get()/getAll(), which trim; getRaw()/getAllRaw()
						/// exist for a value whose own leading whitespace is part of what it
						/// asserts, and must survive the reader.
						/// </remarks>
						std::pair<std::string, std::string> splitKeyValue(const std::string& line)
						{
							// line[0] is the '<' or '=' marker; everything from index 1 is
							// "key value" with any leading padding after the marker removed.
							std::string rest = line.substr(1);
							size_t restBegin = 0;
							while (restBegin < rest.size() && (rest[restBegin] == ' ' || rest[restBegin] == '\t'))
								++restBegin;
							rest = rest.substr(restBegin);

							size_t space = rest.find(' ');
							std::string key = (space == std::string::npos) ? rest : rest.substr(0, space);
							std::string value = (space == std::string::npos) ? std::string() : rest.substr(space + 1);

							return { key, value };
						}
					}

					std::optional<std::string> FixtureCase::getRaw(const std::string& key) const
					{
						for (const auto& pair : expectations)
						{
							if (pair.first == key)
								return pair.second;
						}
						return std::nullopt;
					}

					std::optional<std::string> FixtureCase::get(const std::string& key) const
					{
						std::optional<std::string> value = getRaw(key);
						if (!value.has_value())
							return std::nullopt;
						return trim(*value);
					}

					std::vector<std::string> FixtureCase::getAll(const std::string& key) const
					{
						std::vector<std::string> result;
						for (const auto& pair : expectations)
						{
							if (pair.first == key)
								result.push_back(trim(pair.second));
						}
						return result;
					}

					std::vector<std::string> FixtureCase::getAllRaw(const std::string& key) const
					{
						std::vector<std::string> result;
						for (const auto& pair : expectations)
						{
							if (pair.first == key)
								result.push_back(pair.second);
						}
						return result;
					}

					bool FixtureCase::has(const std::string& key) const
					{
						for (const auto& pair : expectations)
						{
							if (pair.first == key)
								return true;
						}
						return false;
					}

					std::vector<std::string> FixtureCase::givenAll(const std::string& key) const
					{
						std::vector<std::string> result;
						for (const auto& pair : given)
						{
							if (pair.first == key)
								result.push_back(trim(pair.second));
						}
						return result;
					}

					bool FixtureCase::hasGiven(const std::string& key) const
					{
						for (const auto& pair : given)
						{
							if (pair.first == key)
								return true;
						}
						return false;
					}

					bool FixtureCase::getBool(const std::string& key) const
					{
						std::optional<std::string> value = get(key);

						if (value == "yes")
							return true;
						if (value == "no")
							return false;

						throw FixtureFormatError(describe(
							"'" + key + "' must be yes or no, was '" + (value.has_value() ? *value : std::string("<absent>")) + "'"));
					}

					std::string FixtureCase::describe(const std::string& problem) const
					{
						std::ostringstream text;
						text << file << ':' << line << "  $ " << subject << '\n';
						text << "  " << problem << '\n';
						text << "  case as written:\n";

						for (const auto& pair : given)
							text << "    < " << pair.first << ' ' << pair.second << '\n';

						for (const auto& pair : expectations)
							text << "    = " << pair.first << ' ' << pair.second << '\n';

						return text.str();
					}

					namespace Fixture
					{
						std::vector<FixtureCase> load(const std::string& name)
						{
							std::string path = std::string(CONFORMANCE_FIXTURES_DIR) + "/" + name;

							std::ifstream stream(path, std::ios::binary);
							if (!stream)
								throw FixtureNotFoundError("conformance fixture not found: " + path);

							std::ostringstream buffer;
							buffer << stream.rdbuf();

							return parse(name, Runtime::Strings::splitLines(buffer.str()));
						}

						std::vector<FixtureCase> parse(const std::string& file, const std::vector<std::string>& lines)
						{
							std::vector<FixtureCase> cases;
							FixtureCase* current = nullptr;

							for (size_t i = 0; i < lines.size(); ++i)
							{
								std::string line = trim(lines[i]);

								if (!line.empty() && line[0] == '#')
									continue;

								if (line.empty())
								{
									// A blank line ends the current case, as the format
									// documents. Merely skipping it would silently attach a
									// later '=' line to the case above the gap --- which a
									// stricter reader in another language would reject, so the
									// same fixture would mean different things in different
									// ports.
									current = nullptr;
									continue;
								}

								if (line[0] == '$')
								{
									FixtureCase next;
									next.subject = trim(line.substr(1));
									next.file = file;
									next.line = static_cast<int>(i + 1);
									cases.push_back(std::move(next));
									current = &cases.back();
									continue;
								}

								if (line[0] == '<')
								{
									if (current == nullptr)
									{
										throw FixtureFormatError(
											file + ":" + std::to_string(i + 1) + ": '<' line with no preceding '$' case");
									}

									if (!current->expectations.empty())
									{
										throw FixtureFormatError(file + ":" + std::to_string(i + 1)
											+ ": '<' describes what the case is given and comes before every '=' line");
									}

									current->given.push_back(splitKeyValue(line));
									continue;
								}

								if (line[0] == '=')
								{
									if (current == nullptr)
									{
										throw FixtureFormatError(
											file + ":" + std::to_string(i + 1) + ": expectation with no preceding '$' case");
									}

									current->expectations.push_back(splitKeyValue(line));
									continue;
								}

								throw FixtureFormatError(file + ":" + std::to_string(i + 1) + ": unrecognised line: " + line);
							}

							return cases;
						}
					}
				}
			}
		}
	}
}
