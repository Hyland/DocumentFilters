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

from ..planning.format_spec import FormatKind
from ..planning.output_name_template import OutputNameTemplate
from ..runtime.cli_error import ErrorCodes, UsageError
from .shared_options import OnErrorBehaviour

# The rules that no single argument can enforce on its own, because they are relations
# between two or more arguments.
#
# Everything here is decidable from the command line alone. Rules that need to know how
# many input files actually exist — --out-file with more than one input, for instance —
# belong in planning.output_planner, which runs after the filesystem walk.
#
# A module of functions rather than a class: the C# is a static class, which is what C#
# has instead of a module.


def validate(shared):
    """Runs every shared-option rule. Raises UsageError when a rule was broken."""
    check_out_dir_names_a_directory(shared)
    check_output_targets_exclusive(shared)
    check_out_name_needs_out_dir(shared)
    check_stdout_has_no_directory(shared)
    check_stdout_has_no_file_target(shared)
    check_log_aliases_consistent(shared)
    check_recursion_depth_consistent(shared)
    check_verbosity_consistent(shared)
    check_pattern_languages_exclusive(shared)
    check_error_policy_consistent(shared)


def validate_convert(shared, options):
    """Runs the convert-specific rules in addition to the shared ones."""
    validate(shared)
    check_format_is_provided(options)
    check_combine_unsupported_format(options)
    check_stdout_needs_single_file_format(shared, options)
    check_combine_has_a_file_to_combine_into(shared, options)
    check_out_file_needs_single_file_format(shared, options)
    check_template_has_page_for_per_page_format(shared, options)
    check_fingerprint_has_no_normal_output(shared, options)
    check_reports_need_canvas_format(options)


def validate_extract(shared, options):
    """Runs the extract-specific rules in addition to the shared ones."""
    validate(shared)
    check_directory_layout_consistent(options)


def check_reports_need_canvas_format(options):
    """
    A structure report needs a format that opens the document paginated.

    Not a preference — one document open cannot serve both. Page structures only exist on
    a paginated handle, and text extraction only works on a text one, so pairing a report
    with a text format fails whichever way the format bit goes: opening paginated leaves
    TextConversion with IGR_E_NOT_VALID_FOR_THIS_CLASS and a zero-byte file, and opening
    as text leaves the report reading a handle the engine rejects (Invalid DocHandle for
    --bookmarks, which needs a paginated handle despite never touching a page). docpack is
    in the same position: it opens with no format bit at all.

    So the combination is refused before anything opens, which is also what doc2text does
    — its report flags and its conversions are alternative output modes, never both at
    once. One code, two messages, because text and docpack are unusable here for
    different reasons.

    Reads --to, which is the only thing a conversion format comes from: the planner
    resolves targets from --to alone, so that is the format the run is described as producing, and a rule that judged
    a different one would contradict the plan it validates.
    """
    if not options.reports.any:
        return

    output_format = options.format
    if output_format is None or output_format.kind == FormatKind.CANVAS:
        return

    report = options.reports.requested[0]
    if output_format.kind == FormatKind.DOC_PACK:
        message = "{0} needs a high-definition format; '{1}' writes a document pack, not pages".format(
            report, output_format.token)
    else:
        message = "{0} needs a high-definition format; '{1}' is text-mode only".format(report, output_format.token)
    raise UsageError(ErrorCodes.REPORT_NEEDS_HIDEF, message)


def check_format_is_provided(options):
    """
    --to is convert's one required argument.

    Nothing at the parser level enforces a required argument — ArgumentArity only ever
    means "does this token take a value", not "must this argument appear" — so an omitted
    --to left ConvertOptions.format None. Every other rule in validate_convert reads
    options.format unconditionally, so this has to run first: otherwise the first
    user-visible symptom of a missing --to would have been an AttributeError escaping
    uncaught past the entry point, which only catches CliError, instead of the usage error
    the mistake actually is.
    """
    if options.format is None:
        raise UsageError(ErrorCodes.MISSING_ARGUMENT, "--to is required")


def check_out_dir_names_a_directory(shared):
    """
    --out-dir "" names no directory, and is refused rather than read as one.

    Every output path is the directory, a slash and a name, so an empty directory does
    not mean "here": it turns a.pdf into /a.pdf, and an archive's tmp/file into
    /tmp/file -- a path relative to the root of the drive, chosen by the container. The
    current directory is ".".
    """
    if shared.output.out_dir is not None and len(shared.output.out_dir) == 0:
        raise UsageError(ErrorCodes.BAD_VALUE, "--out-dir does not accept the value ''")


