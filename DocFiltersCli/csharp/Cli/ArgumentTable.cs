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

// ---------------------------------------------------------------------------
//  WHY THIS FILE LOOKS THE WAY IT DOES
//
//  If you are reading this to learn how to build a CLI on Document Filters in
//  C#, the idiomatic approach is McMaster's attributes, and it is what the other
//  samples in this repository use:
//
//      class Program
//      {
//          [Option("-o|--out-dir", "Write into a directory", CommandOptionType.SingleValue)]
//          public string OutDir { get; set; }
//
//          [Option("-s|--subfiles", "Descend into subfiles", CommandOptionType.NoValue)]
//          public bool Subfiles { get; set; }
//
//          public static int Main(string[] args)
//              => CommandLineApplication.Execute<Program>(args);
//      }
//
//  That is shorter and perfectly good. Reach for it in your own code.
//
//  This sample uses McMaster's builder API and a table of setters instead, for a
//  reason specific to this project: the same CLI is implemented four times, in
//  C#, Python, Java and C++17, and the four must stay byte-for-byte identical.
//
//    * C# attributes cannot hold a lambda — they only take compile-time
//      constants — so an attribute cannot say where its value ends up. Neither
//      can picocli's annotations, which the Java port uses.
//    * A table where each row is (argument, help, arity, setter) says the whole
//      mapping on one line, ports directly to argparse, picocli and CLI11, and
//      can be enumerated to emit `--print-contract` for the conformance tests.
//    * Values that can fail to convert are parsed in ValueParsers, shared with
//      every other language, so a bad `--max-depth` gives the same message and
//      the same exit code in all four builds rather than four different ones
//      written by four different parsing libraries.
//
//  In short: the attribute form is better for one program, the table form is
//  better for one program written four times.
// ---------------------------------------------------------------------------

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Planning;

namespace Hyland.DocumentFilters.Cli.Cli
{
	/// <summary>
	/// Every argument the tool accepts, paired with what it does. The single source of
	/// truth for the command line surface.
	/// </summary>
	public static class ArgumentTable
	{
		private static string[] commands = { "convert", "extract", "list", "identify", "meta", "formats", "options" };
		private static string[] verbs = [.. commands, "help", "version"];

        /// <summary>
        /// The processing verbs, in help-display order. These are the commands that
        /// accept shared options, so this list drives <see cref="SharedFor"/> and the
        /// contract dump.
        /// </summary>
        public static IReadOnlyList<string> Commands => commands;

		/// <summary>
		/// Everything <c>docfilters &lt;verb&gt;</c> will dispatch, including the two
		/// that take no shared options at all.
		/// </summary>
		/// <remarks>
		/// <c>help</c> and <c>version</c> are verbs, but they are not in
		/// <see cref="Commands"/> because they process no documents and accept no
		/// shared options.
		///
		/// Note that <c>--help</c> and <c>-h</c> as *arguments* appear in neither list:
		/// every parsing library provides them, and their rendering differs between
		/// McMaster, argparse and CLI11, which is exactly why help text is not part
		/// of the cross-language contract.
		/// </remarks>
		public static IReadOnlyList<string> AllVerbs => verbs;

		/// <summary>
		/// The shared arguments a given verb accepts.
		/// </summary>
		/// <remarks>
		/// Composed from the group methods below rather than filtered out of one flat
		/// list. A verb simply does not call the groups it has no use for:
		/// <c>identify</c> writes no files, so it never calls
		/// <see cref="OutputArguments"/>; <c>formats</c> and <c>options</c> take no
		/// input files, so they call neither the walk nor the selection groups.
		///
		/// Expressing it as composition rather than a group tag on each argument means
		/// there is no group name to mistype and no filter to get wrong — an argument
		/// is on a command because that command asked for its group.
		/// </remarks>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> SharedFor(string command)
		{
			var result = new List<ArgumentDefinition<SharedOptions>>();
			switch (command)
			{
				case "convert":
				case "extract":
				case "list":
					result.AddRange(OutputArguments());
					result.AddRange(SubfileArguments());
					result.AddRange(CoreArguments());
					break;
				case "identify":
					result.AddRange(CoreArguments());
					break;
				case "meta":
					result.AddRange(OutputArguments());
					result.AddRange(CoreArguments());
					break;
				case "formats":
				case "options":
					result.AddRange(ConsoleArguments());
					result.AddRange(Global());
					result.AddRange(LibraryArguments());
					break;
				default:
					throw new ArgumentException($"unknown command '{command}'", nameof(command));
			}
			return result;
		}

