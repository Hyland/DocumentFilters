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


class CommandContext:
    """
    Everything a command needs that is not its own options. Per-command options are
    passed to the command's constructor instead, so there is no object here holding seven
    verbs' worth of options with six of them unused.
    """

    def __init__(self):
        self.shared_options = None

        self.console = None

        self.file_system = None

        self.session = None
        """
        The initialised engine. None for commands that do not open documents, and created
        lazily so that --print-plan never needs a license.

        Left untyped rather than annotated execution.DocumentFiltersSession, which keeps
        this module free of any import that reaches the native library: the C# needs the
        type named to declare the field at all, Python does not, so the abstraction is
        cheaper here than there.
        """

        self.interruption = None
        """
        Whether the user has interrupted the run, or None when nothing is watching.

        None for a caller that installed no handler — every test, and --dry-run, which
        finishes long before anyone could press anything.
        """

        self.clock = None
        """
        A monotonic reading in seconds for Diagnostics, or None for the real one.

        Here rather than inside each command because the commands build their own
        Diagnostics and a test driving the whole CLI has no other way in. It is what lets
        the limit cases assert on a document that outran --timeout without the suite
        waiting out the smallest value the validator accepts.
        """

        self.open_viewer = None
        """
        What --view hands the finished output to, or None for the desktop's own handler.

        Here for the same reason clock is. What --view has to get right is not what opens
        but *when*: the output has to be finished before anything is asked to open it, and
        under --combine it was not -- the canvas and the writer were still open, so a PDF
        whose trailer is written on close was handed over half-written.
        """


class ICommand:
    """One verb."""

    @property
    def name(self) -> str:
        """The verb as typed, e.g. convert."""
        raise NotImplementedError

    @property
    def requires_engine(self) -> bool:
        """True when this command opens documents and therefore needs the engine."""
        raise NotImplementedError

    def execute(self, context: CommandContext) -> int:
        """Runs the command. Returns a value from runtime.exit_code."""
        raise NotImplementedError

    def describe_plan(self, context: CommandContext) -> str:
        """
        Runs stages 1 to 3 only and returns the canonical plan text, for --print-plan.
        Never opens a document.
        """
        raise NotImplementedError
