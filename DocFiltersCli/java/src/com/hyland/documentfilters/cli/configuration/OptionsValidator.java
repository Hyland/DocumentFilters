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

package com.hyland.documentfilters.cli.configuration;

import com.hyland.documentfilters.cli.planning.FormatKind;
import com.hyland.documentfilters.cli.planning.FormatSpec;
import com.hyland.documentfilters.cli.planning.OutputNameTemplate;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.UsageError;

/**
 * The rules that no single argument can enforce on its own, because they are
 * relations between two or more arguments.
 *
 * <p>Everything here is decidable from the command line alone. Rules that need to
 * know how many input files actually exist --- {@code --out-file} with more than
 * one input, for instance --- belong in {@code OutputPlanner}, which runs after the
 * filesystem walk.
 *
 * <p>The individual checks are public where the reference's are internal: Java
 * has no assembly-wide visibility, and the suite lives in its own package.
 */
public final class OptionsValidator
{
	private OptionsValidator()
	{
	}

	/** Runs every shared-option rule. */
	public static void validate(SharedOptions shared)
	{
		checkOutDirNamesADirectory(shared);
		checkOutputTargetsExclusive(shared);
		checkOutNameNeedsOutDir(shared);
		checkStdoutHasNoDirectory(shared);
		checkStdoutHasNoFileTarget(shared);
		checkLogAliasesConsistent(shared);
		checkRecursionDepthConsistent(shared);
		checkSubfileDepthNeedsSubfiles(shared);
		checkVerbosityConsistent(shared);
		checkPatternLanguagesExclusive(shared);
		checkErrorPolicyConsistent(shared);
	}

	/** Runs the {@code convert}-specific rules in addition to the shared ones. */
	public static void validateConvert(SharedOptions shared, ConvertOptions options)
	{
		validate(shared);
		checkFormatIsProvided(options);
		checkCombineUnsupportedFormat(options);
		checkStdoutNeedsSingleFileFormat(shared, options);
		checkCombineHasAFileToCombineInto(shared, options);
		checkOutFileNeedsSingleFileFormat(shared, options);
		checkTemplateHasPageForPerPageFormat(shared, options);
		checkFingerprintHasNoNormalOutput(shared, options);
		checkReportsNeedCanvasFormat(options);
	}

	/**
	 * A structure report needs a format that opens the document paginated.
	 *
	 * <p>Not a preference --- one document open cannot serve both. Page structures
	 * only exist on a paginated handle, and text extraction only works on a text
	 * one, so pairing a report with a text format fails whichever way the format
	 * bit goes: opening paginated leaves {@code TextConversion} with
	 * {@code IGR_E_NOT_VALID_FOR_THIS_CLASS} and a zero-byte file, and opening as
	 * text leaves the report reading a handle the engine rejects
	 * ({@code Invalid DocHandle} for {@code --bookmarks}, which needs a paginated
	 * handle despite never touching a page). {@code docpack} is in the same
	 * position: it opens with no format bit at all.
	 *
	 * <p>So the combination is refused before anything opens, which is also what
	 * doc2text does --- its report flags and its conversions are alternative output
	 * modes, never both at once. One code, two messages, because {@code text} and
	 * {@code docpack} are unusable here for different reasons.
	 *
	 * <p>Reads {@code --to}, which is the only thing a conversion format comes from: the planner
	 * resolves targets from {@code --to} alone, so that is the format the run is
	 * described as producing, and a rule that judged a different one would
	 * contradict the plan it validates.
	 */
	public static void checkReportsNeedCanvasFormat(ConvertOptions options)
	{
		if (!options.reports.any())
			return;

		FormatSpec format = options.format;
		if (format == null || format.getKind() == FormatKind.CANVAS)
			return;

		String report = options.reports.requested().get(0);
		throw new UsageError(ErrorCodes.REPORT_NEEDS_HI_DEF, format.getKind() == FormatKind.DOCPACK
			? report + " needs a high-definition format; '" + format.getToken() + "' writes a document pack, not pages"
			: report + " needs a high-definition format; '" + format.getToken() + "' is text-mode only");
	}

	/**
	 * {@code --to} is {@code convert}'s one required argument.
	 *
	 * <p>Nothing at the parser level enforces a required argument --- an argument's
	 * arity only ever means "does this token take a value", not "must this argument
	 * appear" --- so an omitted {@code --to} leaves {@link ConvertOptions#format}
	 * null. Every other rule in {@link #validateConvert} reads {@code options.format}
	 * unconditionally, so this has to run first: otherwise the first user-visible
	 * symptom of a missing {@code --to} would be a {@link NullPointerException}
	 * escaping past {@code Program}, which only catches {@code CliError}, instead
	 * of the usage error the mistake actually is.
	 */
	public static void checkFormatIsProvided(ConvertOptions options)
	{
		if (options.format == null)
			throw new UsageError(ErrorCodes.MISSING_ARGUMENT, "--to is required");
	}

