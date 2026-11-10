# Java traps, for porting from the C# reference

Constructs that run, pass a casual test, and do something other than what the
C# they were translated from does. Everything here was hit, measured or
deliberately avoided while building `java/`; nothing is folklore. Each entry
names the file whose comment records it, under
`java/src/com/hyland/documentfilters/cli/` unless the path says otherwise.

**How to use it.** Before porting a C# delta to Java, read the category that
covers what the delta touches. After writing the port, grep your diff for the
constructs named in bold. Most of these leave the suite green: they differ
from the reference only on inputs the tests do not happen to use, or only on
one platform.

The generating rule behind most of the list: **the JDK's defaults belong to
the host, and the binding is thinner than the .NET one.** `toLowerCase()`,
`String.format` and a `PrintStream` follow the default locale and charset, so
the port names `Locale.ROOT` and UTF-8 itself; and the Java binding leaves out
a handful of calls the reference makes, so the port either composes them or
says plainly that it cannot.

---

## Errors and control flow

### A null message is not an empty one

**C#** `Exception.Message` is never null; the framework fills in a default.
**Java** `getMessage()` is null for most exceptions thrown without one, and
`"x: " + null` prints `x: null`.

**Rule** Report the class's simple name when the message is null, and never
concatenate an engine string that may be null without a helper. See
`messageOf` in `commands/CommandExecution.java`, and the `text()` helpers in
`commands/IdentifyCommand.java`, `commands/ListCommand.java`,
`execution/StructureReport.java`.

### Nothing may be thrown back into native code

**C#** an exception in a callback the engine invokes travels out through
`Open` to the caller. **Java** the binding's JNI wrapper catches it and turns
it into a generic failure, so the cause --- a password file that cannot be
read, an interrupt --- is lost.

**Rule** Park the failure, tell the engine to stop, and rethrow the parked
original once `Open` has returned, even when the open went on to succeed. See
`rethrowParked` in `engine/DocumentFiltersEngine.java`.

### The binding's messages carry a suffix

**C#** an engine failure's message is the engine's text. **Java** the JNI glue
formats it as `"<text>, error code <N>"`, so every `path: code: message` line
would end differently from the reference's.

**Rule** Strip a trailing `", error code " + getErrorCode()`, and only that
exact suffix. See `messageOf` in `engine/DocumentFiltersEngine.java`.

### Loading the binding is a class initialiser

**C#** a native library that cannot be found fails the first P/Invoke, which
the engine's `Initialize` catches. **Java** the binding's static initialiser
calls `System.loadLibrary`, and a failure there is a `LinkageError` from
whatever line first touches the class --- not an `Exception`, so `catch
(Exception)` does not see it, and it would escape as a stack trace.

**Rule** Create the binding's objects lazily inside `initialize`, and catch
`LinkageError` there into the same `E_INIT_FAILED` as any start-up failure.
See `engine/DocumentFiltersEngine.java`.

---

## Text and encoding

### `toLowerCase()` and `String.format` follow the default locale

**C#** the reference compares tokens ordinally and formats with the invariant
culture. **Java** `"TIFF".toLowerCase()` is `"tıff"` under a Turkish locale,
and `String.format("%.3f", x)` writes a comma under a German one.

**Rule** Fold enumerated tokens with the ASCII-only `ValueParsers.foldToken`,
and pass `Locale.ROOT` to every `String.format` that writes a number. See
`configuration/ValueParsers.java`, `execution/Diagnostics.java`.

### The test harness has no predicate form of `DoesNotContain`

**C#** `Assert.DoesNotContain(json, c => char.IsSurrogate(c))` checks that an
escaped document holds no surrogate.
**Java** `tests/Assert` offers only the substring form of
`assertDoesNotContain`, so `anEscapedDocumentCarriesNoSurrogatesOfItsOwn`
loops over the chars and calls `assertTrue(!Character.isSurrogate(...))`. Same
property, different route.

**Rule** Do not add a predicate overload to the harness for one test; loop. The
production side needs no deviation: a `String` is UTF-16 units as in C#, so
`JsonWriter.escape` mirrors the reference with `Character.isSurrogate`.

### The test harness has no `[Theory]`

**C#** `A_dash_token_that_is_not_an_option_spelling_is_a_path` is a `[Theory]`
with five `[InlineData]` rows. **Java** the JDK-only runner has no
parameterised form, so `aDashTokenThatIsNotAnOptionSpellingIsAPath` is one
test that loops over the same five tokens. Same property, different route; a
failure names the test, not the row.

**Rule** Keep the rows in the loop in step with the reference's, and do not add
a data-row mechanism to the runner for one test.

### Enum names are not the reference's names

**C#** `PageElementType.TextBox.ToString().ToLowerInvariant()` is `textbox`.
**Java** the binding's constants are `TEXT_BOX`, so the same expression gives
`text_box`.

**Rule** Drop the underscores before folding. See `kindOf` in
`execution/StructureReport.java`.

### A `PrintStream` writes '?' where `Encoding.UTF8` writes U+FFFD

**C#** the console's UTF-8 encoder replaces an unpaired surrogate with U+FFFD.
**Java** `System.out` uses the platform charset (Cp1252 on a Western Windows
host, whatever `file.encoding` says) and replaces what it cannot carry with
`?`, so a report's bytes would depend on the host.

