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

using System;
using System.Collections.Generic;
using System.Linq;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Conformance;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// <c>--out-name</c>, checked against the shared <c>out-names.txt</c> fixture.
	/// </summary>
	/// <remarks>
	/// The render cases are tabular — template, source, extension, page, expected
	/// filename — so they live in the fixture and all four implementations run
	/// them. What stays here is behaviour a row cannot express: the defaults, and
	/// the guarantee that a rendered name is a filename and never a path.
	/// </remarks>
	public class OutputNameTemplateTests
	{
		public static IEnumerable<object[]> Cases()
		{
			return Fixture.Load("out-names.txt").Select(c => new object[] { c });
		}

		[Theory]
		[MemberData(nameof(Cases))]
		public void Template_matches_the_shared_fixture(FixtureCase testCase)
		{
			if (testCase.Error != null)
			{
				UsageError error = Assert.Throws<UsageError>(() => OutputNameTemplate.Parse(testCase.Subject));

				Assert.True(
					error.Code == testCase.Error,
					testCase.Describe($"expected error {testCase.Error}, got {error.Code}"));
				return;
			}

			OutputNameTemplate template = OutputNameTemplate.Parse(testCase.Subject);

			if (testCase.Has("uses-page"))
			{
				bool expected = testCase.GetBool("uses-page");
				Assert.True(
					template.UsesPage == expected,
					testCase.Describe($"uses-page was {(template.UsesPage ? "yes" : "no")}, expected {(expected ? "yes" : "no")}"));
			}

			foreach (string row in testCase.GetAll("render"))
			{
				RenderRow expectation = RenderRow.Parse(row, testCase);
				string actual = template.Render(expectation.Source, expectation.Extension, expectation.Page);

				Assert.True(
					actual == expectation.Expected,
					testCase.Describe(
						$"render {expectation.Source} {expectation.Extension} {expectation.Page}" +
						$" produced '{actual}', expected '{expectation.Expected}'"));
			}
		}

		/// <summary>One <c>= render source ext page =&gt; expected</c> row.</summary>
		private sealed class RenderRow
		{
			public string Source { get; private set; }

			public string Extension { get; private set; }

			public int Page { get; private set; }

			public string Expected { get; private set; }

			public static RenderRow Parse(string row, FixtureCase testCase)
			{
				string[] halves = row.Split(new[] { "=>" }, StringSplitOptions.None);

				if (halves.Length != 2)
					throw new System.IO.InvalidDataException(testCase.Describe($"render row has no '=>': {row}"));

				string[] inputs = halves[0].Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);

				if (inputs.Length != 3)
					throw new System.IO.InvalidDataException(
						testCase.Describe($"render row needs 'source ext page => expected': {row}"));

				return new RenderRow
				{
					Source = inputs[0],
					Extension = inputs[1],
					Page = int.Parse(inputs[2]),
					Expected = halves[1].Trim()
				};
			}
		}

		// ------------------------------------------- behaviour the fixture cannot state

		[Fact]
		public void Default_for_a_single_file_format_has_no_page_placeholder()
		{
			OutputNameTemplate template = OutputNameTemplate.Default(isSingleFile: true);

			Assert.False(template.UsesPage);
			Assert.Equal("report.pdf", template.Render("report.docx", "pdf", 1));
		}

		[Fact]
		public void Default_for_a_one_per_page_format_includes_a_padded_page_number()
		{
			// Without this, every page of a PNG conversion would overwrite the last.
			OutputNameTemplate template = OutputNameTemplate.Default(isSingleFile: false);

			Assert.True(template.UsesPage);
			Assert.Equal("report-0001.png", template.Render("report.docx", "png", 1));
		}

		[Theory]
		[InlineData("../../etc/passwd")]
		[InlineData(@"..\..\windows\system32\config")]
		[InlineData("/absolute/path/report.docx")]
		public void Rendering_never_emits_a_path_separator(string source)
		{
			// The template names a file inside --out-dir. Whatever the input path looked
			// like, the result must not be able to escape that directory. This is an
			// assertion about the output rather than an expected value, so it cannot be
			// a fixture row.
			OutputNameTemplate template = OutputNameTemplate.Parse("{stem}.{ext}");

			string rendered = template.Render(source, "txt", 1);

			Assert.DoesNotContain("/", rendered);
			Assert.DoesNotContain("\\", rendered);
			Assert.DoesNotContain("..", rendered);
		}

		[Fact]
		public void Two_pages_of_the_same_document_never_render_to_the_same_name()
		{
			// The property that makes a one-per-page format safe at all.
			OutputNameTemplate template = OutputNameTemplate.Default(isSingleFile: false);

			var seen = new HashSet<string>();

			for (int page = 1; page <= 200; page++)
				Assert.True(seen.Add(template.Render("report.docx", "png", page)), "duplicate name at page " + page);
		}

		[Fact]
		public void The_original_text_is_recoverable_for_the_plan_dump()
		{
			const string text = "{stem}-{page:04}.{ext}";

			Assert.Equal(text, OutputNameTemplate.Parse(text).ToString());
		}
	}
}
