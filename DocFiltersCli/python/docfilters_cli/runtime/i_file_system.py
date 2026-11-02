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

from abc import ABC, abstractmethod
from typing import BinaryIO, List


class IFileSystem(ABC):
    """
    The filesystem operations this tool needs, behind one seam so the directory
    walk and the output planner can be unit tested entirely in memory.
    """

    @abstractmethod
    def file_exists(self, path: str) -> bool:
        pass

    @abstractmethod
    def directory_exists(self, path: str) -> bool:
        pass

    @abstractmethod
    def list_directory(self, path: str) -> List[str]:
        """Immediate children of a directory, files and directories alike, unsorted."""
        pass

    @abstractmethod
    def is_hidden(self, path: str) -> bool:
        """True for entries the walk skips unless --include-hidden is given."""
        pass

    @abstractmethod
    def is_symbolic_link(self, path: str) -> bool:
        """True for symlinked directories, which the walk enters only under -L/--follow."""
        pass

    @abstractmethod
    def get_directory_identity(self, path: str) -> str:
        """
        A stable identity for a directory, with links resolved — two paths that
        reach the same directory return the same value.

        Needed to make -L/--follow terminate. Knowing that an entry is a link
        is not enough: a link pointing at one of its own ancestors produces an
        endless supply of distinct path strings, so a set of visited paths
        never detects the cycle. A set of visited identities does.
        """
        pass

    @abstractmethod
    def is_case_sensitive(self, path: str) -> bool:
        """
        Whether the filesystem holding path tells two names apart by case -- true where
        Report.txt and report.txt are two files.

        Asked of the filesystem rather than derived from the host, for the same reason
        get_directory_identity is: the planner cannot work it out and guessing is wrong
        often enough to matter. An --out-dir may sit on a mounted NTFS or exFAT volume
        under Linux, or on a case-sensitive volume under macOS, and the answer decides
        whether two planned outputs are one file.

        path need not exist: the question is about the volume it would live on, which is
        why the collision check can ask before anything is written and --dry-run can ask
        without touching the disk at all.
        """
        pass

    @abstractmethod
    def open_read(self, path: str) -> BinaryIO:
        pass

    @abstractmethod
    def create_file(self, path: str) -> BinaryIO:
        """Creates or truncates a file, creating parent directories as needed."""
        pass

    @abstractmethod
    def create_directory(self, path: str) -> None:
        pass

    @abstractmethod
    def move_file(self, source: str, destination: str) -> None:
        """
        Renames source to destination, replacing an existing file at the destination.

        For web-safe, which is the one format whose extension is not knowable until the
        page has been rendered: the engine picks PNG, GIF, JPEG or WebP per page, and the
        file is named for what it turned out to be. Whether replacing is allowed at all is
        settled before this is called, by the same output policy every other target goes
        through.
        """
        pass

    @abstractmethod
    def read_all_lines(self, path: str) -> List[str]:
        """Reads a --files-from list, one path per line."""
        pass
