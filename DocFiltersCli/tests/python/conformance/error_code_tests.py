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

from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import ErrorCodes

from conformance import plan_corpus


class ErrorCodeTests(unittest.TestCase):
    """
    The error vocabulary, checked against the fixtures that already state it.

    There is no separate fixture listing the codes: every failing case in plan-corpus.txt
    already states its code, its message and its exit code, and a second file repeating
    them would be a second place to keep in step.

    ErrorCodes.all_codes is a hand-written list beside the constants rather than
    reflection, because C++17 has none and these tests are translated into the other three
    languages rather than rewritten there.
    """

    CODES_NO_PLAN_CAN_PROVOKE = [
        ErrorCodes.INIT_FAILED,
        ErrorCodes.DOCUMENT_FAILED,
        ErrorCodes.LIMIT_EXCEEDED,
        ErrorCodes.INTERRUPTED,
        ErrorCodes.UNSAFE_OUTPUT
    ]
    """
    The codes no planning fixture can provoke.

    plan-corpus.txt resolves argv into a plan and opens nothing, so none of these can
    appear there: three need a live engine — the library has to fail to initialise, a
    document has to fail to open, or one has to outrun --timeout — the fourth needs a
    person pressing Ctrl+C, and the fifth needs an open container, because what a subfile
    is named is not knowable until one is. They are exercised all the same, by
    subfile_extraction_tests and CliTests against the fake engine and by tests/e2e/end2end.py against the real one, so what is listed here is the
    fact that they have a home, not an exemption from having one.

    Written out rather than derived, so that adding a code and forgetting to provoke it
    still fails: this list is the only way to opt out, and it is short enough to argue
    with.
    """

    def _codes_in_fixtures(self):
        """Every code named by a failing case in the corpus, plus those no case can reach."""
        codes = set(ErrorCodeTests.CODES_NO_PLAN_CAN_PROVOKE)

        for test_case in plan_corpus.load():
            if test_case.error is not None:
                codes.add(test_case.error)

        return codes

    def test_every_code_the_tool_declares_is_exercised_by_a_fixture(self):
        # The direction that matters: a code nobody provokes has no message, which
        # contradicts stderr being contract.
        exercised = self._codes_in_fixtures()

        for code in ErrorCodes.all_codes():
            self.assertIn(
                code, exercised,
                code + " is declared in ErrorCodes but no fixture case provokes it, so its message is untested")

    def test_every_code_a_fixture_names_is_one_the_tool_declares(self):
        # Catches a typo in a fixture, which would otherwise look like an implementation
        # bug in whichever language ran it next.
        declared = set(ErrorCodes.all_codes())

        for code in self._codes_in_fixtures():
            self.assertIn(code, declared, "a fixture names " + code + ", which ErrorCodes does not declare")

    def test_one_code_always_means_one_exit_code(self):
        # E_MUTUALLY_EXCLUSIVE appears in eight cases. Nothing else checks they agree, so
        # one drifting to a different exit code would reach three more implementations
        # before anyone noticed.
        seen = {}
        first_seen = {}

        for test_case in plan_corpus.load():
            if test_case.error is None:
                continue

            exit_value = plan_corpus.expected_exit(test_case)

            if test_case.error in seen:
                existing = seen[test_case.error]
                self.assertEqual(
                    existing, exit_value,
                    test_case.describe("{0} exits {1} here but {2} at {3}".format(
                        test_case.error, exit_value, existing, first_seen[test_case.error])))
            else:
                seen[test_case.error] = exit_value
                first_seen[test_case.error] = test_case

    def test_every_exit_code_a_fixture_states_is_one_the_table_defines(self):
        valid = {
            exit_code.SUCCESS, exit_code.PARTIAL_FAILURE, exit_code.USAGE_ERROR,
            exit_code.NO_INPUTS_MATCHED, exit_code.OUTPUT_CONFLICT,
            exit_code.INITIALISATION_FAILURE, exit_code.LIMIT_EXCEEDED, exit_code.INTERRUPTED
        }

        for test_case in plan_corpus.load():
            self.assertIn(plan_corpus.expected_exit(test_case), valid,
                          test_case.describe("states an exit code that is not in the §07 table"))


if __name__ == '__main__':
    unittest.main()
