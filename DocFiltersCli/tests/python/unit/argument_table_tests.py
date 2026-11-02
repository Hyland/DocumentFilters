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

from docfilters_cli.cli.argument_definition import ArgumentArity
from docfilters_cli.cli.argument_table import ArgumentTable

from conformance import fixture

# The seven processing commands, written out rather than read from ArgumentTable, which
# stands in for the repeated [InlineData] lines of the C# theories. The duplication is
# deliberate: a command quietly dropped from the table still gets tested here, and
# test_the_seven_processing_commands_are_declared_in_help_order is what ties the two
# lists back together.
_COMMANDS = ("convert", "extract", "list", "identify", "meta", "formats", "options")


class _ArgumentRow:
    """One `= argument template arity` row."""

    def __init__(self):
        self.template = None
        self.arity = None

        self.long_name = None
        """Long name without dashes, e.g. out-dir."""

        self.short_name = None
        """Short name without its dash, or None."""

    @staticmethod
    def parse(row, test_case):
        # str.split() with no argument splits on runs of whitespace and drops the empty
        # fields, which is what the C#'s Split(new[] { ' ', '\t' },
        # RemoveEmptyEntries) does for the space-and-tab padding these rows use.
        parts = row.split()

        if len(parts) != 2:
            raise ValueError(test_case.describe("argument row needs 'template arity': " + row))

        result = _ArgumentRow()
        result.template = parts[0]
        result.arity = _ArgumentRow._parse_arity(parts[1], test_case)

        for form in parts[0].split('|'):
            if form.startswith("--"):
                result.long_name = form[2:]
            elif form.startswith("-"):
                result.short_name = form[1:]

        # An argument written only in short form, such as -x, is named by that form.
        if result.long_name is None:
            result.long_name = result.short_name

        return result

    @staticmethod
    def _parse_arity(text, test_case):
        if text == "none":
            return ArgumentArity.NO_VALUE
        if text == "single":
            return ArgumentArity.SINGLE_VALUE
        if text == "multiple":
            return ArgumentArity.MULTIPLE_VALUE

        raise ValueError(test_case.describe("unknown arity '" + text + "'"))


