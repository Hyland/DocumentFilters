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

from ..configuration.command_options import FormatsOptions
from ..execution.catalog_reports import CatalogReports
from ..runtime import exit_code
from . import command_execution, plan_format
from .i_command import CommandContext, ICommand


class FormatsCommand(ICommand):
    """
    formats — list the file types this build can read, with their ids, extensions and
    names. Takes no input files, which is why it reports no capability flags: those come
    from an opened document, so identify owns them.
    """

    def __init__(self, options: FormatsOptions):
        self.options = options
        """
        The bound per-command options. Readable so the binding itself can be tested.

        A plain attribute where the C# is a get-only auto-property. Python has no cheap
        read-only attribute, and wrapping this in a @property purely to withhold a setter
        would add three lines that say nothing the name does not; discovery/input_source.py
        settles the same question the same way.
        """

    @property
    def name(self) -> str:
        return "formats"

    @property
    def requires_engine(self) -> bool:
        return True

    def execute(self, context: CommandContext) -> int:
        # The reference hands the body to CommandExecution.WithSession as a lambda; a Python
        # lambda holds one expression and this body is two statements, so it is a nested def.
        def run() -> int:
            CatalogReports(context.session, context.console).write_formats(self.options)
            return exit_code.SUCCESS

        return command_execution.with_session(context, run)

    def describe_plan(self, context: CommandContext) -> str:
        lines: List[str] = [
            "command formats",
            "json " + plan_format.yes_no(self.options.json),
            "filter " + plan_format.none_or(self.options.filter)
        ]
        return "\n".join(lines)
