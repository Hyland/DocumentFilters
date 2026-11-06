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

// What the subfile walk does with each way a container can misbehave. The
// engine hands subfile names straight out of the container, which is
// attacker-controlled data in a way a file on disk is not. These check both
// halves: that a failure inside a container is handled, and that a hostile
// name cannot write outside the output directory (D17).

#include "Configuration/CommandOptions.h"
#include "Configuration/SharedOptions.h"
#include "Execution/SubfileExtraction.h"
#include "Fakes/FakeDocumentFiltersEngine.h"
#include "Fakes/InMemoryFileSystem.h"
#include "Planning/NameMatcher.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <ctime>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocument;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocumentFiltersEngine;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeSubFile;
using Hyland::DocumentFilters::Cli::Tests::Fakes::InMemoryFileSystem;

namespace
{
	std::vector<Engine::EngineError> everyEngineError()
	{
		return {
			Engine::EngineError::openError,
			Engine::EngineError::wrongType,
			Engine::EngineError::inUse,
			Engine::EngineError::notReadable,
			Engine::EngineError::password,
			Engine::EngineError::notFound,
			Engine::EngineError::writeError,
			Engine::EngineError::notValidForThisClass,
			Engine::EngineError::error,
			Engine::EngineError::invalidHandle,
			Engine::EngineError::invalidPointer,
			Engine::EngineError::invalidParameter,
			Engine::EngineError::fileCorrupt,
			Engine::EngineError::outOfMemory,
			Engine::EngineError::badError,
			Engine::EngineError::wrongThread,
			Engine::EngineError::tooManyHandles,
			Engine::EngineError::fontsNotFound,
			Engine::EngineError::versionMismatch,
			Engine::EngineError::actionNotSupported,
		};
	}

	Configuration::SharedOptions shared(bool subfilesEnabled = false, std::optional<int> depth = std::nullopt)
	{
		Configuration::SharedOptions options;
		options.output.outDir = "out";
		options.subfiles.enabled = subfilesEnabled;
		options.subfiles.maxDepth = depth;
		return options;
	}

	std::chrono::system_clock::time_point makeTime(int year, int month, int day, int hour, int minute, int second)
	{
		std::tm tm {};
		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = minute;
		tm.tm_sec = second;
#if defined(_WIN32)
		return std::chrono::system_clock::from_time_t(_mkgmtime(&tm));
#else
		return std::chrono::system_clock::from_time_t(timegm(&tm));
#endif
	}
}

// ============================================ listing

TEST_CASE("every entry the container reports is listed", "[EngineSubfileOutcome]")
{
	FakeDocument container("bundle.zip");
	auto readme = std::make_shared<Tests::Fakes::FakeSubFileState>();
	readme->name = "readme.txt";
	readme->id = "0001";
	readme->size = 4096;
	readme->modified = makeTime(2026, 1, 15, 9, 30, 0);
	container.state()->subFileEntries.push_back(readme);

	auto q1 = std::make_shared<Tests::Fakes::FakeSubFileState>();
	q1->name = "reports/q1.docx";
	q1->id = "0002";
	q1->size = 20480;
	container.state()->subFileEntries.push_back(q1);

	InMemoryFileSystem fileSystem;
	std::vector<Execution::SubfileEntry> entries = Execution::SubfileExtraction(fileSystem).listAll(container, "bundle.zip", shared());

	REQUIRE(entries.size() == 2);
	CHECK(entries[0].displayName == "bundle.zip>readme.txt");
	CHECK(entries[0].modified == std::optional<std::string>("2026-01-15T09:30:00Z"));
	CHECK(entries[0].depth == 1);
}

TEST_CASE("a container with no entries lists nothing rather than failing", "[EngineSubfileOutcome]")
{
	FakeDocument container("flat.txt");
	InMemoryFileSystem fileSystem;
	std::vector<Execution::SubfileEntry> entries = Execution::SubfileExtraction(fileSystem).listAll(container, "flat.txt", shared());

	CHECK(entries.empty());
}

TEST_CASE("a container that fails to enumerate surfaces the engine error", "[EngineSubfileOutcome]")
{
	// Nothing above the seam catches the binding's own exception type, so
	// this has to arrive as an EngineException for the command layer to turn
	// into a DocumentError.
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocument container("bundle.zip");
		container.state()->readFailure = error;
		container.state()->readFailureOn = "subFiles";

		InMemoryFileSystem fileSystem;
		bool threw = false;
		try
		{
			Execution::SubfileExtraction(fileSystem).listAll(container, "bundle.zip", shared());
		}
		catch (const Engine::EngineException& thrown)
		{
			threw = true;
			CHECK(thrown.error() == error);
			CHECK(thrown.code() == static_cast<int>(error));
		}
		CHECK(threw);
	}
}

