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

from ..configuration.command_options import FormatsOptions, OptionCatalogOptions
from ..configuration.value_parsers import fold_token
from ..engine.i_document_filters_engine import EngineFormat, EngineOption
from ..runtime.i_console import IConsole
from ..runtime.json_writer import JsonWriter

# The C# constructor takes a DocumentFiltersSession, and this module deliberately does not
# import it. Execution has one import cycle — extractor_open_settings calls
# canvas_conversion.image_suppression, canvas_conversion holds a DocumentFiltersSession, and
# DocumentFiltersSession.open_document takes an ExtractorOpenSettings — and the rule this
# package follows is to keep the cycle broken at the live-call edge (a deferred
# function-level import inside extractor_open_settings.for_format) and to add no new module
# edges for references that are only types. Here the session is exactly that: nothing is
# constructed, subclassed or isinstance-tested, only `session.api` is read, which Python
# resolves at call time without knowing the class. So the parameter is left unannotated, as
# engine_log.py leaves `console`. The practical consequence is that this module imports
# cleanly whether or not document_filters_session.py has been written yet.


class CatalogReports:
    """
    The two catalogue commands: formats and options. Both ask the engine what this build
    supports and print it; neither opens a document.
    """

    def __init__(self, session, console: IConsole):
        self._session = session
        self._console = console

    def write_formats(self, options: FormatsOptions) -> None:
        """
        formats — every readable file type, from DocumentFilters.GetSupportedFormats.

        There is no capability column here, unlike identify: capability flags
        (text/subfiles/hd) exist only on an opened IDocument (get_file_capabilities), and
        neither Format — the .NET binding's row type, EngineFormat at this seam — nor
        doc2text's own catalogue dump (print_file_types in main.cpp) exposes them for a
        format nothing has been opened for.
        """
        # The C# types this as IEnumerable<EngineFormat> and leaves LINQ's Where lazy. A
        # list instead: whichever branch below runs iterates it exactly once and
        # immediately, so deferring the filtering buys nothing, and the two branches would
        # otherwise share a one-shot enumerable, which is a trap for whoever next adds a
        # count or an "is empty" test to this method.
        #
        # format_row, not format: format is a Python builtin, and shadowing it inside a
        # method whose other branch calls str.format on every line would read badly for no
        # gain. subfile_extraction._insert_subfile_id settles the same question the same way.
        formats: List[EngineFormat] = [
            format_row for format_row in self._session.api.get_supported_formats()
            if _matches_filter(options.filter, format_row.display_name, format_row.short_name)]

        if options.json:
            json = JsonWriter().begin_array()
            for format_row in formats:
                # The C#'s fluent chain. Python needs the expression wrapped in parentheses
                # to span lines; the alternative is structure_report.py's trailing backslash,
                # and parentheses are preferred for a chain this long because a lost
                # backslash is a silent syntax change and a lost bracket is not. The call
                # order is the C#'s exactly, and JsonWriter emits keys in the order they are
                # written, so the object's shape is byte-identical across the ports.
                (json.begin_object()
                 .property("id", format_row.id)
                 .property("ext", format_row.short_name)
                 .property("name", format_row.display_name)
                 .end_object())
            self._console.write(json.end_array().to_string())
        else:
            for format_row in formats:
                self._console.write_line("id={0} ext={1} name={2}".format(
                    format_row.id, _text(format_row.short_name), _text(format_row.display_name)))

    def write_options(self, options: OptionCatalogOptions) -> None:
        """
        options — every engine tunable, from DocumentFilters.GetAvailableOptions. These are
        the keys accepted by --options.

        applies-to comes from EngineOption.flags — Option.Flags in the C#, naming the .NET
        binding's own row type: bit 0x1 means document, 0x2 means canvas, confirmed against
        doc2text's own --options --json dump (main.cpp, the appliesTo array built from
        f & 0x1 / f & 0x2).
        """
        matches: List['_OptionMatch'] = []
        for option in self._session.api.get_available_options():
            if not _matches_filter(options.filter, option.display_name, option.description):
                continue

            applies_to = _applies_to(option.flags)
            # The C# is appliesTo.Contains(options.AppliesTo, StringComparer.OrdinalIgnoreCase),
            # and that comparer needs no equivalent here: --applies-to arrives already folded
            # (argument_table.py binds it through value_parsers.fold_token) and the candidates
            # are the two lowercase ASCII literals _applies_to appends, so a plain `in` is an
            # exact match for what the comparer decides.
            if options.applies_to is not None and options.applies_to not in applies_to:
                continue

            matches.append(_OptionMatch(option, ",".join(applies_to) if applies_to else "none"))

        if options.json:
            json = JsonWriter().begin_array()
            for match in matches:
                (json.begin_object()
                 .property("name", match.option.display_name)
                 .property("type", match.option.type)
                 .property("default", match.option.default_value)
                 .property("appliesTo", match.applies_to)
                 .property("description", match.option.description)
                 .end_object())
            self._console.write(json.end_array().to_string())
        else:
            for match in matches:
                # One format call where the C# concatenates two interpolated strings. That
                # split is a source line-length artifact there, not a boundary in the output:
                # the five fields and the single spaces between them are the same either way.
                self._console.write_line(
                    "name={0} type={1} default={2} applies-to={3} description={4}".format(
                        _text(match.option.display_name), _text(match.option.type),
                        _text(match.option.default_value), match.applies_to,
                        _text(match.option.description)))


