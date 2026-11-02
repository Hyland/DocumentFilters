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

package com.hyland.documentfilters.cli.cli;

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.ExtractOptions;
import com.hyland.documentfilters.cli.configuration.FormatsOptions;
import com.hyland.documentfilters.cli.configuration.IdentifyOptions;
import com.hyland.documentfilters.cli.configuration.ListOptions;
import com.hyland.documentfilters.cli.configuration.MetaOptions;
import com.hyland.documentfilters.cli.configuration.OnErrorBehaviour;
import com.hyland.documentfilters.cli.configuration.OptionCatalogOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.planning.FormatCatalog;
import com.hyland.documentfilters.cli.planning.OutputNameTemplate;
import com.hyland.documentfilters.cli.planning.PageRange;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.function.BiConsumer;

/**
 * Every argument the tool accepts, paired with what it does. The single source of
 * truth for the command line surface.
 */
public final class ArgumentTable
{
	private static final ArgumentArity NONE = ArgumentArity.NO_VALUE;
	private static final ArgumentArity SINGLE = ArgumentArity.SINGLE_VALUE;
	private static final ArgumentArity MULTIPLE = ArgumentArity.MULTIPLE_VALUE;

	/**
	 * The processing verbs, in help-display order. These are the commands that
	 * accept shared options, so this list drives {@link #sharedFor} and the
	 * contract dump.
	 */
	public static final List<String> COMMANDS = Collections.unmodifiableList(Arrays.asList(
		"convert", "extract", "list", "identify", "meta", "formats", "options"));

	/**
	 * Everything {@code docfilters <verb>} will dispatch, including the two that
	 * take no shared options at all: {@code help} and {@code version} are verbs, but
	 * not in {@link #COMMANDS}, because they process no documents and accept no
	 * shared options.
	 */
	public static final List<String> ALL_VERBS = Collections.unmodifiableList(allVerbs());

	private ArgumentTable()
	{
	}

	private static List<String> allVerbs()
	{
		List<String> result = new ArrayList<>(COMMANDS);
		result.add("help");
		result.add("version");
		return result;
	}

	private static ArgumentDefinition<SharedOptions> shared(String template, String description, ArgumentArity arity,
		BiConsumer<SharedOptions, String> setter)
	{
		return new ArgumentDefinition<>(template, description, arity, setter);
	}

	/**
	 * The shared groups every input-processing command takes regardless of whether
	 * it also writes output or descends into subfiles: name selection, type filters,
	 * the walk, passwords, processing, batch control, console, the global switches,
	 * diagnostics and the engine location.
	 */
	private static List<ArgumentDefinition<SharedOptions>> coreArguments()
	{
		List<ArgumentDefinition<SharedOptions>> result = new ArrayList<>();
		result.addAll(matchArguments());
		result.addAll(typeArguments());
		result.addAll(walkArguments());
		result.addAll(passwordArguments());
		result.addAll(processingArguments());
		result.addAll(batchArguments());
		result.addAll(consoleArguments());
		result.addAll(global());
		result.addAll(diagnosticArguments());
		result.addAll(libraryArguments());
		return result;
	}

	/**
	 * The shared arguments a given verb accepts.
	 *
	 * <p>Composed from the group methods below rather than filtered out of one flat
	 * list. A verb simply does not call the groups it has no use for:
	 * {@code identify} writes no files, so it never calls {@link #outputArguments};
	 * {@code formats} and {@code options} take no input files, so they call neither
	 * the walk nor the selection groups.
	 *
	 * <p>Expressing it as composition rather than a group tag on each argument means
	 * there is no group name to mistype and no filter to get wrong --- an argument is
	 * on a command because that command asked for its group.
	 *
	 * @throws IllegalArgumentException {@code command} is not in {@link #COMMANDS}.
	 */
	public static List<ArgumentDefinition<SharedOptions>> sharedFor(String command)
	{
		List<ArgumentDefinition<SharedOptions>> result = new ArrayList<>();
		switch (command)
		{
			case "convert":
			case "extract":
			case "list":
				result.addAll(outputArguments());
				result.addAll(subfileArguments());
				result.addAll(coreArguments());
				break;
			case "identify":
				result.addAll(coreArguments());
				break;
			case "meta":
				result.addAll(outputArguments());
				result.addAll(coreArguments());
				break;
			case "formats":
			case "options":
				result.addAll(consoleArguments());
				result.addAll(global());
				result.addAll(libraryArguments());
				break;
			default:
				throw new IllegalArgumentException("unknown command '" + command + "'");
		}
		return result;
	}