class ArgumentTableTests(unittest.TestCase):
    """
    The command line surface, checked against the shared arguments.txt fixture.

    The fixture is written from the CLI design document, not dumped from any
    implementation. That direction is the point: a file generated from the C# build
    could only prove the other three agree with C#, whereas this proves all four agree
    with the specification — so an argument nobody implemented still fails.

    This asks "does the right set of arguments exist, with the right arity?".
    ArgumentBindingTests asks "does each one do what it says?".
    """

    def test_command_declares_exactly_the_arguments_in_the_shared_fixture(self):
        for test_case in fixture.load("arguments.txt"):
            with self.subTest(command=test_case.subject):
                command = test_case.subject

                expected = [_ArgumentRow.parse(row, test_case) for row in test_case.get_all("argument")]

                # The C# builds this with ToDictionary, which throws on a duplicate key.
                # A Python dict quietly keeps the last one instead, so the duplicate is
                # left for test_no_command_declares_the_same_argument_twice to report.
                declared = {}
                for argument in ArgumentTable.describe_command(command):
                    declared[argument.long_name] = argument

                # Nothing missing.
                for row in expected:
                    self.assertTrue(
                        row.long_name in declared,
                        test_case.describe("'" + command + "' does not declare " + row.template))

                    actual = declared[row.long_name]

                    self.assertTrue(
                        actual.arity == row.arity,
                        test_case.describe(row.template + ": arity is " + str(actual.arity)
                                           + ", fixture says " + str(row.arity)))

                    self.assertTrue(
                        (actual.short_name or '') == (row.short_name or ''),
                        test_case.describe(row.template + ": short form is '" + str(actual.short_name)
                                           + "', fixture says '" + str(row.short_name) + "'"))

                # And nothing extra: an undocumented argument is drift just as surely as
                # a missing one, and would never be implemented in the other three.
                expected_names = {row.long_name for row in expected}

                for actual in ArgumentTable.describe_command(command):
                    self.assertTrue(
                        actual.long_name in expected_names,
                        test_case.describe("'" + command + "' declares --" + actual.long_name
                                           + ", which arguments.txt does not list"))

    # --------------------------------------- behaviour the fixture cannot state

    def test_the_fixture_covers_every_processing_command(self):
        covered = {case.subject for case in fixture.load("arguments.txt")}

        for command in ArgumentTable.COMMANDS:
            self.assertTrue(command in covered, "arguments.txt has no section for: " + command)

    def test_the_fixture_lists_no_command_the_tool_does_not_have(self):
        for test_case in fixture.load("arguments.txt"):
            self.assertTrue(
                test_case.subject in ArgumentTable.COMMANDS,
                test_case.describe("arguments.txt describes a command the tool does not declare"))

    def test_the_seven_processing_commands_are_declared_in_help_order(self):
        self.assertEqual(
            ["convert", "extract", "list", "identify", "meta", "formats", "options"],
            list(ArgumentTable.COMMANDS))

    def test_help_and_version_are_verbs_but_not_processing_commands(self):
        # They accept no shared options and process no documents, so they are
        # dispatchable without appearing in the applicability table.
        self.assertIn("help", ArgumentTable.ALL_VERBS)
        self.assertIn("version", ArgumentTable.ALL_VERBS)
        self.assertNotIn("help", ArgumentTable.COMMANDS)
        self.assertNotIn("version", ArgumentTable.COMMANDS)

    def test_no_command_declares_the_same_argument_twice(self):
        for command in _COMMANDS:
            with self.subTest(command=command):
                seen = set()

                for argument in ArgumentTable.describe_command(command):
                    self.assertNotIn(argument.long_name, seen,
                                     command + " declares --" + argument.long_name + " twice")
                    seen.add(argument.long_name)

    def test_no_command_reuses_a_short_name(self):
        for command in _COMMANDS:
            with self.subTest(command=command):
                seen = set()

                for argument in ArgumentTable.describe_command(command):
                    if not argument.short_name:
                        continue

                    self.assertNotIn(argument.short_name, seen,
                                     command + " reuses -" + argument.short_name)
                    seen.add(argument.short_name)

    def test_every_argument_has_help_text(self):
        # Prose, so it is not in the fixture — help text may differ in wording between
        # the four implementations. It may not be absent.
        for command in _COMMANDS:
            with self.subTest(command=command):
                for argument in ArgumentTable.describe_command(command):
                    self.assertTrue(argument.description and argument.description.strip(),
                                    "--" + argument.long_name + " has no description")

    def test_a_shared_argument_keeps_the_same_arity_on_every_command_that_accepts_it(self):
        # The defining property of a shared option, and true without exception: --glob
        # cannot be repeatable on convert and single-valued on list, and --regex cannot
        # be a path pattern on one command and a mode switch on another.
        arities = {}
        first_seen_on = {}

        for command in ArgumentTable.COMMANDS:
            for argument in ArgumentTable.describe_command(command):
                if argument.long_name in arities:
                    existing = arities[argument.long_name]
                    self.assertTrue(
                        existing == argument.arity,
                        "--" + argument.long_name + " is " + str(existing) + " on "
                        + first_seen_on[argument.long_name] + " but " + str(argument.arity)
                        + " on " + command)
                else:
                    arities[argument.long_name] = argument.arity
                    first_seen_on[argument.long_name] = command

    def test_extract_accepts_the_shared_regex_like_every_other_walking_command(self):
        # extract walks directories like the others, so it selects paths the same way:
        # -g/--glob and --regex, with --include/--exclude picking subfiles.
        declared = [a.long_name for a in ArgumentTable.describe_command("extract")]

        self.assertIn("regex", declared)
        self.assertIn("include", declared)
        self.assertNotIn("filter", declared)

    def test_include_and_exclude_are_a_matched_pair_on_extract(self):
        # A matched pair: same arity, opposite direction.
        include = self._single("extract", "include")
        exclude = self._single("extract", "exclude")

        self.assertEqual(exclude.arity, include.arity)

    def _single(self, command, long_name):
        """The one argument on a command with this long name, as the C#'s Single() does."""
        matches = [a for a in ArgumentTable.describe_command(command) if a.long_name == long_name]

        self.assertEqual(1, len(matches),
                         command + " declares --" + long_name + " " + str(len(matches)) + " times")
        return matches[0]


if __name__ == '__main__':
    unittest.main()
