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
import static com.hyland.documentfilters.cli.tests.Assert.assertDoesNotContain;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.runtime.JsonWriter;
import com.hyland.documentfilters.cli.tests.Test;

/**
 * The hand-rolled JSON writer. Key order, indent, escaping and the trailing newline
 * are all contract: the four implementations must produce the same bytes.
 */
public final class JsonWriterTests
{
	@Test
	public void anEmptyObjectRendersOnOneLine()
	{
		assertEquals("{}\n", new JsonWriter().beginObject().endObject().toString());
	}

	@Test
	public void propertiesAreIndentedByTwoSpaces()
	{
		String json = new JsonWriter()
			.beginObject()
			.property("name", "report.pdf")
			.endObject()
			.toString();

		assertEquals("{\n  \"name\": \"report.pdf\"\n}\n", json);
	}

	@Test
	public void keysStayInInsertionOrderAndAreNeverSorted()
	{
		// Sorting would be a gratuitous difference from the order the engine reports
		// fields in.
		String json = new JsonWriter()
			.beginObject()
			.property("zebra", 1)
			.property("apple", 2)
			.endObject()
			.toString();

		assertTrue(json.indexOf("zebra") < json.indexOf("apple"), json);
	}

	@Test
	public void nestedObjectsIndentCumulatively()
	{
		String json = new JsonWriter()
			.beginObject()
			.name("inner").beginObject().property("a", 1).endObject()
			.endObject()
			.toString();

		assertEquals("{\n  \"inner\": {\n    \"a\": 1\n  }\n}\n", json);
	}

	@Test
	public void arraysRenderOneElementPerLine()
	{
		String json = new JsonWriter()
			.beginArray()
			.value("a")
			.value("b")
			.endArray()
			.toString();

		assertEquals("[\n  \"a\",\n  \"b\"\n]\n", json);
	}

	@Test
	public void anEmptyArrayRendersOnOneLine()
	{
		assertEquals("[]\n", new JsonWriter().beginArray().endArray().toString());
	}

	@Test
	public void booleansAndNumbersAreUnquoted()
	{
		String json = new JsonWriter()
			.beginObject()
			.property("count", 42)
			.property("ok", true)
			.endObject()
			.toString();

		assertContains("\"count\": 42", json);
		assertContains("\"ok\": true", json);
	}

	@Test
	public void aNullStringRendersAsNull()
	{
		String json = new JsonWriter().beginObject().property("modified", (String) null).endObject().toString();

		assertContains("\"modified\": null", json);
	}

	@Test
	public void theMandatoryEscapesAreApplied()
	{
		String[][] cases = {
			{ "a\"b", "a\\\"b" },
			{ "a\\b", "a\\\\b" },
			{ "a\nb", "a\\nb" },
			{ "a\tb", "a\\tb" },
			{ "a\rb", "a\\rb" },
		};

		for (String[] testCase : cases)
			assertEquals(testCase[1], JsonWriter.escape(testCase[0]), testCase[1]);
	}

	@Test
	public void controlCharactersUseTheSixCharacterForm()
	{
		assertEquals("\\u0000", JsonWriter.escape("\0"));
	}

	@Test
	public void nonAsciiIsWrittenLiterallyRatherThanEscaped()
	{
		// Escaping it would be legal JSON, but the four implementations must produce
		// the same bytes for the same document.
		assertEquals("\u00dcn\u00efc\u00f6d\u00e9", JsonWriter.escape("\u00dcn\u00efc\u00f6d\u00e9"));
	}

	@Test
	public void aForwardSlashIsNotEscaped()
	{
		// Legal either way in JSON, so it has to be pinned down.
		assertEquals("a/b", JsonWriter.escape("a/b"));
	}

	@Test
	public void aSurrogatePairIsWrittenLiterally()
	{
		// U+1F600, held as two UTF-16 chars, which the writer must not mistake for
		// two unpaired surrogates.
		assertEquals("\ud83d\ude00", JsonWriter.escape("\ud83d\ude00"));
	}

	@Test
	public void anUnpairedHighSurrogateIsEscaped()
	{
		assertEquals("\\ud800", JsonWriter.escape("\ud800"));
	}

	@Test
	public void anUnpairedLowSurrogateIsEscaped()
	{
		assertEquals("\\udfff", JsonWriter.escape("\udfff"));
	}

	@Test
	public void anUnpairedSurrogateBesideAPairLeavesThePairIntact()
	{
		// A high surrogate followed by the start of a valid pair: the first is
		// unpaired, and a rule that consumed two chars on seeing a high surrogate
		// would swallow the emoji.
		assertEquals("\\ud800\ud83d\ude00", JsonWriter.escape("\ud800\ud83d\ude00"));
	}

	@Test
	public void anEscapedDocumentCarriesNoSurrogatesOfItsOwn()
	{
		String json = new JsonWriter()
			.beginObject()
			.property("title", "a\ud800b")
			.endObject()
			.toString();

		assertContains("\"title\": \"a\\ud800b\"", json);
		for (int i = 0; i < json.length(); i++)
			assertTrue(!Character.isSurrogate(json.charAt(i)), "surrogate at index " + i);
	}

	@Test
	public void theDocumentEndsWithExactlyOneNewline()
	{
		String json = new JsonWriter().beginObject().property("a", 1).endObject().toString();

		assertTrue(json.endsWith("}\n"), json);
		assertDoesNotContain("\n\n", json);
	}

	@Test
	public void lineEndingsAreAlwaysLfEvenOnWindows()
	{
		String json = new JsonWriter().beginObject().property("a", 1).endObject().toString();

		assertDoesNotContain("\r", json);
	}

	@Test
	public void anUnclosedObjectIsAProgrammingError()
	{
		assertThrows(IllegalStateException.class, () -> new JsonWriter().beginObject().toString());
	}
}
