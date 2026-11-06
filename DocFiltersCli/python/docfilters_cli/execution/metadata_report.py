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

from typing import List, Tuple

from ..configuration.value_parsers import trim
from ..engine.i_document_filters_engine import IDocument
from ..runtime.i_console import IConsole
from ..runtime.json_writer import JsonWriter
from .text_conversion import TextConversion


class MetadataReport:
    """
    The meta command's output.

    Console form is one key: value per line, no padding or boxes, so that the four
    language implementations can agree without anyone matching column widths. JSON form
    is a flat object with keys in the order the engine reports them.
    """

    def write_text(self, document: IDocument, display_name: str, console: IConsole) -> None:
        """Writes the plain-text form."""
        for key, value in _read_fields(document):
            console.write_line("{0}: field={1} value={2}".format(display_name, key, value))

    def write_json(self, document: IDocument, display_name: str, json: JsonWriter) -> None:
        """
        Writes one document's JSON object into an open writer.

        The parameter is named json, as the C#'s is, and shadows nothing here: this module
        never imports the standard library's json — see JsonWriter's own docstring for why
        the contract is written by hand in all four ports.
        """
        json.begin_object().property("path", display_name).name("metadata").begin_object()
        for key, value in _read_fields(document):
            json.property(key, value)
        json.end_object().end_object()


def _read_fields(document: IDocument) -> List[Tuple[str, str]]:
    """
    Reads the document's metadata fields, in the order the engine reports them.

    There is no structured metadata API: the document must be opened with
    extractor_open_settings.for_metadata_only, and get_text then returns the fields as
    plain text, one "Label: value" line per field — confirmed against a real
    isys_doc2text.exe --meta run, e.g. Title: _.pdf, Author: slop001,
    xmp:dc:creator: slop001. Splitting on the first ": " rather than the first ':' is what
    keeps a label such as xmp:dc:creator, which contains colons of its own, from being cut
    apart — none of its internal colons are followed by a space, only the one separating
    the label from the value is.

    The C#'s `private static` member, so a module-level private function here, as
    debug_input.py and structure_report.py already settle for this package.

    Each field is a two-element tuple where the C# has a KeyValuePair. Both are immutable,
    which is why the continuation case below replaces the last entry rather than mutating
    it, in both ports.
    """
    # The C# accumulates into a StringBuilder. A list of chunks joined once is the same
    # thing in Python: repeated `text += chunk` would copy the whole accumulated string per
    # chunk, and a metadata blob is read 4096 characters at a time.
    text = []  # type: List[str]
    while not document.get_eof():
        text.append(document.get_text(4096))

    fields = []  # type: List[Tuple[str, str]]

    # The engine does not separate metadata records with newlines. It emits 0x0F before the
    # block and 0x0E between records, so splitting the raw text on '\n' found no boundary at
    # all and collapsed every field of a document into one, with the 0x0F still stuck to the
    # first label. Running it through the same table the text path uses turns those into
    # newlines.
    for record in TextConversion.strip_control_codes(''.join(text)).split('\n'):
        # CR and TAB inside a value each become a single space, so a record stays one line
        # however the document wrote it. A bare CR is the one that matters: it survives the
        # split above and returns a terminal's cursor to column 0, so the record overwrites
        # itself.
        #
        # trim rather than str.strip(): Python calls U+001C-U+001F whitespace and .NET does
        # not, so a value ending in one of those would be a different record here than in
        # the other three ports.
        line = trim(record.replace('\r', ' ').replace('\t', ' '))
        if len(line) == 0:
            continue

        # The C# asks for StringComparison.Ordinal explicitly, because IndexOf(string)
        # defaults to a culture-sensitive comparison there. str.find has no culture-aware
        # variant to opt out of — it is always ordinal — so the argument has no counterpart.
        separator = line.find(": ")
        if separator >= 0:
            fields.append((line[:separator], line[separator + 2:]))
        elif len(fields) > 0:
            # No label, so this is the rest of a value that carried a newline of its own — a
            # Word Title or Comments field routinely does. Dropping it silently truncated
            # the value; it rejoins with the single space the newline became.
            previous_key, previous_value = fields[-1]
            fields[-1] = (previous_key, previous_value + " " + line)

    return fields
