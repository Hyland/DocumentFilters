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

package com.hyland.documentfilters.cli.planning;

import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.UsageError;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * A {@code --pages} selection such as {@code 1,3-5,8}. Pages are one-based, as
 * the user writes them.
 */
public final class PageRange
{
	/** Selects every page. */
	public static final PageRange ALL = new PageRange(null, "all");

	// Null means "every page" (ALL); otherwise the selected one-based pages as
	// inclusive from-to spans, ascending, and merged so that no two overlap or
	// touch.
	//
	// Spans and not the pages themselves, because a span costs the same however
	// wide it is. Expanding one into a list made --pages 1-2147483647 --- a valid
	// value --- allocate two billion entries while the arguments were still being
	// read.
	private final List<int[]> _spans;
	private final String _canonicalText;

	private PageRange(List<int[]> spans, String canonicalText)
	{
		_spans = spans;
		_canonicalText = canonicalText;
	}

	/** True when no restriction was given. */
	public boolean isAll()
	{
		return _spans == null;
	}

	/** The lowest page selected: 1 for {@link #ALL}. */
	public int firstPage()
	{
		return isAll() ? 1 : _spans.get(0)[0];
	}

	/**
	 * Parses a range expression. Accepts comma-separated single pages and
	 * {@code from-to} spans. Duplicates collapse and the result is ascending, so
	 * {@code 3,1,1-2} and {@code 1,2,3} are the same selection.
	 *
	 * @throws UsageError malformed syntax, a page below 1, or a descending span
	 *         such as {@code 5-3}.
	 */
	public static PageRange parse(String text)
	{
		if (text == null || text.isEmpty())
			throw error();

		List<int[]> given = new ArrayList<>();
		for (String token : ValueParsers.split(text, ','))
		{
			if (token.isEmpty())
				throw error(); // a leading, trailing or doubled comma

			int dash = token.indexOf('-');
			if (dash < 0)
			{
				int page = parsePageNumber(token);
				given.add(new int[] { page, page });
			}
			else
			{
				int from = parsePageNumber(token.substring(0, dash));
				int to = parsePageNumber(token.substring(dash + 1));

				if (to < from)
					throw error(); // "5-3": far more likely a typo than a request to reverse

				given.add(new int[] { from, to });
			}
		}

		List<int[]> spans = merge(given);
		return new PageRange(spans, buildCanonicalText(spans));
	}

	/**
	 * Sorts spans and joins every pair that overlaps or touches, so {@code 3,1,1-2}
	 * and {@code 1-3} become the same single span.
	 */
	private static List<int[]> merge(List<int[]> spans)
	{
		Collections.sort(spans, (a, b) -> Integer.compare(a[0], b[0]));

		List<int[]> merged = new ArrayList<>();
		for (int[] span : spans)
		{
			// long, because the page after Integer.MAX_VALUE is not an int.
			if (!merged.isEmpty() && span[0] <= (long)merged.get(merged.size() - 1)[1] + 1)
			{
				int[] last = merged.get(merged.size() - 1);
				last[1] = Math.max(last[1], span[1]);
			}
			else
			{
				merged.add(span);
			}
		}

		return merged;
	}

	/**
	 * One page number: ASCII digits, at least 1, within an int. Strict, as
	 * {@link ValueParsers} is, so {@code +3} and {@code " 3"} are rejected.
	 */
	private static int parsePageNumber(String text)
	{
		Long value = ValueParsers.tryParseDigits(text, Integer.MAX_VALUE);
		if (value == null || value < 1)
			throw error();

		return value.intValue();
	}

	private static UsageError error()
	{
		return new UsageError(ErrorCodes.BAD_PAGE_RANGE, "--pages must be a list of pages or ranges, such as 1,3-5,8");
	}

	/**
	 * Writes merged spans in their canonical form, e.g. {@code 1,3,4,5,8} becomes
	 * {@code 1,3-5,8}: a run of three or more pages as {@code from-to}, and a run of
	 * two as the two pages.
	 */
	private static String buildCanonicalText(List<int[]> spans)
	{
		List<String> parts = new ArrayList<>();
		for (int[] span : spans)
		{
			int start = span[0];
			int end = span[1];
			if (end - start <= 1)
			{
				parts.add(Integer.toString(start));
				if (end != start)
					parts.add(Integer.toString(end));
			}
			else
			{
				parts.add(start + "-" + end);
			}
		}

		return String.join(",", parts);
	}

	/** True when this selection includes the given one-based page number. */
	public boolean contains(int pageNumber)
	{
		if (isAll())
			return pageNumber >= 1;
		for (int[] span : _spans)
		{
			if (span[0] <= pageNumber && pageNumber <= span[1])
				return true;
		}
		return false;
	}

	/**
	 * The selected page numbers, ascending: every page of a document with
	 * {@code pageCount} pages for {@link #ALL}, or the explicit selection exactly
	 * as given otherwise.
	 *
	 * <p>Lazy, so a caller that stops early pays only for the pages it took: a span
	 * can be two billion pages wide, and a document is rarely that long.
	 *
	 * <p>An explicit selection is never filtered against {@code pageCount} here ---
	 * a count is all this class is ever given, not a verified list of which page
	 * numbers are real, and not every caller even has a genuine one yet: a plan
	 * built before the document is open has nothing better than a guess. Deciding
	 * whether a requested page actually exists belongs to whoever opens it, once it
	 * has a count it can trust; this only ever answers "what did the user ask for".
	 */
	public Iterable<Integer> enumerate(int pageCount)
	{
		final List<int[]> spans = isAll()
			? (pageCount > 0 ? Collections.singletonList(new int[] { 1, pageCount }) : Collections.<int[]>emptyList())
			: _spans;
		return () -> new SpanIterator(spans);
	}

	/** The canonical text form, used by the conformance corpus. */
	@Override
	public String toString()
	{
		return _canonicalText;
	}

	private static final class SpanIterator implements Iterator<Integer>
	{
		private final List<int[]> _spans;
		private int _span;
		private int _next;

		SpanIterator(List<int[]> spans)
		{
			_spans = spans;
			_next = spans.isEmpty() ? 0 : spans.get(0)[0];
		}

		@Override
		public boolean hasNext()
		{
			return _span < _spans.size();
		}

		@Override
		public Integer next()
		{
			if (!hasNext())
				throw new NoSuchElementException();

			int page = _next;
			// Not page < to then ++: with to at Integer.MAX_VALUE the increment past it
			// wraps round to a negative page. Compare first, step after.
			if (page == _spans.get(_span)[1])
			{
				++_span;
				if (_span < _spans.size())
					_next = _spans.get(_span)[0];
			}
			else
			{
				_next = page + 1;
			}
			return page;
		}
	}
}
