"""
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
"""

import unittest

from docfilters_cli.cli.bound_command_line import BoundCommandLine
from docfilters_cli.cli.command_line_app import CommandLineApp
from docfilters_cli.commands.convert_command import ConvertCommand
from docfilters_cli.commands.extract_command import ExtractCommand
from docfilters_cli.commands.formats_command import FormatsCommand
from docfilters_cli.commands.identify_command import IdentifyCommand
from docfilters_cli.commands.list_command import ListCommand
from docfilters_cli.commands.meta_command import MetaCommand
from docfilters_cli.commands.options_command import OptionsCommand
from docfilters_cli.commands.version_command import VersionCommand
from docfilters_cli.configuration.command_options import ConvertOptions, ExtractOptions
from docfilters_cli.configuration.shared_options import OnErrorBehaviour, SharedOptions
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import ErrorCodes, UsageError
from fakes.in_memory_file_system import InMemoryFileSystem
from fakes.recording_console import RecordingConsole


class ArgumentBindingTests(unittest.TestCase):
    """
    That each argument's setter writes to the field it is supposed to.

    This is the one mapping in the project that is hand-written once per argument, roughly
    seventy-five times, which makes it the likeliest place for a copy-and-paste mistake. A
    setter pointed at the wrong field imports cleanly, keeps its correct name and arity, and
    would pass every declaration test in argument_table_tests -- so it needs checking
    directly.

    argument_table_tests asks "does this argument exist?". This asks "does it do what it
    says?".
    """

    # ====================================================== shared: output

    def test_out_dir_writes_to_output_out_dir(self):
        self.assertEqual("out", _shared().output.out_dir)

    def test_short_o_is_an_alias_for_out_dir_and_never_out_file(self):
        # The old tool's -o guessed file-or-directory from the format. It must not.
        shared = _bind("convert", "--to", "text", "-o", "out", "a.docx").shared_options

        self.assertEqual("out", shared.output.out_dir)
        self.assertIsNone(shared.output.out_file)

    def test_out_file_writes_to_output_out_file_and_not_out_dir(self):
        shared = _bind("convert", "--to", "text", "--out-file", "a.txt",
                       "a.docx").shared_options

        self.assertEqual("a.txt", shared.output.out_file)
        self.assertIsNone(shared.output.out_dir)

    def test_out_name_compiles_to_a_template(self):
        shared = _shared("--out-name", "{stem}-{page:04}.{ext}")

        self.assertIsNotNone(shared.output.name_template)
        self.assertTrue(shared.output.name_template.uses_page)

    def test_stdout_sets_the_stdout_argument(self):
        shared = _bind("convert", "--to", "text", "--stdout", "a.docx").shared_options

        self.assertTrue(shared.output.use_stdout)

    def test_out_file_dash_is_the_same_as_stdout(self):
        # Two spellings, one setting. Nothing downstream should see the difference.
        shared = _bind("convert", "--to", "text", "--out-file", "-", "a.docx").shared_options

        self.assertTrue(shared.output.use_stdout)

    def test_overwrite_is_a_switch(self):
        self.assertTrue(_shared("--overwrite").output.overwrite)
        self.assertFalse(_shared().output.overwrite)

    # ====================================================== shared: walk

    def test_positional_paths_land_in_input_paths_in_order(self):
        shared = _bind("convert", "--to", "text", "--out-dir", "out", "a.docx",
                       "b.docx").shared_options

        self.assertEqual(["a.docx", "b.docx"], shared.input.paths)

    def test_a_dash_token_that_is_not_an_option_spelling_is_a_path(self):
        # A token that starts with a dash but is not one of this tool's own option
        # spellings is a path. looks_like_option is the whole rule, and these five shapes
        # are the ones a parsing library reads differently: a cluster of short options,
        # the end-of-options separator, a long option, and a long option whose name
        # begins with a dash. Reported from the field against 1.0.0-prerelease.1 as -tsv.
        #
        # The C# is an xUnit Theory with one InlineData per token. unittest has no
        # parametrised test, so this is one test looping over subTest, which reports each
        # token separately on failure and keeps the one name compare_inventory matches.
        for token in ["-tsv", "-ab", "--", "---weird", "----"]:
            with self.subTest(token=token):
                shared = _bind("convert", "--to", "text", "--out-dir", "out",
                               token).shared_options

                self.assertEqual([token], shared.input.paths)

    def test_a_dash_shaped_path_keeps_its_position_among_the_others(self):
        # Order is load-bearing: --combine writes the inputs in the order given.
        # Collecting the odd tokens separately and appending them would pass the test
        # above and still get this wrong.
        shared = _bind("convert", "--to", "text", "--out-dir", "out",
                       "a.docx", "-tsv", "b.docx").shared_options

        self.assertEqual(["a.docx", "-tsv", "b.docx"], shared.input.paths)

    def test_an_option_value_may_look_like_a_path(self):
        # The partition walks by arity, not by shape: --out-name takes a value, so the
        # next token is that value however it is spelled.
        shared = _bind("convert", "--to", "text", "--out-dir", "out",
                       "--out-name", "-{stem}.txt", "a.docx").shared_options

        self.assertIsNotNone(shared.output.name_template)
        self.assertEqual(["a.docx"], shared.input.paths)

    def test_files_from_writes_its_path(self):
        self.assertEqual("list.txt", _shared("--files-from", "list.txt").input.files_from)

    def test_max_depth_parses_into_its_own_field(self):
        shared = _shared("--max-depth", "3")

        self.assertEqual(3, shared.input.max_depth)
        self.assertFalse(shared.input.no_recurse_was_given)
        self.assertEqual(3, shared.input.effective_max_depth)

    def test_max_depth_defaults_to_unlimited(self):
        # Directories are walked recursively by default.
        shared = _shared()

        self.assertIsNone(shared.input.max_depth)
        self.assertIsNone(shared.input.effective_max_depth)

    def test_no_recurse_does_not_write_the_depth_field(self):
        # The two arguments must stay distinguishable: if --no-recurse wrote 1 into
        # max_depth, then "--max-depth 5 --no-recurse" would be indistinguishable from a
        # plain "--max-depth 1", and the contradiction could never be reported. The
        # resolved answer comes from effective_max_depth instead.
        shared = _shared("--no-recurse")

        self.assertTrue(shared.input.no_recurse_was_given)
        self.assertIsNone(shared.input.max_depth)
        self.assertEqual(1, shared.input.effective_max_depth)

    def test_the_contradictory_depth_pair_stays_distinguishable_after_binding(self):
        # The state the validator has to be able to see. Binding must not collapse it
        # before validation runs.
        conflicting = _bind("convert", "--to", "text", "--out-dir", "out",
                            "--max-depth", "5", "--no-recurse", "a.docx").shared_options

        self.assertEqual(5, conflicting.input.max_depth)
        self.assertTrue(conflicting.input.no_recurse_was_given)

    def test_follow_uses_capital_l(self):
        self.assertTrue(_shared("-L").input.follow_symlinks)
        self.assertTrue(_shared("--follow").input.follow_symlinks)

    def test_sort_and_include_hidden_are_switches(self):
        self.assertTrue(_shared("--sort").input.sort)
        self.assertTrue(_shared("--include-hidden").input.include_hidden)
        self.assertFalse(_shared().input.sort)
        self.assertFalse(_shared().input.include_hidden)

    def test_glob_accumulates_and_builds_a_matcher(self):
        shared = _shared("-g", "**/*.pdf")

        self.assertEqual(["**/*.pdf"], shared.input.glob_patterns)
        self.assertEqual([], shared.input.regex_patterns)
        self.assertIsNotNone(shared.input.matcher)
        self.assertTrue(shared.input.matcher.matches("a/b.pdf"))

    def test_glob_is_repeatable_and_accumulates(self):
        shared = _shared("-g", "**/*.pdf", "-g", "!**/draft/*")

        self.assertEqual(["**/*.pdf", "!**/draft/*"], shared.input.glob_patterns)
        self.assertTrue(shared.input.matcher.matches("final/a.pdf"))
        self.assertFalse(shared.input.matcher.matches("draft/a.pdf"))

    def test_regex_accumulates_into_its_own_list(self):
        # The two pattern languages must stay distinguishable after binding, or the
        # validator cannot tell that both were given.
        shared = _shared("--regex", r".*\.pdf$")

        self.assertEqual([r".*\.pdf$"], shared.input.regex_patterns)
        self.assertEqual([], shared.input.glob_patterns)
        self.assertIsNotNone(shared.input.matcher)
        self.assertTrue(shared.input.matcher.matches("a/b.pdf"))

    def test_regex_is_repeatable_and_accumulates(self):
        shared = _shared("--regex", r".*\.pdf$", "--regex", r".*\.docx$")

        self.assertEqual(2, len(shared.input.regex_patterns))

    def test_neither_pattern_argument_leaves_the_matcher_unset(self):
        shared = _shared()

        self.assertEqual([], shared.input.glob_patterns)
        self.assertEqual([], shared.input.regex_patterns)
        self.assertIsNone(shared.input.matcher)

    def test_a_malformed_regex_is_a_usage_error_at_bind_time(self):
        # Compiled in stage 2, the same as the subfile globs, so a typo is reported
        # before any directory walk or file work starts.
        with self.assertRaises(UsageError):
            _shared("--regex", "[unclosed")

    def test_include_and_exclude_types_split_on_semicolons(self):
        shared = _shared("--include-types", "PDF;DOCX", "--exclude-types", "ZIP")

        self.assertEqual(["PDF", "DOCX"], shared.input.include_types)
        self.assertEqual(["ZIP"], shared.input.exclude_types)

    # ================================================ shared: subfiles

    def test_subfiles_short_and_long_forms_agree(self):
        self.assertTrue(_shared("-s").subfiles.enabled)
        self.assertTrue(_shared("--subfiles").subfiles.enabled)
        self.assertFalse(_shared().subfiles.enabled)

    def test_subfile_depth_is_separate_from_directory_depth(self):
        # The single most confusable pair of arguments in the tool.
        shared = _shared("--max-depth", "2", "--subfile-depth", "5")

        self.assertEqual(2, shared.input.max_depth)
        self.assertEqual(5, shared.subfiles.max_depth)

    # ================================================ shared: passwords

    def test_password_is_repeatable_and_accumulates_in_order(self):
        shared = _shared("-p", "first", "-p", "second")

        self.assertEqual(["first", "second"], shared.passwords.passwords)

    def test_password_file_and_prompt_write_their_own_fields(self):
        shared = _shared("--password-file", "pw.txt", "--password-prompt")

        self.assertEqual("pw.txt", shared.passwords.password_file)
        self.assertTrue(shared.passwords.prompt)

    # ================================================ shared: processing

    def test_engine_options_are_stored_verbatim_and_not_parsed(self):
        # The CLI does not interpret this string; the engine does.
        shared = _shared("--options", "BW_MODE=3;CODEPAGE=1252")

        self.assertEqual("BW_MODE=3;CODEPAGE=1252", shared.processing.engine_options)

    def test_codepage_and_temp_dir_write_their_own_fields(self):
        shared = _shared("--codepage", "1252", "--temp-dir", "/tmp/df")

        self.assertEqual("1252", shared.processing.codepage)
        self.assertEqual("/tmp/df", shared.processing.temp_dir)

    # ================================================ shared: batch

    def test_on_error_defaults_to_skip(self):
        shared = _shared()

        self.assertEqual(OnErrorBehaviour.SKIP, shared.batch.on_error)
        self.assertFalse(shared.batch.on_error_was_explicit)

    def test_on_error_stop_is_recorded_as_explicit(self):
        shared = _shared("--on-error", "stop")

        self.assertEqual(OnErrorBehaviour.STOP, shared.batch.on_error)
        self.assertTrue(shared.batch.on_error_was_explicit)

    def test_fail_fast_writes_stop_through_the_same_field_and_flags_itself(self):
        # Recorded separately so a contradictory pair can be rejected rather than
        # silently resolved.
        shared = _shared("--fail-fast")

        self.assertEqual(OnErrorBehaviour.STOP, shared.batch.on_error)
        self.assertTrue(shared.batch.fail_fast_was_given)
        self.assertFalse(shared.batch.on_error_was_explicit)

    # ================================================ shared: console, library

    def test_console_switches_write_their_own_fields(self):
        self.assertTrue(_shared("--color").console.color)
        self.assertTrue(_shared("--verbose").console.verbose)
        self.assertTrue(_shared("--silent").console.silent)

    def test_license_key_and_library_have_no_short_forms(self):
        shared = _shared("--license-key", "KEY", "--library", "/opt/df")

        self.assertEqual("KEY", shared.library.license_key)
        self.assertEqual("/opt/df", shared.library.library_path)

    # ================================================ shared: diagnostics

    def test_timing_switches_write_their_own_fields(self):
        self.assertTrue(_shared("--time").diagnostics.time)
        self.assertTrue(_shared("--logtime").diagnostics.log_time)
        self.assertTrue(_shared("--logmem").diagnostics.log_memory)
        self.assertTrue(_shared("--progress").diagnostics.progress)
        self.assertTrue(_shared("--pause").diagnostics.pause)

    def test_limits_parse_as_numbers(self):
        shared = _shared("--max-time", "60", "--timeout", "10", "--max-memory", "512")

        self.assertEqual(60, shared.diagnostics.max_time)
        self.assertEqual(10, shared.diagnostics.timeout)
        self.assertEqual(512, shared.diagnostics.max_memory)

    def test_memory_status_levels_escalate(self):
        # -x reports once at the end of the run, --xx after every document.
        self.assertEqual(0, _shared().diagnostics.memory_status_level)
        self.assertEqual(1, _shared("-x").diagnostics.memory_status_level)
        self.assertEqual(2, _shared("--xx").diagnostics.memory_status_level)

    def test_the_higher_memory_status_level_wins_whichever_order_they_are_given(self):
        # A level, not two independent switches: --xx already includes what -x does,
        # so the pair is redundant rather than contradictory.
        self.assertEqual(2, _shared("-x", "--xx").diagnostics.memory_status_level)
        self.assertEqual(2, _shared("--xx", "-x").diagnostics.memory_status_level)

    def test_memory_status_and_logmem_are_separate_settings(self):
        # --logmem samples this process's peak memory while working; -x/--xx ask the
        # engine what it has allocated.
        shared = _shared("--xx", "--logmem")

        self.assertEqual(2, shared.diagnostics.memory_status_level)
        self.assertTrue(shared.diagnostics.log_memory)

    def test_the_log_aliases_write_their_own_fields_and_resolve_to_one_value(self):
        # Each alias keeps its own field so the validator can see that both were given;
        # effective_log_level is what everything downstream reads.
        via_short = _shared("--log-level", "trace")
        self.assertEqual("trace", via_short.diagnostics.log_level)
        self.assertIsNone(via_short.diagnostics.debug_log_level)
        self.assertEqual("trace", via_short.diagnostics.effective_log_level)

        via_debug = _shared("--debug-log-level", "trace")
        self.assertIsNone(via_debug.diagnostics.log_level)
        self.assertEqual("trace", via_debug.diagnostics.debug_log_level)
        self.assertEqual("trace", via_debug.diagnostics.effective_log_level)

    def test_the_module_aliases_write_their_own_fields_too(self):
        self.assertEqual("pdf",
                         _shared("--log-modules", "pdf").diagnostics.effective_log_modules)
        self.assertEqual(
            "pdf", _shared("--debug-log-modules", "pdf").diagnostics.effective_log_modules)
        self.assertIsNone(_shared("--log-modules", "pdf").diagnostics.debug_log_modules)

    def test_the_remaining_debug_switches_write_their_own_fields(self):
        self.assertTrue(_shared("--debug-use-memory-stream").diagnostics.use_memory_stream)
        self.assertTrue(_shared("--debug-measure-io").diagnostics.measure_io)
        self.assertEqual(
            4096, _shared("--debug-truncate", "4096").diagnostics.truncate_input_bytes)

    def test_dry_run_and_print_plan_write_the_same_field(self):
        self.assertTrue(_shared("--dry-run").dry_run)
        self.assertTrue(_shared("--print-plan").dry_run)
        self.assertFalse(_shared().dry_run)

    def test_print_contract_writes_its_own_field(self):
        self.assertTrue(_shared("--print-contract").print_contract)

    # ====================================================== convert

    def test_to_resolves_through_the_format_catalogue(self):
        options = _convert()

        self.assertIsNotNone(options.format)
        self.assertEqual("text", options.format.token)

    def test_fallback_is_no_longer_an_argument(self):
        """
        --fallback is gone, so it is an unknown argument like any other.

        Kept as a test rather than deleted with the option: it was accepted for long
        enough to appear in scripts, and "unknown argument" is the answer those should
        get -- not silent acceptance, and not a crash.
        """
        with self.assertRaises(UsageError) as caught:
            _convert("--fallback", "text")

        self.assertEqual(ErrorCodes.UNKNOWN_ARGUMENT, caught.exception.code)
        self.assertEqual(exit_code.USAGE_ERROR, caught.exception.exit_code)

    def test_combine_and_the_simple_switches_write_their_own_fields(self):
        self.assertTrue(_convert("--combine").combine)
        self.assertTrue(_convert("--no-images").no_images)
        self.assertTrue(_convert("--include-control-codes").include_control_codes)
        self.assertTrue(_convert("--fingerprint").fingerprint)
        self.assertTrue(_convert("--view").view)

    def test_canvas_and_render_options_do_not_share_a_field(self):
        options = _convert("--canvas-options", "A=1", "--render-options", "B=2")

        self.assertEqual("A=1", options.canvas_options)
        self.assertEqual("B=2", options.render_options)

    def test_pages_compiles_to_a_range(self):
        options = _convert("--pages", "1,3-5")

        self.assertEqual([1, 3, 4, 5], list(options.pages.enumerate(10)))

    def test_watermark_writes_its_own_field(self):
        self.assertEqual("DRAFT", _convert("--watermark", "DRAFT").watermark)

    def test_redact_is_repeatable_and_each_occurrence_is_parsed(self):
        options = _convert("--redact", "2,10,20,100,50", "--redact", "3,0,0,50,50")

        self.assertEqual(2, len(options.redactions))
        self.assertEqual(2, options.redactions[0].page)
        self.assertEqual(100, options.redactions[0].width)
        self.assertEqual(3, options.redactions[1].page)

    def test_form_is_repeatable_and_each_occurrence_is_parsed(self):
        options = _convert("--form", "1,a=x", "--form", "2,b=y")

        self.assertEqual(2, len(options.form_values))
        self.assertEqual("a", options.form_values[0].name)
        self.assertEqual("y", options.form_values[1].value)

    def test_each_structure_report_argument_sets_exactly_one_report(self):
        # Ten near-identical booleans: the highest copy-and-paste risk in the table.
        # Exactly one must flip, and `any` must notice.
        for argument in ("--fonts",
                         "--font-mappings",
                         "--page-fonts",
                         "--page-font-mappings",
                         "--dimensions",
                         "--page-elements",
                         "--form-elements",
                         "--hyperlinks",
                         "--bookmarks",
                         "--annotations"):
            with self.subTest(argument=argument):
                options = _convert(argument)

                self.assertTrue(options.reports.any)

                switches = [options.reports.fonts, options.reports.font_mappings,
                            options.reports.page_fonts, options.reports.page_font_mappings,
                            options.reports.dimensions, options.reports.page_elements,
                            options.reports.form_elements, options.reports.hyperlinks,
                            options.reports.bookmarks, options.reports.annotations]

                self.assertEqual(1, sum(1 for switch in switches if switch))

    def test_no_structure_reports_are_on_by_default(self):
        self.assertFalse(_convert().reports.any)

    # ====================================================== extract

    def test_images_short_and_long_forms_agree(self):
        self.assertTrue(_extract("-i").images)
        self.assertTrue(_extract("--images").images)
        self.assertFalse(_extract().images)

    def test_subfile_name_filters_do_not_share_a_field(self):
        options = _extract("--include", "*invoice*", "--exclude", "*draft*")

        self.assertIsNotNone(options.subfile_include)
        self.assertIsNotNone(options.subfile_exclude)
        self.assertTrue(options.subfile_include.matches("2026-invoice-01.pdf"))
        self.assertTrue(options.subfile_exclude.matches("draft-notes.txt"))

    def test_subfile_filters_are_globs_compiled_during_binding(self):
        # The same pattern language as -g/--glob, compiled in stage 2 so a malformed
        # pattern is a usage error before any archive is opened. A different axis though:
        # -g picks files on disk, this picks subfiles inside a container.
        options = _extract("--include", "**/*.docx")

        self.assertTrue(options.subfile_include.matches("reports/q1.docx"))
        self.assertFalse(options.subfile_include.matches("reports/q1.pdf"))

    def test_subfile_filters_are_repeatable_like_the_path_globs(self):
        # Repeatable for the same reason -g is: one glob per idea reads better than one
        # glob doing several jobs.
        options = _extract("--include", "*.docx", "--include", "*.xlsx")

        self.assertEqual(["*.docx", "*.xlsx"], options.include_patterns)
        self.assertTrue(options.subfile_include.matches("q1.docx"))
        self.assertTrue(options.subfile_include.matches("q1.xlsx"))
        self.assertFalse(options.subfile_include.matches("q1.pdf"))

    def test_include_and_exclude_may_both_be_given(self):
        # Unlike -g and --regex, which are two spellings of one idea and contradict,
        # these are complementary: keep these, then drop those.
        options = _extract("--include", "*.docx", "--exclude", "draft-*")

        self.assertTrue(options.subfile_include.matches("q1.docx"))
        self.assertTrue(options.subfile_exclude.matches("draft-q1.docx"))

    def test_a_malformed_subfile_glob_is_a_usage_error(self):
        with self.assertRaises(UsageError):
            _extract("--include", "[")

    def test_extract_regex_selects_paths_like_it_does_everywhere_else(self):
        # --regex means one thing everywhere: select paths by regular expression.
        # extract accepts it like any other verb that walks a directory.
        shared = _bind("extract", "--regex", r".*\.zip$", "-o", "out",
                       "archives").shared_options

        self.assertIsNotNone(shared.input.matcher)
        self.assertTrue(shared.input.matcher.matches("a/b.zip"))

    def test_extract_has_no_way_to_make_the_subfile_filters_regular_expressions(self):
        # The subfile filters are globs, with no mode switch to make them regular
        # expressions. --regex on extract selects paths, as it does everywhere.
        with self.assertRaises(UsageError):
            _bind("extract", "--include", "inv.*", "--content-regex", "-o", "out", "a.zip")

    def test_make_dirs_is_the_default_and_flat_turns_it_off(self):
        self.assertTrue(_extract().replicate_tree)
        self.assertTrue(_extract("--make-dirs").replicate_tree)
        self.assertFalse(_extract("--flat").replicate_tree)

    def test_the_two_layout_arguments_each_record_their_own_presence(self):
        # So that --make-dirs --flat can be rejected rather than silently resolved, for
        # the same reason --max-depth and --no-recurse stay apart.
        self.assertTrue(_extract("--make-dirs").make_dirs_was_given)
        self.assertFalse(_extract("--make-dirs").flat_was_given)
        self.assertTrue(_extract("--flat").flat_was_given)
        self.assertFalse(_extract("--flat").make_dirs_was_given)
        self.assertFalse(_extract().make_dirs_was_given)
        self.assertFalse(_extract().flat_was_given)

    def test_sub_id_writes_its_own_field(self):
        self.assertTrue(_extract("--sub-id").append_subfile_id)

    # ====================================================== other verbs

    def test_json_binds_on_each_listing_command(self):
        # No casts: the reference needs one per line to reach Options through ICommand,
        # and Python reads the attribute off whatever the verb produced.
        self.assertTrue(_bind("list", "--json", "a.zip").command.options.json)
        self.assertTrue(_bind("identify", "--json", "a.bin").command.options.json)
        self.assertTrue(_bind("meta", "--json", "a.pdf").command.options.json)
        self.assertTrue(_bind("formats", "--json").command.options.json)
        self.assertTrue(_bind("options", "--json").command.options.json)

    def test_identify_capabilities_binds(self):
        self.assertTrue(
            _bind("identify", "--capabilities", "a.bin").command.options.capabilities)

    def test_formats_filter_binds(self):
        options = _bind("formats", "--filter", "pdf").command.options

        self.assertEqual("pdf", options.filter)
        self.assertFalse(options.json)

    def test_options_filter_and_applies_to_do_not_share_a_field(self):
        options = _bind("options", "--filter", "BW", "--applies-to",
                        "document").command.options

        self.assertEqual("BW", options.filter)
        self.assertEqual("document", options.applies_to)

    # ====================================================== arity behaviour

    def test_a_single_value_argument_given_twice_keeps_the_last(self):
        shared = _bind("convert", "--to", "text", "-o", "first", "-o", "second",
                       "a.docx").shared_options

        self.assertEqual("second", shared.output.out_dir)

    def test_an_absent_argument_leaves_its_default_untouched(self):
        shared = _shared()

        self.assertIsNone(shared.processing.codepage)
        self.assertIsNone(shared.output.out_file)
        self.assertEqual([], shared.passwords.passwords)
        self.assertEqual(OnErrorBehaviour.SKIP, shared.batch.on_error)

    def test_verb_selection_produces_the_right_command_type(self):
        # The reference's eight Assert.IsType calls, as one loop: each line would
        # otherwise wrap, and isinstance is the mapping for IsType -- exact there,
        # subclass-tolerant here, which no command has a subclass to exploit.
        for expected, args in (
                (ConvertCommand, ("convert", "--to", "text", "-o", "out", "a.docx")),
                (ExtractCommand, ("extract", "-o", "out", "a.zip")),
                (ListCommand, ("list", "a.zip")),
                (IdentifyCommand, ("identify", "a.bin")),
                (MetaCommand, ("meta", "a.pdf")),
                (FormatsCommand, ("formats",)),
                (OptionsCommand, ("options",)),
                (VersionCommand, ("version",))):
            with self.subTest(command=expected.__name__):
                self.assertIsInstance(_bind(*args).command, expected)

    # ====================================================== rejection

    def test_an_unknown_verb_is_a_usage_error(self):
        with self.assertRaises(UsageError) as caught:
            _bind("nonsense", "a.docx")

        self.assertEqual(ErrorCodes.UNKNOWN_COMMAND, caught.exception.code)

    def test_an_unknown_argument_is_a_usage_error(self):
        with self.assertRaises(UsageError) as caught:
            _bind("convert", "--to", "text", "--nonsense", "a.docx")

        self.assertEqual(ErrorCodes.UNKNOWN_ARGUMENT, caught.exception.code)

    def test_prefix_abbreviation_does_not_resolve(self):
        # --out-d must not resolve to --out-dir. Prefix abbreviation is a common
        # parsing-library default, and none of the four allows it.
        with self.assertRaises(UsageError):
            _bind("convert", "--to", "text", "--out-d", "out", "a.docx")

    def test_an_argument_from_another_verb_is_rejected(self):
        # --combine belongs to convert; extract must not quietly accept it.
        with self.assertRaises(UsageError):
            _bind("extract", "--combine", "-o", "out", "a.zip")

    def test_a_bad_value_is_reported_by_us_and_not_by_the_parsing_library(self):
        # The error code proves the message came from value_parsers, which is what keeps
        # stderr identical across the four ports.
        with self.assertRaises(UsageError) as caught:
            _bind("convert", "--to", "text", "--max-depth", "abc", "-o", "out", "a.docx")

        self.assertEqual(ErrorCodes.BAD_VALUE, caught.exception.code)


def _bind(*args: str) -> BoundCommandLine:
    app = CommandLineApp(RecordingConsole(), InMemoryFileSystem())
    return app.bind(list(args))


def _shared(*arguments: str) -> SharedOptions:
    """A minimal valid convert invocation, with the arguments under test spliced in."""
    return _bind("convert", "--to", "text", "--out-dir", "out",
                 *arguments, "a.docx").shared_options


def _convert(*arguments: str) -> ConvertOptions:
    return _bind("convert", "--to", "text",
                 *arguments, "--out-dir", "out", "a.docx").command.options


def _extract(*arguments: str) -> ExtractOptions:
    return _bind("extract", *arguments, "--out-dir", "out", "a.zip").command.options
