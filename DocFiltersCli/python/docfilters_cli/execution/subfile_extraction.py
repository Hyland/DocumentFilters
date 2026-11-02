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

import datetime
from typing import Callable, List, Optional

from ..engine.i_document_filters_engine import IDocument, ISubFileEntry, OpenType
from ..planning import output_planner
from ..runtime.cli_error import ErrorCodes, OutputConflictError


class SubfileExtraction:
    """
    Walking into documents that contain other documents: archives, mail stores,
    attachments, embedded images.

    The depth limit here is --subfile-depth, which is entirely separate from the
    directory walk's --max-depth. A .docx inside a .zip inside an .msg is three subfile
    levels regardless of where the .msg sits on disk.
    """

    def __init__(self, file_system):
        """
        Takes the filesystem because extraction creates the directories a subfile's name
        implies.

        The same split as everywhere else a file is produced: the directory is ours to
        create, the file itself is written by ISubFileEntry.copy_to inside the engine.
        Creating it through IFileSystem rather than os.makedirs is what keeps the unit
        suite off the real disk — a subfile whose name carries a path, or --make-dirs,
        would otherwise leave directories behind wherever the tests happened to run.
        """
        self._file_system = file_system

        self._claimed = set()
        """
        Every output path this extractor has already resolved, for the whole run.

        Extraction resolves its own paths -- the planner never sees them, because what a
        container holds is not known until it is open -- so the output policy has to be
        applied here or not at all. It was not: an existing file was overwritten without
        --overwrite, and two subfiles whose names sanitise to one path overwrote each
        other, both silently and both exit 0.

        On the extractor rather than on the call, because extract builds one of these and
        runs every input through it. Per call, two archives each holding a readme.txt
        flattened into one output directory would not collide at all -- the set would have
        been emptied in between -- and under --overwrite the second would silently replace
        the first. That is what the planner's own collision check refuses for ordinary
        outputs, with or without --overwrite, so extraction refuses it too.
        """

    def extract_all(self, container: IDocument, container_path: str, shared, options) -> List[str]:
        """
        Extracts every subfile to disk, for the extract command.

        Every written path must be inside the output directory. A subfile's name
        comes from the container, not from the user, so an archive can carry a name like
        ../../outside.txt or an absolute path and walk out of the destination. Names are
        put through resolve_safe_output_path before anything is created; no path reaches
        the filesystem any other way.

        Returns a plain list where the C# returns IReadOnlyList<string>, which Python has
        no equivalent of — the engine seam hands back plain lists for the same reason.
        """
        written: List[str] = []
        output_root = shared.output.out_dir if shared.output.out_dir is not None else "."

        # A closure, as in the C#, over both written and output_root. No `nonlocal` is
        # needed for either: written is only ever appended to, never rebound, and
        # output_root is only read.
        #
        # display_name and depth go unused here exactly as they do in the C# local
        # function — the signature is the walk callback's, not this function's choice.
        def extract_one(subfile: ISubFileEntry, display_name: str, depth: int) -> None:
            if not SubfileExtraction.passes_subfile_filters(subfile, options):
                return

            output_path = SubfileExtraction.resolve_safe_output_path(output_root, subfile.name,
                                                                    options.replicate_tree)
            if options.append_subfile_id:
                output_path = SubfileExtraction._insert_subfile_id(output_path, subfile.id)

            if not self._claim(output_path):
                raise OutputConflictError(ErrorCodes.OUTPUT_COLLISION,
                                          "two subfiles would be extracted to the same output; use --sub-id to distinguish them")

            output_planner.verify_page_target(output_path, shared, self._file_system)
            self._verify_no_linked_component(output_root, output_path)

            # C#'s string.IsNullOrEmpty(directory). _directory_of returns "" and never
            # None, so the one truth test covers both halves of it.
            directory = _directory_of(output_path)
            if directory:
                self._file_system.create_directory(directory)

            # The entry's bytes are copied by the engine, which opens the destination in
            # binary mode. Never re-implemented here as a read-and-write: a text-mode
            # write would translate every \n in a subfile to \r\n on Windows and corrupt
            # every extracted file that is not text.
            subfile.copy_to(output_path)
            written.append(output_path)

        self.walk(container, container_path, 1, shared, extract_one)

        if options.images:
            for image in container.images:
                with image:
                    extract_one(image, SubfileExtraction.compose_display_name(container_path, image.name), 1)

        return written

    def _claim(self, output_path: str) -> bool:
        """
        HashSet.Add: records output_path and says whether it was new. Python's set.add
        returns None whatever happened, so the membership test is spelled out.
        """
        key = output_planner.collision_key_for(output_path, self._file_system)
        if key in self._claimed:
            return False

        self._claimed.add(key)
        return True

    def _verify_no_linked_component(self, output_root: str, output_path: str) -> None:
        """
        Refuses an output path that reaches its destination through a symbolic link.

        resolve_safe_output_path guarantees the path spells out as a plain name under the
        root, which is a statement about the string. Where it lands is a statement about
        the disk: if out/latest is a link to a web root, a container entry named
        latest/index.html is lexically contained and still overwrites a file outside the
        output directory entirely. The container chooses that name, so the choice is not
        the user's.

        Only components below the root are examined. The root itself may well be a link --
        --out-dir is the user's own instruction, and refusing it would break ordinary
        setups -- and so may anything above it.

        This closes the hole rather than sealing it: a link created between this check and
        copy_to would still be followed. Winning that race needs write access to the output
        directory while the run is in progress, at which point the file can simply be
        replaced afterwards.
        """
        root = output_root.replace('\\', '/').rstrip('/')
        path = output_path.replace('\\', '/')

        if not path.startswith(root + "/"):
            return

        # The file itself is a component like any other: a link named report.txt is
        # written through just as a link named reports/ is.
        cursor = len(root)
        while cursor < len(path):
            slash = path.find('/', cursor + 1)
            component = path if slash < 0 else path[:slash]

            present = self._file_system.directory_exists(component) or self._file_system.file_exists(component)
            if present and self._file_system.is_symbolic_link(component):
                raise OutputConflictError(ErrorCodes.UNSAFE_OUTPUT,
                                          "a subfile would be written through '" + component
                                          + "', which is a link out of the output directory")

            if slash < 0:
                break

            cursor = slash

    @staticmethod
    def _insert_subfile_id(path: str, subfile_id: str) -> str:
        """
        Inserts a subfile's id before its extension, for --sub-id.

        The C# names the parameter id; spelled subfile_id here because id is a Python
        builtin, and shadowing it inside the function that also has to read a path apart
        would be a trap for no gain.
        """
        slash = path.rfind('/')
        directory = "" if slash < 0 else path[:slash + 1]
        file_name = path if slash < 0 else path[slash + 1:]

        dot = file_name.rfind('.')
        stem = file_name if dot < 0 else file_name[:dot]
        extension = "" if dot < 0 else file_name[dot:]

        return directory + stem + "-" + subfile_id + extension

    @staticmethod
    def resolve_safe_output_path(output_root: str, subfile_name: str, replicate_tree: bool) -> str:
        """
        Turns a container-supplied subfile name into a path that is guaranteed to sit
        inside output_root.

        The name is untrusted input. Traversal segments, absolute paths and drive letters
        are all neutralised; when replicate_tree is false the name collapses to its final
        component.

        This is the lexical half of the guarantee and all of it that is knowable without
        touching the disk: every segment it returns is a plain name under the root. The
        other half is _verify_no_linked_component, because a name can be lexically
        contained and still land outside -- a symlink already sitting in the output
        directory resolves wherever it points.

        The published ExtractSubfiles sample does the same thing; a tool that extracts
        archives has to.

        The C# trims the root with TrimEnd('/'), which is a character-set trim and not a
        whitespace trim, so it ports to str.rstrip('/') and specifically not to
        value_parsers.trim: a root that ends in a space is the user's own --out-dir text
        and is left exactly as given.
        """
        segments = SubfileExtraction._safe_segments(subfile_name, replicate_tree)
        return output_root.rstrip('/') + "/" + "/".join(segments)

    @staticmethod
    def _safe_segments(subfile_name: Optional[str], replicate_tree: bool) -> List[str]:
        """
        Splits a subfile name into path components that are guaranteed not to traverse
        upward or escape via a drive letter or UNC share, never leaving the result empty.

        Every "." and ".." component is dropped outright, not merely neutralised, so a
        name that is nothing but traversal ("../..") cannot leave the result empty either
        — that case falls back to a single placeholder segment. Removing rather than
        clamping is also what tells "..data", a legitimate filename, apart from an actual
        traversal component: only an exact match is dropped, never a prefix or substring.
        """
        # str.replace rather than os.path or pathlib, which would introduce the
        # platform's separator and make an extracted layout differ between Windows and
        # Linux. The name is container text, not a path on this machine, and the result
        # is forward-slash by construction.
        normalised = (subfile_name if subfile_name is not None else "").replace('\\', '/')

        # A drive letter ("C:...") names something outside any output directory;
        # dropping just the colon is enough, because the leading "C" and the rest
        # of the path pass through the same segment filter as everything else.
        if len(normalised) >= 2 and normalised[1] == ':':
            normalised = normalised[2:]

        segments = []
        for segment in normalised.split('/'):
            if len(segment) == 0 or segment == "." or segment == "..":
                continue

            segments.append(segment)

        if not replicate_tree:
            # C#'s segments[^1]; Python spells the same index -1.
            last = segments[-1] if len(segments) > 0 else None
            segments = []
            if last is not None:
                segments.append(last)

        if len(segments) == 0:
            segments.append("_")

        return segments

    def list_all(self, container: IDocument, container_path: str, shared) -> List['SubfileEntry']:
        """Lists subfiles without extracting, for the list command."""
        entries: List['SubfileEntry'] = []

        def collect(subfile: ISubFileEntry, display_name: str, depth: int) -> None:
            # The C# uses an object initialiser, which Python has no equivalent of; the
            # fields are assigned one by one, as the engine seam's own value types are.
            entry = SubfileEntry()
            entry.id = subfile.id
            entry.name = subfile.name
            entry.display_name = display_name
            entry.size = subfile.size
            entry.modified = SubfileExtraction.format_timestamp(subfile.modified)
            entry.depth = depth
            entries.append(entry)

        self.walk(container, container_path, 1, shared, collect)
        return entries

    def walk(self, container: IDocument, container_path: str, depth: int, shared,
             action: Callable[[ISubFileEntry, str, int], None]) -> None:
        """
        Invokes action for every subfile, depth-first, honouring --subfile-depth. Shared
        by convert, extract, list, identify and meta when -s is given.

        container must already be open — recursion opens each subfile itself, the same
        way the published ExtractSubfiles sample does, since an entry is a document in
        its own right and its own sub_files are unavailable until it is opened.

        action is C#'s Action<ISubFileEntry, string, int>, which ports to a
        Callable[..., None]; a plain function or a bound method satisfies it either way.
        """
        for subfile in container.sub_files:
            # C#'s `using (subfile)`.
            with subfile:
                display_name = SubfileExtraction.compose_display_name(container_path, subfile.name)
                action(subfile, display_name, depth)

                can_recurse = (shared.subfiles.enabled
                               and (shared.subfiles.max_depth is None or depth < shared.subfiles.max_depth))

                if can_recurse:
                    # Opening the entry yields a document sharing its handle, so the
                    # entry's own with-block is what closes it. Deliberately not wrapped
                    # in a `with` of its own: closing it would close the entry too, and
                    # the walk below still needs the handle.
                    #
                    # The C# writes (int)OpenType.BodyOnly, reaching OpenType through
                    # the .NET binding's namespace; here it comes from the engine seam,
                    # which declares it because the Python binding publishes these only
                    # as bare IGR_* constants. No cast: an IntEnum member already is an
                    # int, where a C# enum is not.
                    opened = subfile.open(OpenType.BODY_ONLY, "")
                    self.walk(opened, display_name, depth + 1, shared, action)

    @staticmethod
    def format_timestamp(modified: Optional[datetime.datetime]) -> Optional[str]:
        """
        Renders a subfile timestamp in the ISO 8601 form the console contract uses, or
        None when the container records none.

        Kept as a pure function of the nullable value so the boundary — a container with
        no timestamp versus one dated at the epoch — is testable without an archive.

        C#'s DateTime? is Optional[datetime.datetime], and its DateTime.MinValue — what
        the binding gives for an entry with no date — is datetime.datetime.min, whose
        year is 1 and so fails the same test. That order matters: the year check has to
        stay behind the None check, because Python would raise on modified.year rather
        than report anything useful.
        """
        if modified is None or modified.year <= 1970:
            return None

        # The C# format string is "yyyy-MM-ddTHH:mm:ss" with the literal T and the Z
        # appended after. strftime spells the same four fields %Y-%m-%d and %H:%M:%S, and
        # the "Z" is appended the same way rather than folded into the format string, so
        # the two files read alike. No fractional seconds here, so engine_log's %f
        # truncation has no counterpart.
        return modified.strftime('%Y-%m-%dT%H:%M:%S') + "Z"

    @staticmethod
    def compose_display_name(container_path: str, subfile_name: str) -> str:
        """Builds the display name for a subfile: container.zip>inner.docx."""
        return container_path + ">" + subfile_name

    @staticmethod
    def passes_subfile_filters(subfile: ISubFileEntry, options) -> bool:
        """
        Applies extract's --include and --exclude globs to a subfile's name.

        Both are already-compiled Planning.NameMatcher instances; a malformed pattern
        fails when the argument is read, not here with an archive half-extracted.

        Matching on the name lets a subfile be rejected without being opened, which
        matters for a large archive.
        """
        if options.subfile_include is not None and not options.subfile_include.matches(subfile.name):
            return False

        if options.subfile_exclude is not None and options.subfile_exclude.matches(subfile.name):
            return False

        return True


