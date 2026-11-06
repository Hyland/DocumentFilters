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

from ..configuration.command_options import OptionCatalogOptions
from ..execution.catalog_reports import CatalogReports
from ..runtime import exit_code
from . import command_execution, plan_format
from .i_command import CommandContext, ICommand


class OptionsCommand(ICommand):
    """
    options — list the engine processing options, the tunables passed via --options, with
    type, default and scope. Takes no input files.
    """

    def __init__(self, options: OptionCatalogOptions):
        self.options = options
        """
        The bound per-command options. Readable so the binding itself can be tested.

        A plain attribute where the C# is a get-only auto-property, for the reason
        formats_command.FormatsCommand.options gives.
        """

    @property
    def name(self) -> str:
        return "options"

    @property
    def requires_engine(self) -> bool:
        return True

    def execute(self, context: CommandContext) -> int:
        # A nested def rather than the reference's lambda, because the body is two
        # statements; formats_command.FormatsCommand.execute says the same.
        def run() -> int:
            CatalogReports(context.session, context.console).write_options(self.options)
            return exit_code.SUCCESS

        return command_execution.with_session(context, run)

    def describe_plan(self, context: CommandContext) -> str:
        lines: List[str] = [
            "command options",
            "json " + plan_format.yes_no(self.options.json),
            "filter " + plan_format.none_or(self.options.filter),
            "applies-to " + plan_format.none_or(self.options.applies_to)
        ]
        return "\n".join(lines)
