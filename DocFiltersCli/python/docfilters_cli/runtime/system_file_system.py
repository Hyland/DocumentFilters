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
import stat
import sys
from typing import BinaryIO, Dict, List, Optional

from . import case_probe
from .i_file_system import IFileSystem


class SystemFileSystem(IFileSystem):
    """
    The real filesystem.

    Every path this returns uses forward slashes, whatever separator the OS or the
    caller used going in: NameMatcher and the rest of the portable walk logic
    compare paths as plain strings, so a backslash on Windows would make every glob
    and regex platform-specific.
    """

    def __init__(self):
        self._case_sensitivity: Dict[str, bool] = {}

    def file_exists(self, path: str) -> bool:
        return os.path.isfile(path)

    def directory_exists(self, path: str) -> bool:
        return os.path.isdir(path)

    def list_directory(self, path: str) -> List[str]:
        return [_normalise(os.path.join(path, name)) for name in os.listdir(path)]

    def is_hidden(self, path: str) -> bool:
        """
        A dot-prefixed name is hidden on every platform this runs on, in addition to
        whatever the OS itself flags — POSIX has no hidden attribute at all, and
        checking only the Windows attribute would make --include-hidden a no-op there.
        """
        name = os.path.basename(path.rstrip('/'))
        if name.startswith('.'):
            return True

        return _has_attribute(path, stat.FILE_ATTRIBUTE_HIDDEN)

    def is_symbolic_link(self, path: str) -> bool:
        # A junction is a reparse point that os.path.islink does not report, and the
        # walk has to treat one exactly like a symlink or -L/--follow could recurse
        # forever on Windows.
        #
        # False for a path that does not exist, because extraction asks about every
        # component of a file it is about to create. lexists rather than exists, so a
        # dangling link still answers true: writing through it would create its target.
        if not os.path.lexists(path):
            return False

        return os.path.islink(path) or _has_attribute(path, stat.FILE_ATTRIBUTE_REPARSE_POINT)

    def get_directory_identity(self, path: str) -> str:
        return _normalise(os.path.realpath(path)).rstrip('/')

    def is_case_sensitive(self, path: str) -> bool:
        """
        Whether the volume holding path tells names apart by case, measured where it can
        be and assumed where it cannot.

        The host default alone is wrong in both directions, and only one of them is safe.
        On Windows and macOS, guessing case-insensitive for a case-sensitive volume refuses
        a run the user can fix with --out-name. On Linux, guessing case-sensitive for a
        mounted NTFS or exFAT volume lets two planned outputs collide into one file and
        overwrite each other -- the very defect the collision check exists to catch.

        So the directory is asked, and asked about itself: the probe flips the case of
        something already inside it and looks for that spelling in the same directory.
        Nothing is created, which is what lets the collision check run before any output
        exists and --dry-run still write nothing. A directory that cannot answer -- empty,
        unreadable, or holding nothing with a letter in its name -- falls back to the host
        default.
        """
        directory = _nearest_existing_directory(path)
        if directory is None:
            return _HOST_IS_CASE_SENSITIVE

        cached = self._case_sensitivity.get(directory)
        if cached is not None:
            return cached

        sensitive = _probe(directory)
        if sensitive is None:
            sensitive = _HOST_IS_CASE_SENSITIVE
        self._case_sensitivity[directory] = sensitive
        return sensitive

    def open_read(self, path: str) -> BinaryIO:
        return open(path, 'rb')

    def create_file(self, path: str) -> BinaryIO:
        directory = os.path.dirname(path)
        if directory:
            self.create_directory(directory)

        return open(path, 'wb')

    def create_directory(self, path: str) -> None:
        os.makedirs(path, exist_ok=True)

    def move_file(self, source: str, destination: str) -> None:
        # os.replace rather than os.rename, which refuses an existing destination on
        # Windows; File.Move(overwrite: true) replaces on every platform, and so does this.
        os.replace(source, destination)

    def read_all_lines(self, path: str) -> List[str]:
        with open(path, 'r', encoding='utf-8-sig') as handle:
            text = handle.read()

        # Not str.splitlines(), which also breaks on vertical tab, form feed and
        # U+2028 — all legal in a filename, so a --files-from list naming one would
        # split into two paths here and stay whole in the other three ports.
        lines = text.split('\n')
        if lines and lines[-1] == '':
            lines.pop()
        return lines


_HOST_IS_CASE_SENSITIVE = sys.platform not in ('win32', 'darwin')


def _nearest_existing_directory(path: str) -> Optional[str]:
    """
    The first ancestor of path that exists, or None.

    The output directory usually does not exist yet when the plan is checked -- creating
    it is part of the run -- so the question is answered about the nearest ancestor that
    does, which is on the same volume.
    """
    candidate = os.path.dirname(os.path.abspath(path))
    while candidate:
        if os.path.isdir(candidate):
            return candidate

        parent = os.path.dirname(candidate)
        if parent == candidate:
            return None

        candidate = parent

    return None


def _probe(directory: str) -> Optional[bool]:
    """
    True, false, or None when the directory cannot be asked.

    os.scandir rather than os.listdir, because measure stops at the first name with a
    letter in it and a lazy listing lets it stop there too, as the reference's
    EnumerateFileSystemEntries does. A directory that cannot be listed at all raises
    rather than returning nothing, and is the one case this answers for itself.
    """
    try:
        with os.scandir(directory) as entries:
            return case_probe.measure(directory, (entry.name for entry in entries), os.path.exists)
    except OSError:
        return None


def _normalise(path: str) -> str:
    return path.replace('\\', '/')


def _has_attribute(path: str, attribute: int) -> bool:
    """A Windows file attribute, read without following a link. Always false elsewhere."""
    if sys.platform != 'win32':
        return False

    return bool(os.lstat(path).st_file_attributes & attribute)
