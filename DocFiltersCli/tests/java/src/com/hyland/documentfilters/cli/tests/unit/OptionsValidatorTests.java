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
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.ExtractOptions;
import com.hyland.documentfilters.cli.configuration.FormValue;
import com.hyland.documentfilters.cli.configuration.OnErrorBehaviour;
import com.hyland.documentfilters.cli.configuration.OptionsValidator;
import com.hyland.documentfilters.cli.configuration.RedactSpec;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.planning.FormatCatalog;
import com.hyland.documentfilters.cli.planning.OutputNameTemplate;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.UsageError;
import com.hyland.documentfilters.cli.tests.Test;

/**
 * The rules no single argument can enforce, because they are relations between
 * arguments. Everything here is decidable from the command line alone --- rules that
 * need to know how many files exist live in the planner instead.
 */
public final class OptionsValidatorTests
{
	private static ConvertOptions toFormat(String token)
	{
		ConvertOptions options = new ConvertOptions();
		options.format = FormatCatalog.resolve(token);
		return options;
	}

	private static ExtractOptions layout(boolean makeDirs, boolean flat)
	{
		ExtractOptions options = new ExtractOptions();
		options.makeDirsWasGiven = makeDirs;
		options.flatWasGiven = flat;
		return options;
	}

	@Test
	public void aBareDefaultConfigurationIsValid()
	{
		OptionsValidator.validate(new SharedOptions());
	}

