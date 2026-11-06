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

import ctypes
import enum
import os
import re
import sys
from typing import BinaryIO, Callable, List, Optional

from .engine_exception import EngineError, EngineException
from .i_document_filters_engine import (AnnotationKind, EngineAnnotation, EngineBookmark,
                                        EngineFileType, EngineFormat, EngineFormElement,
                                        EngineHyperlink, EngineOption, EnginePageDecorations,
                                        EnginePageElement, EngineRectangle, FormElementType,
                                        HyperlinkActionType, IDocument, IDocumentFiltersEngine,
                                        IDocumentPage, IOutputCanvas, ISubFileEntry,
                                        PageElementType)
from ..planning.format_spec import CanvasType

# -------------------------------------------------------------------------
#  THE REAL ENGINE
#
#  The only file in the project that touches the DocumentFilters binding. Every
#  method here is one binding call plus, where the binding hands back a type the
#  seam does not describe, a copy into the plain values from
#  i_document_filters_engine.
#
#  Nothing here decides anything. If a method in this file grows an 'if' that is
#  not null-handling or error translation, it belongs above the seam.
#
#  THE IMPORT IS LAZY, AND THAT IS THE POINT
#
#  Importing this module must not require the DocumentFilters package, because
#  program.py imports the module that chooses an engine, and the pure suite runs
#  from a clone that has no native library and no binding installed. So the import
#  lives in _bind() rather than at the top of the file. Merely importing the
#  binding does not load the native library — ctypes is only asked for it on the
#  first call that needs a handle — but the import itself still fails when the
#  package is absent, and that is enough to take the suite down.
#
#  Constructing DocumentFiltersEngine does need the binding, exactly as the C#
#  needs Hyland.DocumentFilters.dll to construct one. Importing the module does
#  not.
# -------------------------------------------------------------------------

_binding = None
"""The DocumentFilters package once imported, cached so the import cost is paid once."""


def _bind():
    """
    The binding module, imported on first use.

    sys.modules already caches the import, so the module-level name is not strictly
    necessary; it is kept because _translate() has to ask whether the binding has
    been loaded *without* trying to load it, and a None check here is the only way
    to answer that.
    """
    global _binding

    if _binding is None:
        import DocumentFilters

        _binding = DocumentFilters

    return _binding


def _api_type():
    """
    The binding's DocumentFilters class.

    Reached through the package rather than imported by name because the binding
    names its package, its main module and its principal class all DocumentFilters:
    `import DocumentFilters` gives the package, whose DocumentFilters attribute the
    package's own __init__ rebinds from the submodule to the class. Going through
    this one accessor keeps that piece of trivia in a single place.
    """
    return _bind().DocumentFilters


class _FormatAttribute(enum.IntEnum):
    """
    The two IGR_FORMAT_* selectors this file asks a format for.

    The C# names IGRFormatWhat.IGR_FORMAT_MIMETYPE and IGR_FORMAT_LONG_NAME; the
    Python binding publishes the same two numbers as module-level ints, spelling the
    first IGR_FORMAT_MIME_TYPE. They are declared here rather than imported for the
    same reason the seam declares its own: an import would have to run at module
    import time, which is exactly what the lazy binding is arranged to avoid. The
    numbers are ABI and cannot drift, which is what makes that safe.

    Two members rather than all seven, because a partial copy of a constant table is
    worse than a minimal one — anyone adding a third selector will look it up.
    """

    LONG_NAME = 0
    MIME_TYPE = 5


class _OptionAttribute(enum.IntEnum):
    """
    IGR_Get_Option_Attribute selectors, all six of them.

    Neither binding names these; both hard-code 0 to 5 in the loop that reads an
    option row. Named here because get_available_options has to make the same calls
    itself — see the note there. POSSIBLE_VALUES is unused, because the seam's
    EngineOption does not carry it; it is listed so the table is complete.
    """

    NAME = 0
    DESCRIPTION = 1
    DEFAULT_VALUE = 2
    FLAGS = 3
    TYPE = 4
    POSSIBLE_VALUES = 5


_REDACTION_BLACK = 0xFF000000
"""
Opaque black, as the engine's redaction colour word.

The C# passes System.Drawing.Color.Black and the .NET binding packs it as
(A << 24) | (R << 16) | (G << 8) | B, which for black is this. Python has no Color,
and the binding's AddRedaction takes the packed word directly, so the constant is
written out. ctypes narrows it to the same 32-bit pattern the .NET sends.
"""

_SIGNED_INTEGER = re.compile(r'^-?[0-9]+$')
"""
What a number the engine reported as text has to look like before it is converted.

Bare int() is never used on text: it accepts ' +5 ', '5_0' and non-ASCII digits, and
is unbounded. This is engine output rather than user input, so configuration's
value_parsers — which raise a CliError with a user-facing message — are the wrong
shape, but the ASCII-only rule is the same one and the pattern follows theirs.
"""

_INT32_MIN = -2147483648
_INT32_MAX = 2147483647

