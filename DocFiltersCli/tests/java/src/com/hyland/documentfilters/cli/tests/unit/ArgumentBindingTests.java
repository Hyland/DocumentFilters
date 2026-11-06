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

package com.hyland.documentfilters.cli.tests.unit;

import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertNotNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.cli.BoundCommandLine;
import com.hyland.documentfilters.cli.cli.CommandLineApp;
import com.hyland.documentfilters.cli.commands.ConvertCommand;
import com.hyland.documentfilters.cli.commands.ExtractCommand;
import com.hyland.documentfilters.cli.commands.FormatsCommand;
import com.hyland.documentfilters.cli.commands.IdentifyCommand;
import com.hyland.documentfilters.cli.commands.ListCommand;
import com.hyland.documentfilters.cli.commands.MetaCommand;
import com.hyland.documentfilters.cli.commands.OptionsCommand;
import com.hyland.documentfilters.cli.commands.VersionCommand;
import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.ExtractOptions;
import com.hyland.documentfilters.cli.configuration.FormatsOptions;
import com.hyland.documentfilters.cli.configuration.OnErrorBehaviour;
import com.hyland.documentfilters.cli.configuration.OptionCatalogOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.configuration.StructureReportOptions;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.UsageError;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * That each argument's setter writes to the field it is supposed to.
 *
 * <p>This is the one mapping in the project that is hand-written once per argument,
 * roughly seventy-five times, which makes it the likeliest place for a
 * copy-and-paste mistake. A setter pointed at the wrong field compiles, keeps its
 * correct name and arity, and would pass every declaration test in
 * {@link ArgumentTableTests} --- so it needs checking directly.
 *
 * <p>{@link ArgumentTableTests} asks "does this argument exist?". This asks "does it
 * do what it says?".
 */
public final class ArgumentBindingTests
{
	private static BoundCommandLine bind(String... args)
	{
		CommandLineApp app = new CommandLineApp(new RecordingConsole(), new InMemoryFileSystem());
		return app.bind(args);
	}

	private static SharedOptions shared(String... arguments)
	{
		// A minimal valid convert invocation, with the arguments under test spliced in.
		return bind(splice(new String[] { "convert", "--to", "text", "--out-dir", "out" }, arguments, "a.docx"))
			.getSharedOptions();
	}

	private static ConvertOptions convert(String... arguments)
	{
		String[] args = splice(new String[] { "convert", "--to", "text" }, arguments, "--out-dir", "out", "a.docx");
		return ((ConvertCommand) bind(args).getCommand()).options;
	}

	private static ExtractOptions extract(String... arguments)
	{
		String[] args = splice(new String[] { "extract" }, arguments, "--out-dir", "out", "a.zip");
		return ((ExtractCommand) bind(args).getCommand()).options;
	}

	private static String[] splice(String[] head, String[] middle, String... tail)
	{
		List<String> args = new ArrayList<>(Arrays.asList(head));
		args.addAll(Arrays.asList(middle));
		args.addAll(Arrays.asList(tail));
		return args.toArray(new String[0]);
	}

	// ====================================================== shared: output

	@Test
	public void outDirWritesToOutputOutDir()
	{
		assertEquals("out", shared().output.outDir);
	}

	@Test
	public void shortOIsAnAliasForOutDirAndNeverOutFile()
	{
		// The old tool's -o guessed file-or-directory from the format. It must not.
		SharedOptions shared = bind("convert", "--to", "text", "-o", "out", "a.docx").getSharedOptions();

		assertEquals("out", shared.output.outDir);
		assertNull(shared.output.outFile);
	}

	@Test
	public void outFileWritesToOutputOutFileAndNotOutDir()
	{
		SharedOptions shared = bind("convert", "--to", "text", "--out-file", "a.txt", "a.docx").getSharedOptions();

		assertEquals("a.txt", shared.output.outFile);
		assertNull(shared.output.outDir);
	}

	@Test
	public void outNameCompilesToATemplate()
	{
		SharedOptions shared = shared("--out-name", "{stem}-{page:04}.{ext}");

		assertNotNull(shared.output.nameTemplate);
		assertTrue(shared.output.nameTemplate.usesPage());
	}