	/** Runs the {@code extract}-specific rules in addition to the shared ones. */
	public static void validateExtract(SharedOptions shared, ExtractOptions options)
	{
		validate(shared);
		checkDirectoryLayoutConsistent(options);
	}

	/**
	 * {@code --out-dir ""} names no directory, and is refused rather than read as one.
	 *
	 * <p>Every output path is the directory, a slash and a name, so an empty directory
	 * does not mean "here": it turns {@code a.pdf} into {@code /a.pdf}, and an
	 * archive's {@code tmp/file} into {@code /tmp/file} --- a path relative to the root
	 * of the drive, chosen by the container. The current directory is {@code .}.
	 */
	public static void checkOutDirNamesADirectory(SharedOptions shared)
	{
		if (shared.output.outDir != null && shared.output.outDir.isEmpty())
			throw new UsageError(ErrorCodes.BAD_VALUE, "--out-dir does not accept the value ''");
	}

	/** {@code --out-file} and {@code --out-dir} are mutually exclusive. */
	public static void checkOutputTargetsExclusive(SharedOptions shared)
	{
		if (shared.output.outDir != null && shared.output.outFile != null)
			throw contradiction("--out-dir", "--out-file");
	}

	/** {@code --out-name} is a template for {@code --out-dir} and means nothing without it. */
	public static void checkOutNameNeedsOutDir(SharedOptions shared)
	{
		if (shared.output.nameTemplate != null && shared.output.outDir == null)
			throw new UsageError(ErrorCodes.OUT_NAME_NEEDS_OUT_DIR,
				"--out-name is a template for --out-dir and has no meaning without it");
	}

	/** {@code --stdout} cannot be combined with a directory target. */
	public static void checkStdoutHasNoDirectory(SharedOptions shared)
	{
		if (shared.output.outDir != null && shared.output.useStdout)
			throw contradiction("--out-dir", "--stdout");
	}

	/**
	 * {@code --stdout} cannot be combined with a file target either.
	 *
	 * <p>{@code --out-file -} is the spelling that means stdout, so it is not caught
	 * here; {@code --stdout --out-file book.pdf} is, because output cannot go to
	 * both places. Neither {@link #checkOutputTargetsExclusive} nor
	 * {@link #checkStdoutHasNoDirectory} covers this pair.
	 */
	public static void checkStdoutHasNoFileTarget(SharedOptions shared)
	{
		if (shared.output.outFile != null && shared.output.useStdout)
			throw contradiction("--out-file", "--stdout");
	}

	/**
	 * {@code --log-level} and {@code --debug-log-level} are the same setting, so
	 * giving both with different values is a contradiction. Likewise
	 * {@code --log-modules} and {@code --debug-log-modules}.
	 *
	 * <p>Agreeing values are allowed, as with {@code --fail-fast} alongside an
	 * explicit {@code --on-error stop}: redundant is not wrong.
	 */
	public static void checkLogAliasesConsistent(SharedOptions shared)
	{
		DiagnosticOptions diagnostics = shared.diagnostics;
		if (diagnostics.logLevel != null && diagnostics.debugLogLevel != null
			&& !diagnostics.logLevel.equals(diagnostics.debugLogLevel))
		{
			throw contradiction("--log-level " + diagnostics.logLevel, "--debug-log-level " + diagnostics.debugLogLevel);
		}
		if (diagnostics.logModules != null && diagnostics.debugLogModules != null
			&& !diagnostics.logModules.equals(diagnostics.debugLogModules))
		{
			throw contradiction("--log-modules " + diagnostics.logModules, "--debug-log-modules " + diagnostics.debugLogModules);
		}
	}

	/**
	 * {@code --no-recurse} contradicts an explicit {@code --max-depth} other than 1.
	 *
	 * <p>Decidable only because the two arguments write separate fields; if
	 * {@code --no-recurse} wrote the depth, this rule could not exist.
	 */
	public static void checkRecursionDepthConsistent(SharedOptions shared)
	{
		if (shared.input.maxDepth != null && shared.input.maxDepth != 1 && shared.input.noRecurseWasGiven)
			throw contradiction("--max-depth", "--no-recurse");
	}

