# How a port is made, and how a change reaches all four

This CLI exists four times. One of them is the reference; the other three are
translations of it that have to behave identically, down to the bytes. This
file is the method that gets them there --- written after doing it three times,
so it is a record rather than a proposal.

Two audiences, one method. Read it before porting the whole tool into a fifth
language, and before moving a one-line change from the reference into the three
ports. The second is the common case and the harder one to do well, because
nothing about a small change announces which of these steps it needs.

The companions to this file:

| File | Holds |
| --- | --- |
| [`traps-csharp.md`](./traps-csharp.md) | what the reference's own constructs mean, where a local idiom would be close but wrong |
| [`traps-cpp17.md`](./traps-cpp17.md) | C++17: constructs that compile, pass a casual test, and do something else |
| [`traps-python.md`](./traps-python.md) | the same for Python |
| [`traps-java.md`](./traps-java.md) | the same for Java |
| [`../../CLAUDE.md`](../../CLAUDE.md) | the rules, as constraints rather than prose |
| [`../../languages.yml`](../../languages.yml) | where each implementation lives and how it is built |

---

## The reference moves first, and it carries evidence

**The reference, then the ports --- and never a port on its own.** That half is
absolute. A behaviour implemented first in `cpp17/` and back-filled into
`csharp/` is drift that happens to point the other way.

The second half is about what travels with it. A change to the reference's
behaviour has to leave behind something a port can be *moved by* and *checked
against*, because a port translated from code alone has nothing to verify
itself with. There are three places that something can live, and which one
depends on what the change is:

| Where the behaviour is pinned | Covers |
| --- | --- |
| `tests/conformance/` | the CLI surface: arguments, the resolved plan, messages, exit codes, `--out-name` rendering, the format catalogue |
| the reference's own suite, `tests/csharp/` | everything below the planning seam, against a fake engine and an in-memory filesystem --- most of `Execution/` |
| `tests/e2e/end2end.py` | behaviour only the real engine shows: a call it rejects, an option that never arrives, a file written but blank |

**Most changes do not reach the fixtures**, for two separate reasons, and it
is worth knowing which one applies.

They describe the CLI surface, so a fix inside the page loop, a correction to
how a subfile name is derived or a stricter check on an engine return code
changes nothing they say --- writing those in would make the contract a
changelog. And `tests/conformance/README.md` scopes them to stages 1 to 3:
argv to a resolved plan, run through `--dry-run` with nothing executed, so
everything below that is invisible to them by design. Either way, if the
change is in `Execution/`, the shared fixtures are the wrong tool and a
reference unit test or an end-to-end check is the right one.

What is not negotiable is that *something* goes with the change. The fixtures
are the strongest of the three, where they apply, because they are written by
hand from what the tool is meant to do rather than generated from the
reference: a file dumped from `csharp/` could only prove the others agree with
it, and would never catch an argument nobody implemented. A case in
`tests/conformance/` gives the port a definition of done that is not "it looks
right" --- *make these cases pass*.
A reference unit test is weaker but still translatable, which is the property
that matters.

`end2end.py` is the third and is worth knowing early, because it is where a
good deal of real behaviour gets pinned. It runs a built CLI as a subprocess
against a real document and the real library, so it sees what no fake can;
`--cli` points it at any of the four, so one suite covers all of them. It is
also the recorder the parity comparison reads --- see the rings below.

`.github/workflows/cli.yml` has a `contract-order` job enforcing exactly this:
it fails a pull request that changes one implementation's source without
touching either the shared fixtures or that language's own tests. Either
satisfies it. It is soft --- the `contract-unchanged` label overrides it,
because a refactor or a crash fix legitimately pins nothing new. The point is
to make the rule visible at the moment it is being broken, not to win the
argument.

---

## Porting a whole tool

### 1. Read the contract before the code

Start with `tests/conformance/`, not with `csharp/`.
Reading the reference first teaches you one implementation's choices and leaves
you unable to tell which of them are contract. Reading the fixtures first
teaches you what is *required*, and the reference then reads as one way of
satisfying it.

`tests/conformance/README.md` scopes what the fixtures do and do not cover.
They stop at the plan: argv to a resolved plan, a message and an exit code.
Everything below that --- the engine calls, the page loop, the bytes written
--- is yours to get right by other means, which is what the rest of this file
is about.

### 2. Port in dependency order, pure stages first

The folder layout is the order:

```
Runtime/  ->  Configuration/  ->  Planning/  ->  Discovery/  ->  Engine/  ->  Execution/  ->  Commands/  ->  Cli/
```

Everything up to `Engine/` is pure: no engine, no filesystem, no console. It is
the half that can be finished and tested before the binding is involved at all,
and the half where a translation error is cheapest to find. Each of the three
ports took roughly this shape, and each found the pure half to be the larger
one.