_CLOSE_REPORTED_NO_CODE = "Unknown Error: None"
"""
What the binding says when Close() succeeded, which it says every time.

DocumentFilters.Close() is `_Call(self.API.Close_Instance)`, and _Call checks the
value the entry point returned — but Close_Instance is declared `restype = None`, so
there is nothing to check and res is always Python's None. _Check then compares None
against IGR_OK, finds it different, reads an empty message out of the error block, and
raises IGRException("Unknown Error: " + str(None)). Every Close() in every program
that uses this binding raises, on a call that did exactly what it was asked.

Initialize() is the same shape of call and gets it right — it invokes Init_Instance
itself and then asks _Check(0, ecb), because these entry points report through the
error control block rather than through a return value — so this is a defect in
Close() alone rather than in how the binding is built. The .NET binding has no
equivalent, which is why the C# reference needs nothing here.

The message is the only signal available. The exception carries the code None, which
_unpack has to widen to EngineError.ERROR, and a genuine failure of Close_Instance
would arrive with that same absent code and a real message out of the error block —
so it is the message that separates the two, and exactly this string, which is
`"Unknown Error: " + str(None)`, that means nothing went wrong. A real message is
still raised, and once the binding is fixed nothing here matches any more.
"""


def _run(call):
    """
    Turns anything the binding raises into an EngineException.

    One place, so no caller above the seam ever catches IGRException and no binding
    type reaches the conversion paths. A failure that is not an IGRException — a
    missing native library surfaces as OSError, a bad page index as IndexError — is
    wrapped too, under EngineError.ERROR, because the caller's question is the same
    either way: the call did not work, and here is what to tell the user.

    Catches Exception and not BaseException, so KeyboardInterrupt and SystemExit
    still travel: the C# equivalent catches System.Exception, which likewise does not
    stop a stack overflow or a thread abort.

    One function rather than the C#'s EngineCall.Run/Run<T> pair, because Python has
    no void: `_run(lambda: x.f())` returns whatever f returned, None included.
    """
    try:
        return call()
    except EngineException:
        # Already translated — a nested call inside this one. Must be caught before
        # Exception, since EngineException is one.
        raise
    except Exception as error:
        raise _translate(error)


def _translate(error):
    """Builds the EngineException for one caught failure."""
    igr_exception = None if _binding is None else getattr(_binding, 'IGRException', None)

    if igr_exception is not None and isinstance(error, igr_exception):
        code, message = _unpack(error)
        return EngineException(EngineException.classify(code), code, message, error)

    return EngineException(EngineError.ERROR, int(EngineError.ERROR), str(error), error)


def _unpack(error):
    """
    The code and message out of an IGRException.

    Nothing like the C#, which reads error.ErrorCode and error.Message off the .NET
    IGRException. The Python IGRException does not have either: it subclasses
    RuntimeError and passes a single dict as the exception's one argument —
    RuntimeError.__init__(self, {'errorCode': id, 'errorMsg': message}) — so both
    values have to be dug out of args[0], and str() on the exception yields the dict's
    repr rather than the message.

    Defensive about the shape because the dict is the binding's, not ours, and
    because IGRException's own default code is 4 (IGR_E_NOT_READABLE) rather than
    anything neutral: the binding raises it with no code at all from _Check when the
    call succeeded but left a message in the error block, which then arrives here
    looking like an unreadable file. The code is carried faithfully either way; it is
    named here as a binding quirk rather than corrected, because guessing would be
    worse.
    """
    detail = error.args[0] if error.args else None

    code = detail.get('errorCode') if isinstance(detail, dict) else None
    message = detail.get('errorMsg') if isinstance(detail, dict) else None

    # bool before int, because in Python True is 1 and would classify as
    # IGR_E_OPEN_ERROR.
    if isinstance(code, bool) or not isinstance(code, int):
        code = int(EngineError.ERROR)

    if not isinstance(message, str):
        message = str(error)

    return code, message


def _try_parse_int(text, fallback):
    """
    A number the engine reported as text, or fallback when it did not report one.

    int.TryParse, which is what the .NET binding uses in both places this is needed:
    an option's flag word, where it falls back to 0, and a bookmark's page index,
    where it falls back to -1. Reproduced here rather than reached for through
    Python's int(), which differs from int.TryParse on ' +5 ', '5_0', '٣' and on
    anything wider than an int32 — see _SIGNED_INTEGER.
    """
    if not isinstance(text, str) or _SIGNED_INTEGER.match(text) is None:
        return fallback

    value = int(text)
    return value if _INT32_MIN <= value <= _INT32_MAX else fallback


def _named(value, kind, fallback):
    """
    One of the seam's enums from whatever the binding reported.

    Not in the C#, where the binding hands back the enum itself. The Python binding
    reports three of the four as bare ints and annotations as its own Enum, so both
    shapes are accepted; a value neither names becomes the fallback rather than
    raising, because a newer native library reporting a kind this build has not heard
    of is not a reason to fail the document.
    """
    raw = value.value if isinstance(value, enum.Enum) else value

    # bool before int again: an isinstance(raw, int) check alone would accept True.
    if isinstance(raw, bool) or not isinstance(raw, int):
        return fallback

    try:
        return kind(raw)
    except ValueError:
        return fallback


_LIBRARY_NAMES = ("ISYS11df.dll", "libISYS11df.so", "libISYS11df.dylib")
"""
The three filenames ISYS11dfAPI.__init__ loads the engine by, one per host.

All three are looked for on every host rather than the one this platform would use.
A directory holding only a foreign-platform library is a directory somebody pointed
--lib-path at on purpose, and letting the binding try it and report its own failure
is better than quietly ignoring what they asked for.
"""