class SubfileEntry:
    """One subfile, as reported by list."""

    def __init__(self):
        self.id: Optional[str] = None

        self.name: Optional[str] = None

        self.display_name: Optional[str] = None
        """Full display path including containers."""

        self.size = 0

        self.modified: Optional[str] = None
        """Modification time in ISO 8601, or None when the container does not record one."""

        self.depth = 0
        """Nesting level; 1 for a direct child of the document named on the command line."""


def _directory_of(path: str) -> str:
    """
    The directory portion of a resolved output path, standing in for the C#'s
    Path.GetDirectoryName.

    A module-level helper rather than a method, because it replaces a framework call
    rather than expressing anything about extraction; planning/output_planner.py carries
    its own copy of the same three lines for the same reason.

    String work rather than os.path.dirname or pathlib, which on Windows would hand back
    a backslash-separated answer for a path this file built with forward slashes — and
    the whole point of resolve_safe_output_path is that the text it produces is the text
    that reaches the filesystem.

    Two harmless differences from Path.GetDirectoryName, both only in what is trimmed:
    it strips any run of trailing separators ("C:/out//x" gives "C:/out", this gives
    "C:/out/"), and for a path whose only separator is the leading one it answers the
    root ("/x" gives "/") where this answers empty and the caller then creates nothing.
    In the first case the two name the same directory; in the second the root already
    exists. Neither can change which file is written, because the filename itself is
    never touched.
    """
    slash = path.rfind('/')
    if slash < 0:
        return ""
    return path[:slash]