	@Test
	public void stdoutSetsTheStdoutArgument()
	{
		SharedOptions shared = bind("convert", "--to", "text", "--stdout", "a.docx").getSharedOptions();

		assertTrue(shared.output.useStdout);
	}

	@Test
	public void outFileDashIsTheSameAsStdout()
	{
		// Two spellings, one setting. Nothing downstream should see the difference.
		SharedOptions shared = bind("convert", "--to", "text", "--out-file", "-", "a.docx").getSharedOptions();

		assertTrue(shared.output.useStdout);
	}

	@Test
	public void overwriteIsASwitch()
	{
		assertTrue(shared("--overwrite").output.overwrite);
		assertFalse(shared().output.overwrite);
	}

	// ====================================================== shared: walk

	@Test
	public void positionalPathsLandInInputPathsInOrder()
	{
		SharedOptions shared = bind("convert", "--to", "text", "--out-dir", "out", "a.docx", "b.docx").getSharedOptions();

		assertEquals(Arrays.asList("a.docx", "b.docx"), shared.input.paths);
	}

	// A token that starts with a dash but is not one of this tool's own option
	// spellings is a path. looksLikeOption is the whole rule, and these four shapes
	// are the ones a parsing library reads differently: a cluster of short options,
	// the end-of-options separator, a long option, and a long option whose name
	// begins with a dash. The reference once exited with a .NET stack trace on them.
	// Reported from the field against 1.0.0-prerelease.1 as `-tsv`.
	//
	// Deviation: the reference's [Theory] with five [InlineData] rows is one test
	// here that loops over the same five tokens, because the JDK-only runner has no
	// parameterised form. See "The test harness has no theories" in traps-java.md.
	@Test
	public void aDashTokenThatIsNotAnOptionSpellingIsAPath()
	{
		for (String token : new String[] { "-tsv", "-ab", "--", "---weird", "----" })
		{
			SharedOptions shared = bind("convert", "--to", "text", "--out-dir", "out", token).getSharedOptions();

			assertEquals(Collections.singletonList(token), shared.input.paths);
		}
	}

	@Test
	public void aDashShapedPathKeepsItsPositionAmongTheOthers()
	{
		// Order is load-bearing: --combine writes the inputs in the order given.
		// Collecting the odd tokens separately and appending them would pass the
		// test above and still get this wrong.
		SharedOptions shared = bind("convert", "--to", "text", "--out-dir", "out",
			"a.docx", "-tsv", "b.docx").getSharedOptions();

		assertEquals(Arrays.asList("a.docx", "-tsv", "b.docx"), shared.input.paths);
	}

	@Test
	public void anOptionValueMayLookLikeAPath()
	{
		// The partition walks by arity, not by shape: --out-name takes a value, so
		// the next token is that value however it is spelled.
		SharedOptions shared = bind("convert", "--to", "text", "--out-dir", "out",
			"--out-name", "-{stem}.txt", "a.docx").getSharedOptions();

		assertNotNull(shared.output.nameTemplate);
		assertEquals(Collections.singletonList("a.docx"), shared.input.paths);
	}

	@Test
	public void filesFromWritesItsPath()
	{
		assertEquals("list.txt", shared("--files-from", "list.txt").input.filesFrom);
	}

	@Test
	public void maxDepthParsesIntoItsOwnField()
	{
		SharedOptions shared = shared("--max-depth", "3");

		assertEquals(Integer.valueOf(3), shared.input.maxDepth);
		assertFalse(shared.input.noRecurseWasGiven);
		assertEquals(Integer.valueOf(3), shared.input.effectiveMaxDepth());
	}

	@Test
	public void maxDepthDefaultsToUnlimited()
	{
		// Directories are walked recursively by default.
		SharedOptions shared = shared();

		assertNull(shared.input.maxDepth);
		assertNull(shared.input.effectiveMaxDepth());
	}

