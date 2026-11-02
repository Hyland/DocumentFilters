# docfilters — the Python implementation

The same command line as the [C# reference](../csharp), in Python: the same
verbs, the same flags, the same defaults, the same output model, the same
messages and the same exit codes. Start with [the tool's README](../README.md) —
it describes what `docfilters` does and why the code is arranged the way it is.
This file is only about running *this* implementation.

## Quick start

```bash
cd python
python -m pip install ../../bindings/python     # the Document Filters binding

# Point DF_PATH at the native libraries (absolute path required):
DF_PATH=/absolute/path/to/document-filters/native python -m docfilters_cli \
    convert --to pdf --out-dir out/ report.docx
```

Python 3.8 or newer. Nothing else to install: everything above the binding is
standard library. See [What has to be findable](#what-has-to-be-findable) for
where the native libraries come from.

## Running it

The package has a `__main__.py`, so it runs as a module:

```bash
python -m docfilters_cli help
python -m docfilters_cli help convert
python -m docfilters_cli convert --to png --pages 1-2 report.docx --out-dir out/
```

**Those examples assume the environment is set up** — the binding installed and
the native library path configured. Without that, the first two print help and
exit, but the third fails with `E_INIT_FAILED: could not initialise Document
Filters`. See below for what needs to be findable.

The package directory needs to be on the import path — `cd python` is the short
way, `PYTHONPATH=/path/to/DocFiltersCli/python` the way that works from
anywhere. There is no packaging metadata in the tree, which is why `docfilters
version` reports the tool's own version as `unknown` from a clone; the reference
prints the same word for an assembly carrying no version.

### What has to be findable

Two things, and they are separate questions:

| | |
| --- | --- |
| the binding | `import DocumentFilters` has to work — `pip install ../../bindings/python`, or the binding's own directory on `PYTHONPATH` |
| the engine | the native libraries (ISYS11df.dll on Windows, libisys11df.so on Linux), which the binding package does not carry |

The engine libraries are resolved exactly as the reference resolves them:
`--library`, then `DF_PATH`, then the directory the tool's own code lives in —
never the current directory. For a pip-installed binding that third answer holds
no library, and the binding is then left to look inside its own package, so a
checkout run this way needs `--library` or `DF_PATH`.

The native libraries come from one of:

- **A Document Filters installation** — the binary directory of the SDK as it
  was installed or unpacked.
- **The C# CLI's NuGet packages** — building `csharp/DocFiltersCli.csproj`
  restores Document Filters from NuGet, which puts the native libraries in
  `csharp/bin/Debug/net8.0/runtimes/<rid>/native/` (e.g. `win-x64` on 64-bit
  Windows). The e2e tests use this path as a fallback.

Point `DF_PATH` or `--library` at whichever directory holds them. `DF_PATH`
must be an **absolute path** — a relative one is rejected with `WinError 87: The
parameter is incorrect` on Windows, or the equivalent on other platforms.

One exception worth knowing, and it is shared with the reference: `version`
takes no options at all, so `--library` does not reach it. `DF_PATH` does. The
built executable appears not to need either only because the libraries sit
beside it.

## Building and testing

There is nothing to build.

```bash
python tests/python/run_tests.py                   # the whole suite
python tests/python/run_tests.py '*plan*'          # a subset, by file name
python tests/python/run_tests.py PageRangeTests    # or by test name
```

Run it from `DocFiltersCli`. A pattern is tried against file names first and, if that
matches nothing, against test ids, so a class or method name selects what it names —
and a pattern that selects nothing at all exits non-zero, rather than printing `OK`
over an empty run.

Standard-library `unittest`, no license, no native
library, no scratch directory — the same properties the C# suite has, and for
the same reason: the engine sits behind `engine/` and the filesystem behind
`runtime/i_file_system.py`, so the tests supply their own.

`python -m unittest discover` is deliberately not the entry point. The suite
imports both the package under test and the fakes beside it, from two
directories that no packaging step has put on the path; `run_tests.py` is those
two `sys.path` entries and nothing else.

The suite includes the four shared fixtures in
[`tests/conformance/`](../tests/conformance), which are the authority for the
CLI surface and are run identically by every implementation. A port is done
when they pass.

### End-to-end tests

The end-to-end script drives any implementation from the one file:

```bash
python tests/e2e/end2end.py                 # the newest built executable
python tests/e2e/end2end.py --cli python    # this port, from python/
```

`--cli` names the implementation: a built executable for the C# and C++17 ports,
or the directory holding this package for this one, which has no build output to
be found by. Given a directory, the script composes `PYTHONPATH` from it and the
published binding, and takes the native library out of the C# build output for
`DF_PATH`. `--binding` and `--library` override either guess. Everything else
about the run is the same for every implementation, including which checks are
skipped without a license.

## Layout

The modules are the reference's folders, lower-cased, holding the same things in
the same order:

```
python/docfilters_cli/
├── cli/            the argument table and argparse — the only per-language part
├── configuration/  the options model, value parsers, cross-argument validation
├── planning/       format catalogue, output planner, name templates, page ranges
├── discovery/      the filesystem walk
├── engine/         the Document Filters seam — the only code importing the binding
├── execution/      the Document Filters code, written against that seam
├── commands/       one thin orchestrator per verb
├── runtime/        console, filesystem, JSON writer, batch loop, exit codes
├── program.py      argv in, exit code out
└── __main__.py     python -m docfilters_cli
```

`Cli/` is the one folder that is not a port. The reference uses
`McMaster.Extensions.CommandLineUtils`; this uses `argparse`, because that is
what the published Python samples use. The argument *table* is the same data in
both, and the fixtures check the result rather than the library.

## Where it differs from the reference, and why

Three places, each one commented where it is:

| | |
| --- | --- |
| `runtime/system_console.py` | The standard streams are put into UTF-8 with a replacement fallback, and their newline translation is turned off. Without the first, a report naming a document with an em dash ended a redirected run in `UnicodeEncodeError` rather than in an exit code; without the second, a line break *inside* engine text — an option description is several lines of Markdown table — came out as `\r\n` on Windows where the reference writes `\n`. UTF-8 is what .NET writes on Linux and macOS. On Windows it writes the console's code page, or the ANSI code page with no console attached, in both cases with best-fit substitution — which loses the em dash and is not reproduced here. |
| `engine/document_filters_engine.py` | `close()` tolerates exactly one message. The binding declares `Close_Instance` with `restype = None` and then checks what it returned, so every successful `Close()` raises. The tolerance is keyed on the synthesised message and nothing wider, so a real failure still raises; `tests/python/unit/document_filters_engine_tests.py` pins that. |
| a subfile's timestamp | An entry whose container records none reads as absent here and as a date in 1601 in the reference, whose `DateTime.MinValue` guard is dead code — the .NET binding turns a zero `FILETIME` into 1601-01-01 and never into `MinValue`. Deliberately not bug-compatible. The two bindings also convert differently, .NET in floating point and Python in exact microseconds, so a timestamp that *does* exist can differ in its last fractional digit. |

Nothing else is a deliberate difference. `--help` text, the `--time` /
`--progress` / `--logmem` figures, the version strings and a docpack's
millisecond `<created>` stamp are
[not compared across implementations](../README.md#what-is-deliberately-not-compared);
everything else is, down to the byte — verified against the reference on real
documents across every verb, every other entry of a docpack included. The two
gaps left are non-ASCII report text on Windows and the missing subfile
timestamp, and the reference is the side in the wrong in both.

**No new dependencies.** The binding, `argparse` and `unittest` — the first is
what the published Python samples already depend on, the other two are the
standard library. Nothing is installed from PyPI to run this tool or its tests.
