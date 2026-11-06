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
import shutil
from typing import BinaryIO, List, Optional, TextIO

from ..configuration import options_validator
from ..configuration.command_options import ConvertOptions
from ..configuration.shared_options import SharedOptions
from ..discovery.input_enumerator import InputEnumerator
from ..engine.engine_exception import EngineError, EngineException
from ..engine.i_document_filters_engine import IDocument, IOutputCanvas
from ..execution import debug_input
from ..execution.canvas_conversion import CanvasConversion
from ..execution.diagnostics import Diagnostics
from ..execution.engine_log import EngineLog
from ..execution.extractor_open_settings import ExtractorOpenSettings
from ..execution.password_provider import PasswordProvider
from ..execution.structure_report import StructureReport
from ..execution.text_conversion import TextConversion
from ..planning import output_planner
from ..planning.format_spec import FormatKind, FormatSpec
from ..planning.page_range import PageRange
from ..planning.run_plan import OutputCardinality, PlannedItem, RunPlan
from ..runtime import exit_code, viewer
from ..runtime.batch_runner import BatchRunner
from ..runtime.cli_error import CliError, DocumentError
from . import command_execution, plan_format
from .i_command import CommandContext, ICommand

# canvas_conversion and extractor_open_settings are both imported at module scope, which adds
# no edge to execution's one import cycle: that cycle is ExtractorOpenSettings ->
# CanvasConversion -> DocumentFiltersSession -> ExtractorOpenSettings, it is already broken at
# its single live-call edge by the deferred import inside extractor_open_settings.for_format,
# and nothing in execution imports commands.
#
# Nor does anything imported here reach the DocumentFilters binding at module scope, which is
# the rule for everything above the seam. document_filters_session is the one file that names
# the concrete engine, and this module never imports it -- the session arrives on
# CommandContext, and command_execution defers its own import of it.


