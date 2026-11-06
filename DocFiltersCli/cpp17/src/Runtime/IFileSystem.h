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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_IFILESYSTEM_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_IFILESYSTEM_H

#include <filesystem>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
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
				/// The filesystem operations this tool needs, behind one seam so the
				/// directory walk and the output planner can be unit tested entirely in
				/// memory.
				/// </summary>
				class IFileSystem
				{
				public:
					virtual ~IFileSystem() = default;

					virtual bool fileExists(const std::string& path) = 0;

					virtual bool directoryExists(const std::string& path) = 0;

					/// <summary>Immediate children of a directory, files and directories alike, unsorted.</summary>
					virtual std::vector<std::string> listDirectory(const std::string& path) = 0;

					/// <summary>True for entries the walk skips unless <c>--include-hidden</c> is given.</summary>
					virtual bool isHidden(const std::string& path) = 0;

					/// <summary>True for symlinked directories, which the walk enters only under <c>-L/--follow</c>.</summary>
					virtual bool isSymbolicLink(const std::string& path) = 0;

					/// <summary>
					/// A stable identity for a directory, with links resolved --- two paths
					/// that reach the same directory return the same value.
					/// </summary>
					/// <remarks>
					/// Needed to make <c>-L/--follow</c> terminate. Knowing that an entry is a
					/// link is not enough: a link pointing at one of its own ancestors
					/// produces an endless supply of distinct path strings, so a set of
					/// visited <em>paths</em> never detects the cycle. A set of visited
					/// identities does.
					/// </remarks>
					virtual std::string getDirectoryIdentity(const std::string& path) = 0;

					/// <summary>
					/// Whether the filesystem holding <paramref name="path"/> tells two
					/// names apart by case --- true where <c>Report.txt</c> and
					/// <c>report.txt</c> are two files.
					/// </summary>
					/// <remarks>
					/// Asked of the filesystem rather than derived from the host, for the
					/// same reason <c>getDirectoryIdentity</c> is: the planner cannot work
					/// it out and guessing is wrong often enough to matter. An
					/// <c>--out-dir</c> may sit on a mounted NTFS or exFAT volume under
					/// Linux, or on a case-sensitive volume under macOS, and the answer
					/// decides whether two planned outputs are one file.
					///
					/// The path need not exist: the question is about the volume it would
					/// live on, which is why the collision check can ask before anything is
					/// written and <c>--dry-run</c> can ask without touching the disk.
					/// </remarks>
					virtual bool isCaseSensitive(const std::string& path) = 0;

					virtual std::unique_ptr<std::istream> openRead(const std::string& path) = 0;

					/// <summary>Creates or truncates a file, creating parent directories as needed.</summary>
					virtual std::unique_ptr<std::ostream> createFile(const std::string& path) = 0;

					virtual void createDirectory(const std::string& path) = 0;

					/// <summary>
					/// Renames <paramref name="from"/> to <paramref name="to"/>, replacing an
					/// existing file at the destination.
					/// </summary>
					/// <remarks>
					/// For <c>web-safe</c>, the one format whose extension is not knowable until
					/// the page has been rendered: the engine picks PNG, GIF, JPEG or WebP per
					/// page, and the file is named for what it turned out to be. Whether
					/// replacing is allowed at all is settled before this is called, by the same
					/// output policy every other target goes through.
					/// </remarks>
					virtual void moveFile(const std::string& from, const std::string& to) = 0;

					/// <summary>Reads a <c>--files-from</c> list, one path per line.</summary>
					virtual std::vector<std::string> readAllLines(const std::string& path) = 0;
				};

				/// <summary>
				/// The one place a path crosses between this tool's strings and
				/// <c>std::filesystem</c>.
				/// </summary>
				/// <remarks>
				/// Every path in this tool is a UTF-8 <c>std::string</c>, which is also what
				/// the Document Filters C++ binding takes --- <c>DF::u8_to_w</c> is the first
				/// thing it does with a narrow filename. <c>std::filesystem::path</c> does not
				/// share that assumption: on Windows it stores UTF-16 and converts a narrow
				/// string through the active ANSI code page, so <c>fs::path(utf8)</c> and
				/// <c>path.string()</c> quietly destroy every name the code page cannot
				/// spell. A Japanese filename came back as <c>???.txt</c> and the run reported
				/// <c>E_NO_INPUTS</c> for a file that was plainly there; the C# reference,
				/// whose strings are UTF-16 throughout, opened it without comment.
				///
				/// So the conversion is named and used everywhere instead of being implied.
				/// On POSIX both directions are the identity --- the bytes already are the
				/// name --- which is why the defect was invisible on Linux.
				///
				/// Beside <see cref="IFileSystem"/> because it belongs to the same boundary:
				/// what crosses this seam is a path string, and this is what that string
				/// means.
				/// </remarks>
				namespace FilePath
				{
					/// <summary>A UTF-8 path string as <c>std::filesystem</c> wants it.</summary>
					std::filesystem::path of(const std::string& utf8);

					/// <summary>A <c>std::filesystem</c> path back as a UTF-8 string.</summary>
					std::string toUtf8(const std::filesystem::path& path);
				}
			}
		}
	}
}

#endif
