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

// Containment of extracted subfile names. A subfile's name comes from the
// container, not from the person running the tool. An archive can carry
// "../../../etc/passwd", an absolute path, or a Windows drive letter, and a
// tool that joins that name onto the output directory writes wherever the
// archive says --- the vulnerability usually called zip slip.
//
// Everything here is a pure string operation, so the whole attack surface is
// testable without an archive, a document or a filesystem.

#include "Execution/SubfileExtraction.h"

#include <catch2/catch_test_macros.hpp>

#include <sstream>

using namespace Hyland::DocumentFilters::Cli;

namespace
{
	std::string safe(const std::string& name, bool replicateTree = true)
	{
		return Execution::SubfileExtraction::resolveSafeOutputPath("/out", name, replicateTree);
	}

	void assertInsideRoot(const std::string& resolved)
	{
		std::string normalised = resolved;
		for (char& ch : normalised)
		{
			if (ch == '\\')
				ch = '/';
		}

		CHECK(normalised.rfind("/out/", 0) == 0);

		// Traversal is a path *component* equal to "..", not any occurrence
		// of the two characters. Rejecting the substring would also reject
		// the legitimate filename "..data", which the test below requires to
		// survive.
		std::istringstream stream(normalised);
		std::string component;
		while (std::getline(stream, component, '/'))
		{
			CHECK(component != "..");
			CHECK(component != ".");
		}
	}
}

// ------------------------------------------------------------ ordinary names

TEST_CASE("an ordinary name lands directly in the output directory", "[SubfileExtraction]")
{
	CHECK(safe("readme.txt") == "/out/readme.txt");
}

TEST_CASE("a nested name replicates its structure", "[SubfileExtraction]")
{
	CHECK(safe("reports/q1.docx") == "/out/reports/q1.docx");
}

TEST_CASE("flat mode keeps only the final component", "[SubfileExtraction]")
{
	CHECK(safe("reports/q1.docx", false) == "/out/q1.docx");
}

// ---------------------------------------------------------------- traversal

TEST_CASE("a traversing name cannot escape the output directory", "[SubfileExtraction]")
{
	for (const char* name : { "../outside.txt", "../../outside.txt", "../../../../../../etc/passwd", "reports/../../outside.txt",
			"./../outside.txt", "a/b/../../../outside.txt" })
		assertInsideRoot(safe(name));
}

TEST_CASE("backslash traversal is caught too", "[SubfileExtraction]")
{
	// An archive written on Windows uses backslashes, and a tool that only
	// looked for forward slashes would let those through on Linux.
	for (const char* name : { "..\\outside.txt", "..\\..\\windows\\system32\\evil.dll", "reports\\..\\..\\outside.txt" })
		assertInsideRoot(safe(name));
}

TEST_CASE("an absolute name is taken as relative", "[SubfileExtraction]")
{
	for (const char* name : { "/etc/passwd", "/absolute/path.txt", "\\windows\\system32\\evil.dll" })
		assertInsideRoot(safe(name));
}

TEST_CASE("a drive or share qualified name is taken as relative", "[SubfileExtraction]")
{
	for (const char* name : { "C:\\windows\\evil.dll", "C:/windows/evil.dll", "\\\\server\\share\\evil.dll" })
		assertInsideRoot(safe(name));
}

TEST_CASE("traversal is neutralised in flat mode as well", "[SubfileExtraction]")
{
	// Flat mode reduces to the final component, which happens to defuse most
	// traversal --- but a name that is *only* traversal must not slip
	// through.
	assertInsideRoot(safe("../../outside.txt", false));
}

// -------------------------------------------------------------- odd names

TEST_CASE("a degenerate name still produces a path inside the root", "[SubfileExtraction]")
{
	for (const char* name : { "..", ".", "...", "", "   " })
		assertInsideRoot(safe(name));
}

TEST_CASE("a name that is only separators produces a path inside the root", "[SubfileExtraction]")
{
	assertInsideRoot(safe("///"));
}

TEST_CASE("a directory named dot dot something is not traversal and survives", "[SubfileExtraction]")
{
	// "..data" is a legitimate filename; only the "." and ".." components are
	// traversal, and an over-eager filter would corrupt valid names.
	CHECK(safe("..data/x.txt").find("..data") != std::string::npos);
	assertInsideRoot(safe("..data/x.txt"));
}

TEST_CASE("two subfiles with the same name resolve to the same path", "[SubfileExtraction]")
{
	// Containment does not deduplicate: overwriting is the extract command's
	// business, through --overwrite. This just records that the guard is not
	// silently renaming things.
	CHECK(safe("readme.txt") == safe("readme.txt"));
}
