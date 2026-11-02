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

from docfilters_cli import program
from docfilters_cli.engine.engine_exception import EngineError
from docfilters_cli.runtime.i_console import IConsole
from docfilters_cli.runtime.interruption import Interruption
from fakes.fake_document_filters_engine import FakeDocumentFiltersEngine
from fakes.in_memory_file_system import InMemoryFileSystem
from fakes.recording_console import RecordingConsole


class CliHarness:
    """
    Runs a whole command line the way the shell does, in process.

    argv in; exit code, stdout and stderr out. Everything between is the real thing -- the
    real parser, the real validator, the real walk, the real planner, the real commands,
    the real error-to-exit-code mapping -- with only the three edges swapped: the console,
    the filesystem and the engine.

    This covers the one seam the conformance corpus cannot reach. The corpus stops at bind
    and describe_plan; everything above that -- program.run, CommandLineApp.run's dispatch,
    --print-contract, the dry-run branch, the session's lifetime, and report_error's stderr
    format -- had no test at all before this.

    No license and no native library: the engine is a fake, so these run wherever the unit
    tests run. What they deliberately do not prove is that the real engine behaves as the
    fake claims -- that is tier 3's job.
    """

    def __init__(self, interruption: Optional[Interruption] = None):
        """
        Args:
            interruption: What the run watches for an interrupt, or None for the default,
                which is nothing watching.

        The C# has two constructors, one taking an Interruption and a parameterless one.
        Python has no overloading, so the two become one signature with a default -- which
        is also what program.run itself does with the same argument.
        """
        self._file_system = InMemoryFileSystem()
        self._interruption = interruption

        self.clock = TestClock()
        """
        A clock the test moves by hand, so a run can outlast --timeout without the suite
        waiting.
        """

        self.viewed: List[str] = []
        """
        What --view was handed, and (in viewed_content) what was in it at that moment.

        The content is read when the viewer is called rather than afterwards, because the
        question --view raises is one of timing: a combined output is not a finished file
        until its canvas or its writer is closed, and reading it after the run would look
        the same either way.
        """

        self.viewed_content: List[Optional[str]] = []

        self.engine = FakeDocumentFiltersEngine()
        """The engine the run sees. Script documents and failures on it before running."""

        self.console = RecordingConsole()
        """Everything the run wrote, kept after run returns."""

        self.exit_code = 0
        """
        The last run's exit code.

        A plain attribute, where the C# property has a private setter. Python has no way to
        say "read me but do not write me" that does not cost a property and a backing
        field, and nothing outside this class assigns it.
        """

    @property
    def file_system(self) -> InMemoryFileSystem:
        """The fake disk, so a test can assert what the run left on it."""
        return self._file_system

    @property
    def out(self) -> List[str]:
        return self.console.out_lines

    @property
    def output(self) -> str:
        """
        Everything written to stdout, verbatim.

        out only records whole lines, so it misses output written without one -- the JSON
        reports build a document and write it in a single call, which is exactly the output
        whose exact shape is worth asserting.
        """
        return self.console.output

    @property
    def errors(self) -> List[str]:
        return self.console.error_lines

    @property
    def created(self) -> List[str]:
        """The files the run created, for asserting on what it wrote."""
        return self._file_system.created

    def with_document(self, path: str, text: str = "") -> 'CliHarness':
        """
        Adds an input that exists on disk and opens in the engine.

        Both halves, always. A path in only one of the two is the setup mistake that
        otherwise shows up as a walk selecting nothing, or an open failing for a file the
        walk did find -- neither of which is what the test meant to say.
        """
        self._file_system.add_file(path)
        self.engine.add_document(path, text)
        return self

    def with_unreadable_document(self, path: str, error: EngineError) -> 'CliHarness':
        """Adds a file on disk that the engine will refuse to open."""
        self._file_system.add_file(path)
        self.engine.fail_to_open(path, error)
        return self

    def with_protected_document(self, path: str, password: str,
                               text: str = "") -> 'CliHarness':
        """Adds a file the engine opens only with the right password."""
        self._file_system.add_file(path)
        self.engine.add_protected_document(path, password, text)
        return self

    def with_unopenable_file(self, path: str) -> 'CliHarness':
        """
        Adds a file on disk that the engine has never heard of.

        The walk finds it and the open fails -- which is what a file of a format this build
        cannot read looks like from the outside.
        """
        self._file_system.add_file(path)
        return self

    def with_existing_output(self, path: str) -> 'CliHarness':
        """Puts a file where an output would go, for the overwrite rules."""
        self._file_system.add_file(path, "already here")
        return self

    def run(self, *args: str) -> int:
        """
        Runs the command line and returns the exit code.

        *args is C#'s `params string[]`, so a test writes the arguments out as it would
        type them rather than building a list.
        """
        self.exit_code = program.run(list(args), self.console, self._file_system,
                                     self.engine, self._interruption, self.clock.read,
                                     self._record_view)
        return self.exit_code

    def _record_view(self, path: str, console: IConsole) -> None:
        self.viewed.append(path)
        self.viewed_content.append(self._file_system.read_text(path)
                                   if self._file_system.file_exists(path) else None)


class TestClock:
    """
    A monotonic reading a test advances itself.

    Starts at zero and only moves when a test says so, so a run takes no real time however
    long it claims to have taken. Every harness run uses one, which also means timings
    printed by --time are reproducible rather than whatever the machine happened to do.

    Seconds as a float where the C# holds a TimeSpan, because that is what Diagnostics'
    clock reads in.
    """

    # Not a test class, whatever its name says to a collector that goes by prefix.
    __test__ = False

    def __init__(self):
        self.now = 0.0

    def read(self) -> float:
        return self.now

    def advance(self, seconds: float) -> None:
        self.now += seconds
