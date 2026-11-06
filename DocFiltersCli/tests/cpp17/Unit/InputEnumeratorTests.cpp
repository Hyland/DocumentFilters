/*
   (c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// ---------------------------------------------------------------------------
//  The filesystem walk: which files a command line actually selects.
//
//  Runs entirely against InMemoryFileSystem, so these are ordinary fast unit
//  tests despite being about the filesystem. Directory depth here is
//  entirely separate from subfile depth, which is recursion into documents
//  nested inside other documents.
//
//  This file also carries the fake's own tests (the InMemoryFileSystemTests
//  section below), mirroring tests/csharp/Unit/InputEnumeratorTests.cs,
//  which keeps both C# test classes in one file too.
// ---------------------------------------------------------------------------

#include "Discovery/InputEnumerator.h"
#include "Fakes/InMemoryFileSystem.h"
#include "Fakes/RecordingConsole.h"
#include "Planning/NameMatcher.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <sstream>

using namespace Hyland::DocumentFilters::Cli::Configuration;
using namespace Hyland::DocumentFilters::Cli::Discovery;
using namespace Hyland::DocumentFilters::Cli::Planning;
using namespace Hyland::DocumentFilters::Cli::Runtime;
using namespace Hyland::DocumentFilters::Cli::Tests::Fakes;

namespace
{
	InMemoryFileSystem tree()
	{
		InMemoryFileSystem fs;
		fs.addFile("corpus/a.docx")
			.addFile("corpus/b.pdf")
			.addFile("corpus/reports/q1.docx")
			.addFile("corpus/reports/q2.docx")
			.addFile("corpus/reports/archive/old.docx")
			.addFile("corpus/notes.txt");
		return fs;
	}

	std::vector<std::string> walk(InMemoryFileSystem& fs, const InputOptions& options)
	{
		RecordingConsole console;
		InputEnumerator enumerator(fs, console);
		std::vector<InputSource> found = enumerator.enumerate(options);

		std::vector<std::string> paths;
		for (const InputSource& source : found)
			paths.push_back(source.path);
		return paths;
	}

	InputOptions from(std::initializer_list<std::string> paths)
	{
		InputOptions options;
		options.paths.assign(paths.begin(), paths.end());
		return options;
	}

	bool contains(const std::vector<std::string>& haystack, const std::string& needle)
	{
		return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
	}
}

// ------------------------------------------------------------------ depth

TEST_CASE("a directory is walked recursively by default", "[InputEnumerator]")
{
	// Matching fd and ripgrep rather than grep and cp: a directory handed to
	// a batch converter is a corpus to walk.
	InMemoryFileSystem fs = tree();
	std::vector<std::string> found = walk(fs, from({ "corpus" }));

	CHECK(contains(found, "corpus/a.docx"));
	CHECK(contains(found, "corpus/reports/q1.docx"));
	CHECK(contains(found, "corpus/reports/archive/old.docx"));
}

TEST_CASE("no-recurse stays in the named directory", "[InputEnumerator]")
{
	InputOptions options = from({ "corpus" });
	options.noRecurseWasGiven = true;

	InMemoryFileSystem fs = tree();
	std::vector<std::string> found = walk(fs, options);

	CHECK(contains(found, "corpus/a.docx"));
	CHECK_FALSE(contains(found, "corpus/reports/q1.docx"));
}

TEST_CASE("max-depth one is the same as no-recurse", "[InputEnumerator]")
{
	InputOptions options = from({ "corpus" });
	options.maxDepth = 1;

	InMemoryFileSystem fs = tree();
	CHECK_FALSE(contains(walk(fs, options), "corpus/reports/q1.docx"));
}

TEST_CASE("max-depth two reaches one level of subdirectory", "[InputEnumerator]")
{
	InputOptions options = from({ "corpus" });
	options.maxDepth = 2;

	InMemoryFileSystem fs = tree();
	std::vector<std::string> found = walk(fs, options);

	CHECK(contains(found, "corpus/reports/q1.docx"));
	CHECK_FALSE(contains(found, "corpus/reports/archive/old.docx"));
}

// ------------------------------------------------------------ explicit files

TEST_CASE("an explicit file is taken as given", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	CHECK(walk(fs, from({ "corpus/a.docx" })) == std::vector<std::string>{ "corpus/a.docx" });
}

TEST_CASE("files and directories can be mixed and keep their order", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	std::vector<std::string> found = walk(fs, from({ "corpus/notes.txt", "corpus/reports" }));

	REQUIRE_FALSE(found.empty());
	CHECK(found[0] == "corpus/notes.txt");
	CHECK(contains(found, "corpus/reports/q1.docx"));
}

TEST_CASE("an explicit file is not subject to the name matcher", "[InputEnumerator]")
{
	// Naming a file directly is an instruction, not a suggestion; a glob
	// filters what a walk discovers.
	InputOptions options = from({ "corpus/notes.txt" });
	options.matcher = NameMatcher::fromGlobs({ "**/*.docx" });

	InMemoryFileSystem fs = tree();
	CHECK(walk(fs, options) == std::vector<std::string>{ "corpus/notes.txt" });
}

