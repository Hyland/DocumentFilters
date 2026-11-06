# C# traps, for porting *from* the reference

The other three lists in this directory are written from a port's side: *my
language does this differently from C#*. This one is written from the
reference's side, and is read in two directions.

**Porting from it.** These are constructs in `csharp/` whose exact semantics
are the contract, not an implementation detail. Each one is a place where the
idiomatic equivalent in another language is close enough to pass its own suite
and still disagree byte for byte. Reproduce the semantics; do not reach for the
local idiom.

**Editing it.** A change to any construct named here is a contract change for
three other implementations, whatever the diff looks like. It goes through the
design and the shared fixtures first, not straight into `csharp/`.

Nothing here is folklore: every entry names the reference call site it comes
from, and most name the port that paid for finding it. Where a claim was
measured, the trap list that measured it says so and is cited.

---

## Text and culture

### `FoldToken` is ASCII-only, and only some values go through it

`Configuration/ValueParsers.cs`. Values drawn from a vocabulary *this tool
publishes* are folded A-Z to a-z and nothing else, so `PDF`, `Pdf` and `pdf`
are one token. A locale-aware fold maps `I` to the dotless `ı` under a Turkish
locale, and `--to TIFF` would stop resolving on a Turkish machine --- in C#,
Java and Python alike, and in C++17 where `std::tolower` is locale-sensitive
too.

**Rule** Port the fold, not `toLowerCase()`. And port *where* it is applied:
the argument table calls it for `--to`, `--on-error`, `--applies-to` and the
two log levels, and for nothing else. Paths, globs, regular expressions,
`--out-name` templates, `--filter` text, passwords, `--codepage` and the
`--options` string are used exactly as typed. Widening that set is a contract
change, and so is narrowing it.

### An enum's name, lowercased, is the wire format

`Execution/StructureReport.cs`, `Planning/FormatSpec.cs`:
`value.ToString().ToLowerInvariant()`. .NET returns the identifier, never
anything localised, so `PageElementType.TextBox` prints as `textbox`.

**Rule** The contract is the *C# identifier* lowercased, not your enum's name
lowercased. Java separates words with underscores where C# runs them together,
so `TEXT_BOX` cannot be the source of this string. See "Enum names are not the
reference's names" in `traps-java.md`.

### `string.Trim()` and `char.IsWhiteSpace` are .NET's set

`ValueParsers.cs` trims each part of a `;`-separated list with the no-argument
`Trim()`. That set is fixed and not culture-sensitive, but it is not the same
as anyone else's. The set is the Unicode one: U+0009 to U+000D, U+0085, and the
space, line and paragraph separator categories --- so the no-break spaces
U+00A0, U+2007 and U+202F are in it, and the four information separators
U+001C to U+001F are not.

Both Python and Java had to build the predicate rather than call the local one,
and arrived at the same definition independently: `is_whitespace` in
`python/docfilters_cli/configuration/value_parsers.py` subtracts U+001C-U+001F
from `str.isspace()`, and `isWhiteSpace` in Java's `ValueParsers` names the
categories outright. `String.trim()` and `str.strip()` are both wrong, in
opposite directions.

**Rule** Mirror the set, not the method name. Python and Java each pin it with
a test (`is_all_whitespace_matches_is_null_or_white_space`,
`isNullOrWhiteSpaceMatchesTheReference`).

C++17 reaches it a third way, because a `std::string` is bytes and the standard
library has no Unicode tables: `Strings::trim` and `Strings::isAllWhiteSpace`
decode the UTF-8 and carry the set as a list. It folded the ASCII six until
writing this file turned the gap up; see `traps-cpp17.md` for what that cost
and why nothing caught it.

### Tokens are compared with `StringComparison.Ordinal`

`Cli/ArgumentDefinition.cs`, `Execution/CanvasConversion.cs`. Never the default
overload, never the current culture.

**Rule** Ordinal, byte for byte. In a language whose default comparison is
already ordinal this costs nothing; in one where it is not, it is the
difference between `--to` resolving the same way everywhere and resolving the
same way on the machines anyone happened to test.

### An unpaired surrogate is written as U+FFFD

The console's UTF-8 encoder substitutes the replacement character. A
`PrintStream` writes `?` instead, which is why the Java port builds its own
encoder (`traps-java.md`).

