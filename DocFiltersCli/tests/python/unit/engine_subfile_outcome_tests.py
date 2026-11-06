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

import datetime
import unittest
from typing import List, Optional

from docfilters_cli.configuration.command_options import ExtractOptions
from docfilters_cli.configuration.shared_options import SharedOptions
from docfilters_cli.engine.engine_exception import EngineError, EngineException
from docfilters_cli.execution.subfile_extraction import SubfileExtraction
from docfilters_cli.planning.name_matcher import NameMatcher
from fakes.fake_document_filters_engine import FakeDocument, FakeSubFile
from fakes.in_memory_file_system import InMemoryFileSystem


class EngineSubfileOutcomeTests(unittest.TestCase):
    """
    What the subfile walk does with each way a container can misbehave.

    The engine hands subfile names straight out of the container, which is
    attacker-controlled data in a way a file on disk is not. These check both halves: that
    a failure inside a container is handled, and that a hostile name cannot write outside
    the output directory (D17).
    """

    # ============================================ listing

    def test_every_entry_the_container_reports_is_listed(self):
        container = FakeDocument("bundle.zip")
        readme = FakeSubFile("readme.txt", "0001")
        readme.size = 4096
        readme.modified = datetime.datetime(2026, 1, 15, 9, 30, 0)
        container.sub_file_entries.append(readme)
        report = FakeSubFile("reports/q1.docx", "0002")
        report.size = 20480
        container.sub_file_entries.append(report)

        entries = SubfileExtraction(InMemoryFileSystem()).list_all(
            container, "bundle.zip", _shared())

        self.assertEqual(2, len(entries))
        self.assertEqual("bundle.zip>readme.txt", entries[0].display_name)
        self.assertEqual("2026-01-15T09:30:00Z", entries[0].modified)
        self.assertEqual(1, entries[0].depth)

    def test_a_container_with_no_entries_lists_nothing_rather_than_failing(self):
        entries = SubfileExtraction(InMemoryFileSystem()).list_all(
            FakeDocument("flat.txt"), "flat.txt", _shared())

        self.assertEqual([], entries)

    def test_a_container_that_fails_to_enumerate_surfaces_the_engine_error(self):
        # Nothing above the seam catches IGRException, so this has to arrive as an
        # EngineException for the command layer to turn into a DocumentError.
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                container = FakeDocument("bundle.zip")
                container.read_failure = error
                container.read_failure_on = "SubFiles"

                with self.assertRaises(EngineException) as caught:
                    SubfileExtraction(InMemoryFileSystem()).list_all(
                        container, "bundle.zip", _shared())

                self.assertEqual(error, caught.exception.error)
                self.assertEqual(int(error), caught.exception.code)

    # ============================================ nesting

    def test_nested_containers_are_walked_when_subfiles_are_enabled(self):
        inner = FakeDocument("inner.zip")
        inner.sub_file_entries.append(FakeSubFile("note.txt", "0001"))

        outer = FakeDocument("nested.zip")
        entry = FakeSubFile("inner.zip", "0001")
        entry.content = inner
        outer.sub_file_entries.append(entry)

        entries = SubfileExtraction(InMemoryFileSystem()).list_all(
            outer, "nested.zip", _shared(subfiles_enabled=True))

        self.assertEqual(2, len(entries))
        self.assertEqual("nested.zip>inner.zip", entries[0].display_name)
        self.assertEqual("nested.zip>inner.zip>note.txt", entries[1].display_name)
        self.assertEqual(2, entries[1].depth)

    def test_subfile_depth_stops_the_walk_where_it_says(self):
        deepest = FakeDocument("deepest.zip")
        deepest.sub_file_entries.append(FakeSubFile("buried.txt", "0001"))

        middle = FakeDocument("middle.zip")
        deepest_entry = FakeSubFile("deepest.zip", "0001")
        deepest_entry.content = deepest
        middle.sub_file_entries.append(deepest_entry)

        outer = FakeDocument("outer.zip")
        middle_entry = FakeSubFile("middle.zip", "0001")
        middle_entry.content = middle
        outer.sub_file_entries.append(middle_entry)

        entries = SubfileExtraction(InMemoryFileSystem()).list_all(
            outer, "outer.zip", _shared(subfiles_enabled=True, depth=2))

        # Depth 2 reaches middle.zip's own children and stops before their contents.
        self.assertEqual(2, len(entries))
        self.assertFalse(any(entry.name == "buried.txt" for entry in entries))

    def test_without_subfiles_enabled_only_the_top_level_is_reported(self):
        inner = FakeDocument("inner.zip")
        inner.sub_file_entries.append(FakeSubFile("note.txt", "0001"))

        outer = FakeDocument("nested.zip")
        entry = FakeSubFile("inner.zip", "0001")
        entry.content = inner
        outer.sub_file_entries.append(entry)

        entries = SubfileExtraction(InMemoryFileSystem()).list_all(
            outer, "nested.zip", _shared())

        self.assertEqual(1, len(entries))

    def test_an_entry_that_will_not_open_surfaces_the_engine_error(self):
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                outer = FakeDocument("nested.zip")
                entry = FakeSubFile("inner.zip", "0001")
                entry.open_failure = error
                outer.sub_file_entries.append(entry)

                with self.assertRaises(EngineException) as caught:
                    SubfileExtraction(InMemoryFileSystem()).list_all(
                        outer, "nested.zip", _shared(subfiles_enabled=True))

                self.assertEqual(error, caught.exception.error)

    # ============================================ extraction

    def test_extraction_writes_every_entry_that_passes_the_filters(self):
        container = FakeDocument("archive.zip")
        keep = FakeSubFile("keep.docx", "0001")
        drop = FakeSubFile("draft-notes.txt", "0002")
        container.sub_file_entries.append(keep)
        container.sub_file_entries.append(drop)

        # Compiled the way command_line_app compiles them when the argument is read,
        # so this tests the walk's use of a matcher rather than the binding of one.
        options = ExtractOptions()
        options.subfile_include = NameMatcher.from_globs(["*.docx"])

        written = SubfileExtraction(InMemoryFileSystem()).extract_all(
            container, "archive.zip", _shared(), options)

        self.assertEqual(1, len(written))
        self.assertEqual(1, len(keep.copied_to))
        self.assertEqual([], drop.copied_to)

    def test_an_entry_that_will_not_copy_surfaces_the_engine_error(self):
        for error in _every_engine_error():
            with self.subTest(error=error.name):
                container = FakeDocument("archive.zip")
                entry = FakeSubFile("broken.docx", "0001")
                entry.copy_failure = error
                container.sub_file_entries.append(entry)

                with self.assertRaises(EngineException) as caught:
                    SubfileExtraction(InMemoryFileSystem()).extract_all(
                        container, "archive.zip", _shared(), ExtractOptions())

                self.assertEqual(error, caught.exception.error)

    def test_a_hostile_entry_name_stays_inside_the_output_directory(self):
        """
        A container-supplied name can never write outside the output directory.

        D17. The name comes from the archive, not from the person running the tool, so it
        is attacker-controlled: traversal, an absolute path, a drive letter and a UNC share
        all have to be neutralised. Checked through the real extraction path rather than
        only against resolve_safe_output_path, so that a future caller bypassing it is
        caught here.
        """
        for hostile_name in ("../../outside.txt",
                             "..\\..\\outside.txt",
                             "/etc/passwd",
                             "C:\\Windows\\System32\\evil.dll",
                             "\\\\server\\share\\evil.dll",
                             "../",
                             ".."):
            with self.subTest(name=hostile_name):
                container = FakeDocument("archive.zip")
                entry = FakeSubFile(hostile_name, "0001")
                container.sub_file_entries.append(entry)

                written = SubfileExtraction(InMemoryFileSystem()).extract_all(
                    container, "archive.zip", _shared(), ExtractOptions())

                self.assertEqual(1, len(written))
                target = written[0]
                self.assertTrue(target.startswith("out/"),
                                "expected {0!r} to start with 'out/'".format(target))
                self.assertNotIn("..", target)
                self.assertEqual(1, len(entry.copied_to))
                self.assertEqual(target, entry.copied_to[0])

    def test_images_are_extracted_only_when_asked_for(self):
        container = FakeDocument("doc.docx")
        image = FakeSubFile("image1.png", "0001")
        container.image_entries.append(image)

        self.assertEqual([], SubfileExtraction(InMemoryFileSystem()).extract_all(
            container, "doc.docx", _shared(), ExtractOptions()))
        self.assertEqual([], image.copied_to)

        with_images = ExtractOptions()
        with_images.images = True
        written = SubfileExtraction(InMemoryFileSystem()).extract_all(
            container, "doc.docx", _shared(), with_images)

        self.assertEqual(1, len(written))
        self.assertEqual(1, len(image.copied_to))

    # ============================================ timestamps

    def test_a_container_that_records_no_timestamp_reports_none(self):
        # The binding gives DateTime.MinValue for an entry with no date; reporting
        # that as a date in year one would be a lie.
        self.assertIsNone(SubfileExtraction.format_timestamp(None))
        self.assertIsNone(SubfileExtraction.format_timestamp(datetime.datetime.min))
        self.assertEqual("2026-01-15T09:30:00Z", SubfileExtraction.format_timestamp(
            datetime.datetime(2026, 1, 15, 9, 30, 0)))


def _every_engine_error() -> List[EngineError]:
    """
    Every failure the engine can report of its own accord.

    EngineError.NONE is not a failure, and EngineError.CANCELLED is not the engine's: it is
    only ever raised because this tool asked, through the open callback, so the caller that
    asked translates it into its own error -- a --timeout reported as E_LIMIT_EXCEEDED
    rather than a document reported as broken. Sweeping it here would assert the opposite.

    The same axis engine_outcome_tests sweeps, and duplicated here for the same reason the
    reference duplicates its [MemberData] source: neither file is the other's to import,
    and a shared base class would put a test axis somewhere neither test names.
    """
    return [error for error in EngineError
            if error != EngineError.NONE and error != EngineError.CANCELLED]


def _shared(subfiles_enabled: bool = False, depth: Optional[int] = None) -> SharedOptions:
    shared = SharedOptions()
    shared.output.out_dir = "out"
    shared.subfiles.enabled = subfiles_enabled
    shared.subfiles.max_depth = depth
    return shared
