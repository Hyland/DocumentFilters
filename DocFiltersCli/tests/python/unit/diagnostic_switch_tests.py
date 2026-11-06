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

import io
import unittest

from docfilters_cli.commands import command_execution
from docfilters_cli.commands.convert_command import ConvertCommand
from docfilters_cli.configuration.command_options import ConvertOptions
from docfilters_cli.configuration.shared_options import DiagnosticOptions, SharedOptions
from docfilters_cli.execution import debug_input, engine_log
from docfilters_cli.execution.canvas_conversion import CanvasConversion
from docfilters_cli.execution.diagnostics import Diagnostics
from docfilters_cli.execution.document_filters_session import DocumentFiltersSession
from docfilters_cli.execution.engine_log import EngineLog
from docfilters_cli.execution.extractor_open_settings import ExtractorOpenSettings
from docfilters_cli.execution.stop_signal import StopReason
from docfilters_cli.planning.format_catalog import FormatCatalog
from docfilters_cli.planning.run_plan import (OutputCardinality, PlannedItem, RunPlan,
                                              TargetPattern)
from docfilters_cli.runtime.cli_error import DocumentError
from docfilters_cli.runtime.interruption import Interruption, RunInterruptedError
from fakes.fake_document_filters_engine import FakeDocumentFiltersEngine
from fakes.in_memory_file_system import InMemoryFileSystem
from fakes.recording_console import RecordingConsole


class _TestClock:
    """
    A clock a test moves by hand, so a deadline can pass instantly.

    Seconds as a float, because that is what Diagnostics' default clock,
    time.perf_counter, reads in. The C# reads a TimeSpan.
    """

    def __init__(self):
        self.now = 0.0

    def read(self) -> float:
        return self.now

    def advance(self, seconds: float) -> None:
        self.now += seconds


