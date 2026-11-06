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
from typing import List, Optional

from docfilters_cli.commands.convert_command import ConvertCommand
from docfilters_cli.commands.i_command import CommandContext
from docfilters_cli.commands.identify_command import IdentifyCommand
from docfilters_cli.configuration.command_options import ConvertOptions, IdentifyOptions
from docfilters_cli.configuration.shared_options import OnErrorBehaviour, SharedOptions
from docfilters_cli.engine.engine_exception import EngineError, EngineException
from docfilters_cli.execution.document_filters_session import DocumentFiltersSession
from docfilters_cli.planning.format_catalog import FormatCatalog
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import CliError, ErrorCodes, InitialisationError
from docfilters_cli.runtime.interruption import Interruption, RunInterruptedError
from fakes.fake_document_filters_engine import FakeDocumentFiltersEngine
from fakes.in_memory_file_system import InMemoryFileSystem
from fakes.recording_console import RecordingConsole


class EngineOutcomeTests(unittest.TestCase):
    """
    What the CLI does with each way Document Filters can fail.

    The engine is the one dependency the execution paths could not be tested around: it
    needs a license, a native library, and a document that actually exhibits the outcome
    under test. Behind IDocumentFiltersEngine every outcome is scriptable, so the question
    these tests ask is the one that matters -- not "does the engine do this", which is
    Hyland's to answer, but "when it does, does the CLI behave".

    The codes are not invented: EngineError has one member per IGR_E_* constant in
    ISYS11dfConstants, carrying the constant's own value.
    """

    # ============================================ opening a document

    def test_any_open_failure_becomes_one_failed_input(self):
        """
        Whatever the engine says went wrong, one bad input is one failed input.

        This is the property the whole batch model rests on: an engine failure has to
        arrive as a DocumentError naming the input, not as an exception that ends the
        process. An EngineException escaping a command would bypass run_plan entirely --
        --on-error skip would stop skipping, and the exit code would be whatever program
        makes of an unhandled exception rather than 1.
        """
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                engine = FakeDocumentFiltersEngine()
                engine.add_document("good.docx", "hello")
                engine.fail_to_open("bad.docx", error)

                console = RecordingConsole()
                code = _run_convert(engine, console, _shared(), "good.docx", "bad.docx")

                self.assertEqual(exit_code.PARTIAL_FAILURE, code)
                self.assertTrue(any("bad.docx" in line for line in console.error_lines))
                self.assertFalse(any("good.docx" in line for line in console.error_lines))

    def test_any_open_failure_stops_the_run_under_fail_fast(self):
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                engine = FakeDocumentFiltersEngine()
                engine.fail_to_open("first.docx", error)
                engine.add_document("second.docx", "hello")

                console = RecordingConsole()
                shared = _shared()
                shared.batch.on_error = OnErrorBehaviour.STOP

                code = _run_convert(engine, console, shared, "first.docx", "second.docx")

                self.assertEqual(exit_code.PARTIAL_FAILURE, code)

                # The second document is never opened, which is the whole point of stopping.
                self.assertFalse(any("second.docx" in call for call in engine.calls))

    def test_a_document_that_opens_is_converted_and_closed(self):
        engine = FakeDocumentFiltersEngine()
        document = engine.add_document("a.docx", "the text")

        code = _run_convert(engine, RecordingConsole(), _shared(), "a.docx")

        self.assertEqual(exit_code.SUCCESS, code)
        self.assertTrue(document.was_opened)
        self.assertTrue(document.is_disposed)

    # ============================================ opening a document

    def test_an_interrupt_while_a_document_opens_exits_130_and_does_not_report_a_timeout(self):
        """
        Ctrl+C arriving while a document is opening exits 130, not a timeout error.

        Both causes reach the engine through one predicate and come back as one
        EngineError.CANCELLED, and every command used to translate that into a timeout
        unconditionally -- which reads options.timeout. With no --timeout given there is
        no value, so an interrupt during open failed on None rather than reporting
        anything. The cause has to be recovered at the catch, because the engine cannot
        carry it.

        The interrupt is requested from inside the open: the batch runner checks between
        documents, so one requested any earlier stops the run before anything opens and
        never reaches this path at all.
        """
        engine = FakeDocumentFiltersEngine()
        engine.add_document("a.docx", "body")
        console = RecordingConsole()
        interruption = Interruption()
        engine.during_open = interruption.request

        shared = _shared()
        shared.input.paths.append("a.docx")

        context = _context(engine, console, shared)
        context.interruption = interruption

        options = ConvertOptions()
        options.format = FormatCatalog.resolve("text")
        command = ConvertCommand(options)

        # Raised, not returned: RunInterruptedError travels past the batch runner -- which
        # only absorbs DocumentError -- and program turns it into the exit code. What
        # matters here is which error it is, and that it is a CliError at all rather than
        # the failure on None that used to end the process.
        with self.assertRaises(CliError) as caught:
            command.execute(context)

        error = caught.exception
        self.assertIsInstance(error, RunInterruptedError)
        self.assertEqual(exit_code.INTERRUPTED, error.exit_code)
        self.assertNotIn("--timeout", error.message)

    # ============================================ passwords

    def test_the_right_password_opens_a_protected_document(self):
        engine = FakeDocumentFiltersEngine()
        engine.add_protected_document("locked.docx", "hunter2", "secret text")

        shared = _shared()
        shared.passwords.passwords.append("hunter2")

        self.assertEqual(exit_code.SUCCESS,
                         _run_convert(engine, RecordingConsole(), shared, "locked.docx"))

    def test_a_wrong_password_fails_that_document_only(self):
        engine = FakeDocumentFiltersEngine()
        engine.add_protected_document("locked.docx", "hunter2", "secret")
        engine.add_document("open.docx", "plain")

        shared = _shared()
        shared.passwords.passwords.append("wrong")

        console = RecordingConsole()
        code = _run_convert(engine, console, shared, "locked.docx", "open.docx")

        self.assertEqual(exit_code.PARTIAL_FAILURE, code)
        self.assertTrue(any("locked.docx" in line for line in console.error_lines))

    def test_no_password_at_all_fails_the_same_way_a_wrong_one_does(self):
        # The engine reports both as IGR_E_PASSWORD, so the CLI cannot tell them
        # apart and must not pretend to.
        engine = FakeDocumentFiltersEngine()
        engine.add_protected_document("locked.docx", "hunter2", "secret")

        console = RecordingConsole()
        code = _run_convert(engine, console, _shared(), "locked.docx")

        self.assertEqual(exit_code.PARTIAL_FAILURE, code)
        self.assertTrue(any("locked.docx" in line for line in console.error_lines))

    # ============================================ failures after opening

    def test_a_read_failure_after_opening_is_still_one_failed_input(self):
        """
        A failure part-way through reading is still one failed input, not a crash.

        Worth separating from an open failure because the document is open when it happens:
        the handle has to be closed on the way out, and the batch has to carry on. A
        corrupt document that opens and then fails on read is the common shape of it.
        """
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                engine = FakeDocumentFiltersEngine()
                broken = engine.add_document("broken.docx", "some text")
                broken.read_failure = error
                engine.add_document("fine.docx", "text")

                console = RecordingConsole()
                code = _run_convert(engine, console, _shared(), "broken.docx", "fine.docx")

                self.assertEqual(exit_code.PARTIAL_FAILURE, code)
                self.assertTrue(any("broken.docx" in line for line in console.error_lines))
                self.assertTrue(broken.is_disposed,
                                "the document must be closed even when reading it failed")

    # ============================================ identify

    def test_identify_reports_a_failure_per_input_and_keeps_going(self):
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                engine = FakeDocumentFiltersEngine()
                engine.fail_to_open("bad.bin", error)
                good = engine.add_document("good.bin")
                good.file_type = 254
                good.mime_type = "application/vnd.ms-word"
                good.format_name = "Microsoft Word"

                console = RecordingConsole()
                command = IdentifyCommand(IdentifyOptions())
                shared = _shared(out_dir=None)
                shared.input.paths.append("bad.bin")
                shared.input.paths.append("good.bin")

                code = command.execute(_context(engine, console, shared))

                self.assertEqual(exit_code.PARTIAL_FAILURE, code)
                self.assertTrue(any("good.bin" in line and "format=254" in line
                                    for line in console.out_lines))

    def test_identify_never_opens_the_document(self):
        # identify asks what a file is, not what it says. Opening it would need a
        # password for a protected document and would read bytes it has no use for.
        engine = FakeDocumentFiltersEngine()
        engine.add_protected_document("locked.docx", "hunter2")

        console = RecordingConsole()
        shared = _shared(out_dir=None)
        shared.input.paths.append("locked.docx")

        code = IdentifyCommand(IdentifyOptions()).execute(_context(engine, console, shared))

        self.assertEqual(exit_code.SUCCESS, code)
        self.assertTrue(any(call.startswith("IdentifyFile(") for call in engine.calls))
        self.assertFalse(any(call.startswith("OpenDocument(") for call in engine.calls))

    # ============================================ initialisation

    def test_an_initialisation_failure_is_not_a_document_failure(self):
        # Nothing was processed, so this is the run never starting: exit 5, not the
        # partial-failure 1 that means "some files did not work".
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                engine = FakeDocumentFiltersEngine()
                engine.initialisation_failure = error

                with self.assertRaises(InitialisationError) as caught:
                    _document_filters_session_factory(engine)

                self.assertEqual(exit_code.INITIALISATION_FAILURE, caught.exception.exit_code)
                self.assertEqual(ErrorCodes.INIT_FAILED, caught.exception.code)


