# Cross-implementation parity

The same command lines through every build of the CLI, compared byte for byte.

Nothing in here states an expected value. The only claim is that every
implementation did the same thing.

The commands are **not** a corpus of their own: they are whatever
`tests/e2e/end2end.py` ran. That suite already drives every verb against a real
document, so a second list would be a second thing to keep in step with the
code --- and the first time it fell behind nothing would notice, because a
command list has no assertions to fail. `end2end.py --record` writes down what
it ran; `compare.py` diffs the transcripts. A check added there joins the
parity comparison for free.

## Why this exists when the other suites already pass

Each of the others is blind to something, and the blind spots line up.

| Suite | Covers | Cannot see |
| ----- | ------ | ---------- |
| `tests/conformance/` | argv → plan → message → exit code, against a shared contract | anything below the plan: it runs stages 1–3 through `--dry-run` and executes nothing |
| `tests/<language>/` | execution, against a fake engine | a case dropped in translation — each suite is a *translated copy*, so a missing test is simply absent |
| `tests/e2e/` | the real library, real files | *equality*: every check asserts a property ("exit 0", "one file per page"), and two implementations can satisfy all of them while disagreeing byte for byte |

So an implementation can pass everything and still print different bytes. That
is not hypothetical — the first run of this suite found the C++17 port writing
CRLF where the reference writes bare LF, in output every other suite had
already approved.

## Running it

Record each implementation, then diff the transcripts:

```bash
cd ../e2e
python end2end.py --cli ../../csharp/bin/Release/net8.0/docfilters --record /tmp/csharp.txt
python end2end.py --cli ../cpp17/build/bin/docfilters              --record /tmp/cpp17.txt
python end2end.py --cli python                                     --record /tmp/python.txt
python end2end.py --cli ../../java                                 --record /tmp/java.txt
python end2end.py --cli ../../csharp/bin/Release/net8.0/docfilters --record /tmp/csharp-repeat.txt

cd ../parity
python compare.py /tmp/csharp.txt /tmp/cpp17.txt /tmp/python.txt /tmp/java.txt --repeat /tmp/csharp-repeat.txt
```

The first transcript is the reference; every other is compared against it, and
each is labelled by its filename. Exit 0 means they agree.

`--repeat` is a second transcript of the *reference*. It is what makes the
comparison trustworthy — see "Nondeterminism is measured" below. Without it
nothing is masked, and the run says so in its header.

No license needed: `end2end.py` runs licenseless by default and skips what
evaluation forbids, and the document it uses is committed beside it. A clean
checkout can do all of this.

## What is compared

Per invocation, per implementation: the exit code, raw stdout, raw stderr, and
a SHA-256 of every file in the scratch directory afterwards.

Records are keyed by the check that made them plus their position within it,
not by position in the file. Position alone would misalign the moment one
implementation skipped a check the others ran, and every later record would
then be compared against the wrong one.

**Raw bytes, never text.** Python's text mode applies universal-newline
translation, and `str.splitlines()` treats CRLF and LF as the same separator.
Either one would erase exactly the drift this suite exists to find. `compare.py`
splits on `b"\n"` and reports lines through `repr`, so a stray `\r` is visible
in the diff rather than silently equal.

## Nondeterminism is measured, not guessed

Before comparing anything, the reference implementation runs the command
**twice**. Whatever differs between those two runs — a timestamp, a duration, a
PID in a path — is nondeterminism rather than disagreement, and is excluded
from the comparison for that command.

Doing this by measurement rather than by a list of regexes means a new source
of nondeterminism is handled the first time it appears, instead of the first
time it reddens a build nobody wants to debug. It is also what keeps this from
becoming a flaky gate that people learn to re-run until it passes.

## What is deliberately absent

**`--help` output.** `tests/conformance/README.md` excludes it from the
contract: each language's parsing library formats usage its own way, and none
can be made to match without replacing the renderer, which the
no-new-dependencies rule forbids. `help --json` *is* compared, because it
renders from the shared argument table rather than from the parsing library.

**`--time`, `--progress`, `--logmem`.** Their figures differ every run. The
self-reproducibility pass would mask them anyway; leaving them out states that
on purpose rather than by accident.

**Anything `end2end.py` does not run.** The command vocabulary is that suite's,
by design. Widening parity coverage means adding a check there, which is the
same edit that widens its own coverage.

## A check one implementation cannot run

`end2end.py --skip "<check>=<reason>"` does not run it, and the transcript
records that it did not, with the reason. `compare.py` then knows the missing
commands are missing on purpose, and names them in its header:

```
    java did not run 'docpack describes the document': the 26.3.0 Java native
    library exports no MakeDocPack entry point
```

Without that line in the transcript, a skipped check and a check quietly lost
in translation look identical from here — records the reference has and the
port does not — so the skip has to be written down rather than simply not
happening.

The reason is required, because a skip is a hole in the evidence this suite
produces and one with no reason beside it is a hole nobody can later decide to
close. It should be about something the implementation *cannot* do: a call its
binding does not reach, a platform with no such concept. A difference that
could be fixed is drift, and the gate reporting it is the gate working.

A skip is per check, not per command, so it costs whatever else that check
covered — see the `--skip` in `.github/workflows/cli.yml`, which gives up the
PDF, PNG and text legs of a check to skip its docpack one. That is the price
of the simple version, and it is the reason a skip is worth removing as soon
as it can be.

The same mechanism carries the skips nobody chose: a check licenseless
evaluation forbids, or one that needs a slower document than the one to hand.
Those are skipped by every implementation and so compare equal; the header
lists only the ones that are not shared.