		/// <summary>
		/// The shared groups every input-processing command takes regardless of
		/// whether it also writes output or descends into subfiles: name selection,
		/// type filters, the walk, passwords, processing, batch control, console,
		/// the global switches, diagnostics and the engine location.
		/// </summary>
		private static IEnumerable<ArgumentDefinition<SharedOptions>> CoreArguments()
		{
			return MatchArguments()
				.Concat(TypeArguments())
				.Concat(WalkArguments())
				.Concat(PasswordArguments())
				.Concat(ProcessingArguments())
				.Concat(BatchArguments())
				.Concat(ConsoleArguments())
				.Concat(Global())
				.Concat(DiagnosticArguments())
				.Concat(LibraryArguments());
		}

		/// <summary><c>-o/--out-dir</c>, <c>--out-file</c>, <c>--out-name</c>, <c>--stdout</c>, <c>--overwrite</c>.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> OutputArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("-o|--out-dir", "Write into a directory; safe for any format",
					ArgumentArity.SingleValue, (s, v) => s.Output.OutDir = v),

				new ArgumentDefinition<SharedOptions>("--out-file", "Write one artifact to exactly this path; '-' means stdout",
					ArgumentArity.SingleValue, (s, v) =>
					{
						if (v == "-")
							s.Output.UseStdout = true;
						else
							s.Output.OutFile = v;
					}),

				new ArgumentDefinition<SharedOptions>("--out-name",
					"Filename template inside --out-dir, e.g. {stem}-{page:04}.{ext}",
					ArgumentArity.SingleValue, (s, v) => s.Output.NameTemplate = OutputNameTemplate.Parse(v)),

				new ArgumentDefinition<SharedOptions>("--stdout", "Stream a single-file format to stdout",
					ArgumentArity.NoValue, (s, v) => s.Output.UseStdout = true),

