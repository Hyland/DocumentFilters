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

namespace Hyland.DocumentFilters.Cli.Planning
{
	/// <summary>
	/// Who owns the output target.
	/// </summary>
	/// <remarks>
	/// This is one axis only: whether each input writes its own output or they all
	/// share one. How many <em>files</em> a single input produces is a separate
	/// question, answered by <see cref="FormatSpec.IsSingleFile"/> — a .docx
	/// converted to PNG writes one file per page but is still
	/// <see cref="PerInput"/>. Keeping the two apart is what stops <c>-o</c> from
	/// ever depending on <c>--to</c>.
	///
	/// </remarks>
	public enum OutputCardinality
	{
		/// <summary>Each input owns its output, inside <c>--out-dir</c>. Printed as <c>per-input</c>.</summary>
		PerInput,

		/// <summary>Every input merges into one document: <c>--combine --out-file</c>. Printed as <c>combined</c>.</summary>
		Combined,

		/// <summary>A single input to a single <c>--out-file</c>. Printed as <c>single-target</c>.</summary>
		SingleTarget,

		/// <summary>A single-file format streamed to stdout. Printed as <c>stream</c>.</summary>
		Stream
	}

	/// <summary>
	/// The complete description of a run, produced before anything is opened or
	/// written.
	/// </summary>
	/// <remarks>
	/// Holding the whole run as a value is what makes <c>--print-plan</c> possible,
	/// and it is the only way to detect two inputs resolving to the same output path
	/// before the first one has already been written.
	/// </remarks>
	public sealed class RunPlan
	{
		public OutputCardinality Cardinality { get; set; }

		/// <summary>One entry per input, in processing order.</summary>
		public List<PlannedItem> Items { get; } = new List<PlannedItem>();

		/// <summary>The shared target when <see cref="Cardinality"/> is <see cref="OutputCardinality.Combined"/>; otherwise null.</summary>
		/// <remarks>
		/// A run-level fact rather than a <see cref="PlannedItem"/>, because
		/// <see cref="Items"/> is keyed by <em>input</em>: one entry per document, in
		/// every cardinality and on every command. That is the unit execution works
		/// in, the unit <c>--on-error</c> counts, and the unit a combined run still
		/// needs — a document that fails mid-combine is skipped while the rest are
		/// still written, so the run has to be able to fail one input and keep going.
		///
		/// Keying items by output instead would collapse a combined run into a single
		/// entry and lose that granularity.
		/// </remarks>
		public string CombinedTarget { get; set; }

		/// <summary>The format every item renders to.</summary>
		public FormatSpec Format { get; set; }

		/// <summary>
		/// Renders the plan in the canonical form the conformance corpus compares.
		/// All four language implementations must produce byte-identical output.
		/// </summary>
		/// <remarks>
		/// <c>cardinality</c> always appears; <c>target</c>/<c>pattern</c> lines do
		/// not, because extract, list and meta produce names that only exist once a
		/// document is open and so stop at cardinality. For <see cref="OutputCardinality.Combined"/>
		/// the one target is <see cref="CombinedTarget"/> rather than a per-item
		/// line, since every item feeds that single output. For
		/// <see cref="OutputCardinality.Stream"/> no item owns a path at all.
		/// </remarks>
		public string ToCanonicalString()
		{
			var lines = new List<string> { "cardinality " + CardinalityText(Cardinality) };
			if (Cardinality == OutputCardinality.Combined)
			{
				lines.Add("target " + CombinedTarget);
			}
			else
			{
				foreach (PlannedItem item in Items)
				{
					if (item.Target != null)
						lines.Add("target " + item.Target);
					else if (item.TargetPattern != null)
						lines.Add("pattern " + PatternText(item.TargetPattern));
				}
			}
			return string.Join("\n", lines);
		}

		/// <summary>The canonical text for one cardinality, shared by every command that states one.</summary>
		public static string CardinalityText(OutputCardinality cardinality)
		{
			switch (cardinality)
			{
				case OutputCardinality.PerInput:
					return "per-input";
				case OutputCardinality.Combined:
					return "combined";
				case OutputCardinality.SingleTarget:
					return "single-target";
				case OutputCardinality.Stream:
					return "stream";
				default:
					throw new ArgumentOutOfRangeException(nameof(cardinality));
			}
		}

		private static string PatternText(TargetPattern pattern)
		{
			return string.IsNullOrEmpty(pattern.Directory) ? pattern.Template.ToString() : pattern.Directory + "/" + pattern.Template;
		}
	}

