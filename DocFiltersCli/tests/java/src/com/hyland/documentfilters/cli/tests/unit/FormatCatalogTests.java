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

import static com.hyland.documentfilters.cli.tests.Assert.assertContains;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertNotNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.planning.FormatCatalog;
import com.hyland.documentfilters.cli.planning.FormatKind;
import com.hyland.documentfilters.cli.planning.FormatSpec;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.UsageError;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.conformance.Fixture;
import com.hyland.documentfilters.cli.tests.conformance.FixtureCase;
import java.util.HashSet;
import java.util.Set;

/** {@code --to}, checked against the shared formats.txt fixture. */
public final class FormatCatalogTests
{
	@Test
	public void tokenMatchesTheSharedCatalogue()
	{
		for (FixtureCase testCase : Fixture.load("formats.txt"))
		{
			String token = testCase.subject;

			if (testCase.getError() != null)
			{
				UsageError error = assertThrows(UsageError.class, () -> FormatCatalog.resolve(token));

				assertTrue(
					error.getCode().equals(testCase.getError()),
					testCase.describe("expected error " + testCase.getError() + ", got " + error.getCode()));
				continue;
			}

			FormatSpec spec = FormatCatalog.resolve(token);

			assertTrue(spec != null, testCase.describe("token did not resolve"));

			// The canonical token is the folded one: the fixture states PDF and Tiff as
			// their own cases, and both must land on the lower-case entry.
			assertTrue(
				spec.getToken().equals(fold(token)),
				testCase.describe("canonical token was '" + spec.getToken() + "', expected '" + fold(token) + "'"));

			FormatKind expectedKind = parseKind(testCase);
			assertTrue(spec.getKind() == expectedKind, testCase.describe("kind was " + spec.getKind() + ", expected " + expectedKind));

			boolean singleFile = testCase.getBool("single-file");
			assertTrue(
				spec.isSingleFile() == singleFile,
				testCase.describe("single-file was " + yes(spec.isSingleFile()) + ", expected " + yes(singleFile)));

			boolean combine = testCase.getBool("combine");
			assertTrue(
				spec.supportsCombine() == combine,
				testCase.describe("combine was " + yes(spec.supportsCombine()) + ", expected " + yes(combine)));

			boolean hidef = testCase.getBool("hidef");
			assertTrue(
				spec.isHiDefCapable() == hidef,
				testCase.describe("hidef was " + yes(spec.isHiDefCapable()) + ", expected " + yes(hidef)));

			String ext = testCase.get("ext");
			assertTrue(spec.getExtension().equals(ext), testCase.describe("ext was '" + spec.getExtension() + "', expected '" + ext + "'"));

			// 'none' rather than an empty value, so the row is visible in the fixture.
			String options = "none".equals(testCase.get("options")) ? "" : testCase.get("options");
			String actualOptions = spec.getCanvasOptions() == null ? "" : spec.getCanvasOptions();
			assertTrue(
				actualOptions.equals(options),
				testCase.describe("options were '" + spec.getCanvasOptions() + "', expected '" + options + "'"));
		}
	}

	/** The ASCII-only fold the catalogue matches tokens with. */
	private static String fold(String token)
	{
		StringBuilder folded = new StringBuilder(token.length());

		for (char c : token.toCharArray())
			folded.append(c >= 'A' && c <= 'Z' ? (char) (c + 32) : c);

		return folded.toString();
	}

	private static FormatKind parseKind(FixtureCase testCase)
	{
		switch (testCase.get("kind"))
		{
			case "text":
				return FormatKind.TEXT;
			case "canvas":
				return FormatKind.CANVAS;
			case "docpack":
				return FormatKind.DOCPACK;
			default:
				throw new IllegalStateException(testCase.describe("unknown 'kind'"));
		}
	}

	private static String yes(boolean value)
	{
		return value ? "yes" : "no";
	}

	// ------------------------------------------- behaviour the fixture cannot state

	@Test
	public void theCatalogueHasAnEntryForEveryResolvableToken()
	{
		// Guards against a token that resolves but is missing from all(), which would
		// leave it out of the help text and the formats listing.
		for (FixtureCase testCase : Fixture.load("formats.txt"))
		{
			if (testCase.getError() != null)
				continue;

			boolean listed = false;

			for (FormatSpec spec : FormatCatalog.all())
				listed |= spec.getToken().equals(fold(testCase.subject));

			assertTrue(listed, testCase.describe("resolves, but is absent from FormatCatalog.all()"));
		}
	}

	@Test
	public void theCatalogueDeclaresNothingTheFixtureDoesNotList()
	{
		// The reverse direction: an implementation must not quietly support a format
		// the other three have never heard of.
		Set<String> expected = new HashSet<>();

		for (FixtureCase testCase : Fixture.load("formats.txt"))
			if (testCase.getError() == null)
				expected.add(testCase.subject);

		for (FormatSpec spec : FormatCatalog.all())
			assertTrue(expected.contains(spec.getToken()), "'" + spec.getToken() + "' is in the catalogue but not in formats.txt");
	}

	@Test
	public void everyCatalogueEntryHasAUniqueToken()
	{
		Set<String> seen = new HashSet<>();

		for (FormatSpec spec : FormatCatalog.all())
			assertTrue(seen.add(spec.getToken()), "duplicate token: " + spec.getToken());
	}

	@Test
	public void everyCatalogueEntryHasADisplayNameForHelpText()
	{
		// Not in the fixture because it is prose, and prose is allowed to differ
		// between languages in a way that ids and extensions are not.
		for (FormatSpec spec : FormatCatalog.all())
			assertFalse(spec.getDisplayName() == null || spec.getDisplayName().isEmpty(), spec.getToken() + " has no display name");
	}

	@Test
	public void tokensAreMatchedCaseInsensitivelyButCanonicaliseToLowerCase()
	{
		// Users type --to PDF; the plan dump must still say pdf, or the corpus
		// comparison becomes case sensitive by accident.
		assertEquals("pdf", FormatCatalog.resolve("PDF").getToken());
		assertEquals("md-text", FormatCatalog.resolve("MD-Text").getToken());
	}

	@Test
	public void theErrorNamesTheTokenAndWhereToFindTheValidOnes()
	{
		// The message points at the formats command rather than listing the
		// catalogue inline. Two dozen tokens in an error string would have to stay
		// byte-identical across four languages and be edited every time the
		// catalogue grows; a pointer does not.
		UsageError error = assertThrows(UsageError.class, () -> FormatCatalog.resolve("pdxf"));

		assertContains("pdxf", error.getMessage());
		assertContains("formats", error.getMessage());
	}

	@Test
	public void anUnknownTokenExitsWithTheUsageCode()
	{
		assertEquals(ExitCode.USAGE_ERROR, assertThrows(UsageError.class, () -> FormatCatalog.resolve("pdxf")).getExitCode());
	}

	@Test
	public void tryResolveReturnsNullRatherThanThrowing()
	{
		assertNull(FormatCatalog.tryResolve("nonsense"));
		assertNotNull(FormatCatalog.tryResolve("pdf"));
	}

	@Test
	public void aTextModeFormatIsNeverHighDefinitionCapable()
	{
		// Hi-def output is paginated by definition, and a text format never opens a
		// canvas to paginate onto.
		for (FormatSpec spec : FormatCatalog.all())
			if (spec.getKind() == FormatKind.TEXT)
				assertFalse(spec.isHiDefCapable(), spec.getToken() + " is a text format but claims hi-def");
	}
}
