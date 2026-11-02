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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_OUTPUTPLANNER_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_OUTPUTPLANNER_H

#include "Configuration/CommandOptions.h"
#include "Configuration/SharedOptions.h"
#include "Discovery/InputSource.h"
#include "RunPlan.h"
#include "Runtime/IFileSystem.h"

#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Planning
			{
				/// <summary>
				/// Decides where every output goes, before anything is written.
				/// </summary>
				/// <remarks>
				/// The one rule underneath all of this: <b>the meaning of <c>-o</c>
				/// never depends on <c>--to</c></b>. Four cardinalities, chosen by the
				/// arguments alone:
				/// <list type="bullet">
				/// <item><see cref="OutputCardinality::PerInput"/> --- <c>--out-dir</c>, each input writes its own output</item>
				/// <item><see cref="OutputCardinality::Combined"/> --- <c>--combine --out-file book.pdf</c></item>
				/// <item><see cref="OutputCardinality::SingleTarget"/> --- <c>--out-file</c> with exactly one input</item>
				/// <item><see cref="OutputCardinality::Stream"/> --- <c>--stdout</c>, or <c>--out-file -</c></item>
				/// </list>
				/// These name target ownership only. How many <em>files</em> one input
				/// produces is the separate question <c>FormatSpec::isSingleFile</c>
				/// answers: a .docx to PNG writes one file per page and is still
				/// <see cref="OutputCardinality::PerInput"/>.
				///
				/// <c>--out-file</c> with several inputs and no <c>--combine</c> is a
				/// hard error, never a guess.
				/// </remarks>
				namespace OutputPlanner
				{
					/// <summary>
					/// Builds the plan. Pure: no filesystem access, so it is exhaustively
					/// testable with a table of cases.
					/// </summary>
					/// <exception cref="Runtime::NoInputsError"><paramref name="inputs"/> is empty.</exception>
					/// <exception cref="Runtime::OutputConflictError">
					/// <c>--out-file</c> with multiple inputs and no <c>--combine</c>, or
					/// two inputs resolving to the same output path.
					/// </exception>
					RunPlan plan(
						const Configuration::SharedOptions& shared, const Configuration::ConvertOptions& convert,
						const std::vector<Discovery::InputSource>& inputs, Runtime::IFileSystem& fileSystem);

					/// <summary>
					/// The one part of planning that has to touch the filesystem:
					/// refusing to clobber existing files without <c>--overwrite</c>.
					/// Kept separate so that <see cref="plan"/> itself stays pure.
					/// </summary>
					/// <remarks>
					/// For a single-file item this is the whole check. For a per-page
					/// item it checks only <c>TargetPattern::firstTarget</c>, because the
					/// remaining names depend on the document's page count; execution
					/// checks each page as it goes, through <see cref="verifyPageTarget"/>.
					/// </remarks>
					/// <exception cref="Runtime::OutputConflictError">A target exists and <c>--overwrite</c> was not given.</exception>
					void verifyTargets(const RunPlan& plan, const Configuration::SharedOptions& shared, Runtime::IFileSystem& fileSystem);

					/// <summary>
					/// The deferred half of <see cref="verifyTargets"/>, called during
					/// execution once a page-count-dependent item's real filenames are
					/// known.
					/// </summary>
					void verifyPageTarget(const std::string& path, const Configuration::SharedOptions& shared, Runtime::IFileSystem& fileSystem);

					/// <summary>A planned path reduced to what two outputs must not share.</summary>
					std::string collisionKeyFor(const std::string& path, Runtime::IFileSystem& fileSystem);

					/// <summary>
					/// The same resolution <see cref="plan"/> uses internally, for a
					/// command with an output group but no <c>--combine</c> of its own
					/// --- extract, list and meta, which always pass <c>combine: false</c>.
					/// Their output names come from the container once it is open, so
					/// they stop at cardinality rather than resolving a target.
					/// </summary>
					OutputCardinality resolveCardinality(const Configuration::SharedOptions& shared, int inputCount, bool combine);
				}
			}
		}
	}
}

#endif