// ---------------------------------------------------------------- selection

TEST_CASE("a glob filters what the walk discovers", "[InputEnumerator]")
{
	InputOptions options = from({ "corpus" });
	options.matcher = NameMatcher::fromGlobs({ "**/*.docx" });

	InMemoryFileSystem fs = tree();
	std::vector<std::string> found = walk(fs, options);

	CHECK(contains(found, "corpus/a.docx"));
	CHECK_FALSE(contains(found, "corpus/b.pdf"));
	CHECK_FALSE(contains(found, "corpus/notes.txt"));
}

TEST_CASE("an exclusion removes a subtree", "[InputEnumerator]")
{
	InputOptions options = from({ "corpus" });
	options.matcher = NameMatcher::fromGlobs({ "**/*.docx", "!**/archive/*" });

	InMemoryFileSystem fs = tree();
	std::vector<std::string> found = walk(fs, options);

	CHECK(contains(found, "corpus/reports/q1.docx"));
	CHECK_FALSE(contains(found, "corpus/reports/archive/old.docx"));
}

// ------------------------------------------------------------------ hidden

TEST_CASE("hidden entries are skipped by default", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	fs.addFile("corpus/.secret.docx").markHidden("corpus/.secret.docx");

	CHECK_FALSE(contains(walk(fs, from({ "corpus" })), "corpus/.secret.docx"));
}

TEST_CASE("include-hidden brings them back", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	fs.addFile("corpus/.secret.docx").markHidden("corpus/.secret.docx");

	InputOptions options = from({ "corpus" });
	options.includeHidden = true;

	CHECK(contains(walk(fs, options), "corpus/.secret.docx"));
}

TEST_CASE("a hidden directory hides everything under it", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	fs.addFile("corpus/.git/config.docx").markHidden("corpus/.git");

	CHECK_FALSE(contains(walk(fs, from({ "corpus" })), "corpus/.git/config.docx"));
}

// ---------------------------------------------------------------- symlinks

TEST_CASE("a symlinked directory is not entered by default", "[InputEnumerator]")
{
	// Following by default risks walking in a circle.
	InMemoryFileSystem fs = tree();
	fs.addFile("corpus/link/x.docx").markSymlink("corpus/link");

	CHECK_FALSE(contains(walk(fs, from({ "corpus" })), "corpus/link/x.docx"));
}

TEST_CASE("follow enters symlinked directories", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	fs.addFile("corpus/link/x.docx").markSymlink("corpus/link");

	InputOptions options = from({ "corpus" });
	options.followSymlinks = true;

	CHECK(contains(walk(fs, options), "corpus/link/x.docx"));
}

TEST_CASE("follow terminates when a link points at an ancestor", "[InputEnumerator]")
{
	// The reason the walk tracks directory identity rather than path
	// strings: a link back to its own parent produces an endless supply of
	// distinct paths --- corpus/loop/loop/loop/... --- so a visited-paths
	// set never closes the cycle.
	InMemoryFileSystem fs = tree();
	fs.markSymlink("corpus/loop", "corpus");

	InputOptions options = from({ "corpus" });
	options.followSymlinks = true;

	std::vector<std::string> found = walk(fs, options);
	for (const std::string& path : found)
		CHECK(path.find("loop/loop") == std::string::npos);
}

