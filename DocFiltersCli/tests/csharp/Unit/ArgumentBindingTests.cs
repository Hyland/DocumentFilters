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
using Hyland.DocumentFilters.Cli.Cli;
using Hyland.DocumentFilters.Cli.Commands;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// That each argument's setter writes to the field it is supposed to.
	/// </summary>
	/// <remarks>
	/// This is the one mapping in the project that is hand-written once per argument,
	/// roughly seventy-five times, which makes it the likeliest place for a
	/// copy-and-paste mistake. A setter pointed at the wrong field compiles, keeps
	/// its correct name and arity, and would pass every declaration test in
	/// <see cref="ArgumentTableTests"/> — so it needs checking directly.
	///
	/// <see cref="ArgumentTableTests"/> asks "does this argument exist?".
	/// This asks "does it do what it says?".
	/// </remarks>
	public class ArgumentBindingTests
	{
		private static BoundCommandLine Bind(params string[] args)
		{
			var app = new CommandLineApp(new RecordingConsole(), new InMemoryFileSystem());
			return app.Bind(args);
		}

		private static SharedOptions Shared(params string[] arguments)
		{
			// A minimal valid convert invocation, with the arguments under test spliced in.
			string[] args = new[] { "convert", "--to", "text", "--out-dir", "out" }
				.Concat(arguments)
				.Concat(new[] { "a.docx" })
				.ToArray();

			return Bind(args).SharedOptions;
		}

		private static ConvertOptions Convert(params string[] arguments)
		{
			string[] args = new[] { "convert", "--to", "text" }
				.Concat(arguments)
				.Concat(new[] { "--out-dir", "out", "a.docx" })
				.ToArray();

			return ((ConvertCommand)Bind(args).Command).Options;
		}

		private static ExtractOptions Extract(params string[] arguments)
		{
			string[] args = new[] { "extract" }
				.Concat(arguments)
				.Concat(new[] { "--out-dir", "out", "a.zip" })
				.ToArray();

			return ((ExtractCommand)Bind(args).Command).Options;
		}

		// ====================================================== shared: output

		[Fact]
		public void Out_dir_writes_to_output_out_dir()
		{
			Assert.Equal("out", Shared().Output.OutDir);
		}

		[Fact]
		public void Short_o_is_an_alias_for_out_dir_and_never_out_file()
		{
			// The old tool's -o guessed file-or-directory from the format. It must not.
			SharedOptions shared = Bind("convert", "--to", "text", "-o", "out", "a.docx").SharedOptions;

			Assert.Equal("out", shared.Output.OutDir);
			Assert.Null(shared.Output.OutFile);
		}

		[Fact]
		public void Out_file_writes_to_output_out_file_and_not_out_dir()
		{
			SharedOptions shared = Bind("convert", "--to", "text", "--out-file", "a.txt", "a.docx").SharedOptions;

			Assert.Equal("a.txt", shared.Output.OutFile);
			Assert.Null(shared.Output.OutDir);
		}

		[Fact]
		public void Out_name_compiles_to_a_template()
		{
			SharedOptions shared = Shared("--out-name", "{stem}-{page:04}.{ext}");

			Assert.NotNull(shared.Output.NameTemplate);
			Assert.True(shared.Output.NameTemplate.UsesPage);
		}

		[Fact]
		public void Stdout_sets_the_stdout_argument()
		{
			SharedOptions shared = Bind("convert", "--to", "text", "--stdout", "a.docx").SharedOptions;

			Assert.True(shared.Output.UseStdout);
		}

		[Fact]
		public void Out_file_dash_is_the_same_as_stdout()
		{
			// Two spellings, one setting. Nothing downstream should see the difference.
			SharedOptions shared = Bind("convert", "--to", "text", "--out-file", "-", "a.docx").SharedOptions;

			Assert.True(shared.Output.UseStdout);
		}

		[Fact]
		public void Overwrite_is_a_switch()
		{
			Assert.True(Shared("--overwrite").Output.Overwrite);
			Assert.False(Shared().Output.Overwrite);
		}

		// ====================================================== shared: walk

		[Fact]
		public void Positional_paths_land_in_input_paths_in_order()
		{
			SharedOptions shared = Bind("convert", "--to", "text", "--out-dir", "out", "a.docx", "b.docx").SharedOptions;

			Assert.Equal(new[] { "a.docx", "b.docx" }, shared.Input.Paths);
		}

		[Fact]
		public void Files_from_writes_its_path()
		{
			Assert.Equal("list.txt", Shared("--files-from", "list.txt").Input.FilesFrom);
		}

		[Fact]
		public void Max_depth_parses_into_its_own_field()
		{
			SharedOptions shared = Shared("--max-depth", "3");

			Assert.Equal(3, shared.Input.MaxDepth);
			Assert.False(shared.Input.NoRecurseWasGiven);
			Assert.Equal(3, shared.Input.EffectiveMaxDepth);
		}

		[Fact]
		public void Max_depth_defaults_to_unlimited()
		{
			// Directories are walked recursively by default.
			SharedOptions shared = Shared();

			Assert.Null(shared.Input.MaxDepth);
			Assert.Null(shared.Input.EffectiveMaxDepth);
		}

		[Fact]
		public void No_recurse_does_not_write_the_depth_field()
		{
			// The two arguments must stay distinguishable: if --no-recurse wrote 1 into
			// MaxDepth, then "--max-depth 5 --no-recurse" would be indistinguishable
			// from a plain "--max-depth 1", and the contradiction could never be
			// reported. The resolved answer comes from EffectiveMaxDepth instead.
			SharedOptions shared = Shared("--no-recurse");

			Assert.True(shared.Input.NoRecurseWasGiven);
			Assert.Null(shared.Input.MaxDepth);
			Assert.Equal(1, shared.Input.EffectiveMaxDepth);
		}

		[Fact]
		public void The_contradictory_depth_pair_stays_distinguishable_after_binding()
		{
			// The state the validator has to be able to see. Binding must not collapse
			// it before validation runs.
			SharedOptions conflicting = Bind("convert", "--to", "text", "--out-dir", "out",
				"--max-depth", "5", "--no-recurse", "a.docx").SharedOptions;

			Assert.Equal(5, conflicting.Input.MaxDepth);
			Assert.True(conflicting.Input.NoRecurseWasGiven);
		}

		[Fact]
		public void Follow_uses_capital_L()
		{
			Assert.True(Shared("-L").Input.FollowSymlinks);
			Assert.True(Shared("--follow").Input.FollowSymlinks);
		}

		[Fact]
		public void Sort_and_include_hidden_are_switches()
		{
			Assert.True(Shared("--sort").Input.Sort);
			Assert.True(Shared("--include-hidden").Input.IncludeHidden);
			Assert.False(Shared().Input.Sort);
			Assert.False(Shared().Input.IncludeHidden);
		}

		[Fact]
		public void Glob_accumulates_and_builds_a_matcher()
		{
			SharedOptions shared = Shared("-g", "**/*.pdf");

			Assert.Equal(new[] { "**/*.pdf" }, shared.Input.GlobPatterns);
			Assert.Empty(shared.Input.RegexPatterns);
			Assert.NotNull(shared.Input.Matcher);
			Assert.True(shared.Input.Matcher.Matches("a/b.pdf"));
		}

		[Fact]
		public void Glob_is_repeatable_and_accumulates()
		{
			SharedOptions shared = Shared("-g", "**/*.pdf", "-g", "!**/draft/*");

			Assert.Equal(new[] { "**/*.pdf", "!**/draft/*" }, shared.Input.GlobPatterns);
			Assert.True(shared.Input.Matcher.Matches("final/a.pdf"));
			Assert.False(shared.Input.Matcher.Matches("draft/a.pdf"));
		}

		[Fact]
		public void Regex_accumulates_into_its_own_list()
		{
			// The two pattern languages must stay distinguishable after binding, or the
			// validator cannot tell that both were given.
			SharedOptions shared = Shared("--regex", @".*\.pdf$");

			Assert.Equal(new[] { @".*\.pdf$" }, shared.Input.RegexPatterns);
			Assert.Empty(shared.Input.GlobPatterns);
			Assert.NotNull(shared.Input.Matcher);
			Assert.True(shared.Input.Matcher.Matches("a/b.pdf"));
		}

		[Fact]
		public void Regex_is_repeatable_and_accumulates()
		{
			SharedOptions shared = Shared("--regex", @".*\.pdf$", "--regex", @".*\.docx$");

			Assert.Equal(2, shared.Input.RegexPatterns.Count);
		}

		[Fact]
		public void Neither_pattern_argument_leaves_the_matcher_unset()
		{
			SharedOptions shared = Shared();

			Assert.Empty(shared.Input.GlobPatterns);
			Assert.Empty(shared.Input.RegexPatterns);
			Assert.Null(shared.Input.Matcher);
		}

		[Fact]
		public void A_malformed_regex_is_a_usage_error_at_bind_time()
		{
			// Compiled in stage 2, the same as the subfile globs, so a typo is reported
			// before any directory walk or file work starts.
			Assert.Throws<UsageError>(() => Shared("--regex", "[unclosed"));
		}

		[Fact]
		public void Include_and_exclude_types_split_on_semicolons()
		{
			SharedOptions shared = Shared("--include-types", "PDF;DOCX", "--exclude-types", "ZIP");

			Assert.Equal(new[] { "PDF", "DOCX" }, shared.Input.IncludeTypes);
			Assert.Equal(new[] { "ZIP" }, shared.Input.ExcludeTypes);
		}

		// ================================================ shared: subfiles

		[Fact]
		public void Subfiles_short_and_long_forms_agree()
		{
			Assert.True(Shared("-s").Subfiles.Enabled);
			Assert.True(Shared("--subfiles").Subfiles.Enabled);
			Assert.False(Shared().Subfiles.Enabled);
		}

		[Fact]
		public void Subfile_depth_is_separate_from_directory_depth()
		{
			// The single most confusable pair of arguments in the tool.
			SharedOptions shared = Shared("--max-depth", "2", "--subfile-depth", "5");

			Assert.Equal(2, shared.Input.MaxDepth);
			Assert.Equal(5, shared.Subfiles.MaxDepth);
		}

		// ================================================ shared: passwords

		[Fact]
		public void Password_is_repeatable_and_accumulates_in_order()
		{
			SharedOptions shared = Shared("-p", "first", "-p", "second");

			Assert.Equal(new[] { "first", "second" }, shared.Passwords.Passwords);
		}

		[Fact]
		public void Password_file_and_prompt_write_their_own_fields()
		{
			SharedOptions shared = Shared("--password-file", "pw.txt", "--password-prompt");

			Assert.Equal("pw.txt", shared.Passwords.PasswordFile);
			Assert.True(shared.Passwords.Prompt);
		}

		// ================================================ shared: processing

		[Fact]
		public void Engine_options_are_stored_verbatim_and_not_parsed()
		{
			// The CLI does not interpret this string; the engine does.
			SharedOptions shared = Shared("--options", "BW_MODE=3;CODEPAGE=1252");

			Assert.Equal("BW_MODE=3;CODEPAGE=1252", shared.Processing.EngineOptions);
		}

		[Fact]
		public void Codepage_and_temp_dir_write_their_own_fields()
		{
			SharedOptions shared = Shared("--codepage", "1252", "--temp-dir", "/tmp/df");

			Assert.Equal("1252", shared.Processing.Codepage);
			Assert.Equal("/tmp/df", shared.Processing.TempDir);
		}

		// ================================================ shared: batch

		[Fact]
		public void On_error_defaults_to_skip()
		{
			SharedOptions shared = Shared();

			Assert.Equal(OnErrorBehaviour.Skip, shared.Batch.OnError);
			Assert.False(shared.Batch.OnErrorWasExplicit);
		}

		[Fact]
		public void On_error_stop_is_recorded_as_explicit()
		{
			SharedOptions shared = Shared("--on-error", "stop");

			Assert.Equal(OnErrorBehaviour.Stop, shared.Batch.OnError);
			Assert.True(shared.Batch.OnErrorWasExplicit);
		}

		[Fact]
		public void Fail_fast_writes_stop_through_the_same_field_and_flags_itself()
		{
			// Recorded separately so a contradictory pair can be rejected rather than
			// silently resolved.
			SharedOptions shared = Shared("--fail-fast");

			Assert.Equal(OnErrorBehaviour.Stop, shared.Batch.OnError);
			Assert.True(shared.Batch.FailFastWasGiven);
			Assert.False(shared.Batch.OnErrorWasExplicit);
		}

		// ================================================ shared: console, library

		[Fact]
		public void Console_switches_write_their_own_fields()
		{
			Assert.True(Shared("--color").Console.Color);
			Assert.True(Shared("--verbose").Console.Verbose);
			Assert.True(Shared("--silent").Console.Silent);
		}

		[Fact]
		public void License_key_and_library_have_no_short_forms()
		{
			SharedOptions shared = Shared("--license-key", "KEY", "--library", "/opt/df");

			Assert.Equal("KEY", shared.Library.LicenseKey);
			Assert.Equal("/opt/df", shared.Library.LibraryPath);
		}

		// ================================================ shared: diagnostics

		[Fact]
		public void Timing_switches_write_their_own_fields()
		{
			Assert.True(Shared("--time").Diagnostics.Time);
			Assert.True(Shared("--logtime").Diagnostics.LogTime);
			Assert.True(Shared("--logmem").Diagnostics.LogMemory);
			Assert.True(Shared("--progress").Diagnostics.Progress);
			Assert.True(Shared("--pause").Diagnostics.Pause);
		}

		[Fact]
		public void Limits_parse_as_numbers()
		{
			SharedOptions shared = Shared("--max-time", "60", "--timeout", "10", "--max-memory", "512");

			Assert.Equal(60, shared.Diagnostics.MaxTime);
			Assert.Equal(10, shared.Diagnostics.Timeout);
			Assert.Equal(512L, shared.Diagnostics.MaxMemory);
		}

		[Fact]
		public void Memory_status_levels_escalate()
		{
			// -x reports once at the end of the run, --xx after every document.
			Assert.Equal(0, Shared().Diagnostics.MemoryStatusLevel);
			Assert.Equal(1, Shared("-x").Diagnostics.MemoryStatusLevel);
			Assert.Equal(2, Shared("--xx").Diagnostics.MemoryStatusLevel);
		}

		[Fact]
		public void The_higher_memory_status_level_wins_whichever_order_they_are_given()
		{
			// A level, not two independent switches: --xx already includes what -x does,
			// so the pair is redundant rather than contradictory.
			Assert.Equal(2, Shared("-x", "--xx").Diagnostics.MemoryStatusLevel);
			Assert.Equal(2, Shared("--xx", "-x").Diagnostics.MemoryStatusLevel);
		}

		[Fact]
		public void Memory_status_and_logmem_are_separate_settings()
		{
			// --logmem samples this process's peak memory while working; -x/--xx ask the
			// engine what it has allocated.
			SharedOptions shared = Shared("--xx", "--logmem");

			Assert.Equal(2, shared.Diagnostics.MemoryStatusLevel);
			Assert.True(shared.Diagnostics.LogMemory);
		}

		[Fact]
		public void The_log_aliases_write_their_own_fields_and_resolve_to_one_value()
		{
			// Each alias keeps its own field so the validator can see that both were
			// given; EffectiveLogLevel is what everything downstream reads.
			SharedOptions viaShort = Shared("--log-level", "trace");
			Assert.Equal("trace", viaShort.Diagnostics.LogLevel);
			Assert.Null(viaShort.Diagnostics.DebugLogLevel);
			Assert.Equal("trace", viaShort.Diagnostics.EffectiveLogLevel);

			SharedOptions viaDebug = Shared("--debug-log-level", "trace");
			Assert.Null(viaDebug.Diagnostics.LogLevel);
			Assert.Equal("trace", viaDebug.Diagnostics.DebugLogLevel);
			Assert.Equal("trace", viaDebug.Diagnostics.EffectiveLogLevel);
		}

		[Fact]
		public void The_module_aliases_write_their_own_fields_too()
		{
			Assert.Equal("pdf", Shared("--log-modules", "pdf").Diagnostics.EffectiveLogModules);
			Assert.Equal("pdf", Shared("--debug-log-modules", "pdf").Diagnostics.EffectiveLogModules);
			Assert.Null(Shared("--log-modules", "pdf").Diagnostics.DebugLogModules);
		}

		[Fact]
		public void The_remaining_debug_switches_write_their_own_fields()
		{
			Assert.True(Shared("--debug-use-memory-stream").Diagnostics.UseMemoryStream);
			Assert.True(Shared("--debug-measure-io").Diagnostics.MeasureIo);
			Assert.Equal(4096L, Shared("--debug-truncate", "4096").Diagnostics.TruncateInputBytes);
		}

		[Fact]
		public void Dry_run_and_print_plan_write_the_same_field()
		{
			Assert.True(Shared("--dry-run").DryRun);
			Assert.True(Shared("--print-plan").DryRun);
			Assert.False(Shared().DryRun);
		}

		[Fact]
		public void Print_contract_writes_its_own_field()
		{
			Assert.True(Shared("--print-contract").PrintContract);
		}

		// ====================================================== convert

		[Fact]
		public void To_resolves_through_the_format_catalogue()
		{
			ConvertOptions options = Convert();

			Assert.NotNull(options.Format);
			Assert.Equal("text", options.Format.Token);
		}

		/// <summary>
		/// <c>--fallback</c> is gone, so it is an unknown argument like any other.
		/// </summary>
		/// <remarks>
		/// Kept as a test rather than deleted with the option: it was accepted for long
		/// enough to appear in scripts, and "unknown argument" is the answer those should
		/// get — not silent acceptance, and not a crash.
		/// </remarks>
		[Fact]
		public void Fallback_is_no_longer_an_argument()
		{
			UsageError error = Assert.Throws<UsageError>(() => Convert("--fallback", "text"));

			Assert.Equal(ErrorCodes.UnknownArgument, error.Code);
			Assert.Equal(ExitCode.UsageError, error.ExitCode);
		}

		[Fact]
		public void Combine_and_the_simple_switches_write_their_own_fields()
		{
			Assert.True(Convert("--combine").Combine);
			Assert.True(Convert("--no-images").NoImages);
			Assert.True(Convert("--include-control-codes").IncludeControlCodes);
			Assert.True(Convert("--fingerprint").Fingerprint);
			Assert.True(Convert("--view").View);
		}

		[Fact]
		public void Canvas_and_render_options_do_not_share_a_field()
		{
			ConvertOptions options = Convert("--canvas-options", "A=1", "--render-options", "B=2");

			Assert.Equal("A=1", options.CanvasOptions);
			Assert.Equal("B=2", options.RenderOptions);
		}

		[Fact]
		public void Pages_compiles_to_a_range()
		{
			ConvertOptions options = Convert("--pages", "1,3-5");

			Assert.Equal(new[] { 1, 3, 4, 5 }, options.Pages.Enumerate(10).ToArray());
		}

		[Fact]
		public void Watermark_writes_its_own_field()
		{
			Assert.Equal("DRAFT", Convert("--watermark", "DRAFT").Watermark);
		}

		[Fact]
		public void Redact_is_repeatable_and_each_occurrence_is_parsed()
		{
			ConvertOptions options = Convert("--redact", "2,10,20,100,50", "--redact", "3,0,0,50,50");

			Assert.Equal(2, options.Redactions.Count);
			Assert.Equal(2, options.Redactions[0].Page);
			Assert.Equal(100, options.Redactions[0].Width);
			Assert.Equal(3, options.Redactions[1].Page);
		}

		[Fact]
		public void Form_is_repeatable_and_each_occurrence_is_parsed()
		{
			ConvertOptions options = Convert("--form", "1,a=x", "--form", "2,b=y");

			Assert.Equal(2, options.FormValues.Count);
			Assert.Equal("a", options.FormValues[0].Name);
			Assert.Equal("y", options.FormValues[1].Value);
		}

		[Theory]
		[InlineData("--fonts")]
		[InlineData("--font-mappings")]
		[InlineData("--page-fonts")]
		[InlineData("--page-font-mappings")]
		[InlineData("--dimensions")]
		[InlineData("--page-elements")]
		[InlineData("--form-elements")]
		[InlineData("--hyperlinks")]
		[InlineData("--bookmarks")]
		[InlineData("--annotations")]
		public void Each_structure_report_argument_sets_exactly_one_report(string argument)
		{
			// Ten near-identical booleans: the highest copy-and-paste risk in the table.
			// Exactly one must flip, and Any must notice.
			ConvertOptions options = Convert(argument);

			Assert.True(options.Reports.Any);

			int enabled = new[]
			{
				options.Reports.Fonts, options.Reports.FontMappings,
				options.Reports.PageFonts, options.Reports.PageFontMappings,
				options.Reports.Dimensions, options.Reports.PageElements,
				options.Reports.FormElements, options.Reports.Hyperlinks,
				options.Reports.Bookmarks, options.Reports.Annotations
			}.Count(v => v);

			Assert.Equal(1, enabled);
		}

		[Fact]
		public void No_structure_reports_are_on_by_default()
		{
			Assert.False(Convert().Reports.Any);
		}

		// ====================================================== extract

		[Fact]
		public void Images_short_and_long_forms_agree()
		{
			Assert.True(Extract("-i").Images);
			Assert.True(Extract("--images").Images);
			Assert.False(Extract().Images);
		}

		[Fact]
		public void Subfile_name_filters_do_not_share_a_field()
		{
			ExtractOptions options = Extract("--include", "*invoice*", "--exclude", "*draft*");

			Assert.NotNull(options.SubfileInclude);
			Assert.NotNull(options.SubfileExclude);
			Assert.True(options.SubfileInclude.Matches("2026-invoice-01.pdf"));
			Assert.True(options.SubfileExclude.Matches("draft-notes.txt"));
		}

		[Fact]
		public void Subfile_filters_are_globs_compiled_during_binding()
		{
			// The same pattern language as -g/--glob, compiled in stage 2 so a malformed
			// pattern is a usage error before any archive is opened. A different axis
			// though: -g picks files on disk, this picks subfiles inside a container.
			ExtractOptions options = Extract("--include", "**/*.docx");

			Assert.True(options.SubfileInclude.Matches("reports/q1.docx"));
			Assert.False(options.SubfileInclude.Matches("reports/q1.pdf"));
		}

		[Fact]
		public void Subfile_filters_are_repeatable_like_the_path_globs()
		{
			// Repeatable for the same reason -g is: one glob per idea reads better than
			// one glob doing several jobs.
			ExtractOptions options = Extract("--include", "*.docx", "--include", "*.xlsx");

			Assert.Equal(new[] { "*.docx", "*.xlsx" }, options.IncludePatterns);
			Assert.True(options.SubfileInclude.Matches("q1.docx"));
			Assert.True(options.SubfileInclude.Matches("q1.xlsx"));
			Assert.False(options.SubfileInclude.Matches("q1.pdf"));
		}

		[Fact]
		public void Include_and_exclude_may_both_be_given()
		{
			// Unlike -g and --regex, which are two spellings of one idea and contradict,
			// these are complementary: keep these, then drop those.
			ExtractOptions options = Extract("--include", "*.docx", "--exclude", "draft-*");

			Assert.True(options.SubfileInclude.Matches("q1.docx"));
			Assert.True(options.SubfileExclude.Matches("draft-q1.docx"));
		}

		[Fact]
		public void A_malformed_subfile_glob_is_a_usage_error()
		{
			Assert.Throws<UsageError>(() => Extract("--include", "["));
		}

		[Fact]
		public void Extract_regex_selects_paths_like_it_does_everywhere_else()
		{
			// --regex means one thing everywhere: select paths by regular expression.
			// extract accepts it like any other verb that walks a directory.
			SharedOptions shared = Bind("extract", "--regex", @".*\.zip$", "-o", "out", "archives").SharedOptions;

			Assert.NotNull(shared.Input.Matcher);
			Assert.True(shared.Input.Matcher.Matches("a/b.zip"));
		}

		[Fact]
		public void Extract_has_no_way_to_make_the_subfile_filters_regular_expressions()
		{
			// The subfile filters are globs, with no mode switch to make them regular
			// expressions. --regex on extract selects paths, as it does everywhere.
			Assert.Throws<UsageError>(
				() => Bind("extract", "--include", "inv.*", "--content-regex", "-o", "out", "a.zip"));
		}

		[Fact]
		public void Make_dirs_is_the_default_and_flat_turns_it_off()
		{
			Assert.True(Extract().ReplicateTree);
			Assert.True(Extract("--make-dirs").ReplicateTree);
			Assert.False(Extract("--flat").ReplicateTree);
		}

		[Fact]
		public void The_two_layout_arguments_each_record_their_own_presence()
		{
			// So that --make-dirs --flat can be rejected rather than silently resolved,
			// for the same reason --max-depth and --no-recurse stay apart.
			Assert.True(Extract("--make-dirs").MakeDirsWasGiven);
			Assert.False(Extract("--make-dirs").FlatWasGiven);
			Assert.True(Extract("--flat").FlatWasGiven);
			Assert.False(Extract("--flat").MakeDirsWasGiven);
			Assert.False(Extract().MakeDirsWasGiven);
			Assert.False(Extract().FlatWasGiven);
		}

		[Fact]
		public void Sub_id_writes_its_own_field()
		{
			Assert.True(Extract("--sub-id").AppendSubfileId);
		}

		// ====================================================== other verbs

		[Fact]
		public void Json_binds_on_each_listing_command()
		{
			Assert.True(((ListCommand)Bind("list", "--json", "a.zip").Command).Options.Json);
			Assert.True(((IdentifyCommand)Bind("identify", "--json", "a.bin").Command).Options.Json);
			Assert.True(((MetaCommand)Bind("meta", "--json", "a.pdf").Command).Options.Json);
			Assert.True(((FormatsCommand)Bind("formats", "--json").Command).Options.Json);
			Assert.True(((OptionsCommand)Bind("options", "--json").Command).Options.Json);
		}

		[Fact]
		public void Identify_capabilities_binds()
		{
			Assert.True(((IdentifyCommand)Bind("identify", "--capabilities", "a.bin").Command).Options.Capabilities);
		}

		[Fact]
		public void Formats_filter_binds()
		{
			FormatsOptions options = ((FormatsCommand)Bind("formats", "--filter", "pdf").Command).Options;

			Assert.Equal("pdf", options.Filter);
			Assert.False(options.Json);
		}

		[Fact]
		public void Options_filter_and_applies_to_do_not_share_a_field()
		{
			OptionCatalogOptions options =
				((OptionsCommand)Bind("options", "--filter", "BW", "--applies-to", "document").Command).Options;

			Assert.Equal("BW", options.Filter);
			Assert.Equal("document", options.AppliesTo);
		}

		// ====================================================== arity behaviour

		[Fact]
		public void A_single_value_argument_given_twice_keeps_the_last()
		{
			SharedOptions shared = Bind("convert", "--to", "text", "-o", "first", "-o", "second", "a.docx").SharedOptions;

			Assert.Equal("second", shared.Output.OutDir);
		}

		[Fact]
		public void An_absent_argument_leaves_its_default_untouched()
		{
			SharedOptions shared = Shared();

			Assert.Null(shared.Processing.Codepage);
			Assert.Null(shared.Output.OutFile);
			Assert.Empty(shared.Passwords.Passwords);
			Assert.Equal(OnErrorBehaviour.Skip, shared.Batch.OnError);
		}

		[Fact]
		public void Verb_selection_produces_the_right_command_type()
		{
			Assert.IsType<ConvertCommand>(Bind("convert", "--to", "text", "-o", "out", "a.docx").Command);
			Assert.IsType<ExtractCommand>(Bind("extract", "-o", "out", "a.zip").Command);
			Assert.IsType<ListCommand>(Bind("list", "a.zip").Command);
			Assert.IsType<IdentifyCommand>(Bind("identify", "a.bin").Command);
			Assert.IsType<MetaCommand>(Bind("meta", "a.pdf").Command);
			Assert.IsType<FormatsCommand>(Bind("formats").Command);
			Assert.IsType<OptionsCommand>(Bind("options").Command);
			Assert.IsType<VersionCommand>(Bind("version").Command);
		}

		// ====================================================== rejection

		[Fact]
		public void An_unknown_verb_is_a_usage_error()
		{
			UsageError error = Assert.Throws<UsageError>(() => Bind("nonsense", "a.docx"));

			Assert.Equal(ErrorCodes.UnknownCommand, error.Code);
		}

		[Fact]
		public void An_unknown_argument_is_a_usage_error()
		{
			UsageError error = Assert.Throws<UsageError>(() => Bind("convert", "--to", "text", "--nonsense", "a.docx"));

			Assert.Equal(ErrorCodes.UnknownArgument, error.Code);
		}

		[Fact]
		public void Prefix_abbreviation_does_not_resolve()
		{
			// argparse allows this by default and the other three do not, so it is
			// switched off everywhere. Without this test the Python port silently
			// accepts command lines the others reject.
			Assert.Throws<UsageError>(() => Bind("convert", "--to", "text", "--out-d", "out", "a.docx"));
		}

		[Fact]
		public void A_argument_from_another_verb_is_rejected()
		{
			// --combine belongs to convert; extract must not quietly accept it.
			Assert.Throws<UsageError>(() => Bind("extract", "--combine", "-o", "out", "a.zip"));
		}

		[Fact]
		public void A_bad_value_is_reported_by_us_and_not_by_the_parsing_library()
		{
			// The error code proves the message came from ValueParsers rather than
			// McMaster, which is what keeps stderr identical across the four ports.
			UsageError error = Assert.Throws<UsageError>(
				() => Bind("convert", "--to", "text", "--max-depth", "abc", "-o", "out", "a.docx"));

			Assert.Equal(ErrorCodes.BadValue, error.Code);
		}
	}
}
