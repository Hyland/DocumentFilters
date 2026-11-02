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

from docfilters_cli.configuration.shared_options import InputOptions
from docfilters_cli.discovery.input_enumerator import InputEnumerator
from docfilters_cli.planning.name_matcher import NameMatcher
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import CliError, NoInputsError
from fakes.in_memory_file_system import InMemoryFileSystem
from fakes.recording_console import RecordingConsole


class InputEnumeratorTests(unittest.TestCase):
    """
    The filesystem walk: which files a command line actually selects.

    Runs entirely against InMemoryFileSystem, so these are ordinary fast unit tests
    despite being about the filesystem. Directory depth here is entirely separate from
    subfile depth, which is recursion into documents nested inside other documents.
    """

    @staticmethod
    def _tree():
        return (InMemoryFileSystem()
                .add_file("corpus/a.docx")
                .add_file("corpus/b.pdf")
                .add_file("corpus/reports/q1.docx")
                .add_file("corpus/reports/q2.docx")
                .add_file("corpus/reports/archive/old.docx")
                .add_file("corpus/notes.txt"))

    @staticmethod
    def _walk(fs, options):
        enumerator = InputEnumerator(fs, RecordingConsole())
        return [source.path for source in enumerator.enumerate(options)]

    @staticmethod
    def _from(*paths):
        options = InputOptions()
        options.paths.extend(paths)
        return options

    # -------------------------------------------------------------------------- depth

    def test_a_directory_is_walked_recursively_by_default(self):
        # Matching fd and ripgrep rather than grep and cp: a directory handed to a batch
        # converter is a corpus to walk.
        found = self._walk(self._tree(), self._from("corpus"))

        self.assertIn("corpus/a.docx", found)
        self.assertIn("corpus/reports/q1.docx", found)
        self.assertIn("corpus/reports/archive/old.docx", found)

    def test_no_recurse_stays_in_the_named_directory(self):
        options = self._from("corpus")
        options.no_recurse_was_given = True

        found = self._walk(self._tree(), options)

        self.assertIn("corpus/a.docx", found)
        self.assertNotIn("corpus/reports/q1.docx", found)

    def test_max_depth_one_is_the_same_as_no_recurse(self):
        options = self._from("corpus")
        options.max_depth = 1

        self.assertNotIn("corpus/reports/q1.docx", self._walk(self._tree(), options))

    def test_max_depth_two_reaches_one_level_of_subdirectory(self):
        options = self._from("corpus")
        options.max_depth = 2

        found = self._walk(self._tree(), options)

        self.assertIn("corpus/reports/q1.docx", found)
        self.assertNotIn("corpus/reports/archive/old.docx", found)

    # ----------------------------------------------------------------- explicit files

    def test_an_explicit_file_is_taken_as_given(self):
        self.assertEqual(["corpus/a.docx"], self._walk(self._tree(), self._from("corpus/a.docx")))

    def test_files_and_directories_can_be_mixed_and_keep_their_order(self):
        found = self._walk(self._tree(), self._from("corpus/notes.txt", "corpus/reports"))

        self.assertEqual("corpus/notes.txt", found[0])
        self.assertIn("corpus/reports/q1.docx", found)

    def test_an_explicit_file_is_not_subject_to_the_name_matcher(self):
        # Naming a file directly is an instruction, not a suggestion; a glob filters what
        # a walk discovers.
        options = self._from("corpus/notes.txt")
        options.matcher = NameMatcher.from_globs(["**/*.docx"])

        self.assertEqual(["corpus/notes.txt"], self._walk(self._tree(), options))

    # --------------------------------------------------------------------- selection

    def test_a_glob_filters_what_the_walk_discovers(self):
        options = self._from("corpus")
        options.matcher = NameMatcher.from_globs(["**/*.docx"])

        found = self._walk(self._tree(), options)

        self.assertIn("corpus/a.docx", found)
        self.assertNotIn("corpus/b.pdf", found)
        self.assertNotIn("corpus/notes.txt", found)

    def test_an_exclusion_removes_a_subtree(self):
        options = self._from("corpus")
        options.matcher = NameMatcher.from_globs(["**/*.docx", "!**/archive/*"])

        found = self._walk(self._tree(), options)

        self.assertIn("corpus/reports/q1.docx", found)
        self.assertNotIn("corpus/reports/archive/old.docx", found)

    # ------------------------------------------------------------------------ hidden

    def test_hidden_entries_are_skipped_by_default(self):
        fs = self._tree().add_file("corpus/.secret.docx").mark_hidden("corpus/.secret.docx")

        self.assertNotIn("corpus/.secret.docx", self._walk(fs, self._from("corpus")))

    def test_include_hidden_brings_them_back(self):
        fs = self._tree().add_file("corpus/.secret.docx").mark_hidden("corpus/.secret.docx")
        options = self._from("corpus")
        options.include_hidden = True

        self.assertIn("corpus/.secret.docx", self._walk(fs, options))

    def test_a_hidden_directory_hides_everything_under_it(self):
        fs = self._tree().add_file("corpus/.git/config.docx").mark_hidden("corpus/.git")

        self.assertNotIn("corpus/.git/config.docx", self._walk(fs, self._from("corpus")))

    # ---------------------------------------------------------------------- symlinks

    def test_a_symlinked_directory_is_not_entered_by_default(self):
        # Following by default risks walking in a circle.
        fs = self._tree().add_file("corpus/link/x.docx").mark_symlink("corpus/link")

        self.assertNotIn("corpus/link/x.docx", self._walk(fs, self._from("corpus")))

    def test_follow_enters_symlinked_directories(self):
        fs = self._tree().add_file("corpus/link/x.docx").mark_symlink("corpus/link")
        options = self._from("corpus")
        options.follow_symlinks = True

        self.assertIn("corpus/link/x.docx", self._walk(fs, options))

    def test_follow_terminates_when_a_link_points_at_an_ancestor(self):
        # The reason the walk tracks directory identity rather than path strings: a link
        # back to its own parent produces an endless supply of distinct paths —
        # corpus/loop/loop/loop/… — so a visited-paths set never closes the cycle.
        fs = self._tree().mark_symlink("corpus/loop", "corpus")
        options = self._from("corpus")
        options.follow_symlinks = True

        found = self._walk(fs, options)

        self.assertFalse(any("loop/loop" in path for path in found))

    def test_follow_visits_a_shared_directory_once(self):
        # Two links to the same place is not a cycle, but it is still one directory:
        # processing it twice would convert every document in it twice.
        fs = (self._tree()
              .add_file("shared/x.docx")
              .mark_symlink("corpus/one", "shared")
              .mark_symlink("corpus/two", "shared"))

        options = self._from("corpus")
        options.follow_symlinks = True

        found = self._walk(fs, options)

        self.assertEqual(1, len([path for path in found if path.endswith("x.docx")]))

    # ---------------------------------------------------------------------------- sort

    def test_sort_orders_the_walk(self):
        options = self._from("corpus")
        options.sort = True

        found = self._walk(self._tree(), options)

        # Python compares strings by code point, which is what StringComparer.Ordinal
        # asks for in the C#.
        self.assertEqual(sorted(found), found)

    def test_without_sort_the_walk_follows_the_filesystem_order(self):
        # The counterpart that makes the test above meaningful: the fake returns entries
        # unordered, so if --sort were ignored the sorted assertion would still have to
        # fail somewhere.
        found = self._walk(self._tree(), self._from("corpus"))

        self.assertNotEqual(sorted(found), found)

    # ---------------------------------------------------------------------- files-from

    def test_files_from_reads_a_list(self):
        fs = self._tree().add_file("list.txt", "corpus/a.docx\ncorpus/b.pdf\n")
        options = InputOptions()
        options.files_from = "list.txt"

        self.assertEqual(["corpus/a.docx", "corpus/b.pdf"], self._walk(fs, options))

    def test_files_from_ignores_blank_lines(self):
        fs = self._tree().add_file("list.txt", "corpus/a.docx\n\n\ncorpus/b.pdf\n")
        options = InputOptions()
        options.files_from = "list.txt"

        self.assertEqual(["corpus/a.docx", "corpus/b.pdf"], self._walk(fs, options))

    def test_files_from_dash_reads_standard_input(self):
        # "-" is not a path, so it goes through the console seam rather than the
        # filesystem — otherwise a test could not supply it.
        console = RecordingConsole()
        console.input_lines.append("corpus/a.docx")
        console.input_lines.append("corpus/b.pdf")

        enumerator = InputEnumerator(self._tree(), console)
        options = InputOptions()
        options.files_from = "-"

        found = enumerator.enumerate(options)

        self.assertEqual(["corpus/a.docx", "corpus/b.pdf"], [source.path for source in found])

    def test_files_from_combines_with_positional_paths(self):
        fs = self._tree().add_file("list.txt", "corpus/b.pdf\n")
        options = self._from("corpus/a.docx")
        options.files_from = "list.txt"

        found = self._walk(fs, options)

        self.assertIn("corpus/a.docx", found)
        self.assertIn("corpus/b.pdf", found)

    # ------------------------------------------------------------------ relative paths

    def test_a_walked_file_records_its_path_relative_to_the_root(self):
        # This is what lets --out-dir replicate the source tree.
        enumerator = InputEnumerator(self._tree(), RecordingConsole())

        matches = [source for source in enumerator.enumerate(self._from("corpus"))
                   if source.path == "corpus/reports/q1.docx"]
        self.assertEqual(1, len(matches))
        found = matches[0]

        self.assertEqual("reports/q1.docx", found.relative_path)
        self.assertEqual("corpus", found.root)

    def test_an_explicit_file_has_no_root_and_is_relative_to_itself(self):
        enumerator = InputEnumerator(self._tree(), RecordingConsole())

        found = enumerator.enumerate(self._from("corpus/a.docx"))
        self.assertEqual(1, len(found))

        self.assertEqual("a.docx", found[0].relative_path)
        self.assertIsNone(found[0].root)

    def test_a_backslash_path_still_reduces_to_a_bare_filename(self):
        # A directory walk always yields forward-slash paths (_walk_into builds them from
        # IFileSystem.list_directory), but an explicit argument is whatever the user typed
        # on their own shell — backslashes on Windows. Reproduces the real bug:
        # "C:\tmp\a.pdf" was kept whole as relative_path because the old file-name-of only
        # recognised '/', so --out-dir nested the source drive underneath itself instead of
        # flattening to "a.pdf".
        enumerator = InputEnumerator(self._tree(), RecordingConsole())

        found = enumerator.enumerate(self._from("corpus\\a.docx"))
        self.assertEqual(1, len(found))

        self.assertEqual("a.docx", found[0].relative_path)
        self.assertIsNone(found[0].root)

    # ------------------------------------------------------------------------- nothing

    def test_matching_nothing_is_its_own_error(self):
        options = self._from("corpus")
        options.matcher = NameMatcher.from_globs(["**/*.nonexistent"])

        with self.assertRaises(NoInputsError) as caught:
            self._walk(self._tree(), options)

        self.assertEqual(exit_code.NO_INPUTS_MATCHED, caught.exception.exit_code)

    def test_an_empty_directory_matches_nothing(self):
        fs = InMemoryFileSystem().add_directory("empty")

        with self.assertRaises(NoInputsError):
            self._walk(fs, self._from("empty"))

    def test_a_path_that_does_not_exist_is_an_error(self):
        with self.assertRaises(CliError):
            self._walk(self._tree(), self._from("corpus/missing.docx"))

    def test_a_directory_that_cannot_be_read_is_reported_rather_than_skipped(self):
        fs = InMemoryFileSystem().add_file("corpus/a.docx").add_file("corpus/locked/b.docx")
        fs.unreadable.add("corpus/locked")

        with self.assertRaises(NoInputsError) as caught:
            self._walk(fs, self._from("corpus"))

        self.assertEqual("corpus/locked: could not be read", caught.exception.message)
        self.assertEqual(exit_code.NO_INPUTS_MATCHED, caught.exception.exit_code)


if __name__ == '__main__':
    unittest.main()