	/** {@code -o/--out-dir}, {@code --out-file}, {@code --out-name}, {@code --stdout}, {@code --overwrite}. */
	public static List<ArgumentDefinition<SharedOptions>> outputArguments()
	{
		return Arrays.asList(
			shared("-o|--out-dir", "Write into a directory; safe for any format", SINGLE, (s, v) -> s.output.outDir = v),
			shared("--out-file", "Write one artifact to exactly this path; '-' means stdout", SINGLE, (s, v) ->
			{
				if (v.equals("-"))
					s.output.useStdout = true;
				else
					s.output.outFile = v;
			}),
			shared("--out-name", "Filename template inside --out-dir, e.g. {stem}-{page:04}.{ext}", SINGLE,
				(s, v) -> s.output.nameTemplate = OutputNameTemplate.parse(v)),
			shared("--stdout", "Stream a single-file format to stdout", NONE, (s, v) -> s.output.useStdout = true),
			shared("--overwrite", "Replace existing outputs instead of erroring", NONE, (s, v) -> s.output.overwrite = true));
	}

	/** {@code -s/--subfiles} and {@code --subfile-depth}. */
	public static List<ArgumentDefinition<SharedOptions>> subfileArguments()
	{
		return Arrays.asList(
			shared("-s|--subfiles", "Descend into subfiles, attachments and embedded documents", NONE, (s, v) -> s.subfiles.enabled = true),
			shared("--subfile-depth", "How deep to recurse into nested subfiles", SINGLE,
				(s, v) -> s.subfiles.maxDepth = ValueParsers.parseCount("--subfile-depth", v)));
	}

	/** {@code -g/--glob} and {@code --regex}: selecting inputs by path. */
	public static List<ArgumentDefinition<SharedOptions>> matchArguments()
	{
		return Arrays.asList(
			shared("-g|--glob", "Keep paths matching a glob, e.g. **/*.pdf; prefix ! to exclude", MULTIPLE,
				(s, v) -> s.input.globPatterns.add(v)),
			shared("--regex", "Same selection, but match the path with a regular expression", MULTIPLE,
				(s, v) -> s.input.regexPatterns.add(v)));
	}

	/** {@code --include-types} and {@code --exclude-types}: selecting by detected format. */
	public static List<ArgumentDefinition<SharedOptions>> typeArguments()
	{
		return Arrays.asList(
			shared("--include-types", "Semicolon list of format ids to keep", SINGLE, (s, v) ->
			{
				s.input.includeTypes.clear();
				s.input.includeTypes.addAll(ValueParsers.parseIdList(v));
			}),
			shared("--exclude-types", "Semicolon list of format ids to drop", SINGLE, (s, v) ->
			{
				s.input.excludeTypes.clear();
				s.input.excludeTypes.addAll(ValueParsers.parseIdList(v));
			}));
	}

	/** The filesystem walk: {@code --files-from}, depth, {@code --follow}, {@code --sort}, {@code --include-hidden}. */
	public static List<ArgumentDefinition<SharedOptions>> walkArguments()
	{
		return Arrays.asList(
			shared("--files-from", "Read input paths from a file, one per line; - for stdin", SINGLE, (s, v) -> s.input.filesFrom = v),
			shared("--max-depth", "Limit walk depth; --max-depth 1 stays in the given directory", SINGLE,
				(s, v) -> s.input.maxDepth = ValueParsers.parseCount("--max-depth", v)),
			shared("--no-recurse", "Don't descend into subdirectories (= --max-depth 1)", NONE, (s, v) -> s.input.noRecurseWasGiven = true),
			shared("-L|--follow", "Follow symlinked directories", NONE, (s, v) -> s.input.followSymlinks = true),
			shared("--sort", "Sort entries before processing", NONE, (s, v) -> s.input.sort = true),
			shared("--include-hidden", "Include hidden and system files", NONE, (s, v) -> s.input.includeHidden = true));
	}

	/** {@code -p/--password}, {@code --password-file}, {@code --password-prompt}. */
	public static List<ArgumentDefinition<SharedOptions>> passwordArguments()
	{
		return Arrays.asList(
			shared("-p|--password", "A candidate password, repeatable", MULTIPLE, (s, v) -> s.passwords.passwords.add(v)),
			shared("--password-file", "One candidate password per line", SINGLE, (s, v) -> s.passwords.passwordFile = v),
			shared("--password-prompt", "Prompt for passwords interactively", NONE, (s, v) -> s.passwords.prompt = true));
	}

