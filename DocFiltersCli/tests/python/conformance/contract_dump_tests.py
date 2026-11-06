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

# json is imported for exactly one purpose: to decide whether a string parses as a JSON
# document. The tool never generates JSON with it — that is JsonWriter's job, so that the
# escaping policy belongs to the contract rather than to a library — and neither does this
# suite. Parsing for verification is the one legitimate use, and it stands in for the C#'s
# JsonDocument.Parse.
import json
import unittest
from typing import List

from docfilters_cli.cli.argument_table import ArgumentTable
from docfilters_cli.cli.contract_dump import ContractDump

from conformance import fixture


class ContractDumpTests(unittest.TestCase):
    """
    --print-contract: the tool's own account of its command line surface.

    A debugging aid and the machine-readable answer to `help --json`, so what is tested
    here is that it faithfully reflects the argument table.

    The surface itself is checked against arguments.txt in unit.argument_table_tests
    .ArgumentTableTests.
    """

    def test_every_verb_appears_in_the_dump(self):
        dump = ContractDump.render()

        for command in ArgumentTable.COMMANDS:
            self.assertIn("command " + command, dump)

    def test_every_argument_on_every_command_appears_in_the_dump(self):
        for command in ArgumentTable.COMMANDS:
            with self.subTest(command=command):
                section = ContractDump.render_command(command)

                for argument in ArgumentTable.describe_command(command):
                    self.assertTrue("--" + argument.long_name in section,
                                    command + ": --" + argument.long_name + " missing from the dump")

    def test_the_dump_declares_nothing_the_argument_table_does_not_have(self):
        # Without this a stale line in the dump would send the other three chasing a
        # argument that no longer exists.
        for command in ArgumentTable.COMMANDS:
            with self.subTest(command=command):
                declared = {f.long_name for f in ArgumentTable.describe_command(command)}

                for name in self._long_names_in(ContractDump.render_command(command)):
                    self.assertTrue(name in declared,
                                    command + ": --" + name + " is in the dump but not in the argument table")

    def test_the_dump_agrees_with_the_shared_fixture(self):
        # Transitively true given the two tests above and ArgumentTableTests, but worth
        # asserting directly: this is the pair of files a reader will diff by hand
        # when something looks wrong.
        for test_case in fixture.load("arguments.txt"):
            with self.subTest(command=test_case.subject):
                section = ContractDump.render_command(test_case.subject)

                for row in test_case.get_all("argument"):
                    # str.split() with no argument splits on runs of whitespace and drops
                    # the empty fields, which is what the C#'s Split(new[] { ' ', '\t' },
                    # RemoveEmptyEntries) does for the space-and-tab padding these rows use.
                    template = row.split()[0]
                    long_form = template.split('|')[-1]

                    self.assertTrue(
                        long_form in section,
                        test_case.describe(long_form + " is in arguments.txt but not in the contract dump"))

    def test_each_argument_records_its_arity(self):
        # The other three need this to register an argument correctly: a switch declared
        # as taking a value silently swallows the next argument.
        section = ContractDump.render_command("convert")

        self.assertIn("none", section)
        self.assertIn("single", section)
        self.assertIn("multiple", section)

    def test_the_dump_is_deterministic(self):
        self.assertEqual(ContractDump.render(), ContractDump.render())

    def test_the_dump_is_line_oriented_and_free_of_json(self):
        # Not "contains no brace" - a description is free to mention template
        # placeholders like {stem} or another flag's name. The actual property
        # that matters is that the dump, as a whole, is not itself JSON: valid
        # JSON permits nothing but whitespace outside its one outermost value,
        # so a document with "command convert" before any brace, English prose,
        # and multiple top-level lines can never parse as one JSON document -
        # unlike render_json's output, which always does.
        dump = ContractDump.render()

        # The C# is Assert.ThrowsAny<JsonException> and asserts nothing about the
        # exception itself, so neither does this: that a parse was attempted and refused
        # is the whole claim. json.JSONDecodeError is the Python counterpart, a ValueError
        # subclass raised for any malformed document.
        with self.assertRaises(json.JSONDecodeError):
            json.loads(dump)

    def test_the_dump_uses_lf_line_endings(self):
        # Otherwise it differs between a Windows and a Linux build.
        self.assertNotIn("\r", ContractDump.render())

    # ----------------------------------------------------------- help --json

    def test_the_json_rendering_is_actually_json(self):
        # The whole reason it is a separate renderer: the flat dump is asserted
        # brace-free just above, so an argument named --json could not share it.
        text = ContractDump.render_json()

        self.assertTrue(text.lstrip().startswith("["), "the JSON rendering does not start with [")
        self.assertTrue(text.endswith("\n"), "the JSON rendering does not end with a newline")
        self.assertNotIn("\r", text)

        # Extra to the C#, and the one assertion this method's name earns: the C# only
        # looks at the first and last characters, which the two-space indented output the
        # Python JsonWriter emits satisfies just as the compact System.Text.Json output
        # did. Parsing it proves the indentation did not cost validity.
        self.assertIsInstance(json.loads(text), list)

    def test_the_json_rendering_names_every_command(self):
        text = ContractDump.render_json()

        for command in ArgumentTable.COMMANDS:
            self.assertIn("\"" + command + "\"", text)

    def test_the_json_rendering_names_every_argument_on_every_command(self):
        text = ContractDump.render_json()

        for command in ArgumentTable.COMMANDS:
            with self.subTest(command=command):
                for argument in ArgumentTable.describe_command(command):
                    self.assertTrue("--" + argument.long_name in text,
                                    command + ": --" + argument.long_name + " missing from the JSON")

    def test_the_json_rendering_carries_arity_and_help_text(self):
        # A port registering arguments from this needs the arity; a person reading
        # it wants the description. Both have to survive the rendering.
        text = ContractDump.render_json()

        self.assertIn("\"arity\"", text)
        self.assertIn("\"description\"", text)
        self.assertIn("\"template\"", text)

    def test_the_json_rendering_is_deterministic(self):
        self.assertEqual(ContractDump.render_json(), ContractDump.render_json())

    def test_the_two_renderings_describe_the_same_surface(self):
        # They share the argument table rather than the renderer, so nothing but a
        # test keeps them agreeing.
        text = ContractDump.render_json()

        for command in ArgumentTable.COMMANDS:
            with self.subTest(command=command):
                for name in self._long_names_in(ContractDump.render_command(command)):
                    self.assertTrue("--" + name in text,
                                    "--" + name + " is in the flat dump but not in the JSON")

    @staticmethod
    def _long_names_in(section: str) -> List[str]:
        """
        Pulls the long argument names back out of a rendered section by reading
        only the template column, never the free-form description — a line's
        tab-separated fields are `argument`, template, arity, description, and
        only the template is safe to search for --name tokens. A word
        split over the whole line would just as happily find one inside prose
        that happens to mention another flag by name.
        """
        names = []

        for line in section.split('\n'):
            trimmed = line.strip()
            if not trimmed.startswith("argument"):
                continue

            fields = trimmed.split('\t')
            if len(fields) < 2:
                continue

            for form in fields[1].split('|'):
                if form.startswith("--"):
                    names.append(form[2:])

        return names


if __name__ == '__main__':
    unittest.main()
