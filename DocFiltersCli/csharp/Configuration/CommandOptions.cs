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

using Hyland.DocumentFilters.Cli.Planning;

namespace Hyland.DocumentFilters.Cli.Configuration
{
	/// <summary>Options that only <c>convert</c> accepts.</summary>
	public sealed class ConvertOptions
	{
		/// <summary><c>--to</c>: the resolved target format. Required.</summary>
		public FormatSpec Format { get; set; }

		/// <summary><c>--combine</c>: merge every input into one output document.</summary>
		public bool Combine { get; set; }

		/// <summary><c>--pages</c>. Null means every page.</summary>
		public PageRange Pages { get; set; }

		/// <summary><c>--canvas-options</c>, passed to the engine untouched.</summary>
		public string CanvasOptions { get; set; }

		/// <summary><c>--render-options</c>, passed to the engine untouched.</summary>
		public string RenderOptions { get; set; }

		/// <summary><c>--no-images</c>.</summary>
		public bool NoImages { get; set; }

		/// <summary><c>--include-control-codes</c>: keep ISYS control codes in text output.</summary>
		public bool IncludeControlCodes { get; set; }

		/// <summary><c>--fingerprint</c>.</summary>
		public bool Fingerprint { get; set; }

		/// <summary><c>--watermark</c>: a file path or a literal string.</summary>
		public string Watermark { get; set; }

		/// <summary><c>--redact</c>, repeatable.</summary>
		public List<RedactSpec> Redactions { get; } = new List<RedactSpec>();

		/// <summary><c>--form</c>, repeatable.</summary>
		public List<FormValue> FormValues { get; } = new List<FormValue>();

		/// <summary><c>--view</c>: open the result when done. Best effort, platform specific.</summary>
		public bool View { get; set; }

		/// <summary>The structure reports emitted during conversion.</summary>
		public StructureReportOptions Reports { get; } = new StructureReportOptions();
	}

	/// <summary>
	/// The document and page structure aspects <c>convert</c> can report while it
	/// works, emitted during a conversion run rather than by a separate command.
	/// </summary>
	public sealed class StructureReportOptions
	{
		public bool Fonts { get; set; }

		public bool FontMappings { get; set; }

		public bool PageFonts { get; set; }

		public bool PageFontMappings { get; set; }

		public bool Dimensions { get; set; }

		public bool PageElements { get; set; }

		public bool FormElements { get; set; }

		public bool Hyperlinks { get; set; }

		public bool Bookmarks { get; set; }

		public bool Annotations { get; set; }

		/// <summary>True when any report was requested.</summary>
		public bool Any =>
			Fonts || FontMappings || PageFonts || PageFontMappings || Dimensions ||
			PageElements || FormElements || Hyperlinks || Bookmarks || Annotations;

		/// <summary>
		/// The report flags asked for, spelled as the user typed them, in a fixed
		/// order.
		/// </summary>
		/// <remarks>
		/// So that a message naming one report names the same one in all four
		/// implementations: <c>--fonts --bookmarks</c> and <c>--bookmarks --fonts</c>
		/// are the same request and must produce the same error text. Hand-written
		/// beside the properties, in the same idiom as <c>ErrorCodes.All</c> and for
		/// the same reason — C++17 has no reflection.
		/// </remarks>
		public IReadOnlyList<string> Requested
		{
			get
			{
				var requested = new List<string>();
				if (Fonts) requested.Add("--fonts");
				if (FontMappings) requested.Add("--font-mappings");
				if (PageFonts) requested.Add("--page-fonts");
				if (PageFontMappings) requested.Add("--page-font-mappings");
				if (Dimensions) requested.Add("--dimensions");
				if (PageElements) requested.Add("--page-elements");
				if (FormElements) requested.Add("--form-elements");
				if (Hyperlinks) requested.Add("--hyperlinks");
				if (Bookmarks) requested.Add("--bookmarks");
				if (Annotations) requested.Add("--annotations");
				return requested;
			}
		}
	}

	/// <summary>Options that only <c>extract</c> accepts.</summary>
	public sealed class ExtractOptions
	{
		/// <summary><c>-i, --images</c>: pull embedded images too, not just subfiles.</summary>
		public bool Images { get; set; }

