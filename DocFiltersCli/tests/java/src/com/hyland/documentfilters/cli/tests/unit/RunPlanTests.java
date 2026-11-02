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
import static com.hyland.documentfilters.cli.tests.Assert.assertNotEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;

import com.hyland.documentfilters.cli.planning.OutputNameTemplate;
import com.hyland.documentfilters.cli.planning.PageRange;
import com.hyland.documentfilters.cli.planning.PlannedItem;
import com.hyland.documentfilters.cli.planning.TargetPattern;
import com.hyland.documentfilters.cli.tests.Test;
import java.util.Arrays;

/**
 * {@link PlannedItem#outputDirectory} and the three {@link TargetPattern} methods.
 *
 * <p>The C# suite has no counterpart: these members are reached only through
 * OutputPlanner's own tests, which assert the plan they appear in rather than what
 * they answer. They are the two places execution asks the plan a question directly
 * --- "which directory do I create" and "which path do I write" --- so an unnoticed
 * difference here is a wrong file on disk rather than a wrong plan dump.
 */
public final class RunPlanTests
{
	private static TargetPattern targetPattern(String directory)
	{
		return targetPattern(directory, "{stem}-{page:04}.{ext}", "png");
	}

	private static TargetPattern targetPattern(String directory, String template, String extension)
	{
		return new TargetPattern(directory, OutputNameTemplate.parse(template), extension);
	}

	// ------------------------------------------------------- PlannedItem.outputDirectory

	@Test
	public void aPatternedItemReportsThePatternDirectory()
	{
		PlannedItem item = new PlannedItem();
		item.targetPattern = targetPattern("out/pages");

		assertEquals("out/pages", item.outputDirectory());
	}

	@Test
	public void thePatternWinsEvenWhenATargetIsAlsoSet()
	{
		// The C# tests TargetPattern first and returns without looking at Target, so
		// an item carrying both reports the pattern's directory. The planner never
		// builds such an item; this pins the precedence rather than the situation.
		PlannedItem item = new PlannedItem();
		item.targetPattern = targetPattern("out/pages");
		item.target = "elsewhere/a.txt";

		assertEquals("out/pages", item.outputDirectory());
	}

	@Test
	public void aSingleTargetReportsTheDirectoryHoldingIt()
	{
		PlannedItem item = new PlannedItem();
		item.target = "out/sub/a.txt";

		assertEquals("out/sub", item.outputDirectory());
	}

	@Test
	public void aBareFilenameHasAnEmptyDirectoryRatherThanNull()
	{
		// Null means "this item has no file target at all", which a bare filename is
		// not. Execution creates nothing for either, so only the distinction is pinned.
		PlannedItem item = new PlannedItem();
		item.target = "a.txt";

		assertEquals("", item.outputDirectory());
	}

	@Test
	public void anItemWithNoTargetAtAllHasNoDirectory()
	{
		// --stdout, or an item feeding a combined target.
		assertNull(new PlannedItem().outputDirectory());
	}

	@Test
	public void anEmptyTargetIsTreatedAsNoTarget()
	{
		PlannedItem item = new PlannedItem();
		item.target = "";

		assertNull(item.outputDirectory());
	}

	@Test
	public void aBackslashTargetSplitsTheSameWayOnEveryHost()
	{
		// A raw --out-file of this shape is the one case where splitting with the
		// host's own path rules would give a different answer on Windows and Linux.
		// The target is normalised first and the Windows answer chosen for all four.
		PlannedItem item = new PlannedItem();
		item.target = "C:\\tmp\\a.pdf";

		assertEquals("C:/tmp", item.outputDirectory());
	}

	// ------------------------------------------------------------ TargetPattern.resolve

	@Test
	public void resolveJoinsTheDirectoryAndTheRenderedName()
	{
		TargetPattern pattern = targetPattern("out/pages");

		assertEquals("out/pages/report-0007.png", pattern.resolve("in/report.pdf", 7));
	}

	@Test
	public void resolveOmitsTheSeparatorWhenThereIsNoDirectory()
	{
		// Not "/report-0007.png", which would be an absolute path on Linux and so a
		// different file entirely from the one the user asked for.
		for (String directory : Arrays.asList(null, ""))
			assertEquals("report-0007.png", targetPattern(directory).resolve("in/report.pdf", 7), "directory " + directory);
	}

	@Test
	public void resolveJoinsTheDirectoryVerbatimAndNormalisesOnlyTheName()
	{
		// The directory is joined with no normalisation, and --out-dir reaches the
		// plan exactly as typed --- only the replicated relative portion is
		// normalised, by the planner. So a backslash the user typed survives into the
		// path, identically in all four ports. The rendered filename is the half that
		// is guaranteed separator-free, because the template scanner rejects one.
		TargetPattern pattern = targetPattern("out\\pages");

		assertEquals("out\\pages/report-0001.png", pattern.resolve("in\\report.pdf", 1));
	}

	// -------------------------------------------------------- TargetPattern.collisionKey

	@Test
	public void theCollisionKeyIgnoresThePageNumber()
	{
		TargetPattern pattern = targetPattern("out");

		assertEquals(pattern.collisionKey("in/report.pdf"), pattern.resolve("in/report.pdf", 0));
		assertNotEquals(pattern.resolve("in/report.pdf", 1), pattern.resolve("in/report.pdf", 2));
	}

	@Test
	public void twoInputsWithTheSameStemShareACollisionKey()
	{
		// The case that actually bites: a flattened walk holding two files of the same
		// stem collide on every page, and this says so without knowing either page count.
		TargetPattern pattern = targetPattern("out");

		assertEquals(pattern.collisionKey("one/report.pdf"), pattern.collisionKey("two/report.pdf"));
	}

	@Test
	public void inputsWithDifferentStemsDoNotShareACollisionKey()
	{
		TargetPattern pattern = targetPattern("out");

		assertNotEquals(pattern.collisionKey("in/report.pdf"), pattern.collisionKey("in/summary.pdf"));
	}

	@Test
	public void aTemplateWithoutAPagePlaceholderStillKeysConsistently()
	{
		// A single-file format's template has no {page}, so every page renders the same
		// name and the key is simply that name.
		TargetPattern pattern = targetPattern("out", "{stem}.{ext}", "txt");

		assertEquals("out/report.txt", pattern.collisionKey("in/report.pdf"));
	}

	// --------------------------------------------------------- TargetPattern.firstTarget

	@Test
	public void theFirstTargetOfAnUnrestrictedRangeIsPageOne()
	{
		TargetPattern pattern = targetPattern("out");

		assertEquals("out/report-0001.png", pattern.firstTarget("in/report.pdf", PageRange.ALL));
	}

	@Test
	public void theFirstTargetOfAnExplicitRangeIsItsFirstPageNotPageOne()
	{
		// --pages 5-6 writes page 5 first. No document is open yet, so this is read
		// straight off the selection rather than checked against a page count.
		TargetPattern pattern = targetPattern("out");

		assertEquals("out/report-0005.png", pattern.firstTarget("in/report.pdf", PageRange.parse("5-6")));
	}

	@Test
	public void theFirstTargetOfAnUnorderedRangeIsTheLowestPage()
	{
		// The selection is sorted and collapsed when it is parsed, so the first entry is
		// the lowest page however the user wrote the range.
		TargetPattern pattern = targetPattern("out");

		assertEquals("out/report-0003.png", pattern.firstTarget("in/report.pdf", PageRange.parse("9,3,7")));
	}
}