`Engine/` is the seam, and it is the one place that names a binding type. Each
member maps to exactly one Document Filters call, so the question "does this
binding expose that call?" is asked once per member rather than discovered
halfway through `Execution/`. Where a binding does not, say so in that port's
traps file under a gap heading, and compose it from what the binding does
expose or report that it cannot --- do not silently do something else.

`Cli/` **was** the one folder allowed to look unlike the reference, because
each implementation held whatever argument parser its own published samples
already used, behind the shared hand-written argument table. None of them has
a parser now, so it is a translation like any other and the rule above applies
to it in full. The table is still the contract; binding argv against it is now
shared logic rather than a per-language detail.

### 3. Mirror the source file for file; mirror the tests name for name

**Translate the source, don't rewrite it.** Same folders, same file names
transliterated to the language's convention, same type and member names, same
order within a file. The reason is mechanical: it has to be possible to put two
implementations side by side and diff them by eye, and every departure costs
that for the whole life of the file.

**The tests are held to a weaker rule, on purpose: the names, not the
layout.** What must hold is that every case in `tests/csharp/` has a
counterpart in every port, under the same name in the local casing. That is
what `tests/parity/compare_inventory.py` checks, and it is what tells a
translated suite from one that quietly dropped a third of its cases. A port
may have *more* tests --- a language's own quirks need their own --- but never
fewer.

Where a case *lives* is the local suite's business, and the four already
differ. C++17 has a `Unit/StringsTests.cpp` with no counterpart at all,
because the reference gets `Trim` and `ReadAllLines` from its framework and
has nothing to test. Java files its engine-adapter tests outside the test
package entirely, beside the code they exercise. Python has a `fakes/`
package where C# has a `Fakes/` folder of a different shape. None of this is
drift.

`compare_inventory.py` says so itself and explains why: it records each test's
class or file as a *scope*, uses it only to tell two same-named tests apart,
and never compares scopes between languages --- "C++17 files its fixture tests
where C# files them under another class, and demanding those line up would
reject 25 pairs that are correctly matched today."

### 4. Deviate out loud or not at all

Sometimes the local language cannot do what the reference does. More often it
can, by another route --- a different construct, a different type, a test that
has to be written differently to pin the same thing. Both are deviations, and
reading the rule as being about the first kind is how the second kind goes
unrecorded: the behaviour matches, so nothing feels like it was given up, and
the next translation meets the same fork with nothing to go on.

Every deliberate difference is written down twice, at the moment the choice is
made, because it has two readers who never meet.

A **comment**, beside the code, saying what the reference does, what this does
instead, and why. That reader is looking at this line already.

An **entry in the port's traps file**, in the same change. That reader is
translating the *next* change into this port and has no reason to open the file
your comment is in. Three traps files exist because somebody wrote both at the
time; none of them was ever assembled afterwards by going back through the
comments, and nobody is going to. A difference recorded only as a comment is
one the following translation meets cold and resolves its own way.

A gap --- something the port cannot do at all --- is written down in one of two
further places, and neither of them is a deletion.

A **unit test the port cannot express at all** gets a line in
`tests/parity/port-exemptions.txt` naming the language, the test and the
reason. `compare_inventory.py` reads it, and fails if the port turns out to
have that test after all.

An **end-to-end check the port cannot run** --- because the call its binding
needs does not exist --- is left out of the run instead. `end2end.py` takes
`--skip "<check name>=<reason>"`, repeatable, and both the printed report and
the recorded transcript carry the reason, so `compare.py` can tell a
deliberate skip from a check that went missing.

A skip is verified rather than trusted. The check still runs, with its
commands kept out of the transcript so they have nothing to be compared
against, and a skip whose check *passes* fails the run and names the waiver
to delete. This is the same rule `port-exemptions.txt` has always had, and it
is what stops a waiver outliving the gap it was written for --- the engine
release that closes one of those two Java skips will not announce itself.

### 5. A faithful port is also a review

Re-deriving a behaviour in another language is the most thorough review the
reference ever gets. The C++17 port found twenty defects in it, eight of them
silent data loss. The Python and Java ports found more.

When the reference turns out to be wrong, fix it **in the reference**, with the
fixtures, and let the fix reach the other ports the normal way. Do not quietly
do the right thing in the new port and leave the reference wrong --- that is
drift with good intentions. Say what you found in the change that fixes it,
where the fix is the evidence.

---

## Verify in widening rings

Each ring sees something the one inside it cannot. Run them in this order,
because the inner ones are fast and localise a failure, and the outer ones only
tell you that something, somewhere, differs.

