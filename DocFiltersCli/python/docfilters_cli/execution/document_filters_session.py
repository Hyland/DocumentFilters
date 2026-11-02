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

import os
from typing import TYPE_CHECKING, BinaryIO, Callable, Optional

from ..configuration.shared_options import LibraryOptions
from ..engine.engine_exception import EngineException
from ..engine.i_document_filters_engine import IDocument, IDocumentFiltersEngine
from ..runtime.cli_error import InitialisationError
from .engine_log import EngineLog
from .password_provider import PasswordProvider

if TYPE_CHECKING:
    # A type annotation and nothing else, so it lives under TYPE_CHECKING and the
    # annotations that use it are quoted. Execution has one import cycle --
    # ExtractorOpenSettings calls CanvasConversion, CanvasConversion holds a
    # DocumentFiltersSession, and this file's open_document takes an
    # ExtractorOpenSettings -- which costs C# nothing inside one assembly and in Python
    # would mean a module running before the module it needs has finished defining its
    # class. That cycle is already broken at its one live-call edge, by the deferred
    # import inside extractor_open_settings.for_format; importing the same module here
    # at module scope would put the cycle straight back. Under TYPE_CHECKING the import
    # never runs, so this file names the type without depending on it at import time.
    from .extractor_open_settings import ExtractorOpenSettings


class DocumentFiltersSession:
    """
    Owns the one DocumentFilters instance for the whole run.

    The engine is expensive to create and is designed to be initialised once for the
    lifetime of the process, so this is created lazily -- the first time a command actually
    needs to open something -- and closed at the end. --print-plan therefore never needs a
    license key.

    The C# constructor is private, and the two ways in are the For and Create statics.
    Python has no private constructor, so __init__ is what For does and for_engine is kept
    beside it anyway: it is the name the other three ports use, and it says at the call site
    which of the two routes a caller took.
    """

    def __init__(self, engine: IDocumentFiltersEngine):
        self._engine = engine

    @staticmethod
    def for_engine(engine: IDocumentFiltersEngine) -> 'DocumentFiltersSession':
        """
        Wraps an engine that is already initialised.

        How the tests put a fake engine behind the execution paths. The production route is
        create, which builds the real one.

        Named for_engine rather than for, which is a Python keyword -- the same rename
        engine_log.for_options and debug_input.for_document make.
        """
        return DocumentFiltersSession(engine)

    @staticmethod
    def create(options: LibraryOptions, temp_dir: Optional[str] = None) -> 'DocumentFiltersSession':
        """
        Initialises the engine.

        Raises:
            InitialisationError: The native library could not be found, or the license was
                rejected.
        """
        # The one import in this project that must not happen at module scope. Everything
        # above the seam is importable without the DocumentFilters package -- that is what
        # lets the pure suite run from a clone with no native library -- and this is the
        # only file above the seam that names the concrete engine at all. Importing
        # document_filters_engine does not itself import the binding, whose import is lazy
        # in _bind(), but constructing DocumentFiltersEngine does, so the import is
        # deferred here to keep the two facts in one place.
        from ..engine.document_filters_engine import DocumentFiltersEngine

        # C#'s ?? tests for null only, so an explicitly empty --license-key or --library
        # wins over the environment there. `x if x is not None else y` reproduces that; `or`
        # would not, because it also falls through on an empty string.
        license_key = (options.license_key if options.license_key is not None
                       else os.environ.get('DF_LICENSE_KEY'))
        library_path = DocumentFiltersSession.resolve_library_path(
            options, os.environ.get('DF_PATH'), _base_directory())

        # ImportError is caught alongside EngineException, which the C# has no equivalent
        # of. There `new DocumentFiltersEngine()` needs Hyland.DocumentFilters.dll, and a
        # missing assembly escapes Create as a FileNotFoundException rather than as the
        # InitialisationError this method documents -- a hole that is nearly unreachable for
        # a published .NET tool, since the assembly ships beside it. In Python the binding
        # is an ordinary pip dependency that a user can simply not have installed, so the
        # same hole is the common case, and an uncaught traceback is not what "the native
        # library could not be found" is supposed to look like. Same error code and same
        # message prefix either way, so nothing about the contract moves.
        try:
            engine = DocumentFiltersEngine()
            engine.initialize(license_key, library_path)
        except ImportError as error:
            raise InitialisationError("could not initialise Document Filters: " + str(error))
        except EngineException as error:
            raise InitialisationError("could not initialise Document Filters: " + error.message)

        # --temp-dir, after initialize because it is a setting on an initialised
        # library rather than an argument to starting one.
        #
        # Plain truthiness is string.IsNullOrEmpty exactly, for a string: None and '' are
        # the only falsy values an Optional[str] has. Deliberately not is_all_whitespace,
        # which is the wider IsNullOrWhiteSpace the reference did not ask for here.
        if temp_dir:
            engine.set_temp_path(temp_dir)

        return DocumentFiltersSession(engine)

    @staticmethod
    def resolve_library_path(options: LibraryOptions, environment_value: Optional[str],
                             base_directory: str) -> str:
        """
        --library, then DF_PATH, then the directory this tool's own executable lives in --
        never the process's current directory.

        This is also where DocumentFilters.Initialize looks for fonts (ISYS_INIT_PATH/fonts,
        per the binding's own doc comment on the parameter), which is where the
        Hyland.DocumentFilters.Fonts package copies its bundled fonts.ini and TTFs at build
        time -- next to the executable, not next to whatever the caller's shell happens to
        be sitting in. doc2text and the other published samples default this to "." and get
        away with it only because they are always run from their own build output directory;
        this tool is invoked from wherever the user's documents are, so "." there resolves
        to a directory with no fonts at all. The engine still renders in that case -- it
        does not fail to initialise -- but falls back to a font it cannot rasterise glyphs
        from, so text comes out blank instead of missing. base_directory is threaded in
        rather than read here so this stays a pure function of its inputs.

        Two None checks rather than `or`: C#'s ?? chain falls through on null and not on an
        empty string, so `--library ""` resolves to the empty string in both ports rather
        than silently picking up DF_PATH.
        """
        if options.library_path is not None:
            return options.library_path
        if environment_value is not None:
            return environment_value
        return base_directory

    @property
    def api(self) -> IDocumentFiltersEngine:
        """The initialised engine, for the execution helpers."""
        return self._engine

    def open_document(self, path: str, open: 'ExtractorOpenSettings', passwords: PasswordProvider,
                      should_cancel: Optional[Callable[[], bool]] = None,
                      log: Optional[EngineLog] = None) -> IDocument:
        """
        Opens one document with the given flags and options, wiring up the password
        callback.

        The open settings are passed in rather than worked out here, because the engine
        applies them during Open: the format-specific bits for flow HTML or text-mode
        markdown have no effect once the document is already open. Each conversion path
        resolves its extractor_open_settings.ExtractorOpenSettings first, and the document
        is opened exactly once.

        Args:
            should_cancel: Polled while the engine parses; true stops the open. None for a
                caller with no deadline to enforce.
        """
        # `open` keeps the reference's parameter name, shadowing the builtin for the body of
        # this method only -- which nothing here calls, and which the same trade already
        # made for `format` in extractor_open_settings.for_format. A parameter name is
        # visible in a keyword call, so the four ports agree on it.
        #
        # No casts around the two log callbacks. The C# needs (Func<string, int>) and
        # (Action<int, string, string>) purely to give its conditional operator a common
        # type; a bound method is already the callable the seam asks for.
        return self._engine.open_document(path, open.flags, open.options,
                                          lambda: passwords.get_password(path), should_cancel,
                                          None if log is None else log.level_for,
                                          None if log is None else log.write)

    def open_document_from_stream(self, source: BinaryIO, display_name: str,
                                  open: 'ExtractorOpenSettings', passwords: PasswordProvider,
                                  should_cancel: Optional[Callable[[], bool]] = None,
                                  log: Optional[EngineLog] = None) -> IDocument:
        """
        The same, from a stream -- for the debug switches that change how bytes reach the
        engine.

        C# spells this as a second OpenDocument overload; Python has none, so the name
        carries the difference, with the _from_stream suffix the seam already uses for the
        engine call this forwards to.
        """
        return self._engine.open_document_from_stream(source, display_name, open.flags, open.options,
                                                      lambda: passwords.get_password(display_name),
                                                      should_cancel,
                                                      None if log is None else log.level_for,
                                                      None if log is None else log.write)

    def describe_version(self) -> str:
        """
        Version and build information, for the version command.

        The native build number doc2text prints comes from reading each engine DLL's own
        file version directly, which the managed binding does not expose; this reports the
        binding's own package version instead.
        """
        return self._engine.describe_version()

    def close(self) -> None:
        """Releases the engine. C#'s Dispose."""
        self._engine.close()

    def __enter__(self) -> 'DocumentFiltersSession':
        return self

    def __exit__(self, exception_type, exception_value, traceback) -> None:
        self.close()


