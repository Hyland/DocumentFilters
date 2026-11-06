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
from typing import List

from docfilters_cli.cli.argument_table import ArgumentTable
from docfilters_cli.cli.command_line_app import CommandLineApp
from docfilters_cli.commands.i_command import CommandContext
from docfilters_cli.runtime.cli_error import CliError, ErrorCodes
from docfilters_cli.runtime.i_file_system import IFileSystem
from fakes.recording_console import RecordingConsole

from conformance import fixture, plan_corpus


class PlanCorpusTests(unittest.TestCase):
    """
    The cross-language contract, executed.

    Each case runs stages 1 to 3 -- parse, bind, plan -- over the filesystem it is given,
    and compares the canonical plan text against the shared fixture. No document is opened,
    so these need neither a license nor the native library.

    The C#, Java and C++17 suites run the same fixture through their own implementations. A
    case passing in all four is the definition of the four agreeing.
    """

    def test_case_matches_the_shared_corpus(self):
        # xUnit's [Theory] with [MemberData] gives one test per case; unittest has no
        # counterpart, so the cases are a loop and each iteration is a subTest. A subTest
        # failure is reported on its own with the case named, and the loop runs to the end
        # rather than stopping at the first -- which is what the [Theory] does too, and what
        # makes a corpus of this size worth reading the output of.
        for test_case in plan_corpus.load():
            with self.subTest(case=str(test_case)):
                self._assert_case_matches(test_case)

    def _assert_case_matches(self, test_case: fixture.FixtureCase) -> None:
        args = plan_corpus.split_arguments(test_case.subject)
        expected_exit = plan_corpus.expected_exit(test_case)

        file_system = plan_corpus.build_file_system(test_case)

        if test_case.error is not None:
            # Assert.ThrowsAny<CliError>, which accepts a subclass -- and every error this
            # can raise is one, CliError itself being abstract in exit_code.
            with self.assertRaises(CliError) as caught:
                _plan(args, file_system)
            error = caught.exception

            self.assertTrue(
                error.code == test_case.error,
                test_case.describe("expected error {0}, got {1}: {2}".format(
                    test_case.error, error.code, error.message)))

            self.assertTrue(
                error.exit_code == expected_exit,
                test_case.describe("expected exit {0}, got {1}".format(
                    expected_exit, error.exit_code)))

            # Error text is part of the contract, so four ports cannot each phrase the same
            # failure differently and all pass.
            expected_message = test_case.get("message")

            self.assertTrue(
                _normalise(error.message) == _normalise(expected_message),
                test_case.describe("message was '{0}', expected '{1}'".format(
                    error.message, expected_message)))
            return

        # The whole document, in order -- not a search for fragments. Substring matching
        # would let extra lines, reordered fields and duplicate targets pass in one
        # implementation and fail in another, which is exactly the drift this corpus exists
        # to catch.
        expected = [_normalise(key + " " + value)
                    for key, value in test_case.expectations if key != "exit"]

        plan = _plan(args, file_system)

        actual = [line for line in (_normalise(row) for row in plan.split('\n'))
                  if len(line) > 0]

        # assertEqual rather than the reference's Assert.True over SequenceEqual: the
        # comparison is the same, and unittest adds its own line-by-line diff to the
        # message, which is strictly more than the C# prints. Expected first, as every
        # suite in this port has it.
        self.assertEqual(expected, actual,
                         test_case.describe("plan does not match. Plan as produced:\n"
                                            + _indent(plan)))

    # ------------------------------------------------------ the corpus itself

    def test_the_corpus_is_present_and_not_empty(self):
        # A missing fixture would otherwise make every case above vanish silently rather
        # than fail.
        self.assertTrue(len(plan_corpus.load()) > 0)

    def test_every_case_states_an_expected_exit_code(self):
        for test_case in plan_corpus.load():
            self.assertTrue(test_case.has("exit"),
                            test_case.describe("no '= exit' expectation"))

    def test_every_failing_case_expects_a_non_zero_exit(self):
        for test_case in _failing_cases():
            self.assertTrue(plan_corpus.expected_exit(test_case) != 0,
                            test_case.describe("expects an error but exit 0"))

    def test_every_expected_error_code_is_one_the_tool_can_raise(self):
        # Catches a typo in the fixture, which would otherwise look like an implementation
        # bug in whichever language ran it next.
        known = set(ErrorCodes.all_codes())

        for test_case in _failing_cases():
            self.assertTrue(test_case.error in known,
                            test_case.describe("unknown error code"))

    def test_every_failing_case_states_its_message(self):
        # stderr is contract. A case that gives only a code would let the four
        # implementations phrase the same failure four ways and all pass.
        for test_case in _failing_cases():
            self.assertTrue(test_case.has("message"),
                            test_case.describe("no '= message' expectation"))

    def test_every_succeeding_case_states_a_complete_plan(self):
        # A case listing only '= exit 0' would pass against any plan whatsoever.
        for test_case in _succeeding_cases():
            self.assertTrue(test_case.has("command"),
                            test_case.describe("a succeeding case must state 'command'"))

    def test_only_commands_with_an_output_group_state_a_cardinality(self):
        """
        Commands that declare the output group state a cardinality; the rest do not.

        Cardinality answers "who owns the output target". identify declares no output group
        at all, and formats and options read no inputs and write no files, so for those
        three the question has no answer and printing one would be inventing it.
        """
        owns = {"convert", "extract", "list", "meta"}

        for test_case in _succeeding_cases():
            command = test_case.get("command")
            if command is None:
                continue

            self.assertTrue(
                test_case.has("cardinality") == (command in owns),
                test_case.describe(
                    command + " owns its output and must state a cardinality"
                    if command in owns
                    else command + " owns no output target and must not state a cardinality"))

    def test_all_cases_for_one_command_emit_the_same_line_set(self):
        """
        Every succeeding case for one command emits the same lines, in the same order.

        The corpus is compared as a whole document (D14), so which lines appear has to be a
        property of the command and not of the arguments. Two cases with the same argv and
        different line sets cannot both be satisfied, in any language.
        """
        shapes = {}
        first_seen = {}

        for test_case in _succeeding_cases():
            command = test_case.get("command")
            if command is None:
                continue

            # target and pattern are the one part of the plan that legitimately varies: one
            # per input for a resolved path, one per input for a per-page pattern (D13), and
            # none at all when the output is streamed to stdout. Their placement is checked
            # separately below.
            keys = [key for key, _ in test_case.expectations if not _is_variable_key(key)]
            shape = " ".join(keys)

            if command in shapes:
                self.assertTrue(
                    shapes[command] == shape,
                    test_case.describe(
                        "every '{0}' case must emit the same lines in the same order.\n"
                        "      this case: {1}\n"
                        "      {2}: {3}".format(command, shape, first_seen[command],
                                                shapes[command])))
            else:
                shapes[command] = shape
                first_seen[command] = test_case

    def test_planner_errors_declare_the_inputs_they_need(self):
        """
        Every case whose error comes from the planner declares the inputs it needs.

        The walk runs before planning, so a case meaning to provoke E_OUTFILE_MULTI_INPUT
        with no tree gets E_NO_INPUTS instead: it passes for the wrong reason and tests
        nothing.
        """
        after_discovery = {ErrorCodes.OUTFILE_MULTI_INPUT, ErrorCodes.OUTPUT_COLLISION,
                           ErrorCodes.OUTPUT_EXISTS}

        for test_case in plan_corpus.load():
            # The C# reads `c.Error ?? string.Empty` to keep a null out of the set lookup;
            # Python can test None against a set of strings directly, so the fallback has no
            # counterpart.
            if test_case.error not in after_discovery:
                continue

            self.assertTrue(
                test_case.has_given("tree"),
                test_case.describe(test_case.error
                                   + " is raised after the walk, so the case needs a '< tree'"))

    def test_cases_that_walk_a_directory_ask_for_a_defined_order(self):
        """A case that walks a directory sorts, because directory order is otherwise undefined."""
        for test_case in _succeeding_cases():
            # More than one file under a common root means the walk chose an order.
            paths = [part
                     for row in test_case.given_all("tree")
                     for part in row.replace('\t', ' ').split(' ') if part]
            walks_more_than_one_file = len([path for path in paths if '/' in path]) > 1

            if not walks_more_than_one_file:
                continue

            # `is not None` for the reference's ??, which falls through on null only. The two
            # agree here -- neither an absent nor an empty value contains anything -- but the
            # explicit test is what the port writes everywhere else.
            walk = test_case.get("walk")
            self.assertTrue(
                "sort=yes" in (walk if walk is not None else ""),
                test_case.describe("a case walking several files must pass --sort: a listing's "
                                   "order is unspecified without it, and this corpus compares "
                                   "order"))

    def test_target_lines_are_contiguous_last_and_of_one_kind(self):
        """
        Targets come last, in one run, and a case never mixes the two spellings.

        The line set test above ignores them, because a streamed run has no target at all.
        This is their guard: wherever they appear they are contiguous, they sit after every
        fixed line, and an item carries either a resolved target or a deferred pattern,
        never both (D13).
        """
        for test_case in _succeeding_cases():
            keys = [key for key, _ in test_case.expectations if key != "exit"]

            # List.FindIndex, which answers -1 when nothing matches; next() with a default
            # is the Python spelling that does not raise on the miss.
            first = next((index for index, key in enumerate(keys)
                          if key == "target" or key == "pattern"), -1)
            if first < 0:
                continue

            tail = keys[first:]

            self.assertTrue(
                all(key == "target" or key == "pattern" for key in tail),
                test_case.describe(
                    "target and pattern lines must come after every other plan line"))

            self.assertTrue(
                len(set(tail)) == 1,
                test_case.describe("a case states either targets or patterns, never both"))

    def test_no_two_cases_share_a_subject_and_a_filesystem(self):
        # Two cases with the same input and different expectations cannot both pass. The
        # input is argv *and* the tree it runs against: several cases reuse one command line
        # to show what a hidden entry or a symlink changes, and those are different runs
        # rather than a contradiction.
        seen = {}

        for test_case in plan_corpus.load():
            # The C# builds this with a StringBuilder; a list joined once is the Python
            # spelling of the same thing, and repeated str concatenation is what the
            # StringBuilder is there to avoid in both languages.
            parts = [test_case.subject]
            for key, value in test_case.given:
                parts.append("\n" + key + " " + value.strip())

            identity = "".join(parts)

            self.assertTrue(
                identity not in seen,
                test_case.describe("same command line and same filesystem as "
                                   + (str(seen[identity]) if identity in seen else "?")))

            seen[identity] = test_case

    def test_the_dry_run_switch_itself_is_exercised(self):
        # Every other case reaches the planner directly, so without this the advertised
        # switch could be missing entirely and the corpus still pass.
        #
        # Assert.Contains(collection, predicate) has no unittest counterpart; any() over a
        # generator is the same question.
        cases = plan_corpus.load()
        self.assertTrue(any("--dry-run" in case.subject for case in cases))
        self.assertTrue(any("--print-plan" in case.subject for case in cases))

    def test_the_corpus_covers_every_command(self):
        # Coverage guard: a new verb must arrive with at least one case.
        covered = set()
        for case in plan_corpus.load():
            args = plan_corpus.split_arguments(case.subject)
            if len(args) > 0:
                covered.add(args[0])

        for command in ArgumentTable.COMMANDS:
            self.assertTrue(command in covered, "no corpus case exercises: " + command)