def _loaded_module_path(name: str) -> Optional[str]:
    """
    The full path of a DLL already loaded into this process, or None. Windows only.

    GetModuleHandleW rather than enumerating every module the way the C# does: the
    library's name is known, and asking for it by name is one call where a walk of the
    process's modules is a snapshot API and a loop.
    """
    try:
        kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel32.GetModuleHandleW.restype = ctypes.c_void_p
        kernel32.GetModuleHandleW.argtypes = [ctypes.c_wchar_p]
        kernel32.GetModuleFileNameW.restype = ctypes.c_uint32
        kernel32.GetModuleFileNameW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_uint32]

        module = kernel32.GetModuleHandleW(name)
        if not module:
            return None

        buffer = ctypes.create_unicode_buffer(32768)
        length = kernel32.GetModuleFileNameW(module, buffer, len(buffer))
        return buffer.value if length else None
    except (AttributeError, OSError):
        return None


def _read_file_version(path: str) -> Optional[str]:
    """
    A file's fixed version, as major.minor.build.revision, or None. Windows only.

    VS_FIXEDFILEINFO's numbers rather than the FileVersion string the C# reads: the C++
    port reads the numbers too, and they cannot carry the free text a string resource
    may. The two agree wherever the library sets both, which it does.
    """
    if not os.path.isfile(path):
        return None

    try:
        version = ctypes.WinDLL('version', use_last_error=True)
        version.GetFileVersionInfoSizeW.restype = ctypes.c_uint32
        version.GetFileVersionInfoSizeW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_uint32)]
        version.GetFileVersionInfoW.restype = ctypes.c_int
        version.GetFileVersionInfoW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32,
                                                ctypes.c_void_p]
        version.VerQueryValueW.restype = ctypes.c_int
        version.VerQueryValueW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p,
                                           ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_uint32)]

        handle = ctypes.c_uint32(0)
        size = version.GetFileVersionInfoSizeW(path, ctypes.byref(handle))
        if size == 0:
            return None

        data = ctypes.create_string_buffer(size)
        if not version.GetFileVersionInfoW(path, 0, size, data):
            return None

        info = ctypes.c_void_p()
        info_size = ctypes.c_uint32(0)
        if not version.VerQueryValueW(data, "\\", ctypes.byref(info), ctypes.byref(info_size)) or not info.value:
            return None

        # VS_FIXEDFILEINFO: dwSignature, dwStrucVersion, dwFileVersionMS, dwFileVersionLS, ...
        fields = ctypes.cast(info, ctypes.POINTER(ctypes.c_uint32 * 4)).contents
        if fields[0] != 0xFEEF04BD:
            return None

        most, least = fields[2], fields[3]
        return "{0}.{1}.{2}.{3}".format(most >> 16, most & 0xFFFF, least >> 16, least & 0xFFFF)
    except (AttributeError, OSError):
        return None


def _dll_path(library_path: str) -> str:
    """
    What library_path means as the binding's dll_path, which is not what it means as
    resource_path.

    The binding substitutes its own bundled DF_source_files directory for dll_path only
    when that value is None, empty, or names a directory that does not exist
    (DocumentFilters.Initialize). The CLI's default for --lib-path is the directory the
    tool's own code lives in, which always exists and, for a pip-installed binding,
    holds no engine library at all — so passing it through verbatim would suppress the
    fallback and leave ctypes searching one directory that cannot answer. On Windows
    that is fatal: windll_load tries the named directory and nothing else.

    So the directory is handed on as dll_path only when the library is really in it, and
    otherwise replaced with the empty string that asks the binding for its own copy.
    This refines the binding's own existence test rather than replacing it — same
    question, asked about the library instead of about the directory — which is why it
    lives below the seam: above it there is one path and no notion of what a library is
    called on which host.

    resource_path is not treated this way and must not be: that is ISYS_INIT_PATH, where
    the engine looks for fonts and support files, and D33's whole point is that it
    resolves next to the tool rather than to the current directory.
    """
    if len(library_path) == 0:
        return ""

    for name in _LIBRARY_NAMES:
        if os.path.isfile(os.path.join(library_path, name)):
            return library_path

    return ""


