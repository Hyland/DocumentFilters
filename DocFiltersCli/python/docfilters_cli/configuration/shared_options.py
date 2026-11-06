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

from enum import Enum
from typing import List, Optional


class OnErrorBehaviour(Enum):
    """Values for --on-error."""

    SKIP = 0
    """Continue with the next file; still exit non-zero at the end."""

    STOP = 1
    """Stop the run at the first failure."""


class InputOptions:
    """Which inputs to process, and how to find them."""

    def __init__(self):
        self.paths: List[str] = []
        """Positional paths: files or directories, in the order given."""

        self.files_from: Optional[str] = None
        """--files-from <file>; "-" means read from stdin."""

        # --no-recurse deliberately does *not* write this field. Two arguments
        # sharing one field destroys the information validation needs: if
        # --no-recurse wrote 1 here, then "--max-depth 5 --no-recurse" would be
        # indistinguishable from a plain "--max-depth 1", and the contradiction
        # could never be reported. Each argument records its own presence;
        # effective_max_depth derives the answer.
        self.max_depth: Optional[int] = None
        """--max-depth. None means it was not given; directories are walked recursively."""

        self.no_recurse_was_given = False
        """--no-recurse, which means a depth of 1."""

        self.follow_symlinks = False
        """-L, --follow: enter symlinked directories."""

        self.sort = False
        """--sort: order entries before processing, for reproducible runs."""

        self.include_hidden = False
        """--include-hidden."""

        # Kept apart from regex_patterns rather than both compiling into matcher
        # directly: two arguments writing one field would let
        # -g "**/*.pdf" --regex ".*\\.docx" silently drop one of the two, and the
        # validator could not see that both were given.
        self.glob_patterns: List[str] = []
        """-g/--glob, repeatable. A leading ! excludes."""

        self.regex_patterns: List[str] = []
        """--regex, repeatable."""

        # Populated by CommandLineApp.bind. OptionsValidator is where the two pattern
        # languages are rejected if both appear, but it does not build this itself.
        # Reading it before binding gives None.
        self.matcher = None
        """The compiled NameMatcher. None means everything matches."""

        self.include_types: List[str] = []
        """--include-types: format ids to keep. Empty means no restriction."""

        self.exclude_types: List[str] = []
        """--exclude-types: format ids to drop."""

    @property
    def effective_max_depth(self) -> Optional[int]:
        """
        The depth actually used by the walk, once the two arguments above are
        reconciled. None means unlimited.
        """
        return 1 if self.no_recurse_was_given else self.max_depth


class OutputOptions:
    """Where output goes. See planning.output_planner for the rules."""

    def __init__(self):
        self.out_dir: Optional[str] = None
        """-o, --out-dir. Safe for every format."""

        self.out_file: Optional[str] = None
        """--out-file. Exactly one artifact at exactly this path."""

        self.name_template = None
        """--out-name: OutputNameTemplate used inside out_dir."""

        self.use_stdout = False
        """--stdout, or --out-file -."""

        self.overwrite = False
        """--overwrite: replace existing outputs instead of erroring."""


class SubfileOptions:
    """Recursion into documents embedded in other documents."""

    def __init__(self):
        self.enabled = False
        """-s, --subfiles."""

        self.max_depth: Optional[int] = None
        """--subfile-depth. None means unlimited. Entirely separate from
        InputOptions.max_depth, which is directory depth."""


class PasswordOptions:
    """How to open password-protected documents."""

    def __init__(self):
        self.passwords: List[str] = []
        """-p, --password, repeatable."""

        self.password_file: Optional[str] = None
        """--password-file: one candidate password per line."""

        self.prompt = False
        """--password-prompt: ask interactively. Requires a terminal."""


