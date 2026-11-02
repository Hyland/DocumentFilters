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
import sys
from typing import BinaryIO, List, Optional

from ..configuration.shared_options import ConsoleOptions
from .i_console import IConsole

# Built from the code point rather than written as an escape sequence, so no
# source file in any port carries a literal control byte — those survive a
# copy-paste or a diff viewer badly.
_ESCAPE = chr(27)

_LINE_TERMINATOR = os.linesep
"""
What ends a line, written by this class rather than by the stream.

Console.WriteLine ends a line with Environment.NewLine, which is this, and leaves the
text it was given alone. Python's print() ends a line with '\\n' and lets the stream
translate it, which on Windows turns *every* '\\n' in the text into '\\r\\n' as well --
and an engine string can hold one: an option description in the options catalogue is
several lines of Markdown table, and the reference prints those line breaks exactly as
the engine gave them. See _use_verbatim_newlines.
"""


class SystemConsole(IConsole):
    """
    The real console. Honours --silent, --verbose and --color.

    --color is applied here, by the sink, and only where the sink already knows
    what a line *is*: an error is red, verbose chatter is dim. Report output —
    what the user actually asked for — is never coloured, because it is routinely
    piped into something else and because this class cannot tell one part of a
    report from another. Colouring those would mean giving IConsole semantic roles
    for call sites to choose from, which is a wider contract for every port to
    mirror than a sample warrants.
    """

    _RED = _ESCAPE + '[31m'
    _DIM = _ESCAPE + '[2m'
    _RESET = _ESCAPE + '[0m'

    def __init__(self, options: ConsoleOptions, color_errors: Optional[bool] = None,
                 color_verbose: Optional[bool] = None):
        """
        Args:
            options: Console behavior options.
            color_errors: Override color decision for stderr (tests only).
            color_verbose: Override color decision for stdout (tests only).
        """
        self._options = options
        _use_utf8_streams()
        _use_verbatim_newlines()

        # Decides per stream whether colour is wanted and would be seen.
        # Redirection is checked once, here, rather than per line: a redirected stream
        # is a file or a pipe, and escape sequences written into one are noise in
        # whatever reads it next. The two streams are asked separately because they
        # are redirected separately — 2>log leaves stdout a terminal.
        if color_errors is None:
            color_errors = self.should_color(options.color, not sys.stderr.isatty())
        if color_verbose is None:
            color_verbose = self.should_color(options.color, not sys.stdout.isatty())

        self._color_errors = color_errors
        self._color_verbose = color_verbose

    @staticmethod
    def should_color(requested: bool, stream_redirected: bool) -> bool:
        """
        Whether to colour a stream: asked for, and going somewhere that can show it.

        Named and pure so the rule can be checked without a terminal, and so each
        port mirrors one expression rather than re-deriving it. Detecting redirection
        is the ambient half and stays at the call site.
        """
        return requested and not stream_redirected

    def write(self, text: str) -> None:
        """--silent means "print nothing but errors", so this is a no-op under it."""
        if self._options.silent:
            return
        sys.stdout.write(text)
        sys.stdout.flush()

    def write_line(self, text: str) -> None:
        if self._options.silent:
            return
        sys.stdout.write(text + _LINE_TERMINATOR)

    def write_error(self, text: str) -> None:
        """
        Always written, even under --silent: stderr is the contract.

        Colour wraps the line without altering it, so the contract text is still
        exactly what a reader sees once the terminal has consumed the sequences —
        and is byte-for-byte unchanged whenever stderr is redirected, which is how
        the conformance tests and every script read it.
        """
        if self._color_errors:
            sys.stderr.write(self._RED + text + self._RESET + _LINE_TERMINATOR)
        else:
            sys.stderr.write(text + _LINE_TERMINATOR)

    def write_verbose(self, text: str) -> None:
        if not self._options.verbose or self._options.silent:
            return

        if self._color_verbose:
            sys.stdout.write(self._DIM + text + self._RESET + _LINE_TERMINATOR)
        else:
            sys.stdout.write(text + _LINE_TERMINATOR)

    def open_standard_output(self) -> BinaryIO:
        return sys.stdout.buffer

    @property
    def is_input_interactive(self) -> bool:
        return sys.stdin.isatty()

    def read_all_input_lines(self) -> List[str]:
        return [line.rstrip('\n') for line in sys.stdin]

    def read_password(self, prompt: str) -> Optional[str]:
        """Escape cancels and returns None; Enter accepts what was typed so far."""
        sys.stdout.write(prompt)
        sys.stdout.flush()
        password = []

        while True:
            key = _read_key()

            if key in ('\r', '\n'):
                _end_line()
                return ''.join(password)

            if key == _ESCAPE:
                _end_line()
                return None

            if key in ('\b', '\x7f'):
                if password:
                    password.pop()
                    sys.stdout.write('\b \b')
                    sys.stdout.flush()
                continue

            if not _is_control(key):
                password.append(key)
                sys.stdout.write('*')
                sys.stdout.flush()

    def wait_for_key(self) -> None:
        _read_key()