class DocumentFiltersEngine(IDocumentFiltersEngine):
    """IDocumentFiltersEngine over the real library."""

    def __init__(self):
        self._api = _api_type()()

        self._initialised = False
        """
        Whether initialize has succeeded, so close knows there is an instance to close.

        Not in the C#, where DocumentFilters.Close() is itself guarded by `if
        (_handle > 0)` and so is a no-op on an engine that never came up. The Python
        binding's Close() has no such guard: it calls Close_Instance unconditionally,
        and reaching that entry point loads the native library — so closing an engine
        whose initialize failed for want of that library would fail a second time, in
        a `with` block's exit, and replace the real error with a confusing one.
        """

        self._library_path = ""
        """The --lib-path given to initialize, for describe_version."""

    def initialize(self, license_key: str, library_path: str) -> None:
        # The CLI has one --lib-path, and the .NET binding has one parameter for it.
        # The Python binding splits the two jobs the directory does —
        # Initialize(license, resource_path, dll_path) — where resource_path is where
        # the engine looks for fonts and support files and dll_path is where ctypes
        # looks for the library itself. They are the same directory for an install that
        # keeps both together, which is every .NET one, and so the C# has no reason to
        # tell them apart — but a pip-installed binding keeps the libraries inside its
        # own package, so the two genuinely differ here. _dll_path says which of the two
        # the value can answer for.
        #
        # Empty is meaningful to the binding rather than wrong: an empty or missing
        # dll_path falls back to the DF_source_files directory inside the installed
        # package, which is where the wheel puts the libraries, and an empty
        # resource_path leaves the engine to find its support files the way it does
        # when none is given. The `or ""` is not cosmetic — the binding calls
        # resource_path.encode('utf8'), which would raise AttributeError on None and
        # report a missing --lib-path as an internal error.
        path = library_path or ""

        _run(lambda: self._api.Initialize(license_key, path, _dll_path(path)))
        self._initialised = True

        # Remembered for describe_version, as the C# remembers it.
        self._library_path = path

    def open_document(self, path: str, flags: int, options: str,
                      password: Optional[Callable[[], Optional[str]]] = None,
                      should_cancel: Optional[Callable[[], bool]] = None,
                      log_level_for: Optional[Callable[[str], int]] = None,
                      on_log_message: Optional[Callable[[int, str, str], None]] = None) -> IDocument:
        # filename= rather than the positional source=, so the binding routes to
        # Extractor.FromFilename instead of type-sniffing the argument. GetExtractor's
        # positional form dispatches on isinstance and would read a str as a path
        # anyway, but naming it says which of the four overloads is meant.
        return _run(lambda: _open(self._api.GetExtractor(filename=path), flags, options, password,
                                  should_cancel, log_level_for, on_log_message))

    def open_document_from_stream(self, source: BinaryIO, display_name: str, flags: int, options: str,
                                  password: Optional[Callable[[], Optional[str]]] = None,
                                  should_cancel: Optional[Callable[[], bool]] = None,
                                  log_level_for: Optional[Callable[[str], int]] = None,
                                  on_log_message: Optional[Callable[[int, str, str], None]] = None) -> IDocument:
        return _run(lambda: _open(self._api.GetExtractor(stream=source), flags, options, password,
                                  should_cancel, log_level_for, on_log_message))

    def set_temp_path(self, path: str) -> None:
        def call():
            # No Marshal.StringToHGlobalAnsi and no try/finally around a FreeHGlobal:
            # a ctypes buffer is owned by the interpreter, stays alive as long as this
            # frame holds it, and is collected however the call ends. There is no
            # handle to leak here, so the rule about releasing one on the failure path
            # has nothing to release.
            #
            # UTF-8 rather than the C#'s ANSI, which is lossy for a temp directory
            # outside the host's code page. UTF-8 is what the rest of the Python
            # binding encodes char* arguments as.
            buffer = ctypes.create_string_buffer((path or "").encode('utf-8'))

            address = ctypes.c_size_t(ctypes.cast(buffer, ctypes.c_void_p).value)
            unused = ctypes.c_size_t(0)
            error = _bind().Error_Control_Block()

            # The return code is discarded, as the C# discards it. Worth knowing that
            # it is: a --temp-dir the engine refuses is silently ignored in both.
            self._api.API.IGR_Multiplex(_bind().IGR_Multi_Set_Temp_Path, ctypes.byref(address),
                                        ctypes.byref(unused), ctypes.byref(error))

        _run(call)

    def get_memory_status(self) -> str:
        def call():
            # Straight through the binding's declared entry point, unlike the C#,
            # which has to redeclare IGR_Get_Memory_Status because the .NET binding
            # marshals its length parameter in a way that throws before reaching the
            # engine. The Python binding declares it correctly —
            # argtypes [c_char_p, c_size_t], restype c_size_t — so nothing here goes
            # around the binding.
            #
            # Its high-level wrapper is still unusable, for a different reason:
            # DocumentFilters.GetMemoryStatus() is `return ""`, a stub that never
            # calls the engine at all. So the call is made against the API object the
            # binding publishes rather than against that method.
            buffer = ctypes.create_string_buffer(4096)
            written = self._api.API.IGR_Get_Memory_Status(buffer, len(buffer))

            # The engine reports how much it wrote; a zero means it had nothing to say
            # rather than that the call failed.
            return "" if written == 0 else buffer.value.decode('utf-8', 'replace')

        return _run(call)

    def identify_file(self, path: str) -> EngineFileType:
        return _run(lambda: _identify(self._api.GetExtractor(filename=path)))

    def identify_file_from_stream(self, source: BinaryIO, display_name: str) -> EngineFileType:
        return _run(lambda: _identify(self._api.GetExtractor(stream=source)))

    def make_canvas(self, target: str, canvas_type: CanvasType, options: str) -> IOutputCanvas:
        return _run(lambda: _CanvasAdapter(
            self._api.MakeOutputCanvas(target, int(canvas_type), options or "")))

    def make_canvas_on_stream(self, target: BinaryIO, canvas_type: CanvasType, options: str) -> IOutputCanvas:
        return _run(lambda: _CanvasAdapter(
            self._api.MakeOutputCanvas(target, int(canvas_type), options or "")))

    def get_supported_formats(self) -> List[EngineFormat]:
        def call():
            formats = []
            for item in self._api.GetSupportedFormats():
                entry = EngineFormat()
                entry.id = item.Id
                entry.short_name = item.ShortName
                entry.display_name = item.DisplayName
                formats.append(entry)

            return formats

        return _run(call)

    def get_available_options(self) -> List[EngineOption]:
        """
        The engine's option table, read attribute by attribute.

        The C# is one loop over the binding's own Option objects, because the .NET
        Option carries all five fields this needs. The Python binding's Option carries
        two — DisplayName and Description — and its GetAll() does not report the index
        either, so there is nothing to ask for the other three with.

        The row's own six attributes are still reachable through the binding, via the
        public Option.Fetch static that its GetAll uses; what is reconstructed here is
        only the loop around it, and the selector numbers and the 1024*64 bound are
        the .NET binding's. So this is not going around the binding, and the three
        fields the Python wrapper drops do not have to be reported empty — which
        matters, because the options command's output is compared across all four
        ports.
        """
        def call():
            option_type = _api_type().Option
            api = self._api.API
            options = []

            for index in range(1024 * 64):
                name = option_type.Fetch(api, index, int(_OptionAttribute.NAME))

                # Fetch answers None for a row the engine does not have, which is how
                # both bindings find the end of the table.
                if name is None:
                    break

                entry = EngineOption()
                entry.display_name = name
                entry.description = option_type.Fetch(api, index, int(_OptionAttribute.DESCRIPTION))
                entry.default_value = option_type.Fetch(api, index, int(_OptionAttribute.DEFAULT_VALUE))
                entry.flags = _try_parse_int(
                    option_type.Fetch(api, index, int(_OptionAttribute.FLAGS)), 0)
                entry.type = option_type.Fetch(api, index, int(_OptionAttribute.TYPE))
                options.append(entry)

            return options

        return _run(call)

    def describe_version(self) -> str:
        """
        The version of the native library this process is actually running, or unknown.

        Asked of the loaded module first, as the C# does, because that is the only answer
        that is a fact rather than a guess: it stays right when --lib-path points somewhere
        else or the loader found the library on PATH. The probes after it are for a
        version command that runs before anything loaded the library.

        This used to report the binding's own package version, the same defect the C#
        had with its binding assembly's: a plausible-looking number under the engine's
        label. A wrong answer in the right shape is worse than none, so the fallback is
        unknown.

        A file version is a PE resource, read here through version.dll with ctypes --
        standard library, so no dependency is added. There is no ELF or Mach-O equivalent,
        so every other platform reports unknown, as the C# and C++ ports do there.
        """
        if sys.platform != 'win32':
            return "unknown"

        for candidate in self._library_path_candidates():
            version = _read_file_version(candidate)
            if version is not None:
                return version

        return "unknown"

    def _library_path_candidates(self) -> List[str]:
        """Where the native library might be, most authoritative first."""
        candidates = []

        loaded = _loaded_module_path("ISYS11df.dll")
        if loaded is not None:
            candidates.append(loaded)

        if self._library_path:
            candidates.append(os.path.join(self._library_path, "ISYS11df.dll"))

        return candidates

    def close(self) -> None:
        if not self._initialised:
            return

        try:
            _run(lambda: self._api.Close())
        except EngineException as error:
            # The instance is closed by the time this arrives — the entry point ran and
            # only the check on what it returned is wrong. See _CLOSE_REPORTED_NO_CODE.
            if error.message != _CLOSE_REPORTED_NO_CODE:
                raise