	/** {@code --options}, {@code --codepage}, {@code --temp-dir}. */
	public static List<ArgumentDefinition<SharedOptions>> processingArguments()
	{
		return Arrays.asList(
			shared("--options", "Semicolon-separated engine options, e.g. BW_MODE=3;CODEPAGE=1252", SINGLE,
				(s, v) -> s.processing.engineOptions = v),
			shared("--codepage", "Default codepage when the document does not declare one", SINGLE, (s, v) -> s.processing.codepage = v),
			shared("--temp-dir", "Temp directory for scratch files", SINGLE, (s, v) -> s.processing.tempDir = v));
	}

	/** {@code --on-error} and {@code --fail-fast}. */
	public static List<ArgumentDefinition<SharedOptions>> batchArguments()
	{
		return Arrays.asList(
			shared("--on-error", "On a per-file failure: skip (default) or stop the run", SINGLE, (s, v) ->
			{
				s.batch.onError = ValueParsers.parseOnError(ValueParsers.foldToken(v));
				s.batch.onErrorWasExplicit = true;
			}),
			shared("--fail-fast", "Stop at the first failure (= --on-error stop)", NONE, (s, v) ->
			{
				// Guarded rather than unconditional: --on-error's own setter always has
				// the last word on onError/onErrorWasExplicit whenever it runs, so leaving
				// an explicit value alone here is what makes the pair's resolution
				// independent of argument order rather than of registration order in
				// this table.
				if (!s.batch.onErrorWasExplicit)
					s.batch.onError = OnErrorBehaviour.STOP;
				s.batch.failFastWasGiven = true;
			}));
	}

	/** {@code --color}, {@code --verbose}, {@code --silent}. Every verb takes these. */
	public static List<ArgumentDefinition<SharedOptions>> consoleArguments()
	{
		return Arrays.asList(
			shared("--color", "Colour errors and verbose output; ignored on a redirected stream", NONE, (s, v) -> s.console.color = true),
			shared("--verbose", "Print more about what the run is doing", NONE, (s, v) -> s.console.verbose = true),
			shared("--silent", "Print nothing but errors", NONE, (s, v) -> s.console.silent = true));
	}

	/** Timing, limits, tracing and the {@code --debug-*} switches. */
	public static List<ArgumentDefinition<SharedOptions>> diagnosticArguments()
	{
		return Arrays.asList(
			shared("--time", "Report how long the run took", NONE, (s, v) -> s.diagnostics.time = true),
			shared("--logtime", "Log timing for each stage", NONE, (s, v) -> s.diagnostics.logTime = true),
			shared("--logmem", "Sample and report peak process memory use", NONE, (s, v) -> s.diagnostics.logMemory = true),
			shared("--progress", "Print a progress indicator", NONE, (s, v) -> s.diagnostics.progress = true),
			shared("--pause", "Pause before exiting", NONE, (s, v) -> s.diagnostics.pause = true),
			shared("--max-memory", "Abort the run if it exceeds this many megabytes", SINGLE,
				(s, v) -> s.diagnostics.maxMemory = ValueParsers.parseMegabytes("--max-memory", v)),
			shared("--max-time", "Abort the run after this many seconds", SINGLE,
				(s, v) -> s.diagnostics.maxTime = ValueParsers.parseSeconds("--max-time", v)),
			shared("--timeout", "Abort a single document after this many seconds", SINGLE,
				(s, v) -> s.diagnostics.timeout = ValueParsers.parseSeconds("--timeout", v)),
			shared("-x", "Report engine memory status once, at the end of the run", NONE,
				(s, v) -> s.diagnostics.memoryStatusLevel = Math.max(s.diagnostics.memoryStatusLevel, 1)),
			shared("--xx", "Report engine memory status after every document too", NONE,
				(s, v) -> s.diagnostics.memoryStatusLevel = Math.max(s.diagnostics.memoryStatusLevel, 2)),
			shared("--log-level", "How much the low-level readers log", SINGLE,
				(s, v) -> s.diagnostics.logLevel = ValueParsers.foldToken(v)),
			shared("--debug-log-level", "--log-level setting under its original name", SINGLE,
				(s, v) -> s.diagnostics.debugLogLevel = ValueParsers.foldToken(v)),
			shared("--log-modules", "Which modules log", SINGLE, (s, v) -> s.diagnostics.logModules = v),
			shared("--debug-log-modules", "--log-modules setting under its original name", SINGLE,
				(s, v) -> s.diagnostics.debugLogModules = v),
			shared("--debug-use-memory-stream", "Read each document through a memory stream", NONE,
				(s, v) -> s.diagnostics.useMemoryStream = true),
			shared("--debug-measure-io", "Report I/O counts and volumes per document", NONE, (s, v) -> s.diagnostics.measureIo = true),
			shared("--debug-truncate", "Read only the first N bytes of each input", SINGLE,
				(s, v) -> s.diagnostics.truncateInputBytes = (long)ValueParsers.parseCount("--debug-truncate", v)));
	}