class FixtureReaderTests(unittest.TestCase):
    """
    The shared fixture reader, which the C#, Java and C++17 suites each reimplement in
    about twenty lines.

    Every case the reference expects a System.IO.InvalidDataException from expects a
    ValueError here. Python has no InvalidDataException, and ValueError is what the standard
    library raises for a value of the right type whose content is wrong -- which is what a
    malformed fixture line is. assertRaises accepts a subclass where Assert.ThrowsAny does
    too, so the two are the same assertion.
    """

    def test_comments_and_blank_lines_are_ignored(self):
        lines = ["# a comment", "", "$ identify a.bin", "= exit 0"]

        self.assertEqual(1, len(fixture.parse("test.txt", lines)))

    def test_a_case_collects_its_expectations_in_order(self):
        lines = ["$ identify a.bin", "= command identify", "= exit 0"]

        parsed = fixture.parse("test.txt", lines)[0]

        self.assertEqual("identify a.bin", parsed.subject)
        self.assertEqual(2, len(parsed.expectations))
        self.assertEqual("identify", parsed.get("command"))

    def test_a_repeated_key_keeps_every_value(self):
        # How multi-target and multi-argument cases are expressed.
        lines = ["$ convert", "= target out/a.pdf", "= target out/b.pdf"]

        self.assertEqual(["out/a.pdf", "out/b.pdf"],
                         fixture.parse("test.txt", lines)[0].get_all("target"))

    def test_an_error_expectation_is_recognised(self):
        lines = ["$ convert --to pdxf a.docx", "= error E_UNKNOWN_FORMAT", "= exit 2"]

        self.assertEqual("E_UNKNOWN_FORMAT", fixture.parse("test.txt", lines)[0].error)

    def test_yes_and_no_expectations_parse_as_booleans(self):
        lines = ["$ pdf", "= single-file yes", "= hidef no"]

        parsed = fixture.parse("test.txt", lines)[0]

        self.assertTrue(parsed.get_bool("single-file"))
        self.assertFalse(parsed.get_bool("hidef"))

    def test_anything_other_than_yes_or_no_is_rejected(self):
        lines = ["$ pdf", "= single-file true"]

        with self.assertRaises(ValueError):
            fixture.parse("test.txt", lines)[0].get_bool("single-file")

    def test_an_empty_subject_is_allowed(self):
        # formats.txt uses one to assert that an empty --to is rejected.
        lines = ["$", "= error E_UNKNOWN_FORMAT"]

        self.assertEqual("", fixture.parse("test.txt", lines)[0].subject)

    def test_an_expectation_before_any_case_is_an_error(self):
        with self.assertRaises(ValueError):
            fixture.parse("test.txt", ["= command convert"])

    def test_a_case_keeps_what_it_is_given_apart_from_what_it_expects(self):
        lines = ["$ convert a.docx", "< tree a.docx", "= command convert"]

        parsed = fixture.parse("test.txt", lines)[0]

        self.assertEqual(["a.docx"], parsed.given_all("tree"))
        self.assertEqual(1, len(parsed.expectations))
        self.assertFalse(parsed.has("tree"))

    def test_what_a_case_is_given_comes_before_what_it_expects(self):
        # '<' describes the world before the run and '=' what the run must produce, so a
        # fixture that interleaved them would read as though the order meant something.
        with self.assertRaises(ValueError):
            fixture.parse("test.txt",
                          ["$ convert a.docx", "= command convert", "< tree a.docx"])

    def test_a_given_line_before_any_case_is_an_error(self):
        with self.assertRaises(ValueError):
            fixture.parse("test.txt", ["< tree a.docx"])

    def test_an_unrecognised_line_is_an_error(self):
        # Rather than being skipped, which would silently drop expectations.
        with self.assertRaises(ValueError):
            fixture.parse("test.txt", ["$ convert", "command convert"])

    def test_a_failure_message_names_the_file_the_line_and_the_whole_case(self):
        # The reason externalising these cases is affordable at all: a failure has to be
        # readable without opening the fixture.
        lines = ["# header", "$ pdf", "= kind canvas", "= ext pdf"]

        message = fixture.parse("formats.txt", lines)[0].describe("kind was text")

        self.assertIn("formats.txt:2", message)
        self.assertIn("$ pdf", message)
        self.assertIn("kind was text", message)
        self.assertIn("= kind canvas", message)
        self.assertIn("= ext pdf", message)

    def test_quoted_arguments_may_contain_spaces(self):
        self.assertEqual(
            ["convert", "--options", "BW_MODE=3;X=1", "a.docx"],
            plan_corpus.split_arguments('convert --options "BW_MODE=3;X=1" a.docx'))


