---
description: Review a port's diff against the C# reference it was translated from
argument-hint: <language id from languages.yml> [base ref, default origin/develop]
---

Review the **$1** changes on this branch as a *translation*, against the
reference they came from.

This is not a general code review. The question is narrower and harder: does
this do the same thing as `csharp/`, on every input, including the ones nobody
wrote a test for.

## Context to load first

- `DocFiltersCli/CLAUDE.md`
- `DocFiltersCli/languages.yml`, the `$1` entry
- `DocFiltersCli/design/porting/traps-csharp.md` and `$1`'s traps file
- The reference diff for the same change, if one is on this branch. Read the
  two side by side; most of what follows is only visible that way.

## What to look for

Work down this list. It is ordered by how much each class of defect has
actually cost on this project.

1. **A silent deviation.** The port does something other than the reference and
   nothing says so. This is the expensive one, because it reads as correct.
   Every difference should carry a comment giving the reference's behaviour
   and the reason, *and* an entry in `$1`'s traps file in the same change. A
   difference recorded only in the comment is half-silent: the next
   translation into `$1` meets the same fork and will not be reading this
   file. Note that "the reference could not be done here" is not the
   threshold --- a different construct reaching the same behaviour is a
   deviation too, and is the kind that goes unrecorded.
2. **A trap from the list, taken.** Go through the categories in `$1`'s traps
   file that the diff touches and check each one. Grep the diff for the
   constructs those entries name in bold. These are, by construction, things
   that compile and pass a casual test.
3. **A reference semantic reproduced by idiom rather than by meaning.** Check
   `traps-csharp.md` for anything the diff touches: case folding, trimming,
   number parsing, regex anchoring, exception filters, newline handling,
   stream encoding, path manipulation. The local one-liner is usually close
   and occasionally wrong.
4. **A dropped test.** Every reference case needs a counterpart here. Run
   `compare_inventory.py` rather than eyeballing it, but also look at whether a
   *translated* case still asserts the same thing --- the inventory check
   compares names, not bodies.
5. **A test that asserts the port's behaviour instead of the reference's.**
   The worst failure mode available, because it makes the gap permanent and
   invisible. A test named for what the code does rather than for what the
   contract requires is the signature.
6. **Structure drift in the source.** A file in the wrong folder, a renamed
   member, a reordered file --- each one costs the side-by-side diff for the
   life of the file. This applies to the source only: the four test suites are
   laid out differently on purpose, and only the test *names* are held in
   step.
7. **A new dependency**, including a transitive one and including test-only.
8. **The pure stages reaching for something real.** Anything up to `Engine/`
   that touches the engine, the filesystem or the console. In Python,
   a binding import at module scope.
9. **House conventions**: copyright header, tabs or spaces to match the
   language, expected-first assertions, no literal non-ASCII in source, no em
   dashes and 80-column wrapping in Markdown.

## What not to raise

- Style preferences the existing code does not already follow.
- "This could be simpler" where the reference is shaped the same way. The port
  mirrors the reference deliberately; simplifying one of the four is drift.
- Anything in `Cli/`, which is allowed to look unlike the reference because it
  wraps a different parser.

## Report

Findings ranked most severe first. For each: the file and line, what the
reference does, what this does, and the input that tells them apart. A finding
without that last part is a guess --- say so, or verify it.

Say plainly if you found nothing. A clean translation is a normal outcome, and
inventing a finding to look thorough wastes the reviewer the agent exists to
help.
