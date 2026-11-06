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

from ..configuration import options_validator
from ..configuration.command_options import MetaOptions
from ..discovery.input_enumerator import InputEnumerator
from ..engine.engine_exception import EngineError, EngineException
from ..execution import debug_input
from ..execution.diagnostics import Diagnostics
from ..execution.engine_log import EngineLog
from ..execution.extractor_open_settings import ExtractorOpenSettings
from ..execution.metadata_report import MetadataReport
from ..execution.password_provider import PasswordProvider
from ..planning import output_planner
from ..planning.run_plan import RunPlan
from ..runtime.batch_runner import BatchRunner
from ..runtime.cli_error import CliError, DocumentError
from ..runtime.json_writer import JsonWriter
from . import command_execution, plan_format
from .i_command import CommandContext, ICommand

# extractor_open_settings is imported at module scope, which adds no edge to execution's one
# import cycle: that cycle is ExtractorOpenSettings -> CanvasConversion ->
# DocumentFiltersSession -> ExtractorOpenSettings, it is already broken at its single
# live-call edge by the deferred import inside extractor_open_settings.for_format, and nothing
# in execution imports commands. for_metadata_only is a live call rather than a type
# reference, so there is nothing to defer here either.
#
# Nor does anything imported here reach the DocumentFilters binding at module scope, which is
# the rule for everything above the seam. The one file that names the concrete engine is
# execution/document_filters_session.py, and it is reached only through
# command_execution.with_session, whose own import of it is deferred.


class MetaCommand(ICommand):
    """meta — output document metadata only."""

    def __init__(self, options: MetaOptions):
        self.options = options
        """The bound per-command options. Readable so the binding itself can be tested."""

    @property
    def name(self) -> str:
        return "meta"

    @property
    def requires_engine(self) -> bool:
        return True

    def execute(self, context: CommandContext) -> int:
        # The C# passes the body to CommandExecution.WithSession as a lambda; a lambda in
        # Python is a single expression, so the same body is a nested def. The name is the
        # only thing that changes -- it is still a closure over context and self, passed as
        # a value and called once.
        def body() -> int:
            shared = context.shared_options
            options_validator.validate(shared)

            enumerator = InputEnumerator(context.file_system, context.console)
            inputs = enumerator.enumerate(shared.input)

            passwords = PasswordProvider(shared.passwords, context.console, context.file_system)
            diagnostics = Diagnostics(shared.diagnostics, context.console, context.session.api, context.clock)
            log = EngineLog.for_options(shared.diagnostics, context.console)
            batch = BatchRunner(shared.batch, context.console, context.interruption)
            report = MetadataReport()
            json = JsonWriter().begin_array() if self.options.json else None

            diagnostics.begin_run()

            processed = 0

            # The C# names the per-item lambda's parameter `input`, which is a Python
            # builtin; `source` is what plan_format.inputs and planning/output_planner.py
            # already call an InputSource, and a lambda parameter is not visible to a caller
            # the way a method's is, so nothing about the four ports' agreement moves.
            def process(source) -> None:
                nonlocal processed
                processed += 1
                diagnostics.begin_document(source.path, processed, len(inputs))
                passwords.reset()

                # Declared out here so it outlives the document that reads from it.
                #
                # Named debug_stream rather than the reference's debugInput, because
                # debug_input is the module this calls into. The C# spells the lifetime as
                # `using Stream debugInput`, which does nothing when the stream is null;
                # `with None:` raises, so the close is a try/finally carrying the None test
                # that `using` does implicitly -- and the finally is what puts the close
                # after the document's, in both ports.
                debug_stream = debug_input.for_document(source.path, shared.diagnostics,
                                                       context.file_system, context.console,
                                                       source.path)
                try:
                    stop = command_execution.watch_for_stop(diagnostics, context.interruption)
                    try:
                        # `open_settings`, where the C# local is named `open`. The reference
                        # can spell it that way because C# has no such builtin; the session
                        # method this hands it to does keep the name, since a parameter is
                        # visible in a keyword call and the four ports agree on it.
                        open_settings = ExtractorOpenSettings.for_metadata_only(shared)

                        document = (
                            context.session.open_document(source.path, open_settings, passwords,
                                                          stop.should_stop, log)
                            if debug_stream is None
                            else context.session.open_document_from_stream(debug_stream, source.path,
                                                                          open_settings, passwords,
                                                                          stop.should_stop, log))
                    except EngineException as error:
                        # The C#'s first clause is `catch (EngineException error) when
                        # (error.Error == EngineError.Cancelled)`, and a non-cancelled
                        # EngineException falls through to its second. Python has no
                        # exception filter, so the test is an `if` inside one handler and the
                        # fall-through is written out: an EngineException is not a CliError,
                        # so the second clause would have caught it.
                        #
                        # CANCELLED is reported as whatever the stop signal recorded: the
                        # interrupt, the --timeout, or -- when neither asked -- the document.
                        if error.error == EngineError.CANCELLED:
                            raise stop.cause(source.path)
                        raise DocumentError(source.path, "could not open document", error)
                    except CliError:
                        # The reference's `when (!(error is CliError))` filter, spelled as
                        # catching the excluded type first and re-raising it untouched. A
                        # bare `raise` keeps the original traceback rather than starting a
                        # new one at this line.
                        raise
                    except Exception as error:
                        raise DocumentError(source.path, "could not open document", error)

                    with document:
                        try:
                            if self.options.json:
                                report.write_json(document, source.path, json)
                            else:
                                report.write_text(document, source.path, context.console)
                        except CliError:
                            raise
                        except Exception as error:
                            # str(error) for the C#'s error.Message. The two agree for every
                            # exception this port raises, because CliError and
                            # EngineException both pass their message to the base
                            # constructor; they differ only for an exception constructed
                            # with no arguments at all, where .NET substitutes a default
                            # message and str() is empty. Nothing here constructs one.
                            raise DocumentError(source.path, str(error), error)

                    diagnostics.end_document(source.path)
                    diagnostics.check_document_timeout(source.path)
                    diagnostics.check_limits()
                    diagnostics.pause_if_requested()
                finally:
                    if debug_stream is not None:
                        debug_stream.close()

            exit_code = batch.run(inputs, process)

            if self.options.json:
                context.console.write(json.end_array().to_string())

            diagnostics.end_run(batch.success_count, batch.failure_count)
            return exit_code

        return command_execution.with_session(context, body)

    def describe_plan(self, context: CommandContext) -> str:
        shared = context.shared_options
        options_validator.validate(shared)

        enumerator = InputEnumerator(context.file_system, context.console)
        inputs = enumerator.enumerate(shared.input)

        cardinality = output_planner.resolve_cardinality(shared, len(inputs), combine=False)

        lines = [
            "command meta",
            plan_format.inputs(inputs),
            shared.to_canonical_string(False),
            "json " + plan_format.yes_no(self.options.json),
            "cardinality " + RunPlan.cardinality_text(cardinality)
        ]
        return "\n".join(lines)
