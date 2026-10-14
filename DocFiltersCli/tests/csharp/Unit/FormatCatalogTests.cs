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

using System.Collections.Generic;
using System.Linq;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Conformance;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The <c>--to</c> token table, checked against the shared
	/// <c>formats.txt</c> fixture.
	/// </summary>
	/// <remarks>
	/// The table is data, and the Python, Java and C++17 implementations need
	/// exactly the same data, so it lives in the fixture rather than being
	/// transcribed into four test suites. What stays here is the behaviour the
	/// fixture cannot express: catalogue-wide invariants, and how lookup failures
	/// are reported.
	/// </remarks>
	public class FormatCatalogTests
	{
		public static IEnumerable<object[]> Cases()
		{
			return Fixture.Load("formats.txt").Select(c => new object[] { c });
		}

		[Theory]
		[MemberData(nameof(Cases))]
		public void Token_matches_the_shared_catalogue(FixtureCase testCase)
		{
			string token = testCase.Subject;

			if (testCase.Error != null)
			{
				UsageError error = Assert.Throws<UsageError>(() => FormatCatalog.Resolve(token));

				Assert.True(
					error.Code == testCase.Error,
					testCase.Describe($"expected error {testCase.Error}, got {error.Code}"));
				return;
			}

			FormatSpec spec = FormatCatalog.Resolve(token);

			Assert.True(spec != null, testCase.Describe("token did not resolve"));

			// The canonical token is the folded one: the fixture states PDF and Tiff as
			// their own cases, and both must land on the lower-case entry.
			Assert.True(
				spec.Token == Fold(token),
				testCase.Describe($"canonical token was '{spec.Token}', expected '{Fold(token)}'"));

			FormatKind expectedKind = ParseKind(testCase);
			Assert.True(spec.Kind == expectedKind, testCase.Describe($"kind was {spec.Kind}, expected {expectedKind}"));

			bool singleFile = testCase.GetBool("single-file");
			Assert.True(
				spec.IsSingleFile == singleFile,
				testCase.Describe($"single-file was {Yes(spec.IsSingleFile)}, expected {Yes(singleFile)}"));

			bool combine = testCase.GetBool("combine");
			Assert.True(
				spec.SupportsCombine == combine,
				testCase.Describe($"combine was {Yes(spec.SupportsCombine)}, expected {Yes(combine)}"));

			bool hidef = testCase.GetBool("hidef");
			Assert.True(
				spec.IsHiDefCapable == hidef,
				testCase.Describe($"hidef was {Yes(spec.IsHiDefCapable)}, expected {Yes(hidef)}"));

			string ext = testCase.Get("ext");
			Assert.True(spec.Extension == ext, testCase.Describe($"ext was '{spec.Extension}', expected '{ext}'"));

			// 'none' rather than an empty value, so the row is visible in the fixture.
			string options = testCase.Get("options") == "none" ? string.Empty : testCase.Get("options");
			Assert.True(
				(spec.CanvasOptions ?? string.Empty) == options,
				testCase.Describe($"options were '{spec.CanvasOptions}', expected '{options}'"));
		}

		/// <summary>The ASCII-only fold the catalogue matches tokens with.</summary>
		private static string Fold(string token)
		{
			var folded = new System.Text.StringBuilder(token.Length);

			foreach (char c in token)
				folded.Append(c >= 'A' && c <= 'Z' ? (char)(c + 32) : c);

			return folded.ToString();
		}

		private static FormatKind ParseKind(FixtureCase testCase)
		{
			switch (testCase.Get("kind"))
			{
				case "text": return FormatKind.Text;
				case "canvas": return FormatKind.Canvas;
				case "docpack": return FormatKind.DocPack;
				default:
					throw new System.IO.InvalidDataException(testCase.Describe("unknown 'kind'"));
			}
		}

		private static string Yes(bool value) => value ? "yes" : "no";

		// ------------------------------------------- behaviour the fixture cannot state

		[Fact]
		public void The_catalogue_has_an_entry_for_every_resolvable_token()
		{
			// Guards against a token that resolves but is missing from All(), which would
			// leave it out of the help text and the formats listing.
			foreach (FixtureCase testCase in Fixture.Load("formats.txt").Where(c => c.Error == null))
			{
				Assert.True(
					FormatCatalog.All().Any(f => f.Token == Fold(testCase.Subject)),
					testCase.Describe("resolves, but is absent from FormatCatalog.All()"));
			}
		}

		[Fact]
		public void The_catalogue_declares_nothing_the_fixture_does_not_list()
		{
			// The reverse direction: an implementation must not quietly support a format
			// the other three have never heard of.
			var expected = new HashSet<string>(
				Fixture.Load("formats.txt").Where(c => c.Error == null).Select(c => c.Subject));

			foreach (FormatSpec spec in FormatCatalog.All())
				Assert.True(expected.Contains(spec.Token), $"'{spec.Token}' is in the catalogue but not in formats.txt");
		}

		[Fact]
		public void Every_catalogue_entry_has_a_unique_token()
		{
			var seen = new HashSet<string>();

			foreach (FormatSpec spec in FormatCatalog.All())
				Assert.True(seen.Add(spec.Token), "duplicate token: " + spec.Token);
		}

		[Fact]
		public void Every_catalogue_entry_has_a_display_name_for_help_text()
		{
			// Not in the fixture because it is prose, and prose is allowed to differ
			// between languages in a way that ids and extensions are not.
			foreach (FormatSpec spec in FormatCatalog.All())
				Assert.False(string.IsNullOrEmpty(spec.DisplayName), spec.Token + " has no display name");
		}

		[Fact]
		public void Tokens_are_matched_case_insensitively_but_canonicalise_to_lower_case()
		{
			// Users type --to PDF; the plan dump must still say pdf, or the corpus
			// comparison becomes case sensitive by accident.
			Assert.Equal("pdf", FormatCatalog.Resolve("PDF").Token);
			Assert.Equal("md-text", FormatCatalog.Resolve("MD-Text").Token);
		}

		[Fact]
		public void The_error_names_the_token_and_where_to_find_the_valid_ones()
		{
			// The message points at the formats command rather than listing the
			// catalogue inline. Two dozen tokens in an error string would have to stay
			// byte-identical across four languages and be edited every time the
			// catalogue grows; a pointer does not.
			UsageError error = Assert.Throws<UsageError>(() => FormatCatalog.Resolve("pdxf"));

			Assert.Contains("pdxf", error.Message);
			Assert.Contains("formats", error.Message);
		}

		[Fact]
		public void An_unknown_token_exits_with_the_usage_code()
		{
			Assert.Equal(ExitCode.UsageError, Assert.Throws<UsageError>(() => FormatCatalog.Resolve("pdxf")).ExitCode);
		}

		[Fact]
		public void TryResolve_returns_null_rather_than_throwing()
		{
			Assert.Null(FormatCatalog.TryResolve("nonsense"));
			Assert.NotNull(FormatCatalog.TryResolve("pdf"));
		}

		[Fact]
		public void A_text_mode_format_is_never_high_definition_capable()
		{
			// A structural invariant rather than a per-row fact: if these ever
			// disagreed, --fallback would have nothing coherent to fall back to.
			foreach (FormatSpec spec in FormatCatalog.All().Where(f => f.Kind == FormatKind.Text))
				Assert.False(spec.IsHiDefCapable, spec.Token + " is text mode but claims hi-def capability");
		}
	}
}
