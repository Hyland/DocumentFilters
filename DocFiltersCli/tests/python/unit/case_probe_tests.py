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
from typing import Iterable, List

from docfilters_cli.runtime import case_probe


class CaseProbeTests(unittest.TestCase):
    """
    Where the case question is asked, which is the part a real filesystem cannot check: on
    one volume every directory answers the same, so a probe aimed at the parent passes on
    every machine until someone converts into a mounted volume.
    """

    def test_asks_only_about_names_inside_the_directory(self):
        """
        The whole of the finding this answers: every path the probe asks about lies inside
        the directory being measured. Aimed at the parent -- which is what it used to do --
        an output under a mounted volume is measured against the filesystem holding the
        mount point, and two outputs differing only by case are planned as two files that
        turn out to be one.
        """
        asked: List[str] = []

        def exists(candidate: str) -> bool:
            asked.append(candidate)
            return False

        case_probe.measure("/mnt/ntfs", ["/mnt/ntfs/Report.txt"], exists)

        self.assertEqual(["/mnt/ntfs/rEPORT.TXT"], asked)

    def test_finding_the_flipped_spelling_means_case_insensitive(self):
        self.assertIs(False, case_probe.measure("out", ["out/Report.txt"], lambda candidate: True))

    def test_not_finding_it_means_case_sensitive(self):
        self.assertIs(True, case_probe.measure("out", ["out/Report.txt"], lambda candidate: False))

    def test_stops_at_the_first_entry_that_can_answer(self):
        """
        Lazily, and only as far as it has to be: the first name with a letter in it settles
        the question, and a directory of a hundred thousand files costs one entry.
        """
        enumerated: List[str] = []

        case_probe.measure("out", _watching(["out/2026", "out/Report.txt", "out/Other.txt"], enumerated),
                           lambda candidate: False)

        self.assertEqual(["out/2026", "out/Report.txt"], enumerated)

    def test_a_directory_with_nothing_to_say_answers_nothing(self):
        self.assertIsNone(case_probe.measure("out", [], lambda candidate: True))
        self.assertIsNone(case_probe.measure("out", ["out/2026", "out/4.1"], lambda candidate: True))

    def test_probes_inside_the_directory_being_measured(self):
        self.assertEqual("/mnt/ntfs/out/rEPORT.TXT",
                         case_probe.flipped_sibling("/mnt/ntfs/out", "/mnt/ntfs/out/Report.txt"))

    def test_never_probes_the_parent(self):
        """
        The defect this replaced: it flipped the directory's own name and looked for it in
        the parent, so an output under /mnt/ntfs measured /mnt -- a different filesystem
        with a different answer.
        """
        probe = case_probe.flipped_sibling("/mnt/ntfs", "/mnt/ntfs/Report.txt")

        self.assertTrue(probe.startswith("/mnt/ntfs/"), probe)
        self.assertNotEqual("/mnt/NTFS", probe)

    def test_accepts_a_bare_name_as_well_as_a_full_path(self):
        self.assertEqual("out/rEPORT.TXT", case_probe.flipped_sibling("out", "Report.txt"))

    def test_flips_every_letter_rather_than_lowercasing(self):
        self.assertEqual("out/rEPORT", case_probe.flipped_sibling("out", "Report"))
        self.assertEqual("out/report", case_probe.flipped_sibling("out", "REPORT"))

    def test_leaves_everything_that_is_not_an_ascii_letter_alone(self):
        """
        Written with escapes rather than the characters themselves so the test says the
        same thing whatever encoding reads this file. str.swapcase() would flip the
        e-acute too, which is the habit this pins.
        """
        self.assertEqual("out/q4-2026_R\u00e9SUM\u00e9.pdf",
                         case_probe.flipped_sibling("out", "Q4-2026_r\u00e9sum\u00e9.PDF"))

    def test_declines_a_name_with_no_letter_in_it(self):
        """
        A name with no letter to flip is the same name, which would make the probe ask
        whether the entry it already has exists -- always yes, and the directory would read
        as case-insensitive whatever it is. The caller moves on to the next entry instead.
        """
        self.assertIsNone(case_probe.flipped_sibling("out", "2026-01-31.4"))
        self.assertIsNone(case_probe.flipped_sibling("out", "_"))

    def test_reads_a_trailing_separator_as_part_of_neither_name(self):
        self.assertEqual("out/sUB", case_probe.flipped_sibling("out/", "out/Sub/"))

    def test_reads_a_windows_separator_the_same_way(self):
        self.assertEqual("C:/work/out/rEPORT.TXT",
                         case_probe.flipped_sibling("C:/work/out", "C:\\work\\out\\Report.txt"))


def _watching(entries: Iterable[str], seen: List[str]) -> Iterable[str]:
    """The C# iterator method: a generator, so measure's laziness is observable."""
    for entry in entries:
        seen.append(entry)
        yield entry


if __name__ == '__main__':
    unittest.main()