	/**
	 * {@code --subfile-depth} limits how deep {@code --subfiles} descends and means
	 * nothing without it.
	 *
	 * <p>Rejected rather than ignored, which is what it used to be. The two arguments
	 * write separate fields --- {@code enabled} and {@code maxDepth} --- so a depth
	 * given without {@code -s} left the walk disabled and the depth unread, and the
	 * run reported success having descended into nothing. That is the failure mode
	 * worth spending a rule on: not a crash, which is noticed, but a command line
	 * that asks for one thing, does another, and exits 0.
	 *
	 * <p>Decidable only because the fields are separate; if {@code --subfile-depth}
	 * also enabled the walk, the user's intent and the resulting state would be
	 * indistinguishable and this rule could not exist. That is the same property
	 * {@link #checkRecursionDepthConsistent} depends on, one directory level up.
	 */
	public static void checkSubfileDepthNeedsSubfiles(SharedOptions shared)
	{
		if (shared.subfiles.maxDepth != null && !shared.subfiles.enabled)
		{
			throw new UsageError(ErrorCodes.SUBFILE_DEPTH_NEEDS_SUBFILES,
				"--subfile-depth limits how deep --subfiles descends and has no meaning without it");
		}
	}

	/** {@code --verbose} and {@code --silent} contradict each other. */
	public static void checkVerbosityConsistent(SharedOptions shared)
	{
		if (shared.console.verbose && shared.console.silent)
			throw contradiction("--verbose", "--silent");
	}

	/**
	 * {@code -g/--glob} and {@code --regex} are two ways of saying the same thing,
	 * and cannot both be given.
	 *
	 * <p>Glob is the idiom for ordinary selection and regex the power mode for
	 * patterns a glob cannot express; mixing them in one selection has no
	 * coherent meaning, and combining them silently would make the result depend
	 * on argument order.
	 */
	public static void checkPatternLanguagesExclusive(SharedOptions shared)
	{
		if (!shared.input.globPatterns.isEmpty() && !shared.input.regexPatterns.isEmpty())
			throw contradiction("--glob", "--regex");
	}

	/**
	 * {@code --fail-fast} contradicts an explicit {@code --on-error skip}.
	 *
	 * <p>Rejected rather than resolved by precedence: the two arguments write the same
	 * setting, and the binder records each argument's occurrences rather than the
	 * relative order of two different arguments. All four implementations could be
	 * taught to record that order now that none of them delegates to a parsing library,
	 * and none is: a tie-break is a rule four ports would have to agree on for ever,
	 * where an error is a rule they already agree on.
	 */
	public static void checkErrorPolicyConsistent(SharedOptions shared)
	{
		if (shared.batch.onErrorWasExplicit && shared.batch.onError == OnErrorBehaviour.SKIP && shared.batch.failFastWasGiven)
			throw contradiction("--on-error skip", "--fail-fast");
	}

	/**
	 * {@code --combine} needs a format that can hold several documents.
	 *
	 * <p>Keyed off {@link FormatSpec#supportsCombine}, not {@code isSingleFile}. Two
	 * different formats fail here for two different reasons, and the message says
	 * which: a per-page format writes one file per page and has nothing to merge
	 * into, while {@code docpack} writes a single file but holds exactly one
	 * document, because the engine's document pack takes one extractor.
	 */
	public static void checkCombineUnsupportedFormat(ConvertOptions options)
	{
		if (!options.combine || options.format.supportsCombine())
			return;

		if (!options.format.isSingleFile())
		{
			throw new UsageError(ErrorCodes.COMBINE_UNSUPPORTED,
				"--combine needs a format that writes one file; '" + options.format.getToken() + "' writes one per page");
		}

		throw new UsageError(ErrorCodes.COMBINE_UNSUPPORTED,
			"--combine cannot merge documents into '" + options.format.getToken() + "'; each document makes its own pack");
	}

	/**
	 * {@code --combine} needs an output file to combine into.
	 *
	 * <p>Execution combines only when it has a target to combine into, so with
	 * {@code --stdout} it silently would not combine: each document would be
	 * converted on its own and the results run into one another down the pipe,
	 * which for a single-file format like PDF is several complete documents end to
	 * end rather than one.
	 *
	 * <p>Refused rather than implemented: combining into a stream would mean a canvas
	 * that writes to stdout and outlives every document, which is a feature, not a
	 * fix. {@code --out-file} names the file to combine into, and the result can be
	 * piped from there.
	 *
	 * <p>Streaming several documents is still allowed without {@code --combine}: for
	 * text that is concatenation and exactly what a pipeline wants.
	 */
	public static void checkCombineHasAFileToCombineInto(SharedOptions shared, ConvertOptions options)
	{
		if (options.combine && shared.output.useStdout)
			throw contradiction("--combine", "--stdout");
	}

