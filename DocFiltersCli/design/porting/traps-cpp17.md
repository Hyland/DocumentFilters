# C++17 traps, for porting from the C# reference

Constructs that compile, pass a casual test, and do something other than what
the C# they were translated from does. Everything here was hit, measured or
deliberately avoided while building `cpp17/` (DF-3104); nothing is folklore.

**How to use it.** Before porting a C# delta to C++17, read the category that
covers what the delta touches. After writing the port, grep your diff for the
constructs named in bold. Most of these produce a green test suite: the ones
that cost the most time here were all found by something other than the tests.

The generating rule behind half the list: **C# is a managed language with one
string type and one path type, and C++17 is not.** A reference keeps nothing
alive, a `std::string` says nothing about its encoding, and
`std::filesystem::path` says something different about it on each platform.

---

## Lifetime and ownership

### A reference member keeps nothing alive

**C#** a field holding a reference keeps the object alive for as long as the
field does. **C++17** a reference member binds to whatever was passed and dies
with it.

`SystemConsole` held `const Configuration::ConsoleOptions&`. `main()` builds
those from `preScanConsoleOptions(args)`, which returns by value, so the member
bound to a temporary that died at the end of that declaration. Every later read
was a use-after-free: on Windows the freed memory still held the old bytes and
the tool worked, on Linux it did not and the tool went silent. The whole suite
was green throughout, because the suite constructs its console from a named
object.

**Rule** Members by value unless the owner provably outlives the member, and
assume the C# you are translating says nothing about lifetime because it does
not have to. See the comment on `SystemConsole::_options`.

### A C# `Dictionary` keyed by an object is keyed by its address

**C#** `Dictionary<IArgumentDescriptor, List<string>>` with a key type that
overrides neither `Equals` nor `GetHashCode` compares keys by reference, and
the reference reads naturally as "keyed by the argument". **C++17** there is no
equivalent default: `std::unordered_map` needs a hashable key, and an
`IArgumentDescriptor` has no hash.

`partitionTokens` uses `std::unordered_map<const IArgumentDescriptor*,
std::vector<std::string>>`. The pointer is the C# reference spelled for C++,
and it is correct only because every descriptor outlives the map --- they are
the `ArgumentDefinition` objects `bindVerb`'s caller holds, and the map dies
first. Both the key and the lookup are an implicit upcast from
`ArgumentDefinition<T>*`, which may adjust the address but adjusts it the same
way at both sites, so the two agree. Keying by `longName()` instead would read better and be wrong in a way
nothing would catch: two definitions may be spelled identically in different
tables, and the whole point of the map is that `apply` finds *its own*
definition's values.

**Rule** When the reference keys a container by an object, key by address, and
say in a comment why the addresses are stable. See `partitionTokens` in
`src/Cli/CommandLineApp.cpp`. The Java port spells the same thing as an
`IdentityHashMap`; Python gets it for free, since a class defining no `__eq__`
hashes by identity.

### Destruction happens at the end of the scope, not after the last use

**C#** `using var x = ...` disposes at the end of the enclosing block, same as
C++. The trap is identical in both languages, which is why both ports had it.

`--combine --view` launched the viewer while the combined canvas and writer were
still alive, so a PDF went out without its trailer. A `.reset()` or an explicit
`Dispose()` fixes the behaviour but not the next edit; a block that ends before
the dependent statement does.

**Rule** When the next statement needs the file to be finished, put the owner in
a scope that ends first. Do not rely on a comment saying it is closed.

---

## Text and encoding

### `std::tolower` and friends are locale-sensitive

**C#** `char.ToLowerInvariant` is fixed by definition. **C++17** `std::tolower`
consults the global locale, so under a Turkish locale `'I'` folds to a dotless
`i` and `--to TIFF` stops resolving.

**Rule** Fold ASCII by hand: `Strings::foldAscii` is the whole of it, and
`tests/conformance/README.md`'s "values are folded, or they are verbatim" rule
exists because of this. The same reasoning applies to `CaseProbe` and to
`OutputPlanner`'s collision key.

### `string.Trim()` is Unicode whitespace, and a `std::string` has no idea

**C#** `Trim()` with no argument removes `char.IsWhiteSpace`: U+0009 to U+000D,
U+0085, and the space, line and paragraph separator categories --- so the
no-break spaces U+00A0, U+2007 and U+202F are included, and the information
separators U+001C to U+001F are not. **C++17** a `std::string` is bytes, and
the obvious loop over `isspace` sees only the six ASCII ones.

**Rule** Decode before trimming, and carry the table: the set is enumerable and
about twenty-five characters long. Do not reach for `isspace` on a byte.
`Strings::trim` and `Strings::isAllWhiteSpace` are the whole of it, and nothing
else in the port asks the question.

