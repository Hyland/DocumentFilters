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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_RUNPLAN_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_RUNPLAN_H

#include "FormatSpec.h"
#include "OutputNameTemplate.h"
#include "PageRange.h"

#include <optional>
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
				/// Who owns the output target.
				/// </summary>
				/// <remarks>
				/// This is one axis only: whether each input writes its own output or
				/// they all share one. How many <em>files</em> a single input produces is
				/// a separate question, answered by <c>FormatSpec::isSingleFile</c> --- a
				/// .docx converted to PNG writes one file per page but is still
				/// <see cref="OutputCardinality::PerInput"/>. Keeping the two apart is
				/// what stops <c>-o</c> from ever depending on <c>--to</c>.
				/// </remarks>
				enum class OutputCardinality
				{
					/// <summary>Each input owns its output, inside <c>--out-dir</c>. Printed as <c>per-input</c>.</summary>
					PerInput,

					/// <summary>Every input merges into one document: <c>--combine --out-file</c>. Printed as <c>combined</c>.</summary>
					Combined,

					/// <summary>A single input to a single <c>--out-file</c>. Printed as <c>single-target</c>.</summary>
					SingleTarget,

					/// <summary>A single-file format streamed to stdout. Printed as <c>stream</c>.</summary>
					Stream
				};

				/// <summary>The canonical text for one cardinality, shared by every command that states one.</summary>
				std::string cardinalityText(OutputCardinality cardinality);

				/// <summary>
				/// A directory plus a filename template, for outputs whose count depends
				/// on the document's page count.
				/// </summary>
				class TargetPattern
				{
				public:
					/// <summary>The directory the files land in, already including any replicated source tree.</summary>
					std::string directory;

					/// <summary>The template applied per page.</summary>
					OutputNameTemplate templateName;

					/// <summary>The output format's extension, for <c>{ext}</c>.</summary>
					std::string extension;

					/// <summary>Produces the concrete path for one page, during execution.</summary>
					std::string resolve(const std::string& sourcePath, int pageNumber) const;

					/// <summary>
					/// The part of the path that does not vary with the page number.
					/// </summary>
					/// <remarks>
					/// Two inputs whose keys match collide on <em>every</em> page, so this
					/// catches the case that actually bites --- a flattened walk
					/// containing two files with the same stem --- without knowing either
					/// page count.
					/// </remarks>
					std::string collisionKey(const std::string& sourcePath) const;

					/// <summary>
					/// The path of the first page that would be written, for the
					/// fail-fast check before any work starts.
					/// </summary>
					/// <remarks>
					/// The first <em>selected</em> page, not page 1: with <c>--pages 5-6</c>
					/// the first output is page 5, read straight off the explicit list ---
					/// no document is open yet at this point, so there is no page count to
					/// check it against.
					/// </remarks>
					std::string firstTarget(const std::string& sourcePath, const PageRange& pages) const;
				};

				/// <summary>One input and everything decided about it.</summary>
				/// <remarks>
				/// Exactly one of <c>target</c> and <c>targetPattern</c> is set --- or
				/// neither, when the item contributes to <c>RunPlan::combinedTarget</c>.
				///
				/// <b>Every per-page output is deferred</b>, including when <c>--pages</c>
				/// was given. An explicit range does not make the filenames knowable:
				/// nothing at this stage has opened the document, so whether every
				/// requested page actually exists is still unknown.
				/// </remarks>
				class PlannedItem
				{
				public:
					/// <summary>The path as it will be opened.</summary>
					std::string sourcePath;

					/// <summary>How the input is named in messages; for a subfile this includes the container.</summary>
					std::string displayName;

					/// <summary>
					/// The one output path, for a single-file format. Absent for a
					/// per-page format, and absent when this item feeds
					/// <c>RunPlan::combinedTarget</c>.
					/// </summary>
					std::optional<std::string> target;

					/// <summary>
					/// The unresolved output naming, for a per-page format. Absent when
					/// <c>target</c> is set.
					/// </summary>
					std::optional<TargetPattern> targetPattern;

					/// <summary>The pages to render, or <see cref="PageRange::all"/>.</summary>
					PageRange pages = PageRange::all();

					/// <summary>True when this item's output names are produced during execution.</summary>
					bool isPageCountDependent() const { return targetPattern.has_value(); }

					/// <summary>
					/// The directory this item's output lands in, or absent when it has no
					/// file target --- <c>--stdout</c>, or an item feeding
					/// <c>RunPlan::combinedTarget</c>.
					/// </summary>
					/// <remarks>
					/// The plan already knows this for both target shapes, which is what
					/// lets the directory be created once per item instead of at each of
					/// the three places that open an output.
					/// </remarks>
					std::optional<std::string> outputDirectory() const;
				};

				/// <summary>
				/// The complete description of a run, produced before anything is opened
				/// or written.
				/// </summary>
				/// <remarks>
				/// Holding the whole run as a value is what makes <c>--print-plan</c>
				/// possible, and it is the only way to detect two inputs resolving to the
				/// same output path before the first one has already been written.
				/// </remarks>
				class RunPlan
				{
				public:
					OutputCardinality cardinality;

					/// <summary>One entry per input, in processing order.</summary>
					std::vector<PlannedItem> items;

					/// <summary>
					/// The shared target when <c>cardinality</c> is
					/// <see cref="OutputCardinality::Combined"/>; otherwise absent.
					/// </summary>
					/// <remarks>
					/// A run-level fact rather than a <see cref="PlannedItem"/>, because
					/// <c>items</c> is keyed by <em>input</em>: one entry per document, in
					/// every cardinality and on every command.
					/// </remarks>
					std::optional<std::string> combinedTarget;

					/// <summary>The format every item renders to.</summary>
					const FormatSpec* format = nullptr;

					/// <summary>
					/// Renders the plan in the canonical form the conformance corpus
					/// compares. All four language implementations must produce
					/// byte-identical output.
					/// </summary>
					/// <remarks>
					/// <c>cardinality</c> always appears; <c>target</c>/<c>pattern</c>
					/// lines do not, because extract, list and meta produce names that
					/// only exist once a document is open and so stop at cardinality. For
					/// <see cref="OutputCardinality::Combined"/> the one target is
					/// <c>combinedTarget</c> rather than a per-item line, since every item
					/// feeds that single output. For <see cref="OutputCardinality::Stream"/>
					/// no item owns a path at all.
					/// </remarks>
					std::string toCanonicalString() const;
				};
			}
		}
	}
}

#endif