class ProcessingOptions:
    """Engine tunables and text handling."""

    def __init__(self):
        self.engine_options: Optional[str] = None
        """--options: the semicolon-separated string, passed to the engine
        untouched. The CLI does not parse or validate it — run the options
        command to see the valid keys."""

        self.codepage: Optional[str] = None
        """--codepage: default codepage when the document does not declare one."""

        self.temp_dir: Optional[str] = None
        """--temp-dir."""


class BatchOptions:
    """What happens when one file in a batch fails."""

    def __init__(self):
        self.on_error = OnErrorBehaviour.SKIP
        """--on-error, also written by --fail-fast. Skipping still exits
        non-zero if anything failed."""

        # Recorded so that a contradictory pair — --fail-fast --on-error skip —
        # can be rejected as a usage error rather than silently resolved. None of
        # the four parsing libraries reliably records which of two different
        # options came first on the command line, so a precedence rule could not
        # have been made identical across the ports.
        self.on_error_was_explicit = False
        """True when --on-error was given explicitly."""

        self.fail_fast_was_given = False
        """True when --fail-fast was given."""


class ConsoleOptions:
    """Terminal output verbosity."""

    def __init__(self):
        self.color = False
        """--color."""

        self.verbose = False
        """--verbose."""

        self.silent = False
        """--silent."""


class DiagnosticOptions:
    """
    Timing, limits and debugging aids. Their output is not part of the
    cross-language contract, because timings and memory figures vary per run.
    """

    def __init__(self):
        self.time = False
        self.log_time = False
        self.log_memory = False
        self.progress = False
        self.pause = False

        self.max_memory: Optional[int] = None
        """--max-memory, in megabytes. None means no limit."""

        self.max_time: Optional[int] = None
        """--max-time, in seconds, for the whole run."""

        self.timeout: Optional[int] = None
        """--timeout, in seconds, per document."""

        # 0 = never, the default; 1 = -x, once at the end of the run; 2 = --xx,
        # after every document and at the end of the run. A level rather than two
        # switches, so --xx implies -x without the two needing to be reconciled.
        #
        # Distinct from --logmem, which samples the process's own memory use while
        # a document is being processed and reports the peak. This reports what the
        # engine says about its allocations at a moment in time.
        self.memory_status_level = 0
        """How often to report the engine's memory status, from IGR_Get_Memory_Status."""

        # Wired to the engine's log callbacks, which makes this the one diagnostic
        # here a customer is likely to reach for — it is the fastest way to find
        # out why a particular document will not parse.
        self.log_level: Optional[str] = None
        """--log-level (or --debug-log-level): how much the low-level readers log.
        One of none, crit, error, warn, info, debug, trace."""

        # Kept in its own field rather than both aliases writing log_level: they
        # carry a value, so two arguments sharing one field would let
        # "--log-level trace --debug-log-level info" silently drop one of the two.
        # effective_log_level derives the answer, and the validator rejects the
        # pair when they disagree.
        self.debug_log_level: Optional[str] = None
        """--debug-log-level: the same setting under its original name."""

        self.log_modules: Optional[str] = None
        """--log-modules: which modules log."""

        self.debug_log_modules: Optional[str] = None
        """--debug-log-modules: the same setting under its original name."""

        self.use_memory_stream = False
        """--debug-use-memory-stream: read the document through a memory stream."""

        self.measure_io = False
        """--debug-measure-io: report I/O counts and volumes per document."""

        self.truncate_input_bytes: Optional[int] = None
        """--debug-truncate <bytes>: read only the first N bytes of each input, for
        testing how gracefully a format handles a damaged or partial file."""

    @property
    def effective_log_level(self) -> Optional[str]:
        """The log level actually used, from whichever alias was given."""
        return self.log_level if self.log_level is not None else self.debug_log_level

    @property
    def effective_log_modules(self) -> Optional[str]:
        """The module list actually used, from whichever alias was given."""
        return self.log_modules if self.log_modules is not None else self.debug_log_modules