It was wrong here until it was not. `Strings::trim` folded the ASCII six, and
`InputEnumerator`'s `isBlank` looped `std::isspace` over bytes where the
reference calls `string.IsNullOrWhiteSpace`. The reachable consequences, which
are the reason this is worth decoding for:

- `MetadataReport` trims a record whose text came from the *document*, so no
  unusual command line is involved. A metadata value padded with U+00A0, or
  with the U+3000 ideographic space that is ordinary in CJK documents, printed
  with the padding here and without it in the other three ports.
- A `--list` file line holding nothing but a no-break space is a blank line to
  the other three ports and was a path to open to this one --- a different exit
  code, not only different bytes.
- `ParseIdList` (`--include-types "PDF<U+00A0>;DOCX"`) and `EngineLog`'s module
  list, both of which need the user to type one.

**No suite saw it**, which is the part worth remembering. The unit test
asserted the divergent behaviour and was named for it; the conformance fixtures
stop at the plan; the end-to-end checks assert properties; and the parity
comparison would have caught it only against a document whose metadata carries
such a character, which `tests/e2e/test.pdf` does not. It surfaced from reading
the reference to write `traps-csharp.md`, and from noticing that Python and
Java had both built a predicate where this port had reached for `<cctype>`.

### `char` is signed, and `<cctype>` takes an `int`

**C++17** `std::isspace(c)` with a negative `char` is undefined behaviour, not a
`false`. The port casts at every call site: see
`InputEnumerator`'s `std::isspace(static_cast<unsigned char>(c))`.

**Rule** `static_cast<unsigned char>` on the way in, every time, or do not use
`<cctype>` at all.

### An unpaired surrogate is three bytes here, and a pair is never two

**C#** `JsonWriter.Escape` walks UTF-16 units and asks `IsPaired` before
escaping a surrogate. **C++17** a `std::string` is UTF-8, where a valid pair is
one four-byte character and there is nothing to pair. A lone surrogate can only
be the three-byte form `0xED 0xA0..0xBF 0x80..0xBF`, which a lenient UTF-16 to
UTF-8 conversion of a Windows filename produces.

**Rule** No `IsPaired`: `decodeSurrogate` recognises the three bytes and
`Escape` writes `\uXXXX`, lowercase hex, consuming all three. Test inputs are
those byte sequences, written as separate string literals so `\x` does not
swallow the next hex digit. A CESU-8 pair (two such sequences) is escaped as
two, where C# would write the character; nothing in this port produces one.

### A narrow `main` on Windows has already lost the command line

**C#** `string[] args` is UTF-16, and so is every path from there down.
**C++17** the CRT converts the UTF-16 command line through the active ANSI code
page before `main` sees it, so a filename the code page cannot spell arrives as
`?` characters. Measured: `docfilters identify <japanese>.txt` reported
`E_NO_INPUTS: ...\???.txt` while the C# tool opened the same file.

**Rule** `wmain` on Windows, convert each argument to UTF-8 once, and keep UTF-8
everywhere after that --- which is also what the Document Filters C++ binding
expects (`DF::u8_to_w` is the first thing it does with a narrow filename).

### `std::filesystem::path` converts narrow strings through the ANSI code page

**C++17** `fs::path(utf8)` and `path.string()` are not round trips on Windows:
both go through the active code page. A name that survived `wmain` is destroyed
by the first `fs::path` that touches it.

**Rule** One named seam, used at every crossing: `FilePath::of` /
`FilePath::toUtf8` in `Runtime/IFileSystem.h` wrap `u8path`/`u8string`, which
are the two calls that state an encoding. On POSIX both are the identity, which
is why this defect is invisible on Linux.

### The `*A` Win32 family re-encodes silently

`GetEnvironmentVariableA`, `GetModuleFileNameA`, `ShellExecuteA`,
`GetFileVersionInfoA` all take the active code page. The C# reference gives no
hint that this exists, because `Environment.GetEnvironmentVariable`,
`AppContext.BaseDirectory` and `Process.Start` are wide underneath.

**Rule** `*W` plus the `FilePath` seam. While you are there: `MAX_PATH` is not a
limit on Windows, so `GetModuleFileNameW` needs a growable buffer --- it
truncates silently otherwise.

### A non-ASCII character in a source file is a build-configuration bet

MSVC reads a BOM-less source file in the active code page unless told otherwise,
so a literal `é` or `…` in a string or a comment is not portable. The contract
text is worse than source text: `--to`'s help string carried a U+2026 ellipsis,
which made `arguments.txt` --- compared byte for byte by four suites --- depend
on the encoding each one happened to read.