	/** {@code --stdout} can only stream a format that writes a single file. */
	public static void checkStdoutNeedsSingleFileFormat(SharedOptions shared, ConvertOptions options)
	{
		if (shared.output.useStdout && !options.format.isSingleFile())
		{
			throw new UsageError(ErrorCodes.STDOUT_NEEDS_SINGLE_FILE,
				"--stdout can only stream a format that writes one file; '" + options.format.getToken() + "' writes one per page");
		}
	}

	/**
	 * {@code --out-file} can only name a format that writes a single file.
	 *
	 * <p>The same rule as the one above, for the other way of naming one output. It is
	 * decidable from the command line alone, so it belongs here rather than in the
	 * planner --- which matters, because the planner's
	 * {@code E_OUTFILE_MULTI_INPUT} only fires once there is more than one input and
	 * would otherwise leave {@code --to png --out-file one.png a.docx} unreported.
	 */
	public static void checkOutFileNeedsSingleFileFormat(SharedOptions shared, ConvertOptions options)
	{
		if (shared.output.outFile != null && !options.format.isSingleFile())
		{
			throw new UsageError(ErrorCodes.OUT_FILE_NEEDS_SINGLE_FILE,
				"--out-file can only name a format that writes one file; '" + options.format.getToken() + "' writes one per page");
		}
	}

	/**
	 * A per-page format needs a {@code --out-name} template containing
	 * {@code {page}}, or every page resolves to the same path.
	 *
	 * <p>Without this, {@code --to png --out-name "{stem}.{ext}"} either fails after
	 * writing page one, or --- with {@code --overwrite} --- silently leaves only the
	 * last page. The default template already includes the page number; this rule
	 * covers the case where the user supplied their own.
	 */
	public static void checkTemplateHasPageForPerPageFormat(SharedOptions shared, ConvertOptions options)
	{
		if (options.format.isSingleFile())
			return;

		OutputNameTemplate template = shared.output.nameTemplate != null
			? shared.output.nameTemplate
			: OutputNameTemplate.defaultFor(false);
		if (!template.usesPage())
		{
			throw new UsageError(ErrorCodes.TEMPLATE_NEEDS_PAGE,
				"--out-name must contain {page} when the format writes one file per page");
		}
	}

	/**
	 * {@code --fingerprint} renders a page only to read a fingerprint attribute
	 * off it; the render itself is never kept, so every argument whose entire
	 * purpose is to control or expose that now-discarded content contradicts it.
	 *
	 * <p>doc2text's own {@code --fingerprint} forces a throwaway image render and
	 * throws it away through a temporary stream rather than the requested target,
	 * whenever the selected format is not already an image format. The canvas
	 * conversion here does the equivalent --- a discard stream in place of the real
	 * target --- so {@code --stdout}, {@code --view}, {@code --combine},
	 * {@code --out-file} and {@code --out-dir} would each silently do nothing under
	 * {@code --fingerprint} rather than the thing their name promises, which is
	 * exactly what a usage error is for. {@code --redact}, {@code --form} and
	 * {@code --watermark} are unaffected: they decorate the same throwaway render
	 * fingerprinting already needs, so there is nothing for them to contradict.
	 */
	public static void checkFingerprintHasNoNormalOutput(SharedOptions shared, ConvertOptions options)
	{
		if (!options.fingerprint)
			return;

		if (shared.output.useStdout)
			throw contradiction("--fingerprint", "--stdout");
		if (options.view)
			throw contradiction("--fingerprint", "--view");
		if (options.combine)
			throw contradiction("--fingerprint", "--combine");
		if (shared.output.outFile != null)
			throw contradiction("--fingerprint", "--out-file");
		if (shared.output.outDir != null)
			throw contradiction("--fingerprint", "--out-dir");
	}

	/** {@code --make-dirs} and {@code --flat} contradict each other. */
	public static void checkDirectoryLayoutConsistent(ExtractOptions options)
	{
		if (options.makeDirsWasGiven && options.flatWasGiven)
			throw contradiction("--make-dirs", "--flat");
	}

	/**
	 * Builds the "cannot be used together" message every contradictory pair
	 * shares, naming each side by its long form --- with its current value
	 * attached when the caller passes one --- in argument-table order.
	 */
	public static UsageError contradiction(String first, String second)
	{
		return new UsageError(ErrorCodes.MUTUALLY_EXCLUSIVE, first + " and " + second + " cannot be used together");
	}
}
