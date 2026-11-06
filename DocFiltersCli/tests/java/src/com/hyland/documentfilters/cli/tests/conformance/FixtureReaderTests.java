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

package com.hyland.documentfilters.cli.tests.conformance;

import static com.hyland.documentfilters.cli.tests.Assert.assertContains;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.tests.Test;
import java.util.Arrays;
import java.util.Collections;

/**
 * The shared fixture reader, which every suite reimplements in about twenty lines.
 */
public final class FixtureReaderTests
{
	@Test
	public void commentsAndBlankLinesAreIgnored()
	{
		assertEquals(1, Fixture.parse("test.txt", "# a comment", "", "$ identify a.bin", "= exit 0").size());
	}

	@Test
	public void aCaseCollectsItsExpectationsInOrder()
	{
		FixtureCase parsed = Fixture.parse("test.txt", "$ identify a.bin", "= command identify", "= exit 0").get(0);

		assertEquals("identify a.bin", parsed.subject);
		assertEquals(2, parsed.expectations.size());
		assertEquals("identify", parsed.get("command"));
	}

	@Test
	public void aRepeatedKeyKeepsEveryValue()
	{
		// How multi-target and multi-argument cases are expressed.
		FixtureCase parsed = Fixture.parse("test.txt", "$ convert", "= target out/a.pdf", "= target out/b.pdf").get(0);

		assertEquals(Arrays.asList("out/a.pdf", "out/b.pdf"), parsed.getAll("target"));
	}

	@Test
	public void anErrorExpectationIsRecognised()
	{
		FixtureCase parsed = Fixture.parse("test.txt", "$ convert --to pdxf a.docx", "= error E_UNKNOWN_FORMAT", "= exit 2").get(0);

		assertEquals("E_UNKNOWN_FORMAT", parsed.getError());
	}

	@Test
	public void yesAndNoExpectationsParseAsBooleans()
	{
		FixtureCase parsed = Fixture.parse("test.txt", "$ pdf", "= single-file yes", "= hidef no").get(0);

		assertTrue(parsed.getBool("single-file"));
		assertFalse(parsed.getBool("hidef"));
	}

	@Test
	public void anythingOtherThanYesOrNoIsRejected()
	{
		FixtureCase parsed = Fixture.parse("test.txt", "$ pdf", "= single-file true").get(0);

		assertThrows(FixtureFormatException.class, () -> parsed.getBool("single-file"));
	}

	@Test
	public void anEmptySubjectIsAllowed()
	{
		// formats.txt uses one to assert that an empty --to is rejected.
		assertEquals("", Fixture.parse("test.txt", "$", "= error E_UNKNOWN_FORMAT").get(0).subject);
	}

	@Test
	public void anExpectationBeforeAnyCaseIsAnError()
	{
		assertThrows(FixtureFormatException.class, () -> Fixture.parse("test.txt", "= command convert"));
	}

	@Test
	public void aCaseKeepsWhatItIsGivenApartFromWhatItExpects()
	{
		FixtureCase parsed = Fixture.parse("test.txt", "$ convert a.docx", "< tree a.docx", "= command convert").get(0);

		assertEquals(Collections.singletonList("a.docx"), parsed.givenAll("tree"));
		assertEquals(1, parsed.expectations.size());
		assertFalse(parsed.has("tree"));
	}

	@Test
	public void whatACaseIsGivenComesBeforeWhatItExpects()
	{
		// '<' describes the world before the run and '=' what the run must produce, so
		// a fixture that interleaved them would read as though the order meant
		// something.
		assertThrows(FixtureFormatException.class,
			() -> Fixture.parse("test.txt", "$ convert a.docx", "= command convert", "< tree a.docx"));
	}

	@Test
	public void aGivenLineBeforeAnyCaseIsAnError()
	{
		assertThrows(FixtureFormatException.class, () -> Fixture.parse("test.txt", "< tree a.docx"));
	}

	@Test
	public void anUnrecognisedLineIsAnError()
	{
		// Rather than being skipped, which would silently drop expectations.
		assertThrows(FixtureFormatException.class, () -> Fixture.parse("test.txt", "$ convert", "command convert"));
	}

	@Test
	public void aFailureMessageNamesTheFileTheLineAndTheWholeCase()
	{
		// The reason externalising these cases is affordable at all: a failure has to
		// be readable without opening the fixture.
		String message = Fixture.parse("formats.txt", "# header", "$ pdf", "= kind canvas", "= ext pdf").get(0)
			.describe("kind was text");

		assertContains("formats.txt:2", message);
		assertContains("$ pdf", message);
		assertContains("kind was text", message);
		assertContains("= kind canvas", message);
		assertContains("= ext pdf", message);
	}

	@Test
	public void quotedArgumentsMayContainSpaces()
	{
		assertEquals(
			Arrays.asList("convert", "--options", "BW_MODE=3;X=1", "a.docx"),
			Arrays.asList(PlanCorpus.splitArguments("convert --options \"BW_MODE=3;X=1\" a.docx")));
	}
}
