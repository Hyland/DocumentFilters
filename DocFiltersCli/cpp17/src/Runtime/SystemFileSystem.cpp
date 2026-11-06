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

#include "SystemFileSystem.h"

#include "CaseProbe.h"
#include "Strings.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				namespace
				{
					namespace fs = std::filesystem;

					std::string normalise(const std::string& path)
					{
						std::string result = path;
						for (char& c : result)
						{
							if (c == '\\')
								c = '/';
						}
						return result;
					}

					std::string normalise(const fs::path& path) { return normalise(FilePath::toUtf8(path)); }
				}

				// u8path and u8string rather than the plain constructor and string(): those
				// two go through the active ANSI code page on Windows, these two state the
				// encoding. Both are deprecated in C++20 in favour of char8_t, which is why
				// they are called here and nowhere else --- the whole migration is these two
				// lines.
				fs::path FilePath::of(const std::string& utf8) { return fs::u8path(utf8); }

				std::string FilePath::toUtf8(const fs::path& path) { return path.u8string(); }

				bool SystemFileSystem::fileExists(const std::string& path)
				{
					std::error_code ec;
					return fs::is_regular_file(FilePath::of(path), ec);
				}

				bool SystemFileSystem::directoryExists(const std::string& path)
				{
					std::error_code ec;
					return fs::is_directory(FilePath::of(path), ec);
				}

				std::vector<std::string> SystemFileSystem::listDirectory(const std::string& path)
				{
					std::vector<std::string> entries;

					// The error_code overload reports by reference rather than throwing, and
					// an iterator it failed to build simply equals end() --- so ignoring ec
					// turned an unreadable or vanished directory into "no entries here". A
					// subtree the walk could not read was silently skipped, and a run given
					// only that directory reported E_NO_INPUTS: two different wrong answers,
					// neither of them mentioning the filesystem. The reference implementation
					// throws in both cases, which is what a caller is written for.
					std::error_code ec;
					fs::directory_iterator iterator(FilePath::of(path), ec);
					if (ec)
						throw std::runtime_error("could not list directory: " + path + ": " + ec.message());

					for (const fs::directory_entry& entry : iterator)
						entries.push_back(normalise(entry.path()));

					return entries;
				}

				bool SystemFileSystem::isHidden(const std::string& path)
				{
					std::string normalised = normalise(path);
					while (!normalised.empty() && normalised.back() == '/')
						normalised.pop_back();

					std::string name = FilePath::toUtf8(FilePath::of(normalised).filename());
					if (!name.empty() && name.front() == '.')
						return true;

#ifdef _WIN32
					DWORD attributes = GetFileAttributesW(FilePath::of(path).c_str());
					if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_HIDDEN) != 0)
						return true;
