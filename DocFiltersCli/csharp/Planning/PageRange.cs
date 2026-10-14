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

using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Planning
{
	/// <summary>
	/// A <c>--pages</c> selection such as <c>1,3-5,8</c>. Pages are one-based, as
	/// the user writes them.
	/// </summary>
	public sealed class PageRange
	{
		// Null means "every page" (PageRange.All); otherwise the selected one-based
		// pages as inclusive From-To spans, ascending, and merged so that no two
		// overlap or touch.
		//
		// Spans and not the pages themselves, because a span costs the same however
		// wide it is. Expanding one into a list made --pages 1-2147483647 — a valid
		// value — allocate two billion entries while the arguments were still being
		// read.
		private readonly List<(int From, int To)> _spans;
		private readonly string _canonicalText;

		private PageRange(List<(int From, int To)> spans, string canonicalText)
		{
			_spans = spans;
			_canonicalText = canonicalText;
		}

		/// <summary>Selects every page.</summary>
		public static PageRange All { get; } = new PageRange(null, "all");

		/// <summary>True when no restriction was given.</summary>
		public bool IsAll => _spans == null;

		/// <summary>The lowest page selected: 1 for <see cref="All"/>.</summary>
		public int FirstPage => IsAll ? 1 : _spans[0].From;

        /// <summary>
        /// Parses a range expression. Accepts comma-separated single pages and
        /// <c>from-to</c> spans. Duplicates collapse and the result is ascending, so
        /// <c>3,1,1-2</c> and <c>1,2,3</c> are the same selection.
        /// </summary>
        /// <exception cref="Runtime.UsageError">
        /// Malformed syntax, a page below 1, or a descending span such as <c>5-3</c>.
        /// </exception>
        public static PageRange Parse(string text)
		{
			if (string.IsNullOrEmpty(text))
				Throw();

			var given = new List<(int From, int To)>();
			foreach (string token in text.Split(','))
			{
				if (token.Length == 0)
					Throw(); // a leading, trailing or doubled comma

				int dash = token.IndexOf('-');
				if (dash < 0)
				{
					int page = ParsePageNumber(token);
					given.Add((page, page));
				}
				else
				{
					int from = ParsePageNumber(token.Substring(0, dash));
					int to = ParsePageNumber(token.Substring(dash + 1));

					if (to < from)
						Throw(); // "5-3": far more likely a typo than a request to reverse

					given.Add((from, to));
				}
			}

			List<(int From, int To)> spans = Merge(given);
			return new PageRange(spans, BuildCanonicalText(spans));
		}

		/// <summary>
		/// Sorts spans and joins every pair that overlaps or touches, so <c>3,1,1-2</c>
		/// and <c>1-3</c> become the same single span.
		/// </summary>
		private static List<(int From, int To)> Merge(List<(int From, int To)> spans)
		{
			spans.Sort((a, b) => a.From.CompareTo(b.From));

			var merged = new List<(int From, int To)>();
			foreach ((int From, int To) span in spans)
			{
				// long, because the page after int.MaxValue is not an int.
				if (merged.Count > 0 && span.From <= (long)merged[merged.Count - 1].To + 1)
				{
					(int From, int To) last = merged[merged.Count - 1];
					merged[merged.Count - 1] = (last.From, Math.Max(last.To, span.To));
				}
				else
				{
					merged.Add(span);
				}
			}

			return merged;
		}

		private static int ParsePageNumber(string text)
		{
			if (string.IsNullOrEmpty(text))
				Throw();

			if (!int.TryParse(text, out int value) || value < 1)
				Throw();

			return value;
		}

		private static void Throw()
		{
			throw new UsageError(ErrorCodes.BadPageRange, "--pages must be a list of pages or ranges, such as 1,3-5,8");
		}

		/// <summary>
		/// Writes merged spans in their canonical form, e.g. <c>1,3,4,5,8</c> becomes
		/// <c>1,3-5,8</c>: a run of three or more pages as <c>from-to</c>, and a run of
		/// two as the two pages.
		/// </summary>
		private static string BuildCanonicalText(IReadOnlyList<(int From, int To)> spans)
		{
			var parts = new List<string>();
			foreach ((int start, int end) in spans)
			{
				if (end - start <= 1)
				{
					parts.Add(start.ToString());
					if (end != start)
						parts.Add(end.ToString());
				}
				else
				{
					parts.Add($"{start}-{end}");
				}
			}

			return string.Join(",", parts);
		}

		/// <summary>True when this selection includes the given one-based page number.</summary>
		public bool Contains(int pageNumber)
		{
			return IsAll ? pageNumber >= 1 : _spans.Any(span => span.From <= pageNumber && pageNumber <= span.To);
		}

		/// <summary>
		/// The selected page numbers, ascending: every page of a document with
		/// <paramref name="pageCount"/> pages for <see cref="All"/>, or the explicit
		/// selection exactly as given otherwise.
		/// </summary>
		/// <remarks>
		/// Lazy, so a caller that stops early pays only for the pages it took: a span
		/// can be two billion pages wide, and a document is rarely that long.
		///
		/// An explicit selection is never filtered against <paramref name="pageCount"/>
		/// here — a count is all this class is ever given, not a verified list of which
		/// page numbers are real, and not every caller even has a genuine one yet: a
		/// plan built before the document is open has nothing better than a guess.
		/// Deciding whether a requested page actually exists belongs to whoever opens
		/// it, once it has a count it can trust; this only ever answers "what did the
		/// user ask for".
		/// </remarks>
		public IEnumerable<int> Enumerate(int pageCount)
		{
			if (IsAll)
				return Enumerable.Range(1, Math.Max(0, pageCount));

			return EnumerateSpans();
		}

		private IEnumerable<int> EnumerateSpans()
		{
			foreach ((int from, int to) in _spans)
			{
				// Not `page <= to`: with to at int.MaxValue that is always true, and the
				// increment past it wraps round to a negative page.
				for (int page = from; ; page++)
				{
					yield return page;
					if (page == to)
						break;
				}
			}
		}

		/// <summary>The canonical text form, used by the conformance corpus.</summary>
		public override string ToString()
		{
			return _canonicalText;
		}
	}
}