def _failing_cases() -> List[fixture.FixtureCase]:
    """
    The cases that expect an error.

    The reference writes `.Where(c => c.Error != null)` at each of the four call sites; one
    named helper says it once. `is not None` and not truthiness, because an error code is a
    string and the empty one would read as absent.
    """
    return [case for case in plan_corpus.load() if case.error is not None]


def _succeeding_cases() -> List[fixture.FixtureCase]:
    """The cases that expect a plan."""
    return [case for case in plan_corpus.load() if case.error is None]


def _plan(args: List[str], file_system: IFileSystem) -> str:
    """
    Runs stages 1 to 3 -- parse, bind, plan -- against the filesystem the case is given,
    with the input walk feeding the planner.

    The walk runs, which is what makes `= inputs` and `= target` real assertions rather
    than an echo of the arguments: a case naming a directory states the files that come out
    of it, so --max-depth, --glob and --include-hidden are contract rather than one
    language's own tests.
    """
    app = CommandLineApp(RecordingConsole(), file_system)
    bound = app.bind(args)

    # Python has no object initialiser, so the context is assigned field by field. The
    # session is left None: nothing here opens a document, which is the whole reason these
    # run without a license.
    context = CommandContext()
    context.shared_options = bound.shared_options
    context.console = RecordingConsole()
    context.file_system = file_system

    return bound.command.describe_plan(context)


def _normalise(text: str) -> str:
    """
    Collapses runs of spaces so fixture alignment is not significant.

    str.strip() is parameterless here, matching the reference's Trim(), and deliberately
    not configuration.value_parsers.trim: this is fixture text the corpus authors aligned
    into columns, not a value a user typed, and the two ports must agree on the fixture
    rather than on the tool's own notion of whitespace.
    """
    while "  " in text:
        text = text.replace("  ", " ")

    return text.strip()


def _indent(text: str) -> str:
    return "\n".join("      " + line for line in text.split('\n'))


def _is_variable_key(key: str) -> bool:
    """
    Keys that are not a fixed plan line: exit, which is the process result, and the target
    keys, whose count varies by input and by format.
    """
    return key == "exit" or key == "target" or key == "pattern"


if __name__ == '__main__':
    unittest.main()
