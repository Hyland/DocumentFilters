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
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.configuration.FormValue;
import com.hyland.documentfilters.cli.configuration.OnErrorBehaviour;
import com.hyland.documentfilters.cli.configuration.RedactSpec;
import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.UsageError;
import com.hyland.documentfilters.cli.tests.Test;
import java.util.Arrays;
import java.util.Collections;

/**
 * The conversions that can fail. These live in shared code precisely so that a bad
 * value produces the same message and the same exit code in the C#, Python, Java
 * and C++17 builds, rather than four different parsing libraries' wording.
 */
public final class ValueParsersTests
{
	@Test
	public void countsParse()
	{
		assertEquals(0, ValueParsers.parseCount("--max-depth", "0"));
		assertEquals(1, ValueParsers.parseCount("--max-depth", "1"));
		assertEquals(42, ValueParsers.parseCount("--max-depth", "42"));
	}

	@Test
	public void aBadCountIsAUsageError()
	{
		for (String text : new String[] { "abc", "", "-1", "1.5", "1e3", " 1", "+1" })
		{
			UsageError error = assertThrows(UsageError.class, () -> ValueParsers.parseCount("--max-depth", text));

			assertEquals(ErrorCodes.BAD_VALUE, error.getCode(), text);
		}
	}

	/**
	 * Not in the reference, where the hazard cannot arise. Java's own parsers would
	 * accept a sign and other digit scripts, which is why this port checks the
	 * digits itself; a trailing newline is the case a regex-based check gets wrong.
	 */
	@Test
	public void aTrailingNewlineIsABadValueForEveryStrictParser()
	{
		assertEquals(ErrorCodes.BAD_VALUE, assertThrows(UsageError.class, () -> ValueParsers.parseCount("--max-depth", "5\n")).getCode());
		assertEquals(ErrorCodes.BAD_VALUE, assertThrows(UsageError.class, () -> ValueParsers.parseSeconds("--timeout", "5\n")).getCode());
		assertEquals(ErrorCodes.BAD_VALUE,
			assertThrows(UsageError.class, () -> ValueParsers.parseMegabytes("--max-memory", "5\n")).getCode());
	}

	/** Not in the reference: {@code Integer.parseInt} would take these, and the reference does not. */
	@Test
	public void digitsFromOtherScriptsAreABadValue()
	{
		// ARABIC-INDIC DIGIT FIVE, which Character.isDigit and Integer.parseInt accept.
		assertThrows(UsageError.class, () -> ValueParsers.parseCount("--max-depth", "\u0665"));
	}

	@Test
	public void aCountBeyondIntRangeIsABadValue()
	{
		assertThrows(UsageError.class, () -> ValueParsers.parseCount("--max-depth", "2147483648"));
		assertEquals(2147483647, ValueParsers.parseCount("--max-depth", "2147483647"));
	}

	@Test
	public void theErrorNamesTheArgumentThatWasWrong()
	{
		// Without the argument name, a batch of options makes the message useless.
		UsageError error = assertThrows(UsageError.class, () -> ValueParsers.parseCount("--max-depth", "abc"));

		assertContains("--max-depth", error.getMessage());
	}

	// ------------------------------------------------------------------------ seconds

	@Test
	public void durationsParse()
	{
		assertEquals(1, ValueParsers.parseSeconds("--timeout", "1"));
		assertEquals(60, ValueParsers.parseSeconds("--timeout", "60"));
		assertEquals(86400, ValueParsers.parseSeconds("--timeout", "86400"));
	}

	@Test
	public void aBadDurationIsAUsageError()
	{
		// Zero is rejected as well as negative: a timeout of nothing is a mistake, not
		// a request to finish instantly.
		for (String text : new String[] { "0", "-1", "abc", "", "1.5", "30s" })
		{
			UsageError error = assertThrows(UsageError.class, () -> ValueParsers.parseSeconds("--timeout", text));

			assertEquals(ErrorCodes.BAD_VALUE, error.getCode(), text);
		}
	}

	// --------------------------------------------------------------------- megabytes

	@Test
	public void memoryLimitsParse()
	{
		assertEquals(1L, ValueParsers.parseMegabytes("--max-memory", "1"));
		assertEquals(512L, ValueParsers.parseMegabytes("--max-memory", "512"));
		assertEquals(8192L, ValueParsers.parseMegabytes("--max-memory", "8192"));
	}

	@Test
	public void aMemoryLimitBeyondIntRangeStillParses()
	{
		// The value is megabytes and the field is a long, so a limit larger than
		// Integer.MAX_VALUE is legitimate rather than an overflow.
		assertEquals(4194304L, ValueParsers.parseMegabytes("--max-memory", "4194304"));
		assertEquals(3000000000L, ValueParsers.parseMegabytes("--max-memory", "3000000000"));
	}

	@Test
	public void aBadMemoryLimitIsAUsageError()
	{
		for (String text : new String[] { "0", "-1", "abc", "", "512MB", "1.5" })
		{
			UsageError error = assertThrows(UsageError.class, () -> ValueParsers.parseMegabytes("--max-memory", text));

			assertEquals(ErrorCodes.BAD_VALUE, error.getCode(), text);
		}
	}

	@Test
	public void bothLimitParsersNameTheArgumentThatWasWrong()
	{
		assertContains("--timeout", assertThrows(UsageError.class, () -> ValueParsers.parseSeconds("--timeout", "x")).getMessage());
		assertContains("--max-memory",
			assertThrows(UsageError.class, () -> ValueParsers.parseMegabytes("--max-memory", "x")).getMessage());
	}