TEST_CASE("follow visits a shared directory once", "[InputEnumerator]")
{
	// Two links to the same place is not a cycle, but it is still one
	// directory: processing it twice would convert every document in it
	// twice.
	InMemoryFileSystem fs = tree();
	fs.addFile("shared/x.docx").markSymlink("corpus/one", "shared").markSymlink("corpus/two", "shared");

	InputOptions options = from({ "corpus" });
	options.followSymlinks = true;

	std::vector<std::string> found = walk(fs, options);

	std::string suffix = "x.docx";
	int matches = 0;
	for (const std::string& path : found)
	{
		if (path.size() >= suffix.size() && path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0)
			++matches;
	}
	CHECK(matches == 1);
}

// -------------------------------------------------------------------- sort

TEST_CASE("sort orders the walk", "[InputEnumerator]")
{
	InputOptions options = from({ "corpus" });
	options.sort = true;

	InMemoryFileSystem fs = tree();
	std::vector<std::string> found = walk(fs, options);

	std::vector<std::string> sorted = found;
	std::sort(sorted.begin(), sorted.end());
	CHECK(found == sorted);
}

TEST_CASE("without sort the walk follows the filesystem order", "[InputEnumerator]")
{
	// The counterpart that makes the test above meaningful: the fake
	// returns entries unordered, so if --sort were ignored the sorted
	// assertion would still have to fail somewhere.
	InMemoryFileSystem fs = tree();
	std::vector<std::string> found = walk(fs, from({ "corpus" }));

	std::vector<std::string> sorted = found;
	std::sort(sorted.begin(), sorted.end());
	CHECK_FALSE(found == sorted);
}

// -------------------------------------------------------------- files-from

TEST_CASE("files-from reads a list", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	fs.addFile("list.txt", "corpus/a.docx\ncorpus/b.pdf\n");

	InputOptions options;
	options.filesFrom = "list.txt";

	CHECK(walk(fs, options) == std::vector<std::string>{ "corpus/a.docx", "corpus/b.pdf" });
}

TEST_CASE("files-from ignores blank lines", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	// The last blank is a U+00A0 no-break space. string.IsNullOrWhiteSpace calls
	// that whitespace, so the reference drops the line; a byte-wise isspace does
	// not, and would try to open it as a path.
	fs.addFile("list.txt", "corpus/a.docx\n\n\n\xC2\xA0\ncorpus/b.pdf\n");

	InputOptions options;
	options.filesFrom = "list.txt";

	CHECK(walk(fs, options) == std::vector<std::string>{ "corpus/a.docx", "corpus/b.pdf" });
}

TEST_CASE("files-from dash reads standard input", "[InputEnumerator]")
{
	// "-" is not a path, so it goes through the console seam rather than
	// the filesystem --- otherwise a test could not supply it.
	RecordingConsole console;
	console.inputLines.push_back("corpus/a.docx");
	console.inputLines.push_back("corpus/b.pdf");

	InMemoryFileSystem fs = tree();
	InputEnumerator enumerator(fs, console);

	InputOptions options;
	options.filesFrom = "-";
	std::vector<InputSource> found = enumerator.enumerate(options);

	REQUIRE(found.size() == 2);
	CHECK(found[0].path == "corpus/a.docx");
	CHECK(found[1].path == "corpus/b.pdf");
}

TEST_CASE("files-from combines with positional paths", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	fs.addFile("list.txt", "corpus/b.pdf\n");

	InputOptions options = from({ "corpus/a.docx" });
	options.filesFrom = "list.txt";

	std::vector<std::string> found = walk(fs, options);

	CHECK(contains(found, "corpus/a.docx"));
	CHECK(contains(found, "corpus/b.pdf"));
}

// ------------------------------------------------------- relative paths

