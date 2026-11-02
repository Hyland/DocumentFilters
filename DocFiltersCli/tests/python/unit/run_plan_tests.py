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

from docfilters_cli.planning.output_name_template import OutputNameTemplate
from docfilters_cli.planning.page_range import PageRange
from docfilters_cli.planning.run_plan import PlannedItem, TargetPattern


def target_pattern(directory, template="{stem}-{page:04}.{ext}", extension="png") -> TargetPattern:
    pattern = TargetPattern()
    pattern.directory = directory
    pattern.template = OutputNameTemplate.parse(template)
    pattern.extension = extension
    return pattern


class RunPlanTests(unittest.TestCase):
    """
    PlannedItem.output_directory and the three TargetPattern methods.

    No C# counterpart: the reference has no RunPlanTests, and these members are reached
    only through OutputPlanner's own tests, which assert the plan they appear in rather
    than what they answer. They are the two places execution asks the plan a question
    directly -- "which directory do I create" and "which path do I write" -- so an
    unnoticed difference here is a wrong file on disk rather than a wrong plan dump.

    Written from the reference source, not from this port's behaviour.
    """

    # ------------------------------------------------------- PlannedItem.output_directory

    def test_a_patterned_item_reports_the_pattern_directory(self):
        item = PlannedItem()
        item.target_pattern = target_pattern("out/pages")

        self.assertEqual("out/pages", item.output_directory)

    def test_the_pattern_wins_even_when_a_target_is_also_set(self):
        # The reference tests TargetPattern first and returns without looking at Target,
        # so an item carrying both reports the pattern's directory. The planner never
        # builds such an item; this pins the precedence rather than the situation.
        item = PlannedItem()
        item.target_pattern = target_pattern("out/pages")
        item.target = "elsewhere/a.txt"

        self.assertEqual("out/pages", item.output_directory)

    def test_a_single_target_reports_the_directory_holding_it(self):
        item = PlannedItem()
        item.target = "out/sub/a.txt"

        self.assertEqual("out/sub", item.output_directory)

    def test_a_bare_filename_has_an_empty_directory_rather_than_none(self):
        # Path.GetDirectoryName("a.txt") is the empty string, not null, and the difference
        # matters: None means "this item has no file target at all", which a bare filename
        # is not. Execution creates nothing for either, so only the distinction is pinned.
        item = PlannedItem()
        item.target = "a.txt"

        self.assertEqual("", item.output_directory)

    def test_an_item_with_no_target_at_all_has_no_directory(self):
        # --stdout, or an item feeding a combined target.
        self.assertIsNone(PlannedItem().output_directory)

    def test_an_empty_target_is_treated_as_no_target(self):
        # string.IsNullOrEmpty in the reference, so the empty string is None's equal here.
        item = PlannedItem()
        item.target = ""

        self.assertIsNone(item.output_directory)

    def test_a_backslash_target_splits_the_same_way_on_every_host(self):
        # Path.GetDirectoryName splits on a backslash on Windows and not on Linux, so a
        # raw --out-file of this shape is the one case where the reference's answer depends
        # on the host. The port normalises first and picks the Windows answer for all four.
        item = PlannedItem()
        item.target = "C:\\tmp\\a.pdf"

        self.assertEqual("C:/tmp", item.output_directory)

    # ------------------------------------------------------------ TargetPattern.resolve

    def test_resolve_joins_the_directory_and_the_rendered_name(self):
        pattern = target_pattern("out/pages")

        self.assertEqual("out/pages/report-0007.png", pattern.resolve("in/report.pdf", 7))

    def test_resolve_omits_the_separator_when_there_is_no_directory(self):
        # Not "/report-0007.png", which would be an absolute path on Linux and so a
        # different file entirely from the one the user asked for.
        for directory in [None, ""]:
            with self.subTest(directory=directory):
                self.assertEqual("report-0007.png", target_pattern(directory).resolve("in/report.pdf", 7))

    def test_resolve_joins_the_directory_verbatim_and_normalises_only_the_name(self):
        # The reference is Directory + "/" + fileName with no normalisation of Directory,
        # and --out-dir reaches the plan exactly as typed -- only the replicated relative
        # portion is normalised, by the planner. So a backslash the user typed survives
        # into the path, identically in all four ports. The rendered filename is the half
        # that is guaranteed separator-free, because the template scanner rejects one.
        pattern = target_pattern("out\\pages")

        self.assertEqual("out\\pages/report-0001.png", pattern.resolve("in\\report.pdf", 1))

    # ------------------------------------------------------- TargetPattern.collision_key

    def test_the_collision_key_ignores_the_page_number(self):
        pattern = target_pattern("out")

        self.assertEqual(pattern.collision_key("in/report.pdf"), pattern.resolve("in/report.pdf", 0))
        self.assertNotEqual(pattern.resolve("in/report.pdf", 1), pattern.resolve("in/report.pdf", 2))

    def test_two_inputs_with_the_same_stem_share_a_collision_key(self):
        # The case that actually bites: a flattened walk holding two files of the same
        # stem collide on every page, and this says so without knowing either page count.
        pattern = target_pattern("out")

        self.assertEqual(pattern.collision_key("one/report.pdf"), pattern.collision_key("two/report.pdf"))

    def test_inputs_with_different_stems_do_not_share_a_collision_key(self):
        pattern = target_pattern("out")

        self.assertNotEqual(pattern.collision_key("in/report.pdf"), pattern.collision_key("in/summary.pdf"))

    def test_a_template_without_a_page_placeholder_still_keys_consistently(self):
        # A single-file format's template has no {page}, so every page renders the same
        # name and the key is simply that name.
        pattern = target_pattern("out", template="{stem}.{ext}", extension="txt")

        self.assertEqual("out/report.txt", pattern.collision_key("in/report.pdf"))

    # --------------------------------------------------------- TargetPattern.first_target

    def test_the_first_target_of_an_unrestricted_range_is_page_one(self):
        pattern = target_pattern("out")

        self.assertEqual("out/report-0001.png", pattern.first_target("in/report.pdf", PageRange.ALL))

    def test_the_first_target_of_an_explicit_range_is_its_first_page_not_page_one(self):
        # --pages 5-6 writes page 5 first. No document is open yet, so this is read
        # straight off the selection rather than checked against a page count.
        pattern = target_pattern("out")

        self.assertEqual("out/report-0005.png", pattern.first_target("in/report.pdf", PageRange.parse("5-6")))

    def test_the_first_target_of_an_unordered_range_is_the_lowest_page(self):
        # The selection is sorted and collapsed when it is parsed, so the first entry is
        # the lowest page however the user wrote the range.
        pattern = target_pattern("out")

        self.assertEqual("out/report-0003.png", pattern.first_target("in/report.pdf", PageRange.parse("9,3,7")))


if __name__ == '__main__':
    unittest.main()
