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

// Where DocumentFiltersSession::create looks for the engine's own resources
// --- fonts included. doc2text and the other published samples default this
// to "." and get away with it because they always run from their own build
// output directory. This tool is invoked from wherever the user's documents
// happen to be, so "." there resolves to a directory with no fonts at all:
// the engine still initialises, but falls back to a font it cannot
// rasterise glyphs from, so rendered text comes out blank instead of
// missing. resolveLibraryPath is the precedence decision alone, pure and
// without the environment or the real executable path, so it is testable
// without either.

#include "Configuration/SharedOptions.h"
#include "Execution/DocumentFiltersSession.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli;

TEST_CASE("an explicit library path wins over everything", "[DocumentFiltersSession]")
{
	Configuration::LibraryOptions options;
	options.libraryPath = "/opt/df";

	std::string resolved = Execution::DocumentFiltersSession::resolveLibraryPath(options, std::string("/env/df"), "/exe/dir");

	CHECK(resolved == "/opt/df");
}

TEST_CASE("the environment variable wins over the executable directory", "[DocumentFiltersSession]")
{
	Configuration::LibraryOptions options;

	std::string resolved = Execution::DocumentFiltersSession::resolveLibraryPath(options, std::string("/env/df"), "/exe/dir");

	CHECK(resolved == "/env/df");
}

TEST_CASE("with neither argument nor environment variable it falls back to the executable directory", "[DocumentFiltersSession]")
{
	// Not the process's current directory: a general-purpose CLI is invoked
	// from wherever the user's documents are, not from its own install
	// directory, and "." would send font lookup there instead.
	Configuration::LibraryOptions options;

	std::string resolved = Execution::DocumentFiltersSession::resolveLibraryPath(options, std::nullopt, "/exe/dir");

	CHECK(resolved == "/exe/dir");
}