	@Test
	public void noRecurseDoesNotWriteTheDepthField()
	{
		// The two arguments must stay distinguishable: if --no-recurse wrote 1 into
		// maxDepth, then "--max-depth 5 --no-recurse" would be indistinguishable from
		// a plain "--max-depth 1", and the contradiction could never be reported. The
		// resolved answer comes from effectiveMaxDepth instead.
		SharedOptions shared = shared("--no-recurse");

		assertTrue(shared.input.noRecurseWasGiven);
		assertNull(shared.input.maxDepth);
		assertEquals(Integer.valueOf(1), shared.input.effectiveMaxDepth());
	}

	@Test
	public void theContradictoryDepthPairStaysDistinguishableAfterBinding()
	{
		// The state the validator has to be able to see. Binding must not collapse it
		// before validation runs.
		SharedOptions conflicting = bind("convert", "--to", "text", "--out-dir", "out",
			"--max-depth", "5", "--no-recurse", "a.docx").getSharedOptions();

		assertEquals(Integer.valueOf(5), conflicting.input.maxDepth);
		assertTrue(conflicting.input.noRecurseWasGiven);
	}

	@Test
	public void followUsesCapitalL()
	{
		assertTrue(shared("-L").input.followSymlinks);
		assertTrue(shared("--follow").input.followSymlinks);
	}

	@Test
	public void sortAndIncludeHiddenAreSwitches()
	{
		assertTrue(shared("--sort").input.sort);
		assertTrue(shared("--include-hidden").input.includeHidden);
		assertFalse(shared().input.sort);
		assertFalse(shared().input.includeHidden);
	}

	@Test
	public void globAccumulatesAndBuildsAMatcher()
	{
		SharedOptions shared = shared("-g", "**/*.pdf");

		assertEquals(Arrays.asList("**/*.pdf"), shared.input.globPatterns);
		assertEquals(Collections.emptyList(), shared.input.regexPatterns);
		assertNotNull(shared.input.matcher);
		assertTrue(shared.input.matcher.matches("a/b.pdf"));
	}

	@Test
	public void globIsRepeatableAndAccumulates()
	{
		SharedOptions shared = shared("-g", "**/*.pdf", "-g", "!**/draft/*");

		assertEquals(Arrays.asList("**/*.pdf", "!**/draft/*"), shared.input.globPatterns);
		assertTrue(shared.input.matcher.matches("final/a.pdf"));
		assertFalse(shared.input.matcher.matches("draft/a.pdf"));
	}

	@Test
	public void regexAccumulatesIntoItsOwnList()
	{
		// The two pattern languages must stay distinguishable after binding, or the
		// validator cannot tell that both were given.
		SharedOptions shared = shared("--regex", ".*\\.pdf$");

		assertEquals(Arrays.asList(".*\\.pdf$"), shared.input.regexPatterns);
		assertEquals(Collections.emptyList(), shared.input.globPatterns);
		assertNotNull(shared.input.matcher);
		assertTrue(shared.input.matcher.matches("a/b.pdf"));
	}

	@Test
	public void regexIsRepeatableAndAccumulates()
	{
		SharedOptions shared = shared("--regex", ".*\\.pdf$", "--regex", ".*\\.docx$");

		assertEquals(2, shared.input.regexPatterns.size());
	}

	@Test
	public void neitherPatternArgumentLeavesTheMatcherUnset()
	{
		SharedOptions shared = shared();

		assertEquals(Collections.emptyList(), shared.input.globPatterns);
		assertEquals(Collections.emptyList(), shared.input.regexPatterns);
		assertNull(shared.input.matcher);
	}

	@Test
	public void aMalformedRegexIsAUsageErrorAtBindTime()
	{
		// Compiled in stage 2, the same as the subfile globs, so a typo is reported
		// before any directory walk or file work starts.
		assertThrows(UsageError.class, () -> shared("--regex", "[unclosed"));
	}

	@Test
	public void includeAndExcludeTypesSplitOnSemicolons()
	{
		SharedOptions shared = shared("--include-types", "PDF;DOCX", "--exclude-types", "ZIP");

		assertEquals(Arrays.asList("PDF", "DOCX"), shared.input.includeTypes);
		assertEquals(Arrays.asList("ZIP"), shared.input.excludeTypes);
	}

