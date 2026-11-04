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
End-to-end tests: the real CLI, the real Document Filters library, real files on
a real disk.

This is tier 3 from the design document. Everything in tests/csharp runs against
FakeDocumentFiltersEngine and so can only check what the CLI *says* to the
engine; nothing there can catch a call the engine rejects, a flag the binding
discards on the way through, or an output file that is written but blank. Those
are exactly the bugs this has caught, so several checks below are regressions
rather than smoke: each names the bug it guards, and would still pass if it only
asserted "exit 0 and the file exists".

Stdlib only, and a subprocess boundary rather than a test framework, so it runs
from a clean checkout with nothing installed and stays honest about what the
shipped executable does.

A license key is optional. Without one the library runs in licenseless
evaluation, which permits enough to be worth running unattended -- see
LICENSELESS below for exactly what it does and does not allow. Checks the mode
forbids are skipped with a reason rather than failed, so a run with no key is
green when the tool is correct.

One script, any implementation. --cli names which one: a built executable for
the C# and C++17 ports, or the directory holding the Python package or the Java
project for those two. With nothing given it finds the most recently built executable. The checks
themselves know nothing about which -- they invoke a command line and read files
off a disk, which is the whole point of having a contract rather than a shared
test base class. Nothing here is conditional on the implementation: the runs are
the same checks with the same skips, which is the claim being made.

Usage:
    python end2end.py                       # licenseless; skips what eval forbids
    python end2end.py --license-key KEY     # everything
    python end2end.py --cli path/to/docfilters.exe --keep --verbose
    python end2end.py --cli python          # the Python port, from python/
    python end2end.py --cli java --library DIR   # the Java port, from java/

The output directory is emptied before the run and again afterwards. It is
guarded: see prepare_output_dir.
"""

import argparse
import base64
import difflib
import glob
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

# Dropped into the output directory so cleanup can tell a scratch directory it
# owns from one the user pointed at by mistake. See prepare_output_dir.
MARKER = ".df-e2e-scratch"

# This checkout, two levels up from tests/e2e. Everything else is found relative
# to it, so the script works from any clone and on any platform.
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

# The document this suite exercises, committed beside it.
#
# It used to default to the docfilters repository's sample documents, assuming
# the two checkouts shared a parent -- a guess about someone's disk that this
# repository does not control, and one that made a clean clone unable to run
# this script at all. test.pdf is here instead: multi-page, with extractable
# text, metadata and an embedded image, which is everything the checks below
# need. --documents and --input still override it.
DEFAULT_DOCUMENTS = os.path.join(REPO_ROOT, "tests", "e2e")
DEFAULT_INPUT = "test.pdf"

# The published Python binding, guessed from the same sibling-checkout layout
# DEFAULT_DOCUMENTS assumes. Only used when it is really there: a binding
# installed from PyPI is already importable and needs no path at all.
DEFAULT_BINDING = os.path.normpath(
    os.path.join(REPO_ROOT, os.pardir, os.pardir, "docfilters", "filters", "api", "python"))

# What the native library is called on this host. Used to tell the one runtimes/
# directory that holds a library this host can load from the several a build
# restores -- matching any of the three names would happily pick osx-x64 on
# Windows, which fails at load with "ISYS11df not found".
LIBRARY_NAME = ("ISYS11df.dll" if os.name == "nt"
                else "libISYS11df.dylib" if sys.platform == "darwin"
                else "libISYS11df.so")

# The Java binding's own JNI library, which it loads beside LIBRARY_NAME. The NuGet
# package the C# build restores does not carry it, so the Java port cannot borrow
# that copy the way the Python port does; the release archives do.
JNI_LIBRARY_NAME = ("ISYS11dfjava.dll" if os.name == "nt"
                    else "libISYS11dfjava.dylib" if sys.platform == "darwin"
                    else "libISYS11dfjava.so")

# The runtimes/ directory this interpreter can load from, spelled as NuGet spells
# it. The name is not enough on Windows, where win-x64 and win-x86 both hold an
# ISYS11df.dll and only one of them matches the interpreter -- loading the other
# is an OSError about an invalid image rather than anything about a CLI. The
# architecture is the interpreter's own, not the machine's: a 32-bit Python on a
# 64-bit host wants win-x86.
RUNTIME_IDENTIFIER = "%s-%s" % (
    "win" if os.name == "nt" else "osx" if sys.platform == "darwin" else "linux",
    "arm64" if platform.machine().lower() in ("arm64", "aarch64")
    else "x64" if sys.maxsize > 2 ** 32
    else "x86")

# Exit codes, from Runtime/ExitCode.cs. Named here rather than inlined so a
# failure report says which contract was broken.
EXIT_OK = 0
EXIT_PARTIAL_FAILURE = 1
EXIT_USAGE = 2
EXIT_NO_INPUTS = 3
EXIT_OUTPUT_CONFLICT = 4
EXIT_LIMIT_EXCEEDED = 6


# --------------------------------------------------------------------------
# LICENSELESS
#
# Without a key the library runs in licenseless evaluation, which gates three
# things independently (ISYSreaderImpl.cpp, namespace licenseless_eval):
#
#   input format   text, PDF, Word, Excel, PowerPoint, OpenDocument. Nothing
#                  else opens at all.
#   open flags     none, IGR_FORMAT_TEXT, IGR_FORMAT_MARKDOWN, IGR_FORMAT_IMAGE.
#                  IGR_FORMAT_HTML and IGR_FORMAT_XML are refused.
#   canvas type    PDF and the raster devices (PNG, JPG, TIF, BMP, GIF, WEBP,
#                  PBM/PGM/PPM, TGA, BRK, PCX, DCX, JPEG2000, WEBSAFE) plus
#                  MARKDOWN. HTML, SVG, XML, XPS, EPS and PS are refused.
#
# A refusal is IGR error 4015, which this CLI reports as E_DOCUMENT_FAILED. So
# the vector and markup canvases are the gap, and a check that needs one is
# marked with @needs_license.
#
# Licensed output is also not byte-identical to licenseless output: evaluation
# renders stamp an "Evaluation Copy" watermark onto each page. Nothing below
# depends on exact bytes, for that reason.
# --------------------------------------------------------------------------

def needs_license(reason):
    """Marks a check that licenseless evaluation refuses, with why."""
    def decorate(check):
        check.license_reason = reason
        return check
    return decorate


class CheckFailed(Exception):
    """A check's assertion did not hold. Carries its own explanation."""


class CheckSkipped(Exception):
    """
    A check cannot run here and has not failed. Carries why.

    For conditions only discovered at run time -- a document too small to
    exercise a limit, say. Restrictions known up front use @needs_license.
    """