def _open(extractor, flags, options, password, should_cancel, log_level_for, on_log_message):
    """
    Opens an extractor the caller has already obtained, whether from a path or a
    stream.

    Shared so the callback exists once: a second copy would be a second place for the
    password refusal and the cancellation answer to drift apart.
    """
    binding = _bind()

    # Anything the callback raises, kept until Open has returned.
    #
    # Not in the C#, where an exception thrown inside the callback travels out through
    # Open. The Python binding catches Exception around every call into our callback,
    # prints the exception and a traceback, and returns IGR_E_ERROR. Letting it do
    # that would lose the error and, worse, write to stdout — which --stdout has given
    # to the document. So the callback swallows nothing and decides nothing: it parks
    # the exception, answers IGR_E_ERROR exactly as the binding would have, and the
    # exception is re-raised below, preserving the C#'s behaviour.
    failure = []

    # The engine asks for a password through this callback rather than by returning:
    # refusing it here is what turns into IGR_E_PASSWORD out of Open, which _run then
    # translates like any other failure.
    def callback(request):
        try:
            action = request.Action

            if action == binding.IGR_OPEN_CALLBACK_ACTION_PASSWORD:
                candidate = password() if password is not None else None
                if candidate is None:
                    return binding.IGR_E_PASSWORD

                # SetPassword rather than the C#'s `request.Password.password = ...`:
                # the Python ActionPassword exposes no settable field, because the
                # value has to be written into a fixed UTF-16 array inside the
                # engine's own struct rather than assigned.
                request.Password.SetPassword(candidate)
            elif (action == binding.IGR_OPEN_CALLBACK_ACTION_HEARTBEAT
                  and should_cancel is not None and should_cancel()):
                # The engine's one interruption point, raised periodically while it
                # parses. doc2text answers the same action the same way.
                return binding.IGR_CANCELLED
            elif action == binding.IGR_OPEN_CALLBACK_ACTION_LOG_LEVEL and log_level_for is not None:
                # The engine asks per module how much to log, so the --log-modules
                # filter is applied by answering none for a module nobody named.
                #
                # Level and Module, where the C# writes LogLevel.result and reads
                # LogLevel.module: the Python binding wraps both in properties, and
                # Module arrives already decoded from UTF-8 rather than as bytes.
                request.LogLevel.Level = log_level_for(request.LogLevel.Module)
            elif action == binding.IGR_OPEN_CALLBACK_ACTION_LOG_MESSAGE and on_log_message is not None:
                # LogMessage.Level, not log_level; also already a str, not a cast.
                on_log_message(request.LogMessage.Level, request.LogMessage.Module,
                               request.LogMessage.Message)

            return binding.IGR_OK
        except Exception as error:
            failure.append(error)
            return binding.IGR_E_ERROR

    try:
        # The flags-only form, so the caller's format bit survives. The .NET binding's
        # OpenMode overload would clear bits 16 to 31 and substitute its own; the
        # Python binding has only this form, so there is nothing to choose.
        extractor.Open(flags, options or "", callback)
    finally:
        # Raised from finally deliberately: when Open failed *because* the callback
        # failed, the binding's IGR_E_ERROR is the symptom and this is the cause, so
        # the cause is what should surface. Python keeps the other as __context__.
        if failure:
            raise failure[0]

    return _DocumentAdapter(extractor)