	// ================================================ shared: subfiles

	@Test
	public void subfilesShortAndLongFormsAgree()
	{
		assertTrue(shared("-s").subfiles.enabled);
		assertTrue(shared("--subfiles").subfiles.enabled);
		assertFalse(shared().subfiles.enabled);
	}

	@Test
	public void subfileDepthIsSeparateFromDirectoryDepth()
	{
		// The single most confusable pair of arguments in the tool.
		SharedOptions shared = shared("--max-depth", "2", "--subfile-depth", "5");

		assertEquals(Integer.valueOf(2), shared.input.maxDepth);
		assertEquals(Integer.valueOf(5), shared.subfiles.maxDepth);
	}

	// ================================================ shared: passwords

	@Test
	public void passwordIsRepeatableAndAccumulatesInOrder()
	{
		SharedOptions shared = shared("-p", "first", "-p", "second");

		assertEquals(Arrays.asList("first", "second"), shared.passwords.passwords);
	}

	@Test
	public void passwordFileAndPromptWriteTheirOwnFields()
	{
		SharedOptions shared = shared("--password-file", "pw.txt", "--password-prompt");

		assertEquals("pw.txt", shared.passwords.passwordFile);
		assertTrue(shared.passwords.prompt);
	}

	// ================================================ shared: processing

	@Test
	public void engineOptionsAreStoredVerbatimAndNotParsed()
	{
		// The CLI does not interpret this string; the engine does.
		SharedOptions shared = shared("--options", "BW_MODE=3;CODEPAGE=1252");

		assertEquals("BW_MODE=3;CODEPAGE=1252", shared.processing.engineOptions);
	}

	@Test
	public void codepageAndTempDirWriteTheirOwnFields()
	{
		SharedOptions shared = shared("--codepage", "1252", "--temp-dir", "/tmp/df");

		assertEquals("1252", shared.processing.codepage);
		assertEquals("/tmp/df", shared.processing.tempDir);
	}

	// ================================================ shared: batch

	@Test
	public void onErrorDefaultsToSkip()
	{
		SharedOptions shared = shared();

		assertEquals(OnErrorBehaviour.SKIP, shared.batch.onError);
		assertFalse(shared.batch.onErrorWasExplicit);
	}

	@Test
	public void onErrorStopIsRecordedAsExplicit()
	{
		SharedOptions shared = shared("--on-error", "stop");

		assertEquals(OnErrorBehaviour.STOP, shared.batch.onError);
		assertTrue(shared.batch.onErrorWasExplicit);
	}

	@Test
	public void failFastWritesStopThroughTheSameFieldAndFlagsItself()
	{
		// Recorded separately so a contradictory pair can be rejected rather than
		// silently resolved.
		SharedOptions shared = shared("--fail-fast");

		assertEquals(OnErrorBehaviour.STOP, shared.batch.onError);
		assertTrue(shared.batch.failFastWasGiven);
		assertFalse(shared.batch.onErrorWasExplicit);
	}

	// ================================================ shared: console, library

	@Test
	public void consoleSwitchesWriteTheirOwnFields()
	{
		assertTrue(shared("--color").console.color);
		assertTrue(shared("--verbose").console.verbose);
		assertTrue(shared("--silent").console.silent);
	}

	@Test
	public void licenseKeyAndLibraryHaveNoShortForms()
	{
		SharedOptions shared = shared("--license-key", "KEY", "--library", "/opt/df");

		assertEquals("KEY", shared.library.licenseKey);
		assertEquals("/opt/df", shared.library.libraryPath);
	}

	// ================================================ shared: diagnostics

	@Test
	public void timingSwitchesWriteTheirOwnFields()
	{
		assertTrue(shared("--time").diagnostics.time);
		assertTrue(shared("--logtime").diagnostics.logTime);
		assertTrue(shared("--logmem").diagnostics.logMemory);
		assertTrue(shared("--progress").diagnostics.progress);
		assertTrue(shared("--pause").diagnostics.pause);
	}

