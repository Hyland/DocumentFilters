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

using System.Text;
using Hyland.DocumentFilters.Cli.Planning;

namespace Hyland.DocumentFilters.Cli.Configuration
{
	/// <summary>
	/// Everything the shared options resolved to. One instance per run, filled in by
	/// the argument table's setters, then checked by <see cref="OptionsValidator"/>.
	/// </summary>
	/// <remarks>
	/// <b>Two arguments never share one field.</b> Where two arguments describe the same
	/// setting, each records its own presence and the setting is derived from both —
	/// <c>--max-depth</c> plus <c>NoRecurseWasGiven</c> gives
	/// <see cref="InputOptions.EffectiveMaxDepth"/>, <c>--log-level</c> plus
	/// <c>--debug-log-level</c> gives <see cref="DiagnosticOptions.EffectiveLogLevel"/>.
	/// Collapsing a pair into one field makes a contradiction indistinguishable from a
	/// plain restatement, so it could never be reported — and reporting it is the point,
	/// because argv order is not portable enough to resolve such a pair by precedence.
	///
	/// Defaults live here as field initialisers, never in the parser declarations —
	/// four declarations of the same default drift, one does not.
	/// </remarks>
	public sealed class SharedOptions
	{
		public InputOptions Input { get; } = new InputOptions();

		public OutputOptions Output { get; } = new OutputOptions();

		public SubfileOptions Subfiles { get; } = new SubfileOptions();

		public PasswordOptions Passwords { get; } = new PasswordOptions();

		public ProcessingOptions Processing { get; } = new ProcessingOptions();

		public BatchOptions Batch { get; } = new BatchOptions();

		public ConsoleOptions Console { get; } = new ConsoleOptions();

		/// <summary>
		/// <c>--dry-run</c>, or its alias <c>--print-plan</c>: run stages 1 to 3, print
		/// the resulting plan, and stop without opening a document.
		/// </summary>
		/// <remarks>
		/// Two arguments write this one field, which is safe here where it would not be
		/// elsewhere: both set it to true, so giving both is redundant rather than
		/// contradictory and there is nothing to reconcile. Aliases that carry a
		/// <em>value</em> — <c>--log-level</c> and <c>--debug-log-level</c> — get a
		/// field each instead.
		/// </remarks>
		public bool DryRun { get; set; }

		/// <summary>
		/// <c>--print-contract</c>: emit the whole argument surface as flat text and
		/// stop. A debugging aid, and the same information <c>help --json</c> returns.
		/// </summary>
		public bool PrintContract { get; set; }

		public DiagnosticOptions Diagnostics { get; } = new DiagnosticOptions();

		public LibraryOptions Library { get; } = new LibraryOptions();

        /// <summary>
        /// Renders the options in the canonical form used by the conformance corpus:
        /// the <c>walk</c>, <c>match</c>, <c>types</c>, <c>subfiles</c> and
        /// <c>on-error</c> lines of a <c>--print-plan</c> dump, in that order, one per
        /// line, with no trailing newline.
        /// </summary>
        /// <param name="includeSubfiles">Whether to include the <c>subfiles</c> line.</param>
        /// <remarks>
        /// That order is contiguous exactly as written, for every command that has
        /// these fields: a command's own fields — <c>convert</c>'s <c>pages</c>;
        /// <c>extract</c>'s <c>include</c>/<c>exclude</c>/<c>images</c>/
        /// <c>make-dirs</c>/<c>sub-id</c>; <c>list</c>'s and <c>meta</c>'s
        /// <c>json</c>; every command's <c>cardinality</c> — always come after this
        /// whole block, never inside it, so a caller appends this string and then
        /// its own lines.
        ///
        /// All four language implementations must produce byte-identical output for
        /// each line.
        /// </remarks>
        public string ToCanonicalString(bool includeSubfiles = true)
		{
			var sb = new StringBuilder();

			string depth = Input.EffectiveMaxDepth?.ToString() ?? "unlimited";
			sb.Append("walk depth=").Append(depth)
				.Append(" follow=").Append(Input.FollowSymlinks ? "yes" : "no")
				.Append(" sort=").Append(Input.Sort ? "yes" : "no")
				.Append(" hidden=").Append(Input.IncludeHidden ? "yes" : "no")
				.Append('\n');

			sb.Append("match ").Append(Input.Matcher?.ToString() ?? "none").Append('\n');

			sb.Append("types include=").Append(FormatIdList(Input.IncludeTypes))
				.Append(" exclude=").Append(FormatIdList(Input.ExcludeTypes))
				.Append('\n');

			if (includeSubfiles)
			{
				sb.Append("subfiles ");
				if (Subfiles.Enabled)
					sb.Append("enabled depth=").Append(Subfiles.MaxDepth?.ToString() ?? "unlimited");
				else
					sb.Append("disabled");
				sb.Append('\n');
			}

			sb.Append("on-error ").Append(Batch.OnError == OnErrorBehaviour.Stop ? "stop" : "skip");

			return sb.ToString();
		}

