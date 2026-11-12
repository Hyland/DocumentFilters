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

from docfilters_cli.configuration import options_validator
from docfilters_cli.configuration.command_options import (ConvertOptions, ExtractOptions, FormValue,
                                                          RedactSpec)
from docfilters_cli.configuration.shared_options import OnErrorBehaviour, SharedOptions
from docfilters_cli.planning.format_catalog import FormatCatalog
from docfilters_cli.planning.output_name_template import OutputNameTemplate
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import ErrorCodes, UsageError


class OptionsValidatorTests(unittest.TestCase):
    """
    The rules no single argument can enforce, because they are relations between
    arguments. Everything here is decidable from the command line alone — rules that need
    to know how many files exist live in the planner instead.
    """

    @staticmethod
    def _to_format(token):
        options = ConvertOptions()
        options.format = FormatCatalog.resolve(token)
        return options

    def test_a_bare_default_configuration_is_valid(self):
        options_validator.validate(SharedOptions())

    def test_out_file_and_out_dir_together_are_a_usage_error(self):
        shared = SharedOptions()
        shared.output.out_file = "book.pdf"
        shared.output.out_dir = "out"

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_out_name_without_out_dir_is_a_usage_error(self):
        # The template names a file inside a directory; without one it means nothing.
        shared = SharedOptions()
        shared.output.name_template = OutputNameTemplate.parse("{stem}.{ext}")

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(ErrorCodes.OUTNAME_NEEDS_OUTDIR, caught.exception.code)

    def test_subfile_depth_without_subfiles_is_a_usage_error(self):
        # The depth limits a descent that is not happening, so accepting it would
        # exit 0 having done none of what was asked.
        shared = SharedOptions()
        shared.subfiles.max_depth = 3

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(ErrorCodes.SUBFILE_DEPTH_NEEDS_SUBFILES, caught.exception.code)

    def test_subfiles_without_a_depth_is_an_unlimited_descent(self):
        # The rule is one-directional: it is the depth that needs the flag.
        shared = SharedOptions()
        shared.subfiles.enabled = True

        options_validator.validate(shared)

        self.assertIsNone(shared.subfiles.max_depth)

    def test_stdout_with_out_dir_is_a_usage_error(self):
        shared = SharedOptions()
        shared.output.use_stdout = True
        shared.output.out_dir = "out"

        with self.assertRaises(UsageError):
            options_validator.validate(shared)

    def test_stdout_with_a_file_target_is_a_usage_error(self):
        # Output cannot go to a file and to stdout. Neither the out-file/out-dir rule nor
        # the stdout/out-dir rule covers this pair.
        shared = SharedOptions()
        shared.output.use_stdout = True
        shared.output.out_file = "book.pdf"

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_out_file_dash_is_not_a_conflict_because_it_is_how_stdout_is_spelled(self):
        shared = SharedOptions()
        shared.output.use_stdout = True

        options_validator.validate(shared)

    def test_the_log_aliases_disagreeing_is_a_usage_error(self):
        # --log-level and --debug-log-level are one setting. Two arguments carrying values
        # into one field would silently drop whichever lost.
        shared = SharedOptions()
        shared.diagnostics.log_level = "trace"
        shared.diagnostics.debug_log_level = "info"

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_the_log_aliases_agreeing_is_merely_redundant(self):
        shared = SharedOptions()
        shared.diagnostics.log_level = "trace"
        shared.diagnostics.debug_log_level = "trace"

        options_validator.validate(shared)

        self.assertEqual("trace", shared.diagnostics.effective_log_level)

    def test_either_log_alias_alone_resolves(self):
        via_short = SharedOptions()
        via_short.diagnostics.log_level = "trace"
        self.assertEqual("trace", via_short.diagnostics.effective_log_level)

        via_debug = SharedOptions()
        via_debug.diagnostics.debug_log_level = "trace"
        self.assertEqual("trace", via_debug.diagnostics.effective_log_level)

        self.assertIsNone(SharedOptions().diagnostics.effective_log_level)

    def test_the_module_aliases_behave_the_same_way(self):
        conflicting = SharedOptions()
        conflicting.diagnostics.log_modules = "pdf"
        conflicting.diagnostics.debug_log_modules = "ocr"

        with self.assertRaises(UsageError):
            options_validator.validate(conflicting)

        agreeing = SharedOptions()
        agreeing.diagnostics.log_modules = "pdf"
        agreeing.diagnostics.debug_log_modules = "pdf"

        options_validator.validate(agreeing)
        self.assertEqual("pdf", agreeing.diagnostics.effective_log_modules)

    def test_either_pattern_language_alone_is_valid(self):
        # Compiling the matcher moved to CommandLineApp.Bind; this only exercises the
        # exclusivity rule below, which is still validate's job.
        globs = SharedOptions()
        globs.input.glob_patterns.append("**/*.pdf")
        options_validator.validate(globs)

        regexes = SharedOptions()
        regexes.input.regex_patterns.append(r"a/b\.pdf")
        options_validator.validate(regexes)

    def test_glob_and_regex_together_are_a_usage_error(self):
        # Two ways of saying the same thing. Combining them silently would make the
        # selection depend on argument order, and neither language subsumes the other, so
        # there is no sensible merge.
        shared = SharedOptions()
        shared.input.glob_patterns.append("**/*.pdf")
        shared.input.regex_patterns.append(r".*\.docx$")

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_verbose_and_silent_together_are_a_usage_error(self):
        shared = SharedOptions()
        shared.console.verbose = True
        shared.console.silent = True

        with self.assertRaises(UsageError):
            options_validator.validate(shared)

    def test_no_recurse_with_an_explicit_deeper_max_depth_is_a_usage_error(self):
        # --no-recurse means depth 1. Combined with --max-depth 5 the user has asked for
        # two different things.
        shared = SharedOptions()
        shared.input.max_depth = 5
        shared.input.no_recurse_was_given = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_no_recurse_with_max_depth_one_is_consistent(self):
        # Redundant rather than contradictory: both ask for the same thing.
        shared = SharedOptions()
        shared.input.max_depth = 1
        shared.input.no_recurse_was_given = True

        options_validator.validate(shared)

    def test_no_recurse_alone_is_fine(self):
        shared = SharedOptions()
        shared.input.no_recurse_was_given = True

        options_validator.validate(shared)
        self.assertEqual(1, shared.input.effective_max_depth)

    def test_max_depth_alone_is_fine(self):
        shared = SharedOptions()
        shared.input.max_depth = 5

        options_validator.validate(shared)
        self.assertEqual(5, shared.input.effective_max_depth)

    def test_neither_means_unlimited_depth(self):
        self.assertIsNone(SharedOptions().input.effective_max_depth)

    def test_make_dirs_with_flat_is_a_usage_error(self):
        # Same class of contradiction as the depth pair, and detectable for the same
        # reason: each argument records its own presence.
        options = ExtractOptions()
        options.make_dirs_was_given = True
        options.flat_was_given = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_extract(SharedOptions(), options)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_either_directory_layout_argument_alone_is_fine(self):
        make_dirs = ExtractOptions()
        make_dirs.make_dirs_was_given = True
        options_validator.validate_extract(SharedOptions(), make_dirs)

        flat = ExtractOptions()
        flat.flat_was_given = True
        options_validator.validate_extract(SharedOptions(), flat)

        self.assertTrue(make_dirs.replicate_tree)
        self.assertFalse(flat.replicate_tree)
        self.assertTrue(ExtractOptions().replicate_tree)

    def test_a_per_page_format_with_a_template_lacking_page_is_a_usage_error(self):
        # Every page would resolve to the same path: without --overwrite the run fails
        # after page one, and with it only the last page survives.
        shared = SharedOptions()
        shared.output.out_dir = "out"
        shared.output.name_template = OutputNameTemplate.parse("{stem}.{ext}")

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, self._to_format("png"))

        self.assertEqual(ErrorCodes.TEMPLATE_NEEDS_PAGE, caught.exception.code)

    def test_a_per_page_format_with_a_page_bearing_template_is_valid(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"
        shared.output.name_template = OutputNameTemplate.parse("{stem}-{page:04}.{ext}")

        options_validator.validate_convert(shared, self._to_format("png"))

    def test_a_single_file_format_needs_no_page_in_its_template(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"
        shared.output.name_template = OutputNameTemplate.parse("{stem}.{ext}")

        options_validator.validate_convert(shared, self._to_format("pdf"))

    def test_a_per_page_format_with_no_template_is_valid_because_the_default_has_a_page(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"

        options_validator.validate_convert(shared, self._to_format("png"))

    def test_fail_fast_with_an_explicit_on_error_skip_is_a_usage_error(self):
        # The two arguments write the same setting and contradict each other. Rejected
        # rather than resolved by precedence, because none of the four parsing libraries
        # reliably records which option came first.
        shared = SharedOptions()
        shared.batch.on_error = OnErrorBehaviour.SKIP
        shared.batch.on_error_was_explicit = True
        shared.batch.fail_fast_was_given = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_fail_fast_with_an_explicit_on_error_stop_is_consistent(self):
        shared = SharedOptions()
        shared.batch.on_error = OnErrorBehaviour.STOP
        shared.batch.on_error_was_explicit = True
        shared.batch.fail_fast_was_given = True

        options_validator.validate(shared)

    def test_fail_fast_alone_is_fine(self):
        shared = SharedOptions()
        shared.batch.on_error = OnErrorBehaviour.STOP
        shared.batch.fail_fast_was_given = True

        options_validator.validate(shared)

    # ------------------------------------------------------------------------ convert

    def test_missing_to_is_a_usage_error(self):
        # Nothing at the parser level enforces a required argument, so an omitted --to
        # leaves ConvertOptions.format None; every other convert rule reads it
        # unconditionally, so this has to be caught first.
        shared = SharedOptions()
        shared.output.out_dir = "out"

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, ConvertOptions())

        self.assertEqual(ErrorCodes.MISSING_ARGUMENT, caught.exception.code)
        self.assertEqual(exit_code.USAGE_ERROR, caught.exception.exit_code)

    def test_to_alone_is_valid(self):
        options_validator.validate_convert(SharedOptions(), self._to_format("pdf"))

    def test_combine_with_a_one_per_page_format_is_a_usage_error(self):
        # You cannot merge documents into a format that emits a file per page.
        shared = SharedOptions()
        shared.output.out_file = "out.png"
        options = self._to_format("png")
        options.combine = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, options)

        self.assertEqual(ErrorCodes.COMBINE_UNSUPPORTED, caught.exception.code)

    def test_combine_with_a_single_file_format_is_valid(self):
        shared = SharedOptions()
        shared.output.out_file = "book.pdf"
        options = self._to_format("pdf")
        options.combine = True

        options_validator.validate_convert(shared, options)

    def test_combine_with_text_is_valid(self):
        # Text is a single-file format, so several documents can be concatenated.
        shared = SharedOptions()
        shared.output.out_file = "all.txt"
        options = self._to_format("text")
        options.combine = True

        options_validator.validate_convert(shared, options)

    def test_stdout_with_a_one_per_page_format_is_a_usage_error(self):
        shared = SharedOptions()
        shared.output.use_stdout = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, self._to_format("png"))

        self.assertEqual(ErrorCodes.STDOUT_NEEDS_SINGLE_FILE, caught.exception.code)

    def test_stdout_with_a_single_file_format_is_valid(self):
        shared = SharedOptions()
        shared.output.use_stdout = True

        options_validator.validate_convert(shared, self._to_format("pdf"))

    # -------------------------------------------------------------------- fingerprint

    def test_fingerprint_with_stdout_is_a_usage_error(self):
        # --fingerprint renders a page only to read a fingerprint attribute off it and
        # never keeps the render, the same way doc2text's own --fingerprint discards its
        # render through a temporary stream — so streaming that now-discarded content is a
        # contradiction, not a silent no-op.
        shared = SharedOptions()
        shared.output.use_stdout = True
        options = self._to_format("pdf")
        options.fingerprint = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, options)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_fingerprint_with_view_is_a_usage_error(self):
        shared = SharedOptions()
        options = self._to_format("pdf")
        options.fingerprint = True
        options.view = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, options)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_fingerprint_with_combine_is_a_usage_error(self):
        shared = SharedOptions()
        shared.output.out_file = "book.pdf"
        options = self._to_format("pdf")
        options.fingerprint = True
        options.combine = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, options)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_fingerprint_with_out_file_is_a_usage_error(self):
        shared = SharedOptions()
        shared.output.out_file = "book.pdf"
        options = self._to_format("pdf")
        options.fingerprint = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, options)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_fingerprint_with_out_dir_is_a_usage_error(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"
        options = self._to_format("pdf")
        options.fingerprint = True

        with self.assertRaises(UsageError) as caught:
            options_validator.validate_convert(shared, options)

        self.assertEqual(ErrorCodes.MUTUALLY_EXCLUSIVE, caught.exception.code)

    def test_fingerprint_with_no_output_target_at_all_is_valid(self):
        # The common case: no --out-dir, --out-file or --stdout given, so the planner's
        # own default ('.') is never something the user named.
        options = ConvertOptions()
        options.format = FormatCatalog.resolve("pdf")
        options.fingerprint = True

        options_validator.validate_convert(SharedOptions(), options)

    def test_fingerprint_with_page_decorations_is_valid(self):
        # --redact, --form and --watermark decorate the same throwaway render
        # fingerprinting already needs, so there is nothing for them to contradict.
        options = self._to_format("pdf")
        options.fingerprint = True
        options.watermark = "stamp.png"

        redaction = RedactSpec()
        redaction.page = 1
        redaction.x = 0
        redaction.y = 0
        redaction.width = 10
        redaction.height = 10
        options.redactions.append(redaction)

        form_value = FormValue()
        form_value.page = 1
        form_value.name = "field"
        form_value.value = "value"
        options.form_values.append(form_value)

        options_validator.validate_convert(SharedOptions(), options)

    def test_every_validation_failure_exits_with_the_usage_code(self):
        shared = SharedOptions()
        shared.output.out_file = "a"
        shared.output.out_dir = "b"

        with self.assertRaises(UsageError) as caught:
            options_validator.validate(shared)

        self.assertEqual(exit_code.USAGE_ERROR, caught.exception.exit_code)


if __name__ == '__main__':
    unittest.main()
