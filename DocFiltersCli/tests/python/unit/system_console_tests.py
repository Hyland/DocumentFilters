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

import contextlib
import io
import os
import subprocess
import sys
import unittest

import docfilters_cli
from docfilters_cli.configuration.shared_options import ConsoleOptions
from docfilters_cli.runtime.system_console import SystemConsole

ESCAPE = chr(27)

# The directory holding the package, so a subprocess can import it the same way this
# process did without depending on how the suite was launched.
_PACKAGE_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(docfilters_cli.__file__)))


def options(verbose: bool = False, silent: bool = False) -> ConsoleOptions:
    result = ConsoleOptions()
    result.color = True
    result.verbose = verbose
    result.silent = silent
    return result


def capture_error(console, write) -> str:
    """Runs write and returns what landed on stderr."""
    captured = io.StringIO()
    with contextlib.redirect_stderr(captured):
        write(console)
    return captured.getvalue()


def capture_out(console, write) -> str:
    """Runs write and returns what landed on stdout."""
    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
        write(console)
    return captured.getvalue()


class SystemConsoleTests(unittest.TestCase):
    """
    --color, asserted against the real console rather than a fake.

    RecordingConsole deliberately knows nothing about colour: its error_lines are the
    cross-language contract and are asserted character for character, so letting escape
    sequences into them would mean every one of those assertions had to strip ANSI
    first — and a wrong sequence would then pass. Colour is presentation, the fake
    captures content. That leaves SystemConsole as the only thing that can be asserted
    on here, which is the point: a flag tested only through "the option got set" is
    what let nine others sit unimplemented.

    These tests redirect sys.stdout and sys.stderr, which is process-wide state.
    unittest runs a class's tests serially, the redirection is undone on the way out,
    and nothing else in this suite writes to the real console.
    """

    def test_colour_is_used_only_when_asked_for_and_the_stream_can_show_it(self):
        for requested, redirected, expected in [(True, False, True),    # asked for, terminal
                                                (True, True, False),    # asked for, but redirected
                                                (False, False, False),  # not asked for
                                                (False, True, False)]:
            with self.subTest(requested=requested, redirected=redirected):
                self.assertEqual(expected, SystemConsole.should_color(requested, redirected))

    def test_an_error_is_wrapped_in_red(self):
        console = SystemConsole(options(), color_errors=True, color_verbose=True)

        written = capture_error(console, lambda c: c.write_error("E_DOCUMENT_FAILED: boom"))

        self.assertEqual(ESCAPE + "[31m" + "E_DOCUMENT_FAILED: boom" + ESCAPE + "[0m",
                         written.rstrip('\r\n'))

    def test_an_error_carries_no_escapes_when_colour_is_off(self):
        console = SystemConsole(options(), color_errors=False, color_verbose=False)

        written = capture_error(console, lambda c: c.write_error("E_DOCUMENT_FAILED: boom"))

        self.assertEqual("E_DOCUMENT_FAILED: boom", written.rstrip('\r\n'))

    def test_verbose_chatter_is_dimmed(self):
        console = SystemConsole(options(verbose=True), color_errors=True, color_verbose=True)

        written = capture_out(console, lambda c: c.write_verbose("opening a.pdf"))

        self.assertEqual(ESCAPE + "[2m" + "opening a.pdf" + ESCAPE + "[0m", written.rstrip('\r\n'))

    def test_report_output_is_never_coloured(self):
        """
        The deliberate half of the decision: --color must not reach the answer the user
        asked for, which is routinely piped into something that would choke on escapes.
        A change that colours reports uniformly would pass every test above and fail
        this one.
        """
        console = SystemConsole(options(), color_errors=True, color_verbose=True)

        line = capture_out(console, lambda c: c.write_line("a.pdf: format=51"))
        chunk = capture_out(console, lambda c: c.write("a.pdf: format=51"))

        self.assertEqual("a.pdf: format=51", line.rstrip('\r\n'))
        self.assertEqual("a.pdf: format=51", chunk)

    def test_silent_suppresses_coloured_report_output_but_not_coloured_errors(self):
        """
        --silent still wins over --color: an error is the one thing that survives it,
        and it survives coloured.
        """
        console = SystemConsole(options(verbose=True, silent=True), color_errors=True, color_verbose=True)

        self.assertEqual("", capture_out(console, lambda c: c.write_line("a.pdf: format=51")))
        self.assertEqual("", capture_out(console, lambda c: c.write_verbose("opening a.pdf")))

        error = capture_error(console, lambda c: c.write_error("E_DOCUMENT_FAILED: boom"))
        self.assertEqual(ESCAPE + "[31m" + "E_DOCUMENT_FAILED: boom" + ESCAPE + "[0m",
                         error.rstrip('\r\n'))

    def test_a_redirected_stream_is_written_as_utf8(self):
        """
        Python-only. .NET writes a redirected stdout as UTF-8 on every platform; Python
        uses the locale encoding, so on a default Windows install a report holding an em
        dash ended the run in UnicodeEncodeError instead of an exit code from the table.

        Driven through a real subprocess with its output redirected, because that is the
        only arrangement in which the two disagree — attached to a console Python already
        uses the wide API, and a StringIO has no encoding to get wrong.
        """
        # The text is spelled with escapes so the script itself is pure ASCII: passing
        # non-ASCII through -c would make this test depend on how the platform decodes a
        # command line, which is not what it is here to check.
        text = "em dash — CJK 文字"
        script = (
            "import sys\n"
            "sys.path.insert(0, %r)\n"
            "from docfilters_cli.configuration.shared_options import ConsoleOptions\n"
            "from docfilters_cli.runtime.system_console import SystemConsole\n"
            "SystemConsole(ConsoleOptions(), color_errors=False, color_verbose=False)"
            ".write_line('em dash \\u2014 CJK \\u6587\\u5b57')\n"
        ) % (_PACKAGE_ROOT,)

        completed = subprocess.run([sys.executable, '-c', script], stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)

        self.assertEqual(0, completed.returncode, completed.stderr.decode('utf-8', 'replace'))
        self.assertEqual(text, completed.stdout.decode('utf-8').rstrip('\r\n'))

    def test_a_line_break_inside_the_text_is_written_exactly_as_given(self):
        """
        Python-only, and the same shape of problem as the encoding above.

        Console.WriteLine ends the line with Environment.NewLine and leaves the text alone.
        A Python text stream with no newline of its own translates every '\\n' written
        through it, so print() turned a line break *inside* the text into '\\r\\n' on
        Windows as well. An option description in the options catalogue is several lines of
        Markdown table, and that is 41 bytes of difference between the two ports on
        `docfilters options` -- invisible to every fake, because a fake records the string
        the console was handed rather than what the console did with it.

        So this is driven through a real subprocess with its output redirected, exactly as
        the encoding test is, and asserted on the bytes: the terminator is the platform's,
        and the break in the middle is the one the caller wrote.
        """
        script = (
            "import sys\n"
            "sys.path.insert(0, %r)\n"
            "from docfilters_cli.configuration.shared_options import ConsoleOptions\n"
            "from docfilters_cli.runtime.system_console import SystemConsole\n"
            "SystemConsole(ConsoleOptions(), color_errors=False, color_verbose=False)"
            ".write_line('| a |\\n| b |')\n"
        ) % (_PACKAGE_ROOT,)

        completed = subprocess.run([sys.executable, '-c', script], stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)

        self.assertEqual(0, completed.returncode, completed.stderr.decode('utf-8', 'replace'))
        self.assertEqual(b'| a |\n| b |' + os.linesep.encode('ascii'), completed.stdout)


if __name__ == '__main__':
    unittest.main()