	/** {@code --license-key} and {@code --library}. Every verb that touches the engine takes these. */
	public static List<ArgumentDefinition<SharedOptions>> libraryArguments()
	{
		return Arrays.asList(
			shared("--license-key", "Document Filters license key; falls back to DF_LICENSE_KEY", SINGLE,
				(s, v) -> s.library.licenseKey = v),
			shared("--library", "Path to the Document Filters engine; falls back to DF_PATH", SINGLE,
				(s, v) -> s.library.libraryPath = v));
	}

	/**
	 * Arguments that work on every verb and are not part of any shared group:
	 * {@code --dry-run} (with its {@code --print-plan} alias) and {@code --print-contract}.
	 */
	public static List<ArgumentDefinition<SharedOptions>> global()
	{
		return Arrays.asList(
			shared("--dry-run", "Plan the run and print it, without opening a document", NONE, (s, v) -> s.dryRun = true),
			shared("--print-plan", "Alias for --dry-run", NONE, (s, v) -> s.dryRun = true),
			shared("--print-contract", "Print the whole argument surface as flat text, and stop", NONE, (s, v) -> s.printContract = true));
	}

	private static ArgumentDefinition<ConvertOptions> convertArgument(String template, String description, ArgumentArity arity,
		BiConsumer<ConvertOptions, String> setter)
	{
		return new ArgumentDefinition<>(template, description, arity, setter);
	}

	/**
	 * {@code convert}'s own arguments, including {@code --pages}.
	 *
	 * <p>The CLI design lists {@code --pages} among the shared options, but
	 * {@code convert} is the only command that renders pages and
	 * {@code ConvertOptions.pages} is its only destination. A shared definition is
	 * typed to {@code SharedOptions} and so could not reach that field at all, so it
	 * belongs here.
	 */
	public static List<ArgumentDefinition<ConvertOptions>> convert()
	{
		return Arrays.asList(
			convertArgument("--to", "Target format: text, md, html5, pdf, png, json, xml, tiff ...", SINGLE,
				(o, v) -> o.format = FormatCatalog.resolve(ValueParsers.foldToken(v))),
			convertArgument("--combine", "Merge every input into one output document", NONE, (o, v) -> o.combine = true),
			convertArgument("--canvas-options", "Override canvas defaults, passed to the engine untouched", SINGLE,
				(o, v) -> o.canvasOptions = v),
			convertArgument("--render-options", "Override render-page defaults, passed to the engine untouched", SINGLE,
				(o, v) -> o.renderOptions = v),
			convertArgument("--no-images", "Skip embedded images during conversion", NONE, (o, v) -> o.noImages = true),
			convertArgument("--include-control-codes", "Keep ISYS control codes in text output", NONE,
				(o, v) -> o.includeControlCodes = true),
			convertArgument("--fingerprint", "Emit page fingerprints", NONE, (o, v) -> o.fingerprint = true),
			convertArgument("--watermark", "Stamp each page; a file path or a literal string", SINGLE, (o, v) -> o.watermark = v),
			convertArgument("--redact", "Black out a region: page,x,y,w,h", MULTIPLE,
				(o, v) -> o.redactions.add(ValueParsers.parseRedaction(v))),
			convertArgument("--form", "Set a form field value: page,name=value", MULTIPLE,
				(o, v) -> o.formValues.add(ValueParsers.parseFormValue(v))),
			convertArgument("--view", "Open the result when done", NONE, (o, v) -> o.view = true),
			convertArgument("--fonts", "Report fonts referenced in the document", NONE, (o, v) -> o.reports.fonts = true),
			convertArgument("--font-mappings", "Report the document's font mappings", NONE, (o, v) -> o.reports.fontMappings = true),
			convertArgument("--page-fonts", "Report fonts per page", NONE, (o, v) -> o.reports.pageFonts = true),
			convertArgument("--page-font-mappings", "Report font mappings per page", NONE, (o, v) -> o.reports.pageFontMappings = true),
			convertArgument("--dimensions", "Report each page's dimensions", NONE, (o, v) -> o.reports.dimensions = true),
			convertArgument("--page-elements", "Report each page's page elements", NONE, (o, v) -> o.reports.pageElements = true),
			convertArgument("--form-elements", "Report each page's form elements", NONE, (o, v) -> o.reports.formElements = true),
			convertArgument("--hyperlinks", "Report each page's hyperlink elements", NONE, (o, v) -> o.reports.hyperlinks = true),
			convertArgument("--bookmarks", "Report the document's bookmarks", NONE, (o, v) -> o.reports.bookmarks = true),
			convertArgument("--annotations", "Report the document's annotations", NONE, (o, v) -> o.reports.annotations = true),
			convertArgument("--pages", "Page ranges to render, e.g. 1,3-5,8", SINGLE, (o, v) -> o.pages = PageRange.parse(v)));
	}