	// ---------------------------------------------------------------------- id lists

	@Test
	public void idListsSplitOnSemicolons()
	{
		assertEquals(Arrays.asList("PDF", "DOCX"), ValueParsers.parseIdList("PDF;DOCX"));
	}

	@Test
	public void idListsTrimWhitespaceAndDropEmptyEntries()
	{
		assertEquals(Arrays.asList("PDF", "DOCX"), ValueParsers.parseIdList(" PDF ; ; DOCX ;"));
	}

	@Test
	public void aSingleIdNeedsNoSeparator()
	{
		assertEquals(Arrays.asList("PDF"), ValueParsers.parseIdList("PDF"));
	}

	// ------------------------------------------------------------------------- trimming

	/**
	 * Not in the reference, whose {@code Trim} is the definition. Java's
	 * {@code String.trim} removes only U+0000 to U+0020, so the no-break and wide
	 * spaces below are the ones it would leave behind.
	 */
	@Test
	public void trimRemovesTheWhitespaceTheReferenceCallsWhitespace()
	{
		for (String text : new String[] { " PDF ", "\tPDF\t", "\r\nPDF\r\n", "\u000bPDF\u000c", "\u0085PDF\u0085",
			"\u00a0PDF\u00a0", "\u2007PDF\u202f", "\u3000PDF\u3000" })
			assertEquals("PDF", ValueParsers.trim(text), text);
	}

	/**
	 * The other half of the same difference: {@code Character.isWhitespace} counts
	 * the C0 information separators, and the reference's {@code char.IsWhiteSpace}
	 * does not, so an id ending in one is the same token in all four ports.
	 */
	@Test
	public void trimKeepsTheC0InformationSeparators()
	{
		for (char character : new char[] { '\u001c', '\u001d', '\u001e', '\u001f' })
		{
			assertFalse(ValueParsers.isWhiteSpace(character));
			assertTrue(Character.isWhitespace(character), "the JDK's own definition is what this is not");
			assertEquals("PDF" + character, ValueParsers.trim("PDF" + character));
			assertEquals(Collections.singletonList(String.valueOf(character)), ValueParsers.parseIdList(String.valueOf(character)));
		}
	}

	@Test
	public void trimOfNothingButWhitespaceIsEmpty()
	{
		assertEquals("", ValueParsers.trim("  \t\r\n "));
		assertEquals("", ValueParsers.trim(""));
	}

	@Test
	public void isNullOrWhiteSpaceMatchesTheReference()
	{
		for (String text : new String[] { null, "", " ", "\t\r\n" })
			assertTrue(ValueParsers.isNullOrWhiteSpace(text), String.valueOf(text));

		for (String text : new String[] { "a", " a ", "\u001c" })
			assertFalse(ValueParsers.isNullOrWhiteSpace(text), text);
	}

	// -------------------------------------------------------------------- redactions

	@Test
	public void aRedactionParsesAsPageThenRectangle()
	{
		RedactSpec spec = ValueParsers.parseRedaction("2,10,20,100,50");

		assertEquals(2, spec.page);
		assertEquals(10, spec.x);
		assertEquals(20, spec.y);
		assertEquals(100, spec.width);
		assertEquals(50, spec.height);
	}

	@Test
	public void aMalformedRedactionIsAUsageError()
	{
		for (String text : new String[] { "2,10,20,100", "2,10,20,100,50,60", "2,10,20,100,abc", "" })
			assertThrows(UsageError.class, () -> ValueParsers.parseRedaction(text));
	}

	// ------------------------------------------------------------------- form values

	@Test
	public void aFormValueParsesAsPageThenNameEqualsValue()
	{
		FormValue value = ValueParsers.parseFormValue("1,surname=Smith");

		assertEquals(1, value.page);
		assertEquals("surname", value.name);
		assertEquals("Smith", value.value);
	}

	@Test
	public void onlyTheFirstEqualsSeparatesNameFromValue()
	{
		// Form values legitimately contain '=', for instance a base64 payload.
		FormValue value = ValueParsers.parseFormValue("1,token=abc=def=");

		assertEquals("token", value.name);
		assertEquals("abc=def=", value.value);
	}

	@Test
	public void anEmptyFormValueIsAllowed()
	{
		// Clearing a field is a legitimate operation.
		assertEquals("", ValueParsers.parseFormValue("1,surname=").value);
	}

	@Test
	public void aMalformedFormValueIsAUsageError()
	{
		for (String text : new String[] { "1,surname", "surname=Smith", "" })
			assertThrows(UsageError.class, () -> ValueParsers.parseFormValue(text));
	}

	// ----------------------------------------------------------------------- on-error

	@Test
	public void onErrorAcceptsExactlyTheTwoDocumentedValues()
	{
		assertEquals(OnErrorBehaviour.SKIP, ValueParsers.parseOnError("skip"));
		assertEquals(OnErrorBehaviour.STOP, ValueParsers.parseOnError("stop"));
	}

	@Test
	public void anythingElseForOnErrorIsAUsageError()
	{
		// Case sensitive on purpose: if one implementation accepted "SKIP" and another
		// did not, the four would diverge on what is a valid command line.
		for (String text : new String[] { "SKIP", "Skip", "continue", "" })
			assertThrows(UsageError.class, () -> ValueParsers.parseOnError(text));
	}
}
