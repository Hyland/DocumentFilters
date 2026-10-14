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

using System.Linq;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// <c>--pages</c>. The specification gives one example, <c>1,3-5,8</c>; everything
	/// else here is a decision this suite pins down.
	/// </summary>
	public class PageRangeTests
	{
		[Fact]
		public void Parses_the_example_from_the_specification()
		{
			PageRange range = PageRange.Parse("1,3-5,8");

			Assert.Equal(new[] { 1, 3, 4, 5, 8 }, range.Enumerate(10).ToArray());
		}

		[Fact]
		public void Single_page_selects_only_that_page()
		{
			PageRange range = PageRange.Parse("4");

			Assert.True(range.Contains(4));
			Assert.False(range.Contains(3));
			Assert.False(range.Contains(5));
		}

		[Fact]
		public void Pages_are_one_based()
		{
			PageRange range = PageRange.Parse("1");

			Assert.True(range.Contains(1));
			Assert.False(range.Contains(0));
		}

		[Fact]
		public void Overlapping_ranges_collapse_and_sort()
		{
			// The user may write ranges in any order, with overlaps. The selection is
			// a set, so this is the same as "1,2,3".
			PageRange range = PageRange.Parse("3,1,1-2");

			Assert.Equal(new[] { 1, 2, 3 }, range.Enumerate(10).ToArray());
		}

		[Fact]
		public void A_span_of_one_page_is_allowed()
		{
			PageRange range = PageRange.Parse("2-2");

			Assert.Equal(new[] { 2 }, range.Enumerate(10).ToArray());
		}

		[Fact]
		public void An_explicit_selection_is_returned_regardless_of_the_count_argument()
		{
			// Enumerate's argument is not a verified fact about the document — callers
			// that do not have one yet pass a placeholder — so an explicit selection is
			// never filtered by it here. Whichever of these pages a document of 6 pages
			// does not actually have is discovered when execution opens it, not guessed
			// at from a count handed to this method.
			PageRange range = PageRange.Parse("1,5-9");

			Assert.Equal(new[] { 1, 5, 6, 7, 8, 9 }, range.Enumerate(6).ToArray());
		}

		[Fact]
		public void An_explicit_selection_ignores_the_count_argument_entirely()
		{
			// Not just "not clamped to 6" — genuinely unaffected by it, including a
			// count too small to hold any of the selection at all.
			PageRange range = PageRange.Parse("10,20");

			Assert.Equal(new[] { 10, 20 }, range.Enumerate(1).ToArray());
		}

		[Fact]
		public void All_selects_every_page()
		{
			Assert.True(PageRange.All.IsAll);
			Assert.Equal(new[] { 1, 2, 3 }, PageRange.All.Enumerate(3).ToArray());
		}

		[Fact]
		public void An_explicit_range_is_not_All()
		{
			Assert.False(PageRange.Parse("1-3").IsAll);
		}

		[Theory]
		[InlineData("")]
		[InlineData("abc")]
		[InlineData("1,")]
		[InlineData(",1")]
		[InlineData("1--3")]
		[InlineData("1-")]
		[InlineData("-3")]
		[InlineData("1,,2")]
		public void Malformed_input_is_a_usage_error(string text)
		{
			UsageError error = Assert.Throws<UsageError>(() => PageRange.Parse(text));

			Assert.Equal(ErrorCodes.BadPageRange, error.Code);
			Assert.Equal(ExitCode.UsageError, error.ExitCode);
		}

		[Fact]
		public void Page_zero_is_a_usage_error()
		{
			// Pages are one-based as the user writes them, so 0 is meaningless rather
			// than a synonym for the first page.
			Assert.Throws<UsageError>(() => PageRange.Parse("0"));
		}

		[Fact]
		public void Negative_pages_are_a_usage_error()
		{
			Assert.Throws<UsageError>(() => PageRange.Parse("-1,3"));
		}

		[Fact]
		public void A_descending_span_is_a_usage_error()
		{
			// "5-3" is far more likely to be a typo than a request to reverse the pages,
			// so it is rejected rather than silently normalised.
			Assert.Throws<UsageError>(() => PageRange.Parse("5-3"));
		}

		[Fact]
		public void Canonical_text_form_is_stable_for_the_conformance_corpus()
		{
			// The corpus compares plans as text, so two equivalent selections must
			// render identically in all four language implementations.
			Assert.Equal(PageRange.Parse("1,2,3").ToString(), PageRange.Parse("1-3").ToString());
		}

		[Fact]
		public void Overlapping_and_touching_spans_merge()
		{
			PageRange range = PageRange.Parse("3,1,1-2,5-7,6-9");

			Assert.Equal("1-3,5-9", range.ToString());
			Assert.Equal(1, range.FirstPage);
		}

		[Fact]
		public void The_widest_span_costs_nothing_until_it_is_walked()
		{
			// Every page used to be expanded into a list while the arguments were read,
			// so this valid value allocated two billion entries before anything ran.
			PageRange range = PageRange.Parse("1-2147483647");

			Assert.Equal("1-2147483647", range.ToString());
			Assert.Equal(1, range.FirstPage);
			Assert.True(range.Contains(2147483647));
			Assert.False(range.Contains(0));
			Assert.Equal(new[] { 1, 2, 3 }, range.Enumerate(3).Take(3).ToArray());
			Assert.Equal(new[] { 1, 2, 3 }, CanvasConversion.RenderablePages(range, 3).ToArray());
		}

		[Fact]
		public void A_span_ending_at_the_last_int_does_not_wrap()
		{
			Assert.Equal(new[] { 2147483646, 2147483647 }, PageRange.Parse("2147483646-2147483647").Enumerate(1).ToArray());
		}
	}
}