def _end_line() -> None:
    """
    Console.WriteLine() with nothing to write, which the prompt needs twice.

    Flushed, because the two callers are moving a real terminal's cursor off the line the
    password was typed on, and because the prompt that put it there was flushed for the
    same reason.
    """
    sys.stdout.write(_LINE_TERMINATOR)
    sys.stdout.flush()


def _use_utf8_streams() -> None:
    """
    Puts the three standard streams into UTF-8, which the reference gets for free on the
    two platforms where it is right.

    On Linux and macOS .NET writes a redirected stdout as UTF-8 without a BOM, and reads a
    redirected stdin the same way. Python instead uses the locale encoding, which is
    cp1252 on a default Windows install — so a report naming a document whose metadata
    holds an em dash, or extracted text in any script outside Latin-1, raised
    UnicodeEncodeError and ended the run in a traceback rather than in one of the exit
    codes the contract defines. Attached to a real console Python already uses the wide
    API and needs none of this; it is redirection, which is how the conformance tests and
    every pipeline run the tool, that diverged.

    On Windows the reference is not the thing to copy here. .NET encodes a redirected
    stream with the console's code page when the process has a console and with the ANSI
    code page when it has none, both with best-fit substitution — so `identify` on a file
    whose name holds "é—" writes cp850 "\\x82--" from a shell and cp1252 "\\xe9\\x97"
    detached, and one machine disagrees with itself depending on how the tool was
    started. Matching that would mean choosing to corrupt output, and would make any
    byte comparison between the two implementations a property of the console the
    comparison ran in. So this port writes UTF-8 everywhere, which is what the reference
    writes everywhere but Windows; the remaining difference closes from the other side,
    when the reference sets Console.OutputEncoding.

    errors='replace' rather than the default 'strict' because Encoding.UTF8 — the property,
    not new UTF8Encoding(true, true) — carries a replacement fallback, so the reference
    writes U+FFFD for something unencodable where strict would raise. This only affects the
    report and diagnostic lines: document bytes reach stdout through
    open_standard_output, which is binary and untouched by any of this.

    Guarded by hasattr because a harness may have replaced a stream with something that is
    not a TextIOWrapper, and by an encoding check so an already-UTF-8 stream is left alone
    rather than rewrapped.
    """
    for stream in (sys.stdin, sys.stdout, sys.stderr):
        if stream is None or not hasattr(stream, 'reconfigure'):
            continue

        encoding = getattr(stream, 'encoding', None)
        if encoding is not None and encoding.replace('_', '-').lower() in ('utf-8', 'utf8'):
            continue

        try:
            stream.reconfigure(encoding='utf-8', errors='replace')
        except (ValueError, OSError):
            # A stream that refuses to be reconfigured is one already committed to an
            # encoding, which is the caller's choice to have made and not worth failing the
            # run over.
            pass


def _use_verbatim_newlines() -> None:
    """
    Stops the output streams rewriting line breaks inside the text they are given.

    Another thing the reference gets for free. A Python text stream opened for writing
    with no newline of its own translates every '\\n' written through it into os.linesep,
    which on Windows is '\\r\\n'. That is right for the line break this class puts at the
    end of a line and wrong for one already inside the text: an option description in the
    options catalogue is several lines of Markdown table, and the engine spells those
    breaks '\\n'. The reference prints them exactly as given and Python turned each into
    '\\r\\n' -- 41 bytes of difference across `docfilters options`, which is exactly the
    kind of drift the conformance fixtures exist to catch and the one place they could not
    see, because nothing above a console can tell what the console did to it.

    So translation is turned off and _LINE_TERMINATOR is written explicitly instead.
    stdin is deliberately left alone: universal newlines is what makes read_all_input_lines
    agree with Console.In.ReadLine, which strips '\\r\\n' and '\\n' alike.

    Guarded exactly as _use_utf8_streams is, and for the same reasons -- with no encoding
    check, because a stream already in UTF-8 may still be translating newlines.
    """
    for stream in (sys.stdout, sys.stderr):
        if stream is None or not hasattr(stream, 'reconfigure'):
            continue

        try:
            stream.reconfigure(newline='')
        except (ValueError, OSError):
            pass


def _is_control(char: str) -> bool:
    """
    Whether a character is a control character, matching char.IsControl: the C0
    range and the C1 range, and nothing else.

    Not str.isprintable() inverted — that also rejects every space-separator and
    unassigned code point, so a non-breaking space would be dropped from a password
    on Python but kept on .NET.
    """
    code = ord(char)
    return code <= 0x1f or 0x7f <= code <= 0x9f


def _read_key() -> str:
    """
    One keypress, not echoed and without waiting for a line.

    Split by platform because there is no portable way to do it: Windows has a
    console API for it, and elsewhere the terminal has to be put into raw mode for
    the duration of the read.
    """
    if os.name == 'nt':
        import msvcrt
        return msvcrt.getwch()

    import termios
    import tty

    descriptor = sys.stdin.fileno()
    saved = termios.tcgetattr(descriptor)
    try:
        tty.setraw(descriptor)
        return sys.stdin.read(1)
    finally:
        termios.tcsetattr(descriptor, termios.TCSADRAIN, saved)
