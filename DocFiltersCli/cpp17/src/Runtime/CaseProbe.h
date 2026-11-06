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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_CASEPROBE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_CASEPROBE_H

#include <functional>
#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// Chooses the spelling to look for when asking a directory whether it
				/// tells names apart by case.
				/// </summary>
				/// <remarks>
				/// Separate from <c>SystemFileSystem</c> and pure, because the one thing
				/// that has to be right here is <em>where</em> the question is asked, and
				/// a real filesystem cannot show that in a test: on a single volume every
				/// directory answers the same, so a probe aimed one directory too high
				/// passes every test a machine can run. What a test can check is that the
				/// name handed back lies inside the directory being measured --- which is
				/// the whole of the rule.
				///
				/// Why it matters: the lookup rules belong to the directory, not to the
				/// host and not to the volume above it. <c>/mnt/ntfs</c> is
				/// case-insensitive while the <c>/mnt</c> holding it is not, and Windows
				/// can mark a single directory case-sensitive inside an otherwise
				/// insensitive volume. Measuring the parent gets both of those backwards.
				/// </remarks>
				namespace CaseProbe
				{
					/// <summary>
					/// Whether <c>directory</c> tells names apart by case, or nothing when it
					/// has nothing to answer with.
					/// </summary>
					/// <param name="directory">The directory whose rules are in question.</param>
					/// <param name="nextEntry">
					/// What is already in it, one at a time, and nothing once the listing is
					/// spent. Pulled from only as far as the first name with a letter in it, so
					/// a directory of a hundred thousand files costs one entry rather than a
					/// hundred thousand.
					/// </param>
					/// <param name="exists">
					/// Whether a path is there, file or directory. Anything it cannot answer
					/// reads as absent, which is what <c>File.Exists</c> does with an unreadable
					/// path in the C# reference and therefore what every port does.
					/// </param>
					/// <returns>
					/// Nothing when the directory is empty or holds nothing with a letter in its
					/// name --- the caller falls back to the host default.
					/// </returns>
					std::optional<bool> measure(const std::string& directory,
						const std::function<std::optional<std::string>()>& nextEntry,
						const std::function<bool(const std::string&)>& exists);

					/// <summary>
					/// The path to look for inside <c>directory</c> to measure how it
					/// matches names, or nothing when <c>entry</c> cannot answer.
					/// </summary>
					/// <param name="directory">The directory whose rules are in question.</param>
					/// <param name="entry">
					/// Something already in it, named or full-path. Its name is flipped
					/// letter by letter, so <c>Report.txt</c> is looked for as
					/// <c>rEPORT.TXT</c>: if that is found, the directory matched two
					/// spellings to one file and is case-insensitive.
					/// </param>
					/// <returns>
					/// A sibling path under <c>directory</c>, or nothing when the name has
					/// no letter to flip and so tells the caller nothing.
					/// </returns>
					std::optional<std::string> flippedSibling(const std::string& directory, const std::string& entry);
				}
			}
		}
	}
}

#endif
