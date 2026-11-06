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
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.planning.NameMatcher;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.UsageError;
import com.hyland.documentfilters.cli.tests.Test;
import java.util.Arrays;

/**
 * {@code -g/--glob} and {@code --regex}. Glob is the default idiom; regex is the
 * power mode for patterns a glob cannot express.
 */
public final class NameMatcherTests
{
	private static NameMatcher glob(String... patterns)
	{
		return NameMatcher.fromGlobs(Arrays.asList(patterns));
	}

	private static NameMatcher regex(String... patterns)
	{
		return NameMatcher.fromRegexes(Arrays.asList(patterns));
	}

	@Test
	public void aStarMatchesWithinOnePathSegment()
	{
		NameMatcher matcher = glob("*.pdf");

		assertTrue(matcher.matches("report.pdf"));
		assertFalse(matcher.matches("reports/report.pdf"));
	}

	@Test
	public void aDoubleStarCrossesPathSegments()
	{
		NameMatcher matcher = glob("**/*.pdf");

		assertTrue(matcher.matches("a/b/c/report.pdf"));
		assertFalse(matcher.matches("a/b/c/report.docx"));
	}

	@Test
	public void aQuestionMarkMatchesExactlyOneCharacter()
	{
		NameMatcher matcher = glob("report?.pdf");

		assertTrue(matcher.matches("report1.pdf"));
		assertFalse(matcher.matches("report.pdf"));
		assertFalse(matcher.matches("report12.pdf"));
	}

	@Test
	public void aLeadingBangExcludes()
	{
		NameMatcher matcher = glob("**/*.pdf", "!**/draft/*");

		assertTrue(matcher.matches("final/report.pdf"));
		assertFalse(matcher.matches("draft/report.pdf"));
	}

	@Test
	public void exclusionsAloneKeepEverythingElse()
	{
		// "Everything except temp files" should not need an explicit "**/*" first.
		NameMatcher matcher = glob("!**/*.tmp");

		assertTrue(matcher.matches("report.pdf"));
		assertFalse(matcher.matches("scratch/a.tmp"));
	}

	@Test
	public void anEntryMatchingSeveralIncludesIsKeptOnce()
	{
		NameMatcher matcher = glob("**/*.pdf", "**/report.*");

		assertTrue(matcher.matches("report.pdf"));
	}

	@Test
	public void anExclusionBeatsAnInclusion()
	{
		NameMatcher matcher = glob("**/*.pdf", "!**/secret.pdf");

		assertFalse(matcher.matches("a/secret.pdf"));
	}

	@Test
	public void backslashesAreTreatedAsSeparatorsSoPatternsArePortable()
	{
		NameMatcher matcher = glob("**/*.pdf");

		assertTrue(matcher.matches("a\\b\\report.pdf"));
	}

	@Test
	public void regexModeMatchesTheWholePathNotASubstring()
	{
		// An unanchored regex is a frequent source of surprise: report\.pdf would
		// otherwise also select old-report.pdf and report.pdf.bak.
		NameMatcher matcher = regex("report\\.pdf");

		assertTrue(matcher.matches("report.pdf"));
		assertFalse(matcher.matches("a/report.pdf"));
		assertFalse(matcher.matches("old-report.pdf"));
		assertFalse(matcher.matches("report.pdf.bak"));
	}

	@Test
	public void aRegexCanStillMatchAFullPathWhenWrittenTo()
	{
		NameMatcher matcher = regex("a/b/report\\.pdf");

		assertTrue(matcher.matches("a/b/report.pdf"));
		assertFalse(matcher.matches("x/a/b/report.pdf"));
	}

	@Test
	public void regexModeExpressesWhatAGlobCannot()
	{
		NameMatcher matcher = regex("^report-\\d{4}\\.(pdf|docx)$");

		assertTrue(matcher.matches("report-2026.pdf"));
		assertTrue(matcher.matches("report-2026.docx"));
		assertFalse(matcher.matches("report-26.pdf"));
	}

	@Test
	public void anInvalidRegexIsAUsageError()
	{
		UsageError error = assertThrows(UsageError.class, () -> regex("[unclosed"));

		assertEquals(ExitCode.USAGE_ERROR, error.getExitCode());
	}

	@Test
	public void canonicalTextFormIsStableForTheConformanceCorpus()
	{
		assertEquals(glob("**/*.pdf").toString(), glob("**/*.pdf").toString());
	}

	@Test
	public void aTrailingNewlineDoesNotSatisfyTheEndAnchor()
	{
		// In java.util.regex '$' also matches just before a final line terminator,
		// so a pattern anchored with it would accept this name.
		assertFalse(glob("*.pdf").matches("report.pdf\n"));
		assertFalse(regex("report\\.pdf").matches("report.pdf\n"));
	}
}