class _OptionMatch:
    """
    One row that survived both filters, with the applies-to text already rendered.

    The C# is a List<(EngineOption Option, string AppliesTo)> — a tuple, but a named one,
    read back as match.Option and match.AppliesTo. A small private class rather than a plain
    two-element tuple because that is what keeps those two names; match[0].display_name and
    match[1] would be the same data with the labels thrown away, and write_options' JSON
    branch interleaves the two fields, where a reader has to know which is which. Whereas
    metadata_report.py reaches for a plain tuple for the opposite reason: its pairs are a
    KeyValuePair in the C#, unnamed there too, and are consumed positionally.
    """

    def __init__(self, option: EngineOption, applies_to: str):
        self.option = option
        self.applies_to = applies_to


# _applies_to and _matches_filter are the C#'s two `private static` members, so module-level
# private functions here, as debug_input.py and structure_report.py already settle for this
# package. _text after them has no counterpart in the C#; its own docstring says why.


def _applies_to(flags: int) -> List[str]:
    """The scopes an option's flag word names, in the order doc2text lists them."""
    scopes: List[str] = []
    if (flags & 0x1) != 0:
        scopes.append("document")
    if (flags & 0x2) != 0:
        scopes.append("canvas")
    return scopes


def _matches_filter(filter_text: Optional[str], *fields: Optional[str]) -> bool:
    """
    Whether --filter's text appears in any of the fields offered, or no filter was given.

    filter_text, not filter, because filter is a Python builtin; the C# parameter is named
    filter. The C#'s `params string[] fields` is *fields, which is the same "any number of
    fields, positionally" shape, and both callers pass two.

    string.IsNullOrEmpty, spelled out as the two tests it is, and deliberately not
    is_all_whitespace: a filter of one space is a real filter in the reference and has to
    stay one here.

    The C# is field.IndexOf(filter, StringComparison.OrdinalIgnoreCase) >= 0. Both sides are
    folded with value_parsers.fold_token and then compared with `in`. fold_token is the
    closer model of OrdinalIgnoreCase than str.casefold() is, and for a substring search the
    difference is not academic: OrdinalIgnoreCase can never change a string's length, and
    neither can fold_token, whereas casefold maps ß to ss, so a one-character haystack would
    grow into two and a search for "ss" would report a match .NET cannot. It is not exact —
    OrdinalIgnoreCase folds the whole Unicode simple-case range and fold_token folds only
    A-Z — and that is the same deliberate, documented narrowing every other comparison in
    this port makes; see fold_token's own docstring for why ASCII-only is the point.

    That docstring also lists --filter among the values "used exactly as typed", and this
    does not contradict it: the value is still stored verbatim, byte for byte as the user
    wrote it, and would be echoed back that way. The folding happens only here, at the
    moment of comparison, which is exactly where the C# asks for it too.
    """
    if filter_text is None or len(filter_text) == 0:
        return True

    folded = fold_token(filter_text)
    for field in fields:
        if field is not None and folded in fold_token(field):
            return True

    return False


def _text(value: Optional[str]) -> str:
    """
    A field value as C# string interpolation would render it.

    Not in the C#, and it exists for one difference: C#'s $"ext={null}" writes an empty
    string, where "ext={0}".format(None) writes the four characters None. Every field the
    two line-oriented branches print is Optional[str] at the seam — a format's short and
    display name, an option's name, type, default and description — so a build whose engine
    leaves one unset produces the same line in all four ports.

    structure_report.py carries the same helper for the same reason. Duplicated rather than
    imported: it is private there, and four lines of the obvious is a smaller thing to own
    twice than a module edge into another report for one expression.

    The --json branches need none of this. JsonWriter.value writes null for None, which is
    what the C# JsonWriter does with a null string, so the two agree already.
    """
    return value if value is not None else ""
