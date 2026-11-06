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

import sys
import time
from typing import Callable, Optional

from ..configuration.value_parsers import is_all_whitespace, is_whitespace
from ..engine.engine_exception import EngineException
from ..runtime.cli_error import LimitExceededError

_BYTES_PER_MEGABYTE = 1024 * 1024


class Diagnostics:
    """
    Timing, memory reporting, progress and the run limits.

    Unlike everything else the tool prints, this output is deliberately NOT part of the
    cross-language contract, because elapsed times and memory figures differ from run to
    run. The conformance suite ignores it.
    """

    def __init__(self, options, console, engine=None,
                 clock: Optional[Callable[[], float]] = None):
        """
        clock is a monotonic reading in seconds, for tests that need time to pass without
        waiting for it. --timeout is otherwise only testable by sleeping past it, and the
        smallest value the validator accepts is one second; a clock the test advances
        makes those cases instant and exact. Every reading below goes through it, so
        nothing measures time two different ways.

        Seconds as a float where the C# takes a Func<TimeSpan>: time.perf_counter is the
        default and already answers in that unit, and Python has no TimeSpan to convert
        through. perf_counter rather than time.time because it is monotonic -- the wall
        clock can be stepped backwards mid-document by an NTP correction, which would let a
        document that really did outrun --timeout slip past the check. Its epoch is
        undefined, so only differences are ever taken.
        """
        self._options = options
        self._console = console
        self._engine = engine
        self._clock = clock if clock is not None else time.perf_counter

        # The C# holds Process.GetCurrentProcess() in a field, because the peak working
        # set is a property of that object. Python has no process object in the standard
        # library that exposes it, so there is nothing to hold: _peak_working_set_bytes
        # asks the operating system directly, each time it is asked. Nothing is lost —
        # the C# field is a handle to "this process", which never changes — and the two
        # callers each read the figure once, so caching would buy nothing either.
        self._run_start: Optional[float] = None
        self._document_start: Optional[float] = None
        self._document_elapsed: Optional[float] = None

    def begin_run(self):
        """Marks the start of the run, for --time."""
        self._run_start = self._clock()

    def begin_document(self, display_name: str, index: int, total: int):
        """
        Marks the start of one document, for --logtime and --timeout.

        display_name is printed exactly as it was handed over. It is not re-normalised
        here: turning a path into the text the user sees is the job of whoever built the
        name, and doing it twice in two places is how the two spellings drift apart.
        """
        if self._options.progress:
            self._console.write_line("[{0}/{1}] {2}".format(index, total, display_name))

        self._document_start = self._clock()
        self._document_elapsed = None

    def _current_document_elapsed(self) -> float:
        """
        How long the current document has taken: frozen once it has ended, still running
        before that.
        """
        if self._document_elapsed is not None:
            return self._document_elapsed

        return self._clock() - self._document_start if self._document_start is not None else 0.0

    def end_document(self, display_name: str):
        """
        Ends the document's timing window, emitting its timing if asked and leaving the
        elapsed time readable.

        Stopped rather than discarded. check_document_timeout runs *after* this at every
        call site, so clearing the timer here made its None guard always true and
        --timeout never fired at all — a document could run for any length of time and
        still exit 0. begin_document replaces the timer for the next document, so nothing
        needs clearing here; stopping it also means the elapsed time the check reads is
        the document's own, not the document plus whatever ran between the two calls.
        """
        if self._document_start is None:
            return

        self._document_elapsed = self._clock() - self._document_start

        if self._options.log_time:
            self._console.write_line("{0}: {1:.3f}s".format(display_name, self._document_elapsed))

        self._report_engine_memory(at_least=2)

    def end_run(self, succeeded: int, failed: int):
        """Emits the run summary for --time, and the closing reports."""
        if self._options.time and self._run_start is not None:
            self._console.write_line("{0} succeeded, {1} failed in {2:.3f}s".format(
                succeeded, failed, self._clock() - self._run_start))

        if self._options.log_memory:
            self._console.write_line("peak memory: {0}MB".format(_peak_working_set_bytes() // _BYTES_PER_MEGABYTE))

        # -x reports the engine's own view once, at the end; --xx also reports it
        # after every document, which end_document does.
        self._report_engine_memory(at_least=1)

    def _report_engine_memory(self, at_least: int):
        """
        Prints the engine's memory report when -x or --xx asked for it.

        The two switches are one level: -x is 1 and reports at the end of the run, --xx is
        2 and also reports after each document, which is how doc2text spells the same
        pair. The text is the engine's own and is printed verbatim — it is diagnostics,
        not contract.
        """
        if self._options.memory_status_level < at_least or self._engine is None:
            return

        try:
            status = self._engine.get_memory_status()
            if not is_all_whitespace(status):
                self._console.write_line(_trim_end(status))
        except EngineException as error:
            # A diagnostic must not fail a run that otherwise worked. Reported so
            # the silence is explained rather than mistaken for "no memory in use".
            self._console.write_error("-x could not read the engine's memory status: {0}".format(error.message))

    def check_limits(self):
        """
        Checks --max-time and --max-memory.

        --timeout is per document rather than per run and is checked separately, in
        end_document's caller — a synchronous native call cannot be pre-empted mid-flight,
        so this can only notice a document that has already finished taking too long, not
        stop one early.

        Raises:
            LimitExceededError: A limit was passed.
        """
        if (self._options.max_time is not None and self._run_start is not None
                and self._clock() - self._run_start > self._options.max_time):
            raise LimitExceededError("--max-time of {0}s exceeded".format(self._options.max_time))

        if self._options.max_memory is not None:
            # Integer division, as the C# long division is: the report is whole
            # megabytes, and a true division here would compare a float against the
            # limit and fire a megabyte early.
            used_megabytes = _peak_working_set_bytes() // _BYTES_PER_MEGABYTE
            if used_megabytes >= self._options.max_memory:
                raise LimitExceededError("--max-memory of {0}MB exceeded".format(self._options.max_memory))

    def check_document_timeout(self, display_name: str):
        """
        Checks the just-finished document against --timeout.

        Raises:
            LimitExceededError: The document took longer than allowed.
        """
        if self.document_deadline_exceeded():
            raise self.document_timed_out(display_name)

    def document_deadline_exceeded(self) -> bool:
        """
        True once the document being processed has run past --timeout.

        Polled by the engine's open callback as well as checked after the document,
        because those cover different halves and neither covers both. The callback can
        stop a document that hangs while the engine parses it, which is the only point the
        engine offers; it cannot touch the page loop, because IGR_Render_Page takes no
        callback. The check after the document therefore stays — it is the only thing that
        sees rendering time at all, even though by then the work is already done.
        """
        return (self._options.timeout is not None
                and self._document_start is not None
                and self._current_document_elapsed() > self._options.timeout)

    def document_timed_out(self, display_name: str) -> LimitExceededError:
        """
        The error for a document that outran --timeout.

        Built here rather than at each throw site so both routes — the cancelled open and
        the check afterwards — report the identical line, which the CLI tests pin for
        both.

        Returned rather than raised, as the C# returns it: the caller raises, so a
        `raise` here would move the traceback's origin away from the site that decided the
        document had run too long.

        Reads options.timeout with no None guard, exactly as the C# reads
        Timeout.Value with none. Both routes reach this only after
        document_deadline_exceeded said yes, which requires the option to be set.
        Where the two differ is what a misuse looks like: C# throws
        InvalidOperationException on the spot, whereas Python would quietly format the
        message as "--timeout of Nones exceeded". No guard is added, because a guard would
        have to invent a message for a case that cannot happen and would then be the only
        untested line here.
        """
        return LimitExceededError("--timeout of {0}s exceeded".format(self._options.timeout), display_name)

    def pause_if_requested(self):
        """Waits for a keypress between documents, for --pause."""
        if self._options.pause:
            self._console.wait_for_key()



def _trim_end(text: str) -> str:
    """
    Strips trailing whitespace exactly as string.TrimEnd does.

    Written here rather than taken from value_parsers, which has trim and
    is_all_whitespace but no trailing-only form, and this is the only place that wants
    one. It cannot be trim: the engine's memory report is several lines, and the first of
    them may be indented, so trimming both ends would reflow text the C# prints verbatim
    apart from its trailing newline.

    It cannot be str.rstrip() either, for the reason trim is not str.strip(): Python calls
    U+001C-U+001F whitespace and .NET does not, so a report ending in one of them would
    come out a character shorter here than in the other three ports. Shares
    value_parsers.is_whitespace so there is still only one answer to "what counts as
    whitespace".
    """
    end = len(text)
    while end > 0 and is_whitespace(text[end - 1]):
        end -= 1

    return text[:end]


def _peak_working_set_bytes() -> int:
    """
    The largest working set this process has reached, in bytes.

    Process.PeakWorkingSet64 in the C#, which the standard library has no portable
    equivalent of, so this asks each platform in its own terms. Both answers come from
    modules already in the standard library — no dependency is added.

    On Windows, GetProcessMemoryInfo's PROCESS_MEMORY_COUNTERS.PeakWorkingSetSize, reached
    through ctypes. That is the same counter Process.PeakWorkingSet64 reads, so the figure
    on the platform this tool is most often run on is the reference figure exactly.

    Elsewhere, getrusage's ru_maxrss, which is the peak resident set size — the nearest
    thing POSIX names. Its unit is not portable: Linux reports kilobytes, while macOS and
    the BSDs report bytes, so the scale is chosen by platform rather than assumed.
    ru_maxrss is close to but not identical with the Windows counter, which is where
    Diagnostics' own opening point earns its keep — this output is diagnostic, not
    contract, and no test compares the number across ports.

    Returns 0 when neither route is available, which is the one real gap in this port and
    is deliberate rather than overlooked. A diagnostic must not be the thing that fails a
    run, so an unexpected platform prints "peak memory: 0MB" instead of raising. The cost
    is that --max-memory then never fires there, since 0 is below every limit; a limit that
    never triggers is the safer of the two wrong answers, because the alternative — a
    limit that triggers immediately — would exit 6 on a run that used no memory at all.
    """
    if sys.platform == 'win32':
        return _windows_peak_working_set_bytes()

    try:
        import resource
    except ImportError:
        return 0

    peak = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss

    # Darwin and the BSDs report bytes; Linux and the rest report kilobytes.
    if sys.platform == 'darwin':
        return int(peak)

    return int(peak) * 1024


def _windows_peak_working_set_bytes() -> int:
    """
    PeakWorkingSetSize for the current process, via psapi.

    The struct is declared with c_size_t for every SIZE_T member so the layout is right in
    both 32-bit and 64-bit processes, and the two function signatures are declared rather
    than left to ctypes' defaults: GetCurrentProcess returns a HANDLE, which a default
    c_int return type would truncate on 64-bit.

    The entry point lives in psapi.dll and is also exported from kernel32.dll as
    K32GetProcessMemoryInfo; both are tried, because which one is present depends on the
    Windows version and on how psapi was built. Any failure returns 0, for the reason
    _peak_working_set_bytes gives.
    """
    import ctypes

    class _ProcessMemoryCounters(ctypes.Structure):
        _fields_ = [
            ('cb', ctypes.c_ulong),
            ('PageFaultCount', ctypes.c_ulong),
            ('PeakWorkingSetSize', ctypes.c_size_t),
            ('WorkingSetSize', ctypes.c_size_t),
            ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
            ('QuotaPagedPoolUsage', ctypes.c_size_t),
            ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
            ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
            ('PagefileUsage', ctypes.c_size_t),
            ('PeakPagefileUsage', ctypes.c_size_t),
        ]

    try:
        kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

        get_memory_info = None
        for library_name, entry_point in (('psapi', 'GetProcessMemoryInfo'),
                                          ('kernel32', 'K32GetProcessMemoryInfo')):
            try:
                library = kernel32 if library_name == 'kernel32' else ctypes.WinDLL(library_name, use_last_error=True)
                get_memory_info = getattr(library, entry_point)
                break
            except (OSError, AttributeError):
                continue

        if get_memory_info is None:
            return 0

        kernel32.GetCurrentProcess.argtypes = []
        kernel32.GetCurrentProcess.restype = ctypes.c_void_p

        get_memory_info.argtypes = [ctypes.c_void_p, ctypes.POINTER(_ProcessMemoryCounters), ctypes.c_ulong]
        get_memory_info.restype = ctypes.c_int

        counters = _ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(_ProcessMemoryCounters)

        if not get_memory_info(kernel32.GetCurrentProcess(), ctypes.byref(counters), counters.cb):
            return 0

        return int(counters.PeakWorkingSetSize)
    except (OSError, AttributeError, ValueError):
        return 0