def _every_engine_error() -> List[EngineError]:
    """
    Every failure the engine can report of its own accord.

    EngineError.NONE is not a failure, and EngineError.CANCELLED is not the engine's: it is
    only ever raised because this tool asked, through the open callback, so the caller that
    asked translates it into its own error -- a --timeout reported as E_LIMIT_EXCEEDED
    rather than a document reported as broken. Sweeping it here would assert the opposite.

    The reference's [MemberData] source becomes a plain function returning a list, because
    unittest has no data-driven attribute and each caller loops over it in subTests.
    Iterating the enum type itself is Python's Enum.GetValues, and skips aliases the same
    way.
    """
    return [error for error in EngineError
            if error != EngineError.NONE and error != EngineError.CANCELLED]


def _context(engine: FakeDocumentFiltersEngine, console: RecordingConsole,
             shared: SharedOptions) -> CommandContext:
    """
    Builds the context a command executes against.

    The inputs have to exist in two places: on the fake filesystem, because the walk
    selects them before the engine is ever asked, and in the fake engine, because that is
    what opening them consults. A path present in only one of the two is the setup mistake
    this helper exists to prevent.
    """
    file_system = InMemoryFileSystem()
    for path in shared.input.paths:
        file_system.add_file(path)

    context = CommandContext()
    context.shared_options = shared
    context.console = console
    context.file_system = file_system
    context.session = DocumentFiltersSession.for_engine(engine)
    return context


def _shared(out_dir: Optional[str] = "out") -> SharedOptions:
    shared = SharedOptions()
    shared.output.out_dir = out_dir
    return shared


def _document_filters_session_factory(
        engine: FakeDocumentFiltersEngine) -> DocumentFiltersSession:
    """Mirrors what DocumentFiltersSession.create does, against the fake."""
    try:
        engine.initialize(None, ".")
    except EngineException as error:
        raise InitialisationError("could not initialise Document Filters: " + error.message)

    return DocumentFiltersSession.for_engine(engine)


def _run_convert(engine: FakeDocumentFiltersEngine, console: RecordingConsole,
                 shared: SharedOptions, *paths: str) -> int:
    """
    Converts paths to text and returns the exit code.

    The reference's two overloads collapse to one signature: Python has no overloading, and
    the shorter of the two only existed to default the shared options, which every caller
    here now spells as _shared().
    """
    for path in paths:
        shared.input.paths.append(path)

    options = ConvertOptions()
    options.format = FormatCatalog.resolve("text")
    command = ConvertCommand(options)

    return command.execute(_context(engine, console, shared))
