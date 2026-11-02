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

import io
from typing import Dict, List, Optional

from docfilters_cli.runtime.i_file_system import IFileSystem


class InMemoryFileSystem(IFileSystem):
    """
    A filesystem that lives entirely in a dictionary, so the directory walk and the
    output planner can be tested without touching disk.

    Paths are normalised to forward slashes throughout, which is also how the real
    implementation matches globs, so a test written here means the same thing on
    Windows and Linux.

    This is test infrastructure, so unlike the project under test it is fully
    implemented.
    """

    def __init__(self):
        self._files: Dict[str, bytes] = {}
        self._directories = set()
        self._hidden = set()
        self._symlinks = set()
        self._link_targets: Dict[str, str] = {}

        self._same_file: Dict[str, str] = {}
        """
        Names this filesystem matches to one file, beyond matching them exactly.

        Stated case by case rather than folded by a rule, because the rule is the thing
        under test. A real volume decides for itself which spellings are one file -- NTFS
        matches Å.txt to å.txt, measured -- and the tool's own collision key cannot always
        predict it. Writing the pair down here lets a test say "these two reach one file"
        without this fake acquiring a Unicode table the thing it stands in for would not
        agree with anyway.
        """

        self.case_sensitive = True
        """
        What this filesystem claims about case -- True unless a test says otherwise.

        Case-sensitive by default because this fake stores paths in a plain dictionary and
        so behaves that way; a default of anything else would have the fake's own lookups
        disagree with its answer. Tests covering Windows and macOS set it False. The shared
        plan corpus leaves it alone, so its cases read the same on every platform.
        """

        self.unreadable = set()
        """Directories this filesystem refuses to list, as a real one may."""

        self.created: List[str] = []
        """Everything written through create_file, for assertions."""

    def treat_as_same_file(self, path: str, same_as: str) -> 'InMemoryFileSystem':
        """Declares that two names reach one file."""
        self._same_file[_normalise(path)] = _normalise(same_as)
        return self

    def _key(self, path: str) -> str:
        """The one name a file is stored under, whatever spelling was asked for."""
        normalised = _normalise(path)
        return self._same_file.get(normalised, normalised)

    def add_file(self, path: str, contents: str = "") -> 'InMemoryFileSystem':
        """Adds a file, creating its parent directories."""
        path = self._key(path)
        self._files[path] = contents.encode('utf-8')
        self._add_parents(path)
        return self

    def add_directory(self, path: str) -> 'InMemoryFileSystem':
        """Adds an empty directory."""
        path = _normalise(path)
        self._directories.add(path)
        self._add_parents(path)
        return self

    def mark_hidden(self, path: str) -> 'InMemoryFileSystem':
        """Marks an existing entry hidden, so the walk skips it without --include-hidden."""
        self._hidden.add(_normalise(path))
        return self

    def mark_symlink(self, path: str, target: Optional[str] = None) -> 'InMemoryFileSystem':
        """
        Marks a directory as a symlink, so the walk enters it only under -L.

        Args:
            path: The link itself.
            target: What it resolves to. Point this at an ancestor to build the cycle
                that --follow has to survive; leave it None when the link's
                destination does not matter to the test.
        """
        key = _normalise(path)
        self._symlinks.add(key)

        # The link is a real directory entry, so a walk of its parent finds it.
        # Without this the walk never sees the link at all and any test about
        # following one passes without exercising anything.
        self._directories.add(key)
        self._add_parents(key)

        if target is not None:
            self._link_targets[key] = _normalise(target)

        return self

    def file_exists(self, path: str) -> bool:
        return self._key(path) in self._files

    def directory_exists(self, path: str) -> bool:
        return _normalise(path) in self._directories

    def list_directory(self, path: str) -> List[str]:
        requested = _normalise(path)

        # The same shape of failure a real filesystem raises for a directory the process
        # may not read: an exception, not an empty listing.
        if requested in self.unreadable:
            raise PermissionError("Permission denied: '" + requested + "'")

        # Listing through a link yields the target's children, re-rooted under the
        # path that was asked for — which is what makes a link to an ancestor
        # generate the endless corpus/loop/loop/… that --follow has to survive.
        real = self._resolve_through_links(requested)

        if real != requested:
            return [requested + child[len(real):] for child in self.list_directory(real)]

        prefix = requested
        if prefix and not prefix.endswith('/'):
            prefix += '/'

        children = set()
        for candidate in list(self._files.keys()) + list(self._directories):
            if not candidate.startswith(prefix) or len(candidate) == len(prefix):
                continue

            rest = candidate[len(prefix):]
            slash = rest.find('/')
            children.add(prefix + (rest if slash < 0 else rest[:slash]))

        # Deliberately unordered: IFileSystem promises no order and a real directory
        # listing gives none. A fake that sorted would let a walk ignoring --sort
        # pass the sorting tests.
        return sorted(children, reverse=True)

    def is_hidden(self, path: str) -> bool:
        return _normalise(path) in self._hidden

    def is_symbolic_link(self, path: str) -> bool:
        return _normalise(path) in self._symlinks

    def get_directory_identity(self, path: str) -> str:
        """
        The directory a path resolves to, following any link. Two paths reaching the
        same directory return the same identity, which is what lets the walk detect a
        cycle.
        """
        return self._resolve_through_links(_normalise(path))

    def is_case_sensitive(self, path: str) -> bool:
        return self.case_sensitive

    def _resolve_through_links(self, path: str) -> str:
        """
        Rewrites a path through however many links stand in it, so that every route to
        one directory produces the same answer.

        corpus/loop/loop resolves through corpus/loop → corpus twice and lands back at
        corpus. The iteration is bounded because a link to its own ancestor never
        settles — that is the whole point of the case, and stopping it is the walk's
        job, not the filesystem's.
        """
        for _ in range(64):
            rewritten = self._rewrite_longest_link_prefix(path)

            if rewritten == path:
                return path

            path = rewritten

        return path

    def _rewrite_longest_link_prefix(self, path: str) -> str:
        best_link = None

        for link in self._link_targets:
            matches = path == link or path.startswith(link + '/')

            if matches and (best_link is None or len(link) > len(best_link)):
                best_link = link

        if best_link is None:
            return path

        return self._link_targets[best_link] + path[len(best_link):]

    def open_read(self, path: str):
        key = self._key(path)
        if key not in self._files:
            raise FileNotFoundError("No such file in the fake filesystem: " + key)

        return io.BytesIO(self._files[key])

    def create_file(self, path: str):
        key = self._key(path)
        self.created.append(key)
        self._add_parents(key)
        self._files[key] = b''
        return _CommittingStream(self, key)

    def read_text(self, path: str) -> str:
        """Reads back what was written to a created file."""
        return self._files[self._key(path)].decode('utf-8')

    def create_directory(self, path: str) -> None:
        self.add_directory(path)

    def move_file(self, source: str, destination: str) -> None:
        source = self._key(source)
        destination = self._key(destination)

        if source not in self._files:
            raise FileNotFoundError("no such file: " + source)

        self._files[destination] = self._files.pop(source)
        self.created.append(destination)

    def read_all_lines(self, path: str) -> List[str]:
        with self.open_read(path) as stream:
            text = stream.read().decode('utf-8')

        lines = text.replace('\r\n', '\n').replace('\r', '\n').split('\n')
        if lines and lines[-1] == '':
            lines.pop()
        return lines

    def _add_parents(self, path: str) -> None:
        slash = path.rfind('/')
        while slash > 0:
            path = path[:slash]
            self._directories.add(path)
            slash = path.rfind('/')


class _CommittingStream(io.BytesIO):
    """
    A stream that writes its contents back into the fake when closed.

    Without this the stream returned by create_file would be detached: everything
    written to it would be discarded, and a later open_read would see an empty file.
    Conversion output could then never be asserted on.
    """

    def __init__(self, file_system: InMemoryFileSystem, path: str):
        super().__init__()
        self._file_system = file_system
        self._path = path

    def close(self) -> None:
        # Read the buffer out before closing: BytesIO releases it, and getvalue()
        # afterwards raises.
        if not self.closed:
            self._file_system._files[self._path] = self.getvalue()

        super().close()


def _normalise(path: str) -> str:
    return path.replace('\\', '/').rstrip('/')
