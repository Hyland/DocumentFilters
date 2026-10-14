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
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Discovery;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The output model. This is the highest-risk logic in the tool and the reason
	/// planning is a separate, pure stage.
	/// </summary>
	/// <remarks>
	/// The rule underneath every test here: the meaning of <c>-o</c> never depends on
	/// <c>--to</c>.
	/// </remarks>
	public class OutputPlannerTests
	{
		/// <summary>
		/// <see cref="OutputPlanner.Plan"/> against a filesystem that tells names apart
		/// by case, which is what these cases assume unless one says otherwise.
		/// </summary>
		private static RunPlan Plan(SharedOptions shared, ConvertOptions convert,
								    IReadOnlyList<InputSource> inputs, bool caseSensitive = true)
		{
			return OutputPlanner.Plan(shared, convert, inputs,
									 new InMemoryFileSystem { CaseSensitive = caseSensitive });
		}

		private static IReadOnlyList<InputSource> Inputs(params string[] paths)
		{
			var list = new List<InputSource>();
			foreach (string path in paths)
			{
				int slash = path.LastIndexOf('/');
				list.Add(new InputSource
				{
					Path = path,
					RelativePath = slash < 0 ? path : path.Substring(slash + 1),
					Root = null
				});
			}

			return list;
		}

		private static ConvertOptions ToFormat(string token, bool combine = false)
		{
			var options = new ConvertOptions { Combine = combine };
			options.Format = FormatCatalog.Resolve(token);
			return options;
		}

		// ---------------------------------------------------------------- cardinality

		[Fact]
		public void Out_dir_with_many_inputs_gives_each_its_own_output()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			RunPlan plan = Plan(shared, ToFormat("pdf"), Inputs("a.docx", "b.docx"));

			Assert.Equal(OutputCardinality.PerInput, plan.Cardinality);
			Assert.Equal(2, plan.Items.Count);
			Assert.Equal("out/a.pdf", plan.Items[0].Target);
			Assert.Equal("out/b.pdf", plan.Items[1].Target);
		}

		[Fact]
		public void Out_file_with_one_input_is_a_single_target()
		{
			var shared = new SharedOptions();
			shared.Output.OutFile = "book.pdf";

			RunPlan plan = Plan(shared, ToFormat("pdf"), Inputs("a.docx"));

			Assert.Equal(OutputCardinality.SingleTarget, plan.Cardinality);
			Assert.Equal("book.pdf", plan.Items[0].Target);
		}

		[Fact]
		public void Combine_with_out_file_merges_every_input_into_one_document()
		{
			var shared = new SharedOptions();
			shared.Output.OutFile = "book.pdf";

			RunPlan plan = Plan(shared, ToFormat("pdf", combine: true), Inputs("a.docx", "b.docx", "c.docx"));

			Assert.Equal(OutputCardinality.Combined, plan.Cardinality);
			Assert.Equal("book.pdf", plan.CombinedTarget);
			Assert.Equal(3, plan.Items.Count);

			// Items contribute to the shared target rather than owning one.
			Assert.All(plan.Items, item => Assert.Null(item.Target));
		}

		[Fact]
		public void Stdout_is_its_own_cardinality()
		{
			var shared = new SharedOptions();
			shared.Output.UseStdout = true;

			RunPlan plan = Plan(shared, ToFormat("pdf"), Inputs("a.docx"));

			Assert.Equal(OutputCardinality.Stream, plan.Cardinality);
		}

		// ------------------------------------------------- collisions and letter case

		/// <summary>
		/// Two names differing only in case are one file where the filesystem says so,
		/// and the planner must refuse rather than let the second output replace the
		/// first.
		/// </summary>
		/// <remarks>
		/// This was silent data loss: the run exited 0, printed nothing, and left one
		/// file holding the second document's text. Nothing caught it because the suite
		/// compared planned paths ordinally, which is right on the filesystem the tests
		/// ran against and wrong on the one most users have.
		///
		/// The corpus cannot hold this case — it states the other answer, since its
		/// filesystem is case-sensitive and a shared corpus has to read the same
		/// everywhere. This is that half, and it is a unit test in each language rather
		/// than a contract case for the same reason.
		/// </remarks>
		[Fact]
		public void Names_differing_only_in_case_collide_where_the_filesystem_folds_case()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			OutputConflictError error = Assert.Throws<OutputConflictError>(
				() => Plan(shared, ToFormat("text"), Inputs("a/Report.docx", "b/report.docx"),
						   caseSensitive: false));

			Assert.Equal(ErrorCodes.OutputCollision, error.Code);
			Assert.Equal(ExitCode.OutputConflict, error.ExitCode);
		}

		/// <summary>
		/// The same pair is two outputs where the filesystem tells them apart, and
		/// refusing it would decline work the tool can do.
		/// </summary>
		[Fact]
		public void Names_differing_only_in_case_are_two_outputs_where_the_filesystem_is_case_sensitive()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			RunPlan plan = Plan(shared, ToFormat("text"), Inputs("a/Report.docx", "b/report.docx"),
								caseSensitive: true);

			Assert.Equal(new[] { "out/Report.txt", "out/report.txt" },
						 plan.Items.Select(item => item.Target).ToArray());
		}

		/// <summary>An exact repeat collides whatever the filesystem says about case.</summary>
		[Theory]
		[InlineData(true)]
		[InlineData(false)]
		public void Two_inputs_with_the_same_name_always_collide(bool caseSensitive)
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			OutputConflictError error = Assert.Throws<OutputConflictError>(
				() => Plan(shared, ToFormat("text"), Inputs("a/report.docx", "b/report.docx"),
						   caseSensitive: caseSensitive));

			Assert.Equal(ErrorCodes.OutputCollision, error.Code);
		}

		// ------------------------------------------------------------- the hard error

		[Fact]
		public void Out_file_with_several_inputs_and_no_combine_is_a_hard_error()
		{
			// The specification is explicit that this is an error and never a guess:
			// the tool must not silently pick one input or overwrite repeatedly.
			var shared = new SharedOptions();
			shared.Output.OutFile = "one.pdf";

			OutputConflictError error = Assert.Throws<OutputConflictError>(
				() => Plan(shared, ToFormat("pdf"), Inputs("a.docx", "b.docx")));

			Assert.Equal(ErrorCodes.OutFileMultiInput, error.Code);
			Assert.Equal(ExitCode.OutputConflict, error.ExitCode);
		}

		[Fact]
		public void No_inputs_is_its_own_error_and_exit_code()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			NoInputsError error = Assert.Throws<NoInputsError>(
				() => Plan(shared, ToFormat("pdf"), Inputs()));

			Assert.Equal(ExitCode.NoInputsMatched, error.ExitCode);
		}

		// ------------------------------------------------- page fan-out is a separate axis

		[Fact]
		public void A_one_per_page_format_is_still_per_input()
		{
			// One document producing five PNGs is not a different cardinality. Target
			// ownership and page count are independent questions.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			RunPlan plan = Plan(shared, ToFormat("png"), Inputs("a.docx", "b.docx"));

			Assert.Equal(OutputCardinality.PerInput, plan.Cardinality);
		}

		[Fact]
		public void A_one_per_page_format_without_pages_defers_its_targets()
		{
			// The filenames depend on the document's page count, which is unknown until
			// it is opened, so the planner records a pattern instead of paths.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			RunPlan plan = Plan(shared, ToFormat("png"), Inputs("a.docx"));

			Assert.True(plan.Items[0].IsPageCountDependent);
			Assert.Null(plan.Items[0].Target);
			Assert.NotNull(plan.Items[0].TargetPattern);
		}

		[Fact]
		public void A_one_per_page_format_defers_its_targets_even_with_explicit_pages()
		{
			// --pages does not make the filenames knowable: the range is clamped to the
			// document's real page count, so --pages 1-2 on a one-page document writes
			// one file. Resolving both up front would let the planner reject a run
			// because a page-2 output already exists when page 2 is never written.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			ConvertOptions options = ToFormat("png");
			options.Pages = PageRange.Parse("1-2");

			RunPlan plan = Plan(shared, options, Inputs("a.docx"));

			Assert.True(plan.Items[0].IsPageCountDependent);
			Assert.Null(plan.Items[0].Target);
			Assert.NotNull(plan.Items[0].TargetPattern);
		}

		[Fact]
		public void The_first_checked_target_is_the_first_selected_page_not_page_one()
		{
			// With --pages 5-6 the first output is page 5, so that is what the
			// fail-fast existence check has to look at.
			var pattern = new TargetPattern
			{
				Directory = "out",
				Template = OutputNameTemplate.Parse("{stem}-{page:04}.{ext}"),
				Extension = "png"
			};

			Assert.Equal("out/a-0005.png", pattern.FirstTarget("a.docx", PageRange.Parse("5-6")));
			Assert.Equal("out/a-0001.png", pattern.FirstTarget("a.docx", PageRange.All));
		}

		[Fact]
		public void A_single_file_format_resolves_its_target_even_without_pages()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			RunPlan plan = Plan(shared, ToFormat("pdf"), Inputs("a.docx"));

			Assert.False(plan.Items[0].IsPageCountDependent);
		}

		// ------------------------------------------------------------------ collisions

		[Fact]
		public void Two_inputs_resolving_to_the_same_output_is_an_error()
		{
			// A flattened walk of a/report.docx and b/report.docx would silently
			// overwrite. Only visible because the whole batch is planned at once.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			OutputConflictError error = Assert.Throws<OutputConflictError>(
				() => Plan(shared, ToFormat("pdf"), Inputs("a/report.docx", "b/report.docx")));

			Assert.Equal(ErrorCodes.OutputCollision, error.Code);
		}

		[Fact]
		public void Collisions_are_detected_for_deferred_targets_too()
		{
			// Page count is unknown for PNG, but the stems already collide, so this is
			// decidable from the pattern alone.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			Assert.Throws<OutputConflictError>(
				() => Plan(shared, ToFormat("png"), Inputs("a/report.docx", "b/report.docx")));
		}

		[Fact]
		public void Same_stem_with_different_source_extensions_still_collides()
		{
			// report.docx and report.pdf both become report.txt.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			Assert.Throws<OutputConflictError>(
				() => Plan(shared, ToFormat("text"), Inputs("report.docx", "report.pdf")));
		}

		[Fact]
		public void An_out_name_template_can_resolve_a_collision()
		{
			// {srcext} distinguishes the two, so this must not error.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			shared.Output.NameTemplate = OutputNameTemplate.Parse("{stem}-{srcext}.{ext}");

			RunPlan plan = Plan(shared, ToFormat("text"), Inputs("report.docx", "report.pdf"));

			Assert.Equal("out/report-docx.txt", plan.Items[0].Target);
			Assert.Equal("out/report-pdf.txt", plan.Items[1].Target);
		}

		// -------------------------------------------------------------- existing files

		[Fact]
		public void An_existing_target_without_overwrite_is_an_error()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			var fs = new InMemoryFileSystem().AddFile("out/a.pdf", "old");

			RunPlan plan = Plan(shared, ToFormat("pdf"), Inputs("a.docx"));

			OutputConflictError error = Assert.Throws<OutputConflictError>(
				() => OutputPlanner.VerifyTargets(plan, shared, fs));

			Assert.Equal(ErrorCodes.OutputExists, error.Code);
		}

		[Fact]
		public void Overwrite_permits_an_existing_target()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			shared.Output.Overwrite = true;
			var fs = new InMemoryFileSystem().AddFile("out/a.pdf", "old");

			RunPlan plan = Plan(shared, ToFormat("pdf"), Inputs("a.docx"));

			OutputPlanner.VerifyTargets(plan, shared, fs);
		}

		[Fact]
		public void Verification_checks_the_first_page_of_a_deferred_item()
		{
			// Fail fast on the common case: re-running a conversion into a directory
			// that already holds its output.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			var fs = new InMemoryFileSystem().AddFile("out/a-0001.png", "old");

			RunPlan plan = Plan(shared, ToFormat("png"), Inputs("a.docx"));

			Assert.Throws<OutputConflictError>(() => OutputPlanner.VerifyTargets(plan, shared, fs));
		}

		[Fact]
		public void Verification_does_not_check_later_pages_of_a_deferred_item()
		{
			// Page 2 may never be written, so its presence must not fail the run here.
			// Execution checks each page as it produces it.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			var fs = new InMemoryFileSystem().AddFile("out/a-0002.png", "old");

			RunPlan plan = Plan(shared, ToFormat("png"), Inputs("a.docx"));

			OutputPlanner.VerifyTargets(plan, shared, fs);
		}

		// --------------------------------------------------------------- tree layout

		[Fact]
		public void The_source_tree_is_replicated_under_out_dir()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			var inputs = new List<InputSource>
			{
				new InputSource { Path = "corpus/reports/q1.docx", RelativePath = "reports/q1.docx", Root = "corpus" }
			};

			RunPlan plan = Plan(shared, ToFormat("pdf"), inputs);

			Assert.Equal("out/reports/q1.pdf", plan.Items[0].Target);
		}
	}
}
