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

from typing import List

from ..configuration.value_parsers import is_all_whitespace
from ..runtime.cli_error import CliError, NoInputsError
from .input_source import InputSource


class InputEnumerator:
    """
    Turns the positional paths and --files-from list into the concrete set of files to
    process.

    Directories are walked recursively by default, the way fd and ripgrep behave;
    --max-depth and --no-recurse limit that. Directory depth is entirely separate from
    subfile depth, which controls recursion into documents nested inside other
    documents.

    -g/--glob and --regex match against a walked entry's path relative to the root it
    was found under, not the root-qualified path: an anchored pattern such as
    ^report-\\d{4}\\.pdf$ would otherwise never match anything once the walked root
    itself has more than one path segment. An explicit file argument is never checked
    against either — naming a file is an instruction, not a suggestion.
    """

    def __init__(self, file_system, console):
        self._file_system = file_system
        self._console = console

        # Directory identities already entered by the in-progress walk_directory call.
        #
        # Reset per root rather than per InputEnumerator instance: two different
        # explicit roots are free to share a directory, but a link cycle within one root
        # must not loop forever. Keyed by identity, not path, because a link to an
        # ancestor produces an endless supply of distinct path strings for what is
        # really one directory.
        self._visited = None

    def enumerate(self, options) -> List[InputSource]:
        """
        Produces the input list, in processing order. Selection by detected format
        (--include-types / --exclude-types) is not applied here, because it needs the
        engine to identify each file; that happens during execution.

        Raises NoInputsError when nothing matched.
        """
        raw_paths = list(options.paths)
        if options.files_from is not None:
            raw_paths.extend(self.read_file_list(options.files_from))
        if len(raw_paths) == 0:
            raise NoInputsError("no inputs given; name a file or a directory to process")

        results = []
        for path in raw_paths:
            if self._file_system.directory_exists(path):
                results.extend(self.walk_directory(path, options))
            elif self._file_system.file_exists(path):
                source = InputSource()
                source.path = path
                source.relative_path = _file_name_of(path)
                source.root = None
                results.append(source)
            else:
                raise NoInputsError("{0}: no such file or directory".format(path))

        if options.sort:
            # Python compares strings by code point, which is what string.CompareOrdinal
            # is for here. The two orders agree for every character in the basic
            # multilingual plane and disagree only for a name mixing an astral character
            # with one in U+E000-U+FFFF, because .NET compares UTF-16 code units and the
            # surrogate pair sorts lower there. Left alone rather than emulated: sorted()
            # is also stable where List.Sort is not, which makes this port the more
            # deterministic of the two.
            results.sort(key=lambda source: source.path)

        if len(results) == 0:
            raise NoInputsError("no inputs matched; the walk and filters selected nothing")

        return results

    def read_file_list(self, path: str) -> List[str]:
        """Reads a --files-from list. A path of "-" reads from stdin."""
        lines = self._console.read_all_input_lines() if path == "-" else self._file_system.read_all_lines(path)
        # is_all_whitespace rather than `not line.strip()`, which is string.IsNullOrWhiteSpace
        # as the reference spells it: a line holding only a C0 information separator is a
        # path there, not a blank line.
        return [line for line in lines if not is_all_whitespace(line)]

    def walk_directory(self, root: str, options) -> List[InputSource]:
        """Walks one directory tree, honouring depth, hidden entries and symlinks."""
        results = []
        self._visited = set()
        self._walk_into(root, root, 1, options, results)
        return results

    def _walk_into(self, root: str, current: str, depth: int, options, results: List[InputSource]):
        """
        The recursive worker behind walk_directory, accumulating into a caller-owned
        list.

        Accumulating rather than returning a list per level is deliberate: the
        portability rules rule out yield return, because Java has no clean equivalent
        and C++17 has none at all, and merging a list at every level of the recursion is
        four different idioms in four languages. Passing one list down ports unchanged.
        """
        identity = self._file_system.get_directory_identity(current)
        if identity in self._visited:
            return
        self._visited.add(identity)

        try:
            entries = self._file_system.list_directory(current)
        except CliError:
            raise
        except Exception:
            # A directory the walk cannot read is reported, not skipped. Skipping it
            # silently produces a shorter input list that looks like a complete one, and
            # letting the OSError escape ended the process with a traceback.
            #
            # The message is fixed rather than the platform's own: Python says
            # "[WinError 5] Access is denied" on Windows and "[Errno 13] Permission denied"
            # elsewhere, and error text is part of the cross-language contract. The C#
            # `catch ... when (!(error is CliError))` is the two clauses above, a Python
            # `except` being unable to decline a match.
            raise NoInputsError(current + ": could not be read")

        for entry in entries:
            if not options.include_hidden and self._file_system.is_hidden(entry):
                continue

            if self._file_system.directory_exists(entry):
                if self._file_system.is_symbolic_link(entry) and not options.follow_symlinks:
                    continue

                next_depth = depth + 1
                if not options.no_recurse_was_given and (options.max_depth is None or next_depth <= options.max_depth):
                    self._walk_into(root, entry, next_depth, options, results)
            else:
                relative_path = entry[len(root):].lstrip('/')
                if options.matcher is not None and not options.matcher.matches(relative_path):
                    continue

                source = InputSource()
                source.path = entry
                source.relative_path = relative_path
                source.root = root
                results.append(source)


def _file_name_of(path: str) -> str:
    """
    The filename portion of an explicitly named file argument.

    Unlike a walked entry's relative_path — already forward-slash because _walk_into
    built it from IFileSystem.list_directory output — an explicit argument is whatever
    the user typed on their own shell, backslashes included on Windows. OutputPlanner's
    directory_of normalises before splitting for exactly this reason; skipping it here
    left a bare C:\\tmp\\a.pdf unrecognised as having a directory at all, so its full
    path — not just its filename — became "the relative path", and --out-dir nested the
    source drive underneath itself instead of flattening to the filename.
    """
    normalised = path.replace('\\', '/')
    slash = normalised.rfind('/')
    return normalised if slash < 0 else normalised[slash + 1:]
