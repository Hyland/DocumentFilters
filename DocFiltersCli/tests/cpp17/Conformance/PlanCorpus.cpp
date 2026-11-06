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

#include "PlanCorpus.h"

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
						constexpr const char* fileName = "plan-corpus.txt";

						bool isAsciiSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

						std::vector<std::string> splitPaths(const std::string& row)
						{
							std::vector<std::string> parts;
							size_t i = 0;
							while (i < row.size())
							{
								while (i < row.size() && isAsciiSpace(row[i]))
									++i;
								size_t start = i;
								while (i < row.size() && !isAsciiSpace(row[i]))
									++i;
								if (i > start)
									parts.push_back(row.substr(start, i - start));
							}
							return parts;
						}
					}

					namespace PlanCorpus
					{
						std::vector<FixtureCase> load() { return Fixture::load(fileName); }

						int expectedExit(const FixtureCase& testCase)
						{
							std::optional<std::string> value = testCase.get("exit");
							return value.has_value() ? std::stoi(*value) : 0;
						}

						Fakes::InMemoryFileSystem buildFileSystem(const FixtureCase& testCase)
						{
							Fakes::InMemoryFileSystem fileSystem;

							for (const std::string& row : testCase.givenAll("tree"))
							{
								for (const std::string& path : splitPaths(row))
								{
									if (!path.empty() && path.back() == '/')
										fileSystem.addDirectory(path.substr(0, path.size() - 1));
									else
										fileSystem.addFile(path);
								}
							}

							// Links after the tree, so a link may point at a directory the tree created.
							for (const std::string& row : testCase.givenAll("link"))
							{
								std::vector<std::string> parts = splitPaths(row);

								if (parts.size() != 2)
									throw FixtureFormatError(testCase.describe("'< link' needs '<path> <target>': " + row));

								fileSystem.markSymlink(parts[0], parts[1]);
							}

							// Hidden last: it marks an entry the two above have already created.
							for (const std::string& row : testCase.givenAll("hidden"))
							{
								for (const std::string& path : splitPaths(row))
									fileSystem.markHidden(path);
							}

							return fileSystem;
						}

						std::vector<std::string> splitArguments(const std::string& commandLine)
						{
							std::vector<std::string> args;
							std::string current;
							bool quoted = false;

							for (char c : commandLine)
							{
								if (c == '"')
								{
									quoted = !quoted;
								}
								else if (isAsciiSpace(c) && !quoted)
								{
									if (!current.empty())
									{
										args.push_back(current);
										current.clear();
									}
								}
								else
								{
									current += c;
								}
							}

							if (!current.empty())
								args.push_back(current);

							return args;
						}
					}
				}
			}
		}
	}
}
