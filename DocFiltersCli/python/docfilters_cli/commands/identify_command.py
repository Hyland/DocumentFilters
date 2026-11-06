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
from ..configuration.command_options import IdentifyOptions
from ..discovery.input_enumerator import InputEnumerator
from ..execution import debug_input
from ..execution.diagnostics import Diagnostics
from ..runtime.batch_runner import BatchRunner
from ..runtime.cli_error import CliError, DocumentError
from ..runtime.json_writer import JsonWriter
from . import command_execution, plan_format
from .i_command import CommandContext, ICommand

# The three capability bits. The C# reads them from ISYS11dfConstants, the binding's own
# constant class, which it reaches through the Hyland.DocumentFilters namespace -- so the
# reference names them without appearing to cross the seam. The Python binding publishes them
# only as module constants on a package whose import loads the native library, and nothing
# above the seam may import that at module scope: that is what lets the pure suite run from a
# clone with no engine present. So they are declared here instead, which is the same trade
# engine/i_document_filters_engine.py makes for OpenType and its four other enums. The values
# are from the C header (DocumentFilters.h: IGR_FILE_SUPPORTS_TEXT 1, _SUBFILES 2, _HDHTML 4)
# and are ABI, so they cannot drift.
_IGR_FILE_SUPPORTS_TEXT = 1
_IGR_FILE_SUPPORTS_SUBFILES = 2
_IGR_FILE_SUPPORTS_HDHTML = 4


class IdentifyCommand(ICommand):
    """identify — detect and print each input's format. No conversion."""

    def __init__(self, options: IdentifyOptions):
        self.options = options
        """The bound per-command options. Readable so the binding itself can be tested."""

    @property
    def name(self) -> str:
        return "identify"

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

            diagnostics = Diagnostics(shared.diagnostics, context.console, context.session.api, context.clock)
            batch = BatchRunner(shared.batch, context.console, context.interruption)
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

                # identify never opens the document, so there is no open callback and
                # nothing to cancel or log — but what the engine makes of a truncated
                # or buffered file is exactly what the debug switches ask about.
                #
                # Named debug_stream rather than the reference's debugInput, because
                # debug_input is the module this calls into. The C# spells the lifetime as
                # `using Stream debugInput`, which does nothing when the stream is null;
                # `with None:` raises, so the close is a try/finally carrying the None test
                # that `using` does implicitly.
                debug_stream = debug_input.for_document(source.path, shared.diagnostics,
                                                       context.file_system, context.console,
                                                       source.path)
                try:
                    try:
                        identified = (
                            context.session.api.identify_file(source.path) if debug_stream is None
                            else context.session.api.identify_file_from_stream(debug_stream, source.path))
                    except CliError:
                        # The reference writes the two clauses here as one catch with a
                        # `when (!(error is CliError))` filter. Python has no exception
                        # filter, so the excluded type is caught first and re-raised
                        # untouched: a bare `raise`, which keeps the original traceback
                        # rather than starting a new one at this line.
                        raise
                    except Exception as error:
                        raise DocumentError(source.path, "could not identify document", error)

                    capabilities = (_capabilities_text(identified.capabilities)
                                    if self.options.capabilities else None)

                    if self.options.json:
                        json.begin_object().property("path", source.path).property("format", identified.id)
                        if capabilities is not None:
                            json.property("capabilities", capabilities)
                        json.property("mime", identified.mime_type).property("name", identified.name).end_object()
                    else:
                        capabilities_field = ("caps={0} ".format(capabilities)
                                              if capabilities is not None else "")
                        context.console.write_line(
                            "{0}: format={1} {2}mime={3} name={4}".format(
                                source.path, identified.id, capabilities_field,
                                _text(identified.mime_type), _text(identified.name)))
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

        lines = [
            "command identify",
            plan_format.inputs(inputs),
            shared.to_canonical_string(False),
            "json " + plan_format.yes_no(self.options.json),
            "capabilities " + plan_format.yes_no(self.options.capabilities)
        ]
        return "\n".join(lines)


def _capabilities_text(capabilities: int) -> str:
    """
    The three bits doc2text itself reports (main.cpp's IGR_Get_Stream_Type handling):
    IGR_FILE_SUPPORTS_TEXT, IGR_FILE_SUPPORTS_SUBFILES, IGR_FILE_SUPPORTS_HDHTML.

    The C#'s `private static` member, so a module-level private function here, as
    execution/debug_input.py, execution/structure_report.py and planning/output_planner.py
    already settle for their packages.
    """
    caps: List[str] = []
    if (capabilities & _IGR_FILE_SUPPORTS_TEXT) != 0:
        caps.append("text")
    if (capabilities & _IGR_FILE_SUPPORTS_SUBFILES) != 0:
        caps.append("subfiles")
    if (capabilities & _IGR_FILE_SUPPORTS_HDHTML) != 0:
        caps.append("hd")

    return "none" if len(caps) == 0 else ",".join(caps)


def _text(value: Optional[str]) -> str:
    """
    A field value as C# string interpolation would render it.

    Not in the C#, and it exists for one difference: C#'s $"mime={null}" writes an empty
    string, where "mime={0}".format(None) writes the four characters None. Both of the fields
    the text branch prints from EngineFileType — mime_type and name — are Optional[str] at
    the seam, so a file the engine places without naming a MIME type produces the same line
    in all four ports.

    execution/catalog_reports.py and execution/structure_report.py carry the same helper for
    the same reason, and it is duplicated here rather than imported for the reason the first
    of those gives: it is private there, and four lines of the obvious is a smaller thing to
    own three times than a module edge from a command into a report for one expression.

    The --json branch needs none of this. JsonWriter.value writes null for None, which is
    what the C# JsonWriter does with a null string, so the two agree already.
    """
    return value if value is not None else ""