		private static string FormatIdList(List<string> ids)
		{
			return ids.Count == 0 ? "none" : string.Join(",", ids);
		}
	}

	/// <summary>Which inputs to process, and how to find them.</summary>
	public sealed class InputOptions
	{
		/// <summary>Positional paths: files or directories, in the order given.</summary>
		public List<string> Paths { get; } = new List<string>();

		/// <summary><c>--files-from &lt;file&gt;</c>; "-" means read from stdin.</summary>
		public string FilesFrom { get; set; }

		/// <summary>
		/// <c>--max-depth</c>. Null means it was not given; directories are walked
		/// recursively by default.
		/// </summary>
		/// <remarks>
		/// <c>--no-recurse</c> deliberately does <em>not</em> write this field. Two
		/// arguments sharing one field destroys the information validation needs:
		/// if <c>--no-recurse</c> wrote 1 here, then <c>--max-depth 5 --no-recurse</c>
		/// would be indistinguishable from a plain <c>--max-depth 1</c>, and the
		/// contradiction could never be reported. Each argument records its own
		/// presence; <see cref="EffectiveMaxDepth"/> derives the answer.
		/// </remarks>
		public int? MaxDepth { get; set; }

		/// <summary><c>--no-recurse</c>, which means a depth of 1.</summary>
		public bool NoRecurseWasGiven { get; set; }

		/// <summary>
		/// The depth actually used by the walk, once the two arguments above are
		/// reconciled. Null means unlimited.
		/// </summary>
		public int? EffectiveMaxDepth => NoRecurseWasGiven ? 1 : MaxDepth;

		/// <summary><c>-L, --follow</c>: enter symlinked directories.</summary>
		public bool FollowSymlinks { get; set; }

		/// <summary><c>--sort</c>: order entries before processing, for reproducible runs.</summary>
		public bool Sort { get; set; }

		/// <summary><c>--include-hidden</c>.</summary>
		public bool IncludeHidden { get; set; }

		/// <summary><c>-g/--glob</c>, repeatable. A leading <c>!</c> excludes.</summary>
		/// <remarks>
		/// Kept apart from <see cref="RegexPatterns"/> rather than both compiling into
		/// <see cref="Matcher"/> directly: two arguments writing one field would let
		/// <c>-g "**/*.pdf" --regex ".*\.docx"</c> silently drop one of the two, and
		/// the validator could not see that both were given.
		/// </remarks>
		public List<string> GlobPatterns { get; } = new List<string>();

		/// <summary><c>--regex</c>, repeatable.</summary>
		public List<string> RegexPatterns { get; } = new List<string>();

		/// <summary>
		/// The compiled path selection, built from whichever of the two pattern lists
		/// was given. Null means everything matches.
		/// </summary>
		/// <remarks>
		/// Populated by <see cref="Cli.CommandLineApp.Bind"/>. <see cref="OptionsValidator"/>
		/// is where the two pattern languages are rejected if both appear, but it does
		/// not build this itself. Reading it before binding gives null.
		/// </remarks>
		public NameMatcher Matcher { get; set; }

		/// <summary><c>--include-types</c>: format ids to keep. Empty means no restriction.</summary>
		public List<string> IncludeTypes { get; } = new List<string>();

		/// <summary><c>--exclude-types</c>: format ids to drop.</summary>
		public List<string> ExcludeTypes { get; } = new List<string>();
	}

