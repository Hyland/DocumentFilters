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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_FAKES_INMEMORYFILESYSTEM_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_FAKES_INMEMORYFILESYSTEM_H

#include "Runtime/IFileSystem.h"

#include <map>
#include <optional>
#include <set>
#include <vector>

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
					/// A filesystem that lives entirely in memory, so the directory walk and
					/// the output planner can be tested without touching disk.
					/// </summary>
					/// <remarks>
					/// Paths are normalised to forward slashes throughout, which is also how
					/// the real implementation matches globs, so a test written here means
					/// the same thing on Windows and Linux.
					///
					/// This is test infrastructure, so unlike the project under test it is
					/// fully implemented.
					/// </remarks>
					class InMemoryFileSystem : public Runtime::IFileSystem
					{
					public:
						/// <summary>Everything written through <see cref="createFile"/>, for assertions.</summary>
						std::vector<std::string> created;

						/// <summary>Adds a file, creating its parent directories.</summary>
						InMemoryFileSystem& addFile(const std::string& path, const std::string& contents = "");

						/// <summary>
						/// Declares that two names reach one file, as a filesystem's own matching
						/// can.
						/// </summary>
						/// <remarks>
						/// Stated case by case rather than folded by a rule, because the rule is
						/// the thing under test. A real volume decides for itself which spellings
						/// are one file --- NTFS matches the two spellings of a-ring, measured ---
						/// and the tool's own collision key cannot always predict it. Writing the
						/// pair down here lets a test say "these two reach one file" without this
						/// fake acquiring a Unicode table that the thing it stands in for would not
						/// agree with anyway.
						/// </remarks>
						InMemoryFileSystem& treatAsSameFile(const std::string& path, const std::string& sameAs);

						/// <summary>Adds an empty directory.</summary>
						InMemoryFileSystem& addDirectory(const std::string& path);

						/// <summary>Marks an existing entry hidden, so the walk skips it without <c>--include-hidden</c>.</summary>
						InMemoryFileSystem& markHidden(const std::string& path);

						/// <summary>
						/// Marks a directory as a symlink, so the walk enters it only under
						/// <c>-L</c>.
						/// </summary>
						/// <param name="path">The link itself.</param>
						/// <param name="target">
						/// What it resolves to. Point this at an ancestor to build the cycle
						/// that <c>--follow</c> has to survive; leave absent when the link's
						/// destination does not matter to the test.
						/// </param>
						InMemoryFileSystem& markSymlink(const std::string& path, const std::optional<std::string>& target = std::nullopt);

						bool fileExists(const std::string& path) override;

						bool directoryExists(const std::string& path) override;

						std::vector<std::string> listDirectory(const std::string& path) override;

						bool isHidden(const std::string& path) override;

						bool isSymbolicLink(const std::string& path) override;

						/// <summary>
						/// The directory a path resolves to, following any link. Two paths
						/// reaching the same directory return the same identity, which is
						/// what lets the walk detect a cycle.
						/// </summary>
						std::string getDirectoryIdentity(const std::string& path) override;

						/// <summary>
						/// What this filesystem claims about case --- true unless a test says
						/// otherwise.
						/// </summary>
						/// <remarks>
						/// Case-sensitive by default because this fake stores paths in an
						/// ordinal map and so behaves that way; any other default would have
						/// its own lookups disagree with its answer here. Tests covering
						/// Windows and macOS set it false. The shared plan corpus leaves it
						/// alone, so its cases read the same on every platform.
						/// </remarks>
						bool caseSensitive = true;

						/// <summary>Directories this filesystem refuses to list, as a real one may.</summary>
						std::set<std::string> unreadable;

						bool isCaseSensitive(const std::string&) override { return caseSensitive; }

						std::unique_ptr<std::istream> openRead(const std::string& path) override;

						std::unique_ptr<std::ostream> createFile(const std::string& path) override;

						/// <summary>Reads back what was written to a created file.</summary>
						std::string readText(const std::string& path) const;

						void createDirectory(const std::string& path) override;

						void moveFile(const std::string& from, const std::string& to) override;

						std::vector<std::string> readAllLines(const std::string& path) override;

					private:
						friend class CommittingStream;

						/// <summary>
						/// Rewrites a path through however many links stand in it, so that
						/// every route to one directory produces the same answer.
						/// </summary>
						/// <remarks>
						/// <c>corpus/loop/loop</c> resolves through <c>corpus/loop -&gt;
						/// corpus</c> twice and lands back at <c>corpus</c>. The iteration is
						/// bounded because a link to its own ancestor never settles --- that
						/// is the whole point of the case, and stopping it is the walk's job,
						/// not the filesystem's.
						/// </remarks>
						std::string resolveThroughLinks(const std::string& path) const;

						std::string rewriteLongestLinkPrefix(const std::string& path) const;

						void addParents(const std::string& path);

						static std::string normalise(const std::string& path);

						/// <summary>The one name a file is stored under, whatever spelling was asked for.</summary>
						std::string key(const std::string& path) const;

						std::map<std::string, std::string> _files;
						std::set<std::string> _directories;
						std::set<std::string> _hidden;
						std::set<std::string> _symlinks;
						std::map<std::string, std::string> _linkTargets;
						std::map<std::string, std::string> _sameFile;
					};
				}
			}
		}
	}
}

#endif
