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

from enum import Enum
from typing import List, Optional


class OutputCardinality(Enum):
    """
    Who owns the output target.

    This is one axis only: whether each input writes its own output or they all share
    one. How many files a single input produces is a separate question, answered by
    FormatSpec.is_single_file — a .docx converted to PNG writes one file per page but is
    still PER_INPUT. Keeping the two apart is what stops -o from ever depending on --to.
    """

    PER_INPUT = 0
    """Each input owns its output, inside --out-dir. Printed as per-input."""

    COMBINED = 1
    """Every input merges into one document: --combine --out-file. Printed as combined."""

    SINGLE_TARGET = 2
    """A single input to a single --out-file. Printed as single-target."""

    STREAM = 3
    """A single-file format streamed to stdout. Printed as stream."""


class RunPlan:
    """
    The complete description of a run, produced before anything is opened or written.

    Holding the whole run as a value is what makes --print-plan possible, and it is the
    only way to detect two inputs resolving to the same output path before the first one
    has already been written.
    """

    def __init__(self):
        self.cardinality = OutputCardinality.PER_INPUT

        self.items: List['PlannedItem'] = []
        """One entry per input, in processing order."""

        self.combined_target: Optional[str] = None
        """
        The shared target when cardinality is COMBINED; otherwise None.

        A run-level fact rather than a PlannedItem, because items is keyed by input: one
        entry per document, in every cardinality and on every command. That is the unit
        execution works in, the unit --on-error counts, and the unit a combined run still
        needs — a document that fails mid-combine is skipped while the rest are still
        written, so the run has to be able to fail one input and keep going.

        Keying items by output instead would collapse a combined run into a single entry
        and lose that granularity.
        """

        self.format = None
        """The format every item renders to."""

    def to_canonical_string(self) -> str:
        """
        Renders the plan in the canonical form the conformance corpus compares. All four
        language implementations must produce byte-identical output.

        cardinality always appears; target/pattern lines do not, because extract, list
        and meta produce names that only exist once a document is open and so stop at
        cardinality. For COMBINED the one target is combined_target rather than a
        per-item line, since every item feeds that single output. For STREAM no item owns
        a path at all.
        """
        lines = ["cardinality " + RunPlan.cardinality_text(self.cardinality)]
        if self.cardinality == OutputCardinality.COMBINED:
            lines.append("target " + self.combined_target)
        else:
            for item in self.items:
                if item.target is not None:
                    lines.append("target " + item.target)
                elif item.target_pattern is not None:
                    lines.append("pattern " + _pattern_text(item.target_pattern))
        return "\n".join(lines)

    @staticmethod
    def cardinality_text(cardinality: OutputCardinality) -> str:
        """The canonical text for one cardinality, shared by every command that states one."""
        if cardinality == OutputCardinality.PER_INPUT:
            return "per-input"
        if cardinality == OutputCardinality.COMBINED:
            return "combined"
        if cardinality == OutputCardinality.SINGLE_TARGET:
            return "single-target"
        if cardinality == OutputCardinality.STREAM:
            return "stream"
        raise ValueError("cardinality")


def _pattern_text(pattern: 'TargetPattern') -> str:
    return str(pattern.template) if not pattern.directory else pattern.directory + "/" + str(pattern.template)


class PlannedItem:
    """
    One input and everything decided about it.

    Exactly one of target and target_pattern is set — or neither, when the item
    contributes to RunPlan.combined_target.

    Every per-page output is deferred, including when --pages was given. An explicit
    range does not make the filenames knowable: nothing at this stage has opened the
    document, so whether every requested page actually exists is still unknown —
    --pages 1-2 against a document that turns out to have only one page is discovered
    during execution, once the real page count can be trusted, not guessed at here.
    Resolving both up front would also let the planner reject a run because a page-2
    output already exists when page 2 was never going to be written.

    Execution is the only point at which the pages that actually exist are known, so
    that is where the names are produced.
    """

    def __init__(self):
        self.source_path: Optional[str] = None
        """The path as it will be opened."""

        self.display_name: Optional[str] = None
        """How the input is named in messages; for a subfile this includes the container."""

        self.target: Optional[str] = None
        """
        The one output path, for a single-file format. None for a per-page format, and
        None when this item feeds RunPlan.combined_target.
        """

        self.target_pattern: Optional['TargetPattern'] = None
        """The unresolved output naming, for a per-page format. None when target is set."""

        self.pages = None
        """The pages to render, or PageRange.ALL."""

    @property
    def is_page_count_dependent(self) -> bool:
        """True when this item's output names are produced during execution."""
        return self.target_pattern is not None

    @property
    def output_directory(self) -> Optional[str]:
        """
        The directory this item's output lands in, or None when it has no file target —
        --stdout, or an item feeding RunPlan.combined_target.

        The plan already knows this for both target shapes, which is what lets the
        directory be created once per item instead of at each of the three places that
        open an output. That matters because only one of those three is ours: the text
        path writes through IFileSystem, but a canvas and a docpack are opened by the
        engine, which will not create a directory and reports a missing one as a
        per-document FileStream::Create failure that reads like the input was missing.
        """
        if self.target_pattern is not None:
            return self.target_pattern.directory
        if not self.target:
            return None
        return _directory_name(self.target)


def _directory_name(path: str) -> str:
    """
    The directory portion of a target path, or empty when it has none.

    Path.GetDirectoryName in the C#, which is platform-dependent: on Windows it also
    splits on a backslash and on a drive-relative root, on Linux it does not. Every
    target the planner builds is forward-slash by construction, so the two agree there;
    they disagree only on a raw --out-file such as C:\\tmp\\a.pdf. Normalising first
    picks one answer for all four ports rather than inheriting the host's.
    """
    normalised = path.replace('\\', '/')
    slash = normalised.rfind('/')
    return "" if slash < 0 else normalised[:slash]


class TargetPattern:
    """
    A directory plus a filename template, for outputs whose count depends on the
    document's page count.
    """

    def __init__(self):
        self.directory: Optional[str] = None
        """The directory the files land in, already including any replicated source tree."""

        self.template = None
        """The template applied per page."""

        self.extension: Optional[str] = None
        """The output format's extension, for {ext}."""

    def resolve(self, source_path: str, page_number: int) -> str:
        """Produces the concrete path for one page, during execution."""
        file_name = self.template.render(source_path, self.extension, page_number)
        return file_name if not self.directory else self.directory + "/" + file_name

    def collision_key(self, source_path: str) -> str:
        """
        The part of the path that does not vary with the page number.

        Two inputs whose keys match collide on every page, so this catches the case that
        actually bites — a flattened walk containing two files with the same stem —
        without knowing either page count.
        """
        # Any fixed page number identifies the non-page part of the name: if two items
        # differ only by page, they render identically here and correctly collide; if
        # they differ elsewhere, they differ here too.
        return self.resolve(source_path, 0)

    def first_target(self, source_path: str, pages) -> str:
        """
        The path of the first page that would be written, for the fail-fast check before
        any work starts.

        The first selected page, not page 1: with --pages 5-6 the first output is page 5,
        read straight off the explicit list — no document is open yet at this point, so
        there is no page count to check it against. If the document turns out to have
        fewer pages than that, nothing is written and this check was moot — a rare false
        alarm that costs nothing, because execution re-checks every page before writing
        it. This is a fail-fast convenience for the common case of re-running into a
        directory that already holds output, not the guarantee.
        """
        first_page = pages.first_page
        return self.resolve(source_path, first_page)