	@Test
	public void limitsParseAsNumbers()
	{
		SharedOptions shared = shared("--max-time", "60", "--timeout", "10", "--max-memory", "512");

		assertEquals(Integer.valueOf(60), shared.diagnostics.maxTime);
		assertEquals(Integer.valueOf(10), shared.diagnostics.timeout);
		assertEquals(Long.valueOf(512L), shared.diagnostics.maxMemory);
	}

	@Test
	public void memoryStatusLevelsEscalate()
	{
		// -x reports once at the end of the run, --xx after every document.
		assertEquals(0, shared().diagnostics.memoryStatusLevel);
		assertEquals(1, shared("-x").diagnostics.memoryStatusLevel);
		assertEquals(2, shared("--xx").diagnostics.memoryStatusLevel);
	}

	@Test
	public void theHigherMemoryStatusLevelWinsWhicheverOrderTheyAreGiven()
	{
		// A level, not two independent switches: --xx already includes what -x does,
		// so the pair is redundant rather than contradictory.
		assertEquals(2, shared("-x", "--xx").diagnostics.memoryStatusLevel);
		assertEquals(2, shared("--xx", "-x").diagnostics.memoryStatusLevel);
	}

	@Test
	public void memoryStatusAndLogmemAreSeparateSettings()
	{
		// --logmem samples this process's peak memory while working; -x/--xx ask the
		// engine what it has allocated.
		SharedOptions shared = shared("--xx", "--logmem");

		assertEquals(2, shared.diagnostics.memoryStatusLevel);
		assertTrue(shared.diagnostics.logMemory);
	}

	@Test
	public void theLogAliasesWriteTheirOwnFieldsAndResolveToOneValue()
	{
		// Each alias keeps its own field so the validator can see that both were
		// given; effectiveLogLevel is what everything downstream reads.
		SharedOptions viaShort = shared("--log-level", "trace");
		assertEquals("trace", viaShort.diagnostics.logLevel);
		assertNull(viaShort.diagnostics.debugLogLevel);
		assertEquals("trace", viaShort.diagnostics.effectiveLogLevel());

		SharedOptions viaDebug = shared("--debug-log-level", "trace");
		assertNull(viaDebug.diagnostics.logLevel);
		assertEquals("trace", viaDebug.diagnostics.debugLogLevel);
		assertEquals("trace", viaDebug.diagnostics.effectiveLogLevel());
	}

	@Test
	public void theModuleAliasesWriteTheirOwnFieldsToo()
	{
		assertEquals("pdf", shared("--log-modules", "pdf").diagnostics.effectiveLogModules());
		assertEquals("pdf", shared("--debug-log-modules", "pdf").diagnostics.effectiveLogModules());
		assertNull(shared("--log-modules", "pdf").diagnostics.debugLogModules);
	}

	@Test
	public void theRemainingDebugSwitchesWriteTheirOwnFields()
	{
		assertTrue(shared("--debug-use-memory-stream").diagnostics.useMemoryStream);
		assertTrue(shared("--debug-measure-io").diagnostics.measureIo);
		assertEquals(Long.valueOf(4096L), shared("--debug-truncate", "4096").diagnostics.truncateInputBytes);
	}

	@Test
	public void dryRunAndPrintPlanWriteTheSameField()
	{
		assertTrue(shared("--dry-run").dryRun);
		assertTrue(shared("--print-plan").dryRun);
		assertFalse(shared().dryRun);
	}

	@Test
	public void printContractWritesItsOwnField()
	{
		assertTrue(shared("--print-contract").printContract);
	}

	// ====================================================== convert

	@Test
	public void toResolvesThroughTheFormatCatalogue()
	{
		ConvertOptions options = convert();

		assertNotNull(options.format);
		assertEquals("text", options.format.getToken());
	}

