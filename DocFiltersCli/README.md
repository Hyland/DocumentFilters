# docfilters — a command line tool built on Document Filters

Convert, extract, list and inspect documents from the shell. A complete,
realistic sample of the Document Filters API, built the way you would build a
real tool rather than the way you would demonstrate one call.

## Quick start

```bash
dotnet build csharp/DocFiltersCli.csproj
csharp/bin/Debug/net10.0/docfilters convert --to pdf --out-dir out/ report.docx
```

No license key needed to try it — [Licensing](#licensing) says what evaluation
allows. Add `--dry-run` to any command to see the plan without writing anything.

The Python port is the same tool with nothing to build:
[python/README.md](./python/README.md).

## What it does

```bash
docfilters convert --to pdf --out-dir out/ ./corpus     # render a whole tree to PDF
docfilters convert --to png --pages 1-2 report.docx     # first two pages as images
docfilters extract -i --out-dir dump/ bundle.pst        # pull out attachments and images
docfilters list -s archive.zip                          # what's inside, without extracting
docfilters identify --json *.bin                        # what are these files?
docfilters meta report.pdf                              # metadata only
docfilters formats                                      # what can this build read?
docfilters options                                      # engine tunables for --options
```

Seven verbs, plus `help`. Shared options mean the same thing on every command
that accepts them, and `docfilters help <command>` prints the specifics.

## The output model, which is the part worth copying

The rule the whole tool is built around: **the meaning of `-o` never depends on
`--to`.** Three cardinalities, chosen by the arguments alone.

| You want | You write |
| --- | --- |
| each input its own output | `--out-dir out/` |
| many inputs into one document | `--combine --out-file book.pdf` |
| one input, one exact path | `--out-file report.pdf` |

`--out-file` with several inputs and no `--combine` is a hard error, never a
guess. Whether a format writes one file or one file per page is a static
property of `--to`, so the tool can tell you what it is about to do before it
does any of it.

## Why the code is arranged this way

```
argv → Parse → SharedOptions → Bind → Plan → RunPlan → Execute → Report → exit code
```

Arguments become data, data becomes a plan, and only then does anything happen.
The seam is between planning and execution:

- **Parse, Bind and Plan are pure.** No license, no native library, no
  filesystem. Every rule about output paths, walk depth and argument validation
  is a function over plain data, so it is exhaustively testable in milliseconds.
- **Execute is the only stage that touches Document Filters.** It opens
  extractors, loops pages onto canvases and writes bytes. It makes no decisions,
  because they were all made in Plan.

Execute reaches the engine through one interface, `Engine/`, which is the only
folder that names a `Hyland.DocumentFilters` type. Each member maps to one
engine call and decides nothing. That seam is what makes the execution stage
testable at all — the binding's own types have internal-only constructors and no
virtual members, so nothing outside the assembly can stand in for them.

The practical payoff: the whole run is knowable as a value before the first byte
is written. That is what lets the tool refuse to overwrite one output with
another, and what lets the same command line be verified across four languages
without opening a document.

## Layout

```
DocFiltersCli/
├── CLAUDE.md        the rules for changing any of this — read before editing
├── languages.yml    where each implementation lives, and how it is built
├── design/
│   └── porting/     how a port is made, and what each language gets wrong
├── csharp/          the reference implementation
├── cpp17/           the C++17 port
├── python/          the Python port — see python/README.md
├── java/            the Java port
└── tests/
    ├── conformance/ fixtures shared by every language — no language owns these
    ├── csharp/      the C# test suite
    ├── cpp17/       the C++17 test suite
    ├── python/      the Python test suite
    ├── java/        the Java test suite
    ├── e2e/         end-to-end tests: a built CLI, the real library, real files
    └── parity/      the four implementations compared against each other
```

Inside `csharp/`, the folders are the pipeline stages:

| Folder | Holds |
| --- | --- |
| `Cli/` | the argument table, and the hand-written binding of argv against it |
| `Configuration/` | the options model, value parsers, cross-argument validation |
| `Planning/` | format catalogue, output planner, name templates, page ranges |
| `Discovery/` | the filesystem walk |
| `Engine/` | the Document Filters seam — the only code naming a binding type |
| `Execution/` | **the Document Filters code**, written against that seam |
| `Commands/` | one thin orchestrator per verb |
| `Runtime/` | console, filesystem, JSON writer, batch loop, exit codes |

## If you are here to learn the API

Read `Execution/` and ignore everything else. That folder is the sample; the
rest is the plumbing any real tool needs.

It calls the engine through `Engine/IDocumentFiltersEngine.cs`, where each
member maps to exactly one Document Filters call. If you would rather see the
binding used directly, `Engine/DocumentFiltersEngine.cs` is that — every
adapter there is a line or two around the real API.

| Start with | To learn |
| --- | --- |
| `DocumentFiltersSession.cs` | initialising the engine once, and opening a document |
| `TextConversion.cs` | the shortest useful thing: extract text until EOF |
| `CanvasConversion.cs` | high-definition output — canvas, page loop, watermarks, redaction |
| `SubfileExtraction.cs` | walking into archives, mail stores and attachments |
| `MetadataReport.cs` | reading document metadata |
| `StructureReport.cs` | fonts, page elements, hyperlinks, bookmarks, annotations |
| `CatalogReports.cs` | asking the engine what formats and options it supports |

For smaller, single-purpose examples, see the [C# samples](../samples/csharp).

## One CLI, four implementations

The same command line ships in C#, C++17, Python and Java, and must behave
identically in all four. C# is the reference; the other three are ports, in
[`cpp17/`](./cpp17), [`python/`](./python) and [`java/`](./java). The Python
one has its own README covering how to run it and the places it deliberately
differs.

How a port is made, and how a change reaches all four, is
[`design/porting/workflow.md`](./design/porting/workflow.md). Each language has
a list of the constructs that compile, pass a casual test and behave
differently from the reference — including
[one for C# itself](./design/porting/traps-csharp.md), which is read the other
way round: what the reference's own constructs mean, for the ports to
reproduce.

Keeping them aligned is not left to discipline. `tests/conformance/` holds four
fixtures, in one line-oriented format, that every implementation runs as part of
its own test suite:

| Fixture | Is the authority for |
| --- | --- |
| `arguments.txt` | which arguments exist on which command, with short form and arity |
| `formats.txt` | every `--to` token: kind, single-file or per-page, combinable, hi-def, extension, and the canvas options the format contributes |
| `out-names.txt` | `--out-name` rendering, and templates that must be rejected |
| `plan-corpus.txt` | argv → resolved plan, and every error, message and exit code |

They are written by hand from what the tool is meant to do, rather than
generated from any implementation — a file dumped from the reference could only
prove the others agree with it, and would never catch an argument nobody
implemented.

A change to the CLI is made in the fixtures first. Porting it then has a
definition of done: *make these cases pass*.

Two more checks apply to a port once it exists, and both caught real differences
in the C++17 one:

- `--print-contract` prints the whole argument surface as flat text. Diffing two
  implementations' output compares every argument, short form, arity and
  description in one step.
- `tests/e2e/end2end.py --cli <path>` runs the end-to-end suite against any
  built executable, so a port is exercised against the real engine by the same
  checks as the reference.

### What is deliberately not compared

`--help` text, because each implementation renders usage its own way and
matching them would mean writing a fifth renderer for all four to share. And
`--time` / `--progress` output, whose figures differ every run. The tool's own
version string is in the same category --- the engine version it reports is
not. A `--to docpack` pack is in the same position for a reason that is not
about implementations at all: its `df-core.xml` stamps `<created>` to the
millisecond, so two packs of one document are never byte-identical even from
the same binary. Every other entry in the pack is.

### Moving a change into the three ports

A change to the reference is not finished until the ports carry it.
[`.github/workflows/ai-sync.yml`](../.github/workflows/ai-sync.yml) does that
work: it runs Claude Code against the procedure checked in at
[`.claude/commands/propagate-delta.md`](./.claude/commands/propagate-delta.md)
and opens one draft pull request per port, each targeting **your** branch
rather than the default one --- so the reference change and all three
translations arrive in a single merge, and `master` is never in a state where
the four disagree.

Two ways to start it:

| Way in | What it does |
| --- | --- |
| The **`ai-sync`** label | On a pull request touching an authority path: `design/`, `tests/conformance/`, `csharp/` or `tests/csharp/`. The label is the human sanction and nothing runs without it. Pushing to an authority path again re-runs the translation, updating the same port pull requests rather than opening more. |
| **Actions -> AI port sync -> Run workflow** | Takes the pull request number, with one checkbox per port (all on by default) and `dry_run` **on** by default, which produces the patch as an artifact and pushes nothing. This is the way to try it. |

Either way a maintainer approves the run before it starts. Inference goes
through Amazon Bedrock in our own AWS account, reached with the workflow's
OIDC token; the only stored secret is an IAM role ARN, scoped to the
`claude-sync` environment, and that environment has a reviewer gate. Pull
requests from forks are refused outright, with no override.

The commits it makes are created through GitHub's `createCommitOnBranch` API
rather than pushed, so GitHub signs them and they satisfy the organisation's
"commits must have verified signatures" rule without a signing key being
stored anywhere.

**None of this is privileged.** The `Ports landed` check measures the commits
on your branch, not the agent's run history, so porting by hand and committing
it yourself satisfies it identically. The agent is a way of doing the work,
not a different standard. See [what CI checks](#what-ci-checks) below for how
that check decides, and
[`design/porting/workflow.md`](./design/porting/workflow.md) for the full
method, including what to verify and in what order.

## Building and testing

C#:

```bash
dotnet build csharp/DocFiltersCli.csproj
dotnet test tests/csharp/DocFiltersCli.Tests.csproj
```

C++:

```bash
cmake -S tests/cpp17 -B tests/cpp17/build
cmake --build tests/cpp17/build --config Debug
```

C++17, which builds the tool and its suite together — configure once, then
build. The test project links the tool's library target and adds Catch2 on its
own, so building `cpp17/` alone never fetches a test framework. Both `cmake` and
`ctest` must be on `PATH`; see [`cpp17/README.md`](cpp17/README.md) if your
editor's build tasks fail before reaching CMake:

Run the C++ suite from the build directory. The test binary is built into
`bin/`, where the native library is copied beside the tool, so it starts
without anything being put on the loader path first:

```bash
cd tests/cpp17/build && ./bin/docfilters_tests
```

`bin/` whichever generator you used: the test target sets its own output
directory, so the path does not change between Visual Studio and Ninja. The
tests are also registered with CTest, so `ctest -C Debug` works from the same
directory.

Python:

The Python port needs no build, and its suite is the standard library's:

```bash
python tests/python/run_tests.py
```

All suites run anywhere — no license, no native library, no scratch directory,
no filter to remember. They are fast enough to run on every save.

That is possible because the engine sits behind `Engine/` and the filesystem
behind `IFileSystem`, so the tests supply their own. It is not an option to
point them at the real ones: a corpus backed by real files could not be
byte-identical across four languages and three platforms, because directory
listing order differs between ext4, NTFS and APFS.

### End-to-end tests

The suite above proves what the CLI *says* to the engine. It cannot prove what
the engine hears — a call the engine rejects, an option that never reaches it,
or a file that is written but blank. That needs the real thing:

```bash
python tests/e2e/end2end.py                     # no license needed
python tests/e2e/end2end.py --license-key KEY   # everything
python tests/e2e/end2end.py --cli python        # the Python port instead
python tests/e2e/end2end.py --cli java          # the Java port instead
```

Python 3, standard library only, no test framework. It runs a built CLI as a
subprocess, so it sees exactly what a user sees: exit codes, stderr and files on
disk. It finds the most recently built binary on its own; `--cli` overrides.
`--verbose` prints each command with the key redacted, and `--keep` leaves the
output directory behind for inspection.

`--cli` names any of the four implementations, and the shape of the value says
which kind it is: a built executable for C# and C++17, the directory holding
the package for Python, or the Gradle project directory for Java. The last two
have no executable of ours to find, so they are named rather than discovered.
`--cli python` runs the port in [`python/`](./python) through
`python -m docfilters_cli`; `--cli java` runs the installed distribution from
[`java/`](./java), so `gradlew installDist` has to have run first. Both take
the native library out of a build that has one beside it — the same checks, the
same skips, no exceptions for any of them. There is one script rather than one
per language because these checks are about what the engine hears, which no
port is allowed to differ on.

`--skip "<check>=<reason>"` leaves out a check an implementation cannot run at
all — a call its binding does not reach. The reason is required and is recorded
in the transcript, so the parity comparison can tell a deliberate skip from a
check that went missing, and says which it saw. It is a blunt instrument: a
skip costs everything else that check covered, which is the argument for
removing one as soon as it can be.

Without a key, the checks that licenseless evaluation forbids are skipped, each
printing which restriction it ran into, so a keyless run is green when the tool
is correct. Run it with a key before relying on it as a gate — see
[Licensing](#licensing).

Several of these checks are regressions rather than smoke, and each says which
bug it guards. They are worth reading before adding one: every bug they cover
shipped green, and every one would still pass a check that only asserted "exit 0
and the file exists".

**No new dependencies, and one fewer than there were.** Each implementation
uses only what the published samples in its own language already use, and none
of the four has an argument-parsing library. All four once did: C# on
`McMaster.Extensions.CommandLineUtils`, C++17 on CLI11, Python on `argparse`,
each wrapped behind the same hand-written argument table. The four parsers
disagreed about which tokens are options, what a repeated option means and what
an unknown argument costs, so every one of those decisions had already moved
into shared code --- leaving each library holding only the match from a token to
a row of the table, which is twenty lines to write. So C# depends on Document
Filters alone, and C++17 on Document Filters alone. xUnit and Catch2 are
test-only and are never restored by anyone running the sample; the end-to-end
script needs nothing beyond Python itself.

The same rule applied again in [`python/`](./python): the published binding, and
`unittest` from the standard library. Nothing from PyPI.

And again in [`java/`](./java), which never had a parser to lose: the published
samples in `samples/java` bring in none, so this port was written against the
hand-written table from the start --- and it is the shape the other three
converged on. Its only dependency is the binding jar already vendored
at `bindings/java/lib`, and its suite is a ~200-line runner on the JDK alone
rather than JUnit. The build is Gradle through the wrapper, at the same
version `samples/java` already pins.

## What CI checks

Two status checks are meant to be required on `master`. They ask different
questions and neither substitutes for the other:

| Check | Workflow | Asks |
| --- | --- | --- |
| `CI gate` | [`cli.yml`](../.github/workflows/cli.yml) | did every affected suite pass? |
| `Ports landed` | [`ports-landed.yml`](../.github/workflows/ports-landed.yml) | did the three translations move with the reference? |

**Why one gate and not one check per language.** A workflow skipped by a
`paths:` filter reports no status at all, so a required check that gets
filtered out leaves the pull request waiting forever for something that will
never arrive. `cli.yml` therefore always runs, filters *inside* itself, and
ends in a `ci-gate` job that reads every other job's result. A job skipped
because nothing it covers changed counts as a pass; a failure or a
cancellation anywhere fails the gate. `ci-gate` is the only context from that
workflow branch protection should name.

What runs inside it:

| Job | What it proves |
| --- | --- |
| Detect affected suites | which languages this change touches; everything below is filtered on it |
| C# / C++17 / Python / Java | each implementation builds and its own suite passes, on each OS in its matrix |
| Contract moves first | a behaviour change carries something the ports can be moved by: a shared fixture, or a test in that language's own suite |
| Cross-implementation parity | all four binaries run the same command lines and are compared byte for byte. The only job that compares the implementations to *each other* |
| Validate release version, Package, Publish | the release path, which runs only on a manual dispatch with `release` ticked |
| CI gate | the aggregate of all of the above, and the required check |

### The two labels

Both checks have an escape hatch, and they are easy to confuse:

| Label | Says | Stands down |
| --- | --- | --- |
| `contract-unchanged` | this change needs no shared fixture and no new test | Contract moves first |
| `ports-unaffected` | this reference change has no counterpart in the ports | `Ports landed` |

A refactor needs both halves understood. It pins nothing new, so
`contract-unchanged` is right --- and it still has to be mirrored in the three
ports, because the whole value of the mirror is that two implementations can
be diffed by eye, and a rename landing only in `csharp/` destroys that. So
`contract-unchanged` deliberately does **not** stand down `Ports landed`.
`ports-unaffected` is the narrow case: a C#-only build detail, or a `csharp/`
construct the other three already express correctly their own way. It is a
claim you sign for.

### How `Ports landed` decides

It looks at the commits on your branch, not at any agent's run history ---
porting by hand and committing it yourself satisfies it identically. A check
only a bot could turn green would make the bot mandatory, and it is not.

It goes red while a port pull request is still open against your branch, and
while any port's newest commit is not a descendant of the reference's newest
commit. "After" is ancestry rather than timestamps, which a rebase rewrites.
Push a further change to `csharp/` and the ports are stale again, so the
check reopens until they catch up.

### Running the same checks locally

The five verification rings, run outward --- each port's own suite, the shared
fixtures inside it, `compare_inventory.py`, `end2end.py`, then `compare.py`
--- are what CI runs and are all runnable here. Stop at the first failure and
fix it there: an inner ring's failure explains itself, an outer one does not.
The commands, and the table of what each ring is blind to, are in
[`design/porting/workflow.md`](./design/porting/workflow.md).

## Exit codes

| | |
| --- | --- |
| `0` | success |
| `1` | one or more files failed under `--on-error skip` |
| `2` | usage error |
| `3` | nothing matched |
| `4` | output conflict — several inputs into one file, or an existing file without `--overwrite` |
| `5` | could not initialise Document Filters |
| `6` | `--max-time`, `--max-memory` or `--timeout` exceeded |
| `130` | interrupted |

Console output is minimal. Extracted **document content** is byte-exact — the 
tool's job there is to not corrupt what the engine returns.

## Licensing

For unrestricted use, set `DF_LICENSE_KEY` or pass `--license-key`.

Without one, Document Filters runs in licenseless evaluation, which restricts
which input formats open and which `--to` targets are allowed, and watermarks
rendered pages. See [EVAL.md](../EVAL.md) for the terms.

Worth knowing how a restriction arrives: not as a startup error, but as
`E_DOCUMENT_FAILED` on the document, so a refused format looks at first like a
problem with the file.

`--library` (or `DF_PATH`) points at the native libraries. It defaults to the
directory holding this executable — for the Python port, the directory holding
its own package — and never to the current directory — that is also
where the engine looks for its fonts, and a wrong path there produces pages
whose text is laid out but never drawn, rather than an error.