**Rule** Wrap the raw descriptors in a UTF-8 encoder set to replace with
U+FFFD, and write the host's line end explicitly. The document writers are the
other way round: strict, so a character UTF-8 cannot carry fails the document
rather than altering it. See `runtime/SystemConsole.java`,
`commands/ConvertCommand.java`.

### Closing a wrapper closes stdout

**C#** `Console.OpenStandardOutput()` hands out a stream whose disposal leaves
the process's stdout alone. **Java** closing any writer around `System.out`
closes `FileDescriptor.out` for the rest of the run.

**Rule** The binary stdout the commands receive only flushes on close. See
`runtime/SystemConsole.java`.

---

## Numbers and dates

### The binding's dates are `java.util.Date`

**C#** a subfile's timestamp is a `DateTime`, and "none" is
`DateTime.MinValue`. **Java** it is a `Date` built from the engine's
broken-down local time, and "none" comes back in the year 1900 or before.

**Rule** Convert through the default zone to a `LocalDateTime`, which recovers
the engine's fields, and treat a year of 1900 or earlier as no timestamp. See
`engine/DocumentFiltersEngine.java`.

### A bookmark's page is in its destination

**C#** the reference reads a bookmark's page index from the binding. **Java**
the binding has no such property; the index is in the destination string.

**Rule** Parse it from the destination, and treat a destination without one
as no page. See `engine/DocumentFiltersEngine.java`.

---

## The binding's gaps

These are calls the reference makes that the Java binding does not expose.
Each is either composed from what it does expose, or reported.

- **No `getFileCapabilities`.** Composed from the three booleans the binding
  does give; those are the only bits anything above the engine seam reads. See
  `engine/DocumentFiltersEngine.java`.
- **No way to set the temporary directory.** `--temp-dir` is accepted and
  ignored, because it moves the engine's scratch files and never changes an
  output. See `engine/DocumentFiltersEngine.java`.
- **A docpack writer newer than its native library.** The jar in
  `bindings/java/lib` declares `MakeDocPack`, but the 26.3 release's
  `ISYS11dfjava` has no entry point for it, and the JVM only finds that out on
  the first call, as an `UnsatisfiedLinkError`. Against that release
  `--to docpack` fails each document with `E_DOCUMENT_FAILED: this binding's
  native library cannot write a docpack`, and the two end-to-end docpack
  checks fail; against a native library that has the entry point, both pass
  and the pack matches the reference's in everything but its `<created>`
  stamp. The 26.4.0 native library has the entry point, so the gap described
  here is the 26.3 release's alone. See `docPack` in
  `engine/DocumentFiltersEngine.java`.
- **No native peak memory.** The reference reads `PeakWorkingSet64`. On Linux
  `VmHWM` is the same measure; elsewhere the fallback is the JVM's own pools,
  a lower bound, so `--max-memory` fires late, never spuriously. See
  `execution/PeakMemory.java`.

---

## Modules and the native boundary

### `System.loadLibrary` searches `java.library.path` only

**C#** the reference can point the binding at a directory at run time, which
is what `--library` and `DF_PATH` do. **Java** the binding's static
initialiser loads both `ISYS11df` and `ISYS11dfjava` by name, and the JVM
searches only `java.library.path`, fixed at start-up. Neither option can
choose the library.

**Rule** The launcher sets the property: `DOCFILTERS_OPTS` for the
distribution's start script, a system property for `gradle run`. The start
script strips backslashes on Windows, so the path is written with forward
slashes. See `java/build.gradle.kts` and the Java launcher in
`tests/e2e/end2end.py`.

### `--debug-measure-io` alone still buffers the file

**C#** the reference counts I/O on a seekable file stream. **Java**
`IFileSystem.openRead` returns a stream that cannot seek, and the engine needs
to.

**Rule** Buffer the document in memory and count what the engine asks for;
the figures are the engine's requests, not the disk's, so they are unchanged.
See `execution/DebugInput.java`.

---

## Console and process

The JDK sees less of the terminal than .NET does. Each of these is a
documented limit of the port rather than a translation bug.

- **Only stdin and stdout together can be asked about.** `System.console()`
  is non-null only when both are a terminal, so stderr is taken to be one
  exactly when they are; with only stderr redirected, the error file still
  gets colour codes. From Java 22 a console exists even when redirected, and
  `Console.isTerminal()` is asked first, reflectively. See
  `runtime/SystemConsole.java`.
- **`readPassword` has no cancel key and no echo.** Escape is an ordinary
  character; only end of input cancels. The reference's '*' per character
  cannot be drawn. See `runtime/SystemConsole.java`.
- **`--pause` waits for Enter.** A terminal is read a line at a time. See
  `runtime/SystemConsole.java`.
- **Ctrl+C needs `sun.misc.Signal`.** A shutdown hook runs only after the JVM
  has decided to exit, so it cannot turn Ctrl+C into a request. The signal
  class is reached reflectively; where it is missing, Ctrl+C ends the process.
  See `runtime/Interruption.java`.
- **`--view` uses `Desktop.open` on Windows.** It is `ShellExecute`, so the
  file association applies and there is no command line to quote. See
  `runtime/Viewer.java`.

---

## Verified non-traps

Things that look like the list above and are not defects.

- **A `(int)` cast truncates toward zero.** As the reference's does, so
  element bounds print the same. See `execution/StructureReport.java`.
- **Non-ASCII arguments survive on Windows.** Measured on JDK 21 with a Cp1252
  ANSI code page, `日本語.pdf` and `Ωmega.pdf` --- neither representable in
  it --- reach the engine intact.
