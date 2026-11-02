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

from typing import Any, List

from ..configuration import value_parsers
from ..configuration.shared_options import OnErrorBehaviour
from ..planning.format_catalog import FormatCatalog
from ..planning.output_name_template import OutputNameTemplate
from ..planning.page_range import PageRange
from .argument_definition import ArgumentArity, ArgumentDefinition

# ---------------------------------------------------------------------------
#  WHY THIS FILE LOOKS THE WAY IT DOES
#
#  If you are reading this to learn how to build a CLI on Document Filters in
#  Python, the idiomatic approach is argparse, and it is what the other samples
#  in this repository use:
#
#      parser = argparse.ArgumentParser()
#      parser.add_argument('-o', '--out-dir', help="Write into a directory")
#      parser.add_argument('-s', '--subfiles', action='store_true',
#                          help="Descend into subfiles, attachments and embedded documents")
#      options = parser.parse_args()
#
#  That is shorter and perfectly good. Reach for it in your own code.
#
#  This sample uses a table of setters instead, for a reason specific to this
#  project: the same CLI is implemented four times, in C#, Python, Java and
#  C++17, and the four must stay byte-for-byte identical.
#
#    * C# attributes cannot hold a lambda — they only take compile-time
#      constants — so an attribute cannot say where its value ends up. Neither
#      can picocli's annotations, which the Java port uses. argparse is no better
#      placed: its dest= names a field on one flat Namespace, so a declaration
#      cannot say that --out-dir belongs in shared.output.out_dir.
#    * A table where each row is (argument, help, arity, setter) says the whole
#      mapping on one line, ports directly to McMaster, picocli and CLI11, and
#      can be enumerated to emit `--print-contract` for the conformance tests.
#    * Values that can fail to convert are parsed in value_parsers, shared with
#      every other language, so a bad `--max-depth` gives the same message and
#      the same exit code in all four builds rather than four different ones
#      written by four different parsing libraries.
#
#  In short: the declarative form is better for one program, the table form is
#  better for one program written four times.
# ---------------------------------------------------------------------------

# The rows below write their values two ways, mirroring the C#'s two kinds of
# lambda: a setter that is one assignment stays on the row itself, and a setter
# that needs more than one statement is a function defined just above the table.
# A Python lambda body is an expression and assignment is a statement, so the
# one-liners go through _set() rather than assigning directly — that helper is the
# only thing in this file the C# does not have.


def _set(target: Any, name: str, value: Any) -> None:
    """
    Assigns to target.name, refusing to create a field that was not there already.

    setattr is what lets a row's setter stay on one line, but it names the field with
    a string, and a mistyped string would quietly add a new field and leave the
    argument doing nothing — the one mistake the C# compiler was catching here. The
    guard puts that check back; it is a runtime check rather than a compile-time one,
    which is why every argument is exercised by the binding tests.
    """
    if not hasattr(target, name):
        raise AttributeError("no such option field: " + type(target).__name__ + "." + name)

    setattr(target, name, value)


