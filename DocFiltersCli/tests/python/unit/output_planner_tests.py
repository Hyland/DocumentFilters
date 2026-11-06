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

from docfilters_cli.configuration.command_options import ConvertOptions
from docfilters_cli.configuration.shared_options import SharedOptions
from docfilters_cli.discovery.input_source import InputSource
from docfilters_cli.planning import output_planner
from docfilters_cli.planning.format_catalog import FormatCatalog
from docfilters_cli.planning.output_name_template import OutputNameTemplate
from docfilters_cli.planning.page_range import PageRange
from docfilters_cli.planning.run_plan import OutputCardinality, RunPlan, TargetPattern
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import ErrorCodes, NoInputsError, OutputConflictError
from fakes.in_memory_file_system import InMemoryFileSystem


class OutputPlannerTests(unittest.TestCase):
    """
    The output model. This is the highest-risk logic in the tool and the reason
    planning is a separate, pure stage.

    The rule underneath every test here: the meaning of -o never depends on --to.
    """

    def _plan(self, shared: SharedOptions, convert: ConvertOptions, inputs: List[InputSource],
              case_sensitive: bool = True) -> RunPlan:
        """
        output_planner.plan against a filesystem that tells names apart by case, which is
        what these cases assume unless one says otherwise.
        """
        file_system = InMemoryFileSystem()
        file_system.case_sensitive = case_sensitive
        return output_planner.plan(shared, convert, inputs, file_system)

    def _inputs(self, *paths: str) -> List[InputSource]:
        sources = []
        for path in paths:
            # The relative path is the filename alone, so these inputs model a
            # flattened walk: the paths are forward-slash by construction, so the
            # split is on a literal '/' rather than anything the host supplies.
            slash = path.rfind('/')
            source = InputSource()
            source.path = path
            source.relative_path = path if slash < 0 else path[slash + 1:]
            source.root = None
            sources.append(source)

        return sources

    def _to_format(self, token: str, combine: bool = False) -> ConvertOptions:
        options = ConvertOptions()
        options.combine = combine
        options.format = FormatCatalog.resolve(token)
        return options

    # ---------------------------------------------------------------- cardinality

    def test_out_dir_with_many_inputs_gives_each_its_own_output(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"

        plan = self._plan(shared, self._to_format("pdf"), self._inputs("a.docx", "b.docx"))

        self.assertEqual(OutputCardinality.PER_INPUT, plan.cardinality)
        self.assertEqual(2, len(plan.items))
        self.assertEqual("out/a.pdf", plan.items[0].target)
        self.assertEqual("out/b.pdf", plan.items[1].target)

    def test_out_file_with_one_input_is_a_single_target(self):
        shared = SharedOptions()
        shared.output.out_file = "book.pdf"

        plan = self._plan(shared, self._to_format("pdf"), self._inputs("a.docx"))

        self.assertEqual(OutputCardinality.SINGLE_TARGET, plan.cardinality)
        self.assertEqual("book.pdf", plan.items[0].target)

    def test_combine_with_out_file_merges_every_input_into_one_document(self):
        shared = SharedOptions()
        shared.output.out_file = "book.pdf"

        plan = self._plan(shared, self._to_format("pdf", combine=True),
                          self._inputs("a.docx", "b.docx", "c.docx"))

        self.assertEqual(OutputCardinality.COMBINED, plan.cardinality)
        self.assertEqual("book.pdf", plan.combined_target)
        self.assertEqual(3, len(plan.items))

        # Items contribute to the shared target rather than owning one.
        for item in plan.items:
            self.assertIsNone(item.target)

    def test_stdout_is_its_own_cardinality(self):
        shared = SharedOptions()
        shared.output.use_stdout = True

        plan = self._plan(shared, self._to_format("pdf"), self._inputs("a.docx"))

        self.assertEqual(OutputCardinality.STREAM, plan.cardinality)

    # ------------------------------------------------- collisions and letter case

    def test_names_differing_only_in_case_collide_where_the_filesystem_folds_case(self):
        """
        Two names differing only in case are one file where the filesystem says so, and the
        planner must refuse rather than let the second output replace the first.

        This was silent data loss: the run exited 0, printed nothing, and left one file
        holding the second document's text. Nothing caught it because the suite compared
        planned paths ordinally, which is right on the filesystem the tests ran against and
        wrong on the one most users have.

        The corpus cannot hold this case -- it states the other answer, since its
        filesystem is case-sensitive and a shared corpus has to read the same everywhere.
        This is that half, and it is a unit test in each language rather than a contract
        case for the same reason.
        """
        shared = SharedOptions()
        shared.output.out_dir = "out"

        with self.assertRaises(OutputConflictError) as caught:
            self._plan(shared, self._to_format("text"), self._inputs("a/Report.docx", "b/report.docx"),
                       case_sensitive=False)

        self.assertEqual(ErrorCodes.OUTPUT_COLLISION, caught.exception.code)
        self.assertEqual(exit_code.OUTPUT_CONFLICT, caught.exception.exit_code)

    def test_names_differing_only_in_case_are_two_outputs_where_the_filesystem_is_case_sensitive(self):
        """
        The same pair is two outputs where the filesystem tells them apart, and refusing it
        would decline work the tool can do.
        """
        shared = SharedOptions()
        shared.output.out_dir = "out"

        plan = self._plan(shared, self._to_format("text"), self._inputs("a/Report.docx", "b/report.docx"),
                          case_sensitive=True)

        self.assertEqual(["out/Report.txt", "out/report.txt"], [item.target for item in plan.items])

    def test_two_inputs_with_the_same_name_always_collide(self):
        """An exact repeat collides whatever the filesystem says about case."""
        for case_sensitive in (True, False):
            with self.subTest(case_sensitive=case_sensitive):
                shared = SharedOptions()
                shared.output.out_dir = "out"

                with self.assertRaises(OutputConflictError) as caught:
                    self._plan(shared, self._to_format("text"), self._inputs("a/report.docx", "b/report.docx"),
                               case_sensitive=case_sensitive)

                self.assertEqual(ErrorCodes.OUTPUT_COLLISION, caught.exception.code)

    # ------------------------------------------------------------- the hard error

    def test_out_file_with_several_inputs_and_no_combine_is_a_hard_error(self):
        # The specification is explicit that this is an error and never a guess:
        # the tool must not silently pick one input or overwrite repeatedly.
        shared = SharedOptions()
        shared.output.out_file = "one.pdf"

        with self.assertRaises(OutputConflictError) as caught:
            self._plan(shared, self._to_format("pdf"), self._inputs("a.docx", "b.docx"))

        self.assertEqual(ErrorCodes.OUTFILE_MULTI_INPUT, caught.exception.code)
        self.assertEqual(exit_code.OUTPUT_CONFLICT, caught.exception.exit_code)

    def test_no_inputs_is_its_own_error_and_exit_code(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"

        with self.assertRaises(NoInputsError) as caught:
            self._plan(shared, self._to_format("pdf"), self._inputs())

        self.assertEqual(exit_code.NO_INPUTS_MATCHED, caught.exception.exit_code)

    # ------------------------------------------------- page fan-out is a separate axis

    def test_a_one_per_page_format_is_still_per_input(self):
        # One document producing five PNGs is not a different cardinality. Target
        # ownership and page count are independent questions.
        shared = SharedOptions()
        shared.output.out_dir = "out"

        plan = self._plan(shared, self._to_format("png"), self._inputs("a.docx", "b.docx"))

        self.assertEqual(OutputCardinality.PER_INPUT, plan.cardinality)

    def test_a_one_per_page_format_without_pages_defers_its_targets(self):
        # The filenames depend on the document's page count, which is unknown until
        # it is opened, so the planner records a pattern instead of paths.
        shared = SharedOptions()
        shared.output.out_dir = "out"

        plan = self._plan(shared, self._to_format("png"), self._inputs("a.docx"))

        self.assertTrue(plan.items[0].is_page_count_dependent)
        self.assertIsNone(plan.items[0].target)
        self.assertIsNotNone(plan.items[0].target_pattern)

    def test_a_one_per_page_format_defers_its_targets_even_with_explicit_pages(self):
        # --pages does not make the filenames knowable: the range is clamped to the
        # document's real page count, so --pages 1-2 on a one-page document writes
        # one file. Resolving both up front would let the planner reject a run
        # because a page-2 output already exists when page 2 is never written.
        shared = SharedOptions()
        shared.output.out_dir = "out"
        options = self._to_format("png")
        options.pages = PageRange.parse("1-2")

        plan = self._plan(shared, options, self._inputs("a.docx"))

        self.assertTrue(plan.items[0].is_page_count_dependent)
        self.assertIsNone(plan.items[0].target)
        self.assertIsNotNone(plan.items[0].target_pattern)

    def test_the_first_checked_target_is_the_first_selected_page_not_page_one(self):
        # With --pages 5-6 the first output is page 5, so that is what the
        # fail-fast existence check has to look at.
        pattern = TargetPattern()
        pattern.directory = "out"
        pattern.template = OutputNameTemplate.parse("{stem}-{page:04}.{ext}")
        pattern.extension = "png"

        self.assertEqual("out/a-0005.png", pattern.first_target("a.docx", PageRange.parse("5-6")))
        self.assertEqual("out/a-0001.png", pattern.first_target("a.docx", PageRange.ALL))

    def test_a_single_file_format_resolves_its_target_even_without_pages(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"

        plan = self._plan(shared, self._to_format("pdf"), self._inputs("a.docx"))

        self.assertFalse(plan.items[0].is_page_count_dependent)

    # ------------------------------------------------------------------ collisions

    def test_two_inputs_resolving_to_the_same_output_is_an_error(self):
        # A flattened walk of a/report.docx and b/report.docx would silently
        # overwrite. Only visible because the whole batch is planned at once.
        shared = SharedOptions()
        shared.output.out_dir = "out"

        with self.assertRaises(OutputConflictError) as caught:
            self._plan(shared, self._to_format("pdf"), self._inputs("a/report.docx", "b/report.docx"))

        self.assertEqual(ErrorCodes.OUTPUT_COLLISION, caught.exception.code)

    def test_collisions_are_detected_for_deferred_targets_too(self):
        # Page count is unknown for PNG, but the stems already collide, so this is
        # decidable from the pattern alone.
        shared = SharedOptions()
        shared.output.out_dir = "out"

        with self.assertRaises(OutputConflictError):
            self._plan(shared, self._to_format("png"), self._inputs("a/report.docx", "b/report.docx"))

    def test_same_stem_with_different_source_extensions_still_collides(self):
        # report.docx and report.pdf both become report.txt.
        shared = SharedOptions()
        shared.output.out_dir = "out"

        with self.assertRaises(OutputConflictError):
            self._plan(shared, self._to_format("text"), self._inputs("report.docx", "report.pdf"))

    def test_an_out_name_template_can_resolve_a_collision(self):
        # {srcext} distinguishes the two, so this must not error.
        shared = SharedOptions()
        shared.output.out_dir = "out"
        shared.output.name_template = OutputNameTemplate.parse("{stem}-{srcext}.{ext}")

        plan = self._plan(shared, self._to_format("text"), self._inputs("report.docx", "report.pdf"))

        self.assertEqual("out/report-docx.txt", plan.items[0].target)
        self.assertEqual("out/report-pdf.txt", plan.items[1].target)

    # -------------------------------------------------------------- existing files

    def test_an_existing_target_without_overwrite_is_an_error(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"
        file_system = InMemoryFileSystem().add_file("out/a.pdf", "old")

        plan = self._plan(shared, self._to_format("pdf"), self._inputs("a.docx"))

        with self.assertRaises(OutputConflictError) as caught:
            output_planner.verify_targets(plan, shared, file_system)

        self.assertEqual(ErrorCodes.OUTPUT_EXISTS, caught.exception.code)

    def test_overwrite_permits_an_existing_target(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"
        shared.output.overwrite = True
        file_system = InMemoryFileSystem().add_file("out/a.pdf", "old")

        plan = self._plan(shared, self._to_format("pdf"), self._inputs("a.docx"))

        # The assertion is that this returns: the C# has no Assert here either, because
        # the only observable outcome is the absence of an OutputConflictError.
        output_planner.verify_targets(plan, shared, file_system)

    def test_verification_checks_the_first_page_of_a_deferred_item(self):
        # Fail fast on the common case: re-running a conversion into a directory
        # that already holds its output.
        shared = SharedOptions()
        shared.output.out_dir = "out"
        file_system = InMemoryFileSystem().add_file("out/a-0001.png", "old")

        plan = self._plan(shared, self._to_format("png"), self._inputs("a.docx"))

        with self.assertRaises(OutputConflictError):
            output_planner.verify_targets(plan, shared, file_system)

    def test_verification_does_not_check_later_pages_of_a_deferred_item(self):
        # Page 2 may never be written, so its presence must not fail the run here.
        # Execution checks each page as it produces it.
        shared = SharedOptions()
        shared.output.out_dir = "out"
        file_system = InMemoryFileSystem().add_file("out/a-0002.png", "old")

        plan = self._plan(shared, self._to_format("png"), self._inputs("a.docx"))

        output_planner.verify_targets(plan, shared, file_system)

    # --------------------------------------------------------------- tree layout

    def test_the_source_tree_is_replicated_under_out_dir(self):
        shared = SharedOptions()
        shared.output.out_dir = "out"

        source = InputSource()
        source.path = "corpus/reports/q1.docx"
        source.relative_path = "reports/q1.docx"
        source.root = "corpus"
        inputs = [source]

        plan = self._plan(shared, self._to_format("pdf"), inputs)

        self.assertEqual("out/reports/q1.pdf", plan.items[0].target)


if __name__ == '__main__':
    unittest.main()
