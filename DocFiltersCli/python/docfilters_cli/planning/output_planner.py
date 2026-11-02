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

from typing import List

from ..runtime.cli_error import ErrorCodes, NoInputsError, OutputConflictError
from .output_name_template import OutputNameTemplate
from .page_range import PageRange
from .run_plan import OutputCardinality, PlannedItem, RunPlan, TargetPattern

# Decides where every output goes, before anything is written.
#
# The one rule underneath all of this: the meaning of -o never depends on --to. Four
# cardinalities, chosen by the arguments alone:
#
#   PER_INPUT      --out-dir, each input writes its own output
#   COMBINED       --combine --out-file book.pdf
#   SINGLE_TARGET  --out-file with exactly one input
#   STREAM         --stdout, or --out-file -
#
# These name target ownership only. How many files one input produces is the separate
# question FormatSpec.is_single_file answers: a .docx to PNG writes one file per page and
# is still PER_INPUT.
#
# --out-file with several inputs and no --combine is a hard error, never a guess.
#
# A module of functions rather than a class: the C# is a static class, which is what C#
# has instead of a module.


def plan(shared, convert, inputs: List, file_system) -> RunPlan:
    """
    Builds the plan. Pure: no filesystem access, so it is exhaustively testable with a
    table of cases.

    Raises NoInputsError when inputs is empty, and OutputConflictError for --out-file
    with multiple inputs and no --combine, or two inputs resolving to the same output
    path.
    """
    if len(inputs) == 0:
        raise NoInputsError("no inputs matched; the walk and filters selected nothing")

    result = RunPlan()
    result.format = convert.format
    result.cardinality = resolve_cardinality(shared, len(inputs), convert.combine)

    if result.cardinality == OutputCardinality.COMBINED:
        result.combined_target = shared.output.out_file

    output_format = convert.format
    template = shared.output.name_template
    if template is None:
        template = OutputNameTemplate.default(output_format.is_single_file)
    out_dir = shared.output.out_dir if shared.output.out_dir is not None else "."
    pages = convert.pages if convert.pages is not None else PageRange.ALL

    for source in inputs:
        item = PlannedItem()
        item.source_path = source.path
        item.display_name = source.path
        item.pages = pages

        if result.cardinality == OutputCardinality.PER_INPUT:
            _assign_per_input_target(item, source, output_format, template, out_dir)
        elif result.cardinality == OutputCardinality.SINGLE_TARGET:
            item.target = shared.output.out_file
        # COMBINED and STREAM: neither owns a path of its own: a combined item feeds
        # RunPlan.combined_target, and a streamed one writes to stdout.

        result.items.append(item)

    check_for_collisions(result, file_system)
    check_outputs_are_not_inputs(result, file_system)

    return result


def _assign_per_input_target(item: PlannedItem, source, output_format, template, out_dir: str):
    """
    Resolves one PER_INPUT item's target: the source tree replicated under --out-dir,
    then the naming template applied to the filename only.
    """
    relative = source.relative_path if source.relative_path is not None else source.path
    directory = _combine_directories(out_dir, _directory_of(relative))

    if output_format.is_single_file:
        item.target = _join_path(directory, template.render(item.source_path, output_format.extension, 1))
    else:
        pattern = TargetPattern()
        pattern.directory = directory
        pattern.template = template
        pattern.extension = output_format.extension
        item.target_pattern = pattern


def _directory_of(relative_path: str) -> str:
    """The directory portion of a portable, forward-slash relative path."""
    normalised = relative_path.replace('\\', '/')
    slash = normalised.rfind('/')
    if slash < 0:
        return ""
    return normalised[:slash]


def _combine_directories(out_dir: str, relative_directory: str) -> str:
    if not relative_directory:
        return out_dir

    if not out_dir:
        return relative_directory
    return out_dir + "/" + relative_directory


def _join_path(directory: str, file_name: str) -> str:
    return file_name if not directory else directory + "/" + file_name


def verify_targets(run_plan: RunPlan, shared, file_system):
    """
    The one part of planning that has to touch the filesystem: refusing to clobber
    existing files without --overwrite. Kept separate so that plan itself stays pure.

    For a single-file item this is the whole check. For a per-page item it checks only
    TargetPattern.first_target, because the remaining names depend on the document's page
    count; execution checks each page as it goes, through verify_page_target.
    """
    if shared.output.overwrite:
        return

    if run_plan.cardinality == OutputCardinality.COMBINED:
        verify_page_target(run_plan.combined_target, shared, file_system)
    else:
        for item in run_plan.items:
            # A STREAM item owns neither: it writes to stdout, not a path.
            if item.target is None and item.target_pattern is None:
                continue

            path = (item.target_pattern.first_target(item.source_path, item.pages)
                    if item.is_page_count_dependent else item.target)
            verify_page_target(path, shared, file_system)