def check_output_targets_exclusive(shared):
    """--out-file and --out-dir are mutually exclusive."""
    if shared.output.out_dir is not None and shared.output.out_file is not None:
        raise _contradiction("--out-dir", "--out-file")


def check_out_name_needs_out_dir(shared):
    """--out-name is a template for --out-dir and means nothing without it."""
    if shared.output.name_template is not None and shared.output.out_dir is None:
        raise UsageError(ErrorCodes.OUTNAME_NEEDS_OUTDIR,
                         "--out-name is a template for --out-dir and has no meaning without it")


def check_stdout_has_no_directory(shared):
    """--stdout cannot be combined with a directory target."""
    if shared.output.out_dir is not None and shared.output.use_stdout:
        raise _contradiction("--out-dir", "--stdout")


def check_stdout_has_no_file_target(shared):
    """
    --stdout cannot be combined with a file target either.

    --out-file - is the spelling that means stdout, so it is not caught here;
    --stdout --out-file book.pdf is, because output cannot go to both places. Neither
    check_output_targets_exclusive nor check_stdout_has_no_directory covers this pair.
    """
    if shared.output.out_file is not None and shared.output.use_stdout:
        raise _contradiction("--out-file", "--stdout")


def check_log_aliases_consistent(shared):
    """
    --log-level and --debug-log-level are the same setting, so giving both with different
    values is a contradiction. Likewise --log-modules and --debug-log-modules.

    Agreeing values are allowed, as with --fail-fast alongside an explicit
    --on-error stop: redundant is not wrong.
    """
    diagnostics = shared.diagnostics
    if (diagnostics.log_level is not None and diagnostics.debug_log_level is not None
            and diagnostics.log_level != diagnostics.debug_log_level):
        raise _contradiction("--log-level " + diagnostics.log_level,
                             "--debug-log-level " + diagnostics.debug_log_level)
    if (diagnostics.log_modules is not None and diagnostics.debug_log_modules is not None
            and diagnostics.log_modules != diagnostics.debug_log_modules):
        raise _contradiction("--log-modules " + diagnostics.log_modules,
                             "--debug-log-modules " + diagnostics.debug_log_modules)


def check_recursion_depth_consistent(shared):
    """
    --no-recurse contradicts an explicit --max-depth other than 1.

    Decidable only because the two arguments write separate fields; if --no-recurse wrote
    the depth, this rule could not exist.
    """
    if shared.input.max_depth is not None and shared.input.max_depth != 1 and shared.input.no_recurse_was_given:
        raise _contradiction("--max-depth", "--no-recurse")


def check_verbosity_consistent(shared):
    """--verbose and --silent contradict each other."""
    if shared.console.verbose and shared.console.silent:
        raise _contradiction("--verbose", "--silent")


def check_pattern_languages_exclusive(shared):
    """
    -g/--glob and --regex are two ways of saying the same thing, and cannot both be given.

    Glob is the idiom for ordinary selection and regex the power mode for patterns a glob
    cannot express; mixing them in one selection has no coherent meaning, and combining
    them silently would make the result depend on argument order.
    """
    if len(shared.input.glob_patterns) > 0 and len(shared.input.regex_patterns) > 0:
        raise _contradiction("--glob", "--regex")


def check_error_policy_consistent(shared):
    """
    --fail-fast contradicts an explicit --on-error skip.

    Rejected rather than resolved by precedence: the two arguments write the same setting,
    and the binder records each argument's occurrences rather than the relative order of two
    different arguments. All four implementations could be taught to record that order now
    that none of them delegates to a parsing library, and none is: a tie-break is a rule four
    ports would have to agree on for ever, where an error is a rule they already agree on.
    """
    if (shared.batch.on_error_was_explicit and shared.batch.on_error == OnErrorBehaviour.SKIP
            and shared.batch.fail_fast_was_given):
        raise _contradiction("--on-error skip", "--fail-fast")


def check_combine_unsupported_format(options):
    """
    --combine needs a format that can hold several documents.

    Keyed off FormatSpec.supports_combine, not is_single_file. Two different formats fail
    here for two different reasons, and the message says which: a per-page format writes
    one file per page and has nothing to merge into, while docpack writes a single file but
    holds exactly one document, because Extractor.MakeDocPack takes one extractor.
    """
    if not options.combine or options.format.supports_combine:
        return

    if not options.format.is_single_file:
        raise UsageError(ErrorCodes.COMBINE_UNSUPPORTED,
                         "--combine needs a format that writes one file; '{0}' writes one per page".format(
                             options.format.token))

    raise UsageError(ErrorCodes.COMBINE_UNSUPPORTED,
                     "--combine cannot merge documents into '{0}'; each document makes its own pack".format(
                         options.format.token))


