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

from ..runtime import exit_code
from . import command_execution
from .i_command import CommandContext, ICommand


class VersionCommand(ICommand):
    """
    version — the tool version and the Document Filters build it is running against.

    This is the one command that needs the engine without opening any document: the
    interesting half of the answer is the native library's version, which requires
    initialising it.
    """

    @property
    def name(self) -> str:
        return "version"

    @property
    def requires_engine(self) -> bool:
        return True

    def execute(self, context: CommandContext) -> int:
        # The reference passes the body to CommandExecution.WithSession as a lambda. A
        # Python lambda holds one expression, and this body is three statements, so the
        # closure is spelled out as a nested def. Same closure over context, same single
        # place that owns the session's lifetime.
        def run() -> int:
            context.console.write_line("docfilters {0}".format(_tool_version()))
            context.console.write_line(
                "Document Filters {0}".format(context.session.describe_version()))
            return exit_code.SUCCESS

        return command_execution.with_session(context, run)

    def describe_plan(self, context: CommandContext) -> str:
        """
        version reports the tool and native library versions outright; a dry run has
        nothing further to plan.
        """
        return "command version"


def _tool_version() -> str:
    """
    This tool's own version, for the first line version prints.

    Read from the package's own __version__ rather than from installed
    distribution metadata. importlib.metadata was the closer mirror of the
    reference — which reads a version .NET stamps into the assembly manifest —
    but it answers "unknown" when there is no dist-info to read, and running
    from a source checkout is how this port is run today. The version line is
    compared across implementations, so "unknown" was a difference with no
    meaning behind it.

    importlib.metadata is deliberately not consulted, not even as a first
    choice with this as the fallback. It answers for whatever distribution
    happens to be installed, which in an environment holding an older or newer
    docfilters_cli is not the code being run -- and this line is compared
    across implementations, so it would fail parity for a reason that has
    nothing to do with the checkout. A built wheel takes its metadata from this
    same declaration anyway, so the two agree whenever it matters; where they
    disagree, the checkout is the truth.

    A named function rather than the reference's one inline expression, because
    the import belongs inside it: at module scope it would be a circular import,
    the package importing a command that imports the package.
    """
    from .. import __version__
    return __version__