	private static ArgumentDefinition<ExtractOptions> extractArgument(String template, String description, ArgumentArity arity,
		BiConsumer<ExtractOptions, String> setter)
	{
		return new ArgumentDefinition<>(template, description, arity, setter);
	}

	public static List<ArgumentDefinition<ExtractOptions>> extract()
	{
		return Arrays.asList(
			extractArgument("-i|--images", "Also pull embedded images, not just subfiles", NONE, (o, v) -> o.images = true),
			extractArgument("--include", "Keep subfiles whose name matches this glob, repeatable", MULTIPLE,
				(o, v) -> o.includePatterns.add(v)),
			extractArgument("--exclude", "Drop subfiles whose name matches this glob, repeatable", MULTIPLE,
				(o, v) -> o.excludePatterns.add(v)),
			extractArgument("--make-dirs", "Replicate the source tree under the output directory", NONE, (o, v) -> o.makeDirsWasGiven = true),
			extractArgument("--flat", "Put every extracted file straight into the output directory", NONE, (o, v) -> o.flatWasGiven = true),
			extractArgument("--sub-id", "Append the subfile id to extracted filenames", NONE, (o, v) -> o.appendSubfileId = true));
	}

	public static List<ArgumentDefinition<ListOptions>> list()
	{
		return Collections.singletonList(
			new ArgumentDefinition<ListOptions>("--json", "Machine-readable listing", NONE, (o, v) -> o.json = true));
	}

	public static List<ArgumentDefinition<IdentifyOptions>> identify()
	{
		return Arrays.asList(
			new ArgumentDefinition<IdentifyOptions>("--json", "Machine-readable output", NONE, (o, v) -> o.json = true),
			new ArgumentDefinition<IdentifyOptions>("--capabilities", "Also print the format's capability flags", NONE,
				(o, v) -> o.capabilities = true));
	}

	public static List<ArgumentDefinition<MetaOptions>> meta()
	{
		return Collections.singletonList(
			new ArgumentDefinition<MetaOptions>("--json", "Metadata as JSON instead of plain text", NONE, (o, v) -> o.json = true));
	}

	public static List<ArgumentDefinition<FormatsOptions>> formats()
	{
		return Arrays.asList(
			new ArgumentDefinition<FormatsOptions>("--json", "Machine-readable catalog", NONE, (o, v) -> o.json = true),
			new ArgumentDefinition<FormatsOptions>("--filter", "Only formats whose id or name matches", SINGLE, (o, v) -> o.filter = v));
	}

	public static List<ArgumentDefinition<OptionCatalogOptions>> optionCatalog()
	{
		return Arrays.asList(
			new ArgumentDefinition<OptionCatalogOptions>("--json", "Full catalog: name, description, default, type, appliesTo", NONE,
				(o, v) -> o.json = true),
			new ArgumentDefinition<OptionCatalogOptions>("--filter", "Only options whose name or description matches", SINGLE,
				(o, v) -> o.filter = v),
			new ArgumentDefinition<OptionCatalogOptions>("--applies-to", "Filter by scope: document, page, canvas, render ...", SINGLE,
				(o, v) -> o.appliesTo = ValueParsers.foldToken(v)));
	}

	/**
	 * Every argument on a given verb --- command-specific plus shared --- as
	 * descriptors, for the contract dump and the coverage tests.
	 *
	 * @throws IllegalArgumentException {@code command} is not in {@link #COMMANDS}.
	 */
	public static List<IArgumentDescriptor> describeCommand(String command)
	{
		List<IArgumentDescriptor> result = new ArrayList<>();
		switch (command)
		{
			case "convert":
				result.addAll(convert());
				break;
			case "extract":
				result.addAll(extract());
				break;
			case "list":
				result.addAll(list());
				break;
			case "identify":
				result.addAll(identify());
				break;
			case "meta":
				result.addAll(meta());
				break;
			case "formats":
				result.addAll(formats());
				break;
			case "options":
				result.addAll(optionCatalog());
				break;
			default:
				throw new IllegalArgumentException("unknown command '" + command + "'");
		}

		result.addAll(sharedFor(command));
		return result;
	}
}