// ============================================ nesting

TEST_CASE("nested containers are walked when subfiles are enabled", "[EngineSubfileOutcome]")
{
	FakeDocument inner("inner.zip");
	auto note = std::make_shared<Tests::Fakes::FakeSubFileState>();
	note->name = "note.txt";
	note->id = "0001";
	inner.state()->subFileEntries.push_back(note);

	FakeDocument outer("nested.zip");
	auto innerEntry = std::make_shared<Tests::Fakes::FakeSubFileState>();
	innerEntry->name = "inner.zip";
	innerEntry->id = "0001";
	innerEntry->content = inner.state();
	outer.state()->subFileEntries.push_back(innerEntry);

	InMemoryFileSystem fileSystem;
	std::vector<Execution::SubfileEntry> entries = Execution::SubfileExtraction(fileSystem).listAll(outer, "nested.zip", shared(true));

	REQUIRE(entries.size() == 2);
	CHECK(entries[0].displayName == "nested.zip>inner.zip");
	CHECK(entries[1].displayName == "nested.zip>inner.zip>note.txt");
	CHECK(entries[1].depth == 2);
}

TEST_CASE("subfile depth stops the walk where it says", "[EngineSubfileOutcome]")
{
	FakeDocument deepest("deepest.zip");
	auto buried = std::make_shared<Tests::Fakes::FakeSubFileState>();
	buried->name = "buried.txt";
	buried->id = "0001";
	deepest.state()->subFileEntries.push_back(buried);

	FakeDocument middle("middle.zip");
	auto deepestEntry = std::make_shared<Tests::Fakes::FakeSubFileState>();
	deepestEntry->name = "deepest.zip";
	deepestEntry->id = "0001";
	deepestEntry->content = deepest.state();
	middle.state()->subFileEntries.push_back(deepestEntry);

	FakeDocument outer("outer.zip");
	auto middleEntry = std::make_shared<Tests::Fakes::FakeSubFileState>();
	middleEntry->name = "middle.zip";
	middleEntry->id = "0001";
	middleEntry->content = middle.state();
	outer.state()->subFileEntries.push_back(middleEntry);

	InMemoryFileSystem fileSystem;
	std::vector<Execution::SubfileEntry> entries = Execution::SubfileExtraction(fileSystem).listAll(outer, "outer.zip", shared(true, 2));

	// Depth 2 reaches middle.zip's own children and stops before their
	// contents.
	CHECK(entries.size() == 2);
	for (const Execution::SubfileEntry& entry : entries)
		CHECK(entry.name != "buried.txt");
}

TEST_CASE("without subfiles enabled only the top level is reported", "[EngineSubfileOutcome]")
{
	FakeDocument inner("inner.zip");
	auto note = std::make_shared<Tests::Fakes::FakeSubFileState>();
	note->name = "note.txt";
	note->id = "0001";
	inner.state()->subFileEntries.push_back(note);

	FakeDocument outer("nested.zip");
	auto innerEntry = std::make_shared<Tests::Fakes::FakeSubFileState>();
	innerEntry->name = "inner.zip";
	innerEntry->id = "0001";
	innerEntry->content = inner.state();
	outer.state()->subFileEntries.push_back(innerEntry);

	InMemoryFileSystem fileSystem;
	std::vector<Execution::SubfileEntry> entries = Execution::SubfileExtraction(fileSystem).listAll(outer, "nested.zip", shared());

	CHECK(entries.size() == 1);
}

TEST_CASE("an entry that will not open surfaces the engine error", "[EngineSubfileOutcome]")
{
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocument outer("nested.zip");
		auto innerEntry = std::make_shared<Tests::Fakes::FakeSubFileState>();
		innerEntry->name = "inner.zip";
		innerEntry->id = "0001";
		innerEntry->openFailure = error;
		outer.state()->subFileEntries.push_back(innerEntry);

		InMemoryFileSystem fileSystem;
		bool threw = false;
		try
		{
			Execution::SubfileExtraction(fileSystem).listAll(outer, "nested.zip", shared(true));
		}
		catch (const Engine::EngineException& thrown)
		{
			threw = true;
			CHECK(thrown.error() == error);
		}
		CHECK(threw);
	}
}

// ============================================ extraction