---

## Numbers and patterns

### `int.TryParse(text, NumberStyles.None, InvariantCulture, out value)`

`ValueParsers.cs`, five call sites, and this exact overload every time. It is
the numeric contract for `--pages`, page indices, counts, limits and `--form`.

Measured in `traps-cpp17.md`: it rejects a sign, surrounding whitespace, group
separators and a decimal point. `std::stoi` accepts most of them and throws
rather than returning false; Python's `int()` accepts an underscore, a sign and
digits from other scripts.

**Rule** The permissive local parser is never the right one here. Each port
carries tests for the shapes it would wrongly accept
(`a_trailing_newline_is_a_bad_value_for_every_strict_parser`,
`digitsFromOtherScriptsAreABadValue`,
`only_plain_ascii_digits_are_a_page_number`).

### Patterns are anchored by hand, with `\z`

`Planning/NameMatcher.cs`: `new Regex($"^(?:{raw})\\z")`.

Two reasons, and a port needs both. `Regex.IsMatch` *searches*, so an
unanchored pattern would match a substring --- hence the explicit `^` and the
wrapping group, which also stops a top-level `|` in a user pattern from
escaping the anchors. And `$` is not `\z`: `$` matches before a trailing
newline, so `5\n` would satisfy a pattern anchored with `$` and must not.

**Rule** Reproduce `\z` semantics: end of input, no trailing-newline
allowance. A regex engine whose "match whole string" mode means `$` is the
wrong tool. See `traps-python.md` and `traps-cpp17.md`, which hit this from
opposite directions.

---

## Exceptions and control flow

### An exception filter decides before the stack unwinds

`Commands/ConvertCommand.cs`, `Commands/ExtractCommand.cs`:
`catch (Exception error) when (!(error is CliError))` and
`catch (EngineException error) when (error.Error == EngineError.Cancelled)`.

A filter that returns false means *this handler never ran*: the exception keeps
going with its stack intact, and nothing in the handler's body happened.

**Rule** A language without filters must catch, test, and rethrow --- and the
rethrow must not run any of the handler's side effects first, and must not
replace the original exception or its stack. Getting this subtly wrong turns a
`CliError`, which carries its own exit code, into a per-document failure with a
different one.

### `Exception.Message` is never null

The framework supplies a default, so the reference interpolates the message
directly into `path: code: message`. Java's `getMessage()` is null for most
exceptions thrown without one, and `"x: " + null` prints `x: null`
(`traps-java.md`).

### `using` on a null reference disposes nothing

It is a no-op, not an error (`traps-python.md`).

### `HashSet<T>.Add` reports whether the item was new

`Planning/OutputPlanner.cs`, where the planner asks
`!seen.Add(CollisionKeyFor(key, fileSystem))`. The return value *is* the
collision detection. A port whose set-add returns
nothing has to ask separately, and has to ask before adding.

---

## Console and streams

### `WriteLine` appends the host's terminator and leaves the text alone

The most expensive entry in this file. `Console.WriteLine` adds
`Environment.NewLine` and does not touch what it was given, so a `\n` *inside*
engine text stays a lone LF even on Windows. An engine string really does carry
them: an option description in the options catalogue is several lines of
Markdown table.

C++17's text-mode `std::cout` translated both, which produced 41 bytes of drift
across `docfilters options` alone and a CRLF in every line of every JSON and
`--dry-run` plan. Four suites were green while it did so --- the conformance
fixtures collapse whitespace, the unit suites drive a recording console, and
the end-to-end checks assert properties. Only comparing the implementations
against each other found it.

**Rule** Turn the platform's newline translation off and write the terminator
explicitly. Both streams: `Console.Error.WriteLine` follows the same rule.
`cpp17/src/Runtime/SystemConsole.cpp` and the Python and Java ports all do
exactly this.

### The standard output stream is bytes, and a wrapper over it closes safely

`Commands/ConvertCommand.cs`: a `StreamWriter` with `AutoFlush = true` is built
over it and goes out of scope. Disposing that writer leaves stdout usable for
everything afterwards, and nothing in the byte path is ever translated.

