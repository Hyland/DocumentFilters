# docfilters — C++17

CMake project for the C++17 build of the `docfilters` command line tool.

## Layout

```
cpp17/
├── CMakeLists.txt   builds docfilters_lib and the docfilters executable
└── src/
    ├── main.cpp     argv and the process exit code, and nothing else
    ├── Program.*    the pre-scan and the top-level error report
    ├── Cli/         the argument table, and the binding of argv against it
    ├── Configuration/  the options model, value parsers, cross-argument validation
    ├── Planning/    format catalogue, output planner, name templates, page ranges
    ├── Discovery/   the filesystem walk
    ├── Engine/      the Document Filters seam
    ├── Execution/   the Document Filters code, written against that seam
    ├── Commands/    one thin orchestrator per verb
    └── Runtime/     console, filesystem, JSON writer, batch loop, exit codes
```

The folders are the same pipeline stages as `csharp/`, in the same order, so a
file has the same neighbours in both. `Runtime/Strings.*` has no C# counterpart:
it holds the string helpers the standard library supplies there.

## Naming

Type and namespace names are PascalCase (`Fixture`, `Strings`). Function and
data member names are lowerCamelCase (`trim()`, `foldAscii()`, `subject`).
The name itself travels from the reference and only its casing changes, so a
member can be found in all four implementations by searching for one spelling
--- see "Mirror the reference" in [`../CLAUDE.md`](../CLAUDE.md).

## Building

**`cmake` and `ctest` must be on `PATH`.** Nothing here hardcodes an install
path, and the tasks in `cpp17.code-workspace` invoke both by name, so an editor
launched without them on `PATH` fails every task before it reaches CMake. Visual
Studio ships them but does not put them on `PATH`; they are under
`Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin` in the installation.
Either launch the editor from a Developer Command Prompt, or add that directory
to your user `PATH` once:

```powershell
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
$bin = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$user = [Environment]::GetEnvironmentVariable('Path', 'User')
if ($user -notlike "*$bin*") {
    [Environment]::SetEnvironmentVariable('Path', $user.TrimEnd(';') + ';' + $bin, 'User')
}
```

Restart the editor afterwards so it inherits the change.

Links against `bindings/cpp17`, and nothing else. There is no `FetchContent`
here: this port used to fetch CLI11 v2.4.2 and hand it the option tokens, and
now binds argv itself in `src/Cli/CommandLineApp.cpp`, as all four
implementations do.

```bash
cmake -S cpp17 -B cpp17/build
cmake --build cpp17/build
```

## Testing

`tests/cpp17/` is a separate CMake project that links `docfilters_lib` and
adds Catch2 v3.7.1:

```bash
cmake -S tests/cpp17 -B tests/cpp17/build
cmake --build tests/cpp17/build
ctest --test-dir tests/cpp17/build
```
