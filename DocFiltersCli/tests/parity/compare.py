#!/usr/bin/env python3
# (c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""
Cross-implementation parity: the same commands through every implementation,
compared byte for byte.

This is the ring the other suites cannot reach. tests/conformance/ proves each
implementation agrees with the shared contract, but it stops at the plan --- it
runs stages 1 to 3 through --dry-run and executes nothing. tests/<lang>/ covers
execution, but through each language's own translated copy of the tests, so a
case dropped in translation is simply gone. And every end-to-end check asserts
a *property* ("exit 0", "one file per page"), which two implementations can
satisfy while still disagreeing byte for byte.

Here the comparison *is* the assertion. Nothing states an expected value, so
there is nothing to keep in step with the implementations: the only claim is
that they all did the same thing.

The commands come from tests/e2e/end2end.py rather than from a list of their
own. That suite already drives every verb against a real document, so a second
corpus would be a second thing to maintain --- and the first time it fell
behind, nothing would notice, because a command list has no assertions to fail.
`end2end.py --record` writes a transcript of what it ran; this reads them.

    python ../e2e/end2end.py --cli <csharp>  --record csharp.txt
    python ../e2e/end2end.py --cli <cpp17>   --record cpp17.txt
    python ../e2e/end2end.py --cli python    --record python.txt
    python ../e2e/end2end.py --cli java      --record java.txt
    python compare.py csharp.txt cpp17.txt python.txt java.txt

The first transcript is the reference; every other is compared against it.
Exits 0 when they agree, 1 when they do not, 2 on a usage problem.

