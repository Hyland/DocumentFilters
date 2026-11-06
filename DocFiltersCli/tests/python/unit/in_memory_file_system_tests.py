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

from fakes.in_memory_file_system import InMemoryFileSystem


class InMemoryFileSystemTests(unittest.TestCase):
    """
    The in-memory filesystem itself. Test infrastructure needs testing too: a fake
    that silently discards writes would make every conversion test above it
    meaningless.
    """

    def test_a_created_file_keeps_what_was_written_to_it(self):
        fs = InMemoryFileSystem()

        with fs.create_file("out/report.txt") as writer:
            writer.write(b"hello")

        self.assertEqual("hello", fs.read_text("out/report.txt"))

    def test_a_created_file_can_be_read_back_through_the_interface(self):
        fs = InMemoryFileSystem()

        with fs.create_file("out/report.txt") as writer:
            writer.write(b"hello")

        with fs.open_read("out/report.txt") as reader:
            self.assertEqual(b"hello", reader.read())

    def test_creating_a_file_creates_its_parent_directories(self):
        fs = InMemoryFileSystem()

        fs.create_file("out/deep/nested/report.txt").close()

        self.assertTrue(fs.directory_exists("out/deep/nested"))
        self.assertTrue(fs.file_exists("out/deep/nested/report.txt"))

    def test_listing_returns_immediate_children_only(self):
        fs = InMemoryFileSystem().add_file("a/b.txt").add_file("a/c/d.txt")

        # As a set: the fake returns children in a deliberately unhelpful order so that
        # a walk which ignores --sort cannot pass the sorting tests by accident.
        # Asserting a sequence here would be asserting that anti-feature rather than
        # the property this test is named for.
        self.assertEqual(["a/b.txt", "a/c"], sorted(fs.list_directory("a")))

    def test_paths_are_normalised_so_tests_read_the_same_on_every_platform(self):
        fs = InMemoryFileSystem().add_file("a\\b.txt")

        self.assertTrue(fs.file_exists("a/b.txt"))

    def test_a_link_is_a_real_directory_entry_so_a_walk_can_find_it(self):
        # Without this the walk never sees the link, and every test about following one
        # passes without exercising anything.
        fs = InMemoryFileSystem().add_file("corpus/a.txt").mark_symlink("corpus/link", "target")

        self.assertIn("corpus/link", fs.list_directory("corpus"))
        self.assertTrue(fs.is_symbolic_link("corpus/link"))

    def test_listing_through_a_link_yields_the_targets_children_under_the_link_path(self):
        fs = InMemoryFileSystem().add_file("shared/x.docx").mark_symlink("corpus/link", "shared")

        self.assertEqual(["corpus/link/x.docx"], fs.list_directory("corpus/link"))

    def test_a_link_to_an_ancestor_produces_an_endless_supply_of_distinct_paths(self):
        # The property that makes path-based cycle detection fail, reproduced in the
        # fake so the walk's identity-based detection has something real to solve.
        fs = InMemoryFileSystem().add_file("corpus/a.txt").mark_symlink("corpus/loop", "corpus")

        self.assertIn("corpus/loop/a.txt", fs.list_directory("corpus/loop"))
        self.assertIn("corpus/loop/loop", fs.list_directory("corpus/loop"))
        self.assertIn("corpus/loop/loop/loop", fs.list_directory("corpus/loop/loop"))

    def test_two_links_to_one_directory_share_an_identity(self):
        fs = (InMemoryFileSystem()
              .add_file("shared/x.docx")
              .mark_symlink("corpus/one", "shared")
              .mark_symlink("corpus/two", "shared"))

        self.assertEqual(fs.get_directory_identity("corpus/one"), fs.get_directory_identity("corpus/two"))
        self.assertNotEqual(fs.get_directory_identity("corpus/one"), fs.get_directory_identity("corpus"))


if __name__ == '__main__':
    unittest.main()
