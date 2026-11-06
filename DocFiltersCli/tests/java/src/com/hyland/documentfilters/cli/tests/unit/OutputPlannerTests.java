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

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.discovery.InputSource;
import com.hyland.documentfilters.cli.planning.FormatCatalog;
import com.hyland.documentfilters.cli.planning.OutputCardinality;
import com.hyland.documentfilters.cli.planning.OutputNameTemplate;
import com.hyland.documentfilters.cli.planning.OutputPlanner;
import com.hyland.documentfilters.cli.planning.PageRange;
import com.hyland.documentfilters.cli.planning.PlannedItem;
import com.hyland.documentfilters.cli.planning.RunPlan;
import com.hyland.documentfilters.cli.planning.TargetPattern;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.NoInputsError;
import com.hyland.documentfilters.cli.runtime.OutputConflictError;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * The output model. This is the highest-risk logic in the tool and the reason
 * planning is a separate, pure stage.
 *
 * <p>The rule underneath every test here: the meaning of {@code -o} never depends
 * on {@code --to}.
 */
public final class OutputPlannerTests
{
	/**
	 * {@link OutputPlanner#plan} against a filesystem that tells names apart by case,
	 * which is what these cases assume unless one says otherwise.
	 */
	private static RunPlan plan(SharedOptions shared, ConvertOptions convert, List<InputSource> inputs)
	{
		return plan(shared, convert, inputs, true);
	}

	private static RunPlan plan(SharedOptions shared, ConvertOptions convert, List<InputSource> inputs, boolean caseSensitive)
	{
		InMemoryFileSystem fileSystem = new InMemoryFileSystem();
		fileSystem.caseSensitive = caseSensitive;
		return OutputPlanner.plan(shared, convert, inputs, fileSystem);
	}

	private static List<InputSource> inputs(String... paths)
	{
		List<InputSource> sources = new ArrayList<>();

		for (String path : paths)
		{
			// The relative path is the filename alone, so these inputs model a
			// flattened walk: the paths are forward-slash by construction, so the
			// split is on a literal '/' rather than anything the host supplies.
			int slash = path.lastIndexOf('/');
			sources.add(new InputSource(path, slash < 0 ? path : path.substring(slash + 1), null));
		}

		return sources;
	}

	private static ConvertOptions toFormat(String token)
	{
		return toFormat(token, false);
	}

	private static ConvertOptions toFormat(String token, boolean combine)
	{
		ConvertOptions options = new ConvertOptions();
		options.combine = combine;
		options.format = FormatCatalog.resolve(token);
		return options;
	}