	/**
	 * {@code --fallback} is gone, so it is an unknown argument like any other.
	 *
	 * <p>Kept as a test rather than deleted with the option: it was accepted for long
	 * enough to appear in scripts, and "unknown argument" is the answer those should
	 * get --- not silent acceptance, and not a crash.
	 */
	@Test
	public void fallbackIsNoLongerAnArgument()
	{
		UsageError error = assertThrows(UsageError.class, () -> convert("--fallback", "text"));

		assertEquals(ErrorCodes.UNKNOWN_ARGUMENT, error.getCode());
		assertEquals(ExitCode.USAGE_ERROR, error.getExitCode());
	}

	@Test
	public void combineAndTheSimpleSwitchesWriteTheirOwnFields()
	{
		assertTrue(convert("--combine").combine);
		assertTrue(convert("--no-images").noImages);
		assertTrue(convert("--include-control-codes").includeControlCodes);
		assertTrue(convert("--fingerprint").fingerprint);
		assertTrue(convert("--view").view);
	}

	@Test
	public void canvasAndRenderOptionsDoNotShareAField()
	{
		ConvertOptions options = convert("--canvas-options", "A=1", "--render-options", "B=2");

		assertEquals("A=1", options.canvasOptions);
		assertEquals("B=2", options.renderOptions);
	}

	@Test
	public void pagesCompilesToARange()
	{
		ConvertOptions options = convert("--pages", "1,3-5");

		List<Integer> pages = new ArrayList<>();
		for (int page : options.pages.enumerate(10))
			pages.add(page);

		assertEquals(Arrays.asList(1, 3, 4, 5), pages);
	}

	@Test
	public void watermarkWritesItsOwnField()
	{
		assertEquals("DRAFT", convert("--watermark", "DRAFT").watermark);
	}

	@Test
	public void redactIsRepeatableAndEachOccurrenceIsParsed()
	{
		ConvertOptions options = convert("--redact", "2,10,20,100,50", "--redact", "3,0,0,50,50");

		assertEquals(2, options.redactions.size());
		assertEquals(2, options.redactions.get(0).page);
		assertEquals(100, options.redactions.get(0).width);
		assertEquals(3, options.redactions.get(1).page);
	}

	@Test
	public void formIsRepeatableAndEachOccurrenceIsParsed()
	{
		ConvertOptions options = convert("--form", "1,a=x", "--form", "2,b=y");

		assertEquals(2, options.formValues.size());
		assertEquals("a", options.formValues.get(0).name);
		assertEquals("y", options.formValues.get(1).value);
	}

	@Test
	public void eachStructureReportArgumentSetsExactlyOneReport()
	{
		// Ten near-identical booleans: the highest copy-and-paste risk in the table.
		// Exactly one must flip, and any() must notice.
		String[] arguments = {
			"--fonts", "--font-mappings", "--page-fonts", "--page-font-mappings", "--dimensions",
			"--page-elements", "--form-elements", "--hyperlinks", "--bookmarks", "--annotations" };

		for (String argument : arguments)
		{
			StructureReportOptions reports = convert(argument).reports;

			assertTrue(reports.any(), argument);

			boolean[] flags = {
				reports.fonts, reports.fontMappings,
				reports.pageFonts, reports.pageFontMappings,
				reports.dimensions, reports.pageElements,
				reports.formElements, reports.hyperlinks,
				reports.bookmarks, reports.annotations };

			int enabled = 0;
			for (boolean flag : flags)
			{
				if (flag)
					enabled++;
			}

			assertEquals(Integer.valueOf(1), Integer.valueOf(enabled), argument);
		}
	}

	@Test
	public void noStructureReportsAreOnByDefault()
	{
		assertFalse(convert().reports.any());
	}

	// ====================================================== extract

	@Test
	public void imagesShortAndLongFormsAgree()
	{
		assertTrue(extract("-i").images);
		assertTrue(extract("--images").images);
		assertFalse(extract().images);
	}

	@Test
	public void subfileNameFiltersDoNotShareAField()
	{
		ExtractOptions options = extract("--include", "*invoice*", "--exclude", "*draft*");

		assertNotNull(options.subfileInclude);
		assertNotNull(options.subfileExclude);
		assertTrue(options.subfileInclude.matches("2026-invoice-01.pdf"));
		assertTrue(options.subfileExclude.matches("draft-notes.txt"));
	}

