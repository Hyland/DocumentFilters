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

#include "InputEnumerator.h"

#include "Runtime/CliError.h"
#include "Runtime/Strings.h"

#include <algorithm>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Discovery
			{
				namespace
				{
					using Runtime::NoInputsError;
				}

				std::vector<InputSource> InputEnumerator::enumerate(const Configuration::InputOptions& options)
				{
					std::vector<std::string> rawPaths = options.paths;
					if (options.filesFrom.has_value())
					{
						std::vector<std::string> fromFile = readFileList(*options.filesFrom);
						rawPaths.insert(rawPaths.end(), fromFile.begin(), fromFile.end());
					}

					if (rawPaths.empty())
						throw NoInputsError("no inputs given; name a file or a directory to process");

					std::vector<InputSource> results;
					for (const std::string& path : rawPaths)
					{
						if (_fileSystem.directoryExists(path))
						{
							std::vector<InputSource> walked = walkDirectory(path, options);
							results.insert(results.end(), walked.begin(), walked.end());
						}
						else if (_fileSystem.fileExists(path))
						{
							InputSource source;
							source.path = path;
							source.relativePath = fileNameOf(path);
							source.root = std::nullopt;
							results.push_back(source);
						}
						else
						{
							throw NoInputsError(path + ": no such file or directory");
						}
					}

					if (options.sort)
					{
						std::sort(results.begin(), results.end(),
							[](const InputSource& a, const InputSource& b) { return a.path < b.path; });
					}

					if (results.empty())
						throw NoInputsError("no inputs matched; the walk and filters selected nothing");

					return results;
				}

				std::vector<std::string> InputEnumerator::readFileList(const std::string& path)
				{
					std::vector<std::string> lines = (path == "-") ? _console.readAllInputLines() : _fileSystem.readAllLines(path);

					std::vector<std::string> result;
					for (const std::string& line : lines)
					{
						// Strings::isAllWhiteSpace rather than a loop over std::isspace:
						// the reference drops a line with string.IsNullOrWhiteSpace, whose
						// whitespace includes the no-break spaces, and a line holding one
						// of those is a blank line to the other three ports.
						if (!Runtime::Strings::isAllWhiteSpace(line))
							result.push_back(line);
					}
					return result;
				}

				std::vector<InputSource> InputEnumerator::walkDirectory(const std::string& root, const Configuration::InputOptions& options)
				{
					std::vector<InputSource> results;
					_visited.clear();
					walkInto(root, root, 1, options, results);
					return results;
				}

				void InputEnumerator::walkInto(const std::string& root, const std::string& current, int depth,
					const Configuration::InputOptions& options, std::vector<InputSource>& results)
				{
					if (!_visited.insert(_fileSystem.getDirectoryIdentity(current)).second)
						return;

					// A directory the walk cannot read is reported, not skipped. Skipping it
					// silently produces a shorter input list that looks like a complete one,
					// which is what an unchecked error_code used to do here.
					//
					// The message is fixed rather than the platform's own: libstdc++ says
					// "Permission denied", .NET says "Access to the path ... is denied", and
					// error text is part of the cross-language contract.
					std::vector<std::string> entries;
					try
					{
						entries = _fileSystem.listDirectory(current);
					}
					catch (const Runtime::CliError&)
					{
						throw;
					}
					catch (const std::exception&)
					{
						throw Runtime::NoInputsError(current + ": could not be read");
					}

					for (const std::string& entry : entries)
					{
						if (!options.includeHidden && _fileSystem.isHidden(entry))
							continue;

						if (_fileSystem.directoryExists(entry))
						{
							if (_fileSystem.isSymbolicLink(entry) && !options.followSymlinks)
								continue;

							int nextDepth = depth + 1;
							if (!options.noRecurseWasGiven && (!options.maxDepth.has_value() || nextDepth <= *options.maxDepth))
								walkInto(root, entry, nextDepth, options, results);
						}
						else
						{
							std::string relativePath = entry.substr(root.size());
							size_t nonSlash = relativePath.find_first_not_of('/');
							relativePath = (nonSlash == std::string::npos) ? std::string() : relativePath.substr(nonSlash);

							if (options.matcher.has_value() && !options.matcher->matches(relativePath))
								continue;

							InputSource source;
							source.path = entry;
							source.relativePath = relativePath;
							source.root = root;
							results.push_back(source);
						}
					}
				}

				std::string InputEnumerator::fileNameOf(const std::string& path)
				{
					std::string normalised = path;
					for (char& c : normalised)
					{
						if (c == '\\')
							c = '/';
					}

					size_t slash = normalised.rfind('/');
					return slash == std::string::npos ? normalised : normalised.substr(slash + 1);
				}
			}
		}
	}
}
