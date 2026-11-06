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

import re
from enum import Enum
from typing import List, Optional

from ..runtime.cli_error import ErrorCodes, UsageError

# \Z, not $: Python's $ also matches before a trailing newline, which would make
# {page:4\n} a width of 4 here and a malformed template in the other three ports. \Z is
# the absolute end of the string, the same anchor NameMatcher's translated globs use.
_DIGITS = re.compile(r'^[0-9]+\Z')

# A width the C# int could not hold is a malformed width rather than a huge one, so the
# bound has to be stated here: Python integers have no range of their own.
_INT_MAX = 2147483647

# The characters the scanner stops on. '/' and '\\' are in the set so that a separator in
# the template's own literal text is rejected, not copied into the rendered name.
_SPECIAL_CHARACTERS = '{}/\\'


def _index_of_any(text: str, start_index: int) -> int:
    """The first index at or after start_index holding one of the special characters, or -1."""
    for index in range(start_index, len(text)):
        if text[index] in _SPECIAL_CHARACTERS:
            return index

    return -1


def _parse_width(text: str) -> Optional[int]:
    """
    The zero-padding width from a {page:NN} placeholder, or None when it is malformed.

    Digits and nothing else: no sign, no surrounding whitespace, no group separator, and
    nothing past the range of the C# int this is ported from. int() is not that check --
    it accepts " +4 " and "0_4" and has no upper bound -- and str.isdigit() accepts
    non-ASCII digits such as the Arabic-Indic four, which would make {page:٤} a valid
    template here and a malformed one in the other three ports.
    """
    if _DIGITS.match(text) is None:
        return None

    value = int(text)
    return None if value > _INT_MAX else value


def _normalize_separators(path: str) -> str:
    """
    The path with every backslash turned into a forward slash.

    The C# normalises here because Path.GetFileNameWithoutExtension and
    Path.GetExtension only recognise the separators the running platform does --
    backslash is not one of them on Linux -- but a source path is not guaranteed to
    match the platform running this tool: a walk finds real paths in the platform's own
    convention, but an explicit --files-from list or a subfile name from a container
    built elsewhere can carry either. Without this, {stem} on ..\\..\\windows\\system32\\config
    returned the whole string unchanged on Linux, backslashes included, which is exactly
    the path separator this template exists to keep out of the rendered name. Forward
    slash alone is unambiguous on every platform this tool targets, so normalising onto
    it is safe in both directions.
    """
    return path.replace('\\', '/')


def _file_name(path: str) -> str:
    """Everything after the last forward slash, as Path.GetFileName reads a normalised path."""
    slash = path.rfind('/')
    return path if slash < 0 else path[slash + 1:]


def _file_name_without_extension(path: str) -> str:
    """
    The filename up to its last dot, as Path.GetFileNameWithoutExtension.

    Spelled out rather than delegated to os.path.splitext or PurePath.stem, which are
    close but not the same: both keep a leading dot as part of the name, so ".gitignore"
    has the whole string as its stem there and an empty stem in the C#. They also emit
    and accept the platform's own separator, which is the thing _normalize_separators has
    just taken out.
    """
    file_name = _file_name(path)
    last_period = file_name.rfind('.')

    return file_name if last_period < 0 else file_name[:last_period]


def _extension(path: str) -> str:
    """
    The last dot and everything after it, as Path.GetExtension: empty when the filename
    has no dot at all, and empty when it ends in one.
    """
    for index in range(len(path) - 1, -1, -1):
        character = path[index]

        if character == '.':
            return '' if index == len(path) - 1 else path[index:]

        if character == '/':
            break

    return ''


def _throw(input_text: str):
    raise UsageError(ErrorCodes.BAD_TEMPLATE, "--out-name does not accept the value '{0}'".format(input_text))


class _Segment:
    """One compiled piece of a template: literal text, or a placeholder to render."""

    class Type(Enum):
        LITERAL = 0
        STEM = 1
        PAGE = 2
        SRCEXT = 3
        EXT = 4

    def __init__(self, segment_type: 'Type', text: Optional[str] = None):
        self.segment_type = segment_type
        self.literal_text = text
        self.width = 0