TEST_CASE("a walked file records its path relative to the root", "[InputEnumerator]")
{
	// This is what lets --out-dir replicate the source tree.
	InMemoryFileSystem fs = tree();
	RecordingConsole console;
	InputEnumerator enumerator(fs, console);

	std::vector<InputSource> found = enumerator.enumerate(from({ "corpus" }));

	auto it = std::find_if(found.begin(), found.end(), [](const InputSource& s) { return s.path == "corpus/reports/q1.docx"; });
	REQUIRE(it != found.end());
	CHECK(it->relativePath == "reports/q1.docx");
	REQUIRE(it->root.has_value());
	CHECK(*it->root == "corpus");
}

TEST_CASE("an explicit file has no root and is relative to itself", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	RecordingConsole console;
	InputEnumerator enumerator(fs, console);

	std::vector<InputSource> found = enumerator.enumerate(from({ "corpus/a.docx" }));

	REQUIRE(found.size() == 1);
	CHECK(found[0].relativePath == "a.docx");
	CHECK_FALSE(found[0].root.has_value());
}

TEST_CASE("a backslash path still reduces to a bare filename", "[InputEnumerator]")
{
	// A directory walk always yields forward-slash paths (walkInto builds
	// them from IFileSystem::listDirectory), but an explicit argument is
	// whatever the user typed on their own shell --- backslashes on
	// Windows. Reproduces the real bug: "C:\tmp\a.pdf" was kept whole as
	// relativePath because a naive filename extraction only recognised
	// '/', so --out-dir nested the source drive underneath itself instead
	// of flattening to "a.pdf".
	InMemoryFileSystem fs = tree();
	RecordingConsole console;
	InputEnumerator enumerator(fs, console);

	std::vector<InputSource> found = enumerator.enumerate(from({ "corpus\\a.docx" }));

	REQUIRE(found.size() == 1);
	CHECK(found[0].relativePath == "a.docx");
	CHECK_FALSE(found[0].root.has_value());
}

// ---------------------------------------------------------------- nothing

TEST_CASE("matching nothing is its own error", "[InputEnumerator]")
{
	InputOptions options = from({ "corpus" });
	options.matcher = NameMatcher::fromGlobs({ "**/*.nonexistent" });

	InMemoryFileSystem fs = tree();
	RecordingConsole console;
	InputEnumerator enumerator(fs, console);

	try
	{
		enumerator.enumerate(options);
		FAIL("expected a NoInputsError");
	}
	catch (const NoInputsError& error)
	{
		CHECK(error.exitCode() == ExitCode::noInputsMatched);
	}
}

TEST_CASE("an empty directory matches nothing", "[InputEnumerator]")
{
	InMemoryFileSystem fs;
	fs.addDirectory("empty");

	CHECK_THROWS_AS(walk(fs, from({ "empty" })), NoInputsError);
}

TEST_CASE("a path that does not exist is an error", "[InputEnumerator]")
{
	InMemoryFileSystem fs = tree();
	CHECK_THROWS_AS(walk(fs, from({ "corpus/missing.docx" })), CliError);
}

// ===========================================================================
//  InMemoryFileSystemTests
//
//  The in-memory filesystem itself. Test infrastructure needs testing too: a
//  fake that silently discards writes would make every conversion test above
//  it meaningless.
// ===========================================================================

TEST_CASE("a created file keeps what was written to it", "[InMemoryFileSystem]")
{
	InMemoryFileSystem fs;

	{
		std::unique_ptr<std::ostream> writer = fs.createFile("out/report.txt");
		*writer << "hello";
	}

	CHECK(fs.readText("out/report.txt") == "hello");
}

TEST_CASE("a created file can be read back through the interface", "[InMemoryFileSystem]")
{
	InMemoryFileSystem fs;

	{
		std::unique_ptr<std::ostream> writer = fs.createFile("out/report.txt");
		*writer << "hello";
	}

	std::unique_ptr<std::istream> reader = fs.openRead("out/report.txt");
	std::ostringstream buffer;
	buffer << reader->rdbuf();
	CHECK(buffer.str() == "hello");
}

TEST_CASE("creating a file creates its parent directories", "[InMemoryFileSystem]")
{
	InMemoryFileSystem fs;
	fs.createFile("out/deep/nested/report.txt");

	CHECK(fs.directoryExists("out/deep/nested"));
	CHECK(fs.fileExists("out/deep/nested/report.txt"));
}