	/// <summary>Where output goes. See <see cref="Planning.OutputPlanner"/> for the rules.</summary>
	public sealed class OutputOptions
	{
		/// <summary><c>-o, --out-dir</c>. Safe for every format.</summary>
		public string OutDir { get; set; }

		/// <summary><c>--out-file</c>. Exactly one artifact at exactly this path.</summary>
		public string OutFile { get; set; }

		/// <summary><c>--out-name</c>: filename template used inside <see cref="OutDir"/>.</summary>
		public OutputNameTemplate NameTemplate { get; set; }

		/// <summary><c>--stdout</c>, or <c>--out-file -</c>.</summary>
		public bool UseStdout { get; set; }

		/// <summary><c>--overwrite</c>: replace existing outputs instead of erroring.</summary>
		public bool Overwrite { get; set; }
	}

	/// <summary>Recursion into documents embedded in other documents.</summary>
	public sealed class SubfileOptions
	{
		/// <summary><c>-s, --subfiles</c>.</summary>
		public bool Enabled { get; set; }

		/// <summary>
		/// <c>--subfile-depth</c>. Null means unlimited. Entirely separate from
		/// <see cref="InputOptions.MaxDepth"/>, which is directory depth.
		/// </summary>
		public int? MaxDepth { get; set; }
	}

	/// <summary>How to open password-protected documents.</summary>
	public sealed class PasswordOptions
	{
		/// <summary><c>-p, --password</c>, repeatable.</summary>
		public List<string> Passwords { get; } = new List<string>();

		/// <summary><c>--password-file</c>: one candidate password per line.</summary>
		public string PasswordFile { get; set; }

		/// <summary><c>--password-prompt</c>: ask interactively. Requires a terminal.</summary>
		public bool Prompt { get; set; }
	}

	/// <summary>Engine tunables and text handling.</summary>
	public sealed class ProcessingOptions
	{
		/// <summary>
		/// <c>--options</c>: the semicolon-separated string, passed to the engine
		/// untouched. The CLI does not parse or validate it — run the <c>options</c>
		/// command to see the valid keys.
		/// </summary>
		public string EngineOptions { get; set; }

		/// <summary><c>--codepage</c>: default codepage when the document does not declare one.</summary>
		public string Codepage { get; set; }

		/// <summary><c>--temp-dir</c>.</summary>
		public string TempDir { get; set; }
	}

	/// <summary>What happens when one file in a batch fails.</summary>
	public sealed class BatchOptions
	{
		/// <summary>
		/// <c>--on-error</c>, also written by <c>--fail-fast</c>. Skipping still exits
		/// non-zero if anything failed.
		/// </summary>
		public OnErrorBehaviour OnError { get; set; } = OnErrorBehaviour.Skip;

		/// <summary>True when <c>--on-error</c> was given explicitly.</summary>
		/// <remarks>
		/// Recorded so that a contradictory pair — <c>--fail-fast --on-error skip</c> —
		/// can be rejected as a usage error rather than silently resolved. None of the
		/// four parsing libraries reliably records which of two different options came
		/// first on the command line, so a precedence rule could not have been made
		/// identical across the ports.
		/// </remarks>
		public bool OnErrorWasExplicit { get; set; }

		/// <summary>True when <c>--fail-fast</c> was given.</summary>
		public bool FailFastWasGiven { get; set; }
	}

	/// <summary>Terminal output verbosity.</summary>
	public sealed class ConsoleOptions
	{
		/// <summary><c>--color</c>.</summary>
		public bool Color { get; set; }

		/// <summary><c>--verbose</c>.</summary>
		public bool Verbose { get; set; }

		/// <summary><c>--silent</c>.</summary>
		public bool Silent { get; set; }
	}

	/// <summary>
	/// Timing, limits and debugging aids. Their output is not part of the cross-language 
	/// contract, because timings and memory figures vary per run.
	/// </summary>
	public sealed class DiagnosticOptions
	{
		public bool Time { get; set; }

		public bool LogTime { get; set; }

		public bool LogMemory { get; set; }

		public bool Progress { get; set; }

		public bool Pause { get; set; }

		/// <summary><c>--max-memory</c>, in megabytes. Null means no limit.</summary>
		public long? MaxMemory { get; set; }

