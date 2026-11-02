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

# ***************************************************************************
# Document Filters Example - a command line tool.
#
# Convert, extract, list, identify and inspect documents from the shell.
#
#   docfilters convert --to pdf --out-dir out/ ./corpus
#   docfilters extract -i --out-dir dump/ bundle.pst
#   docfilters identify --json *.bin
#
# The same command line is implemented in C#, Python, Java and C++17. See
# ../README.md for how the pieces fit together and why they are arranged this
# way; if you are here to learn the API, read the execution package and ignore
# the rest.
# ***************************************************************************
#
# Entry point. Wires up the console and filesystem, runs the command line, and turns
# whatever happens into a single exit code.
#
# A module of functions, where the C# has a static class: that is the house rule for a
# C# `static class`, and runtime/exit_code.py, planning/output_planner.py and
# commands/command_execution.py are already spelled the same way. `main` is the
# counterpart of Program.Main, and __main__.py is what makes `python -m docfilters_cli`
# reach it.

import sys
from typing import List, Optional

from .cli.command_line_app import CommandLineApp
from .configuration.shared_options import ConsoleOptions
from .runtime.cli_error import CliError
from .runtime.i_console import IConsole
from .runtime.i_file_system import IFileSystem
from .runtime.interruption import Interruption
from .runtime.system_console import SystemConsole
from .runtime.system_file_system import SystemFileSystem


def main(argv: Optional[List[str]] = None) -> int:
    """
    Args:
        argv: The arguments after the program name, or None to read sys.argv.

    C#'s Main receives its arguments already stripped of the executable name; Python
    leaves sys.argv[0] in place, so the default slices it off here rather than making
    every caller remember to. The parameter exists so a test can pass a list directly,
    which is what C# gets for free from a method that takes one.

    Returns:
        A value from runtime.exit_code.
    """
    args = list(sys.argv[1:]) if argv is None else list(argv)

    # Ctrl+C is recorded rather than allowed to kill the process, so the run reports and
    # exits 130 through the same path as every other exit code.
    return run(args, SystemConsole(pre_scan_console_options(args)), SystemFileSystem(),
               engine=None, interruption=Interruption.install())


def run(args: List[str], console: IConsole, file_system: IFileSystem, engine=None,
        interruption: Optional[Interruption] = None, clock=None, open_viewer=None) -> int:
    """
    Everything main does once its three dependencies exist.

    Split out so a test can drive a whole command line -- argv in, exit code and console
    output out -- without the real console, the real filesystem or the native library.
    That path is otherwise reachable only by starting a process, which needs a licensed
    machine to tell a wiring mistake from an engine one.

    Args:
        engine: None means the real engine, built on first use by whichever command needs
            it.

    engine is left unannotated, and defaulted rather than required as the C# has it.
    Nothing above the seam may import the DocumentFilters binding at module scope, and
    keeping the annotation off keeps this module's imports to the ones it calls;
    cli/command_line_app.py leaves its own engine parameter unannotated for the same
    reason. C# can require the argument because `engine: null` at the call site reads as
    a decision; in Python a positional None before a keyword interruption would not, so
    the default carries the meaning instead.
    """
    app = CommandLineApp(console, file_system, engine, interruption, clock, open_viewer)

    try:
        return app.run(args)
    except CliError as error:
        return report_error(error, console)


def pre_scan_console_options(args: List[str]) -> ConsoleOptions:
    """
    Reads --silent, --verbose and --color directly out of args, before anything is bound.

    SystemConsole needs these *before* the first line is written, but they only exist on
    the SharedOptions that CommandLineApp.bind produces, and CommandLineApp.run starts
    writing -- --print-contract, --dry-run, the command itself -- in the same call that
    does the binding, with no point in between for main to step in. A raw scan sidesteps
    the ordering problem entirely: this tool disables prefix abbreviation and none of
    these three takes a value, so --silent has exactly one spelling, and a plain list
    search for it is exact, not a heuristic. Re-validating them properly -- an unknown
    argument, a value stuck to --silent -- is still CommandLineApp.bind's job; this only
    ever needs to get the three booleans right for a well-formed command line.
    """
    # Python has no object initialiser, so the three fields are assigned after
    # construction, as the rest of this port does. `in` on a list is Enumerable.Contains,
    # and both compare by equality.
    options = ConsoleOptions()
    options.silent = "--silent" in args
    options.verbose = "--verbose" in args
    options.color = "--color" in args
    return options


def report_error(error: CliError, console: IConsole) -> int:
    """
    Maps an error to its exit code and prints its message to stderr.

    Error text is part of the cross-language contract, so it is composed here from
    CliError.code and the message rather than being left to whatever the CLI parsing
    library would have said. The run-level shape is code first, e.g.
    E_INIT_FAILED: could not initialise Document Filters -- code first, then the message,
    with no other punctuation.
    """
    # `error.path is None` and not plain truthiness: an empty path is not the same as an
    # absent one, and only null selects the short form in the reference.
    console.write_error("{0}: {1}".format(error.code, error.message) if error.path is None
                        else "{0}: {1}: {2}".format(error.path, error.code, error.message))
    return error.exit_code