TEST_CASE("extraction writes every entry that passes the filters", "[EngineSubfileOutcome]")
{
	FakeDocument container("archive.zip");
	auto keep = std::make_shared<Tests::Fakes::FakeSubFileState>();
	keep->name = "keep.docx";
	keep->id = "0001";
	auto drop = std::make_shared<Tests::Fakes::FakeSubFileState>();
	drop->name = "draft-notes.txt";
	drop->id = "0002";
	container.state()->subFileEntries.push_back(keep);
	container.state()->subFileEntries.push_back(drop);

	// Compiled the way CommandLineApp compiles them when the argument is
	// read, so this tests the walk's use of a matcher rather than the
	// binding of one.
	Configuration::ExtractOptions options;
	options.subfileInclude = Planning::NameMatcher::fromGlobs({ "*.docx" });

	InMemoryFileSystem fileSystem;
	std::vector<std::string> written = Execution::SubfileExtraction(fileSystem).extractAll(container, "archive.zip", shared(), options);

	REQUIRE(written.size() == 1);
	CHECK(keep->copiedTo.size() == 1);
	CHECK(drop->copiedTo.empty());
}

TEST_CASE("an entry that will not copy surfaces the engine error", "[EngineSubfileOutcome]")
{
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocument container("archive.zip");
		auto broken = std::make_shared<Tests::Fakes::FakeSubFileState>();
		broken->name = "broken.docx";
		broken->id = "0001";
		broken->copyFailure = error;
		container.state()->subFileEntries.push_back(broken);

		InMemoryFileSystem fileSystem;
		bool threw = false;
		try
		{
			Execution::SubfileExtraction(fileSystem).extractAll(container, "archive.zip", shared(), Configuration::ExtractOptions());
		}
		catch (const Engine::EngineException& thrown)
		{
			threw = true;
			CHECK(thrown.error() == error);
		}
		CHECK(threw);
	}
}

TEST_CASE("a hostile entry name stays inside the output directory", "[EngineSubfileOutcome]")
{
	// D17. The name comes from the archive, not from the person running the
	// tool, so it is attacker-controlled: traversal, an absolute path, a
	// drive letter and a UNC share all have to be neutralised. Checked
	// through the real extraction path rather than only against
	// resolveSafeOutputPath, so that a future caller bypassing it is caught
	// here.
	for (const char* hostileName : { "../../outside.txt", "..\\..\\outside.txt", "/etc/passwd", "C:\\Windows\\System32\\evil.dll",
			"\\\\server\\share\\evil.dll", "../", ".." })
	{
		FakeDocument container("archive.zip");
		auto entry = std::make_shared<Tests::Fakes::FakeSubFileState>();
		entry->name = hostileName;
		entry->id = "0001";
		container.state()->subFileEntries.push_back(entry);

		InMemoryFileSystem fileSystem;
		std::vector<std::string> written = Execution::SubfileExtraction(fileSystem).extractAll(container, "archive.zip", shared(), Configuration::ExtractOptions());

		REQUIRE(written.size() == 1);
		CHECK(written[0].rfind("out/", 0) == 0);
		CHECK(written[0].find("..") == std::string::npos);
		REQUIRE(entry->copiedTo.size() == 1);
		CHECK(entry->copiedTo[0] == written[0]);
	}
}

TEST_CASE("images are extracted only when asked for", "[EngineSubfileOutcome]")
{
	FakeDocument container("doc.docx");
	auto image = std::make_shared<Tests::Fakes::FakeSubFileState>();
	image->name = "image1.png";
	image->id = "0001";
	container.state()->imageEntries.push_back(image);

	InMemoryFileSystem fileSystem;
	CHECK(Execution::SubfileExtraction(fileSystem).extractAll(container, "doc.docx", shared(), Configuration::ExtractOptions()).empty());
	CHECK(image->copiedTo.empty());

	Configuration::ExtractOptions withImages;
	withImages.images = true;
	std::vector<std::string> written = Execution::SubfileExtraction(fileSystem).extractAll(container, "doc.docx", shared(), withImages);

	CHECK(written.size() == 1);
	CHECK(image->copiedTo.size() == 1);
}

// ============================================ timestamps

TEST_CASE("a container that records no timestamp reports none", "[EngineSubfileOutcome]")
{
	// The binding gives an unset optional for an entry with no date;
	// reporting that as a real date would be a lie. The epoch stands in for
	// the C# reference's DateTime.MinValue --- also year <= 1970, so it
	// reports none the same way.
	CHECK_FALSE(Execution::SubfileExtraction::formatTimestamp(std::nullopt).has_value());
	CHECK_FALSE(Execution::SubfileExtraction::formatTimestamp(std::chrono::system_clock::time_point{}).has_value());
	CHECK(Execution::SubfileExtraction::formatTimestamp(makeTime(2026, 1, 15, 9, 30, 0)) == std::optional<std::string>("2026-01-15T09:30:00Z"));
}