def verify_page_target(path: str, shared, file_system):
    """
    The deferred half of verify_targets, called during execution once a
    page-count-dependent item's real filenames are known.
    """
    if shared.output.overwrite:
        return

    if file_system.file_exists(path):
        raise OutputConflictError(ErrorCodes.OUTPUT_EXISTS,
                                  "{0} already exists; use --overwrite to replace it".format(path))


def resolve_cardinality(shared, input_count: int, combine: bool) -> OutputCardinality:
    """
    Chooses the cardinality from the output arguments and the input count.

    Also used by a command with an output group but no --combine of its own — extract,
    list and meta, which always pass combine=False. Their output names come from the
    container once it is open, so they stop at cardinality rather than resolving a target.
    """
    if shared.output.use_stdout:
        return OutputCardinality.STREAM

    if shared.output.out_file is not None:
        if combine:
            return OutputCardinality.COMBINED

        if input_count > 1:
            raise OutputConflictError(
                ErrorCodes.OUTFILE_MULTI_INPUT,
                "--out-file takes a single input; use --combine to merge several into one document")

        return OutputCardinality.SINGLE_TARGET

    return OutputCardinality.PER_INPUT


def check_for_collisions(run_plan: RunPlan, file_system):
    """
    Detects two inputs resolving to the same output path, which happens when a flattened
    walk contains two files with the same stem.

    For statically resolved items this compares the target paths directly. For
    page-count-dependent items it compares TargetPattern.collision_key, which catches the
    real-world case — a/report.docx and b/report.docx both flattening into one output
    directory — without needing to know how many pages either has.
    """
    seen = set()
    for item in run_plan.items:
        key = item.target
        if key is None and item.target_pattern is not None:
            key = item.target_pattern.collision_key(item.source_path)

        if key is not None:
            key = collision_key_for(key, file_system)
            if key in seen:
                raise OutputConflictError(ErrorCodes.OUTPUT_COLLISION,
                                          "two inputs would write to the same output; use --out-name to distinguish them")
            seen.add(key)


def check_outputs_are_not_inputs(run_plan: RunPlan, file_system):
    """
    Refuses a plan that would write over one of the documents it reads.

    --overwrite says an existing file may be replaced, and an input is an existing file, so
    nothing else here stops `convert --to text -o . --overwrite notes.txt` from naming
    notes.txt as both. The combined case is the plainest: the output is created, and
    therefore truncated, before the first document is opened, so the source is gone and the
    run then fails to read it. A glob reaches the same place without a typo -- a second run
    over a directory picks up the first run's output as an input.

    This is the collision check pointed at the other set: two outputs must not share a
    path, and an output must not share one with an input either. It uses the same key, so
    it folds case exactly where the filesystem does.

    A page-count-dependent item is compared by its first target, for the reason
    verify_targets gives: the rest of the names are not known until the document is open.
    Page one is the one that would be written first, and writing it is what would destroy
    the source.
    """
    inputs = set(collision_key_for(item.source_path, file_system) for item in run_plan.items)

    for target in _planned_targets(run_plan):
        if collision_key_for(target, file_system) in inputs:
            raise OutputConflictError(ErrorCodes.OUTPUT_IS_INPUT,
                                      target + " is also an input; use --out-dir or --out-name to write somewhere else")


def _planned_targets(run_plan: RunPlan):
    """Every path the plan would write, as far as it is knowable before the run."""
    if run_plan.combined_target is not None:
        yield run_plan.combined_target

    for item in run_plan.items:
        if item.target is not None:
            yield item.target
        elif item.target_pattern is not None:
            yield item.target_pattern.first_target(item.source_path, item.pages)


def collision_key_for(path: str, file_system) -> str:
    """
    A planned path reduced to what two outputs must not share.

    The path itself where the filesystem tells names apart by case, and a case-folded copy
    where it does not. Comparing exactly everywhere let a/Report.docx and b/report.docx
    both pass planning and then write to one file on Windows and macOS: the run exited 0,
    reported nothing, and the second output had silently replaced the first.

    Folding everywhere instead would have been the other error -- on a case-sensitive
    filesystem those are two files, and refusing to write both declines work the tool can
    do correctly. So the question goes to IFileSystem.is_case_sensitive rather than to the
    host or to a fixed rule, which is also what keeps this testable: a fake filesystem
    states its own answer, so both behaviours are covered on every platform.

    ASCII only, and deliberately not str.lower(), which folds the whole of Unicode: the
    reference folds A-Z alone, so that collision detection neither depends on the
    machine's locale nor needs a shared table to agree across the four languages. Two names
    differing only outside ASCII therefore do not collide here. What catches that pair is
    verify_page_target, asked again immediately before each write, when the earlier output
    is on disk and the filesystem answers for itself.
    """
    if file_system.is_case_sensitive(path):
        return path

    return "".join(chr(ord(c) + 32) if "A" <= c <= "Z" else c for c in path)