| Ring | What it proves | Blind to |
| --- | --- | --- |
| **1. The port's own unit suite** | the translation of each unit, against a fake engine and an in-memory filesystem | anything the translation dropped --- a missing test is simply absent |
| **2. `tests/conformance/`** | this implementation agrees with the shared contract: argv, plan, message, exit code | everything below the plan: nothing is executed |
| **3. `tests/parity/compare_inventory.py`** | the suite was translated whole --- every reference test has a counterpart here | whether the tests *pass* for the same reason |
| **4. `tests/e2e/end2end.py`** | the real engine, real files: a call the engine rejects, an option that never arrives, a file written but blank | equality --- each check asserts a property, and two implementations can satisfy all of them while disagreeing |
| **5. `tests/parity/compare.py`** | the implementations agree byte for byte on every command the end-to-end suite runs | anything that suite does not run, and any input the corpus does not contain |

Ring 5 is the only one that compares the implementations to *each other*, and
it is the one that found the C++17 port writing CRLF where the reference writes
bare LF --- in output the four inner rings had already approved.

Ring 5 is also not a backstop. It compares the commands the end-to-end suite
happens to run, against the documents the corpus happens to hold. A difference
that needs an input nobody committed is invisible to all five: that is how the
C++17 port came to trim six ASCII characters where the reference trims
twenty-five, with a green suite in every ring. Widening the corpus is the only
fix for that class, and it is cheaper than it sounds.

---

## Moving one change

The common case, and the one the sync workflow automates. The whole method
above compressed into what a single delta needs:

1. **Decide where the behaviour gets pinned.** If the change alters the
   command line, it starts in `tests/conformance/`. If it alters behaviour
   below the
   planning seam, it starts in a reference unit test, an end-to-end check, or
   both --- see the table at the top. If it alters no behaviour at all --- a
   refactor, a crash fix, a comment --- there is nothing to pin: say so with
   the `contract-unchanged` label. It still gets ported --- that is step 3,
   and the label does not excuse it.
2. **Change the reference**, if not done already, and make the new cases pass 
   there first.
3. **Translate into each port**, one at a time, reading that port's traps file
   for the category the change touches *before* writing it. Grep the finished
   diff for the constructs that file names in bold.
4. **Walk the rings** for every implementation you touched, outward. Stop at
   the first failure and fix it there; an inner ring's failure explains itself,
   an outer one does not.
5. **Land them together.** The four implementations are one tool, and a commit
   that moves one of them is a commit that leaves the tool inconsistent. The
   sync workflow handles this by having each port's pull request target the
   source pull request's branch, so the contract change and all three ports
   merge in one go and the default branch is never in a disagreeing state.

### Having the agent do steps 3 and 4

`.github/workflows/ai-sync.yml` runs `/propagate-delta` for each port and
opens one draft pull request per language, each based on your branch. Add the
**`ai-sync`** label to start it; a maintainer has to approve the run, because
the job reads a cloud credential scoped to an environment that requires a
reviewer. It re-runs when you push to an authority path, updating the same
port pull requests rather than opening more.

`.github/workflows/ports-landed.yml` is the other half, and the one that makes
any of this binding. It keeps a check red on your pull request while a port
pull request is still open against it, and while any port's newest commit is
older than the reference's. It measures the commits on your branch, not the
agent's run history --- so porting by hand and committing it here satisfies it
identically. The agent is a way of doing steps 3 and 4, not a different
method.

### Two labels, two different claims

They are easy to confuse and they do not substitute for each other:

| Label | Says | Stands down |
| --- | --- | --- |
| `contract-unchanged` | this change needs no shared fixture and no new test | `cli.yml`'s `contract-order` |
| `ports-unaffected` | this reference change has no counterpart in the ports | `Ports landed` |

A refactor needs both halves understood. It pins nothing new, so
`contract-unchanged` is right --- and it still has to be mirrored in the three
ports, because the whole value of the mirror is that two implementations can
be diffed by eye, and a rename that lands only in `csharp/` destroys that. So
`contract-unchanged` does **not** stand down `Ports landed`, deliberately.

`ports-unaffected` is for the narrow case where a reference change genuinely
has no counterpart: a C#-only build detail, or a `csharp/`-only construct the
other three express differently and already express correctly. It is a claim
you sign for, and it is not the common case. It got narrower when the parsers
went --- `Cli/` used to supply most of its examples, and now mirrors the
reference like every other folder.

---

## What this method is protecting

Four implementations that agree are worth more than four that are individually
good, because the point of the sample is that a reader can pick the language
they work in and trust that what they learn transfers. Every rule above exists
because something got through without it, and the cost of a rule here is paid
once while the cost of drift is paid by every reader afterwards.