	@Test
	public void subfileFiltersAreGlobsCompiledDuringBinding()
	{
		// The same pattern language as -g/--glob, compiled in stage 2 so a malformed
		// pattern is a usage error before any archive is opened. A different axis
		// though: -g picks files on disk, this picks subfiles inside a container.
		ExtractOptions options = extract("--include", "**/*.docx");

		assertTrue(options.subfileInclude.matches("reports/q1.docx"));
		assertFalse(options.subfileInclude.matches("reports/q1.pdf"));
	}

	@Test
	public void subfileFiltersAreRepeatableLikeThePathGlobs()
	{
		// Repeatable for the same reason -g is: one glob per idea reads better than
		// one glob doing several jobs.
		ExtractOptions options = extract("--include", "*.docx", "--include", "*.xlsx");

		assertEquals(Arrays.asList("*.docx", "*.xlsx"), options.includePatterns);
		assertTrue(options.subfileInclude.matches("q1.docx"));
		assertTrue(options.subfileInclude.matches("q1.xlsx"));
		assertFalse(options.subfileInclude.matches("q1.pdf"));
	}

	@Test
	public void includeAndExcludeMayBothBeGiven()
	{
		// Unlike -g and --regex, which are two spellings of one idea and contradict,
		// these are complementary: keep these, then drop those.
		ExtractOptions options = extract("--include", "*.docx", "--exclude", "draft-*");

		assertTrue(options.subfileInclude.matches("q1.docx"));
		assertTrue(options.subfileExclude.matches("draft-q1.docx"));
	}

	@Test
	public void aMalformedSubfileGlobIsAUsageError()
	{
		assertThrows(UsageError.class, () -> extract("--include", "["));
	}

	@Test
	public void extractRegexSelectsPathsLikeItDoesEverywhereElse()
	{
		// --regex means one thing everywhere: select paths by regular expression.
		// extract accepts it like any other verb that walks a directory.
		SharedOptions shared = bind("extract", "--regex", ".*\\.zip$", "-o", "out", "archives").getSharedOptions();

		assertNotNull(shared.input.matcher);
		assertTrue(shared.input.matcher.matches("a/b.zip"));
	}

	@Test
	public void extractHasNoWayToMakeTheSubfileFiltersRegularExpressions()
	{
		// The subfile filters are globs, with no mode switch to make them regular
		// expressions. --regex on extract selects paths, as it does everywhere.
		assertThrows(UsageError.class, () -> bind("extract", "--include", "inv.*", "--content-regex", "-o", "out", "a.zip"));
	}

	@Test
	public void makeDirsIsTheDefaultAndFlatTurnsItOff()
	{
		assertTrue(extract().replicateTree());
		assertTrue(extract("--make-dirs").replicateTree());
		assertFalse(extract("--flat").replicateTree());
	}

	@Test
	public void theTwoLayoutArgumentsEachRecordTheirOwnPresence()
	{
		// So that --make-dirs --flat can be rejected rather than silently resolved,
		// for the same reason --max-depth and --no-recurse stay apart.
		assertTrue(extract("--make-dirs").makeDirsWasGiven);
		assertFalse(extract("--make-dirs").flatWasGiven);
		assertTrue(extract("--flat").flatWasGiven);
		assertFalse(extract("--flat").makeDirsWasGiven);
		assertFalse(extract().makeDirsWasGiven);
		assertFalse(extract().flatWasGiven);
	}

	@Test
	public void subIdWritesItsOwnField()
	{
		assertTrue(extract("--sub-id").appendSubfileId);
	}

	// ====================================================== other verbs

	@Test
	public void jsonBindsOnEachListingCommand()
	{
		assertTrue(((ListCommand) bind("list", "--json", "a.zip").getCommand()).options.json);
		assertTrue(((IdentifyCommand) bind("identify", "--json", "a.bin").getCommand()).options.json);
		assertTrue(((MetaCommand) bind("meta", "--json", "a.pdf").getCommand()).options.json);
		assertTrue(((FormatsCommand) bind("formats", "--json").getCommand()).options.json);
		assertTrue(((OptionsCommand) bind("options", "--json").getCommand()).options.json);
	}

