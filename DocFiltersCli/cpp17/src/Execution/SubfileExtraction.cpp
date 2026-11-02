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

#include "Execution/SubfileExtraction.h"

#include "Planning/OutputPlanner.h"
#include "Runtime/CliError.h"

#include <set>
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
				std::vector<std::string> SubfileExtraction::extractAll(Engine::IDocument& container, const std::string& containerPath,
					const Configuration::SharedOptions& shared, const Configuration::ExtractOptions& options)
				{
					std::vector<std::string> written;

					std::string outputRoot = shared.output.outDir.value_or(".");

					auto extractOne = [&](Engine::ISubFileEntry& subfile, const std::string&, int)
					{
						if (!passesSubfileFilters(subfile, options))
							return;

						std::string outputPath = resolveSafeOutputPath(outputRoot, subfile.name(), options.replicateTree());
						if (options.appendSubfileId)
							outputPath = insertSubfileId(outputPath, subfile.id());

						if (!_claimed.insert(Planning::OutputPlanner::collisionKeyFor(outputPath, _fileSystem)).second)
						{
							throw Runtime::OutputConflictError(Runtime::ErrorCodes::outputCollision,
								"two subfiles would be extracted to the same output; use --sub-id to distinguish them");
						}

						Planning::OutputPlanner::verifyPageTarget(outputPath, shared, _fileSystem);
						verifyNoLinkedComponent(outputRoot, outputPath);

						std::size_t slash = outputPath.find_last_of('/');
						if (slash != std::string::npos && slash > 0)
							_fileSystem.createDirectory(outputPath.substr(0, slash));

						subfile.copyTo(outputPath);
						written.push_back(outputPath);
					};

					walk(container, containerPath, 1, shared, extractOne);

					if (options.images)
					{
						// Closed one at a time, as each is finished with --- the same
						// per-entry lifetime the recursive walk in Walk() keeps, and for
						// the same reason: a container with many images must not need as
						// many open handles at once as it has entries.
						std::vector<std::unique_ptr<Engine::ISubFileEntry>> images = container.images();
						for (std::unique_ptr<Engine::ISubFileEntry>& image : images)
						{
							extractOne(*image, composeDisplayName(containerPath, image->name()), 1);
							image.reset();
						}
					}

					return written;
				}

				void SubfileExtraction::verifyNoLinkedComponent(const std::string& outputRoot, const std::string& outputPath)
				{
					std::string root = outputRoot;
					for (char& c : root)
					{
						if (c == '\\')
							c = '/';
					}
					while (!root.empty() && root.back() == '/')
						root.pop_back();

					std::string path = outputPath;
					for (char& c : path)
					{
						if (c == '\\')
							c = '/';
					}

					if (path.rfind(root + "/", 0) != 0)
						return;

					// The file itself is a component like any other: a link named report.txt is
					// written through just as a link named reports/ is.
					std::size_t cursor = root.size();
					while (cursor < path.size())
					{
						std::size_t slash = path.find('/', cursor + 1);
						std::string component = slash == std::string::npos ? path : path.substr(0, slash);

						bool present = _fileSystem.directoryExists(component) || _fileSystem.fileExists(component);
						if (present && _fileSystem.isSymbolicLink(component))
						{
							throw Runtime::OutputConflictError(Runtime::ErrorCodes::unsafeOutput,
								"a subfile would be written through '" + component + "', which is a link out of the output directory");
						}

						if (slash == std::string::npos)
							break;

						cursor = slash;
					}
				}

				std::string SubfileExtraction::insertSubfileId(const std::string& path, const std::string& id)
				{
					std::size_t slash = path.find_last_of('/');
					std::string directory = slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
					std::string fileName = slash == std::string::npos ? path : path.substr(slash + 1);

					std::size_t dot = fileName.find_last_of('.');
					std::string stem = dot == std::string::npos ? fileName : fileName.substr(0, dot);
					std::string extension = dot == std::string::npos ? std::string() : fileName.substr(dot);

					return directory + stem + "-" + id + extension;
				}

				std::string SubfileExtraction::resolveSafeOutputPath(const std::string& outputRoot, const std::string& subfileName, bool replicateTree)
				{
					std::vector<std::string> segments = safeSegments(subfileName, replicateTree);

					std::string root = outputRoot;
					while (!root.empty() && root.back() == '/')
						root.pop_back();

					std::string joined;
					for (std::size_t i = 0; i < segments.size(); i++)
					{
						if (i > 0)
							joined += "/";
						joined += segments[i];
					}

					return root + "/" + joined;
				}

				std::vector<std::string> SubfileExtraction::safeSegments(const std::string& subfileName, bool replicateTree)
				{
					std::string normalised = subfileName;
					for (char& ch : normalised)
					{
						if (ch == '\\')
							ch = '/';
					}

					// A drive letter ("C:...") names something outside any output
					// directory; dropping just the colon is enough, because the leading
					// "C" and the rest of the path pass through the same segment filter
					// as everything else.
					if (normalised.size() >= 2 && normalised[1] == ':')
						normalised = normalised.substr(2);

					std::vector<std::string> segments;
					std::string current;
					auto flush = [&]()
					{
						if (!current.empty() && current != "." && current != "..")
							segments.push_back(current);
						current.clear();
					};
					for (char ch : normalised)
					{
						if (ch == '/')
							flush();
						else
							current += ch;
					}
					flush();

					if (!replicateTree)
					{
						std::optional<std::string> last;
						if (!segments.empty())
							last = segments.back();
						segments.clear();
						if (last.has_value())
							segments.push_back(*last);
					}

					if (segments.empty())
						segments.push_back("_");

					return segments;
				}

				std::vector<SubfileEntry> SubfileExtraction::listAll(
					Engine::IDocument& container, const std::string& containerPath, const Configuration::SharedOptions& shared)
				{
					std::vector<SubfileEntry> entries;
					walk(container, containerPath, 1, shared,
						[&](Engine::ISubFileEntry& subfile, const std::string& displayName, int depth)
						{
							SubfileEntry entry;
							entry.id = subfile.id();
							entry.name = subfile.name();
							entry.displayName = displayName;
							entry.size = subfile.size();
							entry.modified = formatTimestamp(subfile.modified());
							entry.depth = depth;
							entries.push_back(std::move(entry));
						});
					return entries;
				}

				void SubfileExtraction::walk(Engine::IDocument& container, const std::string& containerPath, int depth,
					const Configuration::SharedOptions& shared, const std::function<void(Engine::ISubFileEntry&, const std::string&, int)>& action)
				{
					// Drained into a vector up front (subFiles() is a one-shot drain,
					// same as the binding's own GetFirstSubFile/GetNextSubFile), then
					// each entry is closed explicitly as soon as its own action and
					// recursion finish --- matching the C# reference's per-entry `using`.
					// Without that, every sibling stays open until the whole vector is
					// destroyed at the end of the loop, which a container with many
					// subfiles could exhaust the engine's handles over.
					std::vector<std::unique_ptr<Engine::ISubFileEntry>> subfiles = container.subFiles();
					for (std::unique_ptr<Engine::ISubFileEntry>& subfileOwner : subfiles)
					{
						Engine::ISubFileEntry& subfile = *subfileOwner;
						std::string displayName = composeDisplayName(containerPath, subfile.name());
						action(subfile, displayName, depth);

						bool canRecurse = shared.subfiles.enabled && (!shared.subfiles.maxDepth.has_value() || depth < *shared.subfiles.maxDepth);

						if (canRecurse)
						{
							// Opening the entry yields a document sharing its handle, so
							// the entry's own close below is what closes it too.
							constexpr int igrBodyOnly = 0;
							std::unique_ptr<Engine::IDocument> opened = subfile.open(igrBodyOnly, std::string());
							walk(*opened, displayName, depth + 1, shared, action);
						}

						subfileOwner.reset();
					}
				}

				std::optional<std::string> SubfileExtraction::formatTimestamp(const std::optional<std::chrono::system_clock::time_point>& modified)
				{
					if (!modified.has_value())
						return std::nullopt;

					std::time_t seconds = std::chrono::system_clock::to_time_t(*modified);
					std::tm utc {};
#if defined(_WIN32)
					gmtime_s(&utc, &seconds);
#else
					gmtime_r(&seconds, &utc);
#endif

					if (utc.tm_year + 1900 <= 1970)
						return std::nullopt;

					char buffer[32];
					std::snprintf(
						buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02dZ", utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour,
						utc.tm_min, utc.tm_sec);
					return std::string(buffer);
				}

				std::string SubfileExtraction::composeDisplayName(const std::string& containerPath, const std::string& subfileName)
				{
					return containerPath + ">" + subfileName;
				}

				bool SubfileExtraction::passesSubfileFilters(Engine::ISubFileEntry& subfile, const Configuration::ExtractOptions& options)
				{
					if (options.subfileInclude.has_value() && !options.subfileInclude->matches(subfile.name()))
						return false;

					if (options.subfileExclude.has_value() && options.subfileExclude->matches(subfile.name()))
						return false;

					return true;
				}
			}
		}
	}
}