class ConvertCommand(ICommand):
    """
    convert — render documents to a target format, and optionally report document and page
    structure while doing so.
    """

    def __init__(self, options: ConvertOptions):
        self.options = options
        """The bound per-command options. Readable so the binding itself can be tested."""

    @property
    def name(self) -> str:
        return "convert"

    @property
    def requires_engine(self) -> bool:
        return True

    def execute(self, context: CommandContext) -> int:
        # The reference passes the body to CommandExecution.WithSession as a lambda, and the
        # per-item body to BatchRunner.Run as a second one. A Python lambda holds a single
        # expression and both bodies are many statements, so each is spelled out as a nested
        # def, exactly as extract_command.py does for the same pair.
        def run() -> int:
            shared = context.shared_options
            options_validator.validate_convert(shared, self.options)

            enumerator = InputEnumerator(context.file_system, context.console)
            inputs = enumerator.enumerate(shared.input)

            plan = output_planner.plan(shared, self.options, inputs, context.file_system)
            # --fingerprint never writes to plan.items' resolved targets — see
            # canvas_conversion._make_canvas — so a real file already sitting at one
            # of those paths is not a conflict to report.
            if not self.options.fingerprint:
                output_planner.verify_targets(plan, shared, context.file_system)

            diagnostics = Diagnostics(shared.diagnostics, context.console, context.session.api, context.clock)
            log = EngineLog.for_options(shared.diagnostics, context.console)
            diagnostics.begin_run()

            # --combine is allowed for text formats too, which have no canvas: their
            # documents concatenate into one writer instead. Keyed off the format's
            # kind rather than the cardinality alone, because reading
            # canvas_type.value for a text format threw before anything was caught
            # and took the process down with an unhandled exception.
            combined_format = self.options.format
            combining = (plan.cardinality == OutputCardinality.COMBINED
                         and not shared.output.use_stdout)
            if combining:
                _ensure_output_directory(context, _directory_name(plan.combined_target))

            # The reference's two `using` blocks, whose disposal order is the reverse of their
            # declaration, and which close the combined output before --view below: it is not
            # a finished file until it is, a PDF's trailer being written on close. try/finally
            # is the shape that reproduces both, and the None guards are the part C# does not
            # need: `using` on a null disposes nothing, where Python's `with None` raises. Both
            # are initialised inside the try so that a failure opening the second still closes
            # the first, which is what the enclosing block unwinding does there.
            combined_canvas: Optional[IOutputCanvas] = None
            combined_text: Optional[TextIO] = None
            try:
                if combining and combined_format.kind == FormatKind.CANVAS:
                    combined_canvas = context.session.api.make_canvas(
                        plan.combined_target, combined_format.canvas_type,
                        CanvasConversion.compose_canvas_options(combined_format,
                                                                self.options.canvas_options,
                                                                self.options.no_images))

                if combining and combined_format.kind == FormatKind.TEXT:
                    combined_text = _text_writer(
                        context.file_system.create_file(plan.combined_target))

                batch = BatchRunner(shared.batch, context.console, context.interruption)
                canvas_conversion = CanvasConversion(context.session, context.console, shared,
                                                     context.file_system)
                processed = 0

                def process(item: PlannedItem) -> None:
                    # processed is rebound rather than only read, so it needs declaring. The
                    # C# closure captures the variable itself and needs nothing.
                    nonlocal processed

                    processed += 1
                    diagnostics.begin_document(item.display_name, processed, len(plan.items))

                    self._process_document(context, shared, item, combined_canvas, combined_text,
                                           canvas_conversion, diagnostics, log)

                    diagnostics.end_document(item.display_name)
                    diagnostics.check_document_timeout(item.display_name)
                    diagnostics.check_limits()
                    diagnostics.pause_if_requested()

                # The reference names this local exitCode. Renamed, because
                # `from ..runtime import exit_code` has already put a module of that name in
                # scope and the comparison below reads exit_code.SUCCESS out of it — the one
                # convert needs and extract, list and meta do not.
                code = batch.run(plan.items, process)

                diagnostics.end_run(batch.success_count, batch.failure_count)
            finally:
                if combined_text is not None:
                    combined_text.close()
                if combined_canvas is not None:
                    combined_canvas.close()

            # --view opens what was produced, once the run is over and the file is
            # closed. Only a run with one output has something unambiguous to open,
            # which is what the spec means by "single output".
            if self.options.view and code == exit_code.SUCCESS:
                (context.open_viewer or viewer.open_in_viewer)(ConvertCommand.single_output_of(plan),
                                                               context.console)

            return code

        return command_execution.with_session(context, run)

    def _process_document(self, context: CommandContext, shared: SharedOptions, item: PlannedItem,
                          combined_canvas: Optional[IOutputCanvas],
                          combined_text: Optional[TextIO], canvas_conversion: CanvasConversion,
                          diagnostics: Diagnostics, log: Optional[EngineLog]) -> None:
        """
        Resolves the format, opens the document once with settings matching that format,
        converts, and closes it. Failures become a DocumentError for BatchRunner to count
        rather than ending the run.

        The format is settled before the document is opened, which it has to be:
        ExtractorOpenSettings depends on it, and the engine applies those settings during
        Open. It comes from --to and nothing else, so there is nothing to discover from the
        document and no reason to open one twice.
        """
        # `format` keeps the reference's name, shadowing the builtin for the body of this
        # method only, which nothing here calls — the same trade extractor_open_settings
        # and canvas_conversion already made for their own `format` parameters.
        format = self.options.format
        passwords = PasswordProvider(shared.passwords, context.console, context.file_system)
        open_settings = ExtractorOpenSettings.for_format(format, self.options, shared)

        # Once per item, before anything opens an output. --fingerprint is skipped
        # because its target is a plan's guess that is never written to.
        if not self.options.fingerprint:
            _ensure_output_directory(context, item.output_directory)

        # A debug switch that changes how the bytes arrive hands the engine a stream
        # instead of the path; otherwise the engine opens the file itself. Declared
        # out here, not inside the try, so it outlives the document that reads from
        # it — disposing it at the end of the open would close the file mid-read.
        #
        # The C# is a `using` declaration, which disposes at the end of this method;
        # try/finally is the shape that reproduces that, and the None guard is the part C#
        # does not need. debug_input.for_document returns None in the ordinary case, which
        # is every run that passed none of the debug input switches.
        debug_stream = debug_input.for_document(item.source_path, shared.diagnostics,
                                                context.file_system, context.console,
                                                item.display_name)
        try:
            stop = command_execution.watch_for_stop(diagnostics, context.interruption)
            try:
                if debug_stream is None:
                    document = context.session.open_document(
                        item.source_path, open_settings, passwords,
                        stop.should_stop, log)
                else:
                    # C# spells both halves OpenDocument, resolving the overload on the first
                    # argument; the session seam carries the difference in the name instead,
                    # Python having no overloading.
                    document = context.session.open_document_from_stream(
                        debug_stream, item.display_name, open_settings, passwords,
                        stop.should_stop, log)
            except EngineException as error:
                # C#'s two `catch ... when` filters, which Python has no equivalent of.
                # EngineException is not a CliError, so the reference's second filter would
                # catch a non-cancelled one and restate it as a DocumentError; that branch is
                # written out here rather than left to fall through, because a Python
                # `except` clause cannot decline a match.
                if error.error == EngineError.CANCELLED:
                    # We asked for this, so it is the limit or the interrupt being reported, not a bad document.
                    raise stop.cause(item.display_name)
                raise DocumentError(item.source_path, "could not open document", error)
            except CliError:
                # `when (!(error is CliError))`: a usage error, a limit being exceeded or an
                # interruption is not this document's failure and travels on. A bare raise,
                # which keeps the original traceback rather than starting a new one here.
                raise
            except Exception as error:
                # Exception and not BaseException, so KeyboardInterrupt and SystemExit still
                # travel. The C# `catch (Exception)` likewise does not stop the failures .NET
                # considers beyond catching.
                raise DocumentError(item.source_path, "could not open document", error)

            # C#'s `using (document)`.
            with document:
                try:
                    structure = StructureReport(self.options.reports, context.console)
                    structure.report_document(document, item.display_name)

                    if self.options.reports.any:
                        # A while loop, not `for page_index in range(document.get_page_count())`.
                        # The reference's `for` re-reads GetPageCount on every iteration, and
                        # range would read it once — a difference that is invisible against an
                        # engine whose page count is stable and visible against one whose is
                        # not. Reproducing the reference exactly costs two lines and settles
                        # the question without this port having to be right about the engine.
                        page_index = 0
                        while page_index < document.get_page_count():
                            # C#'s `using IDocumentPage page`, whose scope is the loop body.
                            with document.get_page(page_index) as page:
                                structure.report_page(page, page_index + 1, item.display_name)
                            page_index += 1

                    # The reference's switch over format.Kind, whose default arm is itself a
                    # three-way branch. Python 3.8 has no match statement, so it is if/elif —
                    # nested the same way, so the two arms of the switch stay one level above
                    # the three canvas destinations.
                    if format.kind == FormatKind.TEXT:
                        _convert_to_text(context, shared, item, format, document, self.options,
                                         combined_text)
                    elif format.kind == FormatKind.DOC_PACK:
                        _convert_to_doc_pack(context, shared, item, document, self.options)
                    else:
                        if combined_canvas is not None:
                            # C#'s ?? falls through on null only; an explicit `is not None`
                            # reproduces that, where `or` would also fall through on a
                            # PageRange that happened to be falsy.
                            canvas_conversion.append_to(
                                combined_canvas, document,
                                self.options.pages if self.options.pages is not None
                                else PageRange.ALL,
                                self.options, item.display_name)
                        elif shared.output.use_stdout:
                            canvas_conversion.convert_to_stream(
                                document, context.console.open_standard_output(), item, format,
                                self.options)
                        else:
                            canvas_conversion.convert(document, item, format, self.options)
                except CliError:
                    raise
                except Exception as error:
                    # C#'s error.Message. str(error) is that text for everything this can
                    # catch: EngineException passes its message to Exception's constructor,
                    # so both spellings return the engine's own words.
                    raise DocumentError(item.source_path, str(error), error)
        finally:
            if debug_stream is not None:
                debug_stream.close()

    @staticmethod
    def single_output_of(plan: RunPlan) -> Optional[str]:
        """
        The one file a run produced, or None when it produced none or several.

        A per-page format resolves its names during execution, so even a single input can
        leave several files and there is no one thing to open.
        """
        if plan.cardinality == OutputCardinality.COMBINED:
            return plan.combined_target

        return plan.items[0].target if len(plan.items) == 1 else None

    def describe_plan(self, context: CommandContext) -> str:
        shared = context.shared_options
        options_validator.validate_convert(shared, self.options)

        enumerator = InputEnumerator(context.file_system, context.console)
        inputs = enumerator.enumerate(shared.input)

        plan = output_planner.plan(shared, self.options, inputs, context.file_system)
        if not self.options.fingerprint:
            output_planner.verify_targets(plan, shared, context.file_system)

        lines: List[str] = [
            "command convert",
            # The C# concatenates a FormatSpec onto a string, which calls ToString(); Python
            # needs str() spelled out. Never None by the time this runs: validate_convert's
            # check_format_is_provided has already refused a run without --to.
            "to " + str(self.options.format),
            plan_format.inputs(inputs),
            shared.to_canonical_string(),
            "pages " + str(self.options.pages if self.options.pages is not None else PageRange.ALL),
            plan.to_canonical_string()
        ]
        return "\n".join(lines)