def check_combine_has_a_file_to_combine_into(shared, options):
    """
    --combine needs an output file to combine into.

    Execution combines only when it has a target to combine into, so with --stdout it
    silently did not combine: each document was converted on its own and the results ran
    into one another down the pipe, which for a single-file format like PDF is several
    complete documents end to end rather than one. The user asked for one document and got
    a stream no reader can open, with exit 0 to say it worked.

    Refused rather than implemented: combining into a stream would mean a canvas that
    writes to stdout and outlives every document, which is a feature, not a fix. --out-file
    names the file to combine into, and the result can be piped from there.

    Streaming several documents is still allowed without --combine: for text that is
    concatenation and exactly what a pipeline wants.
    """
    if options.combine and shared.output.use_stdout:
        raise _contradiction("--combine", "--stdout")


def check_stdout_needs_single_file_format(shared, options):
    """--stdout can only stream a format that writes a single file."""
    if shared.output.use_stdout and not options.format.is_single_file:
        raise UsageError(ErrorCodes.STDOUT_NEEDS_SINGLE_FILE,
                         "--stdout can only stream a format that writes one file; '{0}' writes one per page".format(
                             options.format.token))


def check_out_file_needs_single_file_format(shared, options):
    """
    --out-file can only name a format that writes a single file.

    The same rule as the one above, for the other way of naming one output. It is
    decidable from the command line alone, so it belongs here rather than in the planner —
    which matters, because the planner's E_OUTFILE_MULTI_INPUT only fires once there is
    more than one input and would otherwise leave --to png --out-file one.png a.docx
    unreported.
    """
    if shared.output.out_file is not None and not options.format.is_single_file:
        raise UsageError(ErrorCodes.OUTFILE_NEEDS_SINGLE_FILE,
                         "--out-file can only name a format that writes one file; '{0}' writes one per page".format(
                             options.format.token))


def check_template_has_page_for_per_page_format(shared, options):
    """
    A per-page format needs a --out-name template containing {page}, or every page
    resolves to the same path.

    Without this, --to png --out-name "{stem}.{ext}" either fails after writing page one,
    or — with --overwrite — silently leaves only the last page. The default template
    already includes the page number; this rule covers the case where the user supplied
    their own.
    """
    if options.format.is_single_file:
        return

    template = shared.output.name_template
    if template is None:
        template = OutputNameTemplate.default(False)
    if not template.uses_page:
        raise UsageError(ErrorCodes.TEMPLATE_NEEDS_PAGE,
                         "--out-name must contain {page} when the format writes one file per page")


def check_fingerprint_has_no_normal_output(shared, options):
    """
    --fingerprint renders a page only to read a fingerprint attribute off it; the render
    itself is never kept, so every argument whose entire purpose is to control or expose
    that now-discarded content contradicts it.

    Confirmed against doc2text's own --fingerprint (main.cpp): it forces a throwaway image
    render and throws it away through a temporary stream (hidecontent) rather than the
    requested target, whenever the selected format is not already an image format.
    CanvasConversion does the equivalent — a discard stream in place of the real target —
    so --stdout, --view, --combine, --out-file and --out-dir would each silently do nothing
    under --fingerprint rather than the thing their name promises, which is exactly what a
    usage error is for. --redact, --form and --watermark are unaffected: they decorate the
    same throwaway render fingerprinting already needs, so there is nothing for them to
    contradict.
    """
    if not options.fingerprint:
        return

    if shared.output.use_stdout:
        raise _contradiction("--fingerprint", "--stdout")
    if options.view:
        raise _contradiction("--fingerprint", "--view")
    if options.combine:
        raise _contradiction("--fingerprint", "--combine")
    if shared.output.out_file is not None:
        raise _contradiction("--fingerprint", "--out-file")
    if shared.output.out_dir is not None:
        raise _contradiction("--fingerprint", "--out-dir")


def check_directory_layout_consistent(options):
    """--make-dirs and --flat contradict each other."""
    if options.make_dirs_was_given and options.flat_was_given:
        raise _contradiction("--make-dirs", "--flat")


def _contradiction(first: str, second: str) -> UsageError:
    """
    Builds the "cannot be used together" message every contradictory pair shares, naming
    each side by its long form — with its current value attached when the caller passes
    one — in argument-table order.
    """
    return UsageError(ErrorCodes.MUTUALLY_EXCLUSIVE, "{0} and {1} cannot be used together".format(first, second))
