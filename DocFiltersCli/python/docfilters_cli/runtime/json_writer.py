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
from typing import Optional


class _ContainerKind(Enum):
    OBJECT = 1
    ARRAY = 2


class _Frame:
    def __init__(self, kind: _ContainerKind):
        self.kind = kind
        self.has_children = False


class JsonWriter:
    """
    A minimal JSON writer, hand-rolled so that no library's escaping policy reaches
    the contract.

    Only used for the --json listings on list, identify, meta, formats and options.
    Note that convert --to json is a completely different thing: that is the engine's
    own JSON canvas, and its bytes are streamed through untouched.

    Why this is not json.dumps: Two of the four languages have no JSON writer to
    reach for: the JDK has never shipped one (JEP 198 was proposed and never
    delivered) and the C++17 standard library has none, so using the native writer
    would mean a new product dependency in half the ports. And the two that do have
    one disagree by default — System.Text.Json escapes <, >, &, + and all non-ASCII
    unless given a relaxed encoder, while Python's json.dumps defaults to
    ensure_ascii=True. The rule below says non-ASCII is written literally, which is
    the default neither of them gives. D1 applied to JSON rather than to error text:
    the contract is ours, so the writer is ours.

    Output rules, which this port reproduces even though Python has a json module:
    - two-space indent, one key per line
    - keys in insertion order, never sorted
    - UTF-8 with no byte order mark, LF line endings
    - non-ASCII written literally, not escaped as \\uXXXX
    - a surrogate code point escaped as \\uXXXX, the single exception to the rule
      above, because an unpaired surrogate is not a character and has no UTF-8
      encoding to write literally
    - a trailing newline at the end of the document
    """

    def __init__(self):
        self._text = []
        self._open = []
        self._at_property_value = False

    def begin_object(self):
        """Starts an object. Must be matched by end_object()."""
        return self._begin_container(_ContainerKind.OBJECT, '{')

    def end_object(self):
        return self._end_container('}')

    def begin_array(self):
        """Starts an array. Must be matched by end_array()."""
        return self._begin_container(_ContainerKind.ARRAY, '[')

    def end_array(self):
        return self._end_container(']')

    def name(self, name: str):
        """Writes a property name, which the next value belongs to."""
        self._begin_item()
        self._text.append('"')
        self._text.append(self.escape(name))
        self._text.append('": ')
        self._at_property_value = True
        return self

    def value(self, val):
        """Writes a value: string, int, bool, or None."""
        self._before_value()
        if val is None:
            self._text.append('null')
        elif isinstance(val, bool):
            self._text.append('true' if val else 'false')
        elif isinstance(val, int):
            self._text.append(str(val))
        elif isinstance(val, str):
            self._text.append('"')
            self._text.append(self.escape(val))
            self._text.append('"')
        else:
            raise TypeError(f"Unsupported JSON value type: {type(val)}")
        return self

    def property(self, name: str, val):
        """Convenience for a name and value in one call."""
        return self.name(name).value(val)

    def to_string(self) -> str:
        """The finished document, ending with a newline."""
        if self._open:
            raise RuntimeError("a JSON object or array was left open")
        return ''.join(self._text) + '\n'

    @staticmethod
    def escape(value: str) -> str:
        """Escapes a string the way value() does. Exposed for tests."""
        escaped = []
        for c in value:
            if c == '"':
                escaped.append('\\"')
            elif c == '\\':
                escaped.append('\\\\')
            elif c == '\n':
                escaped.append('\\n')
            elif c == '\r':
                escaped.append('\\r')
            elif c == '\t':
                escaped.append('\\t')
            # The reference escapes an unpaired UTF-16 unit and writes a well-formed
            # pair literally. A Python str holds code points, so a real pair is one
            # character above U+FFFF and never reaches this test; any code point in
            # U+D800..U+DFFF is unpaired by construction and needs no look-behind.
            # Written literally it makes the UTF-8 encoder raise UnicodeEncodeError.
            # See "A lone surrogate cannot be written literally" in traps-python.md.
            elif ord(c) < 0x20 or 0xD800 <= ord(c) <= 0xDFFF:
                escaped.append('\\u{:04x}'.format(ord(c)))
            else:
                escaped.append(c)
        return ''.join(escaped)

    def _begin_container(self, kind: _ContainerKind, open_token: str):
        """
        Opens a new container as either a property's value or an array element,
        which is what _before_value() also does for a plain value — the two are
        unified here since a nested object or array is just another kind of value.
        """
        self._before_value()
        self._text.append(open_token)
        self._open.append(_Frame(kind))
        return self

    def _end_container(self, close_token: str):
        frame = self._open.pop()
        if frame.has_children:
            self._text.append('\n')
            self._text.append('  ' * len(self._open))
        self._text.append(close_token)
        return self

    def _before_value(self):
        """
        Writes the comma-and-newline separator and this level's indent before a
        new array element or object property, unless the value is a property's
        own value — name() already placed that punctuation.
        """
        if not self._at_property_value:
            self._begin_item()
        self._at_property_value = False

    def _begin_item(self):
        """Marks the current container as non-empty and writes the separator before its next entry."""
        if not self._open:
            return  # the document's own root value; nothing to separate it from

        frame = self._open[-1]
        self._text.append(',\n' if frame.has_children else '\n')
        self._text.append('  ' * len(self._open))
        frame.has_children = True