#endif

					return false;
				}

				bool SystemFileSystem::isSymbolicLink(const std::string& path)
				{
					std::error_code ec;
					return fs::is_symlink(FilePath::of(path), ec);
				}

				namespace
				{
					bool hostIsCaseSensitive()
					{
#if defined(_WIN32) || defined(__APPLE__)
						return false;
#else
						return true;
#endif
					}

					/// <summary>The first ancestor of the path that exists, or empty.</summary>
					/// <remarks>
					/// The output directory usually does not exist yet when the plan is checked
					/// --- creating it is part of the run --- so the question is answered about
					/// the nearest ancestor that does, which is on the same volume.
					/// </remarks>
					fs::path nearestExistingDirectory(const std::string& path)
					{
						std::error_code ec;
						fs::path candidate = fs::absolute(FilePath::of(path), ec);
						if (ec)
							return fs::path();

						candidate = candidate.parent_path();
						while (!candidate.empty())
						{
							if (fs::is_directory(candidate, ec))
								return candidate;

							fs::path parent = candidate.parent_path();
							if (parent == candidate)
								return fs::path();

							candidate = parent;
						}

						return fs::path();
					}

					/// <summary>True, false, or nothing when the directory cannot be asked.</summary>
					/// <remarks>
					/// A directory that cannot be listed at all is the one case this answers for
					/// itself: no listing, no measurement, host default.
					/// </remarks>
					std::optional<bool> probe(const fs::path& directory)
					{
						std::error_code ec;
						fs::directory_iterator entry(directory, ec);
						if (ec)
							return std::nullopt;

						auto nextEntry = [&entry, &ec]() -> std::optional<std::string> {
							if (ec || entry == fs::directory_iterator())
								return std::nullopt;

							std::string path = normalise(entry->path());
							entry.increment(ec);
							return path;
						};

						auto exists = [](const std::string& candidate) {
							// Reports false for a path it cannot reach, which is what File.Exists
							// does in the C# reference.
							std::error_code lookup;
							return fs::exists(FilePath::of(candidate), lookup);
						};

						return CaseProbe::measure(normalise(directory), nextEntry, exists);
					}
				}

				/// <summary>
				/// Whether the volume holding <paramref name="path"/> tells names apart by
				/// case, measured where it can be and assumed where it cannot.
				/// </summary>
				/// <remarks>
				/// The host default alone is wrong in both directions, and only one of them is
				/// safe. On Windows and macOS, guessing case-insensitive for a case-sensitive
				/// volume refuses a run the user can fix with <c>--out-name</c>. On Linux,
				/// guessing case-sensitive for a mounted NTFS or exFAT volume lets two planned
				/// outputs collide into one file and overwrite each other --- the very defect
				/// the collision check exists to catch.
				///
				/// So the directory is asked, and asked about itself: the probe flips the case
				/// of something already inside it and looks for that spelling in the same
				/// directory. Nothing is created, which is what lets the collision check run
				/// before any output exists and <c>--dry-run</c> still write nothing.
				///
				/// Inside, rather than the directory's own name in its parent, because the
				/// rules belong to the directory. A mount point is the case that makes the
				/// difference: probing <c>/mnt/NTFS</c> asks the Linux filesystem holding
				/// <c>/mnt</c>, which answers for itself and not for the volume mounted there,
				/// so every output under it would be planned as though case mattered. Windows
				/// can also mark one directory case-sensitive inside an insensitive volume,
				/// which the parent cannot see either.
				///
				/// Every letter is flipped rather than lowercased, so <c>Report.txt</c> is
				/// looked for as <c>rEPORT.TXT</c>. A second file really named that way would
				/// read as case-insensitive, which is the residual inaccuracy here --- and it
				/// errs the safe way, because believing a volume insensitive only makes the
				/// collision check stricter.
				///
				/// A directory that cannot answer --- empty, unreadable, or holding nothing
				/// with a letter in its name --- falls back to the host default, which is
				/// where this started.
				/// </remarks>
				bool SystemFileSystem::isCaseSensitive(const std::string& path)
				{
					fs::path directory = nearestExistingDirectory(path);
					if (directory.empty())
						return hostIsCaseSensitive();

					std::string key = FilePath::toUtf8(directory);
					auto cached = _caseSensitivity.find(key);
					if (cached != _caseSensitivity.end())
						return cached->second;

					bool sensitive = probe(directory).value_or(hostIsCaseSensitive());

					_caseSensitivity[key] = sensitive;
					return sensitive;
				}

				std::string SystemFileSystem::getDirectoryIdentity(const std::string& path)
				{
					std::error_code ec;
					fs::path resolved = fs::canonical(FilePath::of(path), ec);
					if (ec)
						resolved = fs::absolute(FilePath::of(path));

					std::string result = normalise(resolved);
					while (!result.empty() && result.back() == '/')
						result.pop_back();
					return result;
				}

				std::unique_ptr<std::istream> SystemFileSystem::openRead(const std::string& path)
				{
					auto stream = std::make_unique<std::ifstream>(FilePath::of(path), std::ios::binary);
					if (!stream->is_open())
						throw std::runtime_error("could not open for reading: " + path);

					return stream;
				}

				std::unique_ptr<std::ostream> SystemFileSystem::createFile(const std::string& path)
				{
					fs::path target = FilePath::of(path);
					if (target.has_parent_path())
						createDirectory(FilePath::toUtf8(target.parent_path()));

					auto stream = std::make_unique<std::ofstream>(FilePath::of(path), std::ios::binary | std::ios::trunc);
					if (!stream->is_open())
						throw std::runtime_error("could not create: " + path);

					return stream;
				}

				/// <remarks>
				/// <c>fs::rename</c> replaces an existing destination file, which is what
				/// <c>IFileSystem</c> promises and what <c>File.Move(overwrite: true)</c> does
				/// in the C# reference. POSIX rename has always replaced; on Windows this was
				/// measured rather than assumed, because the belief that it cannot is
				/// widespread and true of the plain <c>MoveFile</c> API --- the MSVC standard
				/// library passes MOVEFILE_REPLACE_EXISTING, and renaming over an existing file
				/// there succeeds with an empty error_code.
				/// </remarks>
				void SystemFileSystem::moveFile(const std::string& from, const std::string& to)
				{
					std::error_code ec;
					fs::rename(FilePath::of(from), FilePath::of(to), ec);
					if (ec)
						throw std::runtime_error("could not rename " + from + " to " + to + ": " + ec.message());
				}

				void SystemFileSystem::createDirectory(const std::string& path)
				{
					// Swallowing ec here did not make the failure go away, it moved it: the
					// directory was still missing when the file was created inside it, so the
					// run reported "could not create: <file>" and named neither the directory
					// nor the reason. A read-only parent and a path that is really a file both
					// arrived as the same misleading line.
					std::error_code ec;
					fs::create_directories(FilePath::of(path), ec);

					// create_directories reports "already there" as success with an empty ec,
					// so an existing directory does not reach this.
					if (ec)
						throw std::runtime_error("could not create directory: " + path + ": " + ec.message());
				}

				std::vector<std::string> SystemFileSystem::readAllLines(const std::string& path)
				{
					std::ifstream stream(FilePath::of(path), std::ios::binary);
					if (!stream.is_open())
						throw std::runtime_error("could not open for reading: " + path);

					std::ostringstream buffer;
					buffer << stream.rdbuf();

					return Strings::splitLines(buffer.str());
				}
			}
		}
	}
}
