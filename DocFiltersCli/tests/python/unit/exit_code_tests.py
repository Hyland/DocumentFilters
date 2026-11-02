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
from docfilters_cli.runtime.cli_error import (DocumentError, ErrorCodes, InitialisationError,
                                              LimitExceededError, NoInputsError,
                                              OutputConflictError, UsageError)
from docfilters_cli.runtime.interruption import RunInterruptedError


class ExitCodeTests(unittest.TestCase):
    """
    The exit code table, which every implementation shares.

    isys_doc2text returns only 0 or 1, and returns 0 even on a usage error, so there
    was no meaningful parity to preserve beyond "non-zero means trouble". These values
    are the agreed replacement.
    """

    def test_the_agreed_values_are_stable(self):
        self.assertEqual(0, exit_code.SUCCESS)
        self.assertEqual(1, exit_code.PARTIAL_FAILURE)
        self.assertEqual(2, exit_code.USAGE_ERROR)
        self.assertEqual(3, exit_code.NO_INPUTS_MATCHED)
        self.assertEqual(4, exit_code.OUTPUT_CONFLICT)
        self.assertEqual(5, exit_code.INITIALISATION_FAILURE)
        self.assertEqual(6, exit_code.LIMIT_EXCEEDED)
        self.assertEqual(130, exit_code.INTERRUPTED)

    def test_a_usage_error_exits_two(self):
        error = UsageError(ErrorCodes.UNKNOWN_ARGUMENT, "unknown option --nonsense")

        self.assertEqual(exit_code.USAGE_ERROR, error.exit_code)

    def test_no_inputs_exits_three(self):
        self.assertEqual(exit_code.NO_INPUTS_MATCHED, NoInputsError("nothing matched").exit_code)

    def test_an_output_conflict_exits_four(self):
        error = OutputConflictError(ErrorCodes.OUTFILE_MULTI_INPUT, "several inputs, one --out-file")

        self.assertEqual(exit_code.OUTPUT_CONFLICT, error.exit_code)

    def test_an_initialisation_failure_exits_five(self):
        self.assertEqual(exit_code.INITIALISATION_FAILURE, InitialisationError("no license").exit_code)

    def test_a_limit_being_exceeded_exits_six(self):
        self.assertEqual(exit_code.LIMIT_EXCEEDED, LimitExceededError("--max-time passed").exit_code)

    def test_an_interruption_exits_one_hundred_and_thirty(self):
        self.assertEqual(exit_code.INTERRUPTED, RunInterruptedError().exit_code)

    def test_a_single_document_failure_maps_to_partial_failure(self):
        error = DocumentError("a.docx", "could not open")

        self.assertEqual(exit_code.PARTIAL_FAILURE, error.exit_code)

    def test_every_error_carries_a_machine_readable_code(self):
        # The corpus matches on the code, not the prose, so the message can be improved
        # without rewriting every fixture.
        self.assertEqual(ErrorCodes.UNKNOWN_ARGUMENT, UsageError(ErrorCodes.UNKNOWN_ARGUMENT, "x").code)
        self.assertEqual(ErrorCodes.NO_INPUTS, NoInputsError("x").code)
        self.assertEqual(ErrorCodes.DOCUMENT_FAILED, DocumentError("a", "x").code)

    def test_a_document_error_remembers_which_file_failed(self):
        error = DocumentError("corpus/a.docx", "could not open")

        self.assertEqual("corpus/a.docx", error.path)


if __name__ == '__main__':
    unittest.main()