				new ArgumentDefinition<SharedOptions>("--overwrite", "Replace existing outputs instead of erroring",
					ArgumentArity.NoValue, (s, v) => s.Output.Overwrite = true),
			};
		}

		/// <summary><c>-s/--subfiles</c> and <c>--subfile-depth</c>.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> SubfileArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("-s|--subfiles",
					"Descend into subfiles, attachments and embedded documents",
					ArgumentArity.NoValue, (s, v) => s.Subfiles.Enabled = true),

				new ArgumentDefinition<SharedOptions>("--subfile-depth", "How deep to recurse into nested subfiles",
					ArgumentArity.SingleValue, (s, v) => s.Subfiles.MaxDepth = ValueParsers.ParseCount("--subfile-depth", v)),
			};
		}

		/// <summary><c>-g/--glob</c> and <c>--regex</c>: selecting inputs by path.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> MatchArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("-g|--glob",
					"Keep paths matching a glob, e.g. **/*.pdf; prefix ! to exclude",
					ArgumentArity.MultipleValue, (s, v) => s.Input.GlobPatterns.Add(v)),

				new ArgumentDefinition<SharedOptions>("--regex", "Same selection, but match the path with a regular expression",
					ArgumentArity.MultipleValue, (s, v) => s.Input.RegexPatterns.Add(v)),
			};
		}

		/// <summary><c>--include-types</c> and <c>--exclude-types</c>: selecting by detected format.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> TypeArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("--include-types", "Semicolon list of format ids to keep",
					ArgumentArity.SingleValue, (s, v) =>
					{
						s.Input.IncludeTypes.Clear();
						s.Input.IncludeTypes.AddRange(ValueParsers.ParseIdList(v));
					}),

				new ArgumentDefinition<SharedOptions>("--exclude-types", "Semicolon list of format ids to drop",
					ArgumentArity.SingleValue, (s, v) =>
					{
						s.Input.ExcludeTypes.Clear();
						s.Input.ExcludeTypes.AddRange(ValueParsers.ParseIdList(v));
					}),
			};
		}

		/// <summary>The filesystem walk: <c>--files-from</c>, depth, <c>--follow</c>, <c>--sort</c>, <c>--include-hidden</c>.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> WalkArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("--files-from",
					"Read input paths from a file, one per line; - for stdin",
					ArgumentArity.SingleValue, (s, v) => s.Input.FilesFrom = v),

				new ArgumentDefinition<SharedOptions>("--max-depth", "Limit walk depth; --max-depth 1 stays in the given directory",
					ArgumentArity.SingleValue, (s, v) => s.Input.MaxDepth = ValueParsers.ParseCount("--max-depth", v)),

				new ArgumentDefinition<SharedOptions>("--no-recurse", "Don't descend into subdirectories (= --max-depth 1)",
					ArgumentArity.NoValue, (s, v) => s.Input.NoRecurseWasGiven = true),

				new ArgumentDefinition<SharedOptions>("-L|--follow", "Follow symlinked directories",
					ArgumentArity.NoValue, (s, v) => s.Input.FollowSymlinks = true),

				new ArgumentDefinition<SharedOptions>("--sort", "Sort entries before processing",
					ArgumentArity.NoValue, (s, v) => s.Input.Sort = true),

				new ArgumentDefinition<SharedOptions>("--include-hidden", "Include hidden and system files",
					ArgumentArity.NoValue, (s, v) => s.Input.IncludeHidden = true),
			};
		}

		/// <summary><c>-p/--password</c>, <c>--password-file</c>, <c>--password-prompt</c>.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> PasswordArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("-p|--password", "A candidate password, repeatable",
					ArgumentArity.MultipleValue, (s, v) => s.Passwords.Passwords.Add(v)),

				new ArgumentDefinition<SharedOptions>("--password-file", "One candidate password per line",
					ArgumentArity.SingleValue, (s, v) => s.Passwords.PasswordFile = v),

				new ArgumentDefinition<SharedOptions>("--password-prompt", "Prompt for passwords interactively",
					ArgumentArity.NoValue, (s, v) => s.Passwords.Prompt = true),
			};
		}

		/// <summary><c>--options</c>, <c>--codepage</c>, <c>--temp-dir</c>.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> ProcessingArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("--options",
					"Semicolon-separated engine options, e.g. BW_MODE=3;CODEPAGE=1252",
					ArgumentArity.SingleValue, (s, v) => s.Processing.EngineOptions = v),

				new ArgumentDefinition<SharedOptions>("--codepage", "Default codepage when the document does not declare one",
					ArgumentArity.SingleValue, (s, v) => s.Processing.Codepage = v),

				new ArgumentDefinition<SharedOptions>("--temp-dir", "Temp directory for scratch files",
					ArgumentArity.SingleValue, (s, v) => s.Processing.TempDir = v),
			};
		}

		/// <summary><c>--on-error</c> and <c>--fail-fast</c>.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> BatchArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("--on-error", "On a per-file failure: skip (default) or stop the run",
					ArgumentArity.SingleValue, (s, v) =>
					{
						s.Batch.OnError = ValueParsers.ParseOnError(ValueParsers.FoldToken(v));
						s.Batch.OnErrorWasExplicit = true;
					}),

				new ArgumentDefinition<SharedOptions>("--fail-fast", "Stop at the first failure (= --on-error stop)",
					ArgumentArity.NoValue, (s, v) =>
					{
						// Guarded rather than unconditional: --on-error's own setter always
						// has the last word on OnError/OnErrorWasExplicit whenever it runs,
						// regardless of which of the two the parsing library applies first,
						// so leaving an explicit value alone here is what makes the pair's
						// resolution independent of argument order rather than of registration
						// order in this table.
						if (!s.Batch.OnErrorWasExplicit)
							s.Batch.OnError = OnErrorBehaviour.Stop;
						s.Batch.FailFastWasGiven = true;
					}),
			};
		}

		/// <summary><c>--color</c>, <c>--verbose</c>, <c>--silent</c>. Every verb takes these.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> ConsoleArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("--color", "Colour errors and verbose output; ignored on a redirected stream",
					ArgumentArity.NoValue, (s, v) => s.Console.Color = true),

				new ArgumentDefinition<SharedOptions>("--verbose", "Print more about what the run is doing",
					ArgumentArity.NoValue, (s, v) => s.Console.Verbose = true),

				new ArgumentDefinition<SharedOptions>("--silent", "Print nothing but errors",
					ArgumentArity.NoValue, (s, v) => s.Console.Silent = true),
			};
		}

		/// <summary>Timing, limits, tracing and the <c>--debug-*</c> switches.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> DiagnosticArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("--time", "Report how long the run took",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.Time = true),

				new ArgumentDefinition<SharedOptions>("--logtime", "Log timing for each stage",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.LogTime = true),

				new ArgumentDefinition<SharedOptions>("--logmem", "Sample and report peak process memory use",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.LogMemory = true),

				new ArgumentDefinition<SharedOptions>("--progress", "Print a progress indicator",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.Progress = true),

				new ArgumentDefinition<SharedOptions>("--pause", "Pause before exiting",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.Pause = true),

				new ArgumentDefinition<SharedOptions>("--max-memory", "Abort the run if it exceeds this many megabytes",
					ArgumentArity.SingleValue, (s, v) => s.Diagnostics.MaxMemory = ValueParsers.ParseMegabytes("--max-memory", v)),

				new ArgumentDefinition<SharedOptions>("--max-time", "Abort the run after this many seconds",
					ArgumentArity.SingleValue, (s, v) => s.Diagnostics.MaxTime = ValueParsers.ParseSeconds("--max-time", v)),

				new ArgumentDefinition<SharedOptions>("--timeout", "Abort a single document after this many seconds",
					ArgumentArity.SingleValue, (s, v) => s.Diagnostics.Timeout = ValueParsers.ParseSeconds("--timeout", v)),

				new ArgumentDefinition<SharedOptions>("-x", "Report engine memory status once, at the end of the run",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.MemoryStatusLevel = Math.Max(s.Diagnostics.MemoryStatusLevel, 1)),

				new ArgumentDefinition<SharedOptions>("--xx", "Report engine memory status after every document too",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.MemoryStatusLevel = Math.Max(s.Diagnostics.MemoryStatusLevel, 2)),

				new ArgumentDefinition<SharedOptions>("--log-level", "How much the low-level readers log",
					ArgumentArity.SingleValue, (s, v) => s.Diagnostics.LogLevel = ValueParsers.FoldToken(v)),

				new ArgumentDefinition<SharedOptions>("--debug-log-level", "--log-level setting under its original name",
					ArgumentArity.SingleValue, (s, v) => s.Diagnostics.DebugLogLevel = ValueParsers.FoldToken(v)),

				new ArgumentDefinition<SharedOptions>("--log-modules", "Which modules log",
					ArgumentArity.SingleValue, (s, v) => s.Diagnostics.LogModules = v),

				new ArgumentDefinition<SharedOptions>("--debug-log-modules", "--log-modules setting under its original name",
					ArgumentArity.SingleValue, (s, v) => s.Diagnostics.DebugLogModules = v),

				new ArgumentDefinition<SharedOptions>("--debug-use-memory-stream", "Read each document through a memory stream",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.UseMemoryStream = true),

				new ArgumentDefinition<SharedOptions>("--debug-measure-io", "Report I/O counts and volumes per document",
					ArgumentArity.NoValue, (s, v) => s.Diagnostics.MeasureIo = true),

				new ArgumentDefinition<SharedOptions>("--debug-truncate", "Read only the first N bytes of each input",
					ArgumentArity.SingleValue, (s, v) => s.Diagnostics.TruncateInputBytes = ValueParsers.ParseCount("--debug-truncate", v)),
			};
		}

		/// <summary><c>--license-key</c> and <c>--library</c>. Every verb that touches the engine takes these.</summary>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> LibraryArguments()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("--license-key", "Document Filters license key; falls back to DF_LICENSE_KEY",
					ArgumentArity.SingleValue, (s, v) => s.Library.LicenseKey = v),

				new ArgumentDefinition<SharedOptions>("--library", "Path to the Document Filters engine; falls back to DF_PATH",
					ArgumentArity.SingleValue, (s, v) => s.Library.LibraryPath = v),
			};
		}

		/// <summary>
		/// Arguments that work on every verb and are not part of any shared group:
		/// <c>--dry-run</c> and <c>--print-contract</c>.
		/// </summary>
		/// <remarks>
		/// <c>--dry-run</c> runs stages 1 to 3 and prints the resulting plan without
		/// opening a document — useful to a user before committing to a long batch,
		/// and the mechanism the conformance corpus compares. <c>--print-plan</c> is
		/// kept as an alias because that is the name the design document uses.
		/// </remarks>
		public static IReadOnlyList<ArgumentDefinition<SharedOptions>> Global()
		{
			return new List<ArgumentDefinition<SharedOptions>>
			{
				new ArgumentDefinition<SharedOptions>("--dry-run", "Plan the run and print it, without opening a document",
					ArgumentArity.NoValue, (s, v) => s.DryRun = true),

				new ArgumentDefinition<SharedOptions>("--print-plan", "Alias for --dry-run",
					ArgumentArity.NoValue, (s, v) => s.DryRun = true),

				new ArgumentDefinition<SharedOptions>("--print-contract", "Print the whole argument surface as flat text, and stop",
					ArgumentArity.NoValue, (s, v) => s.PrintContract = true),
			};
		}

		/// <summary>
		/// <c>convert</c>'s own arguments, including <c>--pages</c>.
		/// </summary>
		/// <remarks>
		/// The CLI design lists <c>--pages</c> among the shared options, but
		/// <c>convert</c> is the only command that renders pages and
		/// <c>ConvertOptions.Pages</c> is its only destination. A shared definition is
		/// typed to <c>SharedOptions</c> and so could not reach that field at all, so
		/// it belongs here.
		/// </remarks>
		public static IReadOnlyList<ArgumentDefinition<ConvertOptions>> Convert()
		{
			return new List<ArgumentDefinition<ConvertOptions>>
			{
				new ArgumentDefinition<ConvertOptions>("--to", "Target format: text, md, html5, pdf, png, json, xml, tiff ...",
					ArgumentArity.SingleValue, (o, v) => o.Format = FormatCatalog.Resolve(ValueParsers.FoldToken(v))),

				new ArgumentDefinition<ConvertOptions>("--combine", "Merge every input into one output document",
					ArgumentArity.NoValue, (o, v) => o.Combine = true),

				new ArgumentDefinition<ConvertOptions>("--canvas-options", "Override canvas defaults, passed to the engine untouched",
					ArgumentArity.SingleValue, (o, v) => o.CanvasOptions = v),

				new ArgumentDefinition<ConvertOptions>("--render-options", "Override render-page defaults, passed to the engine untouched",
					ArgumentArity.SingleValue, (o, v) => o.RenderOptions = v),

				new ArgumentDefinition<ConvertOptions>("--no-images", "Skip embedded images during conversion",
					ArgumentArity.NoValue, (o, v) => o.NoImages = true),

				new ArgumentDefinition<ConvertOptions>("--include-control-codes", "Keep ISYS control codes in text output",
					ArgumentArity.NoValue, (o, v) => o.IncludeControlCodes = true),

				new ArgumentDefinition<ConvertOptions>("--fingerprint", "Emit page fingerprints",
					ArgumentArity.NoValue, (o, v) => o.Fingerprint = true),

				new ArgumentDefinition<ConvertOptions>("--watermark", "Stamp each page; a file path or a literal string",
					ArgumentArity.SingleValue, (o, v) => o.Watermark = v),

				new ArgumentDefinition<ConvertOptions>("--redact", "Black out a region: page,x,y,w,h",
					ArgumentArity.MultipleValue, (o, v) => o.Redactions.Add(ValueParsers.ParseRedaction(v))),

				new ArgumentDefinition<ConvertOptions>("--form", "Set a form field value: page,name=value",
					ArgumentArity.MultipleValue, (o, v) => o.FormValues.Add(ValueParsers.ParseFormValue(v))),

				new ArgumentDefinition<ConvertOptions>("--view", "Open the result when done",
					ArgumentArity.NoValue, (o, v) => o.View = true),

				new ArgumentDefinition<ConvertOptions>("--fonts", "Report fonts referenced in the document",
					ArgumentArity.NoValue, (o, v) => o.Reports.Fonts = true),

				new ArgumentDefinition<ConvertOptions>("--font-mappings", "Report the document's font mappings",
					ArgumentArity.NoValue, (o, v) => o.Reports.FontMappings = true),

				new ArgumentDefinition<ConvertOptions>("--page-fonts", "Report fonts per page",
					ArgumentArity.NoValue, (o, v) => o.Reports.PageFonts = true),

				new ArgumentDefinition<ConvertOptions>("--page-font-mappings", "Report font mappings per page",
					ArgumentArity.NoValue, (o, v) => o.Reports.PageFontMappings = true),

				new ArgumentDefinition<ConvertOptions>("--dimensions", "Report each page's dimensions",
					ArgumentArity.NoValue, (o, v) => o.Reports.Dimensions = true),

				new ArgumentDefinition<ConvertOptions>("--page-elements", "Report each page's page elements",
					ArgumentArity.NoValue, (o, v) => o.Reports.PageElements = true),

				new ArgumentDefinition<ConvertOptions>("--form-elements", "Report each page's form elements",
					ArgumentArity.NoValue, (o, v) => o.Reports.FormElements = true),

				new ArgumentDefinition<ConvertOptions>("--hyperlinks", "Report each page's hyperlink elements",
					ArgumentArity.NoValue, (o, v) => o.Reports.Hyperlinks = true),

				new ArgumentDefinition<ConvertOptions>("--bookmarks", "Report the document's bookmarks",
					ArgumentArity.NoValue, (o, v) => o.Reports.Bookmarks = true),

				new ArgumentDefinition<ConvertOptions>("--annotations", "Report the document's annotations",
					ArgumentArity.NoValue, (o, v) => o.Reports.Annotations = true),

				new ArgumentDefinition<ConvertOptions>("--pages", "Page ranges to render, e.g. 1,3-5,8",
					ArgumentArity.SingleValue, (o, v) => o.Pages = PageRange.Parse(v)),
			};
		}

		public static IReadOnlyList<ArgumentDefinition<ExtractOptions>> Extract()
		{
			return new List<ArgumentDefinition<ExtractOptions>>
			{
				new ArgumentDefinition<ExtractOptions>("-i|--images", "Also pull embedded images, not just subfiles",
					ArgumentArity.NoValue, (o, v) => o.Images = true),

				new ArgumentDefinition<ExtractOptions>("--include", "Keep subfiles whose name matches this glob, repeatable",
					ArgumentArity.MultipleValue, (o, v) => o.IncludePatterns.Add(v)),

				new ArgumentDefinition<ExtractOptions>("--exclude", "Drop subfiles whose name matches this glob, repeatable",
					ArgumentArity.MultipleValue, (o, v) => o.ExcludePatterns.Add(v)),

				new ArgumentDefinition<ExtractOptions>("--make-dirs", "Replicate the source tree under the output directory",
					ArgumentArity.NoValue, (o, v) => o.MakeDirsWasGiven = true),

				new ArgumentDefinition<ExtractOptions>("--flat", "Put every extracted file straight into the output directory",
					ArgumentArity.NoValue, (o, v) => o.FlatWasGiven = true),

				new ArgumentDefinition<ExtractOptions>("--sub-id", "Append the subfile id to extracted filenames",
					ArgumentArity.NoValue, (o, v) => o.AppendSubfileId = true),
			};
		}

		public static IReadOnlyList<ArgumentDefinition<ListOptions>> List()
		{
			return new List<ArgumentDefinition<ListOptions>>
			{
				new ArgumentDefinition<ListOptions>("--json", "Machine-readable listing",
					ArgumentArity.NoValue, (o, v) => o.Json = true),
			};
		}

		public static IReadOnlyList<ArgumentDefinition<IdentifyOptions>> Identify()
		{
			return new List<ArgumentDefinition<IdentifyOptions>>
			{
				new ArgumentDefinition<IdentifyOptions>("--json", "Machine-readable output",
					ArgumentArity.NoValue, (o, v) => o.Json = true),

				new ArgumentDefinition<IdentifyOptions>("--capabilities", "Also print the format's capability flags",
					ArgumentArity.NoValue, (o, v) => o.Capabilities = true),
			};
		}

		public static IReadOnlyList<ArgumentDefinition<MetaOptions>> Meta()
		{
			return new List<ArgumentDefinition<MetaOptions>>
			{
				new ArgumentDefinition<MetaOptions>("--json", "Metadata as JSON instead of plain text",
					ArgumentArity.NoValue, (o, v) => o.Json = true),
			};
		}

		public static IReadOnlyList<ArgumentDefinition<FormatsOptions>> Formats()
		{
			return new List<ArgumentDefinition<FormatsOptions>>
			{
				new ArgumentDefinition<FormatsOptions>("--json", "Machine-readable catalog",
					ArgumentArity.NoValue, (o, v) => o.Json = true),

				new ArgumentDefinition<FormatsOptions>("--filter", "Only formats whose id or name matches",
					ArgumentArity.SingleValue, (o, v) => o.Filter = v),
			};
		}

		public static IReadOnlyList<ArgumentDefinition<OptionCatalogOptions>> OptionCatalog()
		{
			return new List<ArgumentDefinition<OptionCatalogOptions>>
			{
				new ArgumentDefinition<OptionCatalogOptions>("--json",
					"Full catalog: name, description, default, type, appliesTo",
					ArgumentArity.NoValue, (o, v) => o.Json = true),

				new ArgumentDefinition<OptionCatalogOptions>("--filter", "Only options whose name or description matches",
					ArgumentArity.SingleValue, (o, v) => o.Filter = v),

				new ArgumentDefinition<OptionCatalogOptions>("--applies-to", "Filter by scope: document, page, canvas, render ...",
					ArgumentArity.SingleValue, (o, v) => o.AppliesTo = ValueParsers.FoldToken(v)),
			};
		}

		/// <summary>
		/// Every argument on a given verb — shared plus command-specific — as descriptors,
		/// for the contract dump and the coverage tests.
		/// </summary>
		public static IReadOnlyList<IArgumentDescriptor> DescribeCommand(string command)
		{
			var result = new List<IArgumentDescriptor>();
			switch (command)
			{
				case "convert": result.AddRange(Convert()); break;
				case "extract": result.AddRange(Extract()); break;
				case "list": result.AddRange(List()); break;
				case "identify": result.AddRange(Identify()); break;
				case "meta": result.AddRange(Meta()); break;
				case "formats": result.AddRange(Formats()); break;
				case "options": result.AddRange(OptionCatalog()); break;
				default:
					throw new ArgumentException($"unknown command '{command}'", nameof(command));
			}
			result.AddRange(SharedFor(command));
			return result;
		}
	}
}