class EngineSettings:
    """
    Puts this suite's ISYS11df.ini beside the implementation under test for the
    length of the run, and puts back whatever was there before.

    The engine reads defaults from that file, and two of them change what the
    tool produces: `[Class:DEFAULT:TEXT] ENUMERATE_IMAGES=ON` decides whether a
    PDF's embedded images appear as subfiles, and `[Output]
    TIFF_COMPRESSION=AUTO` decides whether a TIFF is compressed. Whether one is
    present is a property of how the library was obtained rather than of the
    code: the GitHub release zip the C++17 build downloads carries one, and the
    NuGet packages the C# build restores do not. The two implementations were
    therefore being asked different questions, and the parity comparison saw it
    as a 3x difference in TIFF size and four subfiles that only one of them
    listed.

    So the file is supplied here, from the checkout, and every implementation
    gets the same one.

    Restoring is not best-effort: whatever was in that directory belongs to the
    build, not to this script, and leaving a test's configuration behind would
    silently change what the next run of anything else does.
    """

    NAME = "ISYS11df.ini"
    SOURCE = os.path.join(os.path.dirname(os.path.abspath(__file__)), NAME)
    SUFFIX = ".before-e2e"

    FONTS = "fonts"

    def __init__(self, directory):
        self.directory = directory
        self.target = None
        self.backup = None
        self.detail = "not installed"
        self.fonts_link = None
        self.fonts_detail = "already present"

    def __enter__(self):
        if not self.directory:
            self.detail = "nowhere to install it; the engine's own defaults apply"
            return self
        if not os.path.isfile(self.SOURCE):
            self.detail = "%s is missing from this checkout" % self.NAME
            return self

        target = os.path.join(self.directory, self.NAME)
        backup = target + self.SUFFIX

        # A backup already sitting there is the wreckage of a run that was
        # killed before it could restore. Its copy is the original, so it is
        # kept and the stale working copy is the one that goes.
        if os.path.exists(backup):
            # A backup already sitting there is the wreckage of a run that was
            # killed before it could restore. *It* holds the original, so it is
            # adopted and the working copy beside it -- this script's, from that
            # run -- is the one that goes. Adopting it is what matters: without
            # that, __exit__ would remove our copy, restore nothing, and leave
            # the directory with no ISYS11df.ini at all and the real one
            # stranded under a name the engine does not read.
            if os.path.exists(target):
                os.remove(target)
            self.backup = backup
        elif os.path.exists(target):
            os.replace(target, backup)
            self.backup = backup

        shutil.copyfile(self.SOURCE, target)
        self.target = target
        self.detail = "%s (from tests/e2e%s)" % (
            target, ", one was already there and is kept" if self.backup else "")

        self._link_fonts()
        return self

    def _link_fonts(self):
        """
        Makes the shipped `fonts` directory visible beside the library, if it is
        not already.

        The engine finds its fonts beside the library (or beside the executable)
        and nowhere else on Windows --- `ISYS_FONTS` is honoured on Linux and
        ignored there --- so an implementation whose library lives apart from
        the resources renders with whatever it can find instead. That is not a
        cosmetic difference: the set of fonts loaded decides how text is
        subsetted, and on Linux a missing set fails the conversion outright.

        The C# and C++17 builds each carry their own copy beside their
        executable and need nothing. The Python port is started from the
        interpreter, so the engine looks beside the library --- which in a NuGet
        layout is runtimes/<rid>/native/, where there is no `fonts` directory.

        A link rather than a copy: it is 198 files, and copying them in and out
        of a directory the build owns, twice per implementation per run, is a
        lot of churn to make one directory visible. A junction is used on
        Windows because a symbolic link there needs a privilege a developer
        shell usually lacks, while `mklink /J` does not.
        """
        if not self.directory:
            return

        link = os.path.join(self.directory, self.FONTS)
        if os.path.isdir(link):
            return
        if os.path.lexists(link):
            # The name is taken by something that is not a usable directory --
            # a link whose target has gone, most likely from a run that was
            # killed. Creating over it would fail, so it goes.
            try:
                os.unlink(link)
            except OSError as error:
                self.fonts_detail = "%s is in the way: %s" % (link, error)
                return

        source = find_fonts()
        if not source:
            self.fonts_detail = "none found to link; the engine will use what it can find"
            return

        try:
            if os.name == "nt":
                # check=True: a junction that silently failed would leave the run
                # measuring the wrong thing, which is worse than stopping.
                subprocess.run(["cmd", "/c", "mklink", "/J", link, source],
                               check=True, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
            else:
                os.symlink(source, link, target_is_directory=True)
            self.fonts_link = link
            self.fonts_detail = "linked to %s" % source
        except (OSError, subprocess.SubprocessError) as error:
            self.fonts_detail = "could not link %s: %s" % (source, error)

    def __exit__(self, *_):
        if self.fonts_link:
            # os.unlink, never shutil.rmtree. unlink removes the name and leaves
            # what it pointed at alone; rmtree would walk through the link and
            # delete the build's fonts.
            #
            # unlink rather than rmdir, which was wrong on POSIX: rmdir refuses
            # a symbolic link with ENOTDIR, so the link survived the run, and
            # because os.path.isdir follows it the next run saw a directory
            # already there and never cleaned it up either. unlink removes both
            # a POSIX symbolic link and a Windows junction. The rmdir fallback
            # is kept for an interpreter where it does not.
            #
            # Branching on os.path.islink would not work: a junction reports
            # False for it on Windows.
            try:
                os.unlink(self.fonts_link)
            except OSError:
                try:
                    os.rmdir(self.fonts_link)
                except OSError as error:
                    # Not silent. A link left behind changes what the next run
                    # of anything else in this directory sees.
                    print("warning: could not remove %s: %s"
                          % (self.fonts_link, error))
            self.fonts_link = None

        if self.target and os.path.exists(self.target):
            os.remove(self.target)
        if self.backup and os.path.exists(self.backup):
            os.replace(self.backup, self.target)
        return False


class Recorder:
    """
    Writes down every invocation this run made, for tests/parity/ to compare
    against the same run of another implementation.

    Why this lives here rather than in a corpus of its own: the command
    vocabulary worth comparing is the one the checks already use. A separate
    list would be a second thing to keep in step, and the first time it fell
    behind nobody would notice -- it has no assertions to fail.

    The format is line-oriented like tests/conformance/, so a transcript can be
    read and diffed by eye. Streams are base64 because they are bytes: whether
    a line break came through as LF or CRLF is the whole point, so nothing here
    may normalise or re-encode them.
    """

    VERSION = 1

    NOT_COMPARED = ("--time", "--logtime", "--logmem", "--progress", "-x",
                    "--debug-measure-io")
    """
    Switches whose output is contract by shape but not by content.

    tests/conformance/README.md excludes --time, --logtime, --progress and
    --logmem for the obvious reason: their figures differ every run. Two more
    belong with them and are not named there:

      -x                  reports the engine's memory status, so its figures
                          are per-process and differ between implementations
                          even when nothing is wrong. A handle count of 229
                          against 138 says only that these are two different
                          programs.
      --debug-measure-io  counts the reads and seeks an implementation made,
                          which is a measurement *of* the implementation. Note
                          that it is excluded because it cannot be contract,
                          not because its answers are uninteresting: C# and
                          C++17 currently report the same reads and the same
                          bytes for one document, and C++17 twice the seeks.

    The invocation is still recorded, and its exit code and the files it wrote
    are still compared. Only the streams are set aside, because the alternative
    --- a rule that knows `Handle Count:` has a volatile number after it ---
    would be the list of content-aware regexes this design avoids everywhere
    else, and every entry would be a place for a real difference to hide.

    --time was here from the start and --logtime was not, which cost a run of
    the parity comparison reddening at random on a per-document duration
    nobody had asked it to compare. The two are the same figure at different
    granularities, and the gap survived because compare.py's --repeat mask
    appeared to cover it: that mask holds a line volatile only when two runs
    of the reference disagree on it, which is right for a temporary path and
    wrong for a duration, since two runs of one binary print the same
    milliseconds often enough to leave the line unmasked. Note that an entry
    here need not have a caller --- --time and --progress have none either.
    The list is read when a check is written, which is the point at which the
    cost of leaving a switch out of it is cheapest to avoid.
    """

    def __init__(self, path, implementation):
        self.handle = open(path, "w", encoding="utf-8", newline="\n")
        self.handle.write("# docfilters parity transcript v%d\n" % self.VERSION)
        self.handle.write("implementation %s\n" % implementation)
        self.case = None
        self.sequence = 0

    def start(self, case):
        self.case = case
        self.sequence = 0

    def skipped(self, case, reason):
        """
        Records that a check did not run here, and why.

        Without this a skip is indistinguishable from a check that vanished:
        compare.py would see records the reference has and this implementation
        does not, which is exactly what a case lost in translation looks like
        and exactly what it is there to catch. Writing the skip down lets it
        tell the two apart, and carries the reason into the comparison's own
        report rather than leaving it in a log nobody kept.
        """
        # Two lines rather than one: both a check's name and a reason are
        # English with spaces in, and no separator that could join them is
        # guaranteed to be absent from either.
        self.handle.write("\nskipped-check %s\n" % case)
        self.handle.write("skipped-because %s\n" % reason)

    def record(self, argv, code, out, err, out_dir):
        self.sequence += 1
        write = self.handle.write
        write("\n")
        # The key: which check, and which invocation within it. Position alone
        # would misalign the moment one implementation skips a check the others
        # ran, and then every later record would be compared against the wrong
        # one.
        write("case %s #%d\n" % (self.case, self.sequence))
        write("argv %s\n" % " ".join(argv))
        write("exit %d\n" % code)

        excluded = [switch for switch in self.NOT_COMPARED if switch in argv]
        if excluded:
            write("streams-not-compared %s\n" % " ".join(excluded))
        else:
            write("stdout %s\n" % base64.b64encode(out).decode("ascii"))
            write("stderr %s\n" % base64.b64encode(err).decode("ascii"))
        for relative, digest in sorted(hash_tree(out_dir).items()):
            write("file %s %s\n" % (relative, digest))

    def close(self):
        self.handle.close()


def hash_tree(directory):
    """
    Every file under directory by path relative to it, with its SHA-256.

    Paths use forward slashes so a Windows run and a POSIX one of the same
    implementation produce comparable names.
    """
    digests = {}
    for root, _, names in os.walk(directory):
        for name in names:
            if name == MARKER:
                continue
            absolute = os.path.join(root, name)
            relative = os.path.relpath(absolute, directory).replace(os.sep, "/")
            try:
                with open(absolute, "rb") as handle:
                    digests[relative] = hashlib.sha256(handle.read()).hexdigest()
            except OSError:
                # A file the tool still holds open is not evidence of anything.
                digests[relative] = "unreadable"
    return digests


class Result:
    """One CLI invocation: what it was asked, and everything it produced."""

    def __init__(self, argv, code, out, err):
        self.argv = argv
        self.code = code
        self.out = out
        self.err = err

    def require(self, expected_code):
        if self.code != expected_code:
            raise CheckFailed(
                "expected exit %d, got %d\n  stderr: %s"
                % (expected_code, self.code, self.err.strip()[:400] or "(empty)"))
        return self

    def require_stdout(self, needle):
        if needle not in self.out:
            raise CheckFailed(
                "stdout did not contain %r\n  stdout: %s"
                % (needle, self.out.strip()[:400] or "(empty)"))
        return self

    def require_stderr(self, needle):
        if needle not in self.err:
            raise CheckFailed(
                "stderr did not contain %r\n  stderr: %s"
                % (needle, self.err.strip()[:400] or "(empty)"))
        return self

    def json(self):
        try:
            return json.loads(self.out)
        except ValueError as error:
            raise CheckFailed("stdout was not valid JSON (%s)\n  stdout: %s"
                              % (error, self.out.strip()[:400] or "(empty)"))


class Runner:
    """Invokes the CLI under test and keeps the shared arguments out of the checks."""

    def __init__(self, launcher, license_key, out_dir, verbose, environment=None):
        self.launcher = list(launcher)
        """
        How to start the CLI, as the front of its argv.

        A list rather than a path, because one of the two implementations is not an
        executable: the Python port is started as `python -m docfilters_cli`, which is
        three argv entries before the first argument a check passes. Everything else
        here treats it as opaque.
        """

        self.license_key = license_key
        self.out_dir = out_dir
        self.verbose = verbose

        self.recorder = None
        """
        Where to write a transcript of what this run did, or None.

        Set by --record. The checks know nothing about it: it observes them
        rather than being something they call, which is what lets the parity
        comparison reuse this suite's command vocabulary without a second copy
        of it existing anywhere.
        """

        self.case = None
        """The name of the check currently running, so records can be keyed by it."""

        self.environment = environment
        """
        The child's environment, or None to inherit this process's.

        The Python port needs PYTHONPATH to find its own package and the binding, and
        DF_PATH to find the native library -- which the built executable gets for free,
        the library sitting beside it. Set once here rather than per invocation, because
        `version` is the one command that binds no arguments at all (it takes the
        reference's default SharedOptions, so not even --library reaches it) and so the
        environment is the only way to tell it where the engine is.
        """

    def describe(self):
        """The launcher as a command line, for the run header."""
        return " ".join(self.launcher)

    def run(self, *args, **kwargs):
        """
        Runs the CLI with --license-key appended, unless with_license=False.

        The key is appended rather than prepended so a check can still exercise
        argument order, and it is redacted from anything printed: this script is
        meant to be pasteable into a bug report.

        record=False runs the command without writing it to the transcript. It
        is for a command a check runs to decide *whether* it can run, rather
        than to assert anything --- tests/parity/ compares every recorded
        command byte for byte across four implementations, so a measurement
        placed in front of it becomes contract, and a measurement is the one
        thing four implementations cannot agree on. Use it for nothing else: a
        command whose output a check actually asserts on belongs in the
        transcript, and Recorder.NOT_COMPARED is how a recorded command's
        volatile *streams* are set aside while its exit code and its files
        stay compared.
        """
        argv = list(self.launcher) + [str(a) for a in args]
        if kwargs.get("with_license", True) and self.license_key:
            argv += ["--license-key", self.license_key]

        if self.verbose:
            print("    $ " + " ".join(self.redact(a) for a in argv))

        # Captured as bytes and decoded here rather than by subprocess, which is
        # the only way to keep both things this needs at once.
        #
        # The checks want text with newlines already normalised, which is what
        # text mode gave them. A transcript wants the bytes exactly as written,
        # because whether a line break inside engine text arrived as LF or CRLF
        # is precisely the kind of drift tests/parity/ exists to catch -- and
        # text mode erases it before anything can look.
        #
        # UTF-8 rather than the host's locale encoding, because that is what
        # every implementation writes to a redirected stream: .NET writes a
        # redirected stdout as UTF-8 on every platform, and the Python port puts
        # its streams into UTF-8 for that reason. errors='replace' so a failure
        # is still reportable rather than a traceback in the harness.
        completed = subprocess.run(argv, capture_output=True, env=self.environment,
                                   timeout=kwargs.get("timeout", 300))

        def as_text(raw):
            # Universal newlines, exactly as text mode applied them.
            return raw.decode("utf-8", "replace").replace("\r\n", "\n").replace("\r", "\n")

        # The record=False test is here rather than inside Recorder.record so
        # that the sequence number is not spent either: that number is half the
        # alignment key, and a gap in it would shift every later record of the
        # check against the other implementations' records of the same check.
        if self.recorder is not None and kwargs.get("record", True):
            self.recorder.record([self.redact(a) for a in argv[len(self.launcher):]],
                                 completed.returncode, completed.stdout,
                                 completed.stderr, self.out_dir)

        return Result(argv, completed.returncode,
                      as_text(completed.stdout), as_text(completed.stderr))

    def redact(self, argument):
        if self.license_key and argument == self.license_key:
            return "***"
        return argument

    def out(self, *parts):
        return os.path.join(self.out_dir, *parts)

    def clear_output(self):
        """
        Empties the output directory between checks, so each one sees only what
        it produced and a glob cannot pick up a neighbour's files.
        """
        for entry in os.listdir(self.out_dir):
            if entry == MARKER:
                continue
            path = os.path.join(self.out_dir, entry)
            shutil.rmtree(path) if os.path.isdir(path) else os.remove(path)


# --------------------------------------------------------------------------
# assertions on what landed on disk
# --------------------------------------------------------------------------

def one_file(pattern):
    """The single file matching pattern; fails loudly when it is not exactly one."""
    matches = sorted(glob.glob(pattern))
    if len(matches) != 1:
        raise CheckFailed("expected exactly one file matching %s, found %d%s"
                          % (pattern, len(matches),
                             (": " + ", ".join(os.path.basename(m) for m in matches)) if matches else ""))
    return matches[0]


def read_text(path):
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read()


def read_bytes(path):
    with open(path, "rb") as handle:
        return handle.read()


def significant_words(text):
    """
    The long words in text, control codes counting as separators.

    Long enough to be distinctive rather than shared boilerplate, and a set
    rather than a sequence, because this answers "are these two renderings of
    one document carrying the same content" -- not "are they the same bytes".
    """
    return set(re.findall(r"[A-Za-z]{6,}", "".join(c if c >= " " else " " for c in text)))


def require_nonempty(path, minimum=64):
    size = os.path.getsize(path)
    if size < minimum:
        raise CheckFailed("%s is %d bytes, expected at least %d"
                          % (os.path.basename(path), size, minimum))
    return path


def require_magic(path, magic, description):
    head = read_bytes(path)[:len(magic)]
    if head != magic:
        raise CheckFailed("%s does not start with the %s signature (got %r)"
                          % (os.path.basename(path), description, head))
    return path


# --------------------------------------------------------------------------
# the checks
#
# Each takes the runner and raises CheckFailed with an explanation. The output
# directory is emptied before each one.
# --------------------------------------------------------------------------

def check_formats_lists_the_catalogue(run, doc):
    result = run.run("formats").require(EXIT_OK)
    if "name=" not in result.out:
        raise CheckFailed("formats printed no entries")


def check_formats_json_parses(run, doc):
    entries = run.run("formats", "--json").require(EXIT_OK).json()
    if not isinstance(entries, list) or not entries:
        raise CheckFailed("formats --json was not a non-empty array")


def check_version_reports_the_engine_not_itself(run, doc):
    """
    BUG: `version` looked for the native library beside the executable, but NuGet
    restores it to runtimes/<rid>/native/, so it was never found -- and the
    fallback reported the *binding assembly's* version instead. The result was a
    plausible-looking "Document Filters 1.0.0.0" that was really the tool's own
    version wearing the engine's label.

    Nothing caught it because this is the one command that needs the real library
    and no check ran it. Asserting "exit 0 and two lines" would still pass today;
    what distinguishes the two is that the engine's version is not the tool's.
    """
    result = run.run("version").require(EXIT_OK)
    lines = [line for line in result.out.splitlines() if line.strip()]
    if len(lines) != 2:
        raise CheckFailed("version printed %d lines, expected 2: %r" % (len(lines), lines))

    tool, engine = lines
    if not engine.startswith("Document Filters "):
        raise CheckFailed("second line was %r, expected it to name Document Filters" % engine)

    engine_version = engine[len("Document Filters "):].strip()
    tool_version = tool.split(" ", 1)[1].strip() if " " in tool else ""

    # A file version is a PE resource. There is no ELF or Mach-O equivalent to
    # read, so "unknown" is the correct answer off Windows -- the C++ port returns
    # it unconditionally there -- and only a failed lookup on it, where the
    # library is present by definition in an end-to-end run.
    if engine_version in ("", "unknown"):
        if sys.platform == "win32":
            raise CheckFailed("the engine version reads %r, so the native library was "
                              "not located" % engine_version)
        raise CheckSkipped("a file version is a PE resource, so there is none to read "
                           "on this platform")

    # The half that holds everywhere: whatever the lookup found, it must not be
    # the tool's own version wearing the engine's label.
    if engine_version == tool_version:
        raise CheckFailed("the engine version (%r) is the tool's own version -- the "
                          "library lookup fell back instead of finding it"
                          % engine_version)


def check_options_json_parses(run, doc):
    entries = run.run("options", "--json").require(EXIT_OK).json()
    if not isinstance(entries, list) or not entries:
        raise CheckFailed("options --json was not a non-empty array")


def check_identify_recognises_the_input(run, doc):
    """
    Asserts the record's shape rather than one document's format, so the suite
    works against whatever --input names.
    """
    result = run.run("identify", doc).require(EXIT_OK)

    fields = dict(re.findall(r"(\w+)=(\S[^=]*?)(?=\s+\w+=|$)", result.out.strip()))
    for name in ("format", "mime", "name"):
        if not fields.get(name, "").strip():
            raise CheckFailed("identify reported no %s: %s" % (name, result.out.strip()[:120]))


def check_identify_json_parses(run, doc):
    entries = run.run("identify", "--json", doc).require(EXIT_OK).json()
    if not entries or "path" not in entries[0]:
        raise CheckFailed("identify --json had no path field")


def check_meta_reports_fields(run, doc):
    result = run.run("meta", doc).require(EXIT_OK)
    if "field=" not in result.out:
        raise CheckFailed("meta printed no fields")


def check_meta_json_parses(run, doc):
    run.run("meta", "--json", doc).require(EXIT_OK).json()


def check_list_enumerates_subfiles(run, doc):
    # A PDF's embedded images surface as subfiles, so this input exercises the
    # walk without needing a container document.
    run.run("list", doc).require(EXIT_OK)


@needs_license("the HTML canvas is refused in licenseless evaluation")
def check_html5_inlines_its_page_images(run, doc):
    """
    BUG (D35): the HTML canvas rasterises each page into a sibling image and lays
    a *transparent* text layer over it. This tool writes only the markup, so
    without HTML_INLINE_IMAGES=ON the sibling never landed and every page
    rendered blank -- text present in the geometry, invisible on screen.

    Asserting "the file exists and is large" would have passed throughout. What
    distinguishes the two is where the raster lives.
    """
    run.run("convert", "--to", "html5", "-o", run.out_dir, doc).require(EXIT_OK)
    markup = read_text(require_nonempty(one_file(run.out("*.html")), minimum=1024))

    # Always true: this tool never writes the sibling, so a reference to one is
    # a blank page whatever the document contains.
    if "url(background_page" in markup:
        raise CheckFailed("markup references background_page_N.png, which is never written "
                          "-- pages will render blank")

    # Only a document with raster content can show that inlining reached the
    # canvas; a text-only page renders as markup with nothing to inline.
    if "data:image" not in markup:
        raise CheckSkipped("this document renders with no page raster, so there is "
                           "nothing for HTML_INLINE_IMAGES to inline")


@needs_license("the HTML canvas is refused in licenseless evaluation")
def check_the_html5_output_identifies_as_html(run, doc):
    """The tool's own output, fed back to the tool: a round trip through both paths."""
    run.run("convert", "--to", "html5", "-o", run.out_dir, doc).require(EXIT_OK)
    produced = one_file(run.out("*.html"))

    result = run.run("identify", produced).require(EXIT_OK)
    if "html" not in result.out.lower():
        raise CheckFailed("the produced .html was not identified as HTML: %s" % result.out.strip())


@needs_license("the SVG canvas is refused in licenseless evaluation")
def check_svg_inlines_its_page_images(run, doc):
    """BUG (D35), the SVG half: SVG_INLINE_IMAGES defaults off the same way."""
    run.run("convert", "--to", "svg", "-o", run.out_dir, "--pages", "1", doc).require(EXIT_OK)
    markup = read_text(require_nonempty(one_file(run.out("*.svg"))))

    if "data:image" in markup or "<image" in markup:
        return

    # No raster inline. Either the page has none, or it was written beside the
    # markup and left behind -- the same failure as the HTML half.
    orphan = [os.path.basename(f) for f in glob.glob(run.out("*.png")) + glob.glob(run.out("*.jpg"))]
    if orphan:
        raise CheckFailed("the SVG's raster was written as a sibling instead of inlined: %s"
                          % ", ".join(orphan))

    raise CheckSkipped("this document renders with no page raster, so there is "
                       "nothing for SVG_INLINE_IMAGES to inline")


def check_a_non_ascii_name_survives_the_command_line(run, doc):
    """
    BUG: the C++ port used the narrow main(), so Windows handed it the command
    line converted through the active ANSI code page -- a filename the code page
    cannot spell arrived as question marks and the run reported E_NO_INPUTS for a
    file that was plainly there. std::filesystem did the same thing to every path
    on the way back out. The C# port was never affected, .NET strings being UTF-16
    throughout, which is exactly why only an end-to-end check covers both.

    The name is written in escapes so this file stays ASCII, and the document is a
    copy of the real input, so what is tested is the name and nothing else.
    """
    name = "\u65e5\u672c\u8a9e" + os.path.splitext(doc)[1]
    renamed = run.out(name)
    shutil.copyfile(doc, renamed)

    out_dir = run.out("\u51fa\u529b")
    os.makedirs(out_dir)
    run.run("convert", "--to", "text", "-o", out_dir, renamed).require(EXIT_OK)

    written = os.listdir(out_dir)
    if len(written) != 1:
        raise CheckFailed("expected one output, got %r" % (written,))

    if os.path.splitext(written[0])[0] != os.path.splitext(name)[0]:
        raise CheckFailed("the output was named %r, not after its input %r"
                          % (written[0], name))


def check_text_conversion_extracts_words(run, doc):
    run.run("convert", "--to", "text", "-o", run.out_dir, doc).require(EXIT_OK)
    text = read_text(require_nonempty(one_file(run.out("*.txt"))))

    if not re.search(r"[A-Za-z]{4,}", text):
        raise CheckFailed("the extracted text contains no words")


@needs_license("IGR_FORMAT_HTML is refused in licenseless evaluation")
def check_flow_html_is_markup_not_plain_text(run, doc):
    """
    BUG (D36): --to html set IGR_FORMAT_HTML, and the binding's
    Open(OpenMode, OpenType, ...) overload masked bits 16-31 away and substituted
    IGR_FORMAT_TEXT. The command produced plain text under a .html name, which
    looks plausible enough to ship.
    """
    run.run("convert", "--to", "html", "-o", run.out_dir, doc).require(EXIT_OK)
    produced = read_text(require_nonempty(one_file(run.out("*.html"))))

    if not re.search(r"<\s*(html|body|p|div|span|table)\b", produced, re.IGNORECASE):
        raise CheckFailed("--to html produced no HTML markup -- the format flag "
                          "is not reaching the engine")


def check_md_text_differs_from_plain_text(run, doc):
    """
    BUG (D36), the markdown half: IGR_FORMAT_MARKDOWN was discarded the same way.

    Compared against the text output rather than searched for markdown syntax: a
    document with no headings or emphasis need not produce any, but the two
    readers cannot produce byte-identical output.
    """
    run.run("convert", "--to", "text", "-o", run.out_dir, doc).require(EXIT_OK)
    plain = read_text(one_file(run.out("*.txt")))

    run.clear_output()
    run.run("convert", "--to", "md-text", "-o", run.out_dir, doc).require(EXIT_OK)
    # No size floor: a document whose body is empty legitimately produces no
    # markdown, and would still differ from the text reader's metadata block.
    markdown = read_text(one_file(run.out("*.md")))

    if markdown == plain:
        raise CheckFailed("--to md-text produced exactly the plain-text output -- "
                          "the markdown reader is not being selected")


#: The ISYS control codes doc2text's processOutput rewrites or drops, and that
#: --to text is supposed to strip unless --include-control-codes asks for them.
CONTROL_CODES = {0x01: "start-of-heading", 0x02: "normal", 0x03: "bold", 0x04: "italic",
                 0x05: "underline", 0x06: "start-of-hit", 0x0C: "soft break",
                 0x0E: "paragraph", 0x0F: "start-of-meta", 0x10: "end-of-meta"}


def found_control_codes(text):
    present = sorted({ord(c) for c in text if ord(c) in CONTROL_CODES})
    return ", ".join("0x%02X %s" % (code, CONTROL_CODES[code]) for code in present)


def check_control_codes_are_stripped_unless_asked_for(run, doc):
    """
    BUG: --to text emitted the raw ISYS control codes, always. The tool tried to
    strip them by *omitting* IGR_FORMAT_TEXT at open, which cannot work -- the
    engine maps a missing format bit and IGR_FORMAT_TEXT to the same kText mode
    (ISYSreaderImpl.cpp: `output_type == IGR_FORMAT_TEXT || output_type == 0`),
    so --include-control-codes had no lever to pull. Stripping is doc2text's
    processOutput table, applied in TextConversion.
    """
    run.run("convert", "--to", "text", "-o", run.out_dir, doc).require(EXIT_OK)
    default = read_text(one_file(run.out("*.txt")))

    run.clear_output()
    run.run("convert", "--to", "text", "--include-control-codes",
            "-o", run.out_dir, doc).require(EXIT_OK)
    raw = read_text(one_file(run.out("*.txt")))

    leaked = found_control_codes(default)
    if leaked:
        raise CheckFailed("--to text left ISYS control codes in its output (%s)" % leaked)

    if not found_control_codes(raw):
        raise CheckFailed("--include-control-codes stripped them anyway, so the flag "
                          "is not reaching TextConversion")


def check_a_missing_output_directory_is_created(run, doc):
    """
    BUG, found from the README's own first example: --out-dir pointed at a
    directory that did not exist yet worked for --to text and failed for every
    other format. Only the text path created it; a canvas and a docpack are
    opened by the engine, which will not, and reports it as
    "FileStream::Create ... The system cannot find the path specified" -- a
    per-document error that reads as though the *input* were missing.

    Needs a real disk to mean anything: InMemoryFileSystem.CreateFile creates
    parent directories on its own, so the unit suite cannot see this.
    """
    for target, extension in (("pdf", "pdf"), ("png", "png"), ("text", "txt"), ("docpack", "docpack")):
        run.clear_output()
        nested = run.out("brand", "new")
        run.run("convert", "--to", target, "-o", nested, "--pages", "1", doc).require(EXIT_OK)

        if not glob.glob(os.path.join(nested, "*." + extension)):
            raise CheckFailed("--to %s wrote nothing into a directory that did not exist yet" % target)


def check_combining_text_formats_concatenates(run, doc):
    """
    BUG: --combine is allowed for text formats, which have no canvas, but the
    combined path read Format.CanvasType.Value unconditionally. That is not a
    CliError, so it escaped the handler entirely -- the process died with an
    unhandled System.InvalidOperationException and exit code 0xE0434352, not any
    documented exit code.
    """
    single = run.out("one.txt")
    run.run("convert", "--to", "text", "-o", run.out_dir, doc).require(EXIT_OK)
    alone = os.path.getsize(one_file(run.out("*.txt")))

    run.clear_output()
    copy = run.out("second.pdf")
    shutil.copyfile(doc, copy)
    run.run("convert", "--to", "text", "--combine", "--out-file", single, doc, copy).require(EXIT_OK)

    combined = os.path.getsize(require_nonempty(single))
    if combined < alone * 2 * 0.9:
        raise CheckFailed("--combine --to text produced %d bytes, expected roughly two "
                          "documents' worth (%d)" % (combined, alone * 2))


def check_metadata_is_one_record_per_field(run, doc):
    """
    BUG: the engine separates metadata records with ISYS control codes -- 0x0F
    before the block, 0x0E between records -- not newlines. Splitting the raw
    text on '
' therefore found no boundary and reported every field of a
    document as a single record, with the 0x0F still stuck to the first label.

    Needs a real document: the fake returns whatever text a test gives it, so
    only the engine produces the separators that were being missed.
    """
    result = run.run("meta", doc).require(EXIT_OK)

    records = [line for line in result.out.splitlines() if "field=" in line]
    if len(records) < 2:
        raise CheckFailed("meta reported %d record(s) for a document with several "
                          "metadata fields -- the records are not being split"
                          % len(records))

    for line in records:
        leaked = found_control_codes(line)
        if leaked:
            raise CheckFailed("a metadata record still carries control codes (%s): %s"
                              % (leaked, line[:120]))


def check_a_per_document_timeout_is_enforced(run, doc):
    """
    BUG: Diagnostics.EndDocument discarded the per-document timer, and every
    caller runs CheckDocumentTimeout immediately after it, so the check's null
    guard was always true and --timeout never fired for any document however
    long it ran.

    Self-calibrating, because --timeout's smallest legal value is 1 second and
    the default sample renders in well under that. Times the document first and
    skips when it is too quick to prove anything.

    The calibration run is recorded nowhere, and that is not tidiness.
    --logtime prints a wall clock duration, tests/parity/ compares recorded
    stdout byte for byte, and four implementations will never agree on how
    long a PDF took: measured on one runner, C++17 came in at 0.573s, the
    reference at 0.595s and the JVM at 0.601s, an ordering that is the
    runtimes rather than noise. compare.py's --repeat mask does not cover it
    either. That mask holds a line volatile only when two runs of the
    reference disagree on it, which is exactly right for a temporary path or a
    docpack timestamp --- they differ every run --- and exactly wrong for a
    duration, which is stable within one implementation and systematically
    different between them. Two runs of the same binary printed the same
    milliseconds in 2 of 11 consecutive pairs when this was measured, so the
    comparison reddened at random until the command stopped being recorded.
    """
    measured = run.run("convert", "--to", "png", "-o", run.out_dir, "--logtime", doc,
                       record=False).require(EXIT_OK)

    seconds = re.search(r":\s*([0-9.]+)s\s*$", measured.out.strip().splitlines()[-1])
    if not seconds:
        raise CheckFailed("--logtime printed no elapsed time to calibrate against")

    elapsed = float(seconds.group(1))
    if elapsed < 2.0:
        raise CheckSkipped("this document renders in %.2fs; --timeout's smallest value is 1s, "
                           "so it needs one taking 2s or more (pass --input)" % elapsed)

    run.clear_output()
    run.run("convert", "--to", "png", "-o", run.out_dir, "--timeout", "1", doc)        .require(EXIT_LIMIT_EXCEEDED).require_stderr("E_LIMIT_EXCEEDED")


def check_diagnostic_switches_do_something(run, doc):
    """
    Nine arguments used to parse and then do nothing at all. These are the ones
    whose effect only a real engine can show.

    -x in particular: the binding's own DocumentFilters.GetMemoryStatus() cannot
    be called -- it declares the length as [MarshalAs(SysUInt)] uint, an illegal
    pairing, so it throws MarshalDirectiveException before reaching the engine.
    This tool redeclares the entry point, and that workaround is only provable
    against the real library.
    """
    def convert(*args):
        run.clear_output()
        return run.run("convert", "--to", "text", "-o", run.out_dir, *(list(args) + [doc])).require(EXIT_OK)

    quiet = convert()
    if quiet.out.strip():
        raise CheckFailed("a run with no diagnostic switch printed something: %s" % quiet.out.strip()[:80])

    if "Process Memory" not in convert("-x").out:
        raise CheckFailed("-x printed no engine memory report")

    if not re.search(r"peak memory: \d+MB", convert("--logmem").out):
        raise CheckFailed("--logmem printed no process peak")

    if not re.search(r"reads=\d+ seeks=\d+ bytes=\d+", convert("--debug-measure-io").out):
        raise CheckFailed("--debug-measure-io printed no I/O counts")


def check_a_truncated_input_is_read_only_that_far(run, doc):
    """
    --debug-truncate hands the engine a prefix, so a document large enough to
    need more than that cannot open. Failing is the switch working.
    """
    whole = run.run("convert", "--to", "text", "-o", run.out_dir, doc).require(EXIT_OK)
    intact = os.path.getsize(one_file(run.out("*.txt")))

    run.clear_output()
    truncated = run.run("convert", "--to", "text", "-o", run.out_dir, "--debug-truncate", "2048", doc)

    if truncated.code == EXIT_OK:
        produced = glob.glob(run.out("*.txt"))
        shortened = os.path.getsize(produced[0]) if produced else 0
        if shortened >= intact:
            raise CheckFailed("--debug-truncate 2048 produced as much text as the whole document "
                              "(%d vs %d bytes), so the prefix was not honoured" % (shortened, intact))
        return

    truncated.require_stderr("E_DOCUMENT_FAILED")


@needs_license("the JSON canvas is refused in licenseless evaluation")
def check_mdast_differs_from_json(run, doc):
    """Both are the JSON canvas; JSON_OUTPUT_SCHEMA=MDAST is the only difference."""
    run.run("convert", "--to", "json", "-o", run.out_dir, doc).require(EXIT_OK)
    plain = read_text(one_file(run.out("*.json")))
    json.loads(plain)

    run.clear_output()
    run.run("convert", "--to", "mdast", "-o", run.out_dir, doc).require(EXIT_OK)
    mdast = read_text(one_file(run.out("*.json")))
    json.loads(mdast)

    if mdast == plain:
        raise CheckFailed("--to mdast produced plain JSON -- the schema option was dropped")


def check_png_writes_one_file_per_page(run, doc):
    run.run("convert", "--to", "png", "-o", run.out_dir, "--pages", "1-2", doc).require(EXIT_OK)
    produced = sorted(glob.glob(run.out("*.png")))

    if len(produced) != 2:
        raise CheckFailed("expected 2 PNGs for --pages 1-2, got %d" % len(produced))
    for path in produced:
        require_magic(require_nonempty(path, minimum=256), b"\x89PNG", "PNG")


def check_pdf_round_trips(run, doc):
    run.run("convert", "--to", "pdf", "-o", run.out_dir, "--pages", "1-2", doc).require(EXIT_OK)
    produced = require_magic(require_nonempty(one_file(run.out("*.pdf")), minimum=1024), b"%PDF", "PDF")

    result = run.run("identify", produced).require(EXIT_OK)
    result.require_stdout("mime=application/pdf")


def check_tiff_is_one_file_for_many_pages(run, doc):
    run.run("convert", "--to", "tiff", "-o", run.out_dir, doc).require(EXIT_OK)
    produced = require_nonempty(one_file(run.out("*.tif")), minimum=1024)

    head = read_bytes(produced)[:4]
    if head not in (b"II*\x00", b"MM\x00*"):
        raise CheckFailed("%s is not a TIFF (got %r)" % (os.path.basename(produced), head))


def check_docpack_describes_the_document(run, doc):
    """
    The only check that opens a pack rather than noting that one appeared.

    Before it, --to docpack's whole end-to-end coverage was one iteration of
    check_a_missing_output_directory_is_created, which asserts that a file with
    that extension exists -- so an empty archive, or one holding a different
    document's text, passed. The unit suites can see less again: the fake engine
    records the name make_doc_pack was handed and writes nothing at all.

    A pack is a zip of XML parts wrapped around one or more plain-text chunks,
    so what is worth asserting is that the parts refer to each other correctly
    and that the text inside is this document's.
    """
    run.run("convert", "--to", "docpack", "-o", run.out_dir, doc).require(EXIT_OK)
    produced = require_magic(require_nonempty(one_file(run.out("*.docpack")), minimum=1024),
                             b"PK\x03\x04", "zip")

    # Read it all out now: the text comparison below needs the output directory.
    with zipfile.ZipFile(produced) as pack:
        entries = pack.namelist()
        sizes = {name: pack.getinfo(name).file_size for name in entries}
        parts = {name: pack.read(name).decode("utf-8", "replace")
                 for name in entries if name.endswith(".xml")}
        packed_text = "".join(pack.read(name).decode("utf-8", "replace")
                              for name in entries if name.startswith("text/"))

    for required in ("df-manifest.xml", "df-core.xml", "df-document-text.xml", "df-metadata.xml"):
        if required not in parts:
            raise CheckFailed("the pack has no %s (it holds %s)"
                              % (required, ", ".join(entries) or "nothing"))

    # Every part the manifest and the text index name has to be in the archive, at
    # the size it claims. A dangling reference is what a reader trips over first
    # and is exactly what an existence check cannot see.
    for referrer, attribute in (("df-manifest.xml", "name"), ("df-document-text.xml", "src")):
        for referenced in re.findall(r'%s="([^"]+)"' % attribute, parts[referrer]):
            if referenced not in entries:
                raise CheckFailed("%s names %s, which is not in the pack (it holds %s)"
                                  % (referrer, referenced, ", ".join(entries)))

    for chunk, declared in re.findall(r'src="([^"]+)"[^>]*size="(\d+)"', parts["df-document-text.xml"]):
        if sizes[chunk] != int(declared):
            raise CheckFailed("df-document-text.xml declares %s as %s bytes, but it is %d"
                              % (chunk, declared, sizes[chunk]))

    if os.path.basename(doc) not in parts["df-core.xml"].replace("\\", "/"):
        raise CheckFailed("df-core.xml does not name the document the pack was made from (%s): %s"
                          % (os.path.basename(doc), parts["df-core.xml"]))

    # And the text in it is this document's text. The pack keeps the ISYS control
    # codes that --to text strips, so the two cannot be compared as strings; they
    # are compared as words, and on the sample documents the sets come out equal.
    run.clear_output()
    run.run("convert", "--to", "text", "-o", run.out_dir, doc).require(EXIT_OK)
    extracted = read_text(one_file(run.out("*.txt")))

    missing = sorted(significant_words(extracted) - significant_words(packed_text))
    if missing:
        raise CheckFailed("the pack's text is missing %d word(s) that --to text extracted "
                          "from the same document: %s" % (len(missing), ", ".join(missing[:8])))


def check_stdout_streams_instead_of_writing(run, doc):
    result = run.run("convert", "--to", "text", "--stdout", doc).require(EXIT_OK)

    if not re.search(r"[A-Za-z]{4,}", result.out):
        raise CheckFailed("--stdout produced no text on stdout")
    if os.listdir(run.out_dir) != [MARKER]:
        raise CheckFailed("--stdout wrote files to disk as well")


def check_structure_reports_are_printed(run, doc):
    """
    Reports read page structures, which only exist in paginated mode (D36). A
    text-mode handle reports nothing rather than failing, so an exit code of 0
    proves nothing here -- the output has to be checked.
    """
    result = run.run("convert", "--to", "pdf", "-o", run.out_dir, "--fonts", "--dimensions",
                     doc).require(EXIT_OK)

    if "font=" not in result.out and "width=" not in result.out:
        raise CheckFailed("neither --fonts nor --dimensions reported anything")


def check_bookmarks_alongside_text_conversion(run, doc):
    """
    BUG (D39), found in two parts, and neither direction works. First:
    --bookmarks forced paginated mode unconditionally, taking the text reader
    away from a --to text conversion opened in the same call -- the engine
    refused with IGR_E_NOT_VALID_FOR_THIS_CLASS once the text reader was asked
    for text, reported as "E_DOCUMENT_FAILED: Unknown Exception: 12", and left a
    zero-byte .txt behind. Second, once bookmarks stopped forcing image mode: a
    bookmark's page number only exists once the document has been laid out, and
    RootBookmark does not degrade to "nothing" on its own for a handle that was
    never laid out -- it throws the binding's own "Invalid DocHandle" instead.

    So one open cannot serve both, and the pairing is refused up front (D42).
    """
    run.run("convert", "--to", "text", "--bookmarks", "-o", run.out_dir, doc) \
       .require(EXIT_USAGE).require_stderr("E_REPORT_NEEDS_HIDEF")

    if [e for e in os.listdir(run.out_dir) if e != MARKER]:
        raise CheckFailed("the refused run still wrote something")


def check_a_report_alongside_a_canvas_conversion_is_allowed(run, doc):
    """The other half of D42: a canvas format opens paginated anyway, so it is fine."""
    # --dimensions alongside it, because every page has a width and a height
    # while bookmarks are a property of the document -- asserting on bookmarks
    # alone only works for a document that happens to have some.
    result = run.run("convert", "--to", "pdf", "-o", run.out_dir, "--pages", "1",
                     "--bookmarks", "--dimensions", doc).require(EXIT_OK)

    if "width=" not in result.out:
        raise CheckFailed("a report alongside a canvas format produced no output at all")


def check_page_selection_is_honoured(run, doc):
    run.run("convert", "--to", "png", "-o", run.out_dir, "--pages", "2", doc).require(EXIT_OK)
    if len(glob.glob(run.out("*.png"))) != 1:
        raise CheckFailed("--pages 2 did not produce exactly one page")


def check_out_name_template_is_applied(run, doc):
    run.run("convert", "--to", "png", "-o", run.out_dir, "--pages", "1",
            "--out-name", "shot-{page:04}.{ext}", doc).require(EXIT_OK)
    one_file(run.out("shot-0001.png"))


def check_existing_output_is_not_overwritten_silently(run, doc):
    run.run("convert", "--to", "text", "-o", run.out_dir, doc).require(EXIT_OK)
    produced = one_file(run.out("*.txt"))
    before = os.path.getmtime(produced)

    again = run.run("convert", "--to", "text", "-o", run.out_dir, doc)
    if again.code == EXIT_OK:
        raise CheckFailed("a second run overwrote an existing output without --overwrite")

    run.run("convert", "--to", "text", "-o", run.out_dir, "--overwrite", doc).require(EXIT_OK)
    if os.path.getmtime(produced) == before and read_text(produced) == "":
        raise CheckFailed("--overwrite did not rewrite the file")


def check_dry_run_writes_nothing(run, doc):
    run.run("convert", "--to", "png", "-o", run.out_dir, "--dry-run", doc).require(EXIT_OK)
    if os.listdir(run.out_dir) != [MARKER]:
        raise CheckFailed("--dry-run wrote files")


def check_extract_images_pulls_them_out(run, doc):
    """
    BUG: --images iterated IDocument.Images without ever asking the engine to
    populate it. ENUMERATE_IMAGES is an open option, so it has to be set when the
    document is opened; without it that collection is empty and --images found
    nothing.

    Caught only because a Release build was tested. The Debug output directory
    had a stray ISYS11df.ini -- gitignored, not produced by the build -- whose
    [Class:DEFAULT:TEXT] section sets ENUMERATE_IMAGES=ON for anything running
    from that directory. That made the check pass against Debug for the wrong
    reason. --images is asked for explicitly here so the result does not depend
    on an untracked file sitting next to the executable.
    """
    run.run("extract", "--images", "-o", run.out_dir, doc).require(EXIT_OK)

    produced = [entry for entry in os.listdir(run.out_dir) if entry != MARKER]
    if not produced:
        raise CheckSkipped("this document has no embedded images or subfiles to extract")


def check_an_unknown_command_is_a_usage_error(run, doc):
    run.run("frobnicate", doc).require(EXIT_USAGE).require_stderr("E_UNKNOWN_COMMAND")


def check_an_unknown_format_is_a_usage_error(run, doc):
    run.run("convert", "--to", "nope", "-o", run.out_dir, doc) \
       .require(EXIT_USAGE).require_stderr("E_UNKNOWN_FORMAT")


def check_a_missing_input_is_reported(run, doc):
    run.run("identify", os.path.join(run.out_dir, "definitely-not-here.pdf")) \
       .require(EXIT_NO_INPUTS).require_stderr("E_NO_INPUTS")


def check_a_per_page_format_cannot_go_to_one_file(run, doc):
    run.run("convert", "--to", "png", "--out-file", run.out("one.png"), doc) \
       .require(EXIT_USAGE)


def check_out_file_with_several_inputs_is_refused(run, doc):
    """
    The rule the whole output model is built around: --out-file names one exact
    path, so several inputs cannot all go to it. The tool refuses rather than
    guessing which one wins or silently overwriting -- and it refuses before
    writing anything, which is what the plan-before-execute split is for.

    Exit 4 rather than 2: this is an output conflict, not a malformed command
    line, and the distinction is contract.
    """
    run.run("convert", "--to", "text", "--out-file", run.out("one.txt"), doc, doc) \
       .require(EXIT_OUTPUT_CONFLICT).require_stderr("E_OUTFILE_MULTI_INPUT")


def check_no_inputs_at_all_is_reported(run, doc):
    """
    Distinct from naming an input that does not exist: here the command line
    carries no input at all. Both are E_NO_INPUTS and exit 3, which is worth
    pinning -- a port that treated an empty argument list as a usage error
    would be a reasonable guess and the wrong answer.
    """
    run.run("convert", "--to", "text", "--out-dir", run.out_dir) \
       .require(EXIT_NO_INPUTS).require_stderr("E_NO_INPUTS")


def check_a_missing_required_argument_is_a_usage_error(run, doc):
    run.run("convert", "--out-dir", run.out_dir, doc) \
       .require(EXIT_USAGE).require_stderr("E_MISSING_ARGUMENT")


def check_an_option_that_wants_a_value_says_so(run, doc):
    """
    `--max-depth -1` reads as an option followed by another option rather than
    by a negative number, so the complaint is that --max-depth got no value.
    Pinned because it is the kind of thing a parsing library decides for you:
    a port whose parser accepted -1 as a value would diverge here without any
    of its own tests noticing.
    """
    run.run("convert", "--to", "text", "--out-dir", run.out_dir, "--max-depth", "-1", doc) \
       .require(EXIT_USAGE).require_stderr("E_MISSING_VALUE")


def check_the_contract_dump_lists_every_command(run, doc):
    """
    `--print-contract` renders the argument table: every command, every
    argument, every arity.

    Here rather than only in each language's own suite because this is the one
    place the whole argument surface is a single string, which makes the parity
    comparison a complete check on it -- an argument that exists in one
    implementation and not another cannot survive a diff of these. Each
    language's ContractDumpTests can only compare it to its own expectations.
    """
    # A shared option rather than a command of its own, so it needs a verb in
    # front of it -- any verb: it prints the whole surface and stops before the
    # one it was given runs.
    result = run.run("convert", "--print-contract").require(EXIT_OK)
    for command in ("convert", "extract", "list", "identify", "meta",
                    "formats", "options"):
        result.require_stdout("command " + command)


def check_the_contract_dump_json_parses(run, doc):
    """The same surface as JSON, which `help --json` renders from."""
    commands = run.run("help", "--json").require(EXIT_OK).json()
    if not isinstance(commands, list) or not commands:
        raise CheckFailed("help --json was not a non-empty list")
    if not all("command" in entry and "arguments" in entry for entry in commands):
        raise CheckFailed("an entry lacked 'command' or 'arguments'")


def check_a_bad_page_range_is_a_usage_error(run, doc):
    run.run("convert", "--to", "text", "--out-dir", run.out_dir, "--pages", "nonsense", doc) \
       .require(EXIT_USAGE).require_stderr("E_BAD_PAGE_RANGE")


CHECKS = [
    # Catalogues and inspection: no conversion, so these fail first and fastest
    # when the library or the license is wrong.
    ("the contract dump lists every command", check_the_contract_dump_lists_every_command),
    ("the contract dump as JSON parses", check_the_contract_dump_json_parses),
    ("formats lists the catalogue", check_formats_lists_the_catalogue),
    ("formats --json parses", check_formats_json_parses),
    ("version reports the engine, not itself", check_version_reports_the_engine_not_itself),
    ("options --json parses", check_options_json_parses),
    ("identify recognises the input", check_identify_recognises_the_input),
    ("identify --json parses", check_identify_json_parses),
    ("meta reports fields", check_meta_reports_fields),
    ("metadata is one record per field", check_metadata_is_one_record_per_field),
    ("meta --json parses", check_meta_json_parses),
    ("list enumerates subfiles", check_list_enumerates_subfiles),

    # Conversion.
    ("html5 inlines its page images", check_html5_inlines_its_page_images),
    ("the html5 output identifies as HTML", check_the_html5_output_identifies_as_html),
    ("svg inlines its page images", check_svg_inlines_its_page_images),
    ("text conversion extracts words", check_text_conversion_extracts_words),
    ("combining text formats concatenates", check_combining_text_formats_concatenates),
    ("flow html is markup, not plain text", check_flow_html_is_markup_not_plain_text),
    ("md-text differs from plain text", check_md_text_differs_from_plain_text),
    ("control codes are stripped unless asked for", check_control_codes_are_stripped_unless_asked_for),
    ("mdast differs from json", check_mdast_differs_from_json),
    ("png writes one file per page", check_png_writes_one_file_per_page),
    ("pdf round trips", check_pdf_round_trips),
    ("tiff is one file for many pages", check_tiff_is_one_file_for_many_pages),
    ("docpack describes the document", check_docpack_describes_the_document),

    # Options that change where output goes or how much of it there is.
    ("--stdout streams instead of writing", check_stdout_streams_instead_of_writing),
    ("structure reports are printed", check_structure_reports_are_printed),
    ("--bookmarks alongside a text conversion", check_bookmarks_alongside_text_conversion),
    ("a report alongside a canvas conversion", check_a_report_alongside_a_canvas_conversion_is_allowed),
    ("page selection is honoured", check_page_selection_is_honoured),
    ("--out-name template is applied", check_out_name_template_is_applied),
    ("existing output is not overwritten silently", check_existing_output_is_not_overwritten_silently),
    ("--dry-run writes nothing", check_dry_run_writes_nothing),
    ("diagnostic switches do something", check_diagnostic_switches_do_something),
    ("a truncated input is read only that far", check_a_truncated_input_is_read_only_that_far),
    ("a per-document timeout is enforced", check_a_per_document_timeout_is_enforced),
    ("a missing output directory is created", check_a_missing_output_directory_is_created),
    ("extract --images pulls them out", check_extract_images_pulls_them_out),
    ("a non-ASCII name survives the command line", check_a_non_ascii_name_survives_the_command_line),

    # Failure paths. Exit codes are contract (§07), so they are asserted exactly.
    ("an unknown command is a usage error", check_an_unknown_command_is_a_usage_error),
    ("an unknown format is a usage error", check_an_unknown_format_is_a_usage_error),
    ("a missing input is reported", check_a_missing_input_is_reported),
    ("no inputs at all is reported", check_no_inputs_at_all_is_reported),
    ("a per-page format cannot go to one file", check_a_per_page_format_cannot_go_to_one_file),
    ("--out-file with several inputs is refused", check_out_file_with_several_inputs_is_refused),
    ("a missing required argument is a usage error", check_a_missing_required_argument_is_a_usage_error),
    ("an option that wants a value says so", check_an_option_that_wants_a_value_says_so),
    ("a bad page range is a usage error", check_a_bad_page_range_is_a_usage_error),
]


# --------------------------------------------------------------------------
# harness
# --------------------------------------------------------------------------

def parse_skips(parser, requested):
    """
    `--skip "<check>=<reason>"` values as {check: reason}.

    The reason is required rather than optional. A skip is a hole in the
    evidence this suite produces, and one with no reason beside it is a hole
    nobody can later decide to close -- which is how a temporary exclusion
    becomes permanent.

    The check has to be one that exists. A typo would otherwise skip nothing,
    quietly, and the run would look like it had covered everything.
    """
    known = {name for name, _ in CHECKS}
    skips = {}
    for entry in requested:
        check, separator, reason = entry.partition("=")
        check, reason = check.strip(), reason.strip()
        if not separator or not reason:
            parser.error("--skip wants CHECK=REASON, got %r" % entry)
        if check not in known:
            closest = difflib.get_close_matches(check, sorted(known), n=1, cutoff=0.5)
            parser.error("--skip names no such check: %r%s"
                         % (check, "\nDid you mean %r?" % closest[0] if closest else ""))
        skips[check] = reason
    return skips


def find_cli():
    """
    The most recently built executable, wherever the build left it -- C# or
    C++17, whichever was built last.

    Newest wins rather than a fixed order among configurations or languages:
    whichever build was run last is the one being worked on, and preferring
    one by name would silently test a stale binary the rest of the time. The
    chosen path is printed at the top of every run, so which one it picked is
    never a guess.

    Only the ports that build an executable can be found this way. The Python
    port produces nothing to timestamp, so it is never the newest and never the
    default; it is named instead, as --cli python. That is the one asymmetry
    here, and it comes from the ports rather than from this script.
    """
    name = "docfilters.exe" if os.name == "nt" else "docfilters"

    candidates = []
    for configuration in ("Debug", "Release"):
        candidates += glob.glob(os.path.join(REPO_ROOT, "csharp", "bin", configuration, "net*", name))

    # The C++17 build: CMAKE_RUNTIME_OUTPUT_DIRECTORY puts the executable at
    # <build dir>/bin/docfilters regardless of what the build directory itself
    # is named, so this looks under any build* directory rather than assuming
    # cpp17/build specifically -- tests/cpp17/build, which configures cpp17/ as
    # a subdirectory and builds both the tool and its test suite in one tree,
    # is just as valid a place to have built it last.
    for root in ("cpp17", os.path.join("tests", "cpp17")):
        candidates += glob.glob(os.path.join(REPO_ROOT, root, "build*", "bin", name))
        candidates += glob.glob(os.path.join(REPO_ROOT, root, "out", "build", "*", "bin", name))

    candidates = [path for path in candidates if os.path.isfile(path)]
    return max(candidates, key=os.path.getmtime) if candidates else None


def find_fonts():
    """
    The shipped `fonts` directory from the C# build, or None.

    Same reasoning as find_native_library, and the same source: a NuGet restore
    puts the fonts at the project output root while the libraries go under
    runtimes/<rid>/native, so the Python run is handed the copy the C# run
    already has. The C++17 build keeps both together in bin/ and never needs
    this.
    """
    for configuration in ("Release", "Debug"):
        for found in sorted(glob.glob(os.path.join(
                REPO_ROOT, "csharp", "bin", configuration, "net*", "fonts"))):
            if os.path.isdir(found):
                return os.path.abspath(found)
    return None


def find_native_library(configuration_root=None):
    """
    The directory in this checkout's build output that holds the native library.

    The Python port has no directory of its own to find one in: the built executable
    resolves --library to the directory it lives in, where NuGet has already put the
    library, and `python -m docfilters_cli` resolves it to the package directory, where
    nothing has. So the same copy the C# run uses is handed to the Python run through
    DF_PATH.

    runtimes/*/native is a per-RID layout and a build restores every RID the package
    carries, so the choice is between six directories and only one of them is loadable
    here. RUNTIME_IDENTIFIER is asked first because it is the exact answer; a directory
    holding a library named for this host is the fallback, which covers a RID this
    function does not spell -- linux-musl-x64 on Alpine, or win-x64 running under
    emulation on an arm64 host. Both tests are needed: the name alone would pick between
    win-x64 and win-x86 by mtime, and the RID alone would pick a directory a build
    created but restored nothing into.

    Newest wins among whatever survives, so a rebuild of one configuration is preferred
    over a stale copy of another.
    """
    root = configuration_root or os.path.join(REPO_ROOT, "csharp", "bin")
    found = [path
             for path in glob.glob(os.path.join(root, "*", "net*", "runtimes", "*", "native"))
             if os.path.isfile(os.path.join(path, LIBRARY_NAME))]

    candidates = [path for path in found
                  if os.path.basename(os.path.dirname(path)) == RUNTIME_IDENTIFIER] or found

    return max(candidates, key=os.path.getmtime) if candidates else None


def launcher_for(target, binding, library):
    """
    How to start the implementation that `target` names, as
    (launcher, env, detail, settings_dir).

    settings_dir is where the engine reads ISYS11df.ini from, or None when this
    run cannot place one. Measured rather than assumed: the engine picks the
    file up from the directory holding the executable *and* from the one holding
    the native library, and not from the working directory. For a built
    executable the first is the obvious answer; for the Python port there is no
    executable of ours to sit beside -- sys.executable is the interpreter, whose
    directory is not ours to write into -- so the library's directory is used.

    One option names any of the four ports, because a port is a thing you run and
    --cli already means "the thing to run". What distinguishes them is only what kind
    of thing that is: the C# and C++17 ports build an executable, the Python port is a
    package directory, and the Java port is a Gradle project directory, which no
    package directory is. So the shape of the value
    decides, with no second switch to keep in step and no way to pass two that
    contradict each other.

    A relative value that is not there is retried against the repository root, so
    `--cli python` works from wherever the script was started rather than only from
    the root. The same courtesy covers `--cli cpp17/build/bin/docfilters`.

    --binding is a modifier for the Python port, and --library for the Python and
    Java ports. They are accepted but useless for an executable, which resolves the library beside itself; saying so is
    left to the printed detail lines, which name what was actually used.
    """
    resolved = target
    if not os.path.exists(resolved) and not os.path.isabs(resolved):
        from_root = os.path.join(REPO_ROOT, target)
        if os.path.exists(from_root):
            resolved = from_root

    # Absolute from here on: what is launched is launched as a subprocess, and a
    # relative path would be read against its working directory rather than the one
    # the value was typed in.
    resolved = os.path.abspath(resolved)

    if os.path.isdir(resolved) and os.path.isfile(os.path.join(resolved, "build.gradle.kts")):
        library = library or find_java_native_library()
        launcher, environment = java_launcher(resolved, library)

        # `library` appears twice because it answers two questions. In the
        # detail list it is a line for the header, saying which of the several
        # build directories find_java_native_library picked -- the one fact a
        # reader needs to know what was actually under test, and not derivable
        # from anything else printed. As the last element it is settings_dir,
        # where EngineSettings puts ISYS11df.ini: for the same reason as the
        # Python port, there is no executable of ours to sit beside, only the
        # JVM's, whose directory is not ours to write into.
        return launcher, environment, [("project", resolved), ("library", library)], library

    if os.path.isdir(resolved):
        binding = binding or (DEFAULT_BINDING if os.path.isdir(DEFAULT_BINDING) else None)
        library = library or find_native_library()
        launcher, environment = python_launcher(resolved, binding, library)

        return launcher, environment, [("package", resolved),
                                       ("binding", binding or "already importable"),
                                       ("library", library or "wherever the binding finds one")], library

    if os.path.isfile(resolved):
        return [resolved], None, [], os.path.dirname(resolved)

    raise SystemExit("not an implementation to check: %s\n"
                     "Pass a built docfilters executable, the directory holding the "
                     "Python package (--cli python), or the Java project (--cli java)." % target)


def find_java_native_library():
    """
    A directory holding both native libraries the Java binding loads, or exit.

    The C++17 build unpacks a release archive and copies its libraries beside its
    executable, so a checkout that has built that port already holds a loadable set;
    nothing else in this checkout does. There is no further guess to make, so the
    alternative is to say what is needed.
    """
    for root in ("cpp17", os.path.join("tests", "cpp17")):
        for path in sorted(glob.glob(os.path.join(REPO_ROOT, root, "build*", "bin")), key=os.path.getmtime,
                           reverse=True):
            if all(os.path.isfile(os.path.join(path, name)) for name in (LIBRARY_NAME, JNI_LIBRARY_NAME)):
                return path

    raise SystemExit("no directory holding both %s and %s was found for the Java port; pass "
                     "--library, naming the native directory of a Document Filters release "
                     "archive" % (LIBRARY_NAME, JNI_LIBRARY_NAME))


def java_launcher(project_root, library):
    """
    How to start the Java port, and the environment it needs, as (launcher, env).

    The installed distribution's jars, started with this script's own `java` rather
    than through the start script: the start script would need the native directory
    in DOCFILTERS_OPTS, where its word-splitting eats Windows backslashes. Running
    `gradlew installDist` first is the build step, as building is for the C# port.

    java.library.path is the only place the binding looks for its natives, and the
    JVM reads it once at start-up -- which is why --library has to arrive here
    rather than as an argument the port could act on. DF_PATH is set as well, so
    the engine is initialised against the same directory.
    """
    libs = os.path.join(project_root, "build", "install", "docfilters", "lib")
    if not os.path.isdir(libs):
        raise SystemExit("no installed distribution under %s; build it with "
                         "`gradlew installDist` in %s" % (libs, project_root))

    java_home = os.environ.get("JAVA_HOME")
    java = os.path.join(java_home, "bin", "java") if java_home else "java"

    environment = dict(os.environ)
    environment["DF_PATH"] = library

    return [java, "-Djava.library.path=" + library, "-cp", os.path.join(libs, "*"),
            "com.hyland.documentfilters.cli.Program"], environment


def python_launcher(package_root, binding, library):
    """
    How to start the Python port, and the environment it needs, as (launcher, env).

    `python -m docfilters_cli` rather than a path to a script: the package has a
    __main__.py precisely so it can be started that way, and it is how the README says to
    run it. sys.executable so the port runs under the same interpreter as this harness,
    which is the one whose version the run was reported against.

    PYTHONPATH is composed rather than appended to any the caller had, with the caller's
    kept last: this script is checking a specific checkout, and a stale copy of
    docfilters_cli earlier on the path would silently be the thing under test.
    """
    if not os.path.isfile(os.path.join(package_root, "docfilters_cli", "__main__.py")):
        raise SystemExit("no docfilters_cli package under %s; --cli names the directory "
                         "that holds it, which is python/ in this checkout" % package_root)

    environment = dict(os.environ)
    entries = [package_root]

    if binding:
        entries.append(binding)
    if environment.get("PYTHONPATH"):
        entries.append(environment["PYTHONPATH"])

    environment["PYTHONPATH"] = os.pathsep.join(entries)

    if library:
        environment["DF_PATH"] = library

    return [sys.executable, "-m", "docfilters_cli"], environment


def prepare_output_dir(path):
    """
    Empties the output directory, or creates it, and leaves a marker behind.

    Guarded deliberately. This script deletes the directory's contents twice per
    run, and a mistyped --out-dir would otherwise be a very bad afternoon: an
    existing directory is only ever emptied when it is already empty or carries
    the marker from a previous run.
    """
    if os.path.exists(path):
        if not os.path.isdir(path):
            raise SystemExit("--out-dir %s is not a directory" % path)

        entries = os.listdir(path)
        if entries and MARKER not in entries:
            raise SystemExit(
                "refusing to empty %s: it is not empty and was not created by this script.\n"
                "Point --out-dir at a new or empty directory." % path)

        for entry in entries:
            target = os.path.join(path, entry)
            shutil.rmtree(target) if os.path.isdir(target) else os.remove(target)
    else:
        os.makedirs(path)

    with open(os.path.join(path, MARKER), "w") as handle:
        handle.write("scratch directory for tests/e2e/end2end.py\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--license-key", default=os.environ.get("DF_LICENSE_KEY"),
                        help="Document Filters license key; defaults to $DF_LICENSE_KEY. "
                             "Optional: without one the library runs in licenseless evaluation "
                             "and the checks it forbids are skipped")
    parser.add_argument("--cli", default=None, metavar="PATH",
                        help="which implementation to check: a built docfilters executable, "
                             "the directory holding the Python package (--cli python), "
                             "or the Java project (--cli java). "
                             "Defaults to the most recently built executable")
    parser.add_argument("--binding", default=None, metavar="DIR",
                        help="directory holding the DocumentFilters Python binding, for the "
                             "Python port; only needed when it is not already importable")
    parser.add_argument("--library", default=None, metavar="DIR",
                        help="directory holding the native Document Filters library, for the "
                             "Python and Java ports; defaults to the one in the C# build output "
                             "for Python, and to the C++17 build's for Java")
    parser.add_argument("--documents", default=DEFAULT_DOCUMENTS,
                        help="directory holding the sample documents")
    parser.add_argument("--input", default=DEFAULT_INPUT,
                        help="the document to exercise, relative to --documents or absolute")
    parser.add_argument("--out-dir", default=None,
                        help="scratch directory; defaults to a new temporary one")
    parser.add_argument("--keep", action="store_true",
                        help="leave the output directory in place, to inspect a failure")
    parser.add_argument("--record", default=None, metavar="FILE",
                        help="write a transcript of every invocation, for "
                             "tests/parity/compare.py to compare against another "
                             "implementation's")
    parser.add_argument("--verbose", action="store_true",
                        help="print each command as it runs, with the license key redacted")
    parser.add_argument("--filter", default=None,
                        help="only run checks whose name contains this text")
    parser.add_argument("--skip", action="append", default=[], metavar="CHECK=REASON",
                        help="do not run this check, and say why. For something an "
                             "implementation cannot do at all -- a call its binding "
                             "does not reach. The reason is printed and recorded, so "
                             "it is worth writing a real one. Repeatable")
    args = parser.parse_args(argv)

    requested_skips = parse_skips(parser, args.skip)

    # Which implementation, decided once. Everything downstream sees a Runner.
    environment = None
    detail = []

    target = args.cli or find_cli()
    if not target:
        raise SystemExit(
            "no built executable found, so nothing to check: pass --cli.\n"
            "Looked in csharp/bin/*/net*/, cpp17/build*/bin/ and tests/cpp17/build*/bin/.\n"
            "The Python port has no build output to find, so it is named rather than "
            "discovered: --cli python")

    launcher, environment, detail, settings_dir = launcher_for(target, args.binding, args.library)

    document = args.input if os.path.isabs(args.input) else os.path.join(args.documents, args.input)
    if not os.path.isfile(document):
        hint = ""
        if document == os.path.join(DEFAULT_DOCUMENTS, DEFAULT_INPUT):
            hint = ("\ntest.pdf is committed beside this script, so a clean checkout "
                    "has it.\nIf it is missing, the checkout is incomplete; pass "
                    "--documents and --input to use another document.")
        raise SystemExit("input document not found: %s%s" % (document, hint))

    out_dir = args.out_dir or tempfile.mkdtemp(prefix="df-e2e-")
    prepare_output_dir(out_dir)

    checks = [(name, fn) for name, fn in CHECKS if not args.filter or args.filter.lower() in name.lower()]
    if not checks:
        raise SystemExit("no checks matched --filter %r" % args.filter)

    licensed = bool(args.license_key)

    runner = Runner(launcher, args.license_key, out_dir, args.verbose, environment=environment)
    if args.record:
        runner.recorder = Recorder(args.record, target)

    settings = EngineSettings(settings_dir)
    settings.__enter__()

    print("CLI      %s" % runner.describe())
    for label, value in detail:
        print("%-8s %s" % (label, value))
    print("document %s" % document)
    print("scratch  %s" % out_dir)
    print("license  %s" % ("supplied" if licensed else "none — licenseless evaluation"))
    print("engine   %s" % settings.detail)
    print("fonts    %s" % settings.fonts_detail)
    print("running  %d checks\n" % len(checks))

    failures = []
    skipped = []

    def skip(name, reason):
        skipped.append((name, reason))
        if runner.recorder is not None:
            runner.recorder.skipped(name, reason)
        print("  skip  %s" % name)

    try:
        for name, check in checks:
            if name in requested_skips:
                skip(name, requested_skips[name])
                continue

            reason = getattr(check, "license_reason", None)
            if reason and not licensed:
                skip(name, reason)
                continue

            runner.clear_output()
            if runner.recorder is not None:
                runner.recorder.start(name)
            try:
                check(runner, document)
                print("  ok    %s" % name)
            except CheckSkipped as reason:
                # Decided part-way through, so some of the check's commands are
                # already recorded. Recording the skip as well is what tells
                # the comparison that the rest are absent on purpose.
                skip(name, str(reason))
            except CheckFailed as failure:
                failures.append((name, str(failure)))
                print("  FAIL  %s" % name)
            except Exception as error:  # noqa: BLE001 - a crashed check is a failed check
                failures.append((name, "%s: %s" % (type(error).__name__, error)))
                print("  ERROR %s" % name)
    finally:
        # First, because it is the only one of these that touches a directory
        # this script does not own.
        settings.__exit__(None, None, None)
        if args.keep:
            print("\nleaving %s in place (--keep)" % out_dir)
        else:
            shutil.rmtree(out_dir, ignore_errors=True)

    print()
    if skipped:
        print("%d skipped:" % len(skipped))
        for name, reason in skipped:
            print("  %-46s %s" % (name, reason))
        print()

    ran = len(checks) - len(skipped)

    if failures:
        print("%d of %d checks failed:\n" % (len(failures), ran))
        for name, detail in failures:
            print("  %s\n    %s\n" % (name, detail.replace("\n", "\n    ")))
        return 1

    print("all %d checks passed" % ran)
    return 0


if __name__ == "__main__":
    sys.exit(main())