def _identify(extractor):
    """Reads what a file is, without opening it."""
    with extractor:
        result = EngineFileType()
        result.id = extractor.getFileType()
        result.mime_type = extractor.getFileType(int(_FormatAttribute.MIME_TYPE))
        result.name = extractor.getFileType(int(_FormatAttribute.LONG_NAME))

        # GetFileCapabilities, capitalised, where the C# calls getFileCapabilities:
        # the Python binding renamed this one and not the getFileType beside it.
        result.capabilities = extractor.GetFileCapabilities()
        return result


def _drain(first, next_entry):
    """
    Every entry of one cursor, as a list.

    Drained here rather than handed back as a cursor: GetFirstSubFile and
    GetNextSubFile advance state on the extractor itself, so anything else touching
    the same handle mid-walk loses entries.
    """
    entries = []

    entry = first()
    while entry is not None:
        entries.append(_SubFileAdapter(entry))
        entry = next_entry()

    return entries


def _convert_bookmark(bookmark):
    """One outline node and everything beneath it."""
    if bookmark is None:
        return None

    children = []
    for child in bookmark.Children or []:
        children.append(_convert_bookmark(child))

    result = EngineBookmark()
    result.title = bookmark.Title
    result.level = bookmark.Level

    # Parsed here, where the C# reads the binding's own PageIndex property.
    #
    # Both bindings derive the index from the bookmark's destination string and answer
    # -1 for anything they cannot read, but they disagree on what they cannot read:
    # .NET uses int.TryParse, and the Python binding uses a bare int() inside a bare
    # except. So a destination of ' +5 ', '5_0', '٣' or a value wider than an int32
    # yields -1 in the C# and a number in Python — a real difference in output for a
    # malformed destination, which a damaged PDF does produce. Reading the raw
    # destination and applying int.TryParse's rules makes the two ports agree; it is
    # the .NET binding's answer that is being reproduced, not a new policy.
    result.page_index = _try_parse_int(bookmark.Destination, -1)
    result.children = children
    return result


def _convert_page_element(element):
    """
    One element-tree node and everything beneath it.

    The whole tree is copied in one pass rather than lazily, for the same reason the
    subfile cursor is drained: the binding's element walk reads through the page
    handle, which the caller is free to close as soon as this returns.

    The walk is written out as get_first_child/get_next_sibling instead of iterating
    the binding's PageElement.Children(), which the C# uses. That generator never
    advances — it yields its first child forever — so iterating it does not return.
    Its AllChildren() sibling does advance correctly, but it flattens the tree, and
    the structure report needs the shape.
    """
    if element is None:
        return None

    children = []
    child = element.GetFirstChild()
    while child is not None:
        children.append(_convert_page_element(child))
        child = child.GetNextSibling()

    result = EnginePageElement()
    result.type = _named(element.Type, PageElementType, PageElementType.PAGE)
    result.depth = element.Depth

    # The binding reports geometry as left/top/right/bottom, where the .NET binding
    # hands the same four numbers to RectangleF.FromLTRB. Converted to x/y/width/
    # height here so every port reports the pair the seam declares.
    bounds = element.Bounds
    result.bounds = EngineRectangle(bounds.left, bounds.top,
                                    bounds.right - bounds.left, bounds.bottom - bounds.top)
    result.children = children
    return result