	@Test
	public void outFileAndOutDirTogetherAreAUsageError()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outFile = "book.pdf";
		shared.output.outDir = "out";

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void outNameWithoutOutDirIsAUsageError()
	{
		// The template names a file inside a directory; without one it means nothing.
		SharedOptions shared = new SharedOptions();
		shared.output.nameTemplate = OutputNameTemplate.parse("{stem}.{ext}");

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));

		assertEquals(ErrorCodes.OUT_NAME_NEEDS_OUT_DIR, error.getCode());
	}

	@Test
	public void stdoutWithOutDirIsAUsageError()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.useStdout = true;
		shared.output.outDir = "out";

		assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));
	}

	@Test
	public void stdoutWithAFileTargetIsAUsageError()
	{
		// Output cannot go to a file and to stdout. Neither the out-file/out-dir rule
		// nor the stdout/out-dir rule covers this pair.
		SharedOptions shared = new SharedOptions();
		shared.output.useStdout = true;
		shared.output.outFile = "book.pdf";

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void outFileDashIsNotAConflictBecauseItIsHowStdoutIsSpelled()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.useStdout = true;

		OptionsValidator.validate(shared);
	}

	@Test
	public void theLogAliasesDisagreeingIsAUsageError()
	{
		// --log-level and --debug-log-level are one setting. Two arguments carrying
		// values into one field would silently drop whichever lost.
		SharedOptions shared = new SharedOptions();
		shared.diagnostics.logLevel = "trace";
		shared.diagnostics.debugLogLevel = "info";

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void theLogAliasesAgreeingIsMerelyRedundant()
	{
		SharedOptions shared = new SharedOptions();
		shared.diagnostics.logLevel = "trace";
		shared.diagnostics.debugLogLevel = "trace";

		OptionsValidator.validate(shared);

		assertEquals("trace", shared.diagnostics.effectiveLogLevel());
	}

	@Test
	public void eitherLogAliasAloneResolves()
	{
		SharedOptions viaShort = new SharedOptions();
		viaShort.diagnostics.logLevel = "trace";
		assertEquals("trace", viaShort.diagnostics.effectiveLogLevel());

		SharedOptions viaDebug = new SharedOptions();
		viaDebug.diagnostics.debugLogLevel = "trace";
		assertEquals("trace", viaDebug.diagnostics.effectiveLogLevel());

		assertNull(new SharedOptions().diagnostics.effectiveLogLevel());
	}

	@Test
	public void theModuleAliasesBehaveTheSameWay()
	{
		SharedOptions conflicting = new SharedOptions();
		conflicting.diagnostics.logModules = "pdf";
		conflicting.diagnostics.debugLogModules = "ocr";

		assertThrows(UsageError.class, () -> OptionsValidator.validate(conflicting));

		SharedOptions agreeing = new SharedOptions();
		agreeing.diagnostics.logModules = "pdf";
		agreeing.diagnostics.debugLogModules = "pdf";

		OptionsValidator.validate(agreeing);
		assertEquals("pdf", agreeing.diagnostics.effectiveLogModules());
	}

	@Test
	public void eitherPatternLanguageAloneIsValid()
	{
		// Compiling the matcher moved to CommandLineApp.bind; this only exercises the
		// exclusivity rule below, which is still validate's job.
		SharedOptions globs = new SharedOptions();
		globs.input.globPatterns.add("**/*.pdf");
		OptionsValidator.validate(globs);

		SharedOptions regexes = new SharedOptions();
		regexes.input.regexPatterns.add("a/b\\.pdf");
		OptionsValidator.validate(regexes);
	}

	@Test
	public void globAndRegexTogetherAreAUsageError()
	{
		// Two ways of saying the same thing. Combining them silently would make the
		// selection depend on argument order, and neither language subsumes the
		// other, so there is no sensible merge.
		SharedOptions shared = new SharedOptions();
		shared.input.globPatterns.add("**/*.pdf");
		shared.input.regexPatterns.add(".*\\.docx$");

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void verboseAndSilentTogetherAreAUsageError()
	{
		SharedOptions shared = new SharedOptions();
		shared.console.verbose = true;
		shared.console.silent = true;

		assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));
	}

	@Test
	public void noRecurseWithAnExplicitDeeperMaxDepthIsAUsageError()
	{
		// --no-recurse means depth 1. Combined with --max-depth 5 the user has asked
		// for two different things.
		SharedOptions shared = new SharedOptions();
		shared.input.maxDepth = 5;
		shared.input.noRecurseWasGiven = true;

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void noRecurseWithMaxDepthOneIsConsistent()
	{
		// Redundant rather than contradictory: both ask for the same thing.
		SharedOptions shared = new SharedOptions();
		shared.input.maxDepth = 1;
		shared.input.noRecurseWasGiven = true;

		OptionsValidator.validate(shared);
	}

	@Test
	public void noRecurseAloneIsFine()
	{
		SharedOptions shared = new SharedOptions();
		shared.input.noRecurseWasGiven = true;

		OptionsValidator.validate(shared);
		assertEquals(Integer.valueOf(1), shared.input.effectiveMaxDepth());
	}

	@Test
	public void maxDepthAloneIsFine()
	{
		SharedOptions shared = new SharedOptions();
		shared.input.maxDepth = 5;

		OptionsValidator.validate(shared);
		assertEquals(Integer.valueOf(5), shared.input.effectiveMaxDepth());
	}

	@Test
	public void neitherMeansUnlimitedDepth()
	{
		assertNull(new SharedOptions().input.effectiveMaxDepth());
	}

	@Test
	public void makeDirsWithFlatIsAUsageError()
	{
		// Same class of contradiction as the depth pair, and detectable for the same
		// reason: each argument records its own presence.
		UsageError error = assertThrows(UsageError.class,
			() -> OptionsValidator.validateExtract(new SharedOptions(), layout(true, true)));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void eitherDirectoryLayoutArgumentAloneIsFine()
	{
		OptionsValidator.validateExtract(new SharedOptions(), layout(true, false));
		OptionsValidator.validateExtract(new SharedOptions(), layout(false, true));

		assertTrue(layout(true, false).replicateTree());
		assertFalse(layout(false, true).replicateTree());
		assertTrue(new ExtractOptions().replicateTree());
	}

	@Test
	public void aPerPageFormatWithATemplateLackingPageIsAUsageError()
	{
		// Every page would resolve to the same path: without --overwrite the run fails
		// after page one, and with it only the last page survives.
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = "out";
		shared.output.nameTemplate = OutputNameTemplate.parse("{stem}.{ext}");

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, toFormat("png")));

		assertEquals(ErrorCodes.TEMPLATE_NEEDS_PAGE, error.getCode());
	}

	@Test
	public void aPerPageFormatWithAPageBearingTemplateIsValid()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = "out";
		shared.output.nameTemplate = OutputNameTemplate.parse("{stem}-{page:04}.{ext}");

		OptionsValidator.validateConvert(shared, toFormat("png"));
	}

	@Test
	public void aSingleFileFormatNeedsNoPageInItsTemplate()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = "out";
		shared.output.nameTemplate = OutputNameTemplate.parse("{stem}.{ext}");

		OptionsValidator.validateConvert(shared, toFormat("pdf"));
	}

	@Test
	public void aPerPageFormatWithNoTemplateIsValidBecauseTheDefaultHasAPage()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = "out";

		OptionsValidator.validateConvert(shared, toFormat("png"));
	}

	@Test
	public void failFastWithAnExplicitOnErrorSkipIsAUsageError()
	{
		// The two arguments write the same setting and contradict each other. Rejected
		// rather than resolved by precedence, because none of the four parsing
		// libraries reliably records which option came first.
		SharedOptions shared = new SharedOptions();
		shared.batch.onError = OnErrorBehaviour.SKIP;
		shared.batch.onErrorWasExplicit = true;
		shared.batch.failFastWasGiven = true;

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void failFastWithAnExplicitOnErrorStopIsConsistent()
	{
		SharedOptions shared = new SharedOptions();
		shared.batch.onError = OnErrorBehaviour.STOP;
		shared.batch.onErrorWasExplicit = true;
		shared.batch.failFastWasGiven = true;

		OptionsValidator.validate(shared);
	}

	@Test
	public void failFastAloneIsFine()
	{
		SharedOptions shared = new SharedOptions();
		shared.batch.onError = OnErrorBehaviour.STOP;
		shared.batch.failFastWasGiven = true;

		OptionsValidator.validate(shared);
	}

	// -------------------------------------------------------------------------- convert

	@Test
	public void missingToIsAUsageError()
	{
		// Nothing at the parser level enforces a required argument, so an omitted --to
		// leaves ConvertOptions.format null; every other convert rule reads it
		// unconditionally, so this has to be caught first.
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = "out";

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, new ConvertOptions()));

		assertEquals(ErrorCodes.MISSING_ARGUMENT, error.getCode());
		assertEquals(ExitCode.USAGE_ERROR, error.getExitCode());
	}

	@Test
	public void toAloneIsValid()
	{
		OptionsValidator.validateConvert(new SharedOptions(), toFormat("pdf"));
	}

	@Test
	public void combineWithAOnePerPageFormatIsAUsageError()
	{
		// You cannot merge documents into a format that emits a file per page.
		SharedOptions shared = new SharedOptions();
		shared.output.outFile = "out.png";
		ConvertOptions options = toFormat("png");
		options.combine = true;

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, options));

		assertEquals(ErrorCodes.COMBINE_UNSUPPORTED, error.getCode());
	}

	@Test
	public void combineWithASingleFileFormatIsValid()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outFile = "book.pdf";
		ConvertOptions options = toFormat("pdf");
		options.combine = true;

		OptionsValidator.validateConvert(shared, options);
	}

	@Test
	public void combineWithTextIsValid()
	{
		// Text is a single-file format, so several documents can be concatenated.
		SharedOptions shared = new SharedOptions();
		shared.output.outFile = "all.txt";
		ConvertOptions options = toFormat("text");
		options.combine = true;

		OptionsValidator.validateConvert(shared, options);
	}

	@Test
	public void stdoutWithAOnePerPageFormatIsAUsageError()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.useStdout = true;

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, toFormat("png")));

		assertEquals(ErrorCodes.STDOUT_NEEDS_SINGLE_FILE, error.getCode());
	}

	@Test
	public void stdoutWithASingleFileFormatIsValid()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.useStdout = true;

		OptionsValidator.validateConvert(shared, toFormat("pdf"));
	}

	// ---------------------------------------------------------------------- fingerprint

	private static ConvertOptions fingerprint()
	{
		ConvertOptions options = toFormat("pdf");
		options.fingerprint = true;
		return options;
	}

	@Test
	public void fingerprintWithStdoutIsAUsageError()
	{
		// --fingerprint renders a page only to read a fingerprint attribute off it and
		// never keeps the render, the same way doc2text's own --fingerprint discards
		// its render through a temporary stream --- so streaming that now-discarded
		// content is a contradiction, not a silent no-op.
		SharedOptions shared = new SharedOptions();
		shared.output.useStdout = true;
		ConvertOptions options = fingerprint();

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, options));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void fingerprintWithViewIsAUsageError()
	{
		SharedOptions shared = new SharedOptions();
		ConvertOptions options = fingerprint();
		options.view = true;

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, options));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void fingerprintWithCombineIsAUsageError()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outFile = "book.pdf";
		ConvertOptions options = fingerprint();
		options.combine = true;

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, options));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void fingerprintWithOutFileIsAUsageError()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outFile = "book.pdf";
		ConvertOptions options = fingerprint();

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, options));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void fingerprintWithOutDirIsAUsageError()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = "out";
		ConvertOptions options = fingerprint();

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validateConvert(shared, options));

		assertEquals(ErrorCodes.MUTUALLY_EXCLUSIVE, error.getCode());
	}

	@Test
	public void fingerprintWithNoOutputTargetAtAllIsValid()
	{
		// The common case: no --out-dir, --out-file or --stdout given, so the planner's
		// own default ('.') is never something the user named.
		OptionsValidator.validateConvert(new SharedOptions(), fingerprint());
	}

	@Test
	public void fingerprintWithPageDecorationsIsValid()
	{
		// --redact, --form and --watermark decorate the same throwaway render
		// fingerprinting already needs, so there is nothing for them to contradict.
		ConvertOptions options = fingerprint();
		options.watermark = "stamp.png";

		RedactSpec redaction = new RedactSpec();
		redaction.page = 1;
		redaction.width = 10;
		redaction.height = 10;
		options.redactions.add(redaction);

		FormValue value = new FormValue();
		value.page = 1;
		value.name = "field";
		value.value = "value";
		options.formValues.add(value);

		OptionsValidator.validateConvert(new SharedOptions(), options);
	}

	@Test
	public void everyValidationFailureExitsWithTheUsageCode()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outFile = "a";
		shared.output.outDir = "b";

		UsageError error = assertThrows(UsageError.class, () -> OptionsValidator.validate(shared));

		assertEquals(ExitCode.USAGE_ERROR, error.getExitCode());
	}
}