def _ensure_output_directory(context: CommandContext, directory: Optional[str]) -> None:
    """
    Creates the directory an output is about to be written into, if it is not already
    there.

    Here, in the orchestrator, rather than at each of the three places that open an
    output, because only one of those three is ours to change: the text path writes
    through IFileSystem, but a canvas and a docpack are opened by the engine, which will
    not create a directory and reports a missing one as FileStream::Create failing — a
    per-document error that reads as though the input were missing. Doing it per format
    meant --out-dir out/ worked for --to text and failed for everything else.
    """
    # Plain truthiness is string.IsNullOrEmpty exactly, for a string: None and '' are the
    # only falsy values an Optional[str] has. Deliberately not value_parsers
    # .is_all_whitespace, which is the wider IsNullOrWhiteSpace the reference did not ask
    # for here.
    if directory:
        context.file_system.create_directory(directory)


def _convert_to_text(context: CommandContext, shared: SharedOptions, item: PlannedItem,
                     format: Optional[FormatSpec], document: IDocument, options: ConvertOptions,
                     combined_text: Optional[TextIO]) -> None:
    text_conversion = TextConversion()
    if combined_text is not None:
        # --combine: every document appends to the one writer execute owns, so it is not
        # closed here.
        text_conversion.convert(document, format, options, combined_text)
    elif shared.output.use_stdout:
        writer = _text_writer(context.console.open_standard_output())
        try:
            text_conversion.convert(document, format, options, writer)
        finally:
            # The reference sets AutoFlush on a StreamWriter it then deliberately never
            # disposes: the stream belongs to the console, and closing it would take
            # standard output down with it. Python cannot simply walk away from the
            # wrapper — a TextIOWrapper closes what it wraps, on close and again when it is
            # collected — so it is detached instead. detach flushes the encoded text into
            # the byte stream and disowns it, which is the whole of what AutoFlush achieves
            # here, and the returned stream is flushed so the bytes reach the console.
            writer.detach().flush()
    else:
        # Looked at again here, not only when the plan was made. The planner checks every
        # target before the run starts, against a directory none of this run's outputs are
        # in yet, so two inputs whose targets the filesystem treats as one name -- which the
        # collision check's ASCII fold cannot see for a pair differing outside ASCII -- both
        # passed, and the second write replaced the first with no --overwrite and exit 0.
        # Asking immediately before the write puts the question to the filesystem, which is
        # the authority on whether two names are one file.
        output_planner.verify_page_target(item.target, shared, context.file_system)

        with _text_writer(context.file_system.create_file(item.target)) as destination:
            text_conversion.convert(document, format, options, destination)


