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

from docfilters_cli import program
from docfilters_cli.engine.engine_exception import EngineError
from docfilters_cli.engine.i_document_filters_engine import EngineFormat
from docfilters_cli.runtime import exit_code
from docfilters_cli.runtime.cli_error import ErrorCodes, LimitExceededError
from docfilters_cli.runtime.interruption import Interruption
from cli.cli_harness import CliHarness
from fakes.fake_document_filters_engine import FakeSubFile
from fakes.recording_console import RecordingConsole


class CliTests(unittest.TestCase):
    """
    Whole command lines, argv to exit code.

    These are about the wiring, not the rules. Every rule here is already pinned by the
    conformance corpus or a unit test; what is new is that the path from program.run
    through dispatch to a command's execute is exercised at all, which it was not before.
    """

    def _single(self, lines: List[str]) -> str:
        """
        xUnit's Assert.Single: one element, and hand it back.

        A method rather than a module function because it asserts, and because the
        failure message wants to say what was there instead.
        """
        self.assertEqual(1, len(lines), "expected exactly one line, got {0!r}".format(lines))
        return lines[0]

    def _assert_starts_with(self, prefix: str, line: str) -> None:
        """xUnit's Assert.StartsWith, expected first, with the line in the message."""
        self.assertTrue(line.startswith(prefix),
                        "expected {0!r} to start with {1!r}".format(line, prefix))

    # ============================================ exit codes out of the front door

    def test_a_clean_conversion_exits_zero_and_says_nothing(self):
        cli = CliHarness().with_document("a.docx", "hello")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "text", "--out-dir", "out", "a.docx"))
        self.assertEqual([], cli.errors)

        # Silence means success: a clean run prints nothing unless asked.
        self.assertEqual([], cli.out)

    def test_a_usage_error_exits_two_and_names_the_problem_on_stderr(self):
        cli = CliHarness().with_document("a.docx")

        self.assertEqual(exit_code.USAGE_ERROR,
                         cli.run("convert", "--to", "nonsense", "--out-dir", "out", "a.docx"))

        line = self._single(cli.errors)
        self._assert_starts_with(ErrorCodes.UNKNOWN_FORMAT + ": ", line)
        self.assertIn("nonsense", line)

    def test_an_unknown_command_exits_two(self):
        cli = CliHarness()

        self.assertEqual(exit_code.USAGE_ERROR, cli.run("nonsense", "a.docx"))
        self.assertTrue(any(line.startswith(ErrorCodes.UNKNOWN_COMMAND + ": ")
                            for line in cli.errors))

    def test_naming_no_input_exits_three(self):
        cli = CliHarness()

        self.assertEqual(exit_code.NO_INPUTS_MATCHED,
                         cli.run("convert", "--to", "text", "--out-dir", "out"))
        self.assertTrue(any(line.startswith(ErrorCodes.NO_INPUTS + ": ") for line in cli.errors))

    def test_an_output_conflict_exits_four(self):
        cli = CliHarness() \
            .with_document("a.docx") \
            .with_document("b.docx")

        self.assertEqual(exit_code.OUTPUT_CONFLICT,
                         cli.run("convert", "--to", "pdf", "--out-file", "one.pdf",
                                 "a.docx", "b.docx"))
        self.assertTrue(any(line.startswith(ErrorCodes.OUTFILE_MULTI_INPUT + ": ")
                            for line in cli.errors))

    def test_one_bad_document_among_good_ones_exits_one(self):
        cli = CliHarness() \
            .with_document("good.docx", "text") \
            .with_unreadable_document("bad.docx", EngineError.FILE_CORRUPT)

        self.assertEqual(exit_code.PARTIAL_FAILURE,
                         cli.run("convert", "--to", "text", "--out-dir", "out",
                                 "good.docx", "bad.docx"))

    # ============================================ the two stderr shapes

    def test_a_per_document_failure_is_prefixed_by_its_input(self):
        """
        A per-document failure carries the input it belongs to; a run-level failure has no
        input to name. Neither shape was checked anywhere before, because nothing drove the
        whole path; these two tests are now their only authority.
        """
        cli = CliHarness() \
            .with_document("good.docx", "text") \
            .with_unreadable_document("bad.docx", EngineError.FILE_CORRUPT)

        cli.run("convert", "--to", "text", "--out-dir", "out", "good.docx", "bad.docx")

        line = self._single(cli.errors)
        self._assert_starts_with("bad.docx: " + ErrorCodes.DOCUMENT_FAILED + ": ", line)

    def test_a_per_document_limit_is_prefixed_like_any_other_per_document_failure(self):
        """
        A limit that applies to one document is per-document even though it is not a
        DocumentError -- it exits 6, not 1. It names its input through CliError.path rather
        than by folding the filename into its own message, so the reporter puts it in the
        same place as every other per-document failure.
        """
        error = LimitExceededError("--timeout of 1s exceeded", "huge.pdf")
        console = RecordingConsole()

        code = program.report_error(error, console)

        self.assertEqual(exit_code.LIMIT_EXCEEDED, code)
        self.assertEqual("huge.pdf: E_LIMIT_EXCEEDED: --timeout of 1s exceeded",
                         self._single(console.error_lines))

    def test_a_run_wide_limit_names_no_input(self):
        # --max-time and --max-memory bound the whole run, so there is nothing to name.
        console = RecordingConsole()

        program.report_error(LimitExceededError("--max-time of 60s exceeded"), console)

        self.assertEqual("E_LIMIT_EXCEEDED: --max-time of 60s exceeded",
                         self._single(console.error_lines))

    def test_a_run_level_failure_has_no_input_to_name(self):
        cli = CliHarness().with_document("a.docx")

        cli.run("convert", "--to", "text", "--out-file", "a.txt", "--out-dir", "out", "a.docx")

        line = self._single(cli.errors)
        self._assert_starts_with(ErrorCodes.MUTUALLY_EXCLUSIVE + ": ", line)

    # ============================================ canvas formats to stdout

    def test_a_canvas_format_can_stream_to_stdout(self):
        # Reproduces a real bug: --stdout resolves to OutputCardinality.STREAM,
        # which leaves both PlannedItem.target and target_pattern None. Convert's
        # own branches only ever expect one or the other, so the per-page branch
        # ran with a None target_pattern and threw a bare AttributeError,
        # reported as "E_DOCUMENT_FAILED: 'NoneType' object has no attribute ..."
        # -- the exact symptom, with none of the detail.
        cli = CliHarness().with_document("a.docx", "text")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "pdf", "--stdout", "a.docx"))

        self.assertEqual([], cli.errors)
        self.assertTrue(any(call.startswith("MakeCanvas(<stream>,") for call in cli.engine.calls))
        self.assertFalse(any(call.startswith("MakeCanvas(a.pdf,") for call in cli.engine.calls))

    def test_docpack_can_stream_to_stdout_too(self):
        # docpack is not a canvas format at all -- FormatSpec.canvas_type is None,
        # it goes through IDocument.make_doc_pack instead -- so the canvas fix above
        # does not cover it. Same root cause, same fix: item.target is None under
        # OutputCardinality.STREAM, and make_doc_pack was called with it unconditionally.
        cli = CliHarness().with_document("a.docx", "text")
        document = cli.engine.add_document("a.docx", "text")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "docpack", "--stdout", "a.docx"))

        self.assertEqual([], cli.errors)
        self.assertEqual(["<stream>"], document.doc_packs_written)

    def test_a_report_alongside_a_text_conversion_is_refused_before_anything_opens(self):
        # Found end to end: convert --to text --bookmarks failed inside the engine,
        # and so did every other pairing of a text format with a report. One open
        # cannot serve both -- text extraction needs the text reader, a report needs
        # a paginated handle -- so whichever format bit is chosen, the other half
        # fails. Refused up front instead, and nothing is opened.
        cli = CliHarness().with_document("a.docx", "hello")

        self.assertEqual(exit_code.USAGE_ERROR,
                         cli.run("convert", "--to", "text", "--bookmarks", "--stdout", "a.docx"))

        self.assertTrue(any(ErrorCodes.REPORT_NEEDS_HIDEF in line for line in cli.errors))
        self.assertFalse(any(call.startswith("OpenDocument(") for call in cli.engine.calls))

    def test_an_output_directory_that_does_not_exist_yet_is_created(self):
        # Found from the README's own first example. Only the text path created it,
        # so --out-dir out/ worked for --to text and failed for every other format
        # with the engine's "FileStream::Create" error, which reads as though the
        # input were missing. One rule now, in convert_command.
        for output_format in ("pdf",       # single-file canvas: item.target
                              "png",       # per-page canvas: item.target_pattern
                              "text",      # written through IFileSystem, not the engine
                              "docpack"):  # a third writer again
            with self.subTest(format=output_format):
                cli = CliHarness().with_document("a.docx", "hello")

                self.assertEqual(exit_code.SUCCESS,
                                 cli.run("convert", "--to", output_format,
                                         "--out-dir", "brand/new", "a.docx"))

                self.assertTrue(cli.file_system.directory_exists("brand/new"),
                                "--to {0} did not create its output directory".format(
                                    output_format))

    def test_combining_a_text_format_concatenates_into_one_writer(self):
        # These formats have supports_combine = True and no canvas, so the combined
        # path read format.canvas_type.value on a None and threw -- not a CliError, so
        # it escaped program's handler and took the process down with an unhandled
        # exception rather than any documented exit code.
        for output_format in ("text", "md-text", "html"):
            with self.subTest(format=output_format):
                cli = CliHarness() \
                    .with_document("a.docx", "first") \
                    .with_document("b.docx", "second")

                self.assertEqual(exit_code.SUCCESS,
                                 cli.run("convert", "--to", output_format, "--combine",
                                         "--out-file", "out/all.txt", "a.docx", "b.docx"))

                self.assertEqual([], cli.errors)
                self.assertEqual("firstsecond", cli.file_system.read_text("out/all.txt"))

    def test_an_open_cancelled_by_the_timeout_is_reported_as_the_limit(self):
        # The engine stops and reports IGR_CANCELLED, which reaches us as
        # EngineError.CANCELLED. That must not be read as a broken document: the
        # cancellation was ours to ask for, so the limit is the cause and
        # E_LIMIT_EXCEEDED with exit 6 is the report -- the same line the check
        # after the document produces, so the two routes are indistinguishable.
        #
        # Driven through the real predicate rather than by scripting a cancelled open: the
        # run has to be the one that asks to stop, because that is what records the
        # reason. Scripting the engine's refusal instead tested the translation without
        # the decision behind it, and went on passing when a cancellation nobody asked
        # for started reporting the document instead.
        for verb in (["convert", "--to", "text"], ["extract"]):
            with self.subTest(verb=" ".join(verb)):
                cli = CliHarness().with_document("slow.docx", "text")
                # The document "takes" a minute without the suite waiting one: the clock
                # is the harness's, and the open moves it past the limit so the predicate
                # fires on its next poll. Sleeping instead would cost more than the whole
                # rest of the suite, since the smallest --timeout the validator accepts is
                # one second.
                cli.engine.during_open = lambda cli=cli: cli.clock.advance(60)

                argv = list(verb) + ["--out-dir", "out", "--timeout", "30", "slow.docx"]
                self.assertEqual(exit_code.LIMIT_EXCEEDED, cli.run(*argv))

                self.assertEqual("slow.docx: E_LIMIT_EXCEEDED: --timeout of 30s exceeded",
                                 self._single(cli.errors))

    def test_opening_a_document_offers_the_engine_a_way_to_stop(self):
        # The predicate has to reach open, because the heartbeat is raised during
        # open and nowhere else -- IGR_Render_Page takes no callback. Passing None
        # here would leave a document that hangs while parsing hanging forever,
        # which is exactly what --timeout is reached for.
        for verb in (["convert", "--to", "text"], ["extract"]):
            with self.subTest(verb=" ".join(verb)):
                cli = CliHarness().with_document("a.docx", "text")

                argv = list(verb) + ["--out-dir", "out", "--timeout", "30", "a.docx"]
                cli.run(*argv)

                self.assertTrue(any(call.startswith("OpenDocument(a.docx,")
                                    and call.endswith(",cancellable)")
                                    for call in cli.engine.calls))

    def test_an_interrupted_run_stops_between_documents_and_exits_130(self):
        # Ctrl+C is recorded rather than killing the process, so the run reports and
        # exits through the same path as every other code. Between documents is the
        # only point in a batch where stopping leaves nothing half-written; a render
        # already in flight cannot be interrupted at all, because IGR_Render_Page
        # takes no callback.
        for verb in (["convert", "--to", "text"], ["extract"]):
            with self.subTest(verb=" ".join(verb)):
                interruption = Interruption()
                interruption.request()

                cli = CliHarness(interruption) \
                    .with_document("a.docx", "one") \
                    .with_document("b.docx", "two")

                argv = list(verb) + ["--out-dir", "out", "a.docx", "b.docx"]
                self.assertEqual(exit_code.INTERRUPTED, cli.run(*argv))

                self.assertEqual("E_INTERRUPTED: interrupted", self._single(cli.errors))
                self.assertFalse(any(call.startswith("OpenDocument(")
                                     for call in cli.engine.calls))

    def test_a_run_nobody_interrupts_is_unaffected(self):
        cli = CliHarness(Interruption()).with_document("a.docx", "one")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "text", "--out-dir", "out", "a.docx"))
        self.assertEqual([], cli.errors)

    # ===================================== diagnostics on every command

    def test_every_command_that_reads_a_document_honours_the_diagnostics(self):
        # These are shared options, so they parse on all five -- and used to do
        # nothing on any of them. convert and extract were wired first; meta, list
        # and identify had no Diagnostics at all, so the switches stayed inert there
        # even after the others worked.
        for verb in (["convert", "--to", "text"], ["extract"], ["meta"], ["list"],
                     ["identify"]):
            with self.subTest(verb=" ".join(verb)):
                cli = CliHarness().with_document("a.docx", "hello")
                cli.engine.memory_status = "heap: 42 bytes"

                # No --out-dir: identify and meta declare no output group, so passing one
                # is a usage error rather than a no-op.
                argv = list(verb) + ["-x", "a.docx"]
                self.assertEqual(exit_code.SUCCESS, cli.run(*argv))

                self.assertTrue(any("heap: 42 bytes" in line for line in cli.out))

    def test_the_debug_input_switches_reach_the_inspection_commands(self):
        # identify never opens the document, so it has no open callback to cancel or
        # log through -- but what the engine makes of a truncated file is exactly what
        # --debug-truncate asks, so it gets the stream form too.
        for verb in (["meta"], ["list"], ["identify"]):
            with self.subTest(verb=" ".join(verb)):
                cli = CliHarness().with_document("a.docx", "hello world")

                argv = list(verb) + ["--debug-use-memory-stream", "a.docx"]
                self.assertEqual(exit_code.SUCCESS, cli.run(*argv))

                self.assertTrue(any("<stream:" in call for call in cli.engine.calls))

    def test_an_interrupt_stops_the_inspection_commands_too(self):
        for verb in (["meta"], ["list"], ["identify"]):
            with self.subTest(verb=" ".join(verb)):
                interruption = Interruption()
                interruption.request()

                cli = CliHarness(interruption) \
                    .with_document("a.docx", "one") \
                    .with_document("b.docx", "two")

                argv = list(verb) + ["a.docx", "b.docx"]
                self.assertEqual(exit_code.INTERRUPTED, cli.run(*argv))

                self.assertEqual("E_INTERRUPTED: interrupted", self._single(cli.errors))

    def test_extraction_creates_its_directories_through_the_filesystem_seam(self):
        # subfile_extraction called os.makedirs, so a subfile whose name carries a path
        # made real directories wherever the unit suite happened to run -- and nothing
        # about the extraction could be asserted without touching the disk.
        cli = CliHarness().with_document("archive.zip")
        container = cli.engine.add_document("archive.zip")
        container.sub_file_entries.append(FakeSubFile("nested/deep/inner.docx"))

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"))

        self.assertEqual([], cli.errors)
        self.assertTrue(cli.file_system.directory_exists("out/nested/deep"),
                        "extraction did not create its directory through IFileSystem")

    def test_combined_output_is_finished_before_the_viewer_sees_it(self):
        # --combine kept the canvas and the writer alive until the enclosing scope ended,
        # which is after --view runs: the viewer was handed a file with nothing written to
        # it yet, and for a PDF that means no trailer. The content is read inside the
        # viewer because afterwards it looks right either way.
        cli = CliHarness() \
            .with_document("a.docx", "first") \
            .with_document("b.docx", "second")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "text", "--combine", "--out-file", "out/all.txt",
                                 "--view", "--sort", "a.docx", "b.docx"))

        self.assertEqual(["out/all.txt"], cli.viewed)
        content = self._single(cli.viewed_content)
        self.assertIn("first", content)
        self.assertIn("second", content)

    def test_an_output_the_run_has_already_written_is_not_replaced_silently(self):
        # The planner checks every target before the run starts, against a directory none
        # of this run's outputs are in yet. Two names the filesystem matches to one file
        # but the ASCII collision key does not -- NTFS matches the two spellings of
        # a-ring, measured -- therefore both passed planning, and the second conversion
        # replaced the first with no --overwrite and exit 0.
        cli = CliHarness() \
            .with_document("a/\u00c5.docx", "from the upper-case name") \
            .with_document("b/\u00e5.docx", "from the lower-case name")
        cli.file_system.treat_as_same_file("out/\u00e5.txt", "out/\u00c5.txt")

        result = cli.run("convert", "--to", "text", "--out-dir", "out", "--sort",
                         "a/\u00c5.docx", "b/\u00e5.docx")

        self.assertEqual(exit_code.OUTPUT_CONFLICT, result)
        self.assertTrue(any("E_OUTPUT_EXISTS" in line for line in cli.errors), cli.errors)
        self.assertEqual("from the upper-case name", cli.file_system.read_text("out/\u00c5.txt"))

    def test_two_containers_cannot_extract_onto_each_other(self):
        # The claim set was built per container while the extractor is built per run, so
        # two archives each holding a readme.txt were never compared: with --overwrite the
        # second silently replaced the first, exit 0. The planner refuses that for
        # ordinary outputs whether or not --overwrite is given.
        cli = CliHarness().with_document("a.zip").with_document("b.zip")
        cli.engine.add_document("a.zip").sub_file_entries.append(FakeSubFile("readme.txt"))
        cli.engine.add_document("b.zip").sub_file_entries.append(FakeSubFile("readme.txt"))

        result = cli.run("extract", "--overwrite", "--out-dir", "out", "a.zip", "b.zip")

        self.assertEqual(exit_code.OUTPUT_CONFLICT, result)
        self.assertTrue(any("E_OUTPUT_COLLISION" in line for line in cli.errors), cli.errors)

    def test_a_subfile_cannot_be_written_through_a_link_out_of_the_output_directory(self):
        # Containment was lexical only: every segment of out/latest/index.html is a plain
        # name under the root, and if out/latest is a link the write still lands wherever
        # it points. The container picks that name, not the user.
        cli = CliHarness().with_document("archive.zip")
        cli.engine.add_document("archive.zip").sub_file_entries.append(FakeSubFile("latest/index.html"))
        cli.file_system.add_directory("elsewhere").mark_symlink("out/latest", "elsewhere")

        result = cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip")

        self.assertEqual(exit_code.OUTPUT_CONFLICT, result)
        self.assertTrue(any("E_UNSAFE_OUTPUT" in line for line in cli.errors), cli.errors)

    def test_an_ordinary_output_directory_is_not_mistaken_for_a_link(self):
        cli = CliHarness().with_document("archive.zip")
        cli.engine.add_document("archive.zip").sub_file_entries.append(FakeSubFile("latest/index.html"))
        cli.file_system.add_directory("out/latest")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"))
        self.assertEqual([], cli.errors)

    def test_extraction_refuses_to_replace_an_existing_file_without_overwrite(self):
        # The copy used to be unconditional, so a second extract into the same directory
        # silently replaced whatever was there. It is the same rule as every other output
        # now: E_OUTPUT_EXISTS, exit 4, nothing written.
        cli = CliHarness().with_document("archive.zip").with_existing_output("out/inner.txt")
        inner = FakeSubFile("inner.txt")
        cli.engine.add_document("archive.zip").sub_file_entries.append(inner)

        self.assertEqual(exit_code.OUTPUT_CONFLICT,
                         cli.run("extract", "--out-dir", "out", "archive.zip"))

        self.assertTrue(any("E_OUTPUT_EXISTS" in line for line in cli.errors), cli.errors)
        self.assertEqual([], inner.copied_to)

    def test_extraction_replaces_an_existing_file_with_overwrite(self):
        cli = CliHarness().with_document("archive.zip").with_existing_output("out/inner.txt")
        inner = FakeSubFile("inner.txt")
        cli.engine.add_document("archive.zip").sub_file_entries.append(inner)

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("extract", "--overwrite", "--out-dir", "out", "archive.zip"))

        self.assertEqual(["out/inner.txt"], inner.copied_to)

    def test_extraction_does_not_write_through_a_symbolic_link_inside_the_output_directory(self):
        # The name is clean text -- out/link/inner.txt sits inside out -- but with out/link
        # pointing elsewhere the bytes would land outside it.
        cli = CliHarness().with_document("archive.zip")
        cli.file_system.mark_symlink("out/link", "/elsewhere")
        inner = FakeSubFile("link/inner.txt")
        cli.engine.add_document("archive.zip").sub_file_entries.append(inner)

        self.assertEqual(exit_code.OUTPUT_CONFLICT,
                         cli.run("extract", "--make-dirs", "--out-dir", "out", "archive.zip"))

        self.assertTrue(any(ErrorCodes.UNSAFE_OUTPUT in line for line in cli.errors), cli.errors)
        self.assertEqual([], inner.copied_to)

    def test_extraction_writes_into_an_output_directory_that_is_itself_a_link(self):
        # The root is the user's own --out-dir; pointing it at a link is a choice.
        cli = CliHarness().with_document("archive.zip")
        cli.file_system.mark_symlink("out", "/elsewhere")
        inner = FakeSubFile("inner.txt")
        cli.engine.add_document("archive.zip").sub_file_entries.append(inner)

        self.assertEqual(exit_code.SUCCESS, cli.run("extract", "--out-dir", "out", "archive.zip"))

        self.assertEqual(["out/inner.txt"], inner.copied_to)

    def test_an_empty_out_dir_is_a_usage_error(self):
        # An empty directory joins as "/name", which is the filesystem root.
        for verb in (["extract"], ["convert", "--to", "text"]):
            with self.subTest(verb=verb):
                cli = CliHarness().with_document("a.docx", "text")

                self.assertEqual(exit_code.USAGE_ERROR, cli.run(*verb, "--out-dir", "", "a.docx"))

                self.assertTrue(any("E_BAD_VALUE" in line
                                    and "--out-dir does not accept the value ''" in line
                                    for line in cli.errors), cli.errors)

    def test_fingerprint_creates_no_output_directory(self):
        # --fingerprint renders into a discarded stream, so the plan's target is a
        # guess that is never written to; creating a directory for it would be a
        # side effect of a run that produces no files.
        cli = CliHarness().with_document("a.docx", "hello")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "pdf", "--fingerprint", "a.docx"))

        self.assertFalse(cli.file_system.directory_exists("brand/new"))

    def test_a_report_alongside_a_canvas_conversion_is_allowed(self):
        cli = CliHarness().with_document("a.docx", "hello")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "pdf", "--bookmarks", "--stdout", "a.docx"))

        self.assertEqual([], cli.errors)

    # ============================================ dispatch branches

    def test_dry_run_prints_the_plan_and_opens_nothing(self):
        cli = CliHarness().with_document("a.docx", "text")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "pdf", "--out-dir", "out",
                                 "--dry-run", "a.docx"))

        self.assertTrue(any("command convert" in line for line in cli.out))
        self.assertTrue(any("target out/a.pdf" in line for line in cli.out))
        self.assertFalse(any(call.startswith("OpenDocument(") for call in cli.engine.calls))

    def test_print_plan_is_the_same_branch_as_dry_run(self):
        dry_run = CliHarness().with_document("a.docx")
        print_plan = CliHarness().with_document("a.docx")

        dry_run.run("convert", "--to", "pdf", "--out-dir", "out", "--dry-run", "a.docx")
        print_plan.run("convert", "--to", "pdf", "--out-dir", "out", "--print-plan", "a.docx")

        self.assertEqual(dry_run.out, print_plan.out)

    def test_print_contract_dumps_the_surface_and_runs_nothing(self):
        cli = CliHarness().with_document("a.docx")

        self.assertEqual(exit_code.SUCCESS,
                         cli.run("convert", "--to", "pdf", "--out-dir", "out",
                                 "--print-contract", "a.docx"))

        self.assertTrue(any("command convert" in line for line in cli.out))
        self.assertTrue(any("--out-dir" in line for line in cli.out))
        self.assertEqual([], cli.engine.calls)

    def test_print_contract_wins_over_a_dry_run(self):
        # Both short-circuit; the contract dump is checked first, so it is what runs.
        cli = CliHarness().with_document("a.docx")

        cli.run("convert", "--to", "pdf", "--out-dir", "out", "--print-contract",
                "--dry-run", "a.docx")

        self.assertTrue(any("--print-contract" in line for line in cli.out))
        self.assertFalse(any("cardinality" in line for line in cli.out))

    # ============================================ the engine's lifetime

    def test_a_command_that_needs_no_engine_never_asks_for_one(self):
        # formats reads no document, but it does read the catalogue, so the engine is
        # touched exactly once and for that alone.
        cli = CliHarness()
        docx = EngineFormat()
        docx.id = 254
        docx.short_name = "docx"
        docx.display_name = "Microsoft Word 2007"
        cli.engine.formats.append(docx)

        self.assertEqual(exit_code.SUCCESS, cli.run("formats"))

        self.assertTrue(any("id=254" in line for line in cli.out))
        self.assertFalse(any(call.startswith("OpenDocument(") for call in cli.engine.calls))

    def test_a_run_that_fails_still_closes_every_document_it_opened(self):
        cli = CliHarness() \
            .with_document("first.docx", "text") \
            .with_unreadable_document("second.docx", EngineError.NOT_READABLE)

        cli.run("convert", "--to", "text", "--out-dir", "out", "first.docx", "second.docx")

        # Assert.All over the filtered calls: a loop, because unittest has no counterpart
        # and a generator inside one assertTrue would name neither the call that failed
        # nor how many there were.
        for call in [call for call in cli.engine.calls if call.startswith("OpenDocument(")]:
            self.assertIn(".docx", call)

    # ============================================ other verbs, end to end

    def test_identify_prints_one_line_per_input(self):
        cli = CliHarness()
        cli.with_document("a.bin")
        cli.engine.add_document("a.bin").format_name = "Microsoft Word 2007"

        self.assertEqual(exit_code.SUCCESS, cli.run("identify", "a.bin"))

        line = self._single(cli.out)
        self._assert_starts_with("a.bin: format=", line)
        self.assertIn("name=Microsoft Word 2007", line)

    def test_version_needs_neither_a_document_nor_a_walk(self):
        cli = CliHarness()

        self.assertEqual(exit_code.SUCCESS, cli.run("version"))
        self.assertNotEqual([], cli.out)

    # ============================================ the console pre-scan

    def test_the_console_settings_are_read_before_anything_is_bound(self):
        """
        Read straight out of argv before anything is bound, because the console has to
        exist before the first line is written and binding has not happened yet. A raw
        scan is only safe because none of the three takes a value and prefix abbreviation
        is off.
        """
        self.assertTrue(program.pre_scan_console_options(["convert", "--silent"]).silent)
        self.assertTrue(program.pre_scan_console_options(["convert", "--verbose"]).verbose)
        self.assertTrue(program.pre_scan_console_options(["convert", "--color"]).color)

        none = program.pre_scan_console_options(["convert", "a.docx"])

        self.assertFalse(none.silent)
        self.assertFalse(none.verbose)
        self.assertFalse(none.color)

    def test_the_pre_scan_reads_only_exact_spellings(self):
        # Prefix abbreviation is off everywhere, so a partial spelling is an unknown
        # argument rather than a match -- the scan must not be more generous than the
        # parser it front-runs.
        self.assertFalse(program.pre_scan_console_options(["convert", "--sil"]).silent)
        self.assertFalse(program.pre_scan_console_options(["convert", "--silent=true"]).silent)

    def test_the_pre_scan_survives_an_empty_command_line(self):
        options = program.pre_scan_console_options([])

        self.assertFalse(options.silent)
