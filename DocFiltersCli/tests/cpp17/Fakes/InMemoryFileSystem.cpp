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

#include "InMemoryFileSystem.h"

#include "Runtime/Strings.h"

#include <algorithm>
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
				namespace Fakes
				{
					/// <summary>
					/// A stream that writes its contents back into the fake when destroyed.
					/// </summary>
					/// <remarks>
					/// Without this the stream returned by <see cref="InMemoryFileSystem::createFile"/>
					/// would be detached: everything written to it would be discarded, and a
					/// later <see cref="InMemoryFileSystem::openRead"/> would see an empty
					/// file. Conversion output could then never be asserted on.
					/// </remarks>
					class CommittingStream : public std::ostringstream
					{
					public:
						CommittingStream(InMemoryFileSystem* fileSystem, std::string path)
							: _fileSystem(fileSystem), _path(std::move(path))
						{
						}

						~CommittingStream() override { _fileSystem->_files[_path] = str(); }

					private:
						InMemoryFileSystem* _fileSystem;
						std::string _path;
					};

					InMemoryFileSystem& InMemoryFileSystem::addFile(const std::string& path, const std::string& contents)
					{
						std::string stored = key(path);
						_files[stored] = contents;
						addParents(stored);
						return *this;
					}

					InMemoryFileSystem& InMemoryFileSystem::treatAsSameFile(const std::string& path, const std::string& sameAs)
					{
						_sameFile[normalise(path)] = normalise(sameAs);
						return *this;
					}

					std::string InMemoryFileSystem::key(const std::string& path) const
					{
						std::string normalised = normalise(path);
						auto canonical = _sameFile.find(normalised);
						return canonical == _sameFile.end() ? normalised : canonical->second;
					}

					InMemoryFileSystem& InMemoryFileSystem::addDirectory(const std::string& path)
					{
						std::string key = normalise(path);
						_directories.insert(key);
						addParents(key);
						return *this;
					}

					InMemoryFileSystem& InMemoryFileSystem::markHidden(const std::string& path)
					{
						_hidden.insert(normalise(path));
						return *this;
					}

					InMemoryFileSystem& InMemoryFileSystem::markSymlink(const std::string& path, const std::optional<std::string>& target)
					{
						std::string key = normalise(path);
						_symlinks.insert(key);

						// The link is a real directory entry, so a walk of its parent finds
						// it. Without this the walk never sees the link at all and any test
						// about following one passes without exercising anything.
						_directories.insert(key);
						addParents(key);

						if (target.has_value())
							_linkTargets[key] = normalise(*target);

						return *this;
					}

					bool InMemoryFileSystem::fileExists(const std::string& path) { return _files.count(key(path)) > 0; }

					bool InMemoryFileSystem::directoryExists(const std::string& path) { return _directories.count(normalise(path)) > 0; }

					std::vector<std::string> InMemoryFileSystem::listDirectory(const std::string& path)
					{
						// The same shape of failure a real filesystem raises for a directory the
						// process may not read: an exception, not an empty listing.
						if (unreadable.count(normalise(path)) != 0)
							throw std::runtime_error("Permission denied: " + path);

						std::string requested = normalise(path);

						// Listing through a link yields the target's children, re-rooted
						// under the path that was asked for --- which is what makes a link
						// to an ancestor generate the endless corpus/loop/loop/... that
						// --follow has to survive.
						std::string real = resolveThroughLinks(requested);

						if (real != requested)
						{
							std::vector<std::string> throughLink;
							for (const std::string& child : listDirectory(real))
								throughLink.push_back(requested + child.substr(real.size()));
							return throughLink;
						}

						std::string prefix = requested;
						if (!prefix.empty() && prefix.back() != '/')
							prefix += '/';

						std::set<std::string> children;
						auto considerCandidate = [&](const std::string& candidate)
						{
							if (candidate.rfind(prefix, 0) != 0 || candidate.size() == prefix.size())
								return;

							std::string rest = candidate.substr(prefix.size());
							size_t slash = rest.find('/');
							children.insert(prefix + (slash == std::string::npos ? rest : rest.substr(0, slash)));
						};

						for (const auto& entry : _files)
							considerCandidate(entry.first);
						for (const std::string& directory : _directories)
							considerCandidate(directory);

						// Deliberately unordered: IFileSystem promises no order and a real
						// directory listing gives none. A fake that sorted would let a walk
						// ignoring --sort pass the sorting tests.
						std::vector<std::string> result(children.begin(), children.end());
						std::reverse(result.begin(), result.end());
						return result;
					}

					bool InMemoryFileSystem::isHidden(const std::string& path) { return _hidden.count(normalise(path)) > 0; }

					bool InMemoryFileSystem::isSymbolicLink(const std::string& path) { return _symlinks.count(normalise(path)) > 0; }

					std::string InMemoryFileSystem::getDirectoryIdentity(const std::string& path) { return resolveThroughLinks(normalise(path)); }

					std::string InMemoryFileSystem::resolveThroughLinks(const std::string& path) const
					{
						std::string current = path;
						for (int hop = 0; hop < 64; ++hop)
						{
							std::string rewritten = rewriteLongestLinkPrefix(current);
							if (rewritten == current)
								return current;
							current = rewritten;
						}
						return current;
					}

					std::string InMemoryFileSystem::rewriteLongestLinkPrefix(const std::string& path) const
					{
						const std::string* bestLink = nullptr;

						for (const auto& entry : _linkTargets)
						{
							const std::string& link = entry.first;
							bool matches = path == link || (path.size() > link.size() && path.rfind(link + "/", 0) == 0);

							if (matches && (bestLink == nullptr || link.size() > bestLink->size()))
								bestLink = &link;
						}

						if (bestLink == nullptr)
							return path;

						return _linkTargets.at(*bestLink) + path.substr(bestLink->size());
					}

					std::unique_ptr<std::istream> InMemoryFileSystem::openRead(const std::string& path)
					{
						std::string stored = key(path);
						auto found = _files.find(stored);
						if (found == _files.end())
							throw std::runtime_error("No such file in the fake filesystem: " + stored);

						return std::make_unique<std::istringstream>(found->second);
					}

					std::unique_ptr<std::ostream> InMemoryFileSystem::createFile(const std::string& path)
					{
						std::string stored = key(path);
						created.push_back(stored);
						addParents(stored);
						_files[stored] = "";
						return std::make_unique<CommittingStream>(this, stored);
					}

					std::string InMemoryFileSystem::readText(const std::string& path) const { return _files.at(key(path)); }

					void InMemoryFileSystem::moveFile(const std::string& from, const std::string& to)
					{
						std::string source = key(from);
						std::string destination = key(to);

						auto found = _files.find(source);
						if (found == _files.end())
							throw std::runtime_error("no such file: " + source);

						_files[destination] = found->second;
						_files.erase(found);
						created.push_back(destination);
					}

					void InMemoryFileSystem::createDirectory(const std::string& path) { addDirectory(path); }

					std::vector<std::string> InMemoryFileSystem::readAllLines(const std::string& path)
					{
						std::string stored = key(path);
						auto found = _files.find(stored);
						if (found == _files.end())
							throw std::runtime_error("No such file in the fake filesystem: " + stored);

						return Runtime::Strings::splitLines(found->second);
					}

					void InMemoryFileSystem::addParents(const std::string& path)
					{
						std::string current = path;
						size_t slash = current.rfind('/');
						while (slash != std::string::npos && slash > 0)
						{
							current = current.substr(0, slash);
							_directories.insert(current);
							slash = current.rfind('/');
						}
					}

					std::string InMemoryFileSystem::normalise(const std::string& path)
					{
						std::string result = path;
						for (char& c : result)
						{
							if (c == '\\')
								c = '/';
						}
						while (!result.empty() && result.back() == '/')
							result.pop_back();
						return result;
					}
				}
			}
		}
	}
}
