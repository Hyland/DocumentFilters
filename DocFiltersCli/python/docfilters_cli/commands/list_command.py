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

from typing import List, Optional

from ..configuration import options_validator
from ..configuration.command_options import ListOptions
from ..discovery.input_enumerator import InputEnumerator
from ..engine.engine_exception import EngineError, EngineException
from ..execution import debug_input
from ..execution.diagnostics import Diagnostics
from ..execution.engine_log import EngineLog
from ..execution.extractor_open_settings import ExtractorOpenSettings
from ..execution.password_provider import PasswordProvider
from ..execution.subfile_extraction import SubfileExtraction
from ..planning import output_planner
from ..planning.run_plan import RunPlan
from ..runtime.batch_runner import BatchRunner
from ..runtime.cli_error import CliError, DocumentError
from ..runtime.json_writer import JsonWriter
from . import command_execution, plan_format
from .i_command import CommandContext, ICommand


class ListCommand(ICommand):
    """list — list a document's subfile contents without extracting."""

    def __init__(self, options: ListOptions):
        self.options = options
        """The bound per-command options. Readable so the binding itself can be tested."""

    @property
    def name(self) -> str:
        return "list"

    @property
    def requires_engine(self) -> bool:
        return True

    def execute(self, context: CommandContext) -> int:
        # The reference passes the body to CommandExecution.WithSession as a lambda, and the
        # per-input body to BatchRunner.Run as a second one. A Python lambda holds a single
        # expression and both bodies are many statements, so each is spelled out as a nested
        # def, exactly as version_command.py does for the outer one. Same closures over
        # context and over the locals below, and the same single place owning the session's
        # lifetime.
        def run() -> int:
            shared = context.shared_options
            options_validator.validate(shared)

            enumerator = InputEnumerator(context.file_system, context.console)
            inputs = enumerator.enumerate(shared.input)

            passwords = PasswordProvider(shared.passwords, context.console, context.file_system)
            diagnostics = Diagnostics(shared.diagnostics, context.console, context.session.api, context.clock)
            log = EngineLog.for_options(shared.diagnostics, context.console)
            batch = BatchRunner(shared.batch, context.console, context.interruption)
            subfiles = SubfileExtraction(context.file_system)
            json: Optional[JsonWriter] = JsonWriter().begin_array() if self.options.json else None

            diagnostics.begin_run()

            processed = 0

            # The reference's lambda parameter is named input, which is a Python builtin;
            # spelled source here, the name plan_format.inputs already gives an InputSource.
            # A lambda parameter is never passed by keyword, so unlike `open` and `format`
            # elsewhere in this port there is nothing for the four ports to agree on.
            def list_one(source) -> None:
                # processed is rebound rather than only read, so it needs declaring. The C#
                # closure captures the variable itself and needs nothing.
                nonlocal processed

                processed += 1
                diagnostics.begin_document(source.path, processed, len(inputs))
                passwords.reset()

                # Declared out here so it outlives the document that reads from it.
                #
                # The C# is a `using` declaration, which disposes at the end of the enclosing
                # block -- after the diagnostics calls at the bottom, not after the container.
                # try/finally is the shape that reproduces that, and the None guard is the
                # part C# does not need: `using` on a null disposes nothing, where Python's
                # `with None` raises. debug_input.for_document returns None in the ordinary
                # case, which is every run that passed none of the debug input switches.
                debug_stream = debug_input.for_document(source.path, shared.diagnostics,
                                                        context.file_system, context.console,
                                                        source.path)
                try:
                    stop = command_execution.watch_for_stop(diagnostics, context.interruption)
                    try:
                        open_settings = ExtractorOpenSettings.for_inspection(shared)
                        if debug_stream is None:
                            container = context.session.open_document(
                                source.path, open_settings, passwords,
                                stop.should_stop, log)
                        else:
                            # C# spells both halves OpenDocument, resolving the overload on the
                            # first argument; the session seam carries the difference in the
                            # name instead, Python having no overloading.
                            container = context.session.open_document_from_stream(
                                debug_stream, source.path, open_settings, passwords,
                                stop.should_stop, log)
                    except EngineException as error:
                        # C#'s two `catch ... when` filters, which Python has no equivalent of.
                        # EngineException is not a CliError, so the reference's second filter
                        # would catch a non-cancelled one and restate it as a DocumentError;
                        # that branch is written out here rather than left to fall through,
                        # because a Python `except` clause cannot decline a match.
                        if error.error == EngineError.CANCELLED:
                            raise stop.cause(source.path)
                        raise DocumentError(source.path, "could not open document", error)
                    except CliError:
                        # `when (!(error is CliError))`: a usage error, a limit being exceeded
                        # or an interruption is not this document's failure and travels on.
                        raise
                    except Exception as error:
                        # Exception and not BaseException, so KeyboardInterrupt and SystemExit
                        # still travel. The C# `catch (Exception)` likewise does not stop the
                        # failures .NET considers beyond catching.
                        raise DocumentError(source.path, "could not open document", error)

                    # C#'s `using (container)`.
                    with container:
                        try:
                            entries = subfiles.list_all(container, source.path, shared)
                        except CliError:
                            raise
                        except Exception as error:
                            # C#'s error.Message. str(error) is that text for everything this
                            # can catch: EngineException passes its message to Exception's
                            # constructor, so both spellings return the engine's own words.
                            raise DocumentError(source.path, str(error), error)

                        if self.options.json:
                            (json.begin_object()
                             .property("path", source.path)
                             .name("subfiles")
                             .begin_array())
                            for entry in entries:
                                # The C#'s fluent chain, wrapped in parentheses to span lines
                                # as catalog_reports.py does. The call order is the C#'s
                                # exactly, and JsonWriter emits keys in the order they are
                                # written, so the object's shape is byte-identical across the
                                # ports. size and depth are ints and modified may be None,
                                # which JsonWriter renders as a bare number and as null --
                                # what the C# writer does with a long and with a null string.
                                (json.begin_object()
                                 .property("id", entry.id)
                                 .property("name", entry.name)
                                 .property("size", entry.size)
                                 .property("modified", entry.modified)
                                 .property("depth", entry.depth)
                                 .end_object())
                            json.end_array().end_object()
                        else:
                            for entry in entries:
                                # One format call where the C# concatenates two interpolated
                                # strings. That split is a source line-length artifact there,
                                # not a boundary in the output.
                                context.console.write_line(
                                    "{0}: id={1} size={2} modified={3} depth={4} name={5}".format(
                                        _text(entry.display_name), _text(entry.id), entry.size,
                                        _text(entry.modified), entry.depth, _text(entry.name)))

                    diagnostics.end_document(source.path)
                    diagnostics.check_document_timeout(source.path)
                    diagnostics.check_limits()
                    diagnostics.pause_if_requested()
                finally:
                    if debug_stream is not None:
                        debug_stream.close()

            # Named as the reference names it. Nothing is shadowed: this module does not
            # import runtime.exit_code, list producing only what BatchRunner.run decides.
            exit_code = batch.run(inputs, list_one)

            if self.options.json:
                context.console.write(json.end_array().to_string())

            diagnostics.end_run(batch.success_count, batch.failure_count)
            return exit_code

        return command_execution.with_session(context, run)

    def describe_plan(self, context: CommandContext) -> str:
        shared = context.shared_options
        options_validator.validate(shared)

        enumerator = InputEnumerator(context.file_system, context.console)
        inputs = enumerator.enumerate(shared.input)

        cardinality = output_planner.resolve_cardinality(shared, len(inputs), combine=False)

        lines: List[str] = [
            "command list",
            plan_format.inputs(inputs),
            shared.to_canonical_string(),
            "json " + plan_format.yes_no(self.options.json),
            "cardinality " + RunPlan.cardinality_text(cardinality)
        ]
        return "\n".join(lines)


def _text(value: Optional[str]) -> str:
    """
    A field value as C# string interpolation would render it.

    Not in the C#, and it exists for one difference: C#'s $"modified={null}" writes an empty
    string, where "modified={0}".format(None) writes the four characters None.

    modified is the field that is actually absent in a real run --
    SubfileExtraction.format_timestamp answers None for a container that records no
    timestamp, which is most of them -- and display_name, id and name go through the same
    helper because SubfileEntry declares all three Optional[str] too, so a container that
    leaves one unset produces the same line in all four ports. size and depth are ints and
    need none of this.

    execution/structure_report.py and execution/catalog_reports.py carry the same helper for
    the same reason. Duplicated rather than imported: it is private in both of those, and
    four lines of the obvious is a smaller thing to own three times than a module edge from
    a command into a report for one expression.

    The --json branch needs none of this either. JsonWriter.value writes null for None, which
    is what the C# JsonWriter does with a null string, so the two agree already.
    """
    return value if value is not None else ""