TEST_CASE("listing returns immediate children only", "[InMemoryFileSystem]")
{
	InMemoryFileSystem fs;
	fs.addFile("a/b.txt").addFile("a/c/d.txt");

	// As a set: the fake returns children in a deliberately unhelpful order
	// so that a walk which ignores --sort cannot pass the sorting tests by
	// accident. Asserting a sequence here would be asserting that
	// anti-feature rather than the property this test is named for.
	std::vector<std::string> found = fs.listDirectory("a");
	std::sort(found.begin(), found.end());

	CHECK(found == std::vector<std::string>{ "a/b.txt", "a/c" });
}

TEST_CASE("paths are normalised so tests read the same on every platform", "[InMemoryFileSystem]")
{
	InMemoryFileSystem fs;
	fs.addFile("a\\b.txt");

	CHECK(fs.fileExists("a/b.txt"));
}

TEST_CASE("a link is a real directory entry so a walk can find it", "[InMemoryFileSystem]")
{
	// Without this the walk never sees the link, and every test about
	// following one passes without exercising anything.
	InMemoryFileSystem fs;
	fs.addFile("corpus/a.txt").markSymlink("corpus/link", "target");

	CHECK(contains(fs.listDirectory("corpus"), "corpus/link"));
	CHECK(fs.isSymbolicLink("corpus/link"));
}

TEST_CASE("listing through a link yields the targets children under the link path", "[InMemoryFileSystem]")
{
	InMemoryFileSystem fs;
	fs.addFile("shared/x.docx").markSymlink("corpus/link", "shared");

	CHECK(fs.listDirectory("corpus/link") == std::vector<std::string>{ "corpus/link/x.docx" });
}

TEST_CASE("a link to an ancestor produces an endless supply of distinct paths", "[InMemoryFileSystem]")
{
	// The property that makes path-based cycle detection fail, reproduced
	// in the fake so the walk's identity-based detection has something
	// real to solve.
	InMemoryFileSystem fs;
	fs.addFile("corpus/a.txt").markSymlink("corpus/loop", "corpus");

	CHECK(contains(fs.listDirectory("corpus/loop"), "corpus/loop/a.txt"));
	CHECK(contains(fs.listDirectory("corpus/loop"), "corpus/loop/loop"));
	CHECK(contains(fs.listDirectory("corpus/loop/loop"), "corpus/loop/loop/loop"));
}

TEST_CASE("two links to one directory share an identity", "[InMemoryFileSystem]")
{
	InMemoryFileSystem fs;
	fs.addFile("shared/x.docx").markSymlink("corpus/one", "shared").markSymlink("corpus/two", "shared");

	CHECK(fs.getDirectoryIdentity("corpus/one") == fs.getDirectoryIdentity("corpus/two"));
	CHECK(fs.getDirectoryIdentity("corpus/one") != fs.getDirectoryIdentity("corpus"));
}

TEST_CASE("a directory that cannot be read is reported rather than skipped", "[InputEnumerator]")
{
	// Carrying on with whatever could be listed hands back a short input list that
	// looks complete, so a run silently converts fewer documents than it was given.
	// Ignoring the listing's error_code did exactly that; the C# reference instead
	// let the platform's exception reach the top and end the process. Neither told
	// anyone a directory had been skipped.
	//
	// The message is fixed rather than the platform's own, which differs between
	// libstdc++ and .NET and would not be the same contract in two languages.
	InMemoryFileSystem fileSystem;
	fileSystem.addFile("corpus/a.docx");
	fileSystem.addFile("corpus/locked/b.docx");
	fileSystem.unreadable.insert("corpus/locked");

	RecordingConsole console;
	InputEnumerator enumerator(fileSystem, console);

	InputOptions options;
	options.paths.push_back("corpus");

	try
	{
		enumerator.enumerate(options);
		FAIL("expected a NoInputsError");
	}
	catch (const CliError& error)
	{
		CHECK(std::string(error.what()) == "corpus/locked: could not be read");
		CHECK(error.exitCode() == ExitCode::noInputsMatched);
	}
}
