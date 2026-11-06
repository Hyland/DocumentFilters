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

from typing import List

from ..configuration import options_validator
from ..configuration.command_options import ExtractOptions
from ..discovery.input_enumerator import InputEnumerator
from ..engine.engine_exception import EngineError, EngineException
from ..execution.diagnostics import Diagnostics
from ..execution.engine_log import EngineLog
from ..execution.extractor_open_settings import ExtractorOpenSettings
from ..execution.password_provider import PasswordProvider
from ..execution.subfile_extraction import SubfileExtraction
from ..planning import output_planner
from ..planning.run_plan import RunPlan
from ..runtime.batch_runner import BatchRunner
from ..runtime.cli_error import CliError, DocumentError
from . import command_execution, plan_format
from .i_command import CommandContext, ICommand


class ExtractCommand(ICommand):
    """
    extract — pull embedded subfiles, attachments and images out of container documents.
    """

    def __init__(self, options: ExtractOptions):
        self.options = options
        """The bound per-command options. Readable so the binding itself can be tested."""

    @property
    def name(self) -> str:
        return "extract"

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
            options_validator.validate_extract(shared, self.options)

            enumerator = InputEnumerator(context.file_system, context.console)
            inputs = enumerator.enumerate(shared.input)

            passwords = PasswordProvider(shared.passwords, context.console, context.file_system)
            diagnostics = Diagnostics(shared.diagnostics, context.console, context.session.api, context.clock)
            log = EngineLog.for_options(shared.diagnostics, context.console)
            batch = BatchRunner(shared.batch, context.console, context.interruption)
            subfiles = SubfileExtraction(context.file_system)

            diagnostics.begin_run()

            processed = 0

            # The reference's lambda parameter is named input, which is a Python builtin;
            # spelled source here, the name plan_format.inputs already gives an InputSource.
            # A lambda parameter is never passed by keyword, so unlike `open` and `format`
            # elsewhere in this port there is nothing for the four ports to agree on.
            def extract_one(source) -> None:
                # processed is rebound rather than only read, so it needs declaring. The C#
                # closure captures the variable itself and needs nothing.
                nonlocal processed

                processed += 1
                diagnostics.begin_document(source.path, processed, len(inputs))
                passwords.reset()

                stop = command_execution.watch_for_stop(diagnostics, context.interruption)
                try:
                    container = context.session.open_document(
                        source.path,
                        ExtractorOpenSettings.for_inspection(shared, self.options.images), passwords,
                        stop.should_stop, log)
                except EngineException as error:
                    # C#'s two `catch ... when` filters, which Python has no equivalent of.
                    # EngineException is not a CliError, so the reference's second filter would
                    # catch a non-cancelled one and restate it as a DocumentError; that branch
                    # is written out here rather than left to fall through, because a Python
                    # `except` clause cannot decline a match.
                    if error.error == EngineError.CANCELLED:
                        raise stop.cause(source.path)
                    raise DocumentError(source.path, "could not open document", error)
                except CliError:
                    # `when (!(error is CliError))`: a usage error, a limit being exceeded or an
                    # interruption is not this document's failure and travels on.
                    raise
                except Exception as error:
                    # Exception and not BaseException, so KeyboardInterrupt and SystemExit still
                    # travel. The C# `catch (Exception)` likewise does not stop the failures
                    # .NET considers beyond catching.
                    raise DocumentError(source.path, "could not open document", error)

                # C#'s `using (container)`.
                with container:
                    try:
                        # The written paths extract_all returns are discarded, as they are in
                        # the C#: every file is reported by the engine writing it, and nothing
                        # here summarises them. The list is what the unit suite asserts on.
                        subfiles.extract_all(container, source.path, shared, self.options)
                    except CliError:
                        raise
                    except Exception as error:
                        # C#'s error.Message. str(error) is that text for everything this can
                        # catch: EngineException passes its message to Exception's constructor,
                        # so both spellings return the engine's own words.
                        raise DocumentError(source.path, str(error), error)

                diagnostics.end_document(source.path)
                diagnostics.check_document_timeout(source.path)
                diagnostics.check_limits()
                diagnostics.pause_if_requested()

            # Named as the reference names it. Nothing is shadowed: this module does not import
            # runtime.exit_code, extract producing only what BatchRunner.run decides.
            exit_code = batch.run(inputs, extract_one)

            diagnostics.end_run(batch.success_count, batch.failure_count)
            return exit_code

        return command_execution.with_session(context, run)

    def describe_plan(self, context: CommandContext) -> str:
        shared = context.shared_options
        options_validator.validate_extract(shared, self.options)

        enumerator = InputEnumerator(context.file_system, context.console)
        inputs = enumerator.enumerate(shared.input)

        cardinality = output_planner.resolve_cardinality(shared, len(inputs), combine=False)

        lines: List[str] = [
            "command extract",
            plan_format.inputs(inputs),
            shared.to_canonical_string(),
            # The C# overloads NoneOr for a string and for a list; Python has no overloading,
            # so the list form is plan_format.none_or_list. Both of these are lists.
            "include " + plan_format.none_or_list(self.options.include_patterns),
            "exclude " + plan_format.none_or_list(self.options.exclude_patterns),
            "images " + plan_format.yes_no(self.options.images),
            "make-dirs " + plan_format.yes_no(self.options.replicate_tree),
            "sub-id " + plan_format.yes_no(self.options.append_subfile_id),
            "cardinality " + RunPlan.cardinality_text(cardinality)
        ]
        return "\n".join(lines)
