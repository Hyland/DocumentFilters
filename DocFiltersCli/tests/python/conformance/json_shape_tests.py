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
import unittest

from docfilters_cli.engine.i_document_filters_engine import EngineFormat, EngineOption
from docfilters_cli.runtime import exit_code
from cli.cli_harness import CliHarness
from fakes.fake_document_filters_engine import FakeSubFile


class JsonShapeTests(unittest.TestCase):
    """
    The --json shape of every command that has one.

    These five shapes are the tool's machine-readable interface, so a renamed key or a
    restructured array breaks whatever is parsing it. They used to be described only by a
    conformance fixture that nothing ever executed, and asserted nowhere: no test called
    write_json at all, and the end-to-end script only checked that the output parsed. See
    D52.

    Driven through CliHarness rather than the report classes, so the assertion covers the
    path a user actually takes -- argv, dispatch, the command, JsonWriter -- and the engine
    answers from a script, which is what lets the expected document be written out in full
    rather than described.
    """

    def _json(self, cli: CliHarness, *args: str) -> str:
        """
        Runs a command that must succeed, and returns the JSON document it wrote.

        A method rather than the reference's private static helper, for the reason
        _assert_inside_root states in subfile_extraction_tests: it asserts, and assertEqual
        lives on the TestCase.
        """
        self.assertEqual(exit_code.SUCCESS, cli.run(*args))
        self.assertEqual([], cli.errors)

        # Raw, not the line list: a JSON report is built as one document and
        # written in a single call, so line capture misses it entirely.
        return cli.output.replace("\r\n", "\n").rstrip("\n")

    def test_identify_reports_an_array_of_path_format_mime_and_name(self):
        cli = CliHarness().with_document("sample.docx")
        document = cli.engine.add_document("sample.docx")
        document.file_type = 254
        document.mime_type = \
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
        document.format_name = "Microsoft Word 2007"

        # Python has no object initialiser, so every field the reference sets inside
        # `{ ... }` is an assignment after the construction here. Same for the formats,
        # the options and the subfile entry below.
        self.assertEqual("\n".join([
            "[",
            "  {",
            "    \"path\": \"sample.docx\",",
            "    \"format\": 254,",
            "    \"mime\": \"application/vnd.openxmlformats-officedocument"
            ".wordprocessingml.document\",",
            "    \"name\": \"Microsoft Word 2007\"",
            "  }",
            "]"]),
            self._json(cli, "identify", "--json", "sample.docx"))

    def test_meta_reports_metadata_as_a_nested_object_keyed_by_field_name(self):
        # Nested rather than flat, and every value a string: the engine reports
        # metadata as text, so a port that guessed at numbers would disagree about
        # "Pages".
        cli = CliHarness().with_document(
            "sample.docx",
            "\u000FAuthor: Jane Smith\u000ETitle: Quarterly Report\u000EPages: 12\u0010")

        self.assertEqual("\n".join([
            "[",
            "  {",
            "    \"path\": \"sample.docx\",",
            "    \"metadata\": {",
            "      \"Author\": \"Jane Smith\",",
            "      \"Title\": \"Quarterly Report\",",
            "      \"Pages\": \"12\"",
            "    }",
            "  }",
            "]"]),
            self._json(cli, "meta", "--json", "sample.docx"))

    def test_list_reports_subfiles_as_an_array_inside_each_container(self):
        cli = CliHarness().with_document("bundle.zip")
        container = cli.engine.add_document("bundle.zip")
        entry = FakeSubFile("readme.txt", "0001")
        entry.size = 4096
        # An aware datetime is Python's DateTimeKind.Utc. format_timestamp reads the
        # calendar fields rather than converting, so the rendered text is the same
        # either way -- the kind is stated because the reference states it.
        entry.modified = datetime.datetime(2026, 1, 15, 9, 30, 0,
                                           tzinfo=datetime.timezone.utc)
        container.sub_file_entries.append(entry)

        self.assertEqual("\n".join([
            "[",
            "  {",
            "    \"path\": \"bundle.zip\",",
            "    \"subfiles\": [",
            "      {",
            "        \"id\": \"0001\",",
            "        \"name\": \"readme.txt\",",
            "        \"size\": 4096,",
            "        \"modified\": \"2026-01-15T09:30:00Z\",",
            "        \"depth\": 1",
            "      }",
            "    ]",
            "  }",
            "]"]),
            self._json(cli, "list", "--json", "bundle.zip"))

    def test_formats_reports_a_flat_array_of_id_ext_and_name(self):
        cli = CliHarness()
        docx = EngineFormat()
        docx.id = 254
        docx.short_name = "docx"
        docx.display_name = "Microsoft Word 2007"
        cli.engine.formats.append(docx)

        doc = EngineFormat()
        doc.id = 112
        doc.short_name = "doc"
        doc.display_name = "Microsoft Word for Windows"
        cli.engine.formats.append(doc)

        self.assertEqual("\n".join([
            "[",
            "  {",
            "    \"id\": 254,",
            "    \"ext\": \"docx\",",
            "    \"name\": \"Microsoft Word 2007\"",
            "  },",
            "  {",
            "    \"id\": 112,",
            "    \"ext\": \"doc\",",
            "    \"name\": \"Microsoft Word for Windows\"",
            "  }",
            "]"]),
            self._json(cli, "formats", "--json", "--filter", "word"))

    def test_options_reports_name_type_default_appliesTo_and_description(self):
        # appliesTo is the one camelCase key in the whole surface -- everything else
        # is a single lowercase word, so it is easy to port as "applies-to" or
        # "appliesto" by accident.
        cli = CliHarness()
        option = EngineOption()
        option.display_name = "BW_MODE"
        option.type = "int"
        option.default_value = "0"
        option.flags = 0x2   # canvas; 0x1 is document, and both join as "document,canvas"
        option.description = "Black and white rendering mode"
        cli.engine.options.append(option)

        self.assertEqual("\n".join([
            "[",
            "  {",
            "    \"name\": \"BW_MODE\",",
            "    \"type\": \"int\",",
            "    \"default\": \"0\",",
            "    \"appliesTo\": \"canvas\",",
            "    \"description\": \"Black and white rendering mode\"",
            "  }",
            "]"]),
            self._json(cli, "options", "--json", "--filter", "BW_MODE"))

    def test_every_json_command_emits_one_document_even_with_nothing_to_report(self):
        # An empty array, not empty output: whatever is parsing this should not
        # have to special-case "no results" as a parse failure.
        cli = CliHarness()

        self.assertEqual("[]",
                         self._json(cli, "formats", "--json", "--filter",
                                    "nothing-matches-this"))
        self.assertEqual("[]",
                         self._json(CliHarness(), "options", "--json", "--filter",
                                    "nothing-matches-this"))