	/// <summary>One input and everything decided about it.</summary>
	/// <remarks>
	/// Exactly one of <see cref="Target"/> and <see cref="TargetPattern"/> is set —
	/// or neither, when the item contributes to <see cref="RunPlan.CombinedTarget"/>.
	///
	/// <b>Every per-page output is deferred</b>, including when <c>--pages</c> was
	/// given. An explicit range does not make the filenames knowable: nothing at
	/// this stage has opened the document, so whether every requested page actually
	/// exists is still unknown — <c>--pages 1-2</c> against a document that turns
	/// out to have only one page is discovered during execution, once the real page
	/// count can be trusted, not guessed at here. Resolving both up front would also
	/// let the planner reject a run because a page-2 output already exists when
	/// page 2 was never going to be written.
	///
	/// Execution is the only point at which the pages that actually exist are known,
	/// so that is where the names are produced.
	/// </remarks>
	public sealed class PlannedItem
	{
		/// <summary>The path as it will be opened.</summary>
		public string SourcePath { get; set; }

		/// <summary>How the input is named in messages; for a subfile this includes the container.</summary>
		public string DisplayName { get; set; }

		/// <summary>
		/// The one output path, for a single-file format. Null for a per-page format,
		/// and null when this item feeds <see cref="RunPlan.CombinedTarget"/>.
		/// </summary>
		public string Target { get; set; }

		/// <summary>
		/// The unresolved output naming, for a per-page format. Null when
		/// <see cref="Target"/> is set.
		/// </summary>
		public TargetPattern TargetPattern { get; set; }

		/// <summary>True when this item's output names are produced during execution.</summary>
		public bool IsPageCountDependent => TargetPattern != null;

		/// <summary>
		/// The directory this item's output lands in, or null when it has no file
		/// target — <c>--stdout</c>, or an item feeding <see cref="RunPlan.CombinedTarget"/>.
		/// </summary>
		/// <remarks>
		/// The plan already knows this for both target shapes, which is what lets the
		/// directory be created once per item instead of at each of the three places
		/// that open an output. That matters because only one of those three is ours:
		/// the text path writes through <see cref="Runtime.IFileSystem"/>, but a canvas
		/// and a docpack are opened by the engine, which will not create a directory
		/// and reports a missing one as a per-document
		/// <c>FileStream::Create</c> failure that reads like the input was missing.
		/// </remarks>
		public string OutputDirectory =>
			TargetPattern != null
				? TargetPattern.Directory
				: string.IsNullOrEmpty(Target) ? null : System.IO.Path.GetDirectoryName(Target);

		/// <summary>The pages to render, or <see cref="PageRange.All"/>.</summary>
		public PageRange Pages { get; set; }
	}

	/// <summary>
	/// A directory plus a filename template, for outputs whose count depends on the
	/// document's page count.
	/// </summary>
	public sealed class TargetPattern
	{
		/// <summary>The directory the files land in, already including any replicated source tree.</summary>
		public string Directory { get; set; }

		/// <summary>The template applied per page.</summary>
		public OutputNameTemplate Template { get; set; }

		/// <summary>The output format's extension, for <c>{ext}</c>.</summary>
		public string Extension { get; set; }

		/// <summary>Produces the concrete path for one page, during execution.</summary>
		public string Resolve(string sourcePath, int pageNumber)
		{
			string fileName = Template.Render(sourcePath, Extension, pageNumber);
			return string.IsNullOrEmpty(Directory) ? fileName : (Directory + "/" + fileName);
		}

		/// <summary>
		/// The part of the path that does not vary with the page number.
		/// </summary>
		/// <remarks>
		/// Two inputs whose keys match collide on <em>every</em> page, so this catches
		/// the case that actually bites — a flattened walk containing two files with
		/// the same stem — without knowing either page count.
		/// </remarks>
		public string CollisionKey(string sourcePath)
		{
			// Any fixed page number identifies the non-page part of the name: if two
			// items differ only by page, they render identically here and correctly
			// collide; if they differ elsewhere, they differ here too.
			return Resolve(sourcePath, 0);
		}

		/// <summary>
		/// The path of the first page that would be written, for the fail-fast check
		/// before any work starts.
		/// </summary>
		/// <remarks>
		/// The first <em>selected</em> page, not page 1: with <c>--pages 5-6</c> the
		/// first output is page 5, read straight off the explicit list — no document
		/// is open yet at this point, so there is no page count to check it against.
		/// If the document turns out to have fewer pages than that, nothing is
		/// written and this check was moot — a rare false alarm that costs nothing,
		/// because execution re-checks every page before writing it. This is a
		/// fail-fast convenience for the common case of re-running into a directory
		/// that already holds output, not the guarantee.
		/// </remarks>
		public string FirstTarget(string sourcePath, PageRange pages)
		{
			int firstPage = pages.FirstPage;
			return Resolve(sourcePath, firstPage);
		}
	}
}
