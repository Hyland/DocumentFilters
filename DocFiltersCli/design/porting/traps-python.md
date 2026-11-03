# Python traps, for porting from the C# reference

Constructs that run, pass a casual test, and do something other than what the
C# they were translated from does. Everything here was hit, measured or
deliberately avoided while building `python/`; nothing is folklore. Each entry
names the file whose comment records it, under `python/docfilters_cli/` unless
the path says otherwise.

**How to use it.** Before porting a C# delta to Python, read the category that
covers what the delta touches. After writing the port, grep your diff for the
constructs named in bold. Most of these leave the suite green: they differ
from the reference only on inputs the tests do not happen to use, or only on
one platform.

The generating rule behind most of the list: **Python's defaults are
Unicode-aware and host-aware where .NET's are fixed.** `str.lower()`,
`str.isspace()`, `str.splitlines()`, `int()` and a text stream's line endings
all answer a broader question than the C# they appear to translate, so the
port states the narrower rule itself.

---

## Errors and control flow

### An `except` clause cannot decline a match

**C#** `catch (Exception error) when (!(error is CliError))` filters before
catching, so an exception the filter rejects travels on untouched. **Python**
has no filter: an `except` that matches has caught.

Every command's open path translates the reference's two `when` filters. The
order of the clauses is the filter: `except CliError: raise` first, then
`except Exception`, so a usage error, a limit or an interrupt is not restated
as this document's failure. A condition on the exception's contents (the
engine's `CANCELLED`) becomes an `if` inside the clause, with the other branch
written out, because there is no falling through to the next clause.

**Rule** Translate each `when` into clause order or an explicit branch, and
re-raise with a bare `raise` so the original traceback survives. See
`commands/convert_command.py`, `commands/extract_command.py`,
`commands/list_command.py`, `discovery/input_enumerator.py`.

### `except Exception` is `catch (Exception)`, not `except BaseException`

**C#** `catch (Exception)` does not stop the failures .NET considers beyond
catching. **Python** `KeyboardInterrupt` and `SystemExit` derive from
`BaseException` alone, so `except Exception` lets them travel, and a bare
`except:` or `except BaseException` would swallow Ctrl+C.

**Rule** `except Exception` wherever the C# says `catch (Exception)`. See
`engine/document_filters_engine.py` and the commands above.

### `with None` raises where `using (null)` does nothing

**C#** `using` on a null reference disposes nothing. **Python** `with None:`
raises `AttributeError` (no `__enter__`), so an optional resource cannot be
written as a `with`.

The combined canvas and writer in `--combine` are each optional, and the
reference's two `using` blocks also fix the order in which they close --- before
`--view` runs, because a PDF is not finished until its trailer is written.

**Rule** `try`/`finally` with explicit `is not None` guards, both variables
initialised to `None` before the `try` so a failure opening the second still
closes the first. See `commands/convert_command.py`,
`commands/identify_command.py`, `commands/meta_command.py`,
`commands/list_command.py`.

### `set.add` does not say whether the item was new

**C#** `HashSet<T>.Add` returns `false` for an item already present.
**Python** `set.add` returns `None` either way, so `if not claimed.add(key)`
is always true and the check silently never fires.

**Rule** Test membership, then add. See `_claim` in
`execution/subfile_extraction.py`.

### A reference name may already be a builtin

**C#** `InterruptedError` is the reference's own class. **Python** has a
builtin `InterruptedError` (an `OSError` for `EINTR`), so reusing the name
shadows it in that module and means something else everywhere else.

**Rule** Rename and say so where the class is defined: the port's is
`RunInterruptedError`. See `runtime/interruption.py`.

---

## Text and encoding

### `str.lower()` and `str.swapcase()` are Unicode-aware

**C#** the reference folds A-Z alone for tokens (`--to`, option names) and
flips A-Z/a-z alone when probing a directory's case rules. **Python**
`str.lower()` folds the Kelvin sign to `k` and turns some capitals into two
characters; `str.swapcase()` flips the whole of Unicode, so the probe would
look for a different name than every other port.

**Rule** Write the ASCII fold out character by character. See `fold_token` in
`configuration/value_parsers.py` and `_flip_case` in `runtime/case_probe.py`.

### `str.isspace()` and `str.strip()` include four characters .NET does not

**C#** `char.IsWhiteSpace` and `string.Trim()`. **Python** also counts
U+001C-U+001F (the information separators) as whitespace, so an entry ending
in one is a different token here than in the other ports.

**Rule** Subtract those four from Python's answer rather than writing a table
of Unicode whitespace. See `is_whitespace` and `trim` in
`configuration/value_parsers.py`. A character-set trim such as `Trim(';')` is
not affected; see "Verified non-traps".

### `str.splitlines()` breaks on more than line feeds

**C#** reading a list file line by line breaks on CR, LF and CRLF.
**Python** `str.splitlines()` also breaks on vertical tab, form feed, U+001C-
U+001E, U+0085, U+2028 and U+2029, several of which are legal in a filename,
so a `--files-from` entry naming one would become two paths.

**Rule** Split on `'\n'` and drop the final empty element. See
`read_all_lines` in `runtime/system_file_system.py`.

### A text stream translates `'\n'` on Windows and uses the locale encoding

**C#** `new StreamWriter(stream)` writes UTF-8 with no byte order mark and
writes `'\n'` as `'\n'`. **Python** `io.TextIOWrapper` defaults to the
locale's encoding (cp1252 on a typical Windows machine) and to translating
every `'\n'` into the host's line ending. Document text written that way is
correct and its bytes are not, on Windows only.

**Rule** `encoding='utf-8', newline=''` on every text stream that carries a
payload, and `reconfigure(newline='')` on the console streams. See
`_text_writer` in `commands/convert_command.py`, `execution/text_conversion.py`,
`runtime/system_console.py`.

### A lone surrogate cannot be written literally

**C#** `JsonWriter.Escape` walks UTF-16 units, writes a well-formed pair
literally and escapes an unpaired unit as `\uXXXX`, so it has to look at the
neighbouring unit (`IsPaired`). **Python** a `str` holds code points: a real
pair is one character above U+FFFF, and any code point in U+D800-U+DFFF is
unpaired by construction. So no `IsPaired` is translated, and a pair spelled
as a high and a low surrogate escape in the source is two lone code points here, escaped where
C# would write the emoji. Left literal, the UTF-8 encoder raises
`UnicodeEncodeError`.

**Rule** `ord(c) < 0x20 or 0xD800 <= ord(c) <= 0xDFFF` and no look-behind. See
`JsonWriter.escape` in `runtime/json_writer.py`.

---

## Numbers and patterns

### `int()` and `str.isdigit()` accept what `int.TryParse` rejects

**C#** `int.TryParse(text, NumberStyles.None, InvariantCulture, out value)`
accepts ASCII digits only, within `Int32`. **Python** `int()` accepts
`" +5 "`, `"5_0"` and non-ASCII digits; `str.isdigit()` accepts non-ASCII
digits too; and Python integers have no upper bound, so an overflow that is a
usage error in C# is accepted here and fails somewhere further in.

The same difference sits in the Python binding itself, which parses a
bookmark's page index with a bare `int()` where the .NET binding uses
`int.TryParse`, so a malformed destination gives `-1` in one and a number in
the other.

**Rule** Match `^[0-9]+\Z` and state the bounds (`2147483647`). See
`configuration/value_parsers.py` and `_try_parse_int` in
`engine/document_filters_engine.py`.

### `$` matches before a trailing newline

**C#** the reference rejects `"5\n"`. **Python** `re`'s `$` also matches
immediately before a final `'\n'`, so `^[0-9]+$` accepts it.

**Rule** `\Z` for the end of the string. See `configuration/value_parsers.py`.

---

## Filesystem

### `os.rename` refuses an existing destination on Windows

**C#** `File.Move(source, destination, overwrite: true)` replaces on every
platform. **Python** `os.rename` replaces on POSIX and raises
`FileExistsError` on Windows.

**Rule** `os.replace`. See `move_file` in `runtime/system_file_system.py`.

### `os.path.splitext` is not `Path.ChangeExtension`

**C#** `Path.ChangeExtension("page.unknown", "png")` and
`ChangeExtension(".unknown", "png")` both replace what follows the last dot.
**Python** `os.path.splitext` treats a leading dot as part of the name, so
`.unknown` has no extension and would become `.unknown.png`.

**Rule** Find the last dot after the last separator yourself. See
`_change_extension` in `execution/canvas_conversion.py`.

### `os.path.islink` does not see junctions

**C#** the reference tests `FileAttributes.ReparsePoint`, which is set on
symlinks and junctions alike. **Python** `os.path.islink` reports symlinks
only, so a junction is walked into, and `-L`/`--follow` can recurse forever on
Windows.

**Rule** `islink` or the reparse-point attribute, and `os.path.lexists` first
so a dangling link still counts as a link. See `is_symbolic_link` in
`runtime/system_file_system.py`.

### `os.listdir` reads the whole directory

**C#** `EnumerateFileSystemEntries` is lazy, and the case probe stops at the
first name with a letter in it. **Python** `os.listdir` builds the full list
first, so a directory of a hundred thousand files costs a hundred thousand
entries to answer a question one settles.

**Rule** `os.scandir` inside a `with`, and pass a generator on. See `_probe`
in `runtime/system_file_system.py`.

---

## Tests

### unittest has no `Theory`

**C#** an xUnit `[Theory]` with one `[InlineData]` per case is reported as one
test per case, under one name. **Python** `unittest` cannot parametrise a
method, and generating a method per case would give names the C# does not
have, which `compare_inventory.py` would count as extra and unmatched.

**Rule** One test under the reference's name (local casing), looping over the
cases inside `with self.subTest(...)`, so each case still fails separately. See
`test_a_dash_token_that_is_not_an_option_spelling_is_a_path` in
`tests/python/unit/argument_binding_tests.py`.

---

## Modules and the native boundary

### Importing the binding at module scope breaks the pure suite

**C#** referencing the binding's types costs nothing until one is constructed.
**Python** a top-level `import DocumentFilters` fails when the package is
absent, and `program.py` imports the module that chooses the engine, so the
whole unit suite would need the binding installed to run.

**Rule** Import the binding inside the function that first needs it, and keep
every module above the engine seam free of it. See the "THE IMPORT IS LAZY"
banner in `engine/document_filters_engine.py`.

### Import cycles are resolved at call time, not at compile time

**C#** classes in one assembly may refer to each other freely. **Python** a
cycle of module-scope imports fails or yields a half-initialised module,
depending on which module is imported first.

**Rule** Break a cycle at its single call-time edge with a deferred import,
and say where in a comment so the next import does not reintroduce it. See
`commands/convert_command.py`, `commands/meta_command.py`,
`execution/extractor_open_settings.py`.

### There is no `Stream.Null`

**C#** `Stream.Null` is a sink the framework provides. **Python** `os.devnull`
is a path, not a stream. A stand-in that pins its position at zero also breaks
the binding's stream bridge, which answers every engine seek with `tell()`.

**Rule** A write-only sink that tracks its position. See `_NullStream` in
`execution/canvas_conversion.py`.

---

## Verified non-traps

Things that look like the list above and are not defects.

- **`str.strip(';')` is `Trim(';')`.** Both are character-set trims; neither
  touches whitespace, so an option string ending in a space keeps it in both.
  See `execution/canvas_conversion.py`.
- **A `dict` keyed by `str` compares ordinally.** That is what the reference's
  dictionaries are built with, and Python has no culture-sensitive comparison
  to opt out of. See `tests/python/fakes/fake_document_filters_engine.py`.
