/****************************************************************************
 * Document Filters Example - a command line tool.
 *
 * Convert, extract, list, identify and inspect documents from the shell.
 *
 *   docfilters convert --to pdf --out-dir out/ ./corpus
 *   docfilters extract -i --out-dir dump/ bundle.pst
 *   docfilters identify --json *.bin
 *
 * The same command line is implemented in C#, Python, Java and C++17. See
 * ../README.md for how the pieces fit together and why they are arranged
 * this way; if you are here to learn the API, read the Execution folder and
 * ignore the rest.
 ****************************************************************************/

#include "Program.h"
#include "Runtime/Interruption.h"
#include "Runtime/SystemConsole.h"
#include "Runtime/SystemFileSystem.h"

#include <filesystem>
#include <string>
#include <vector>

using namespace Hyland::DocumentFilters::Cli;

/// <remarks>
/// wmain on Windows, because the narrow entry point never sees what was typed:
/// the CRT converts the UTF-16 command line through the active ANSI code page,
/// and a filename the code page cannot spell arrives as question marks. The run
/// then reports E_NO_INPUTS for a file that is plainly there --- measured with a
/// Japanese filename, which the C# reference opens without comment because .NET
/// strings are UTF-16 throughout.
///
/// Every argument is converted once, here, into the UTF-8 this tool and the
/// Document Filters binding both use everywhere else. std::filesystem::path is
/// the conversion rather than a Win32 call: it holds the wide text verbatim and
/// u8string() states the encoding coming out. Non-path arguments go through it
/// too --- a path is just a string it does not alter.
/// </remarks>
#if defined(_WIN32)
int wmain(int argc, wchar_t** argv)
{
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++)
		args.push_back(std::filesystem::path(argv[i]).u8string());
#else
int main(int argc, char** argv)
{
	std::vector<std::string> args(argv + 1, argv + argc);
#endif

	// Ctrl+C is recorded rather than allowed to kill the process, so the run
	// reports and exits 130 through the same path as every other exit code.
	Runtime::SystemConsole console(Program::preScanConsoleOptions(args));
	Runtime::SystemFileSystem fileSystem;

	return Program::run(args, console, fileSystem, nullptr, &Runtime::Interruption::install());
}