def _convert_to_doc_pack(context: CommandContext, shared: SharedOptions, item: PlannedItem,
                         document: IDocument, options: ConvertOptions) -> None:
    """
    --stdout for docpack. item.target is None under OutputCardinality.STREAM, the same gap
    canvas_conversion.convert_to_stream closed for canvas formats.

    Renders into a MemoryStream first rather than handing --stdout's own stream to
    make_doc_pack directly, for the same reason as the canvas path: a docpack is a binary
    container whose finished bytes can only be known once make_doc_pack is done, and
    i_console.open_standard_output is forward-only. No sample or doc2text streams a docpack
    to confirm whether it truly needs to seek, so this buffers defensively rather than risk
    the same "renders fine, nothing reaches the console" failure found for canvases in D37.
    """
    # C#'s ?? falls through on null only, so an --canvas-options that was genuinely absent
    # and one given as an empty string reach the engine the same way here and diverge
    # nowhere else.
    canvas_options = options.canvas_options if options.canvas_options is not None else ''
    if not shared.output.use_stdout:
        document.make_doc_pack(item.target, canvas_options)
        return

    # C#'s MemoryStream, and its `using` is the `with`.
    with io.BytesIO() as buffer:
        # C# spells both halves MakeDocPack, resolving the overload on the first argument;
        # the seam carries the difference in the name instead.
        document.make_doc_pack_on_stream(buffer, canvas_options)

        buffer.seek(0)
        destination = context.console.open_standard_output()
        # copyfileobj rather than write(getvalue()), which is Stream.CopyTo's own chunked
        # read-and-write and not a second copy of the whole pack — the same choice
        # canvas_conversion.convert_to_stream makes.
        shutil.copyfileobj(buffer, destination)
        destination.flush()


