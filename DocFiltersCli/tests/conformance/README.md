# Conformance fixtures

These files are **shared by all four language implementations** of the
`docfilters` CLI. They are the executable form of the command line contract.

Nothing in here is language specific, and no language owns it. A change to the
CLI is made here first, and porting it then has a definition of done: *make
these cases pass*.

They are written **from the CLI design**, not generated from any
implementation. That direction matters: a file dumped from the C# reference
could only prove the other three agree with C#, and would never catch an
argument nobody implemented.

## Files

| File | Is the authority for | Checked by |
| ---- | -------------------- | ---------- |
| `arguments.txt` | which arguments exist on which command, with short form and arity | the argument table, directly |
| `formats.txt` | every `--to` token: kind, single-file or per-page, hi-def, extension | the format catalogue |
| `out-names.txt` | `--out-name` rendering, and templates that must be rejected | the name template |
| `plan-corpus.txt` | argv → resolved plan, plus every error code, exit code and message | `--dry-run` |


## Format

One format for all four, so each language needs one reader rather than one per
file. Deliberately line-oriented, with no JSON or YAML, so the C++17 and Java
suites can parse it using nothing but their standard library.

A case is one `$` subject line, then any `<` lines saying what the case is
**given**, then `=` lines saying what it must **produce**. Blank lines separate
cases; `#` starts a comment.

```
$ convert --to pdf --out-dir out a.docx
< tree          a.docx
= command       convert
= cardinality   per-input
= target        out/a.pdf
= exit          0
```

The two prefixes are the opposite kind of thing — one describes the world before
the run, the other what the run must produce — so the reader keeps them in
separate lists rather than distinguishing them by key name. Only `plan-corpus.txt`
uses `<` today; the other fixtures need nothing but a subject.

An expected failure states the code, the message and the exit code:

```
$ convert --to png --out-file one.png a.docx b.docx
< tree          a.docx b.docx
= error         E_OUTFILE_MULTI_INPUT
= message       --out-file takes a single input; use --combine to merge several
= exit          4
```

### The keys

Each fixture uses the keys its subject needs. `= exit` is the one they share.

| Fixture | Keys |
| ------- | ---- |
| `arguments.txt` | `= argument <template> <arity>` |
| `formats.txt` | `= kind`, `= single-file`, `= hidef`, `= ext`, or `= error` |
| `out-names.txt` | `= uses-page`, `= render <source> <ext> <page> => <name>`, or `= error` |
| `plan-corpus.txt` | `< tree`, `< link`, `< hidden`; then the plan lines that file's header lists, or `= error` with `= message` |


### Expectations are complete and ordered

The plan corpus compares the **whole** document, in order — not a search for
fragments. A case that listed only some of the plan would let extra lines,
reordered fields and duplicate targets pass in one implementation and not
another, which is exactly the drift these files exist to prevent.

So every case states every line the plan emits. Alignment is not significant:
runs of spaces are collapsed before comparison.

Which lines a plan emits is a property of the **command**, never of the
arguments: each command emits its full line set every time, in a fixed order,
with defaults written out rather than left silent. `plan-corpus.txt` states the
order and the defaults per command.

Two cases with the same command line and the same tree cannot state different
plans, in any language, so `All_cases_for_one_command_emit_the_same_line_set`
and `No_two_cases_share_a_subject_and_a_filesystem` fail the fixture rather than
leaving it for a port to trip over.

### Cases declare the filesystem they run against

`plan-corpus.txt` runs stages 1 to 3 — parse, bind, plan — with the input walk
feeding the planner, so a case states the tree it walks:

```
< tree    corpus/a.docx corpus/sub/b.docx
< link    corpus/elsewhere other
< hidden  corpus/.cache
```

`= inputs` then lists what the walk *selected*, so `--max-depth`, `--glob`,
`--regex`, `--follow` and `--include-hidden` are asserted rather than echoed back
from the arguments.

**Tests never touch the real filesystem**, and that is load-bearing rather than
tidiness. A real directory listing has no defined order and differs between ext4,
NTFS and APFS, so a corpus backed by real files could not be byte-identical across
four languages and three platforms. The fake also models symlinks and hidden
entries as plain sets, which is the only reason `--follow` and `--include-hidden`
can be contract at all: creating a symlink on Windows needs a privilege most
checkouts do not have, and "hidden" is a dot-prefix on POSIX and a file attribute
on Windows.

So the fake's own semantics are part of this contract. They are specified by
`InMemoryFileSystemTests`, which each port translates alongside the cases —
**those tests are a prerequisite, not optional scaffolding.**

A case that walks several files passes `--sort`. Without it a listing's order is
deliberately unspecified — that is what `--sort` exists to fix — and a contract
cannot assert an order that is undefined by design.

### Values are folded, or they are verbatim

An argument whose valid values this file enumerates — `--to`, `--fallback`,
`--on-error`, `--applies-to`, `--log-level` — is matched after an
**ASCII-only** case fold, and the folded value is what the plan prints.
Everything else is used exactly as typed: paths, globs, regular expressions,
`--out-name` templates, `--filter` text, passwords, `--codepage` and the
`--options` string.

ASCII-only is the load-bearing half: a locale-aware fold maps `I` to `ı` under a
Turkish locale, so `--to TIFF` would stop resolving there — in C#, Java and
Python alike, and in C++ where `std::tolower` is locale-sensitive too.

## Failure messages

A failing case reports the fixture, the line, the problem and the whole case,
so a failure is readable without opening this directory:

```
formats.txt:41  $ pdf
  single-file was no, expected yes
  case as written:
    = kind        canvas
    = single-file yes
    = hidef       yes
    = ext         pdf
```

## What is deliberately not compared

**`--help` text.** Each language's parsing library formats usage its own way,
and none can be made to match without replacing the renderer — which the
no-new-dependencies rule forbids.

**`--time`, `--progress` and `--logmem` output.** The figures differ every run.
Their *shape* is consistent; their content is not compared.