class OutputNameTemplate:
    """
    An --out-name filename template, such as {stem}-{page:04}.{ext}.

    Placeholders:
        {stem}    input filename without its extension
        {page}    one-based page number; {page:04} zero-pads to four digits
        {srcext}  the input's original extension, without a dot
        {ext}     the output format's extension, without a dot

    The template is compiled when the argument is parsed, not on first use, so a typo
    is reported before any work starts.
    """

    def __init__(self):
        self._original_text: Optional[str] = None
        self._segments: List[_Segment] = []

    @staticmethod
    def default(is_single_file: bool) -> 'OutputNameTemplate':
        """
        The template used when --out-name is not given: {stem}.{ext} for single-file
        formats, {stem}-{page:04}.{ext} for one-per-page formats.
        """
        if is_single_file:
            return OutputNameTemplate.parse("{stem}.{ext}")
        else:
            return OutputNameTemplate.parse("{stem}-{page:04}.{ext}")

    @staticmethod
    def parse(text: str) -> 'OutputNameTemplate':
        """
        Compiles a template.

        The template names a file *inside* --out-dir, so path separators and ".." are
        rejected here rather than sanitised at render time. Checking only the rendered
        output would miss them, because they can come from the template's own literal
        text as easily as from the input path.

        Raises:
            UsageError: An unknown placeholder, an unclosed brace, a malformed width
                such as {page:xx}, or a path separator or ".." anywhere in the template.
        """
        result = OutputNameTemplate()
        result._original_text = text

        start_index = 0
        while start_index < len(text):
            end_index = _index_of_any(text, start_index)  # only opening brace supported at this point
            if end_index == -1:
                result._segments.append(_Segment(_Segment.Type.LITERAL, text[start_index:]))
                break

            if text[end_index] != '{':
                _throw(text)  # invalid character

            if end_index > start_index:
                substr = text[start_index:end_index]
                result._segments.append(_Segment(_Segment.Type.LITERAL, substr))

            start_index = end_index
            end_index = text.find('}', start_index + 1)
            if end_index == -1:
                _throw(text)  # no closing brace

            # The name between the braces, e.g. "stem" or "page:04".
            placeholder = text[start_index + 1:end_index]

            if placeholder == "stem":
                result._segments.append(_Segment(_Segment.Type.STEM))
            elif placeholder == "page":
                result._segments.append(_Segment(_Segment.Type.PAGE))
            elif placeholder == "srcext":
                result._segments.append(_Segment(_Segment.Type.SRCEXT))
            elif placeholder == "ext":
                result._segments.append(_Segment(_Segment.Type.EXT))
            elif placeholder.startswith("page:"):
                width_text = placeholder[len("page:"):]
                width = _parse_width(width_text)
                if width is None or width <= 0:
                    _throw(text)  # malformed width

                segment = _Segment(_Segment.Type.PAGE)
                segment.width = width
                result._segments.append(segment)
            else:
                _throw(text)  # unknown placeholder

            start_index = end_index + 1

        return result

    @property
    def uses_page(self) -> bool:
        """True when the template refers to {page}, which a single-file format cannot satisfy."""
        return any(segment.segment_type is _Segment.Type.PAGE for segment in self._segments)

    def render(self, source_path: str, output_extension: str, page_number: int) -> str:
        """Produces a filename. page_number is ignored when the template has no {page}."""
        parts = []

        for segment in self._segments:
            if segment.segment_type is _Segment.Type.LITERAL:
                parts.append(segment.literal_text)
            elif segment.segment_type is _Segment.Type.SRCEXT:
                parts.append(_extension(_normalize_separators(source_path)).lstrip('.'))
            elif segment.segment_type is _Segment.Type.EXT:
                parts.append(output_extension)
            elif segment.segment_type is _Segment.Type.PAGE:
                # rjust, not zfill: padding is a minimum width applied to the number as
                # written, and zfill would put the zeros after a sign rather than before
                # it, which is not what the C# PadLeft does.
                page_str = str(page_number)
                if segment.width > 0:
                    page_str = page_str.rjust(segment.width, '0')
                parts.append(page_str)
            elif segment.segment_type is _Segment.Type.STEM:
                parts.append(_file_name_without_extension(_normalize_separators(source_path)))

        return ''.join(parts)

    def __str__(self) -> str:
        """The original template text."""
        return self._original_text