def _text_writer(destination: BinaryIO) -> io.TextIOWrapper:
    """
    A text stream over a byte stream, standing in for `new StreamWriter(stream)`.

    Not in the C#, which needs nothing here. Two of the three arguments are what make a
    TextIOWrapper mean what a StreamWriter means, and neither is Python's default:

      * newline='' stops the wrapper translating every '\\n' written through it into the
        host's line ending. StreamWriter does no such translation from Write(string), and
        the payload here is the document's own text — so left at its default this would
        rewrite document bytes on Windows and nowhere else, invisibly, since the text
        would still be correct and only the bytes would differ. This is the one argument
        text_conversion.convert's docstring names as the caller's obligation.
      * encoding='utf-8' is StreamWriter's own default encoding, UTF-8 with no byte order
        mark. Python would otherwise use the host's locale encoding, which is cp1252 on a
        default Windows install.

    errors is left alone on purpose: the default is 'strict', and StreamWriter's default
    encoding is constructed with throwOnInvalidBytes, so both refuse rather than
    substitute. That is the opposite of system_console._use_utf8_streams, which matches
    Encoding.UTF8's replacement fallback for report text — this writes the document, and a
    document must not be silently altered.

    write_through=True keeps no text in the wrapper's own buffer, which is what lets the
    --stdout caller detach it and have everything written so far already be in the byte
    stream.
    """
    return io.TextIOWrapper(destination, encoding='utf-8', newline='', write_through=True)


def _directory_name(path: str) -> str:
    """
    The directory portion of a path, or empty when it has none.

    Path.GetDirectoryName in the C#, which is platform-dependent: on Windows it also
    splits on a backslash and on a drive-relative root, on Linux it does not. Normalising
    first picks one answer for all four ports rather than inheriting the host's, which
    matters for RunPlan.combined_target — the one path this is asked about — because that
    is a raw --out-file value and so is the one target the planner did not build out of
    forward slashes.

    A third copy of a three-line function that run_plan._directory_name and
    output_planner._directory_of already spell the same way. Kept rather than shared
    because each mirrors a separate Path.GetDirectoryName call site in the reference, and
    reaching across modules for another file's private name would be worse than the
    duplication; a shared helper is a change to make in all four ports at once, not here.
    """
    normalised = path.replace('\\', '/')
    slash = normalised.rfind('/')
    return "" if slash < 0 else normalised[:slash]
