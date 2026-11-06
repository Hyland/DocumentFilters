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

import unittest

from docfilters_cli.runtime.json_writer import JsonWriter


class JsonWriterTests(unittest.TestCase):
    """
    The hand-rolled JSON writer.

    These tests exist because Python has a JSON module and the other three languages
    do not: this port must reproduce exactly the C# output rather than accept whatever
    json.dumps defaults to. Key order, indent, escaping and the trailing newline are
    all contract.
    """

    def test_an_empty_object_renders_on_one_line(self):
        self.assertEqual("{}\n", JsonWriter().begin_object().end_object().to_string())

    def test_properties_are_indented_by_two_spaces(self):
        json = (JsonWriter()
                .begin_object()
                .property("name", "report.pdf")
                .end_object()
                .to_string())

        self.assertEqual("{\n  \"name\": \"report.pdf\"\n}\n", json)

    def test_keys_stay_in_insertion_order_and_are_never_sorted(self):
        # Sorting would be a gratuitous difference from the order the engine reports
        # fields in, and Python's json module sorts only on request.
        json = (JsonWriter()
                .begin_object()
                .property("zebra", 1)
                .property("apple", 2)
                .end_object()
                .to_string())

        self.assertLess(json.index("zebra"), json.index("apple"))

    def test_nested_objects_indent_cumulatively(self):
        json = (JsonWriter()
                .begin_object()
                .name("inner").begin_object().property("a", 1).end_object()
                .end_object()
                .to_string())

        self.assertEqual("{\n  \"inner\": {\n    \"a\": 1\n  }\n}\n", json)

    def test_arrays_render_one_element_per_line(self):
        json = (JsonWriter()
                .begin_array()
                .value("a")
                .value("b")
                .end_array()
                .to_string())

        self.assertEqual("[\n  \"a\",\n  \"b\"\n]\n", json)

    def test_an_empty_array_renders_on_one_line(self):
        self.assertEqual("[]\n", JsonWriter().begin_array().end_array().to_string())

    def test_booleans_and_numbers_are_unquoted(self):
        json = (JsonWriter()
                .begin_object()
                .property("count", 42)
                .property("ok", True)
                .end_object()
                .to_string())

        self.assertIn("\"count\": 42", json)
        self.assertIn("\"ok\": true", json)

    def test_a_bool_is_not_written_as_a_number(self):
        # Not in the C# suite, and needed only here: bool is a subclass of int in
        # Python, so an isinstance check in the wrong order writes True as 1 and the
        # four ports disagree on every JSON listing with a flag in it.
        self.assertIn("\"ok\": true", JsonWriter().begin_object().property("ok", True).end_object().to_string())
        self.assertIn("\"ok\": false", JsonWriter().begin_object().property("ok", False).end_object().to_string())

    def test_a_null_string_renders_as_null(self):
        json = JsonWriter().begin_object().property("modified", None).end_object().to_string()

        self.assertIn("\"modified\": null", json)

    def test_the_mandatory_escapes_are_applied(self):
        for text, expected in [("a\"b", "a\\\"b"),
                               ("a\\b", "a\\\\b"),
                               ("a\nb", "a\\nb"),
                               ("a\tb", "a\\tb"),
                               ("a\rb", "a\\rb")]:
            with self.subTest(text=text):
                self.assertEqual(expected, JsonWriter.escape(text))

    def test_control_characters_use_the_six_character_form(self):
        self.assertEqual("\\u0000", JsonWriter.escape("\0"))

    def test_non_ascii_is_written_literally_rather_than_escaped(self):
        # json.dumps escapes non-ASCII by default. It must not here, or the four
        # implementations produce different bytes for the same document.
        self.assertEqual("Ünïcödé", JsonWriter.escape("Ünïcödé"))

    def test_a_forward_slash_is_not_escaped(self):
        # Legal either way in JSON, so it has to be pinned down.
        self.assertEqual("a/b", JsonWriter.escape("a/b"))

    def test_a_surrogate_pair_is_written_literally(self):
        # U+1F600. C# holds it as two UTF-16 units and must not mistake them for two
        # unpaired surrogates; Python holds one code point, so this pins that the
        # range test does not catch it.
        self.assertEqual("\U0001F600", JsonWriter.escape("\U0001F600"))

    def test_an_unpaired_high_surrogate_is_escaped(self):
        # Reachable from a Windows filename, which NTFS stores as unvalidated UTF-16.
        # Written literally it has no UTF-8 encoding: Python's encoder raises
        # UnicodeEncodeError, where the other ports invent a replacement. Escaped,
        # all four emit the same six ASCII bytes.
        self.assertEqual("\\ud800", JsonWriter.escape("\ud800"))

    def test_an_unpaired_low_surrogate_is_escaped(self):
        self.assertEqual("\\udfff", JsonWriter.escape("\udfff"))

    def test_an_unpaired_surrogate_beside_a_pair_leaves_the_pair_intact(self):
        self.assertEqual("\\ud800\U0001F600", JsonWriter.escape("\ud800\U0001F600"))

    def test_an_escaped_document_carries_no_surrogates_of_its_own(self):
        # The property that matters downstream, stated over the writer rather than
        # over escape(): whatever goes in, what comes out can be encoded.
        json = (JsonWriter()
                .begin_object()
                .property("title", "a\ud800b")
                .end_object()
                .to_string())

        self.assertIn('"title": "a\\ud800b"', json)
        json.encode("utf-8")

    def test_the_document_ends_with_exactly_one_newline(self):
        json = JsonWriter().begin_object().property("a", 1).end_object().to_string()

        self.assertTrue(json.endswith("}\n"))
        self.assertNotIn("\n\n", json)

    def test_line_endings_are_always_lf_even_on_windows(self):
        json = JsonWriter().begin_object().property("a", 1).end_object().to_string()

        self.assertNotIn("\r", json)

    def test_an_unclosed_object_is_a_programming_error(self):
        with self.assertRaises(RuntimeError):
            JsonWriter().begin_object().to_string()


if __name__ == '__main__':
    unittest.main()