**Rule** ASCII in sources, `\xNN`/`\uNNNN` escapes in tests that need a specific
byte, `...` in anything the contract compares.

---

## Streams and the console

### Text-mode stdout translates line breaks into binary output

**C#** `Console.OpenStandardOutput()` is a byte stream. **C++17** on Windows
`std::cout` is in text mode, so every `0x0A` inside a PDF or a PNG written to
`--stdout` becomes `0x0D 0x0A`.

**Rule** `_setmode(_fileno(stdout), _O_BINARY)` before writing document bytes;
`SystemConsole::openStandardOutput` is the one place that does it.

### `std::getline` ends a line only on LF

**C#** `StreamReader.ReadLine` and `File.ReadAllLines` end a line on CR, LF or
CRLF. **C++17** `getline` with the default delimiter leaves the CR on the end of
every line of a CRLF file, and reads a CR-only file as one line.

**Rule** Split by the same rule as the reference: `Strings::splitLines` is the
one implementation, used by `--files-from -` and the fixture reader alike.

### An imbued locale re-encodes what you write

Giving an iostream a locale with a non-trivial `codecvt` changes the bytes that
reach the file, after the code that produced them has decided what they are.

**Rule** The port never calls `imbue`, and should not start. Output encoding is
settled by the string, not by the stream.

---

## Numbers and patterns

### `std::stoi` throws where `int.TryParse` returns false

**C#** `int.TryParse(text, NumberStyles.None, ...)` rejects a sign, surrounding
whitespace and trailing text, and returns `false`. **C++17** `std::stoi` skips
leading whitespace, accepts a sign, ignores trailing text, and throws on
failure --- three different behaviours, one of which terminates the process if
nothing catches it.

**Rule** `std::from_chars` after a hand-written shape check, which is what
`ValueParsers::tryParseDigitsOnly`, `PageRange` and `OutputNameTemplate` each
do. `from_chars` also reports overflow as `std::errc::result_out_of_range`
rather than by exception.

### `std::regex_match` is not `Regex.IsMatch`, and `$` is not `\z`

**C#** `IsMatch` searches; the reference therefore anchors its patterns by hand
as `^(?:...)\z`. **C++17** `std::regex_match` requires the whole subject to
match, and the ECMAScript dialect has no `\z` at all --- it parses as an
identity escape for the letter `z`. `$` also differs: .NET's matches before a
trailing newline, ECMAScript's only at the end.

**Rule** Anchor structurally, not textually: `NameMatcher` builds the bare body
and calls `std::regex_match`. Translating `^(?:...)\z` literally would silently
match a `z`.

### A malformed pattern throws a different exception

`std::regex_error` where .NET throws `ArgumentException`. Both must come out as
the same `E_BAD_VALUE`, or the two ports disagree on a corpus case.

---

## Filesystem

### `std::filesystem::path` knows only the host's separators, and normalises

**C++17** a backslash is an ordinary character in a filename on Linux, so
`fs::path("..\\..\\windows\\system32\\config").stem()` returns the whole string
there. Output names are plain strings in this tool's contract --- the same
`--out-name` must render identically on both platforms.

**Rule** Normalise separators yourself before any lexical operation
(`OutputNameTemplate::normalizeSeparators`), and treat path text as text.

### The `error_code` overloads fail quietly

**C++17** `fs::directory_iterator(path, ec)` that failed compares equal to
`end()`, so ignoring `ec` turns an unreadable directory into an empty one; a
subtree the walk cannot read is then skipped in silence, and a run given only
that directory reports `E_NO_INPUTS`. `fs::exists(p, ec)` returns `false` on
error, so "not there" and "could not tell" are the same value.

**Rule** Check `ec` at every call, and decide explicitly which of the two
answers you mean. The reference throws in both cases, which is what callers are
written for.

### The host is not the volume

Case sensitivity belongs to the directory, not to the operating system: a
mounted NTFS volume under Linux folds case, and Windows can mark one directory
case-sensitive. `std::filesystem` will not tell you.

**Rule** Ask the directory, read-only (`CaseProbe`), and fall back to the host
default only when it cannot answer.

---

## Processes and signals

### `std::system` runs a shell

**C#** `Process.Start` with `ArgumentList` passes each argument through as one
argv slot. **C++17** `std::system("xdg-open " + path)` hands the whole string to
`/bin/sh`, so an output named `doc$(touch pwned).txt` executes. Measured: the
file was created.

**Rule** `posix_spawnp` with an argv array (`Runtime::Viewer`), or
`ShellExecuteW` on Windows, which takes the path as a verb target rather than as
a command line. The C# analogue of the same mistake is building an argument
*string*, which splits on a quote in the filename.