def _base_directory() -> str:
    """
    What AppContext.BaseDirectory means for this port: the directory this tool's own code
    lives in.

    Python has no AppContext, and sys.argv[0] is not a stand-in for it -- it is the module
    file under `python -m`, a launcher script when the package is installed as a console
    script, and the literal '-c' or '' when there is no script at all, where its directory
    would resolve to the current directory. The current directory is precisely what the
    reference's fallback exists to avoid, so this is derived from __file__ instead, which
    names the tool's own package wherever it was installed from. A guess as to which
    directory a future font bundle would sit in, but a stable one, and kept in one function
    so that decision has one place to change.

    Two differences from the C# value are worth knowing about, and neither is this
    function's to fix:

      * AppContext.BaseDirectory ends with a directory separator and this does not. The
        engine has to tolerate both already, since a --library or DF_PATH value a user types
        does not end with one either.
      * The .NET binding takes one path and uses it for both jobs the directory does,
        because the managed assembly sits beside the native library it loads. The Python
        binding splits them -- Initialize(license, resource_path, dll_path) -- and a
        pip-installed binding keeps the engine libraries inside its own package rather
        than beside this one, so a directory that is right for fonts can be wrong for
        libraries. That asymmetry belongs to the binding and is settled in the binding's
        adapter: document_filters_engine._dll_path passes this value on as dll_path only
        when a library really is in it, so a default that holds no library leaves the
        binding to find its own bundled copy. This function answers the resource_path
        question, which is the one D33 is about, and answers it the same way in all four
        ports.

    os.path rather than the string slicing the output-path rules require: this is a host
    path handed to the engine, exactly as AppContext.BaseDirectory is, and never text this
    tool prints -- so the reason that rule exists, that two hosts must not produce different
    output for the same run, does not apply.
    """
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