**Rule** Where closing a wrapper would close the real descriptor, wrap it in
something whose close only flushes (`traps-java.md`).

### `new StreamWriter(stream)` is UTF-8 with no byte order mark

The default. A port that writes a BOM, or writes the platform's code page,
differs in the first three bytes of every text file.

### Redirection is asked per stream

`Console.IsOutputRedirected` and `Console.IsErrorRedirected` are separate
questions, and `--color` is decided per stream from them. A language that can
only ask about the pair together cannot reproduce `2>log` leaving stdout a
terminal; Java says so in its own list rather than pretending otherwise.

### A line ends on CR, LF or CRLF

`StreamReader.ReadLine` and `File.ReadAllLines`, which is how a `--list` file is
read. Not `\n` alone, and not a split that also breaks on the other separators
Unicode defines --- Python's `str.splitlines()` breaks on several more
(`traps-python.md`).

---

## Filesystem

### `Path.ChangeExtension` replaces after the last dot

`Execution/CanvasConversion.cs`. `ChangeExtension("page.unknown", "png")` gives
`page.png`; a filename with no dot gains one; a null extension removes it.
`os.path.splitext` is not the same function (`traps-python.md`).

### `File.Move(from, to, overwrite: true)` replaces on every platform

`Runtime/SystemFileSystem.cs`. The .NET overload is platform-uniform.
`os.rename` refuses an existing destination on Windows;
`std::filesystem::rename` does replace, which `traps-cpp17.md` records as a
verified non-trap.

### `FileAttributes.ReparsePoint` covers junctions as well as symlinks

`Runtime/SystemFileSystem.cs:69`. This is the link test the whole
write-through-a-link guard rests on, and it is wider than it looks: a Windows
junction sets the attribute, while `os.path.islink` returns False for one
(`traps-python.md`). A port that asks only "is this a symlink?" has a hole on
Windows.

### `EnumerateFileSystemEntries` is lazy

`Runtime/SystemFileSystem.cs`. The case probe stops at the first match rather
than reading the whole directory, which matters on a directory with many
entries.

---

## What looks like contract and is not

Recorded here because each has been mistaken for drift at least once.

- **`--help` layout.** Excluded by `tests/conformance/README.md` from when each
  implementation's parsing library rendered usage its own way. None of the four
  has a parsing library now, so the layout is this tool's own and the same
  everywhere, but it is still not contract: help text is the one surface a
  reader is expected to improve, and pinning it byte for byte would make a
  reworded line a four-implementation change. `help --json` *is* compared,
  because it renders from the shared argument table instead.
- **`--time`, `--progress`, `--logmem`, `-x`, `--debug-measure-io`.** Figures
  that differ every run, or that measure the implementation rather than the
  contract. `Recorder.NOT_COMPARED` in `tests/e2e/end2end.py` lists them with
  the reasoning.
- **The peak-memory number.** `Process.PeakWorkingSet64` includes the engine's
  native allocations. Java has no equivalent and uses a documented lower bound.
- **A docpack's bytes.** `df-core.xml` stamps `<created>` to the millisecond, so
  two packs of one document differ even from the same binary.
- **The tool's own version string.** No longer a trap, and worth saying why: it
  used to be the four-component assembly version, which no other language could
  reproduce. `VERSION.txt` is now the single source for all four ports, and
  `docfilters version`'s first line *is* compared.

---

## Verified non-traps

Things about the reference that look like the list above and are not.

- **`.Trim()` with no argument is not culture-sensitive.** A port does not need
  an invariant variant of it. It needs the same *set* of characters, which is
  the entry above.
- **Enum `ToString()` is not localisable.** .NET returns the identifier
  whatever the culture, so the lowercased name is stable across machines.
- **`string.Split(char)` keeps empty entries.** That is the default, and the
  reference relies on it where it splits argument templates on `|` and list
  values on `;` and `,`. Where it wants the other behaviour it says so, with
  `StringSplitOptions.RemoveEmptyEntries` in `Execution/EngineLog.cs`.

---

## When the reference is the one that is wrong

A faithful port is also a review, and three of them have now read every line of
`csharp/`. What they found does not go here: this file is for what the
reference means, not for where it was mistaken. Fix it in `csharp/` as its own
change, and let the fix travel to the ports.
