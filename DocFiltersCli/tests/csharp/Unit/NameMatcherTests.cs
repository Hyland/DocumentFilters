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

using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// <c>-g/--glob</c> and <c>--regex</c>. Glob is the default idiom; regex is the
	/// power mode for patterns a glob cannot express.
	/// </summary>
	public class NameMatcherTests
	{
		private static NameMatcher Glob(params string[] patterns) => NameMatcher.FromGlobs(patterns);

		private static NameMatcher Regex(params string[] patterns) => NameMatcher.FromRegexes(patterns);

		[Fact]
		public void A_star_matches_within_one_path_segment()
		{
			NameMatcher matcher = Glob("*.pdf");

			Assert.True(matcher.Matches("report.pdf"));
			Assert.False(matcher.Matches("reports/report.pdf"));
		}

		[Fact]
		public void A_double_star_crosses_path_segments()
		{
			NameMatcher matcher = Glob("**/*.pdf");

			Assert.True(matcher.Matches("a/b/c/report.pdf"));
			Assert.False(matcher.Matches("a/b/c/report.docx"));
		}

		[Fact]
		public void A_question_mark_matches_exactly_one_character()
		{
			NameMatcher matcher = Glob("report?.pdf");

			Assert.True(matcher.Matches("report1.pdf"));
			Assert.False(matcher.Matches("report.pdf"));
			Assert.False(matcher.Matches("report12.pdf"));
		}

		[Fact]
		public void A_leading_bang_excludes()
		{
			// The specification names this as the default idiom for exclusion.
			NameMatcher matcher = Glob("**/*.pdf", "!**/draft/*");

			Assert.True(matcher.Matches("final/report.pdf"));
			Assert.False(matcher.Matches("draft/report.pdf"));
		}

		[Fact]
		public void Exclusions_alone_keep_everything_else()
		{
			NameMatcher matcher = Glob("!**/*.tmp");

			Assert.True(matcher.Matches("report.pdf"));
			Assert.False(matcher.Matches("scratch/a.tmp"));
		}

		[Fact]
		public void An_entry_matching_several_includes_is_kept_once()
		{
			NameMatcher matcher = Glob("**/*.pdf", "**/report.*");

			Assert.True(matcher.Matches("report.pdf"));
		}

		[Fact]
		public void An_exclusion_beats_an_inclusion()
		{
			NameMatcher matcher = Glob("**/*.pdf", "!**/secret.pdf");

			Assert.False(matcher.Matches("a/secret.pdf"));
		}

		[Fact]
		public void Backslashes_are_treated_as_separators_so_patterns_are_portable()
		{
			// A pattern written with forward slashes must work against Windows paths,
			// otherwise every test and every corpus case is platform specific.
			NameMatcher matcher = Glob("**/*.pdf");

			Assert.True(matcher.Matches(@"a\b\report.pdf"));
		}

		[Fact]
		public void Regex_mode_matches_the_whole_path_not_a_substring()
		{
			// Deliberately unanchored. An anchored pattern such as ".*\.pdf$" would
			// pass under substring semantics too, so it could not tell whole-match from
			// search — and the four languages differ exactly there: Python has match,
			// search and fullmatch; .NET's IsMatch searches; Java's matches() does not.
			NameMatcher matcher = Regex(@"report\.pdf");

			Assert.True(matcher.Matches("report.pdf"));
			Assert.False(matcher.Matches("a/report.pdf"));
			Assert.False(matcher.Matches("old-report.pdf"));
			Assert.False(matcher.Matches("report.pdf.bak"));
		}

		[Fact]
		public void A_regex_can_still_match_a_full_path_when_written_to()
		{
			NameMatcher matcher = Regex(@"a/b/report\.pdf");

			Assert.True(matcher.Matches("a/b/report.pdf"));
			Assert.False(matcher.Matches("x/a/b/report.pdf"));
		}

		[Fact]
		public void Regex_mode_expresses_what_a_glob_cannot()
		{
			// Alternation with a bounded repeat: the reason --regex exists at all.
			NameMatcher matcher = Regex(@"^report-\d{4}\.(pdf|docx)$");

			Assert.True(matcher.Matches("report-2026.pdf"));
			Assert.True(matcher.Matches("report-2026.docx"));
			Assert.False(matcher.Matches("report-26.pdf"));
		}

		[Fact]
		public void An_invalid_regex_is_a_usage_error()
		{
			UsageError error = Assert.Throws<UsageError>(() => Regex("[unclosed"));

			Assert.Equal(ExitCode.UsageError, error.ExitCode);
		}

		[Fact]
		public void Canonical_text_form_is_stable_for_the_conformance_corpus()
		{
			Assert.Equal(Glob("**/*.pdf").ToString(), Glob("**/*.pdf").ToString());
		}
	}
}