class ArgumentTable:
    """
    Every argument the tool accepts, paired with what it does. The single source of
    truth for the command line surface.
    """

    # Tuples rather than lists, which is how this port says IReadOnlyList: the two
    # are the source of truth for the surface, and a caller that appended to one
    # would change what the tool accepts.
    COMMANDS = ("convert", "extract", "list", "identify", "meta", "formats", "options")
    """
    The processing verbs, in help-display order. These are the commands that accept
    shared options, so this list drives shared_for and the contract dump.
    """

    ALL_VERBS = COMMANDS + ("help", "version")
    """
    Everything `docfilters <verb>` will dispatch, including the two that take no
    shared options at all.

    help and version are verbs, but they are not in COMMANDS because they process no
    documents and accept no shared options.

    Note that --help and -h as *arguments* appear in neither list: every parsing
    library provides them, and their rendering differs between McMaster, argparse and
    CLI11, which is exactly why help text is not part of the cross-language contract.
    """

    @staticmethod
    def shared_for(command: str) -> List[ArgumentDefinition]:
        """
        The shared arguments a given verb accepts.

        Composed from the group methods below rather than filtered out of one flat
        list. A verb simply does not call the groups it has no use for: identify writes
        no files, so it never calls output_arguments; formats and options take no input
        files, so they call neither the walk nor the selection groups.

        Expressing it as composition rather than a group tag on each argument means
        there is no group name to mistype and no filter to get wrong — an argument is
        on a command because that command asked for its group.
        """
        result: List[ArgumentDefinition] = []
        if command in ("convert", "extract", "list"):
            result.extend(ArgumentTable.output_arguments())
            result.extend(ArgumentTable.subfile_arguments())
            result.extend(ArgumentTable._core_arguments())
        elif command == "identify":
            result.extend(ArgumentTable._core_arguments())
        elif command == "meta":
            result.extend(ArgumentTable.output_arguments())
            result.extend(ArgumentTable._core_arguments())
        elif command in ("formats", "options"):
            result.extend(ArgumentTable.console_arguments())
            result.extend(ArgumentTable.global_arguments())
            result.extend(ArgumentTable.library_arguments())
        else:
            raise ValueError("unknown command '" + command + "'")
        return result

    @staticmethod
    def _core_arguments() -> List[ArgumentDefinition]:
        """
        The shared groups every input-processing command takes regardless of whether it
        also writes output or descends into subfiles: name selection, type filters, the
        walk, passwords, processing, batch control, console, the global switches,
        diagnostics and the engine location.
        """
        return (ArgumentTable.match_arguments()
                + ArgumentTable.type_arguments()
                + ArgumentTable.walk_arguments()
                + ArgumentTable.password_arguments()
                + ArgumentTable.processing_arguments()
                + ArgumentTable.batch_arguments()
                + ArgumentTable.console_arguments()
                + ArgumentTable.global_arguments()
                + ArgumentTable.diagnostic_arguments()
                + ArgumentTable.library_arguments())

    @staticmethod
    def output_arguments() -> List[ArgumentDefinition]:
        """-o/--out-dir, --out-file, --out-name, --stdout, --overwrite."""

        def set_out_file(s, v):
            if v == "-":
                s.output.use_stdout = True
            else:
                s.output.out_file = v

        return [
            ArgumentDefinition("-o|--out-dir", "Write into a directory; safe for any format",
                               ArgumentArity.SINGLE_VALUE, lambda s, v: _set(s.output, 'out_dir', v)),

            ArgumentDefinition("--out-file", "Write one artifact to exactly this path; '-' means stdout",
                               ArgumentArity.SINGLE_VALUE, set_out_file),

            ArgumentDefinition("--out-name",
                               "Filename template inside --out-dir, e.g. {stem}-{page:04}.{ext}",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.output, 'name_template', OutputNameTemplate.parse(v))),

            ArgumentDefinition("--stdout", "Stream a single-file format to stdout",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.output, 'use_stdout', True)),

            ArgumentDefinition("--overwrite", "Replace existing outputs instead of erroring",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.output, 'overwrite', True)),
        ]

    @staticmethod
    def subfile_arguments() -> List[ArgumentDefinition]:
        """-s/--subfiles and --subfile-depth."""
        return [
            ArgumentDefinition("-s|--subfiles",
                               "Descend into subfiles, attachments and embedded documents",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.subfiles, 'enabled', True)),

            ArgumentDefinition("--subfile-depth", "How deep to recurse into nested subfiles",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.subfiles, 'max_depth',
                                                 value_parsers.parse_count("--subfile-depth", v))),
        ]

    @staticmethod
    def match_arguments() -> List[ArgumentDefinition]:
        """-g/--glob and --regex: selecting inputs by path."""
        return [
            ArgumentDefinition("-g|--glob",
                               "Keep paths matching a glob, e.g. **/*.pdf; prefix ! to exclude",
                               ArgumentArity.MULTIPLE_VALUE, lambda s, v: s.input.glob_patterns.append(v)),

            ArgumentDefinition("--regex", "Same selection, but match the path with a regular expression",
                               ArgumentArity.MULTIPLE_VALUE, lambda s, v: s.input.regex_patterns.append(v)),
        ]

    @staticmethod
    def type_arguments() -> List[ArgumentDefinition]:
        """--include-types and --exclude-types: selecting by detected format."""

        def set_include_types(s, v):
            s.input.include_types.clear()
            s.input.include_types.extend(value_parsers.parse_id_list(v))

        def set_exclude_types(s, v):
            s.input.exclude_types.clear()
            s.input.exclude_types.extend(value_parsers.parse_id_list(v))

        return [
            ArgumentDefinition("--include-types", "Semicolon list of format ids to keep",
                               ArgumentArity.SINGLE_VALUE, set_include_types),

            ArgumentDefinition("--exclude-types", "Semicolon list of format ids to drop",
                               ArgumentArity.SINGLE_VALUE, set_exclude_types),
        ]

    @staticmethod
    def walk_arguments() -> List[ArgumentDefinition]:
        """The filesystem walk: --files-from, depth, --follow, --sort, --include-hidden."""
        return [
            ArgumentDefinition("--files-from",
                               "Read input paths from a file, one per line; - for stdin",
                               ArgumentArity.SINGLE_VALUE, lambda s, v: _set(s.input, 'files_from', v)),

            ArgumentDefinition("--max-depth", "Limit walk depth; --max-depth 1 stays in the given directory",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.input, 'max_depth',
                                                 value_parsers.parse_count("--max-depth", v))),

            ArgumentDefinition("--no-recurse", "Don't descend into subdirectories (= --max-depth 1)",
                               ArgumentArity.NO_VALUE,
                               lambda s, v: _set(s.input, 'no_recurse_was_given', True)),

            ArgumentDefinition("-L|--follow", "Follow symlinked directories",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.input, 'follow_symlinks', True)),

            ArgumentDefinition("--sort", "Sort entries before processing",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.input, 'sort', True)),

            ArgumentDefinition("--include-hidden", "Include hidden and system files",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.input, 'include_hidden', True)),
        ]

    @staticmethod
    def password_arguments() -> List[ArgumentDefinition]:
        """-p/--password, --password-file, --password-prompt."""
        return [
            ArgumentDefinition("-p|--password", "A candidate password, repeatable",
                               ArgumentArity.MULTIPLE_VALUE, lambda s, v: s.passwords.passwords.append(v)),

            ArgumentDefinition("--password-file", "One candidate password per line",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.passwords, 'password_file', v)),

            ArgumentDefinition("--password-prompt", "Prompt for passwords interactively",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.passwords, 'prompt', True)),
        ]

    @staticmethod
    def processing_arguments() -> List[ArgumentDefinition]:
        """--options, --codepage, --temp-dir."""
        return [
            ArgumentDefinition("--options",
                               "Semicolon-separated engine options, e.g. BW_MODE=3;CODEPAGE=1252",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.processing, 'engine_options', v)),

            ArgumentDefinition("--codepage", "Default codepage when the document does not declare one",
                               ArgumentArity.SINGLE_VALUE, lambda s, v: _set(s.processing, 'codepage', v)),

            ArgumentDefinition("--temp-dir", "Temp directory for scratch files",
                               ArgumentArity.SINGLE_VALUE, lambda s, v: _set(s.processing, 'temp_dir', v)),
        ]

    @staticmethod
    def batch_arguments() -> List[ArgumentDefinition]:
        """--on-error and --fail-fast."""

        def set_on_error(s, v):
            s.batch.on_error = value_parsers.parse_on_error(value_parsers.fold_token(v))
            s.batch.on_error_was_explicit = True

        def set_fail_fast(s, v):
            # Guarded rather than unconditional: --on-error's own setter always has the
            # last word on on_error/on_error_was_explicit whenever it runs, regardless
            # of which of the two the parsing library applies first, so leaving an
            # explicit value alone here is what makes the pair's resolution independent
            # of argument order rather than of registration order in this table.
            if not s.batch.on_error_was_explicit:
                s.batch.on_error = OnErrorBehaviour.STOP
            s.batch.fail_fast_was_given = True

        return [
            ArgumentDefinition("--on-error", "On a per-file failure: skip (default) or stop the run",
                               ArgumentArity.SINGLE_VALUE, set_on_error),

            ArgumentDefinition("--fail-fast", "Stop at the first failure (= --on-error stop)",
                               ArgumentArity.NO_VALUE, set_fail_fast),
        ]

    @staticmethod
    def console_arguments() -> List[ArgumentDefinition]:
        """--color, --verbose, --silent. Every verb takes these."""
        return [
            ArgumentDefinition("--color", "Colour errors and verbose output; ignored on a redirected stream",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.console, 'color', True)),

            ArgumentDefinition("--verbose", "Print more about what the run is doing",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.console, 'verbose', True)),

            ArgumentDefinition("--silent", "Print nothing but errors",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.console, 'silent', True)),
        ]

    @staticmethod
    def diagnostic_arguments() -> List[ArgumentDefinition]:
        """Timing, limits, tracing and the --debug-* switches."""
        return [
            ArgumentDefinition("--time", "Report how long the run took",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.diagnostics, 'time', True)),

            ArgumentDefinition("--logtime", "Log timing for each stage",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.diagnostics, 'log_time', True)),

            ArgumentDefinition("--logmem", "Sample and report peak process memory use",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.diagnostics, 'log_memory', True)),

            ArgumentDefinition("--progress", "Print a progress indicator",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.diagnostics, 'progress', True)),

            ArgumentDefinition("--pause", "Pause before exiting",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.diagnostics, 'pause', True)),

            ArgumentDefinition("--max-memory", "Abort the run if it exceeds this many megabytes",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.diagnostics, 'max_memory',
                                                 value_parsers.parse_megabytes("--max-memory", v))),

            ArgumentDefinition("--max-time", "Abort the run after this many seconds",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.diagnostics, 'max_time',
                                                 value_parsers.parse_seconds("--max-time", v))),

            ArgumentDefinition("--timeout", "Abort a single document after this many seconds",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.diagnostics, 'timeout',
                                                 value_parsers.parse_seconds("--timeout", v))),

            ArgumentDefinition("-x", "Report engine memory status once, at the end of the run",
                               ArgumentArity.NO_VALUE,
                               lambda s, v: _set(s.diagnostics, 'memory_status_level',
                                                 max(s.diagnostics.memory_status_level, 1))),

            ArgumentDefinition("--xx", "Report engine memory status after every document too",
                               ArgumentArity.NO_VALUE,
                               lambda s, v: _set(s.diagnostics, 'memory_status_level',
                                                 max(s.diagnostics.memory_status_level, 2))),

            ArgumentDefinition("--log-level", "How much the low-level readers log",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.diagnostics, 'log_level', value_parsers.fold_token(v))),

            ArgumentDefinition("--debug-log-level", "--log-level setting under its original name",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.diagnostics, 'debug_log_level',
                                                 value_parsers.fold_token(v))),

            ArgumentDefinition("--log-modules", "Which modules log",
                               ArgumentArity.SINGLE_VALUE, lambda s, v: _set(s.diagnostics, 'log_modules', v)),

            ArgumentDefinition("--debug-log-modules", "--log-modules setting under its original name",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.diagnostics, 'debug_log_modules', v)),

            ArgumentDefinition("--debug-use-memory-stream", "Read each document through a memory stream",
                               ArgumentArity.NO_VALUE,
                               lambda s, v: _set(s.diagnostics, 'use_memory_stream', True)),

            ArgumentDefinition("--debug-measure-io", "Report I/O counts and volumes per document",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s.diagnostics, 'measure_io', True)),

            ArgumentDefinition("--debug-truncate", "Read only the first N bytes of each input",
                               ArgumentArity.SINGLE_VALUE,
                               lambda s, v: _set(s.diagnostics, 'truncate_input_bytes',
                                                 value_parsers.parse_count("--debug-truncate", v))),
        ]

    @staticmethod
    def library_arguments() -> List[ArgumentDefinition]:
        """--license-key and --library. Every verb that touches the engine takes these."""
        return [
            ArgumentDefinition("--license-key", "Document Filters license key; falls back to DF_LICENSE_KEY",
                               ArgumentArity.SINGLE_VALUE, lambda s, v: _set(s.library, 'license_key', v)),

            ArgumentDefinition("--library", "Path to the Document Filters engine; falls back to DF_PATH",
                               ArgumentArity.SINGLE_VALUE, lambda s, v: _set(s.library, 'library_path', v)),
        ]

    @staticmethod
    def global_arguments() -> List[ArgumentDefinition]:
        """
        Arguments that work on every verb and are not part of any shared group:
        --dry-run and --print-contract.

        --dry-run runs stages 1 to 3 and prints the resulting plan without opening a
        document — useful to a user before committing to a long batch, and the
        mechanism the conformance corpus compares. --print-plan is kept as an alias
        because that is the name the design document uses.
        """
        return [
            ArgumentDefinition("--dry-run", "Plan the run and print it, without opening a document",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s, 'dry_run', True)),

            ArgumentDefinition("--print-plan", "Alias for --dry-run",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s, 'dry_run', True)),

            ArgumentDefinition("--print-contract", "Print the whole argument surface as flat text, and stop",
                               ArgumentArity.NO_VALUE, lambda s, v: _set(s, 'print_contract', True)),
        ]

    @staticmethod
    def convert() -> List[ArgumentDefinition]:
        """
        convert's own arguments, including --pages.

        The CLI design lists --pages among the shared options, but convert is the only
        command that renders pages and ConvertOptions.pages is its only destination. A
        shared definition is written against SharedOptions and so could not reach that
        field at all, so it belongs here.
        """
        return [
            ArgumentDefinition("--to", "Target format: text, md, html5, pdf, png, json, xml, tiff ...",
                               ArgumentArity.SINGLE_VALUE,
                               lambda o, v: _set(o, 'format',
                                                 FormatCatalog.resolve(value_parsers.fold_token(v)))),

            ArgumentDefinition("--combine", "Merge every input into one output document",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'combine', True)),

            ArgumentDefinition("--canvas-options", "Override canvas defaults, passed to the engine untouched",
                               ArgumentArity.SINGLE_VALUE, lambda o, v: _set(o, 'canvas_options', v)),

            ArgumentDefinition("--render-options",
                               "Override render-page defaults, passed to the engine untouched",
                               ArgumentArity.SINGLE_VALUE, lambda o, v: _set(o, 'render_options', v)),

            ArgumentDefinition("--no-images", "Skip embedded images during conversion",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'no_images', True)),

            ArgumentDefinition("--include-control-codes", "Keep ISYS control codes in text output",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'include_control_codes', True)),

            ArgumentDefinition("--fingerprint", "Emit page fingerprints",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'fingerprint', True)),

            ArgumentDefinition("--watermark", "Stamp each page; a file path or a literal string",
                               ArgumentArity.SINGLE_VALUE, lambda o, v: _set(o, 'watermark', v)),

            ArgumentDefinition("--redact", "Black out a region: page,x,y,w,h",
                               ArgumentArity.MULTIPLE_VALUE,
                               lambda o, v: o.redactions.append(value_parsers.parse_redaction(v))),

            ArgumentDefinition("--form", "Set a form field value: page,name=value",
                               ArgumentArity.MULTIPLE_VALUE,
                               lambda o, v: o.form_values.append(value_parsers.parse_form_value(v))),

            ArgumentDefinition("--view", "Open the result when done",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'view', True)),

            ArgumentDefinition("--fonts", "Report fonts referenced in the document",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'fonts', True)),

            ArgumentDefinition("--font-mappings", "Report the document's font mappings",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'font_mappings', True)),

            ArgumentDefinition("--page-fonts", "Report fonts per page",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'page_fonts', True)),

            ArgumentDefinition("--page-font-mappings", "Report font mappings per page",
                               ArgumentArity.NO_VALUE,
                               lambda o, v: _set(o.reports, 'page_font_mappings', True)),

            ArgumentDefinition("--dimensions", "Report each page's dimensions",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'dimensions', True)),

            ArgumentDefinition("--page-elements", "Report each page's page elements",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'page_elements', True)),

            ArgumentDefinition("--form-elements", "Report each page's form elements",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'form_elements', True)),

            ArgumentDefinition("--hyperlinks", "Report each page's hyperlink elements",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'hyperlinks', True)),

            ArgumentDefinition("--bookmarks", "Report the document's bookmarks",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'bookmarks', True)),

            ArgumentDefinition("--annotations", "Report the document's annotations",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o.reports, 'annotations', True)),

            ArgumentDefinition("--pages", "Page ranges to render, e.g. 1,3-5,8",
                               ArgumentArity.SINGLE_VALUE,
                               lambda o, v: _set(o, 'pages', PageRange.parse(v))),
        ]

    @staticmethod
    def extract() -> List[ArgumentDefinition]:
        return [
            ArgumentDefinition("-i|--images", "Also pull embedded images, not just subfiles",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'images', True)),

            ArgumentDefinition("--include", "Keep subfiles whose name matches this glob, repeatable",
                               ArgumentArity.MULTIPLE_VALUE, lambda o, v: o.include_patterns.append(v)),

            ArgumentDefinition("--exclude", "Drop subfiles whose name matches this glob, repeatable",
                               ArgumentArity.MULTIPLE_VALUE, lambda o, v: o.exclude_patterns.append(v)),

            ArgumentDefinition("--make-dirs", "Replicate the source tree under the output directory",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'make_dirs_was_given', True)),

            ArgumentDefinition("--flat", "Put every extracted file straight into the output directory",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'flat_was_given', True)),

            ArgumentDefinition("--sub-id", "Append the subfile id to extracted filenames",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'append_subfile_id', True)),
        ]

    @staticmethod
    def list() -> List[ArgumentDefinition]:
        return [
            ArgumentDefinition("--json", "Machine-readable listing",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'json', True)),
        ]

    @staticmethod
    def identify() -> List[ArgumentDefinition]:
        return [
            ArgumentDefinition("--json", "Machine-readable output",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'json', True)),

            ArgumentDefinition("--capabilities", "Also print the format's capability flags",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'capabilities', True)),
        ]

    @staticmethod
    def meta() -> List[ArgumentDefinition]:
        return [
            ArgumentDefinition("--json", "Metadata as JSON instead of plain text",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'json', True)),
        ]

    @staticmethod
    def formats() -> List[ArgumentDefinition]:
        return [
            ArgumentDefinition("--json", "Machine-readable catalog",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'json', True)),

            ArgumentDefinition("--filter", "Only formats whose id or name matches",
                               ArgumentArity.SINGLE_VALUE, lambda o, v: _set(o, 'filter', v)),
        ]

    @staticmethod
    def option_catalog() -> List[ArgumentDefinition]:
        return [
            ArgumentDefinition("--json",
                               "Full catalog: name, description, default, type, appliesTo",
                               ArgumentArity.NO_VALUE, lambda o, v: _set(o, 'json', True)),

            ArgumentDefinition("--filter", "Only options whose name or description matches",
                               ArgumentArity.SINGLE_VALUE, lambda o, v: _set(o, 'filter', v)),

            ArgumentDefinition("--applies-to", "Filter by scope: document, page, canvas, render ...",
                               ArgumentArity.SINGLE_VALUE,
                               lambda o, v: _set(o, 'applies_to', value_parsers.fold_token(v))),
        ]

    @staticmethod
    def describe_command(command: str) -> List[ArgumentDefinition]:
        """
        Every argument on a given verb — shared plus command-specific — for the contract
        dump and the coverage tests.
        """
        result: List[ArgumentDefinition] = []
        if command == "convert":
            result.extend(ArgumentTable.convert())
        elif command == "extract":
            result.extend(ArgumentTable.extract())
        elif command == "list":
            result.extend(ArgumentTable.list())
        elif command == "identify":
            result.extend(ArgumentTable.identify())
        elif command == "meta":
            result.extend(ArgumentTable.meta())
        elif command == "formats":
            result.extend(ArgumentTable.formats())
        elif command == "options":
            result.extend(ArgumentTable.option_catalog())
        else:
            raise ValueError("unknown command '" + command + "'")
        result.extend(ArgumentTable.shared_for(command))
        return result