class LibraryOptions:
    """
    How to find and license the Document Filters engine.

    Not in the CLI design document, but nothing runs without them. Both fall back
    to the environment variables the other published samples already use.
    """

    def __init__(self):
        self.license_key: Optional[str] = None
        """--license-key, falling back to DF_LICENSE_KEY."""

        self.library_path: Optional[str] = None
        """--library, falling back to DF_PATH, then to this tool's own directory —
        see execution.document_filters_session.resolve_library_path. None here means
        neither argument nor environment variable was given, not that there is
        nowhere to look."""


class SharedOptions:
    """
    Everything the shared options resolved to. One instance per run, filled in by
    the argument table's setters, then checked by OptionsValidator.

    Two arguments never share one field. Where two arguments describe the same
    setting, each records its own presence and the setting is derived from both —
    --max-depth plus no_recurse_was_given gives InputOptions.effective_max_depth,
    --log-level plus --debug-log-level gives DiagnosticOptions.effective_log_level.
    Collapsing a pair into one field makes a contradiction indistinguishable from a
    plain restatement, so it could never be reported — and reporting it is the point,
    because argv order is not portable enough to resolve such a pair by precedence.

    Defaults live here as field initialisers, never in the parser declarations —
    four declarations of the same default drift, one does not.
    """

    def __init__(self):
        self.input = InputOptions()
        self.output = OutputOptions()
        self.subfiles = SubfileOptions()
        self.passwords = PasswordOptions()
        self.processing = ProcessingOptions()
        self.batch = BatchOptions()
        self.console = ConsoleOptions()
        self.diagnostics = DiagnosticOptions()
        self.library = LibraryOptions()

        # Two arguments write this one field, which is safe here where it would not
        # be elsewhere: both set it to True, so giving both is redundant rather than
        # contradictory and there is nothing to reconcile. Aliases that carry a
        # *value* — --log-level and --debug-log-level — get a field each instead.
        self.dry_run = False
        """--dry-run, or its alias --print-plan: run stages 1 to 3, print the
        resulting plan, and stop without opening a document."""

        self.print_contract = False
        """--print-contract: emit the whole argument surface as flat text and stop.
        A debugging aid, and the same information help --json returns."""

    def to_canonical_string(self, include_subfiles: bool = True) -> str:
        """
        Renders the options in the canonical form used by the conformance corpus:
        the walk, match, types, subfiles and on-error lines of a --print-plan dump,
        in that order, one per line, with no trailing newline.

        That order is contiguous exactly as written, for every command that has
        these fields: a command's own fields — convert's pages; extract's
        include/exclude/images/make-dirs/sub-id; list's and meta's json; every
        command's cardinality — always come after this whole block, never inside
        it, so a caller appends this string and then its own lines.

        All four language implementations must produce byte-identical output for
        each line.
        """
        parts = []

        depth = self.input.effective_max_depth
        parts.append(
            "walk depth=" + ("unlimited" if depth is None else str(depth))
            + " follow=" + ("yes" if self.input.follow_symlinks else "no")
            + " sort=" + ("yes" if self.input.sort else "no")
            + " hidden=" + ("yes" if self.input.include_hidden else "no")
            + "\n")

        parts.append("match " + (str(self.input.matcher) if self.input.matcher is not None else "none") + "\n")

        parts.append("types include=" + self._format_id_list(self.input.include_types)
                     + " exclude=" + self._format_id_list(self.input.exclude_types)
                     + "\n")

        if include_subfiles:
            if self.subfiles.enabled:
                subfile_depth = self.subfiles.max_depth
                parts.append("subfiles enabled depth="
                             + ("unlimited" if subfile_depth is None else str(subfile_depth)))
            else:
                parts.append("subfiles disabled")
            parts.append("\n")

        parts.append("on-error " + ("stop" if self.batch.on_error == OnErrorBehaviour.STOP else "skip"))

        return ''.join(parts)

    @staticmethod
    def _format_id_list(ids: List[str]) -> str:
        return "none" if not ids else ','.join(ids)
