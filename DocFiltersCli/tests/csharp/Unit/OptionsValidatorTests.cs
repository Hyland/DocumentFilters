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

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The rules no single argument can enforce, because they are relations between
	/// arguments. Everything here is decidable from the command line alone — rules that
	/// need to know how many files exist live in the planner instead.
	/// </summary>
	public class OptionsValidatorTests
	{
		private static ConvertOptions ToFormat(string token)
		{
			return new ConvertOptions { Format = FormatCatalog.Resolve(token) };
		}

		[Fact]
		public void A_bare_default_configuration_is_valid()
		{
			OptionsValidator.Validate(new SharedOptions());
		}

		[Fact]
		public void Out_file_and_out_dir_together_are_a_usage_error()
		{
			var shared = new SharedOptions();
			shared.Output.OutFile = "book.pdf";
			shared.Output.OutDir = "out";

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Out_name_without_out_dir_is_a_usage_error()
		{
			// The template names a file inside a directory; without one it means nothing.
			var shared = new SharedOptions();
			shared.Output.NameTemplate = OutputNameTemplate.Parse("{stem}.{ext}");

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));

			Assert.Equal(ErrorCodes.OutNameNeedsOutDir, error.Code);
		}

		[Fact]
		public void Stdout_with_out_dir_is_a_usage_error()
		{
			var shared = new SharedOptions();
			shared.Output.UseStdout = true;
			shared.Output.OutDir = "out";

			Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));
		}

		[Fact]
		public void Stdout_with_a_file_target_is_a_usage_error()
		{
			// Output cannot go to a file and to stdout. Neither the out-file/out-dir
			// rule nor the stdout/out-dir rule covers this pair.
			var shared = new SharedOptions();
			shared.Output.UseStdout = true;
			shared.Output.OutFile = "book.pdf";

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Out_file_dash_is_not_a_conflict_because_it_is_how_stdout_is_spelled()
		{
			var shared = new SharedOptions();
			shared.Output.UseStdout = true;

			OptionsValidator.Validate(shared);
		}

		[Fact]
		public void The_log_aliases_disagreeing_is_a_usage_error()
		{
			// --log-level and --debug-log-level are one setting. Two arguments carrying
			// values into one field would silently drop whichever lost.
			var shared = new SharedOptions();
			shared.Diagnostics.LogLevel = "trace";
			shared.Diagnostics.DebugLogLevel = "info";

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void The_log_aliases_agreeing_is_merely_redundant()
		{
			var shared = new SharedOptions();
			shared.Diagnostics.LogLevel = "trace";
			shared.Diagnostics.DebugLogLevel = "trace";

			OptionsValidator.Validate(shared);

			Assert.Equal("trace", shared.Diagnostics.EffectiveLogLevel);
		}

		[Fact]
		public void Either_log_alias_alone_resolves()
		{
			var viaShort = new SharedOptions();
			viaShort.Diagnostics.LogLevel = "trace";
			Assert.Equal("trace", viaShort.Diagnostics.EffectiveLogLevel);

			var viaDebug = new SharedOptions();
			viaDebug.Diagnostics.DebugLogLevel = "trace";
			Assert.Equal("trace", viaDebug.Diagnostics.EffectiveLogLevel);

			Assert.Null(new SharedOptions().Diagnostics.EffectiveLogLevel);
		}

		[Fact]
		public void The_module_aliases_behave_the_same_way()
		{
			var conflicting = new SharedOptions();
			conflicting.Diagnostics.LogModules = "pdf";
			conflicting.Diagnostics.DebugLogModules = "ocr";

			Assert.Throws<UsageError>(() => OptionsValidator.Validate(conflicting));

			var agreeing = new SharedOptions();
			agreeing.Diagnostics.LogModules = "pdf";
			agreeing.Diagnostics.DebugLogModules = "pdf";

			OptionsValidator.Validate(agreeing);
			Assert.Equal("pdf", agreeing.Diagnostics.EffectiveLogModules);
		}

		[Fact]
		public void Either_pattern_language_alone_is_valid()
		{
			// Compiling the matcher moved to CommandLineApp.Bind; this only exercises
			// the exclusivity rule below, which is still Validate's job.
			var globs = new SharedOptions();
			globs.Input.GlobPatterns.Add("**/*.pdf");
			OptionsValidator.Validate(globs);

			var regexes = new SharedOptions();
			regexes.Input.RegexPatterns.Add(@"a/b\.pdf");
			OptionsValidator.Validate(regexes);
		}

		[Fact]
		public void Glob_and_regex_together_are_a_usage_error()
		{
			// Two ways of saying the same thing. Combining them silently would make the
			// selection depend on argument order, and neither language subsumes the
			// other, so there is no sensible merge.
			var shared = new SharedOptions();
			shared.Input.GlobPatterns.Add("**/*.pdf");
			shared.Input.RegexPatterns.Add(@".*\.docx$");

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Verbose_and_silent_together_are_a_usage_error()
		{
			var shared = new SharedOptions();
			shared.Console.Verbose = true;
			shared.Console.Silent = true;

			Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));
		}

		[Fact]
		public void No_recurse_with_an_explicit_deeper_max_depth_is_a_usage_error()
		{
			// --no-recurse means depth 1. Combined with --max-depth 5 the user has asked
			// for two different things.
			var shared = new SharedOptions();
			shared.Input.MaxDepth = 5;
			shared.Input.NoRecurseWasGiven = true;

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void No_recurse_with_max_depth_one_is_consistent()
		{
			// Redundant rather than contradictory: both ask for the same thing.
			var shared = new SharedOptions();
			shared.Input.MaxDepth = 1;
			shared.Input.NoRecurseWasGiven = true;

			OptionsValidator.Validate(shared);
		}

		[Fact]
		public void No_recurse_alone_is_fine()
		{
			var shared = new SharedOptions();
			shared.Input.NoRecurseWasGiven = true;

			OptionsValidator.Validate(shared);
			Assert.Equal(1, shared.Input.EffectiveMaxDepth);
		}

		[Fact]
		public void Max_depth_alone_is_fine()
		{
			var shared = new SharedOptions();
			shared.Input.MaxDepth = 5;

			OptionsValidator.Validate(shared);
			Assert.Equal(5, shared.Input.EffectiveMaxDepth);
		}

		[Fact]
		public void Neither_means_unlimited_depth()
		{
			Assert.Null(new SharedOptions().Input.EffectiveMaxDepth);
		}

		[Fact]
		public void Make_dirs_with_flat_is_a_usage_error()
		{
			// Same class of contradiction as the depth pair, and detectable for the
			// same reason: each argument records its own presence.
			var options = new ExtractOptions { MakeDirsWasGiven = true, FlatWasGiven = true };

			UsageError error = Assert.Throws<UsageError>(
				() => OptionsValidator.ValidateExtract(new SharedOptions(), options));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Either_directory_layout_argument_alone_is_fine()
		{
			OptionsValidator.ValidateExtract(new SharedOptions(), new ExtractOptions { MakeDirsWasGiven = true });
			OptionsValidator.ValidateExtract(new SharedOptions(), new ExtractOptions { FlatWasGiven = true });

			Assert.True(new ExtractOptions { MakeDirsWasGiven = true }.ReplicateTree);
			Assert.False(new ExtractOptions { FlatWasGiven = true }.ReplicateTree);
			Assert.True(new ExtractOptions().ReplicateTree);
		}

		[Fact]
		public void A_per_page_format_with_a_template_lacking_page_is_a_usage_error()
		{
			// Every page would resolve to the same path: without --overwrite the run
			// fails after page one, and with it only the last page survives.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			shared.Output.NameTemplate = OutputNameTemplate.Parse("{stem}.{ext}");

			UsageError error = Assert.Throws<UsageError>(
				() => OptionsValidator.ValidateConvert(shared, ToFormat("png")));

			Assert.Equal(ErrorCodes.TemplateNeedsPage, error.Code);
		}

		[Fact]
		public void A_per_page_format_with_a_page_bearing_template_is_valid()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			shared.Output.NameTemplate = OutputNameTemplate.Parse("{stem}-{page:04}.{ext}");

			OptionsValidator.ValidateConvert(shared, ToFormat("png"));
		}

		[Fact]
		public void A_single_file_format_needs_no_page_in_its_template()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			shared.Output.NameTemplate = OutputNameTemplate.Parse("{stem}.{ext}");

			OptionsValidator.ValidateConvert(shared, ToFormat("pdf"));
		}

		[Fact]
		public void A_per_page_format_with_no_template_is_valid_because_the_default_has_a_page()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			OptionsValidator.ValidateConvert(shared, ToFormat("png"));
		}

		[Fact]
		public void Fail_fast_with_an_explicit_on_error_skip_is_a_usage_error()
		{
			// The two arguments write the same setting and contradict each other. Rejected
			// rather than resolved by precedence, because none of the four parsing
			// libraries reliably records which option came first.
			var shared = new SharedOptions();
			shared.Batch.OnError = OnErrorBehaviour.Skip;
			shared.Batch.OnErrorWasExplicit = true;
			shared.Batch.FailFastWasGiven = true;

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Fail_fast_with_an_explicit_on_error_stop_is_consistent()
		{
			var shared = new SharedOptions();
			shared.Batch.OnError = OnErrorBehaviour.Stop;
			shared.Batch.OnErrorWasExplicit = true;
			shared.Batch.FailFastWasGiven = true;

			OptionsValidator.Validate(shared);
		}

		[Fact]
		public void Fail_fast_alone_is_fine()
		{
			var shared = new SharedOptions();
			shared.Batch.OnError = OnErrorBehaviour.Stop;
			shared.Batch.FailFastWasGiven = true;

			OptionsValidator.Validate(shared);
		}

		// -------------------------------------------------------------------- convert

		[Fact]
		public void Missing_to_is_a_usage_error()
		{
			// Nothing at the parser level enforces a required argument, so an
			// omitted --to leaves ConvertOptions.Format null; every other convert
			// rule reads it unconditionally, so this has to be caught first.
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";

			UsageError error = Assert.Throws<UsageError>(
				() => OptionsValidator.ValidateConvert(shared, new ConvertOptions()));

			Assert.Equal(ErrorCodes.MissingArgument, error.Code);
			Assert.Equal(ExitCode.UsageError, error.ExitCode);
		}

		[Fact]
		public void To_alone_is_valid()
		{
			OptionsValidator.ValidateConvert(new SharedOptions(), ToFormat("pdf"));
		}

		[Fact]
		public void Combine_with_a_one_per_page_format_is_a_usage_error()
		{
			// You cannot merge documents into a format that emits a file per page.
			var shared = new SharedOptions();
			shared.Output.OutFile = "out.png";
			ConvertOptions options = ToFormat("png");
			options.Combine = true;

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.ValidateConvert(shared, options));

			Assert.Equal(ErrorCodes.CombineUnsupported, error.Code);
		}

		[Fact]
		public void Combine_with_a_single_file_format_is_valid()
		{
			var shared = new SharedOptions();
			shared.Output.OutFile = "book.pdf";
			ConvertOptions options = ToFormat("pdf");
			options.Combine = true;

			OptionsValidator.ValidateConvert(shared, options);
		}

		[Fact]
		public void Combine_with_text_is_valid()
		{
			// Text is a single-file format, so several documents can be concatenated.
			var shared = new SharedOptions();
			shared.Output.OutFile = "all.txt";
			ConvertOptions options = ToFormat("text");
			options.Combine = true;

			OptionsValidator.ValidateConvert(shared, options);
		}

		[Fact]
		public void Stdout_with_a_one_per_page_format_is_a_usage_error()
		{
			var shared = new SharedOptions();
			shared.Output.UseStdout = true;

			UsageError error = Assert.Throws<UsageError>(
				() => OptionsValidator.ValidateConvert(shared, ToFormat("png")));

			Assert.Equal(ErrorCodes.StdoutNeedsSingleFile, error.Code);
		}

		[Fact]
		public void Stdout_with_a_single_file_format_is_valid()
		{
			var shared = new SharedOptions();
			shared.Output.UseStdout = true;

			OptionsValidator.ValidateConvert(shared, ToFormat("pdf"));
		}

		// ---------------------------------------------------------------- fingerprint

		[Fact]
		public void Fingerprint_with_stdout_is_a_usage_error()
		{
			// --fingerprint renders a page only to read a fingerprint attribute off
			// it and never keeps the render, the same way doc2text's own
			// --fingerprint discards its render through a temporary stream — so
			// streaming that now-discarded content is a contradiction, not a
			// silent no-op.
			var shared = new SharedOptions();
			shared.Output.UseStdout = true;
			ConvertOptions options = ToFormat("pdf");
			options.Fingerprint = true;

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.ValidateConvert(shared, options));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Fingerprint_with_view_is_a_usage_error()
		{
			var shared = new SharedOptions();
			ConvertOptions options = ToFormat("pdf");
			options.Fingerprint = true;
			options.View = true;

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.ValidateConvert(shared, options));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Fingerprint_with_combine_is_a_usage_error()
		{
			var shared = new SharedOptions();
			shared.Output.OutFile = "book.pdf";
			ConvertOptions options = ToFormat("pdf");
			options.Fingerprint = true;
			options.Combine = true;

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.ValidateConvert(shared, options));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Fingerprint_with_out_file_is_a_usage_error()
		{
			var shared = new SharedOptions();
			shared.Output.OutFile = "book.pdf";
			ConvertOptions options = ToFormat("pdf");
			options.Fingerprint = true;

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.ValidateConvert(shared, options));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Fingerprint_with_out_dir_is_a_usage_error()
		{
			var shared = new SharedOptions();
			shared.Output.OutDir = "out";
			ConvertOptions options = ToFormat("pdf");
			options.Fingerprint = true;

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.ValidateConvert(shared, options));

			Assert.Equal(ErrorCodes.MutuallyExclusive, error.Code);
		}

		[Fact]
		public void Fingerprint_with_no_output_target_at_all_is_valid()
		{
			// The common case: no --out-dir, --out-file or --stdout given, so the
			// planner's own default ('.') is never something the user named.
			OptionsValidator.ValidateConvert(new SharedOptions(), new ConvertOptions
			{
				Format = FormatCatalog.Resolve("pdf"),
				Fingerprint = true
			});
		}

		[Fact]
		public void Fingerprint_with_page_decorations_is_valid()
		{
			// --redact, --form and --watermark decorate the same throwaway render
			// fingerprinting already needs, so there is nothing for them to
			// contradict.
			ConvertOptions options = ToFormat("pdf");
			options.Fingerprint = true;
			options.Watermark = "stamp.png";
			options.Redactions.Add(new RedactSpec { Page = 1, X = 0, Y = 0, Width = 10, Height = 10 });
			options.FormValues.Add(new FormValue { Page = 1, Name = "field", Value = "value" });

			OptionsValidator.ValidateConvert(new SharedOptions(), options);
		}

		[Fact]
		public void Every_validation_failure_exits_with_the_usage_code()
		{
			var shared = new SharedOptions();
			shared.Output.OutFile = "a";
			shared.Output.OutDir = "b";

			UsageError error = Assert.Throws<UsageError>(() => OptionsValidator.Validate(shared));

			Assert.Equal(ExitCode.UsageError, error.ExitCode);
		}
	}
}
