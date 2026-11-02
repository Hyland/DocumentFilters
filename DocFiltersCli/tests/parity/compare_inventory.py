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
Every implementation tests the same things, by name.

compare.py checks that the implementations *behave* the same. This checks that
they are *covered* the same, which is the failure nothing else can see: each
language's suite is a translated copy, so a case dropped in translation leaves
every remaining test passing and no evidence that anything is missing. Four
implementations can agree perfectly on the cases that survive while one of them
has quietly stopped testing a third of its execution stage.

The comparison is exact after normalisation. That is only possible because the
porting rule says to mirror the reference name for name, idiomatic casing
aside, which turns a test name into a key every suite can be indexed by:

    C#       Report_output_is_never_coloured
    C++17    "report output is never coloured"
    Python   test_report_output_is_never_coloured
    Java     reportOutputIsNeverColoured
    all      reportoutputisnevercoloured

There is deliberately no exceptions file. A list of "these two names mean the
same thing" would grow with every test, and each entry would be a place where a
genuinely missing test could hide. If a name differs, one of the two is
renamed.

Usage:
    compare_inventory.py --impl csharp=cs-list.txt --impl cpp17=cpp-list.txt

where each file is the raw output of that suite's own listing command:

    dotnet test <project> --list-tests
    docfilters_tests --list-tests          (Catch2)
    run_tests.py --list-tests              (Python)
    TestRunner --list-tests                (Java)

