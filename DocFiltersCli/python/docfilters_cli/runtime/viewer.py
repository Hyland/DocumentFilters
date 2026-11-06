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

import os
import subprocess
import sys

from .i_console import IConsole

# --view: hand the finished output to whatever the desktop opens it with.
#
# Best effort, and deliberately so — A7 settled that, and it is why --view is
# excluded from the conformance corpus: what opens is the machine's business, there
# is no output of our own to compare, and a headless box has nothing to open it with
# at all. A failure to launch is reported and ignored rather than failing the run,
# because the conversion itself already succeeded.
#
# Three platforms, three commands, the same three doc2text uses: ShellExecute on
# Windows, open on macOS, xdg-open elsewhere. Windows goes through the shell
# (os.startfile), which is what makes a file association apply rather than trying to
# execute the document.


def open_in_viewer(path: str, console: IConsole) -> None:
    """Opens path, reporting but not failing on trouble."""
    if not path:
        return

    try:
        _start(path)
    except Exception as error:
        # Not a DocumentError: nothing about the document failed, and the output is
        # on disk either way.
        console.write_error("--view could not open {0}: {1}".format(path, error))


def _start(path: str) -> None:
    # The path is passed as its own argument rather than interpolated into a command
    # line, so the C# port's Quote helper has no counterpart here — quoting would
    # make the quotes part of the filename, and there is no shell to protect it from.
    if sys.platform == 'win32':
        os.startfile(path)
    elif sys.platform == 'darwin':
        subprocess.Popen(['open', path])
    else:
        subprocess.Popen(['xdg-open', path])