	@Test
	public void identifyCapabilitiesBinds()
	{
		assertTrue(((IdentifyCommand) bind("identify", "--capabilities", "a.bin").getCommand()).options.capabilities);
	}

	@Test
	public void formatsFilterBinds()
	{
		FormatsOptions options = ((FormatsCommand) bind("formats", "--filter", "pdf").getCommand()).options;

		assertEquals("pdf", options.filter);
		assertFalse(options.json);
	}

	@Test
	public void optionsFilterAndAppliesToDoNotShareAField()
	{
		OptionCatalogOptions options =
			((OptionsCommand) bind("options", "--filter", "BW", "--applies-to", "document").getCommand()).options;

		assertEquals("BW", options.filter);
		assertEquals("document", options.appliesTo);
	}

	// ====================================================== arity behaviour

	@Test
	public void aSingleValueArgumentGivenTwiceKeepsTheLast()
	{
		SharedOptions shared = bind("convert", "--to", "text", "-o", "first", "-o", "second", "a.docx").getSharedOptions();

		assertEquals("second", shared.output.outDir);
	}

	@Test
	public void anAbsentArgumentLeavesItsDefaultUntouched()
	{
		SharedOptions shared = shared();

		assertNull(shared.processing.codepage);
		assertNull(shared.output.outFile);
		assertEquals(Collections.emptyList(), shared.passwords.passwords);
		assertEquals(OnErrorBehaviour.SKIP, shared.batch.onError);
	}

	@Test
	public void verbSelectionProducesTheRightCommandType()
	{
		assertEquals(ConvertCommand.class, bind("convert", "--to", "text", "-o", "out", "a.docx").getCommand().getClass());
		assertEquals(ExtractCommand.class, bind("extract", "-o", "out", "a.zip").getCommand().getClass());
		assertEquals(ListCommand.class, bind("list", "a.zip").getCommand().getClass());
		assertEquals(IdentifyCommand.class, bind("identify", "a.bin").getCommand().getClass());
		assertEquals(MetaCommand.class, bind("meta", "a.pdf").getCommand().getClass());
		assertEquals(FormatsCommand.class, bind("formats").getCommand().getClass());
		assertEquals(OptionsCommand.class, bind("options").getCommand().getClass());
		assertEquals(VersionCommand.class, bind("version").getCommand().getClass());
	}

	// ====================================================== rejection

	@Test
	public void anUnknownVerbIsAUsageError()
	{
		UsageError error = assertThrows(UsageError.class, () -> bind("nonsense", "a.docx"));

		assertEquals(ErrorCodes.UNKNOWN_COMMAND, error.getCode());
	}

	@Test
	public void anUnknownArgumentIsAUsageError()
	{
		UsageError error = assertThrows(UsageError.class, () -> bind("convert", "--to", "text", "--nonsense", "a.docx"));

		assertEquals(ErrorCodes.UNKNOWN_ARGUMENT, error.getCode());
	}

	@Test
	public void prefixAbbreviationDoesNotResolve()
	{
		// Some parsing libraries allow this by default; none of the four ports does,
		// so a command line one rejects is rejected by all of them.
		assertThrows(UsageError.class, () -> bind("convert", "--to", "text", "--out-d", "out", "a.docx"));
	}

	@Test
	public void anArgumentFromAnotherVerbIsRejected()
	{
		// --combine belongs to convert; extract must not quietly accept it.
		assertThrows(UsageError.class, () -> bind("extract", "--combine", "-o", "out", "a.zip"));
	}

	@Test
	public void aBadValueIsReportedByUsAndNotByTheParsingLibrary()
	{
		// The error code proves the message came from ValueParsers rather than from
		// generic parsing, which is what keeps stderr identical across the four ports.
		UsageError error = assertThrows(UsageError.class,
			() -> bind("convert", "--to", "text", "--max-depth", "abc", "-o", "out", "a.docx"));

		assertEquals(ErrorCodes.BAD_VALUE, error.getCode());
	}
}