	private static SharedOptions outDir(String directory)
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = directory;
		return shared;
	}

	private static SharedOptions outFile(String file)
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outFile = file;
		return shared;
	}

	// ---------------------------------------------------------------- cardinality

	@Test
	public void outDirWithManyInputsGivesEachItsOwnOutput()
	{
		RunPlan plan = plan(outDir("out"), toFormat("pdf"), inputs("a.docx", "b.docx"));

		assertEquals(OutputCardinality.PER_INPUT, plan.cardinality);
		assertEquals(2, plan.items.size());
		assertEquals("out/a.pdf", plan.items.get(0).target);
		assertEquals("out/b.pdf", plan.items.get(1).target);
	}

	@Test
	public void outFileWithOneInputIsASingleTarget()
	{
		RunPlan plan = plan(outFile("book.pdf"), toFormat("pdf"), inputs("a.docx"));

		assertEquals(OutputCardinality.SINGLE_TARGET, plan.cardinality);
		assertEquals("book.pdf", plan.items.get(0).target);
	}

	@Test
	public void combineWithOutFileMergesEveryInputIntoOneDocument()
	{
		RunPlan plan = plan(outFile("book.pdf"), toFormat("pdf", true), inputs("a.docx", "b.docx", "c.docx"));

		assertEquals(OutputCardinality.COMBINED, plan.cardinality);
		assertEquals("book.pdf", plan.combinedTarget);
		assertEquals(3, plan.items.size());

		// Items contribute to the shared target rather than owning one.
		for (PlannedItem item : plan.items)
			assertNull(item.target);
	}

	@Test
	public void stdoutIsItsOwnCardinality()
	{
		SharedOptions shared = new SharedOptions();
		shared.output.useStdout = true;

		RunPlan plan = plan(shared, toFormat("pdf"), inputs("a.docx"));

		assertEquals(OutputCardinality.STREAM, plan.cardinality);
	}

	// ------------------------------------------------- collisions and letter case

	/**
	 * Two names differing only in case are one file where the filesystem says so, and
	 * the planner must refuse rather than let the second output replace the first.
	 *
	 * <p>This was silent data loss: the run exited 0, printed nothing, and left one
	 * file holding the second document's text. Nothing caught it because the suite
	 * compared planned paths ordinally, which is right on the filesystem the tests ran
	 * against and wrong on the one most users have.
	 *
	 * <p>The corpus cannot hold this case --- it states the other answer, since its
	 * filesystem is case-sensitive and a shared corpus has to read the same
	 * everywhere. This is that half, and it is a unit test in each language rather
	 * than a contract case for the same reason.
	 */
	@Test
	public void namesDifferingOnlyInCaseCollideWhereTheFilesystemFoldsCase()
	{
		OutputConflictError error = assertThrows(OutputConflictError.class,
			() -> plan(outDir("out"), toFormat("text"), inputs("a/Report.docx", "b/report.docx"), false));

		assertEquals(ErrorCodes.OUTPUT_COLLISION, error.getCode());
		assertEquals(ExitCode.OUTPUT_CONFLICT, error.getExitCode());
	}

	/**
	 * The same pair is two outputs where the filesystem tells them apart, and refusing
	 * it would decline work the tool can do.
	 */
	@Test
	public void namesDifferingOnlyInCaseAreTwoOutputsWhereTheFilesystemIsCaseSensitive()
	{
		RunPlan plan = plan(outDir("out"), toFormat("text"), inputs("a/Report.docx", "b/report.docx"), true);

		List<String> targets = new ArrayList<>();

		for (PlannedItem item : plan.items)
			targets.add(item.target);

		assertEquals(Arrays.asList("out/Report.txt", "out/report.txt"), targets);
	}

	/** An exact repeat collides whatever the filesystem says about case. */
	@Test
	public void twoInputsWithTheSameNameAlwaysCollide()
	{
		for (boolean caseSensitive : new boolean[] { true, false })
		{
			OutputConflictError error = assertThrows(OutputConflictError.class,
				() -> plan(outDir("out"), toFormat("text"), inputs("a/report.docx", "b/report.docx"), caseSensitive));

			assertEquals(ErrorCodes.OUTPUT_COLLISION, error.getCode(), "caseSensitive " + caseSensitive);
		}
	}

	// ------------------------------------------------------------- the hard error

	@Test
	public void outFileWithSeveralInputsAndNoCombineIsAHardError()
	{
		// The specification is explicit that this is an error and never a guess:
		// the tool must not silently pick one input or overwrite repeatedly.
		OutputConflictError error = assertThrows(OutputConflictError.class,
			() -> plan(outFile("one.pdf"), toFormat("pdf"), inputs("a.docx", "b.docx")));

		assertEquals(ErrorCodes.OUT_FILE_MULTI_INPUT, error.getCode());
		assertEquals(ExitCode.OUTPUT_CONFLICT, error.getExitCode());
	}

	@Test
	public void noInputsIsItsOwnErrorAndExitCode()
	{
		NoInputsError error = assertThrows(NoInputsError.class, () -> plan(outDir("out"), toFormat("pdf"), inputs()));

		assertEquals(ExitCode.NO_INPUTS_MATCHED, error.getExitCode());
	}

	// ------------------------------------------------- page fan-out is a separate axis

	@Test
	public void aOnePerPageFormatIsStillPerInput()
	{
		// One document producing five PNGs is not a different cardinality. Target
		// ownership and page count are independent questions.
		RunPlan plan = plan(outDir("out"), toFormat("png"), inputs("a.docx", "b.docx"));

		assertEquals(OutputCardinality.PER_INPUT, plan.cardinality);
	}

	@Test
	public void aOnePerPageFormatWithoutPagesDefersItsTargets()
	{
		// The filenames depend on the document's page count, which is unknown until
		// it is opened, so the planner records a pattern instead of paths.
		RunPlan plan = plan(outDir("out"), toFormat("png"), inputs("a.docx"));

		assertTrue(plan.items.get(0).isPageCountDependent());
		assertNull(plan.items.get(0).target);
		assertNotNull(plan.items.get(0).targetPattern);
	}

	@Test
	public void aOnePerPageFormatDefersItsTargetsEvenWithExplicitPages()
	{
		// --pages does not make the filenames knowable: the range is clamped to the
		// document's real page count, so --pages 1-2 on a one-page document writes
		// one file. Resolving both up front would let the planner reject a run
		// because a page-2 output already exists when page 2 is never written.
		ConvertOptions options = toFormat("png");
		options.pages = PageRange.parse("1-2");

		RunPlan plan = plan(outDir("out"), options, inputs("a.docx"));

		assertTrue(plan.items.get(0).isPageCountDependent());
		assertNull(plan.items.get(0).target);
		assertNotNull(plan.items.get(0).targetPattern);
	}

	@Test
	public void theFirstCheckedTargetIsTheFirstSelectedPageNotPageOne()
	{
		// With --pages 5-6 the first output is page 5, so that is what the
		// fail-fast existence check has to look at.
		TargetPattern pattern = new TargetPattern("out", OutputNameTemplate.parse("{stem}-{page:04}.{ext}"), "png");

		assertEquals("out/a-0005.png", pattern.firstTarget("a.docx", PageRange.parse("5-6")));
		assertEquals("out/a-0001.png", pattern.firstTarget("a.docx", PageRange.ALL));
	}

	@Test
	public void aSingleFileFormatResolvesItsTargetEvenWithoutPages()
	{
		RunPlan plan = plan(outDir("out"), toFormat("pdf"), inputs("a.docx"));

		assertFalse(plan.items.get(0).isPageCountDependent());
	}

	// ------------------------------------------------------------------ collisions

	@Test
	public void twoInputsResolvingToTheSameOutputIsAnError()
	{
		// A flattened walk of a/report.docx and b/report.docx would silently
		// overwrite. Only visible because the whole batch is planned at once.
		OutputConflictError error = assertThrows(OutputConflictError.class,
			() -> plan(outDir("out"), toFormat("pdf"), inputs("a/report.docx", "b/report.docx")));

		assertEquals(ErrorCodes.OUTPUT_COLLISION, error.getCode());
	}

	@Test
	public void collisionsAreDetectedForDeferredTargetsToo()
	{
		// Page count is unknown for PNG, but the stems already collide, so this is
		// decidable from the pattern alone.
		assertThrows(OutputConflictError.class, () -> plan(outDir("out"), toFormat("png"), inputs("a/report.docx", "b/report.docx")));
	}

	@Test
	public void sameStemWithDifferentSourceExtensionsStillCollides()
	{
		// report.docx and report.pdf both become report.txt.
		assertThrows(OutputConflictError.class, () -> plan(outDir("out"), toFormat("text"), inputs("report.docx", "report.pdf")));
	}

	@Test
	public void anOutNameTemplateCanResolveACollision()
	{
		// {srcext} distinguishes the two, so this must not error.
		SharedOptions shared = outDir("out");
		shared.output.nameTemplate = OutputNameTemplate.parse("{stem}-{srcext}.{ext}");

		RunPlan plan = plan(shared, toFormat("text"), inputs("report.docx", "report.pdf"));

		assertEquals("out/report-docx.txt", plan.items.get(0).target);
		assertEquals("out/report-pdf.txt", plan.items.get(1).target);
	}

	// -------------------------------------------------------------- existing files

	@Test
	public void anExistingTargetWithoutOverwriteIsAnError()
	{
		SharedOptions shared = outDir("out");
		InMemoryFileSystem fileSystem = new InMemoryFileSystem().addFile("out/a.pdf", "old");

		RunPlan plan = plan(shared, toFormat("pdf"), inputs("a.docx"));

		OutputConflictError error = assertThrows(OutputConflictError.class, () -> OutputPlanner.verifyTargets(plan, shared, fileSystem));

		assertEquals(ErrorCodes.OUTPUT_EXISTS, error.getCode());
	}

	@Test
	public void overwritePermitsAnExistingTarget()
	{
		SharedOptions shared = outDir("out");
		shared.output.overwrite = true;
		InMemoryFileSystem fileSystem = new InMemoryFileSystem().addFile("out/a.pdf", "old");

		RunPlan plan = plan(shared, toFormat("pdf"), inputs("a.docx"));

		// The assertion is that this returns: the only observable outcome is the
		// absence of an OutputConflictError.
		OutputPlanner.verifyTargets(plan, shared, fileSystem);
	}

	@Test
	public void verificationChecksTheFirstPageOfADeferredItem()
	{
		// Fail fast on the common case: re-running a conversion into a directory
		// that already holds its output.
		SharedOptions shared = outDir("out");
		InMemoryFileSystem fileSystem = new InMemoryFileSystem().addFile("out/a-0001.png", "old");

		RunPlan plan = plan(shared, toFormat("png"), inputs("a.docx"));

		assertThrows(OutputConflictError.class, () -> OutputPlanner.verifyTargets(plan, shared, fileSystem));
	}

	@Test
	public void verificationDoesNotCheckLaterPagesOfADeferredItem()
	{
		// Page 2 may never be written, so its presence must not fail the run here.
		// Execution checks each page as it produces it.
		SharedOptions shared = outDir("out");
		InMemoryFileSystem fileSystem = new InMemoryFileSystem().addFile("out/a-0002.png", "old");

		RunPlan plan = plan(shared, toFormat("png"), inputs("a.docx"));

		OutputPlanner.verifyTargets(plan, shared, fileSystem);
	}

	// --------------------------------------------------------------- tree layout

	@Test
	public void theSourceTreeIsReplicatedUnderOutDir()
	{
		List<InputSource> sources = Arrays.asList(new InputSource("corpus/reports/q1.docx", "reports/q1.docx", "corpus"));

		RunPlan plan = plan(outDir("out"), toFormat("pdf"), sources);

		assertEquals("out/reports/q1.pdf", plan.items.get(0).target);
	}
}