class _DocumentAdapter(IDocument):
    """IDocument over Extractor."""

    def __init__(self, extractor, owns_handle: bool = True):
        """
        Args:
            extractor: The handle to wrap.
            owns_handle: False when the handle belongs to something else — a subfile,
                which the binding models as an Extractor in its own right (SubFile
                literally subclasses it), so opening it yields a document that shares
                the entry's handle and must not close it.
        """
        self._extractor = extractor
        self._owns_handle = owns_handle

    @property
    def inner(self):
        """
        The handle behind this document, for the calls that need it by identity.

        Canvas.RenderPages takes an Extractor, not anything the seam can describe, so
        the canvas adapter has to reach back through here. Python has no `internal`,
        so this is public where the C# is not; the binding type still does not escape
        this module, because nothing outside it knows the property is here.
        """
        return self._extractor

    def get_file_type(self) -> int:
        return _run(lambda: self._extractor.getFileType())

    def get_mime_type(self) -> str:
        return _run(lambda: self._extractor.getFileType(int(_FormatAttribute.MIME_TYPE)))

    def get_format_name(self) -> str:
        return _run(lambda: self._extractor.getFileType(int(_FormatAttribute.LONG_NAME)))

    def get_file_capabilities(self) -> int:
        return _run(lambda: self._extractor.GetFileCapabilities())

    def get_eof(self) -> bool:
        return _run(lambda: self._extractor.getEOF())

    def get_text(self, max_length: int) -> str:
        return _run(lambda: self._extractor.GetText(max_length))

    def get_page_count(self) -> int:
        return _run(lambda: self._extractor.GetPageCount())

    def get_page(self, page_index: int) -> IDocumentPage:
        return _run(lambda: _PageAdapter(self._extractor.GetPage(page_index)))

    @property
    def sub_files(self) -> List[ISubFileEntry]:
        return _run(lambda: _drain(self._extractor.GetFirstSubFile, self._extractor.GetNextSubFile))

    @property
    def images(self) -> List[ISubFileEntry]:
        return _run(lambda: _drain(self._extractor.GetFirstImage, self._extractor.GetNextImage))

    @property
    def root_bookmark(self) -> Optional[EngineBookmark]:
        def call():
            # The two bindings build this root completely differently, and the
            # difference is why the try is here.
            #
            # .NET never calls IGR_Get_Bookmark_Root at all: RootBookmark hands back a
            # synthetic Bookmark wrapping the document handle over a zeroed item, so it
            # is never null and never throws, and its own Title, Level and PageIndex
            # are "", 0 and -1 whatever the document holds. Only its Children are
            # real, walked with IGR_Get_Bookmark_First_Child.
            #
            # The Python binding calls IGR_Get_Bookmark_Root and lets the failure
            # through, so a document with no outline raises. That absence is translated
            # back into None here — error translation, which this file is allowed; the
            # alternative is a port that reports a document without an outline as a
            # failed document. It also means the Python root node carries the engine's
            # real root fields where the C# root carries placeholders, so a caller that
            # prints anything but children would see the two ports disagree.
            try:
                bookmark = self._extractor.RootBookmark
            except Exception:
                return None

            return _convert_bookmark(bookmark)

        return _run(call)

    def make_doc_pack(self, filename: str, options: str) -> None:
        _run(lambda: self._extractor.MakeDocPack(filename, options))

    def make_doc_pack_on_stream(self, destination: BinaryIO, options: str) -> None:
        # One binding method for both destinations, where C# has two overloads: the
        # Python binding dispatches MakeDocPack on the type of its first argument, a
        # path to IGR_Make_DocPack and a stream to IGR_Make_DocPack_On. The seam keeps
        # the two members apart anyway, because what the caller has is what decides.
        _run(lambda: self._extractor.MakeDocPack(destination, options))

    def close(self) -> None:
        if self._owns_handle:
            _run(lambda: self._extractor.Close())


class _PageAdapter(IDocumentPage):
    """IDocumentPage over Page."""

    def __init__(self, page):
        self._page = page

    @property
    def inner(self):
        """The handle behind this page, for Canvas.RenderPage."""
        return self._page

    def get_width(self) -> int:
        return _run(lambda: self._page.GetWidth())

    def get_height(self) -> int:
        return _run(lambda: self._page.GetHeight())

    def get_attribute(self, name: str) -> Optional[str]:
        return _run(lambda: self._page.GetAttribute(name))

    @property
    def hyperlinks(self) -> List[EngineHyperlink]:
        def call():
            links = []
            for link in self._page.Hyperlinks:
                item = EngineHyperlink()

                # GetActionType(), where the C# reads link.ActionType: the Python
                # Hyperlink publishes properties for the geometry and for Uri and Ref
                # but not for the action type, so the getter is the only way to read
                # it. The rest are read through their getters too, so the block reads
                # as one shape rather than switching style mid-list — the binding's
                # properties are one-line delegations to exactly these.
                item.type = _named(link.GetActionType(), HyperlinkActionType,
                                   HyperlinkActionType.GOTO)
                item.uri = link.GetUri()
                item.ref = link.GetRef()
                item.x = link.GetX()
                item.y = link.GetY()
                item.width = link.GetWidth()
                item.height = link.GetHeight()
                links.append(item)

            return links

        return _run(call)

    @property
    def form_elements(self) -> List[EngineFormElement]:
        def call():
            elements = []
            for element in self._page.FormElements:
                item = EngineFormElement()

                # GetElementType, not the C#'s Type: the Python FormElement has no
                # Type property, and its ElementType property returns the bare int.
                item.type = _named(element.GetElementType(), FormElementType,
                                   FormElementType.BUTTON)
                item.name = element.GetName()
                item.x = element.GetX()
                item.y = element.GetY()
                item.width = element.GetWidth()
                item.height = element.GetHeight()
                elements.append(item)

            return elements

        return _run(call)

    @property
    def annotations(self) -> List[EngineAnnotation]:
        def call():
            annotations = []
            for annotation in self._page.Annotations:
                item = EngineAnnotation()

                # The one place the binding does hand back an enum of its own —
                # Annotations.Type — so _named unwraps it rather than reading an int.
                item.type = _named(annotation.Type, AnnotationKind, AnnotationKind.UNKNOWN)
                item.title = annotation.Title
                item.text = annotation.Text
                annotations.append(item)

            return annotations

        return _run(call)

    def get_root_page_element(self) -> Optional[EnginePageElement]:
        def call():
            # Same translation as root_bookmark, and for the same reason: the .NET
            # binding returns null when IGR_Get_Page_Element_Root reports anything but
            # success, and the Python binding raises.
            try:
                element = self._page.GetRootPageElement()
            except Exception:
                return None

            return _convert_page_element(element)

        return _run(call)

    def close(self) -> None:
        _run(lambda: self._page.Close())