The format is detected from the content, so the caller only has to capture it.
Exits 0 when the inventories match, 1 when they do not, 2 on a usage problem.
"""

import argparse
import difflib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def normalise(name):
    """
    A test name reduced to its letters and digits, lowercased.

    Everything that separates words goes rather than becoming a space:
    separators are exactly what the four naming conventions disagree about.
    `per_page`, `per-page`, `per page` and `perPage` are the same name said
    four ways, and only Java's says it with no separator at all -- camelCase
    has nothing to turn into a space, so a key that keeps word boundaries
    cannot match it without re-splitting the name, which needs a rule for
    `Utf8` and `The32Bit` that no two of these languages would agree on.

    Dropping the boundaries makes the key more permissive, so two genuinely
    different names could in principle collide. That does not hide a missing
    test: the comparison counts how many distinct tests carry each key, so a
    port with one of a colliding pair still reports one short.
    """
    return re.sub(r"[^a-z0-9]", "", name.lower())


def load_exemptions(path):
    """
    Tests the reference has that a port is excused from, with the reason.

    One line per exemption: `<language> <normalised name> # <reason>`. The
    reason is not decoration -- it is the whole justification for the line
    existing, and a line without one is rejected.

    This is the exceptions file the design otherwise avoids, and it is kept
    honest by being checked both ways: an exemption whose test *does* exist in
    that port is an error, not a no-op. Without that an entry would silently
    outlive the reason for it, which is exactly how an exceptions file becomes
    a place for a genuinely missing test to hide.

    Reserved for cases a port cannot express, not for ones nobody has written
    yet. A test that is merely absent is a gap, and the check saying so is the
    point of it.
    """
    exemptions = {}
    if not os.path.isfile(path):
        return exemptions

    with open(path, "r", encoding="utf-8") as handle:
        for number, raw in enumerate(handle, start=1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            body, _, reason = line.partition("#")
            language, _, name = body.strip().partition(" ")
            if not language or not name.strip():
                raise SystemExit("%s:%d  expected `<language> <test name> # <reason>`"
                                 % (path, number))
            if not reason.strip():
                raise SystemExit("%s:%d  no reason given, and a reason is the point: %s"
                                 % (path, number, line))
            exemptions.setdefault(language, {})[normalise(name)] = reason.strip()
    return exemptions


def parse_dotnet(text):
    """
    `dotnet test --list-tests`: a header, then one indented fully-qualified name
    per line.

    A [Theory] appears once per data row, with its arguments appended --
    `...Method(path: "/etc/passwd")`. The arguments are dropped, and so is
    everything before the last dot, which has to happen in that order: an
    argument can contain a dot of its own.

    Returns (scope, name): the class and the method. The scope is not compared
    between languages --- see `inventory` --- it is only what tells twenty rows
    of one [Theory] apart from two methods that happen to share a name.
    """
    found = []
    for line in text.splitlines():
        if not line.startswith("    "):
            continue
        candidate = line.strip()
        if not candidate or not re.match(r"^[A-Za-z_]", candidate):
            continue
        cut = candidate.find("(")
        if cut != -1:
            candidate = candidate[:cut]
        parts = candidate.rsplit(".", 2)
        scope = parts[-2] if len(parts) >= 2 else ""
        found.append((scope, parts[-1]))
    return found


def parse_catch2_xml(text):
    """
    `docfilters_tests --list-tests --reporter xml`.

    The XML form rather than the plain one because Catch2's plain listing wraps
    a long name onto a second line, at a width that depends on the console it
    ran in. There is no way to tell a wrapped continuation from a short test
    name of its own, so the plain form silently truncates long names and
    invents extra ones -- which is how `names differing only in case are two
    outputs where the filesystem is case sensitive` turned into a name ending
    at `case` plus a phantom test called `sensitive`.

    The scope is the source file the case was declared in. Catch2 has no
    classes, and its tags group by subject rather than by file, so the file is
    the closest thing it has to the C# class --- and all that is asked of it is
    to be distinct per test, not to match anything in another language.
    """
    found = []
    for block in re.findall(r"<TestCase>(.*?)</TestCase>", text, re.S):
        name = re.search(r"<Name>(.*?)</Name>", block, re.S)
        if not name:
            continue
        source = re.search(r"<File>(.*?)</File>", block, re.S)
        scope = os.path.basename(source.group(1).strip()) if source else ""
        found.append((scope, name.group(1)))
    return found


def parse_catch2(text):
    """
    Catch2's plain `--list-tests`: a header, then each test case indented two
    spaces, with its tags on a further-indented line in square brackets.

    Refuses rather than guesses when it sees wrapping. See parse_catch2_xml.
    """
    names, suspect = [], []
    for line in text.splitlines():
        if not line.strip() or not line.startswith("  "):
            continue
        candidate = line.strip()
        if candidate.startswith("[") or candidate.endswith("test cases"):
            continue
        if line.startswith("      "):      # a tag line
            continue
        if len(line) >= 74:
            suspect.append(candidate)
        names.append(("", candidate))

    if suspect:
        raise SystemExit(
            "This Catch2 listing has lines at the wrap width, so long test names "
            "have been split across lines and cannot be read back reliably.\n"
            "Capture it as XML instead:\n"
            "    docfilters_tests --list-tests --reporter xml")
    return names


def parse_python(text):
    """
    unittest ids, one per line: `module.path.ClassName.test_name`.

    The `test_` prefix is unittest's marker for what to collect rather than
    part of the name, so it comes off before the comparison -- otherwise every
    Python test would differ from its counterpart by that one word.
    """
    found = []
    for line in text.splitlines():
        if not line.startswith("    "):
            continue
        parts = line.strip().rsplit(".", 2)
        candidate = parts[-1]
        if candidate.startswith("test_"):
            candidate = candidate[len("test_"):]
        if candidate:
            found.append((parts[-2] if len(parts) >= 2 else "", candidate))
    return found


def parse_java(text):
    """
    The Java suite's own runner: `<package below tests>.ClassName.methodName`,
    one per line, as `TestRunner --list-tests` writes them.

    Shaped like the Python listing but with nothing to strip: the runner
    collects by annotation rather than by name prefix, so a method is named
    after what it asserts and nothing else.
    """
    found = []
    for line in text.splitlines():
        if not line.startswith("    "):
            continue
        parts = line.strip().rsplit(".", 2)
        if parts[-1]:
            found.append((parts[-2] if len(parts) >= 2 else "", parts[-1]))
    return found


def parse(text, origin):
    """Picks a parser by what the listing looks like, so callers just capture."""
    if "The following Tests are available" in text:
        return parse_dotnet(text)
    if "<TestCase>" in text or "<Name>" in text:
        return parse_catch2_xml(text)
    if "All available test cases" in text:
        return parse_catch2(text)
    if "The following Python tests are available" in text:
        return parse_python(text)
    if "The following Java tests are available" in text:
        return parse_java(text)
    raise SystemExit(
        "%s: cannot tell which suite produced this listing.\n"
        "Expected the header of `dotnet test --list-tests`, of Catch2's "
        "`--list-tests`, or of the Python or Java suite's listing." % origin)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--impl", action="append", default=[], metavar="NAME=FILE",
                        help="a captured test listing; the first is the reference")
    parser.add_argument("--exemptions", default=os.path.join(HERE, "port-exemptions.txt"),
                        help="tests a port cannot express, with reasons "
                             "(default: port-exemptions.txt beside this script)")
    parser.add_argument("--suggest", action="store_true",
                        help="pair unmatched names with their closest counterpart, "
                             "turning the report into a rename list")
    arguments = parser.parse_args()

    exemptions = load_exemptions(arguments.exemptions)

    inventories = []
    for entry in arguments.impl:
        if "=" not in entry:
            parser.error("--impl wants NAME=FILE, got %r" % entry)
        name, path = entry.split("=", 1)
        if not os.path.isfile(path):
            parser.error("no such listing for %s: %s" % (name, path))
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            raw = parse(handle.read(), path)
        # Keyed by normalised name, valued by the distinct tests carrying it.
        #
        # Twenty rows of one [Theory] share a scope and collapse to one entry,
        # which is right -- they are one test. Two different methods that happen
        # to share a name have different scopes and stay two, which is also
        # right, and is what a plain set of names got wrong: deleting one of a
        # same-named pair from a port left the name still present and the check
        # still passing.
        #
        # The scopes themselves are never compared between languages. C++17
        # files its fixture tests where C# files them under another class, and
        # demanding those line up would reject 25 pairs that are correctly
        # matched today. Only how many distinct tests share a name is compared.
        folded = {}
        for scope, original in raw:
            folded.setdefault(normalise(original), {})[scope] = original
        inventories.append((name, folded))

    if len(inventories) < 2:
        parser.error("at least two --impl are needed to compare anything")

    reference_name, reference = inventories[0]
    print("%s: %d test names" % (reference_name, len(reference)))

    failed = False
    for name, inventory in inventories[1:]:
        print("%s: %d test names" % (name, len(inventory)))

        # The rule is one-directional. C# is the reference, so every test it
        # has must exist in every port -- that is what stops a case being lost
        # in translation. A port having *more* is not drift: it is a port
        # covering something the reference gets from its own standard library
        # (C# has no Strings tests because it uses string.Trim and
        # File.ReadAllLines directly), or a quirk of its own binding.
        missing = sorted(key for key in reference
                         if len(inventory.get(key, {})) < len(reference[key]))
        extra = sorted(set(inventory) - set(reference))

        excused = exemptions.get(name, {})
        # Checked both ways. An exemption for a test the port turns out to have
        # is stale, and saying so is what stops the file rotting into a place a
        # real gap could hide.
        stale = sorted(set(excused) - set(missing))
        for key in stale:
            print("  stale exemption: %s has %r after all" % (name, key))
        if stale:
            failed = True

        waived = [key for key in missing if key in excused]
        missing = [key for key in missing if key not in excused]
        for key in waived:
            print("  excused in %s: %s" % (name, excused[key]))

        if extra:
            print("  %d in %s beyond %s (allowed, listed for information)"
                  % (len(extra), name, reference_name))
            for key in extra:
                print("      %s" % sorted(inventory[key].values())[0])

        if not missing:
            print("  covers every %s test" % reference_name)
            continue

        failed = True
        print()
        print("  %d in %s with no counterpart in %s"
              % (len(missing), reference_name, name))
        for key in missing:
            expected, got = len(reference[key]), len(inventory.get(key, {}))
            line = "    %s" % sorted(reference[key].values())[0]
            if expected > 1:
                line += "  (%s has %d tests with this name, %s has %d)" % (
                    reference_name, expected, name, got)
            if arguments.suggest:
                close = difflib.get_close_matches(key, extra, n=1, cutoff=0.6)
                if close:
                    line += "\n        closest in %s: %s" % (name, inventory[close[0]])
            print(line)
        print()

    if failed:
        print("A port is missing a test the reference has. Rename the port's")
        print("version to match, or write it if it is genuinely absent.")
        return 1

    print("Every port covers the reference.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