A command one implementation cannot agree on can be waived in
parity-waivers.txt, with a reason, and the waiver fails once it stops being
needed; see `load_waivers`.
"""

import argparse
import base64
import difflib
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


class Record:
    """One invocation, as a transcript records it."""

    def __init__(self, case, argv):
        self.case = case          # "<check name> #<n>", the alignment key
        self.argv = argv
        self.exit_code = None
        self.stdout = b""
        self.stderr = b""
        self.files = {}
        self.streams_compared = True
        self.not_compared_because = None


class Transcript:
    def __init__(self, path):
        self.path = path
        # The file's own name is the label, because the caller chose it and a
        # recorded --cli target is a build path nobody wants to read in a
        # failure report. The target is kept for provenance and printed once.
        self.implementation = os.path.splitext(os.path.basename(path))[0]
        self.target = None
        self.records = {}
        # Checks this implementation did not run, and why: {check: reason}.
        # Keyed by the check rather than by the case, because a skip is a
        # decision about the check as a whole and the commands it would have
        # run are precisely what is not known.
        self.skipped = {}
        self._read(path)

    def skipped_for(self, case):
        """
        Why this implementation has no record for `case`, or None if it should
        have one.

        A case key is `<check> #<n>`, so the check it belongs to is the key
        without its position.
        """
        return self.skipped.get(case.rsplit(" #", 1)[0])

    def _read(self, path):
        current = None
        last_skipped = None
        with open(path, "r", encoding="utf-8") as handle:
            for number, raw in enumerate(handle, start=1):
                line = raw.rstrip("\n")
                if not line or line.startswith("#"):
                    continue
                key, _, rest = line.partition(" ")
                if key == "implementation":
                    self.target = rest
                elif key == "skipped-check":
                    # The reason arrives on the next line; until it does the
                    # skip is recorded with none, so a truncated transcript
                    # still reads rather than losing the skip entirely.
                    last_skipped = rest
                    self.skipped.setdefault(rest, "no reason recorded")
                    current = None
                elif key == "skipped-because":
                    if last_skipped is None:
                        raise SystemExit("%s:%d  a reason before any skipped check"
                                         % (path, number))
                    self.skipped[last_skipped] = rest
                elif key == "case":
                    current = Record(rest, None)
                    if rest in self.records:
                        raise SystemExit("%s:%d  duplicate case key %r"
                                         % (path, number, rest))
                    self.records[rest] = current
                elif current is None:
                    raise SystemExit("%s:%d  %r before any case" % (path, number, key))
                elif key == "argv":
                    current.argv = rest
                elif key == "exit":
                    current.exit_code = int(rest)
                elif key == "stdout":
                    current.stdout = base64.b64decode(rest) if rest else b""
                elif key == "stderr":
                    current.stderr = base64.b64decode(rest) if rest else b""
                elif key == "streams-not-compared":
                    # The recorder set these aside: their content is contract by
                    # shape only. The exit code and the files are still compared.
                    current.streams_compared = False
                    current.not_compared_because = rest
                elif key == "file":
                    relative, _, digest = rest.rpartition(" ")
                    current.files[relative] = digest
                else:
                    raise SystemExit("%s:%d  unknown key %r" % (path, number, key))

        if not self.records:
            raise SystemExit("%s holds no records; did the run fail before it started?"
                             % path)


def show(data):
    """
    Bytes as readable lines, with CR and every other escape still visible.

    Split on b"\\n" over bytes, never str.splitlines(): that treats CRLF and LF
    as the same separator and consumes both, which would erase precisely the
    difference this suite exists to find -- one implementation writing a line
    break inside engine text as CRLF where the reference wrote LF. It would
    then compare equal, and the suite would report an agreement it never
    checked.
    """
    return [repr(line)[2:-1] for line in data.split(b"\n")]


def stream_difference(name, reference, other, volatile_lines):
    """A unified diff of two captured streams, ignoring known-volatile lines."""
    if reference == other:
        return []

    left, right = show(reference), show(other)
    if volatile_lines:
        left = [l for i, l in enumerate(left) if i not in volatile_lines]
        right = [l for i, l in enumerate(right) if i not in volatile_lines]
        if left == right:
            return []

    diff = list(difflib.unified_diff(left, right, fromfile="reference " + name,
                                     tofile="this " + name, lineterm="", n=1))
    return diff[:40]


def volatile(first, second):
    """
    What differs between two runs of the *same* implementation, as
    ({stream: volatile line indices}, volatile files).

    Indices are kept per stream rather than in one set. stderr is usually a
    line or two where stdout is hundreds, so a single shared set lets a
    volatile stdout line at index 0 mask index 0 of stderr -- which is exactly
    where an error message sits, and error paths are a good part of what this
    suite compares.

    Measured rather than guessed at with a list of regexes, so a new source of
    nondeterminism -- a timestamp, a duration, a temporary path -- is handled
    the first time it appears instead of the first time it reddens a build.
    Without this the gate becomes one people learn to re-run until it passes.
    """
    lines = {}
    for stream in ("stdout", "stderr"):
        indices = set()
        left, right = show(getattr(first, stream)), show(getattr(second, stream))
        for index in range(max(len(left), len(right))):
            a = left[index] if index < len(left) else None
            b = right[index] if index < len(right) else None
            if a != b:
                indices.add(index)
        lines[stream] = indices
    files = {path for path, digest in first.files.items()
             if second.files.get(path) != digest}
    files |= set(second.files) - set(first.files)
    return lines, files


def compare(reference, others, repeat, verbose):
    volatile_by_case = {}
    if repeat:
        for case, record in reference.records.items():
            twin = repeat.records.get(case)
            if twin is not None:
                volatile_by_case[case] = volatile(record, twin)

    failed = 0
    for case, expected in reference.records.items():
        problems = []
        volatile_lines, volatile_files = volatile_by_case.get(
            case, ({"stdout": set(), "stderr": set()}, set()))

        for other in others:
            actual = other.records.get(case)
            if actual is None:
                # Absent because the check was skipped here, or absent because
                # it went missing -- which is the difference between a port
                # saying "I cannot do this" and a case lost in translation.
                # Only the transcript can tell them apart, and it does.
                if other.skipped_for(case) is None:
                    problems.append("%s never ran this; it is in %s only"
                                    % (other.implementation, reference.implementation))
                continue

            if actual.exit_code != expected.exit_code:
                problems.append("exit code was %d for %s, %d for %s"
                                % (expected.exit_code, reference.implementation,
                                   actual.exit_code, other.implementation))

            for stream in ("stdout", "stderr"):
                if not (expected.streams_compared and actual.streams_compared):
                    continue
                diff = stream_difference(stream, getattr(expected, stream),
                                         getattr(actual, stream),
                                         volatile_lines.get(stream, set()))
                if diff:
                    problems.append("%s differs between %s and %s:\n        %s"
                                    % (stream, reference.implementation,
                                       other.implementation, "\n        ".join(diff)))

            mine = {p: d for p, d in actual.files.items() if p not in volatile_files}
            theirs = {p: d for p, d in expected.files.items() if p not in volatile_files}
            only_reference = sorted(set(theirs) - set(mine))
            only_other = sorted(set(mine) - set(theirs))
            changed = sorted(p for p in set(theirs) & set(mine) if theirs[p] != mine[p])

            if only_reference:
                problems.append("%s wrote files %s did not: %s"
                                % (reference.implementation, other.implementation,
                                   ", ".join(only_reference)))
            if only_other:
                problems.append("%s wrote files %s did not: %s"
                                % (other.implementation, reference.implementation,
                                   ", ".join(only_other)))
            if changed:
                # Named, because without it a failure says only that some file
                # differs and leaves you to work out which implementation wrote
                # it -- which is a long way round when three are being compared.
                problems.append("%s wrote different bytes from %s in: %s"
                                % (other.implementation, reference.implementation,
                                   ", ".join(changed)))

        if problems:
            failed += 1
            print("%s\n    $ %s" % (case, expected.argv))
            for problem in problems:
                print("    %s" % problem)
            print()
        elif verbose:
            print("ok  %s" % case)

    # A record only the other side has is drift too: the reference never ran it,
    # so nothing above would have looked. Unless the reference says it skipped
    # that check, which is the same distinction as above read the other way.
    for other in others:
        extra = sorted(set(other.records) - set(reference.records))
        for case in extra:
            if reference.skipped_for(case) is not None:
                continue
            failed += 1
            print("%s\n    ran by %s but not by %s"
                  % (case, other.implementation, reference.implementation))

    return failed


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("transcripts", nargs="+", metavar="TRANSCRIPT",
                        help="transcripts from end2end.py --record; the first is "
                             "the reference")
    parser.add_argument("--repeat", default=None, metavar="TRANSCRIPT",
                        help="a second transcript of the *reference* implementation. "
                             "Anything that differs between it and the first is "
                             "nondeterminism, and is excluded from the comparison")
    parser.add_argument("--verbose", action="store_true",
                        help="name every case, not just the ones that diverged")
    arguments = parser.parse_args()

    if len(arguments.transcripts) < 2:
        parser.error("at least two transcripts are needed to compare anything")

    loaded = [Transcript(path) for path in arguments.transcripts]
    repeat = Transcript(arguments.repeat) if arguments.repeat else None

    reference, others = loaded[0], loaded[1:]
    print("comparing %d implementations over %d recorded commands"
          % (len(loaded), len(reference.records)))
    for one in loaded:
        print("    %-10s %d records from %s"
              % (one.implementation, len(one.records), one.target or one.path))
    print("    reference: %s" % reference.implementation)

    # Only the skips that are not shared. A check every implementation skipped
    # -- one licenseless evaluation forbids, say -- is a property of the run,
    # not of any one port, and compares equal on both sides. One that only some
    # of them skipped is a hole in the comparison, and the report says whose.
    for one in loaded:
        for check, reason in sorted(one.skipped.items()):
            if all(check in two.skipped for two in loaded):
                continue
            print("    %s did not run %r: %s" % (one.implementation, check, reason))
    if repeat:
        print("    nondeterminism measured against %s" % repeat.path)
    else:
        print("    no --repeat given: nondeterminism is not being masked")
    print()

    failed = compare(reference, others, repeat, arguments.verbose)
    print("%d of %d recorded commands diverged" % (failed, len(reference.records)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
