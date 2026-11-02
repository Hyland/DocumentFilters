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

# The one piece every execute needs and none of them owns: DocumentFiltersSession's
# lifetime.
#
# A CLI process runs exactly one command, so creating the session at the start of that
# one execute call and closing it at the end already matches its documented lifetime --
# "once for the lifetime of the process" -- without CommandLineApp or Program needing to
# know about it. A context hand-wired with a session already -- as tests are free to do
# -- is left alone and not closed here, since this did not create it.
#
# A module of functions rather than a class, because the C# is a static class: the house
# rule for this port, and what runtime/exit_code.py and planning/output_planner.py
# already are.

from typing import Callable, Optional

from ..execution.diagnostics import Diagnostics
from ..execution.stop_signal import StopSignal
from ..runtime.interruption import Interruption
from .i_command import CommandContext


def watch_for_stop(diagnostics: Diagnostics, interruption: Optional[Interruption]) -> StopSignal:
    """
    A fresh StopSignal for one document.

    Per document, not per run: the signal latches the first reason it sees, and a deadline
    belongs to the document it was measured against. interruption is None for a caller
    that installed no handler, which is every test that does not ask for one.
    """
    return StopSignal(diagnostics, interruption)


def with_session(context: CommandContext, action: Callable[[], int]) -> int:
    # Not a contextmanager, though Python has one and the shape would suit it. The
    # reference takes the body as a callable and the four call sites read the same in both
    # ports this way; a `with` form would also have to hand the caller its own return
    # value back out, which is the one thing a context manager cannot do.
    owns_session = context.session is None
    if owns_session:
        # Imported here rather than at the top of the module. document_filters_session is
        # the only file above the seam that names the concrete engine, and it defers that
        # import into create for the same reason -- but importing this module must not be
        # what pulls the whole execution package in, because every command imports it and
        # --print-plan reaches none of them.
        from ..execution.document_filters_session import DocumentFiltersSession

        context.session = DocumentFiltersSession.create(context.shared_options.library,
                                                        context.shared_options.processing.temp_dir)

    try:
        return action()
    finally:
        if owns_session:
            context.session.close()
            context.session = None
