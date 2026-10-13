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
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Configuration
{
	/// <summary>
	/// The rules that no single argument can enforce on its own, because they are
	/// relations between two or more arguments.
	/// </summary>
	/// <remarks>
	/// Everything here is decidable from the command line alone. Rules that need to
	/// know how many input files actually exist — <c>--out-file</c> with more than
	/// one input, for instance — belong in <see cref="Planning.OutputPlanner"/>,
	/// which runs after the filesystem walk.
	/// </remarks>
	public static class OptionsValidator
	{
		/// <summary>Runs every shared-option rule.</summary>
		/// <exception cref="Runtime.UsageError">A rule was broken.</exception>
		public static void Validate(SharedOptions shared)
		{
			CheckOutDirNamesADirectory(shared);
			CheckOutputTargetsExclusive(shared);
			CheckOutNameNeedsOutDir(shared);
			CheckStdoutHasNoDirectory(shared);
			CheckStdoutHasNoFileTarget(shared);
			CheckLogAliasesConsistent(shared);
			CheckRecursionDepthConsistent(shared);
			CheckVerbosityConsistent(shared);
			CheckPatternLanguagesExclusive(shared);
			CheckErrorPolicyConsistent(shared);
		}

		/// <summary>Runs the <c>convert</c>-specific rules in addition to the shared ones.</summary>
		public static void ValidateConvert(SharedOptions shared, ConvertOptions options)
		{
			Validate(shared);
			CheckFormatIsProvided(options);
			CheckCombineUnsupportedFormat(options);
			CheckStdoutNeedsSingleFileFormat(shared, options);
			CheckCombineHasAFileToCombineInto(shared, options);
			CheckOutFileNeedsSingleFileFormat(shared, options);
			CheckTemplateHasPageForPerPageFormat(shared, options);
			CheckFingerprintHasNoNormalOutput(shared, options);
			CheckReportsNeedCanvasFormat(options);
		}

		/// <summary>
		/// A structure report needs a format that opens the document paginated.
		/// </summary>
		/// <remarks>
		/// Not a preference — one document open cannot serve both. Page structures
		/// only exist on a paginated handle, and text extraction only works on a text
		/// one, so pairing a report with a text format fails whichever way the format
		/// bit goes: opening paginated leaves <c>TextConversion</c> with
		/// <c>IGR_E_NOT_VALID_FOR_THIS_CLASS</c> and a zero-byte file, and opening as
		/// text leaves the report reading a handle the engine rejects
		/// (<c>Invalid DocHandle</c> for <c>--bookmarks</c>, which needs a paginated
		/// handle despite never touching a page). <c>docpack</c> is in the same
		/// position: it opens with no format bit at all.
		///
		/// So the combination is refused before anything opens, which is also what
		/// doc2text does — its report flags and its conversions are alternative output
		/// modes, never both at once. One code, two messages, because <c>text</c> and
		/// <c>docpack</c> are unusable here for different reasons.
		///
		/// Reads <c>--to</c>, which is the only thing a conversion format comes from: the planner
		/// resolves targets from <c>--to</c> alone, so that is the format the run is
		/// described as producing, and a rule that judged a different one would
		/// contradict the plan it validates.
		/// </remarks>
		internal static void CheckReportsNeedCanvasFormat(ConvertOptions options)
		{
			if (!options.Reports.Any)
				return;

			FormatSpec format = options.Format;
			if (format == null || format.Kind == FormatKind.Canvas)
				return;

			string report = options.Reports.Requested[0];
			throw new UsageError(ErrorCodes.ReportNeedsHiDef, format.Kind == FormatKind.DocPack
				? $"{report} needs a high-definition format; '{format.Token}' writes a document pack, not pages"
				: $"{report} needs a high-definition format; '{format.Token}' is text-mode only");
		}

		/// <summary><c>--to</c> is <c>convert</c>'s one required argument.</summary>
		/// <remarks>
		/// Nothing at the parser level enforces a required argument — <c>ArgumentArity</c>
		/// only ever means "does this token take a value", not "must this argument
		/// appear" — so an omitted <c>--to</c> left <see cref="ConvertOptions.Format"/>
		/// null. Every other rule in <see cref="ValidateConvert"/> reads
		/// <c>options.Format</c> unconditionally, so this has to run first: otherwise
		/// the first user-visible symptom of a missing <c>--to</c> would have been a
		/// <see cref="NullReferenceException"/> escaping uncaught past <see cref="Program"/>,
		/// which only catches <see cref="CliError"/>, instead of the usage error the
		/// mistake actually is.
		/// </remarks>
		internal static void CheckFormatIsProvided(ConvertOptions options)
		{
			if (options.Format == null)
				throw new UsageError(ErrorCodes.MissingArgument, "--to is required");
		}

		/// <summary>Runs the <c>extract</c>-specific rules in addition to the shared ones.</summary>
		public static void ValidateExtract(SharedOptions shared, ExtractOptions options)
		{
			Validate(shared);
			CheckDirectoryLayoutConsistent(options);
		}

		/// <summary><c>--out-dir ""</c> names no directory, and is refused rather than read as one.</summary>
		/// <remarks>
		/// Every output path is the directory, a slash and a name, so an empty directory
		/// does not mean "here": it turns <c>a.pdf</c> into <c>/a.pdf</c>, and an
		/// archive's <c>tmp/file</c> into <c>/tmp/file</c> — a path relative to the root
		/// of the drive, chosen by the container. The current directory is <c>.</c>.
		/// </remarks>
		internal static void CheckOutDirNamesADirectory(SharedOptions shared)
		{
			if (shared.Output.OutDir != null && shared.Output.OutDir.Length == 0)
				throw new UsageError(ErrorCodes.BadValue, "--out-dir does not accept the value ''");
		}

		/// <summary><c>--out-file</c> and <c>--out-dir</c> are mutually exclusive.</summary>
		internal static void CheckOutputTargetsExclusive(SharedOptions shared)
		{
			if (shared.Output.OutDir != null && shared.Output.OutFile != null)
				throw Contradiction("--out-dir", "--out-file");
		}

		/// <summary><c>--out-name</c> is a template for <c>--out-dir</c> and means nothing without it.</summary>
		internal static void CheckOutNameNeedsOutDir(SharedOptions shared)
		{
			if (shared.Output.NameTemplate != null && shared.Output.OutDir == null)
				throw new UsageError(ErrorCodes.OutNameNeedsOutDir,
					"--out-name is a template for --out-dir and has no meaning without it");
		}

		/// <summary><c>--stdout</c> cannot be combined with a directory target.</summary>
		internal static void CheckStdoutHasNoDirectory(SharedOptions shared)
		{
			if (shared.Output.OutDir != null && shared.Output.UseStdout)
				throw Contradiction("--out-dir", "--stdout");
		}

		/// <summary>
		/// <c>--stdout</c> cannot be combined with a file target either.
		/// </summary>
		/// <remarks>
		/// <c>--out-file -</c> is the spelling that means stdout, so it is not caught
		/// here; <c>--stdout --out-file book.pdf</c> is, because output cannot go to
		/// both places. Neither <see cref="CheckOutputTargetsExclusive"/> nor
		/// <see cref="CheckStdoutHasNoDirectory"/> covers this pair.
		/// </remarks>
		internal static void CheckStdoutHasNoFileTarget(SharedOptions shared)
		{
			if (shared.Output.OutFile != null && shared.Output.UseStdout)
				throw Contradiction("--out-file", "--stdout");
		}

		/// <summary>
		/// <c>--log-level</c> and <c>--debug-log-level</c> are the same setting, so
		/// giving both with different values is a contradiction. Likewise
		/// <c>--log-modules</c> and <c>--debug-log-modules</c>.
		/// </summary>
		/// <remarks>
		/// Agreeing values are allowed, as with <c>--fail-fast</c> alongside an
		/// explicit <c>--on-error stop</c>: redundant is not wrong.
		/// </remarks>
		internal static void CheckLogAliasesConsistent(SharedOptions shared)
		{
			DiagnosticOptions diagnostics = shared.Diagnostics;
			if (diagnostics.LogLevel != null && diagnostics.DebugLogLevel != null
				&& diagnostics.LogLevel != diagnostics.DebugLogLevel)
			{
				throw Contradiction("--log-level " + diagnostics.LogLevel, "--debug-log-level " + diagnostics.DebugLogLevel);
			}
			if (diagnostics.LogModules != null && diagnostics.DebugLogModules != null
				&& diagnostics.LogModules != diagnostics.DebugLogModules)
			{
				throw Contradiction("--log-modules " + diagnostics.LogModules, "--debug-log-modules " + diagnostics.DebugLogModules);
			}
		}

		/// <summary><c>--no-recurse</c> contradicts an explicit <c>--max-depth</c> other than 1.</summary>
		/// <remarks>
		/// Decidable only because the two arguments write separate fields; if
		/// <c>--no-recurse</c> wrote the depth, this rule could not exist.
		/// </remarks>
		internal static void CheckRecursionDepthConsistent(SharedOptions shared)
		{
			if (shared.Input.MaxDepth.HasValue && shared.Input.MaxDepth.Value != 1 && shared.Input.NoRecurseWasGiven)
				throw Contradiction("--max-depth", "--no-recurse");
		}

		/// <summary><c>--verbose</c> and <c>--silent</c> contradict each other.</summary>
		internal static void CheckVerbosityConsistent(SharedOptions shared)
		{
			if (shared.Console.Verbose && shared.Console.Silent)
				throw Contradiction("--verbose", "--silent");
		}

		/// <summary>
		/// <c>-g/--glob</c> and <c>--regex</c> are two ways of saying the same thing,
		/// and cannot both be given.
		/// </summary>
		/// <remarks>
		/// Glob is the idiom for ordinary selection and regex the power mode for
		/// patterns a glob cannot express; mixing them in one selection has no
		/// coherent meaning, and combining them silently would make the result depend
		/// on argument order.
		/// </remarks>
		internal static void CheckPatternLanguagesExclusive(SharedOptions shared)
		{
			if (shared.Input.GlobPatterns.Count > 0 && shared.Input.RegexPatterns.Count > 0)
				throw Contradiction("--glob", "--regex");
		}

		/// <summary>
		/// <c>--fail-fast</c> contradicts an explicit <c>--on-error skip</c>.
		/// </summary>
		/// <remarks>
		/// Rejected rather than resolved by precedence: the two arguments write the same
		/// setting, and no portable ordering rule exists across McMaster, argparse,
		/// picocli and CLI11.
		/// </remarks>
		internal static void CheckErrorPolicyConsistent(SharedOptions shared)
		{
			if (shared.Batch.OnErrorWasExplicit && shared.Batch.OnError == OnErrorBehaviour.Skip && shared.Batch.FailFastWasGiven)
				throw Contradiction("--on-error skip", "--fail-fast");
		}

		/// <summary><c>--combine</c> needs a format that can hold several documents.</summary>
		/// <remarks>
		/// Keyed off <see cref="Planning.FormatSpec.SupportsCombine"/>, not
		/// <c>IsSingleFile</c>. Two different formats fail here for two different
		/// reasons, and the message says which: a per-page format writes one file per
		/// page and has nothing to merge into, while <c>docpack</c> writes a single file
		/// but holds exactly one document, because <c>Extractor.MakeDocPack</c> takes
		/// one extractor.
		/// </remarks>
		internal static void CheckCombineUnsupportedFormat(ConvertOptions options)
		{
			if (!options.Combine || options.Format.SupportsCombine)
				return;

			if (!options.Format.IsSingleFile)
			{
				throw new UsageError(ErrorCodes.CombineUnsupported,
					$"--combine needs a format that writes one file; '{options.Format.Token}' writes one per page");
			}

			throw new UsageError(ErrorCodes.CombineUnsupported,
				$"--combine cannot merge documents into '{options.Format.Token}'; each document makes its own pack");
		}

		/// <summary><c>--combine</c> needs an output file to combine into.</summary>
		/// <remarks>
		/// Execution combines only when it has a target to combine into, so with
		/// <c>--stdout</c> it silently did not combine: each document was converted on
		/// its own and the results ran into one another down the pipe, which for a
		/// single-file format like PDF is several complete documents end to end rather
		/// than one. The user asked for one document and got a stream no reader can
		/// open, with exit 0 to say it worked.
		///
		/// Refused rather than implemented: combining into a stream would mean a canvas
		/// that writes to stdout and outlives every document, which is a feature, not a
		/// fix. <c>--out-file</c> names the file to combine into, and the result can be
		/// piped from there.
		///
		/// Streaming several documents is still allowed without <c>--combine</c>: for
		/// text that is concatenation and exactly what a pipeline wants.
		/// </remarks>
		internal static void CheckCombineHasAFileToCombineInto(SharedOptions shared, ConvertOptions options)
		{
			if (options.Combine && shared.Output.UseStdout)
				throw Contradiction("--combine", "--stdout");
		}

		/// <summary><c>--stdout</c> can only stream a format that writes a single file.</summary>
		internal static void CheckStdoutNeedsSingleFileFormat(SharedOptions shared, ConvertOptions options)
		{
			if (shared.Output.UseStdout && !options.Format.IsSingleFile)
			{
				throw new UsageError(ErrorCodes.StdoutNeedsSingleFile,
					$"--stdout can only stream a format that writes one file; '{options.Format.Token}' writes one per page");
			}
		}

		/// <summary><c>--out-file</c> can only name a format that writes a single file.</summary>
		/// <remarks>
		/// The same rule as the one above, for the other way of naming one output. It is
		/// decidable from the command line alone, so it belongs here rather than in the
		/// planner — which matters, because the planner's
		/// <c>E_OUTFILE_MULTI_INPUT</c> only fires once there is more than one input and
		/// would otherwise leave <c>--to png --out-file one.png a.docx</c> unreported.
        /// </remarks>
		internal static void CheckOutFileNeedsSingleFileFormat(SharedOptions shared, ConvertOptions options)
		{
			if (shared.Output.OutFile != null && !options.Format.IsSingleFile)
			{
				throw new UsageError(ErrorCodes.OutFileNeedsSingleFile,
					$"--out-file can only name a format that writes one file; '{options.Format.Token}' writes one per page");
			}
		}

		/// <summary>
		/// A per-page format needs a <c>--out-name</c> template containing
		/// <c>{page}</c>, or every page resolves to the same path.
		/// </summary>
		/// <remarks>
		/// Without this, <c>--to png --out-name "{stem}.{ext}"</c> either fails after
		/// writing page one, or — with <c>--overwrite</c> — silently leaves only the
		/// last page. The default template already includes the page number; this rule
		/// covers the case where the user supplied their own.
		/// </remarks>
		internal static void CheckTemplateHasPageForPerPageFormat(SharedOptions shared, ConvertOptions options)
		{
			if (options.Format.IsSingleFile)
				return;

			OutputNameTemplate template = shared.Output.NameTemplate ?? OutputNameTemplate.Default(false);
			if (!template.UsesPage)
			{
				throw new UsageError(ErrorCodes.TemplateNeedsPage,
					"--out-name must contain {page} when the format writes one file per page");
			}
		}

		/// <summary>
		/// <c>--fingerprint</c> renders a page only to read a fingerprint attribute
		/// off it; the render itself is never kept, so every argument whose entire
		/// purpose is to control or expose that now-discarded content contradicts it.
		/// </summary>
		/// <remarks>
		/// Confirmed against doc2text's own <c>--fingerprint</c> (<c>main.cpp</c>):
		/// it forces a throwaway image render and throws it away through a temporary
		/// stream (<c>hidecontent</c>) rather than the requested target, whenever the
		/// selected format is not already an image format. <see cref="Execution.CanvasConversion"/>
		/// does the equivalent — a discard stream in place of the real target — so
		/// <c>--stdout</c>, <c>--view</c>, <c>--combine</c>, <c>--out-file</c> and
		/// <c>--out-dir</c> would each silently do nothing under <c>--fingerprint</c>
		/// rather than the thing their name promises, which is exactly what a usage
		/// error is for. <c>--redact</c>, <c>--form</c> and <c>--watermark</c> are
		/// unaffected: they decorate the same throwaway render fingerprinting already
		/// needs, so there is nothing for them to contradict.
		/// </remarks>
		internal static void CheckFingerprintHasNoNormalOutput(SharedOptions shared, ConvertOptions options)
		{
			if (!options.Fingerprint)
				return;

			if (shared.Output.UseStdout)
				throw Contradiction("--fingerprint", "--stdout");
			if (options.View)
				throw Contradiction("--fingerprint", "--view");
			if (options.Combine)
				throw Contradiction("--fingerprint", "--combine");
			if (shared.Output.OutFile != null)
				throw Contradiction("--fingerprint", "--out-file");
			if (shared.Output.OutDir != null)
				throw Contradiction("--fingerprint", "--out-dir");
		}

		/// <summary><c>--make-dirs</c> and <c>--flat</c> contradict each other.</summary>
		internal static void CheckDirectoryLayoutConsistent(ExtractOptions options)
		{
			if (options.MakeDirsWasGiven && options.FlatWasGiven)
				throw Contradiction("--make-dirs", "--flat");
		}

		/// <summary>
		/// Builds the "cannot be used together" message every contradictory pair
		/// shares, naming each side by its long form — with its current value
		/// attached when the caller passes one — in argument-table order.
		/// </summary>
		private static UsageError Contradiction(string first, string second)
		{
			return new UsageError(ErrorCodes.MutuallyExclusive, $"{first} and {second} cannot be used together");
		}
	}
}