		/// <summary><c>--max-time</c>, in seconds, for the whole run.</summary>
		public int? MaxTime { get; set; }

		/// <summary><c>--timeout</c>, in seconds, per document.</summary>
		public int? Timeout { get; set; }

		/// <summary>
		/// How often to report the engine's memory status, from
		/// <c>IGR_Get_Memory_Status</c>.
		/// </summary>
		/// <remarks>
		/// <list type="table">
		/// <item><term>0</term><description>never — the default</description></item>
		/// <item><term>1</term><description><c>-x</c>: once, at the end of the run</description></item>
		/// <item><term>2</term><description><c>--xx</c>: after every document, and at the end of the run</description></item>
		/// </list>
		/// A level rather than two switches, so <c>--xx</c> implies <c>-x</c> without
		/// the two needing to be reconciled.
		///
		/// Distinct from <c>--logmem</c>, which samples the process's own memory use
		/// while a document is being processed and reports the peak. This reports what
		/// the engine says about its allocations at a moment in time.
		/// </remarks>
		public int MemoryStatusLevel { get; set; }

		/// <summary>
		/// <c>--log-level</c> (or <c>--debug-log-level</c>): how much the low-level
		/// readers log. One of none, crit, error, warn, info, debug, trace.
		/// </summary>
		/// <remarks>
		/// Wired to the engine's log callbacks, which makes this the one diagnostic
		/// here a customer is likely to reach for — it is the fastest way to find out
		/// why a particular document will not parse.
		/// </remarks>
		public string LogLevel { get; set; }

		/// <summary><c>--debug-log-level</c>: the same setting under its original name.</summary>
		/// <remarks>
		/// Kept in its own field rather than both aliases writing
		/// <see cref="LogLevel"/>: they carry a value, so two arguments sharing one
		/// field would let <c>--log-level trace --debug-log-level info</c> silently
		/// drop one of the two. <see cref="EffectiveLogLevel"/> derives the answer, and
		/// the validator rejects the pair when they disagree.
		/// </remarks>
		public string DebugLogLevel { get; set; }

		/// <summary>The log level actually used, from whichever alias was given.</summary>
		public string EffectiveLogLevel => LogLevel ?? DebugLogLevel;

		/// <summary><c>--log-modules</c>: which modules log.</summary>
		public string LogModules { get; set; }

		/// <summary><c>--debug-log-modules</c>: the same setting under its original name.</summary>
		public string DebugLogModules { get; set; }

		/// <summary>The module list actually used, from whichever alias was given.</summary>
		public string EffectiveLogModules => LogModules ?? DebugLogModules;

		/// <summary><c>--debug-use-memory-stream</c>: read the document through a memory stream.</summary>
		public bool UseMemoryStream { get; set; }

		/// <summary><c>--debug-measure-io</c>: report I/O counts and volumes per document.</summary>
		public bool MeasureIo { get; set; }

		/// <summary>
		/// <c>--debug-truncate &lt;bytes&gt;</c>: read only the first N bytes of each input,
		/// for testing how gracefully a format handles a damaged or partial file.
		/// </summary>
		public long? TruncateInputBytes { get; set; }
	}

	/// <summary>How to find and license the Document Filters engine.</summary>
	/// <remarks>
	/// Not in the CLI design document, but nothing runs without them. Both fall back
	/// to the environment variables the other published samples already use.
	/// </remarks>
	public sealed class LibraryOptions
	{
		/// <summary><c>--license-key</c>, falling back to <c>DF_LICENSE_KEY</c>.</summary>
		public string LicenseKey { get; set; }

		/// <summary>
		/// <c>--library</c>, falling back to <c>DF_PATH</c>, then to this tool's own
		/// executable directory — see <see cref="Execution.DocumentFiltersSession.ResolveLibraryPath"/>.
		/// Null here means neither argument nor environment variable was given, not
		/// that there is nowhere to look.
		/// </summary>
		public string LibraryPath { get; set; }
	}

	/// <summary>Values for <c>--on-error</c>.</summary>
	public enum OnErrorBehaviour
	{
		/// <summary>Continue with the next file; still exit non-zero at the end.</summary>
		Skip,

		/// <summary>Stop the run at the first failure.</summary>
		Stop
	}
}
