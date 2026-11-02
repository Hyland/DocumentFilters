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

import static com.hyland.documentfilters.cli.tests.Assert.assertDoesNotContain;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.planning.OutputNameTemplate;
import com.hyland.documentfilters.cli.runtime.UsageError;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.conformance.Fixture;
import com.hyland.documentfilters.cli.tests.conformance.FixtureCase;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * {@code --out-name}, checked against the shared out-names.txt fixture.
 *
 * <p>The render cases are tabular --- template, source, extension, page, expected
 * filename --- so they live in the fixture and all four implementations run them.
 * What stays here is behaviour a row cannot express: the defaults, and the
 * guarantee that a rendered name is a filename and never a path.
 */
public final class OutputNameTemplateTests
{
	/** One {@code = render} row: {@code source ext page => expected}. */
	private static final class RenderRow
	{
		final String source;
		final String extension;
		final int page;
		final String expected;

		private RenderRow(String source, String extension, int page, String expected)
		{
			this.source = source;
			this.extension = extension;
			this.page = page;
			this.expected = expected;
		}

		static RenderRow parse(String row, FixtureCase testCase)
		{
			String[] halves = row.split("=>", -1);

			if (halves.length != 2)
				throw new IllegalStateException(testCase.describe("render row has no '=>': " + row));

			String[] inputs = halves[0].trim().split("[ \t]+");

			if (inputs.length != 3)
				throw new IllegalStateException(testCase.describe("render row needs 'source ext page => expected': " + row));

			return new RenderRow(inputs[0], inputs[1], Integer.parseInt(inputs[2]), halves[1].trim());
		}
	}

	@Test
	public void templateMatchesTheSharedFixture()
	{
		for (FixtureCase testCase : Fixture.load("out-names.txt"))
		{
			if (testCase.getError() != null)
			{
				UsageError error = assertThrows(UsageError.class, () -> OutputNameTemplate.parse(testCase.subject));

				assertTrue(
					error.getCode().equals(testCase.getError()),
					testCase.describe("expected error " + testCase.getError() + ", got " + error.getCode()));
				continue;
			}

			OutputNameTemplate template = OutputNameTemplate.parse(testCase.subject);

			if (testCase.has("uses-page"))
			{
				boolean expected = testCase.getBool("uses-page");

				assertTrue(
					template.usesPage() == expected,
					testCase.describe("uses-page was " + yes(template.usesPage()) + ", expected " + yes(expected)));
			}

			for (String row : testCase.getAll("render"))
			{
				RenderRow expectation = RenderRow.parse(row, testCase);
				String actual = template.render(expectation.source, expectation.extension, expectation.page);

				assertTrue(
					actual.equals(expectation.expected),
					testCase.describe("render " + expectation.source + " " + expectation.extension + " " + expectation.page
						+ " produced '" + actual + "', expected '" + expectation.expected + "'"));
			}
		}
	}

	@Test
	public void defaultForASingleFileFormatHasNoPagePlaceholder()
	{
		OutputNameTemplate template = OutputNameTemplate.defaultFor(true);

		assertFalse(template.usesPage());
		assertEquals("report.pdf", template.render("report.docx", "pdf", 1));
	}

	@Test
	public void defaultForAOnePerPageFormatIncludesAPaddedPageNumber()
	{
		OutputNameTemplate template = OutputNameTemplate.defaultFor(false);

		assertTrue(template.usesPage());
		assertEquals("report-0001.png", template.render("report.docx", "png", 1));
	}

	@Test
	public void renderingNeverEmitsAPathSeparator()
	{
		// The stem comes from the source filename, which is attacker-influenced in a
		// corpus someone else assembled. A rendered name must never escape --out-dir.
		for (String source : Arrays.asList("../../etc/passwd", "..\\..\\windows\\system32\\config", "/absolute/path/report.docx"))
		{
			OutputNameTemplate template = OutputNameTemplate.parse("{stem}.{ext}");
			String rendered = template.render(source, "txt", 1);

			assertDoesNotContain("/", rendered);
			assertDoesNotContain("\\", rendered);
			assertDoesNotContain("..", rendered);
		}
	}

	@Test
	public void twoPagesOfTheSameDocumentNeverRenderToTheSameName()
	{
		OutputNameTemplate template = OutputNameTemplate.defaultFor(false);
		Set<String> seen = new HashSet<>();

		for (int page = 1; page <= 200; page++)
			assertTrue(seen.add(template.render("report.docx", "png", page)), "duplicate name at page " + page);
	}

	@Test
	public void theOriginalTextIsRecoverableForThePlanDump()
	{
		String text = "{stem}-{page:04}.{ext}";

		assertEquals(text, OutputNameTemplate.parse(text).toString());
	}

	@Test
	public void onlyPlainAsciiDigitsAreAPaddingWidth()
	{
		// Integer.parseInt takes a leading '+' and any Unicode decimal digit, so a
		// width read with it alone would accept several of these.
		for (String width : Arrays.asList(" 4", "4 ", "+4", "0_4", "4.0", "\u0664", "4\n"))
			assertThrows(UsageError.class, () -> OutputNameTemplate.parse("{stem}-{page:" + width + "}.{ext}"));

		assertEquals("{stem}-{page:4}.{ext}", OutputNameTemplate.parse("{stem}-{page:4}.{ext}").toString());
	}

	private static String yes(boolean value)
	{
		return value ? "yes" : "no";
	}
}
