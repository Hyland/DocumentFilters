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

from typing import Dict, Optional, TextIO, Union

from ..configuration.command_options import ConvertOptions
from ..engine.i_document_filters_engine import IDocument
from ..planning.format_spec import FormatSpec

# The ISYS control-code table, as one mapping per value of isFlowHtml. str.translate
# applies exactly the C#'s switch -- ordinal to replacement string, or to None for
# "drop it" -- so the two are the same transformation written two ways, and the table
# being data rather than control flow is what makes it readable as a table.
#
# Building both up front rather than choosing per character keeps the per-chunk work to
# one call into the C implementation; a 4096-character Python loop per chunk is the
# hottest thing in the text path and there is no reason for it to be interpreted.
_STRIPPED = {
    0x01: ' ',   # start-of-heading
    0x02: None,  # normal
    0x03: None,  # bold
    0x04: None,  # italic
    0x05: None,  # underline
    0x06: None,  # start-of-hit
    0x0c: None,  # soft break
    0x10: None,  # end-of-meta
    0x0e: '\n',  # paragraph
    0x0f: '\n',  # start-of-meta
}  # type: Dict[int, Optional[str]]

# The flow-html table differs in exactly one entry, so it is derived rather than restated:
# a second literal is a second place for the other nine to drift.
_STRIPPED_FLOW_HTML = dict(_STRIPPED)
_STRIPPED_FLOW_HTML[0x0e] = None


class TextConversion:
    """
    The text path: formats produced straight from the extractor, without a
    canvas -- text, md-text and flow html.

    This is the shortest and most-copied piece of Document Filters code there is:
    open the extractor with the right arguments, then pull text until EOF.
    """

    def convert(self, document: IDocument, format: Optional[FormatSpec], options: ConvertOptions,
                destination: TextIO) -> None:
        """
        Writes one document's text to destination.

        Precondition: document must already have been opened with
        ExtractorOpenSettings.for_format for this same format. The engine applies those
        flags during IDocument.open and they cannot be changed afterwards; the binding
        exposes no way to read them back, so this is enforced by ConvertCommand being the
        only caller and opening the document in one step.

        Control codes are stripped unless --include-control-codes asks for them; nothing
        else is rewritten -- no line-ending translation, no re-encoding -- because the
        payload is the document.

        destination is a text stream, as the reference's TextWriter is, so the encoding
        stays the caller's decision in both. The Python caller carries one extra
        obligation the reference does not: the stream must have been opened with
        newline='', because a text stream left at its default translates every '\\n'
        written through it into the host's line ending. That would rewrite the payload on
        Windows -- the one thing the remark above says does not happen -- and it would do
        so invisibly, since the text is correct and only the bytes differ.
        """
        while not document.get_eof():
            chunk = document.get_text(4096)
            if options.include_control_codes:
                destination.write(chunk)
            else:
                destination.write(self.strip_control_codes(
                    chunk, format is not None and format.token == "html"))

    @staticmethod
    def strip_control_codes(chunk: str, is_flow_html: bool = False) -> str:
        """
        Replaces the ISYS control codes the extractor emits with what they mean.

        This has to happen here. It cannot be asked of the engine: a text-mode open
        resolves to the same reader whether IGR_FORMAT_TEXT is named or left out --
        a missing format bit and IGR_FORMAT_TEXT select the same text mode -- so there
        is no flag that turns this on or off. doc2text strips them
        in the tool for the same reason, in processOutput, and this is that table:

        - 0x01 start-of-heading becomes a space.
        - 0x02-0x06 (normal, bold, italic, underline, start-of-hit), 0x0C soft break and
          0x10 end-of-meta are dropped: they mark styling this tool does not carry.
        - 0x0E paragraph and 0x0F start-of-meta become newlines.

        Safe on a chunk boundary because every code is a single character and none depends
        on its neighbours, so splitting the stream cannot split one.

        Flow html keeps its paragraph marks: doc2text suppresses the 0x0E newline when the
        open flags say HTML, where the markup already carries the structure and a newline
        would only add whitespace.

        Args:
            is_flow_html: True for flow html only, which wants no newline from a paragraph
                mark. Everything else -- including meta, whose records the engine separates
                with these same codes -- passes False.
        """
        return chunk.translate(_STRIPPED_FLOW_HTML if is_flow_html else _STRIPPED)