		/// <summary><c>--include</c>, repeatable: keep subfiles whose name matches.</summary>
		/// <remarks>
		/// Globs, the same pattern language as <c>-g/--glob</c> — and repeatable for the
		/// same reason, so that one glob per idea reads better than one glob doing
		/// several jobs.
		///
		/// <b>A different axis from <c>-g/--glob</c>.</b> That one chooses which files
		/// on disk to process; this one chooses which subfiles inside a container to
		/// extract — the same distinction <c>--max-depth</c> and <c>--subfile-depth</c>
		/// draw between outside the document and inside it.
		/// </remarks>
		public List<string> IncludePatterns { get; } = new List<string>();

		/// <summary><c>--exclude</c>, repeatable: drop subfiles whose name matches.</summary>
		public List<string> ExcludePatterns { get; } = new List<string>();

		/// <summary>
		/// <see cref="IncludePatterns"/> compiled, or null when none were given.
		/// </summary>
		/// <remarks>
		/// Built by <see cref="OptionsValidator"/>, so a malformed pattern is reported
		/// before any archive is opened rather than with one half-extracted.
		/// </remarks>
		public NameMatcher SubfileInclude { get; set; }

		/// <summary><see cref="ExcludePatterns"/> compiled, or null when none were given.</summary>
		public NameMatcher SubfileExclude { get; set; }

		/// <summary><c>--make-dirs</c>: replicate the source tree under the output directory.</summary>
		/// <remarks>
		/// Recorded separately from <see cref="FlatWasGiven"/> for the same reason
		/// <c>--max-depth</c> and <c>--no-recurse</c> are kept apart: collapsing two
		/// opposing switches into one boolean would make <c>--make-dirs --flat</c>
		/// indistinguishable from <c>--flat</c> alone, so the contradiction could not
		/// be reported.
		/// </remarks>
		public bool MakeDirsWasGiven { get; set; }

		/// <summary><c>--flat</c>: put every extracted file straight into the output directory.</summary>
		public bool FlatWasGiven { get; set; }

		/// <summary>
		/// Whether the source tree is replicated. Defaults to true, so <c>extract</c>
		/// lays out its output the way <c>convert</c> does.
		/// </summary>
		public bool ReplicateTree => !FlatWasGiven;

		/// <summary><c>--sub-id</c>: append the subfile id to extracted filenames.</summary>
		public bool AppendSubfileId { get; set; }
	}

	/// <summary>Options that only <c>list</c> accepts.</summary>
	public sealed class ListOptions
	{
		public bool Json { get; set; }
	}

	/// <summary>Options that only <c>identify</c> accepts.</summary>
	public sealed class IdentifyOptions
	{
		public bool Json { get; set; }

		/// <summary><c>--capabilities</c>: also print the format's capability flags.</summary>
		public bool Capabilities { get; set; }
	}

	/// <summary>Options that only <c>meta</c> accepts.</summary>
	public sealed class MetaOptions
	{
		public bool Json { get; set; }
	}

	/// <summary>Options that only <c>formats</c> accepts.</summary>
	public sealed class FormatsOptions
	{
		public bool Json { get; set; }

		/// <summary><c>--filter</c>: only formats whose id or name matches.</summary>
		public string Filter { get; set; }
	}

	/// <summary>Options that only the <c>options</c> command accepts.</summary>
	public sealed class OptionCatalogOptions
	{
		public bool Json { get; set; }

		/// <summary><c>--filter</c>: only options whose name or description matches.</summary>
		public string Filter { get; set; }

		/// <summary><c>--applies-to</c>: document, page, canvas, render, and so on.</summary>
		public string AppliesTo { get; set; }
	}

	/// <summary>One <c>--redact page,x,y,w,h</c> region.</summary>
	public sealed class RedactSpec
	{
		public int Page { get; set; }

		public int X { get; set; }

		public int Y { get; set; }

		public int Width { get; set; }

		public int Height { get; set; }
	}

	/// <summary>One <c>--form page,name=value</c> assignment.</summary>
	public sealed class FormValue
	{
		public int Page { get; set; }

		public string Name { get; set; }

		public string Value { get; set; }
	}
}
