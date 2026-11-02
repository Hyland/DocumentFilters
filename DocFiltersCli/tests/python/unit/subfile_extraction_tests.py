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

from docfilters_cli.execution.subfile_extraction import SubfileExtraction


def _safe(name: str, replicate_tree: bool = True) -> str:
    return SubfileExtraction.resolve_safe_output_path("/out", name, replicate_tree)


class SubfileExtractionTests(unittest.TestCase):
    """
    Containment of extracted subfile names.

    A subfile's name comes from the container, not from the person running the tool. An
    archive can carry ../../../etc/passwd, an absolute path, or a Windows drive letter, and
    a tool that joins that name onto the output directory writes wherever the archive says
    — the vulnerability usually called zip slip.

    Everything here is a pure string operation, so the whole attack surface is testable
    without an archive, a document or a filesystem.
    """

    def _assert_inside_root(self, resolved: str) -> None:
        # The C#'s AssertInsideRoot is a private static helper. It cannot be one here:
        # unittest's assertions are methods on TestCase rather than statics on an Assert
        # class, so anything that asserts has to be a method. _safe, which only returns a
        # string, stays a module-level function as the reference's other helper is.
        normalised = resolved.replace('\\', '/')

        # Assert.StartsWith(..., StringComparison.Ordinal). A Python str comparison is
        # always ordinal, so there is nothing to opt into.
        self.assertTrue(normalised.startswith("/out/"),
                        "'{0}' does not start with the output root".format(resolved))

        # Traversal is a path *component* equal to "..", not any occurrence of the
        # two characters. Rejecting the substring would also reject the legitimate
        # filename "..data", which the test below requires to survive.
        for component in normalised.split('/'):
            self.assertTrue(component != "..",
                            "'{0}' contains a traversal component".format(resolved))
            self.assertTrue(component != ".",
                            "'{0}' contains an unresolved '.' component".format(resolved))

    # ------------------------------------------------------------ ordinary names

    def test_an_ordinary_name_lands_directly_in_the_output_directory(self):
        # The replace is the reference's own normalisation, kept rather than dropped even
        # though resolve_safe_output_path builds its answer with '/' by construction here:
        # it is what would catch a regression that reached for os.path.join and let the
        # host's separator into a path the container named.
        self.assertEqual("/out/readme.txt", _safe("readme.txt").replace('\\', '/'))

    def test_a_nested_name_replicates_its_structure(self):
        self.assertEqual("/out/reports/q1.docx",
                         _safe("reports/q1.docx").replace('\\', '/'))

    def test_flat_mode_keeps_only_the_final_component(self):
        self.assertEqual("/out/q1.docx",
                         _safe("reports/q1.docx", replicate_tree=False).replace('\\', '/'))

    # ---------------------------------------------------------------- traversal

    def test_a_traversing_name_cannot_escape_the_output_directory(self):
        for name in ["../outside.txt",
                     "../../outside.txt",
                     "../../../../../../etc/passwd",
                     "reports/../../outside.txt",
                     "./../outside.txt",
                     "a/b/../../../outside.txt"]:
            with self.subTest(name=name):
                self._assert_inside_root(_safe(name))

    def test_backslash_traversal_is_caught_too(self):
        # An archive written on Windows uses backslashes, and a tool that only
        # looked for forward slashes would let those through on Linux.
        #
        # The C# writes these as verbatim strings (@"..\outside.txt"); the Python spelling
        # of a string that takes no escapes is a raw literal, which is the same thing.
        for name in [r"..\outside.txt",
                     r"..\..\windows\system32\evil.dll",
                     r"reports\..\..\outside.txt"]:
            with self.subTest(name=name):
                self._assert_inside_root(_safe(name))

    def test_an_absolute_name_is_taken_as_relative(self):
        for name in ["/etc/passwd",
                     "/absolute/path.txt",
                     r"\windows\system32\evil.dll"]:
            with self.subTest(name=name):
                self._assert_inside_root(_safe(name))

    def test_a_drive_or_share_qualified_name_is_taken_as_relative(self):
        for name in [r"C:\windows\evil.dll",
                     "C:/windows/evil.dll",
                     r"\\server\share\evil.dll"]:
            with self.subTest(name=name):
                self._assert_inside_root(_safe(name))

    def test_traversal_is_neutralised_in_flat_mode_as_well(self):
        # Flat mode reduces to the final component, which happens to defuse most
        # traversal — but a name that is *only* traversal must not slip through.
        self._assert_inside_root(_safe("../../outside.txt", replicate_tree=False))

    # -------------------------------------------------------------- odd names

    def test_a_degenerate_name_still_produces_a_path_inside_the_root(self):
        for name in ["..", ".", "...", "", "   "]:
            with self.subTest(name=name):
                self._assert_inside_root(_safe(name))

    def test_a_name_that_is_only_separators_produces_a_path_inside_the_root(self):
        self._assert_inside_root(_safe("///"))

    def test_a_directory_named_dot_dot_something_is_not_traversal_and_survives(self):
        # "..data" is a legitimate filename; only the "." and ".." components are
        # traversal, and an over-eager filter would corrupt valid names.
        self.assertIn("..data", _safe("..data/x.txt"))
        self._assert_inside_root(_safe("..data/x.txt"))

    def test_two_subfiles_with_the_same_name_resolve_to_the_same_path(self):
        # Containment does not deduplicate: overwriting is the extract command's
        # business, through --overwrite. This just records that the guard is not
        # silently renaming things.
        self.assertEqual(_safe("readme.txt"), _safe("readme.txt"))


if __name__ == '__main__':
    unittest.main()
