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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_DISCOVERY_INPUTENUMERATOR_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_DISCOVERY_INPUTENUMERATOR_H

#include "Configuration/SharedOptions.h"
#include "Discovery/InputSource.h"
#include "Runtime/IConsole.h"
#include "Runtime/IFileSystem.h"

#include <set>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Discovery
			{
				/// <summary>
				/// Turns the positional paths and <c>--files-from</c> list into the
				/// concrete set of files to process.
				/// </summary>
				/// <remarks>
				/// Directories are walked recursively by default, the way <c>fd</c> and
				/// <c>ripgrep</c> behave; <c>--max-depth</c> and <c>--no-recurse</c> limit
				/// that. Directory depth is entirely separate from subfile depth, which
				/// controls recursion into documents nested inside other documents.
				///
				/// <c>-g/--glob</c> and <c>--regex</c> match against a walked entry's path
				/// relative to the root it was found under, not the root-qualified path:
				/// an anchored pattern such as <c>^report-\d{4}\.pdf$</c> would otherwise
				/// never match anything once the walked root itself has more than one path
				/// segment. An explicit file argument is never checked against either ---
				/// naming a file is an instruction, not a suggestion.
				/// </remarks>
				class InputEnumerator
				{
				public:
					InputEnumerator(Runtime::IFileSystem& fileSystem, Runtime::IConsole& console)
						: _fileSystem(fileSystem), _console(console)
					{
					}

					/// <summary>
					/// Produces the input list, in processing order. Selection by detected
					/// format (<c>--include-types</c> / <c>--exclude-types</c>) is not
					/// applied here, because it needs the engine to identify each file;
					/// that happens during execution.
					/// </summary>
					/// <exception cref="Runtime::NoInputsError">Nothing matched.</exception>
					std::vector<InputSource> enumerate(const Configuration::InputOptions& options);

				private:
					/// <summary>Reads a <c>--files-from</c> list. A path of "-" reads from stdin.</summary>
					std::vector<std::string> readFileList(const std::string& path);

					/// <summary>Walks one directory tree, honouring depth, hidden entries and symlinks.</summary>
					std::vector<InputSource> walkDirectory(const std::string& root, const Configuration::InputOptions& options);

					/// <summary>
					/// The recursive worker behind <c>walkDirectory</c>, accumulating into
					/// a caller-owned list.
					/// </summary>
					/// <remarks>
					/// Accumulating rather than returning a list per level is deliberate:
					/// the portability rules rule out a generator-style return, because
					/// Java has no clean equivalent and C++17 has none at all, and merging
					/// a list at every level of the recursion is four different idioms in
					/// four languages. Passing one list down ports unchanged.
					/// </remarks>
					void walkInto(const std::string& root, const std::string& current, int depth, const Configuration::InputOptions& options,
						std::vector<InputSource>& results);

					/// <summary>The filename portion of an explicitly named file argument.</summary>
					/// <remarks>
					/// Unlike a walked entry's relative path --- already forward-slash
					/// because <c>walkInto</c> built it from <c>IFileSystem::listDirectory</c>
					/// output --- an explicit argument is whatever the user typed on their
					/// own shell, backslashes included on Windows.
					/// </remarks>
					static std::string fileNameOf(const std::string& path);

					Runtime::IFileSystem& _fileSystem;
					Runtime::IConsole& _console;

					/// <summary>Directory identities already entered by the in-progress <c>walkInto</c> call.</summary>
					/// <remarks>
					/// Cleared per root rather than per <c>InputEnumerator</c> instance: two
					/// different explicit roots are free to share a directory, but a link
					/// cycle within one root must not loop forever. Keyed by identity, not
					/// path, because a link to an ancestor produces an endless supply of
					/// distinct path strings for what is really one directory.
					/// </remarks>
					std::set<std::string> _visited;
				};
			}
		}
	}
}

#endif
