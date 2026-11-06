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


class StructureReportOptions:
    """
    The document and page structure aspects convert can report while it works, emitted
    during a conversion run rather than by a separate command.
    """

    def __init__(self):
        self.fonts = False
        self.font_mappings = False
        self.page_fonts = False
        self.page_font_mappings = False
        self.dimensions = False
        self.page_elements = False
        self.form_elements = False
        self.hyperlinks = False
        self.bookmarks = False
        self.annotations = False

    @property
    def any(self) -> bool:
        """True when any report was requested."""
        return (self.fonts or self.font_mappings or self.page_fonts or self.page_font_mappings
                or self.dimensions or self.page_elements or self.form_elements
                or self.hyperlinks or self.bookmarks or self.annotations)

    @property
    def requested(self) -> List[str]:
        """
        The report flags asked for, spelled as the user typed them, in a fixed order.

        So that a message naming one report names the same one in all four
        implementations: --fonts --bookmarks and --bookmarks --fonts are the same
        request and must produce the same error text. Hand-written beside the fields, in
        the same idiom as ErrorCodes.all_codes and for the same reason — C++17 has no
        reflection.
        """
        requested = []
        if self.fonts:
            requested.append("--fonts")
        if self.font_mappings:
            requested.append("--font-mappings")
        if self.page_fonts:
            requested.append("--page-fonts")
        if self.page_font_mappings:
            requested.append("--page-font-mappings")
        if self.dimensions:
            requested.append("--dimensions")
        if self.page_elements:
            requested.append("--page-elements")
        if self.form_elements:
            requested.append("--form-elements")
        if self.hyperlinks:
            requested.append("--hyperlinks")
        if self.bookmarks:
            requested.append("--bookmarks")
        if self.annotations:
            requested.append("--annotations")
        return requested


class ConvertOptions:
    """Options that only convert accepts."""

    def __init__(self):
        self.format = None
        """--to: the resolved FormatSpec. Required."""

        self.combine = False
        """--combine: merge every input into one output document."""

        self.pages = None
        """--pages, as a PageRange. None means every page."""

        self.canvas_options: Optional[str] = None
        """--canvas-options, passed to the engine untouched."""

        self.render_options: Optional[str] = None
        """--render-options, passed to the engine untouched."""

        self.no_images = False
        """--no-images."""

        self.include_control_codes = False
        """--include-control-codes: keep ISYS control codes in text output."""

        self.fingerprint = False
        """--fingerprint."""

        self.watermark: Optional[str] = None
        """--watermark: a file path or a literal string."""

        self.redactions: List['RedactSpec'] = []
        """--redact, repeatable."""

        self.form_values: List['FormValue'] = []
        """--form, repeatable."""

        self.view = False
        """--view: open the result when done. Best effort, platform specific."""

        self.reports = StructureReportOptions()
        """The structure reports emitted during conversion."""



class ExtractOptions:
    """Options that only extract accepts."""

    def __init__(self):
        self.images = False
        """-i, --images: pull embedded images too, not just subfiles."""

        # Globs, the same pattern language as -g/--glob — and repeatable for the same
        # reason, so that one glob per idea reads better than one glob doing several
        # jobs.
        #
        # A different axis from -g/--glob. That one chooses which files on disk to
        # process; this one chooses which subfiles inside a container to extract — the
        # same distinction --max-depth and --subfile-depth draw between outside the
        # document and inside it.
        self.include_patterns: List[str] = []
        """--include, repeatable: keep subfiles whose name matches."""

        self.exclude_patterns: List[str] = []
        """--exclude, repeatable: drop subfiles whose name matches."""

        # Built by OptionsValidator, so a malformed pattern is reported before any
        # archive is opened rather than with one half-extracted.
        self.subfile_include = None
        """include_patterns compiled into a NameMatcher, or None when none were given."""

        self.subfile_exclude = None
        """exclude_patterns compiled into a NameMatcher, or None when none were given."""

        # Recorded separately from flat_was_given for the same reason --max-depth and
        # --no-recurse are kept apart: collapsing two opposing switches into one boolean
        # would make --make-dirs --flat indistinguishable from --flat alone, so the
        # contradiction could not be reported.
        self.make_dirs_was_given = False
        """--make-dirs: replicate the source tree under the output directory."""

        self.flat_was_given = False
        """--flat: put every extracted file straight into the output directory."""

        self.append_subfile_id = False
        """--sub-id: append the subfile id to extracted filenames."""

    @property
    def replicate_tree(self) -> bool:
        """
        Whether the source tree is replicated. Defaults to true, so extract lays out its
        output the way convert does.
        """
        return not self.flat_was_given


class ListOptions:
    """Options that only list accepts."""

    def __init__(self):
        self.json = False


class IdentifyOptions:
    """Options that only identify accepts."""

    def __init__(self):
        self.json = False

        self.capabilities = False
        """--capabilities: also print the format's capability flags."""


class MetaOptions:
    """Options that only meta accepts."""

    def __init__(self):
        self.json = False


class FormatsOptions:
    """Options that only formats accepts."""

    def __init__(self):
        self.json = False

        self.filter: Optional[str] = None
        """--filter: only formats whose id or name matches."""


class OptionCatalogOptions:
    """Options that only the options command accepts."""

    def __init__(self):
        self.json = False

        self.filter: Optional[str] = None
        """--filter: only options whose name or description matches."""

        self.applies_to: Optional[str] = None
        """--applies-to: document, page, canvas, render, and so on."""


class RedactSpec:
    """One --redact page,x,y,w,h region."""

    def __init__(self):
        self.page = 0
        self.x = 0
        self.y = 0
        self.width = 0
        self.height = 0


class FormValue:
    """One --form page,name=value assignment."""

    def __init__(self):
        self.page = 0
        self.name: Optional[str] = None
        self.value: Optional[str] = None
