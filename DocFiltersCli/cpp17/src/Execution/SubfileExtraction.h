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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_SUBFILEEXTRACTION_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_SUBFILEEXTRACTION_H

#include "Configuration/CommandOptions.h"
#include "Configuration/SharedOptions.h"
#include "Engine/IDocumentFiltersEngine.h"
#include "Runtime/IFileSystem.h"

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>One subfile, as reported by <c>list</c>.</summary>
				struct SubfileEntry
				{
					std::string id;
					std::string name;

					/// <summary>Full display path including containers.</summary>
					std::string displayName;

					long long size = 0;

					/// <summary>Modification time in ISO 8601, or absent when the container does not record one.</summary>
					std::optional<std::string> modified;

					/// <summary>Nesting level; 1 for a direct child of the document named on the command line.</summary>
					int depth = 0;
				};

				/// <summary>
				/// Walking into documents that contain other documents: archives, mail
				/// stores, attachments, embedded images.
				/// </summary>
				/// <remarks>
				/// The depth limit here is <c>--subfile-depth</c>, which is entirely
				/// separate from the directory walk's <c>--max-depth</c>. A .docx inside a
				/// .zip inside an .msg is three subfile levels regardless of where the
				/// .msg sits on disk.
				/// </remarks>
				class SubfileExtraction
				{
				public:
					/// <summary>
					/// Takes the filesystem because extraction creates the directories a
					/// subfile's name implies.
					/// </summary>
					/// <remarks>
					/// The same split as everywhere else a file is produced: the directory
					/// is ours to create, the file itself is written by
					/// <c>ISubFileEntry::copyTo</c> inside the engine. Creating it through
					/// <c>Runtime::IFileSystem</c> rather than the real filesystem directly
					/// is what keeps the unit suite off the real disk --- a subfile whose
					/// name carries a path, or <c>--make-dirs</c>, would otherwise leave
					/// directories behind wherever the tests happened to run.
					/// </remarks>
					explicit SubfileExtraction(Runtime::IFileSystem& fileSystem) : _fileSystem(fileSystem) {}

					/// <summary>Extracts every subfile to disk, for the <c>extract</c> command.</summary>
					/// <remarks>
					/// <b>Every written path must be inside the output directory.</b> A
					/// subfile's name comes from the container, not from the user, so an
					/// archive can carry a name like <c>../../outside.txt</c> or an
					/// absolute path and walk out of the destination. Names are put
					/// through <see cref="resolveSafeOutputPath"/> before anything is
					/// created; no path reaches the filesystem any other way.
					/// </remarks>
					std::vector<std::string> extractAll(Engine::IDocument& container, const std::string& containerPath,
						const Configuration::SharedOptions& shared, const Configuration::ExtractOptions& options);

					/// <summary>
					/// Turns a container-supplied subfile name into a path that is
					/// guaranteed to sit inside <paramref name="outputRoot"/>.
					/// </summary>
					/// <remarks>
					/// The name is untrusted input. Traversal segments, absolute paths and
					/// drive letters are all neutralised; when
					/// <paramref name="replicateTree"/> is false the name collapses to its
					/// final component.
					///
					/// This is the lexical half of the guarantee and all of it that is
					/// knowable without touching the disk: every segment it returns is a
					/// plain name under the root. The other half is
					/// <see cref="verifyNoLinkedComponent"/>, because a name can be lexically
					/// contained and still land outside --- a symlink already sitting in the
					/// output directory resolves wherever it points.
					///
					/// The published <c>ExtractSubfiles</c> sample does the same thing; a
					/// tool that extracts archives has to.
					/// </remarks>
					static std::string resolveSafeOutputPath(const std::string& outputRoot, const std::string& subfileName, bool replicateTree);

					/// <summary>Lists subfiles without extracting, for the <c>list</c> command.</summary>
					std::vector<SubfileEntry> listAll(
						Engine::IDocument& container, const std::string& containerPath, const Configuration::SharedOptions& shared);

					/// <summary>
					/// Invokes <paramref name="action"/> for every subfile, depth-first,
					/// honouring <c>--subfile-depth</c>. Shared by <c>convert</c>,
					/// <c>extract</c>, <c>list</c>, <c>identify</c> and <c>meta</c> when
					/// <c>-s</c> is given.
					/// </summary>
					/// <remarks>
					/// <paramref name="container"/> must already be open --- recursion
					/// opens each subfile itself, the same way the published
					/// <c>ExtractSubfiles</c> sample does, since an
					/// <c>Engine::ISubFileEntry</c> is an <c>Engine::IDocument</c> and its
					/// own subfiles are unavailable until it is.
					/// </remarks>
					void walk(Engine::IDocument& container, const std::string& containerPath, int depth, const Configuration::SharedOptions& shared,
						const std::function<void(Engine::ISubFileEntry&, const std::string&, int)>& action);

					/// <summary>
					/// Renders a subfile timestamp in the ISO 8601 form the console
					/// contract uses, or absent when the container records none.
					/// </summary>
					/// <remarks>
					/// Kept as a pure function of the optional value so the boundary --- a
					/// container with no timestamp versus one dated at the epoch --- is
					/// testable without an archive.
					/// </remarks>
					static std::optional<std::string> formatTimestamp(const std::optional<std::chrono::system_clock::time_point>& modified);

					/// <summary>Builds the display name for a subfile: <c>container.zip&gt;inner.docx</c>.</summary>
					static std::string composeDisplayName(const std::string& containerPath, const std::string& subfileName);

					/// <summary>
					/// Applies <c>extract</c>'s <c>--include</c> and <c>--exclude</c>
					/// globs to a subfile's name.
					/// </summary>
					/// <remarks>
					/// Both are already-compiled <c>Planning::NameMatcher</c> instances; a
					/// malformed pattern fails when the argument is read, not here with an
					/// archive half-extracted.
					///
					/// Matching on the name lets a subfile be rejected without being
					/// opened, which matters for a large archive.
					/// </remarks>
					static bool passesSubfileFilters(Engine::ISubFileEntry& subfile, const Configuration::ExtractOptions& options);

				private:
					/// <summary>Inserts a subfile's id before its extension, for <c>--sub-id</c>.</summary>
					static std::string insertSubfileId(const std::string& path, const std::string& id);

					/// <summary>
					/// Splits a subfile name into path components that are guaranteed not
					/// to traverse upward or escape via a drive letter or UNC share, never
					/// leaving the result empty.
					/// </summary>
					/// <remarks>
					/// Every <c>.</c> and <c>..</c> component is dropped outright, not
					/// merely neutralised, so a name that is nothing but traversal
					/// (<c>"../.."</c>) cannot leave the result empty either --- that case
					/// falls back to a single placeholder segment. Removing rather than
					/// clamping is also what tells "..data", a legitimate filename, apart
					/// from an actual traversal component: only an exact match is dropped,
					/// never a prefix or substring.
					/// </remarks>
					static std::vector<std::string> safeSegments(const std::string& subfileName, bool replicateTree);

					/// <summary>
					/// Refuses an output path that reaches its destination through a symbolic
					/// link.
					/// </summary>
					/// <remarks>
					/// <c>resolveSafeOutputPath</c> guarantees the path spells out as a plain
					/// name under the root, which is a statement about the string. Where it
					/// lands is a statement about the disk: if <c>out/latest</c> is a link to a
					/// web root, a container entry named <c>latest/index.html</c> is lexically
					/// contained and still overwrites a file outside the output directory
					/// entirely. The container chooses that name, so the choice is not the
					/// user's.
					///
					/// Only components below the root are examined. The root itself may well be
					/// a link --- <c>--out-dir</c> is the user's own instruction, and refusing
					/// it would break ordinary setups --- and so may anything above it.
					///
					/// This closes the hole rather than sealing it: a link created between this
					/// check and <c>copyTo</c> would still be followed. Winning that race needs
					/// write access to the output directory while the run is in progress, at
					/// which point the file can simply be replaced afterwards; keeping the link
					/// open across the write is what a tool defending against a hostile
					/// filesystem would do, and that is not what this sample is.
					/// </remarks>
					void verifyNoLinkedComponent(const std::string& outputRoot, const std::string& outputPath);

					Runtime::IFileSystem& _fileSystem;

					/// <summary>
					/// Every output path this extractor has already resolved, for the whole
					/// run.
					/// </summary>
					/// <remarks>
					/// Extraction resolves its own paths --- the planner never sees them,
					/// because what a container holds is not known until it is open --- so the
					/// output policy has to be applied here or not at all. It was not: an
					/// existing file was overwritten without <c>--overwrite</c>, and two
					/// subfiles whose names sanitise to one path overwrote each other, both
					/// silently, exit 0.
					///
					/// On the extractor rather than on the call, because <c>extract</c> builds
					/// one of these and runs every input through it. Per call, two archives
					/// each holding a <c>readme.txt</c> flattened into one output directory
					/// would not collide at all --- the set would have been emptied in between
					/// --- and under <c>--overwrite</c> the second would silently replace the
					/// first. That is what the planner's own collision check refuses for
					/// ordinary outputs, with or without <c>--overwrite</c>, so extraction
					/// refuses it too.
					/// </remarks>
					std::set<std::string> _claimed;
				};
			}
		}
	}
}

#endif
