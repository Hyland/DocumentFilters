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

import com.hyland.documentfilters.cli.execution.CanvasConversion;
import com.hyland.documentfilters.cli.planning.PageRange;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.UsageError;
import com.hyland.documentfilters.cli.tests.Test;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

/**
 * {@code --pages}. The specification gives one example, {@code 1,3-5,8}; everything
 * else here is a decision this suite pins down.
 */
public final class PageRangeTests
{
	@Test
	public void parsesTheExampleFromTheSpecification()
	{
		PageRange pageRange = PageRange.parse("1,3-5,8");

		assertEquals(Arrays.asList(1, 3, 4, 5, 8), list(pageRange.enumerate(10)));
	}

	@Test
	public void singlePageSelectsOnlyThatPage()
	{
		PageRange pageRange = PageRange.parse("4");

		assertTrue(pageRange.contains(4));
		assertFalse(pageRange.contains(3));
		assertFalse(pageRange.contains(5));
	}

	@Test
	public void pagesAreOneBased()
	{
		PageRange pageRange = PageRange.parse("1");

		assertTrue(pageRange.contains(1));
		assertFalse(pageRange.contains(0));
	}

	@Test
	public void overlappingRangesCollapseAndSort()
	{
		PageRange pageRange = PageRange.parse("3,1,1-2");

		assertEquals(Arrays.asList(1, 2, 3), list(pageRange.enumerate(10)));
	}

	@Test
	public void aSpanOfOnePageIsAllowed()
	{
		PageRange pageRange = PageRange.parse("2-2");

		assertEquals(Arrays.asList(2), list(pageRange.enumerate(10)));
	}

	@Test
	public void anExplicitSelectionIsReturnedRegardlessOfTheCountArgument()
	{
		// Clamping to the real page count is the caller's job, done once the
		// document is open; the range itself only knows what was asked for.
		PageRange pageRange = PageRange.parse("1,5-9");

		assertEquals(Arrays.asList(1, 5, 6, 7, 8, 9), list(pageRange.enumerate(6)));
	}

	@Test
	public void anExplicitSelectionIgnoresTheCountArgumentEntirely()
	{
		PageRange pageRange = PageRange.parse("10,20");

		assertEquals(Arrays.asList(10, 20), list(pageRange.enumerate(1)));
	}

	@Test
	public void allSelectsEveryPage()
	{
		assertTrue(PageRange.ALL.isAll());
		assertEquals(Arrays.asList(1, 2, 3), list(PageRange.ALL.enumerate(3)));
	}

	@Test
	public void anExplicitRangeIsNotAll()
	{
		assertFalse(PageRange.parse("1-3").isAll());
	}

	@Test
	public void malformedInputIsAUsageError()
	{
		for (String text : Arrays.asList("", "abc", "1,", ",1", "1--3", "1-", "-3", "1,,2"))
		{
			UsageError error = assertThrows(UsageError.class, () -> PageRange.parse(text));

			assertEquals(ErrorCodes.BAD_PAGE_RANGE, error.getCode(), "'" + text + "'");
			assertEquals(ExitCode.USAGE_ERROR, error.getExitCode(), "'" + text + "'");
		}
	}

	@Test
	public void pageZeroIsAUsageError()
	{
		assertThrows(UsageError.class, () -> PageRange.parse("0"));
	}

	@Test
	public void negativePagesAreAUsageError()
	{
		assertThrows(UsageError.class, () -> PageRange.parse("-1,3"));
	}

	@Test
	public void aDescendingSpanIsAUsageError()
	{
		assertThrows(UsageError.class, () -> PageRange.parse("5-3"));
	}

	@Test
	public void canonicalTextFormIsStableForTheConformanceCorpus()
	{
		// The plan dump prints the range, so equivalent inputs must print the same.
		assertEquals(PageRange.parse("1,2,3").toString(), PageRange.parse("1-3").toString());
	}

	@Test
	public void onlyPlainAsciiDigitsAreAPageNumber()
	{
		// Integer.parseInt takes a leading '+' and any Unicode decimal digit, so a
		// parser built on it alone would accept several of these. A trailing newline
		// is here because a regex ending in '$' matches just before one.
		for (String text : Arrays.asList(" 5", "5 ", "+5", "5_0", "5.0", "\u0663", "\u0665-\u0666", "5\n", "1-3\n"))
			assertThrows(UsageError.class, () -> PageRange.parse(text));
	}

	@Test
	public void aPageNumberBeyondThe32BitRangeIsAUsageError()
	{
		assertThrows(UsageError.class, () -> PageRange.parse("2147483648"));

		assertEquals(Arrays.asList(2147483647), list(PageRange.parse("2147483647").enumerate(1)));
	}

	@Test
	public void overlappingAndTouchingSpansMerge()
	{
		PageRange pageRange = PageRange.parse("3,1,1-2,5-7,6-9");

		assertEquals("1-3,5-9", pageRange.toString());
		assertEquals(1, pageRange.firstPage());
	}

	@Test
	public void theWidestSpanCostsNothingUntilItIsWalked()
	{
		// Two billion pages held as one span, not as a list of two billion numbers.
		PageRange pageRange = PageRange.parse("1-2147483647");

		assertEquals("1-2147483647", pageRange.toString());
		assertEquals(1, pageRange.firstPage());
		assertTrue(pageRange.contains(2147483647));
		assertFalse(pageRange.contains(0));
		assertEquals(Arrays.asList(1, 2, 3), take(pageRange.enumerate(3), 3));
		assertEquals(Arrays.asList(1, 2, 3), list(CanvasConversion.renderablePages(pageRange, 3)));
	}

	@Test
	public void aSpanEndingAtTheLastIntDoesNotWrap()
	{
		// An int counter stepping past Integer.MAX_VALUE wraps to a negative number
		// and never ends the loop.
		assertEquals(Arrays.asList(2147483646, 2147483647), list(PageRange.parse("2147483646-2147483647").enumerate(1)));
	}

	private static List<Integer> list(Iterable<Integer> pages)
	{
		List<Integer> result = new ArrayList<>();

		for (int page : pages)
			result.add(page);

		return result;
	}

	private static List<Integer> take(Iterable<Integer> pages, int count)
	{
		List<Integer> result = new ArrayList<>();
		Iterator<Integer> iterator = pages.iterator();

		while (result.size() < count && iterator.hasNext())
			result.add(iterator.next());

		return result;
	}
}