class DiagnosticSwitchTests(unittest.TestCase):
    """
    The diagnostic switches, each of which used to parse and then do nothing.

    Nine arguments were accepted, validated, and never read again. That is worse than not
    having them: the help text promises something, the validator agrees the command line is
    sensible, and the run behaves exactly as if the argument had been left out. These tests
    are what stops one quietly returning to that state.
    """

    def test_temp_dir_is_given_to_the_engine(self):
        # A library-wide setting, applied once after initialize rather than per
        # document: IGR_Multiplex(IGR_Multi_Set_Temp_Path), as doc2text does it.
        engine = FakeDocumentFiltersEngine()

        DocumentFiltersSession.for_engine(engine)
        engine.set_temp_path("/scratch")

        self.assertEqual("/scratch", engine.temp_path)
        self.assertIn("SetTempPath(/scratch)", engine.calls)

    def test_engine_memory_is_reported_at_the_level_asked_for(self):
        for level, per_document, per_run in ((0, False, False),   # neither switch
                                             (1, False, True),    # -x: the run only
                                             (2, True, True)):    # --xx: every document too
            with self.subTest(level=level):
                engine = FakeDocumentFiltersEngine()
                engine.memory_status = "heap: 42 bytes"
                console = RecordingConsole()
                options = DiagnosticOptions()
                options.memory_status_level = level
                diagnostics = Diagnostics(options, console, engine)

                diagnostics.begin_run()
                diagnostics.begin_document("a.docx", 1, 1)
                diagnostics.end_document("a.docx")
                after_document = len(console.out_lines)
                diagnostics.end_run(1, 0)

                self.assertEqual(1 if per_document else 0, after_document)
                self.assertEqual((1 if per_document else 0) + (1 if per_run else 0),
                                 len(console.out_lines))

                if per_run:
                    self.assertTrue(any("heap: 42 bytes" in line for line in console.out_lines))

    def test_logmem_reports_the_processes_own_peak(self):
        # The process, not the engine: -x and --xx answer the other question.
        console = RecordingConsole()
        options = DiagnosticOptions()
        options.log_memory = True
        Diagnostics(options, console).end_run(1, 0)

        self.assertTrue(any(line.startswith("peak memory: ") and line.endswith("MB")
                            for line in console.out_lines))

    # ------------------------------------------------ the stop predicate

    def test_an_interrupt_outranks_a_deadline_that_also_passed(self):
        """
        When the deadline has passed *and* the user has interrupted, the run reports the
        interrupt.

        The reason is settled by the poll that stopped the document rather than worked out
        afterwards, which is what makes this answerable at all: by the time a cancelled
        open is caught both conditions can be true, and asking then is a second guess at a
        question already answered.
        """
        clock = _TestClock()
        diagnostics = Diagnostics(_diagnostics_with_timeout(30), RecordingConsole(),
                                  engine=None, clock=clock.read)
        diagnostics.begin_document("slow.docx", 1, 1)
        clock.advance(31)

        interruption = Interruption()
        interruption.request()

        stop = command_execution.watch_for_stop(diagnostics, interruption)

        self.assertTrue(stop.should_stop())
        self.assertEqual(StopReason.INTERRUPTED, stop.reason)
        self.assertIsInstance(stop.cause("slow.docx"), RunInterruptedError)

    def test_a_cancellation_with_no_recorded_reason_is_a_document_error(self):
        """A cancelled open nobody asked for is the document failing, not a limit."""
        diagnostics = Diagnostics(DiagnosticOptions(), RecordingConsole())
        diagnostics.begin_document("a.docx", 1, 1)

        stop = command_execution.watch_for_stop(diagnostics, None)

        self.assertFalse(stop.should_stop())
        self.assertIsInstance(stop.cause("a.docx"), DocumentError)

    def test_nothing_stops_a_run_inside_its_limits_that_nobody_interrupted(self):
        """
        Shared by every command that opens a document.

        It was four copies of the same two lines until this moved, which is four places for
        one of them to quietly lose a term -- and neither term is visible from the outside,
        since both produce the same cancelled open.
        """
        diagnostics = Diagnostics(_diagnostics_with_timeout(600), RecordingConsole())
        diagnostics.begin_document("a.docx", 1, 1)

        self.assertFalse(command_execution.watch_for_stop(diagnostics, Interruption()).should_stop())
        self.assertFalse(command_execution.watch_for_stop(diagnostics, None).should_stop())

    def test_the_documents_deadline_stops_it(self):
        clock = _TestClock()
        diagnostics = Diagnostics(_diagnostics_with_timeout(30), RecordingConsole(),
                                  engine=None, clock=clock.read)
        diagnostics.begin_document("slow.docx", 1, 1)
        clock.advance(31)

        stop = command_execution.watch_for_stop(diagnostics, None)

        self.assertTrue(stop.should_stop())
        self.assertEqual(StopReason.TIMED_OUT, stop.reason)

    def test_an_interrupt_stops_it_even_with_time_to_spare(self):
        diagnostics = Diagnostics(_diagnostics_with_timeout(600), RecordingConsole())
        diagnostics.begin_document("a.docx", 1, 1)

        interruption = Interruption()
        stop = command_execution.watch_for_stop(diagnostics, interruption)

        self.assertFalse(stop.should_stop())
        self.assertIsNone(stop.reason)

        interruption.request()

        self.assertTrue(stop.should_stop())
        self.assertEqual(StopReason.INTERRUPTED, stop.reason)

    # ------------------------------------------------------- --no-images

    def test_no_images_suppresses_them_for_the_markdown_formats(self):
        for token in ("md", "md-text"):
            with self.subTest(token=token):
                self.assertEqual("MARKDOWN_INCLUDE_IMAGES=OFF",
                                 CanvasConversion.image_suppression(FormatCatalog.resolve(token)))

    def test_no_images_is_ignored_where_it_has_no_meaning(self):
        for token in ("html5",   # inlines the page raster itself -- suppressing would blank it
                      "svg",
                      "text",    # nowhere to put an image in the first place
                      "pdf"):
            with self.subTest(token=token):
                self.assertEqual("", CanvasConversion.image_suppression(
                    FormatCatalog.resolve(token)))

    def test_no_images_reaches_the_canvas_options_for_hi_def_markdown(self):
        self.assertEqual("MARKDOWN_INCLUDE_IMAGES=OFF",
                         CanvasConversion.compose_canvas_options(
                             FormatCatalog.resolve("md"), None, no_images=True))

        self.assertEqual("", CanvasConversion.compose_canvas_options(
            FormatCatalog.resolve("md"), None, no_images=False))

    def test_no_images_reaches_the_open_options_for_text_mode_markdown(self):
        # md-text never opens a canvas, so the same rule has to arrive at open.
        options = ConvertOptions()
        options.no_images = True
        settings = ExtractorOpenSettings.for_format(
            FormatCatalog.resolve("md-text"), options, SharedOptions())

        self.assertIn("MARKDOWN_INCLUDE_IMAGES=OFF", settings.options)

    # ----------------------------------------------- --log-level/--log-modules

    def test_no_log_level_means_no_log_at_all(self):
        # None rather than a disabled instance, so the engine is handed no callback
        # and never composes a message nobody prints.
        self.assertIsNone(EngineLog.for_options(DiagnosticOptions(), RecordingConsole()))

    def test_every_module_logs_when_none_are_named(self):
        options = DiagnosticOptions()
        options.log_level = "debug"
        log = EngineLog.for_options(options, RecordingConsole())

        self.assertEqual(engine_log.LEVEL_DEBUG, log.level_for("pdf"))
        self.assertEqual(engine_log.LEVEL_DEBUG, log.level_for("anything"))

    def test_naming_modules_silences_the_rest(self):
        # The filter is applied by answering "none" for a module nobody asked about,
        # which is where --log-modules actually takes effect -- the engine has no
        # notion of the user's list.
        options = DiagnosticOptions()
        options.log_level = "trace"
        options.log_modules = "pdf,ole"
        log = EngineLog.for_options(options, RecordingConsole())

        self.assertEqual(engine_log.LEVEL_TRACE, log.level_for("pdf"))
        self.assertEqual(engine_log.LEVEL_TRACE, log.level_for("OLE"))
        self.assertEqual(engine_log.LEVEL_NONE, log.level_for("html"))

    def test_the_debug_spellings_are_the_same_switch(self):
        options = DiagnosticOptions()
        options.debug_log_level = "warning"
        options.debug_log_modules = "pdf"
        log = EngineLog.for_options(options, RecordingConsole())

        self.assertEqual(engine_log.LEVEL_WARNING, log.level_for("pdf"))
        self.assertEqual(engine_log.LEVEL_NONE, log.level_for("xls"))

    def test_a_log_message_goes_to_stderr_with_its_level_and_module(self):
        # stderr, as doc2text does. It shares the stream with the two contract error
        # shapes, which is safe because nothing logs unless --log-level was passed.
        console = RecordingConsole()
        options = DiagnosticOptions()
        options.log_level = "debug"
        log = EngineLog.for_options(options, console)

        log.write(engine_log.LEVEL_DEBUG, "pdf", "parsing xref")

        self.assertEqual(1, len(console.error_lines))
        self.assertIn("DEBUG [pdf] parsing xref", console.error_lines[0])
        self.assertEqual([], console.out_lines)

    def test_an_unknown_level_logs_nothing_rather_than_failing(self):
        # doc2text's own parse falls through to none; a typo should not end a run
        # that was otherwise fine.
        options = DiagnosticOptions()
        options.log_level = "chatty"
        self.assertIsNone(EngineLog.for_options(options, RecordingConsole()))

    # -------------------------------------------------------------- --view

    def test_view_opens_the_one_file_a_run_produced(self):
        plan = RunPlan()
        plan.cardinality = OutputCardinality.PER_INPUT
        item = PlannedItem()
        item.source_path = "a.docx"
        item.target = "out/a.pdf"
        plan.items.append(item)

        self.assertEqual("out/a.pdf", ConvertCommand.single_output_of(plan))

    def test_view_opens_the_combined_target_when_there_is_one(self):
        plan = RunPlan()
        plan.cardinality = OutputCardinality.COMBINED
        plan.combined_target = "out/book.pdf"
        for path in ("a.docx", "b.docx"):
            item = PlannedItem()
            item.source_path = path
            plan.items.append(item)

        self.assertEqual("out/book.pdf", ConvertCommand.single_output_of(plan))

    def test_view_opens_nothing_when_a_run_produced_several_files(self):
        # "Single output" is the spec's own wording: several inputs, or one input to
        # a per-page format, leave nothing unambiguous to open.
        many = RunPlan()
        many.cardinality = OutputCardinality.PER_INPUT
        for path, target in (("a.docx", "out/a.pdf"), ("b.docx", "out/b.pdf")):
            item = PlannedItem()
            item.source_path = path
            item.target = target
            many.items.append(item)

        self.assertIsNone(ConvertCommand.single_output_of(many))

        per_page = RunPlan()
        per_page.cardinality = OutputCardinality.PER_INPUT
        pattern = TargetPattern()
        pattern.directory = "out"
        pattern.extension = "png"
        item = PlannedItem()
        item.source_path = "a.docx"
        item.target_pattern = pattern
        per_page.items.append(item)

        self.assertIsNone(ConvertCommand.single_output_of(per_page))

    # ------------------------------------------------------ the debug streams

    def test_nothing_wraps_the_input_when_no_debug_switch_is_set(self):
        file_system = InMemoryFileSystem()
        file_system.add_file("a.docx", "hello")

        self.assertIsNone(debug_input.for_document(
            "a.docx", DiagnosticOptions(), file_system, RecordingConsole(), "a.docx"))

    def test_use_memory_stream_reads_the_whole_document_into_memory(self):
        file_system = InMemoryFileSystem()
        file_system.add_file("a.docx", "hello world")

        options = DiagnosticOptions()
        options.use_memory_stream = True

        with debug_input.for_document("a.docx", options, file_system,
                                      RecordingConsole(), "a.docx") as stream:
            self.assertIsNotNone(stream)
            self.assertEqual(11, _length(stream))

    def test_truncate_reads_only_the_first_bytes(self):
        file_system = InMemoryFileSystem()
        file_system.add_file("a.docx", "hello world")

        options = DiagnosticOptions()
        options.truncate_input_bytes = 5

        with debug_input.for_document("a.docx", options, file_system,
                                      RecordingConsole(), "a.docx") as stream:
            self.assertEqual(5, _length(stream))

    def test_truncating_past_the_end_stops_at_the_end(self):
        file_system = InMemoryFileSystem()
        file_system.add_file("a.docx", "short")

        options = DiagnosticOptions()
        options.truncate_input_bytes = 4096

        with debug_input.for_document("a.docx", options, file_system,
                                      RecordingConsole(), "a.docx") as stream:
            self.assertEqual(5, _length(stream))

    def test_measuring_composes_with_a_truncated_source(self):
        # The two switches answer independent questions -- where the bytes come from,
        # and whether the asking is counted -- so asking both must not drop one. An
        # if/else chain here silently ignored --debug-measure-io whenever a
        # truncated or buffered source was also requested.
        file_system = InMemoryFileSystem()
        file_system.add_file("a.docx", "hello world")
        console = RecordingConsole()

        options = DiagnosticOptions()
        options.truncate_input_bytes = 5
        options.measure_io = True

        with debug_input.for_document("a.docx", options, file_system,
                                      console, "a.docx") as stream:
            # Truncated to the prefix, asked for in one read of a buffer big enough for
            # the whole file. The reference reads Stream.Length instead, which a Python
            # stream does not have and _length has to seek for -- and a seek is one of
            # the things being counted here, so the length has to be read out of the
            # report's own bytes= figure rather than measured before it.
            self.assertEqual(b"hello", stream.read(4096))
            stream.close()

            self.assertEqual(1, len(console.out_lines))
            self.assertEqual("a.docx: reads=1 seeks=0 bytes=5", console.out_lines[0])

    def test_measuring_composes_with_a_buffered_source(self):
        file_system = InMemoryFileSystem()
        file_system.add_file("a.docx", "hello world")
        console = RecordingConsole()

        options = DiagnosticOptions()
        options.use_memory_stream = True
        options.measure_io = True

        with debug_input.for_document("a.docx", options, file_system,
                                      console, "a.docx") as stream:
            self.assertEqual(11, len(stream.read(4096)))
            stream.close()

            self.assertEqual(1, len(console.out_lines))
            self.assertEqual("a.docx: reads=1 seeks=0 bytes=11", console.out_lines[0])

    def test_measure_io_counts_what_the_engine_asked_for_and_reports_on_close(self):
        # Reported on close so the figures cover the whole document rather than
        # whatever had happened when someone thought to look.
        file_system = InMemoryFileSystem()
        file_system.add_file("a.docx", "hello world")
        console = RecordingConsole()

        options = DiagnosticOptions()
        options.measure_io = True

        stream = debug_input.for_document("a.docx", options, file_system, console, "a.docx")

        stream.read(5)
        stream.seek(0, io.SEEK_SET)
        self.assertEqual([], console.out_lines)

        stream.close()

        self.assertEqual(1, len(console.out_lines))
        self.assertEqual("a.docx: reads=1 seeks=1 bytes=5", console.out_lines[0])


def _diagnostics_with_timeout(seconds: int) -> DiagnosticOptions:
    """
    Options with --timeout set and nothing else.

    C#'s object initialiser written out, which every test in this file needs at least
    once; this one is named because three of them want the same single field.
    """
    options = DiagnosticOptions()
    options.timeout = seconds
    return options


def _length(stream) -> int:
    """
    C#'s Stream.Length, which a Python stream does not have.

    Seeks to the end and back, so it must not be used on a MeasuredStream: the seeks it
    costs are exactly what --debug-measure-io is counting.
    """
    position = stream.tell()
    try:
        return stream.seek(0, io.SEEK_END)
    finally:
        stream.seek(position, io.SEEK_SET)
