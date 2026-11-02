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

from typing import Optional

from ..cli.contract_dump import ContractDump
from ..runtime import exit_code
from .i_command import CommandContext, ICommand


class HelpCommand(ICommand):
    """
    help [command] — usage for the tool, or for one command.

    The plain-text rendering comes from the CLI parsing library and therefore differs
    slightly between the four language implementations. That is accepted: help text is
    deliberately outside the cross-language contract.

    help --json is different. It emits the command and argument reference as structured
    data, so it must be identical everywhere, and it renders through
    ContractDump.render_json rather than the line-oriented ContractDump.render. The two
    share the argument table, not the renderer.
    """

    def __init__(self, topic: Optional[str], json: bool):
        self._topic = topic
        self._json = json

    @property
    def name(self) -> str:
        return "help"

    @property
    def requires_engine(self) -> bool:
        return False

    def execute(self, context: CommandContext) -> int:
        if self._json:
            context.console.write_line(ContractDump.render_json())
        elif self._topic is None:
            context.console.write_line(ContractDump.render())
        else:
            context.console.write_line(ContractDump.render_command(self._topic))

        return exit_code.SUCCESS

    def describe_plan(self, context: CommandContext) -> str:
        """
        help processes no documents, so --dry-run has nothing to add to what running it
        outright already does: it never opens the engine.
        """
        return "command help"