class _CanvasAdapter(IOutputCanvas):
    """IOutputCanvas over Canvas."""

    def __init__(self, canvas):
        self._canvas = canvas

    def render_page(self, page: IDocumentPage, options: str,
                    decorations: Optional[EnginePageDecorations]) -> None:
        """
        Renders one page, with its decorations when it has any.

        The C# picks between two binding overloads here on the grounds that passing a
        properties object built from nothing is not the same as passing none. It is
        not true in either binding: the Python RenderPage builds an empty
        RenderPageProperties when given none and always calls IGR_Render_Page_Ex, and
        the .NET overload the C# reaches for in the empty case does the same with a
        default-constructed struct. The shape is kept anyway — None where there is
        nothing to say, a built object where there is — because it states the intent,
        and because the day a binding does tell the two apart this is the code that
        would have to be right.
        """
        inner = _inner_page(page)

        def call():
            if decorations is None or decorations.is_empty:
                self._canvas.RenderPage(inner, options or "")
                return

            properties = _api_type().RenderPageProperties()

            # left/top/right/bottom, where the C# builds a Rectangle from
            # x/y/width/height: the Python AddRedaction takes the edges and computes
            # the extent itself.
            for redaction in decorations.redactions:
                properties.AddRedaction(redaction.x, redaction.y,
                                        redaction.x + redaction.width,
                                        redaction.y + redaction.height,
                                        _REDACTION_BLACK)

            # 0, not False, for the selected flag the C# passes as false. It reaches a
            # 32-bit integer field in the engine's struct, and bool is an int in
            # Python only by inheritance.
            for value in decorations.form_values:
                properties.AddFormValue(value.name, value.value, 0)

            self._canvas.RenderPage(inner, options or "", properties)

        _run(call)

    def render_pages(self, document: IDocument) -> None:
        if not isinstance(document, _DocumentAdapter):
            raise _mismatch("document")

        _run(lambda: self._canvas.RenderPages(document.inner))

    def draw_image(self, x: int, y: int, image: bytes, mime_type: str) -> None:
        _run(lambda: self._canvas.DrawImage(x, y, image, mime_type))

    def close(self) -> None:
        _run(lambda: self._canvas.Close())


def _inner_page(page):
    if isinstance(page, _PageAdapter):
        return page.inner

    raise _mismatch("page")


def _mismatch(what):
    """
    A real canvas was handed something that did not come from the real engine.

    Only reachable by mixing a fake engine's objects with a real canvas, which no run
    does — the engine is chosen once per process. Reported rather than left as an
    AttributeError so that, if it ever does happen, the message says what went wrong
    instead of naming two adapter classes.
    """
    return EngineException(
        EngineError.INVALID_PARAMETER,
        int(EngineError.INVALID_PARAMETER),
        "this canvas can only render a " + what + " that came from the same engine")


class _SubFileAdapter(ISubFileEntry):
    """ISubFileEntry over SubFile."""

    def __init__(self, subfile):
        self._subfile = subfile

    @property
    def id(self) -> str:
        return self._subfile.ID

    @property
    def name(self) -> str:
        return self._subfile.Name

    @property
    def size(self) -> int:
        return self._subfile.FileSize

    @property
    def modified(self):
        """
        The entry's timestamp, or None when the container records none.

        No MinValue check, unlike the C#, and the two ports do not currently agree
        here. The Python binding's _FileTimeToDateTime answers None for a zero
        FILETIME, which is exactly the contract, so there is nothing to translate.

        The C# tests `_subfile.FileDate == DateTime.MinValue`, but the .NET binding
        never produces DateTime.MinValue: getFileTime is
        `new DateTime(1601, 1, 1).AddSeconds(_date / 10000000.0)`, so a zero FILETIME
        comes back as 1601-01-01T00:00:00 and the guard is dead code. Until the
        reference is fixed, an entry with no timestamp reads as absent here and as a
        date in 1601 there. The reference is the side that is wrong, so this port is
        deliberately not bug-compatible with it.

        The two also compute the conversion differently — .NET divides by 1e7 in
        floating point, the Python binding takes exact integer microseconds — so they
        can disagree in the last fractional digit of a timestamp that does exist.
        """
        return _run(lambda: self._subfile.FileDate)

    def open(self, flags: int, options: str) -> IDocument:
        def call():
            self._subfile.Open(flags, options or "")
            return _DocumentAdapter(self._subfile, owns_handle=False)

        return _run(call)

    def copy_to(self, path: str) -> None:
        # CopyTo given a str opens it itself, in binary mode, so the entry's bytes
        # cross unchanged and no newline is translated on Windows.
        _run(lambda: self._subfile.CopyTo(path))

    def close(self) -> None:
        _run(lambda: self._subfile.Close())
