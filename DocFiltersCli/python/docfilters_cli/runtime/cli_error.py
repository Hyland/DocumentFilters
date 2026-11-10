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

from . import exit_code


class ErrorCodes:
    """
    Stable machine-readable identifiers for every error this tool can report.

    These strings appear in the conformance corpus, so all four language
    implementations must raise the same code for the same situation. The
    human-readable message is also part of the contract; it is authored here
    rather than left to the CLI parsing library, so that it can be identical
    everywhere.
    """

    UNKNOWN_COMMAND = "E_UNKNOWN_COMMAND"
    UNKNOWN_ARGUMENT = "E_UNKNOWN_ARGUMENT"
    MISSING_VALUE = "E_MISSING_VALUE"
    MISSING_ARGUMENT = "E_MISSING_ARGUMENT"
    BAD_VALUE = "E_BAD_VALUE"
    BAD_PAGE_RANGE = "E_BAD_PAGE_RANGE"
    BAD_TEMPLATE = "E_BAD_TEMPLATE"
    UNKNOWN_FORMAT = "E_UNKNOWN_FORMAT"
    MUTUALLY_EXCLUSIVE = "E_MUTUALLY_EXCLUSIVE"
    COMBINE_UNSUPPORTED = "E_COMBINE_UNSUPPORTED"
    STDOUT_NEEDS_SINGLE_FILE = "E_STDOUT_NEEDS_SINGLE_FILE"
    OUTFILE_NEEDS_SINGLE_FILE = "E_OUTFILE_NEEDS_SINGLE_FILE"
    OUTNAME_NEEDS_OUTDIR = "E_OUTNAME_NEEDS_OUTDIR"
    TEMPLATE_NEEDS_PAGE = "E_TEMPLATE_NEEDS_PAGE"
    REPORT_NEEDS_HIDEF = "E_REPORT_NEEDS_HIDEF"
    SUBFILE_DEPTH_NEEDS_SUBFILES = "E_SUBFILE_DEPTH_NEEDS_SUBFILES"
    OUTFILE_MULTI_INPUT = "E_OUTFILE_MULTI_INPUT"
    OUTPUT_EXISTS = "E_OUTPUT_EXISTS"
    OUTPUT_COLLISION = "E_OUTPUT_COLLISION"
    UNSAFE_OUTPUT = "E_UNSAFE_OUTPUT"
    OUTPUT_IS_INPUT = "E_OUTPUT_IS_INPUT"
    NO_INPUTS = "E_NO_INPUTS"
    INIT_FAILED = "E_INIT_FAILED"
    LIMIT_EXCEEDED = "E_LIMIT_EXCEEDED"
    DOCUMENT_FAILED = "E_DOCUMENT_FAILED"
    INTERRUPTED = "E_INTERRUPTED"

    @classmethod
    def all_codes(cls):
        """
        Every code above, so that the tool's error vocabulary can be enumerated
        without reflection.

        A hand-written list beside the declarations, in the same idiom as
        ArgumentTable.commands, and for the same reason: C++17 has no
        reflection, and the Python, Java and C++ ports translate these declarations
        rather than rewriting them.

        ErrorCodeTests checks it against the fixtures in both directions, so a
        code declared here that no case provokes fails, and so does a case naming a
        code that is not here.
        """
        return [
            cls.UNKNOWN_COMMAND, cls.UNKNOWN_ARGUMENT, cls.MISSING_VALUE, cls.MISSING_ARGUMENT,
            cls.BAD_VALUE, cls.BAD_PAGE_RANGE, cls.BAD_TEMPLATE, cls.UNKNOWN_FORMAT,
            cls.MUTUALLY_EXCLUSIVE, cls.COMBINE_UNSUPPORTED, cls.STDOUT_NEEDS_SINGLE_FILE,
            cls.OUTFILE_NEEDS_SINGLE_FILE, cls.OUTNAME_NEEDS_OUTDIR, cls.TEMPLATE_NEEDS_PAGE,
            cls.REPORT_NEEDS_HIDEF, cls.SUBFILE_DEPTH_NEEDS_SUBFILES, cls.OUTFILE_MULTI_INPUT, cls.OUTPUT_EXISTS,
            cls.OUTPUT_COLLISION, cls.UNSAFE_OUTPUT, cls.OUTPUT_IS_INPUT, cls.NO_INPUTS, cls.INIT_FAILED, cls.LIMIT_EXCEEDED,
            cls.DOCUMENT_FAILED, cls.INTERRUPTED
        ]


class CliError(Exception):
    """
    Base class for every error the CLI reports to the user. Thrown from stages
    2 to 4 and caught once, in program.py.
    """

    def __init__(self, code, message, path=None):
        super().__init__(message)
        self.code = code
        self.message = message
        self.path = path

    @property
    def exit_code(self):
        """The process exit code this error produces."""
        raise NotImplementedError("Subclass must implement exit_code")


class UsageError(CliError):
    """Bad command line: unknown argument or verb, missing or malformed value."""

    @property
    def exit_code(self):
        return exit_code.USAGE_ERROR


class NoInputsError(CliError):
    """The walk plus filters selected nothing to process."""

    def __init__(self, message):
        super().__init__(ErrorCodes.NO_INPUTS, message)

    @property
    def exit_code(self):
        return exit_code.NO_INPUTS_MATCHED


class OutputConflictError(CliError):
    """An output target cannot be resolved without guessing, or already exists."""

    @property
    def exit_code(self):
        return exit_code.OUTPUT_CONFLICT


class InitialisationError(CliError):
    """Document Filters could not be initialised."""

    def __init__(self, message):
        super().__init__(ErrorCodes.INIT_FAILED, message)

    @property
    def exit_code(self):
        return exit_code.INITIALISATION_FAILURE


class LimitExceededError(CliError):
    """A run limit was exceeded."""

    def __init__(self, message, path=None):
        """
        Args:
            message: Description of which limit was hit.
            path: The input the limit applied to, for --timeout; None for
                  --max-time and --max-memory, which bound the whole run.
        """
        super().__init__(ErrorCodes.LIMIT_EXCEEDED, message, path)

    @property
    def exit_code(self):
        return exit_code.LIMIT_EXCEEDED


class DocumentError(CliError):
    """
    A single document failed. Collected by BatchRunner rather than
    ending the run, unless --on-error stop is in effect.
    """

    def __init__(self, path, message, inner_error=None):
        super().__init__(ErrorCodes.DOCUMENT_FAILED, message, path)
        self.inner_error = inner_error

    @property
    def exit_code(self):
        return exit_code.PARTIAL_FAILURE