### A signal handler may do almost nothing

Only `volatile std::sig_atomic_t` assignment and lock-free atomic operations are
permitted --- no allocation, no locks, no iostreams.

**Rule** Record a flag and return; everything else happens on the polling side
(`Runtime::Interruption`). See the non-traps below for what *is* allowed.

---

## Declaration order

### A definition must precede its use; the reference's order does not

**C#** members may appear in any order; the compiler reads the whole type
first. **C++17** a name must be declared before it is used, so a file written
in the reference's order will not compile: `bindVerb` is the fourth member and
calls `validateTokens`, which is the seventh.

Written the obvious way, the file comes out callee-first --- `looksLikeOption`
at the top, `bindVerb` near the bottom, `CommandLineApp::run` last of all ---
which is close to the reverse of the reference. That is how every `.cpp` in
this port reads, `src/Cli/CommandLineApp.cpp` included.

Forward-declaring the helpers at the top of the anonymous namespace, so that
the definitions can follow the reference's order with `run` and `bind` first,
was tried and reverted. Two reasons, and the second is the one that decided
it:

- It duplicates nine signatures in the file that is diffed across languages
  more than any other. They disagree loudly if they drift --- change one and
  the other becomes a second overload, so the call fails to resolve or the
  definition is reported unused --- but it is still a second place to edit.
- It fixes one file. `src/Configuration/OptionsValidator.cpp` has the same 25
  members as the reference and nothing like its order: the `check*` helpers
  fill the anonymous namespace and the three `validate*` entry points sit
  underneath them. Every other file with file-local helpers is the same.
  Buying exact order in `Cli/` leaves the port with two conventions and no
  way to tell which a given file follows.

**Rule** Define file-local helpers callee-first and let the order differ from
the reference. Match the reference on *names* and on *which* members exist,
and look them up rather than reading top to bottom. Do not add a
forward-declaration block to restore the order; if a future change makes
exact order worth having, it has to be worth having in every file at once.

Nothing checks any of this. `compare_inventory.py` compares *test* names
across the four suites; no ring looks at source member names at all, so a
helper quietly renamed or dropped in this port is invisible to the build.
That is the gap to close if source parity is ever worth enforcing --- and it
would be a names check, not an order one.

## Toolchain

### MSVC accepts what GCC rejects, and the reverse

MSVC binds a temporary to a non-const lvalue reference without complaint; GCC
and Clang refuse. GCC's warnings catch truncation and format problems MSVC says
nothing about. Neither compiler alone tells you the port builds.

**Rule** Build both before calling anything done. Several defects in this port
existed only on one platform, and the use-after-free above only *mattered* on
one.

### `abort()` does not exit 134 everywhere

Windows exits 3; POSIX reports SIGABRT as 134. Any end-to-end assertion on the
exit code of a crash has to say which.

### `std::error_code::message()` is platform text

The wording differs between the MSVC runtime and libstdc++, so it cannot appear
in anything the conformance fixtures compare. Put your own message in the error
and keep the system text out of the contract.

### A test binary that links the engine needs the engine on its path

`catch_discover_tests` runs each case in its own process at build time; without
`DL_PATHS "$<TARGET_FILE_DIR:docfilters>"` every one of them fails to start, for
reasons that look nothing like a missing DLL.

---

## Tests

### A `[Theory]` is one `TEST_CASE` with a loop

**C#** a `[Theory]` with `[InlineData]` rows is one named test that runs once
per row, and `compare_inventory.py` collapses the rows to that one name.
**C++17** the port writes one `TEST_CASE` under the same name and loops over the
rows inside it, rather than using `GENERATE`, so the loop and the C# row list read side by
side. A failing row is then reported by its `CHECK`, not by its own test run.

**Rule** Same name as the `[Theory]` method, rows in a braced list. See
`a dash token that is not an option spelling is a path`.

---

## Verified non-traps

Things that look like the list above, were measured on both toolchains, and are
not defects. Recorded because each one has already been raised twice.

- **`std::filesystem::rename` does replace an existing file on Windows.** The
  MSVC standard library calls `MoveFileExW` with `MOVEFILE_REPLACE_EXISTING`;
  measured, the rename succeeds with an empty `error_code` and the destination
  holds the source bytes. The belief that it cannot comes from the plain
  `MoveFile` API. (It would be true of a MinGW build, which is not a supported
  configuration here.)
- **`std::atomic<bool>` may be used from a signal handler here.**
  `std::atomic<bool>::is_always_lock_free` is `1` under both MSVC 14.51 and GCC
  for these targets, and C++ permits lock-free atomic operations in a handler.
  The deadlock this is usually warned about needs an implementation that does
  not exist for any platform this tool ships to.
