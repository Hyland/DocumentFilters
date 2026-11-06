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

// How meta turns the engine's metadata blob into one record per field. There
// is no structured metadata API --- getText returns the fields as one run of
// text, and the record separator is not a newline. The engine emits 0x0F
// before the block and 0x0E between records, which is why these fixtures are
// built from control codes rather than '\n': a parser that splits on
// newlines finds no boundary at all and reports every field of a document as
// one.

#include "Engine/IDocumentFiltersEngine.h"
#include "Execution/MetadataReport.h"
#include "Fakes/FakeDocumentFiltersEngine.h"
#include "Fakes/RecordingConsole.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocumentFiltersEngine;
using Hyland::DocumentFilters::Cli::Tests::Fakes::RecordingConsole;

namespace
{
	constexpr char startOfMeta = '\x0F';
	constexpr char separator = '\x0E';
	constexpr char endOfMeta = '\x10';

	std::vector<std::string> report(const std::string& metadata)
	{
		FakeDocumentFiltersEngine engine;
		engine.addDocument("a.docx", metadata);
		RecordingConsole console;

		std::unique_ptr<Engine::IDocument> document = engine.openDocument("a.docx", 0, std::string(), []() { return std::nullopt; }, nullptr, nullptr, nullptr);
		Execution::MetadataReport::writeText(*document, "a.docx", console);

		return console.outLines;
	}
}

TEST_CASE("records separated by control codes each become one line", "[MetadataReport]")
{
	// The real shape, from a PDF: 0x0F opens the block, 0x0E divides it, 0x10
	// closes it. Splitting this on '\n' yields a single bogus field whose
	// label still carries the 0x0F.
	std::vector<std::string> lines =
		report(std::string(1, startOfMeta) + "Title: Report" + separator + "Author: Ada" + separator + "Producer: Quartz" + endOfMeta);

	CHECK(lines == std::vector<std::string>{ "a.docx: field=Title value=Report", "a.docx: field=Author value=Ada", "a.docx: field=Producer value=Quartz" });
}

TEST_CASE("a value carrying a newline stays one record", "[MetadataReport]")
{
	// A Word Title or Comments field routinely has one; the continuation used
	// to be dropped, silently truncating the value.
	std::vector<std::string> lines = report(std::string(1, startOfMeta) + "Title: Quarterly Report\nsecond line" + separator + "Author: Ada");

	CHECK(lines == std::vector<std::string>{ "a.docx: field=Title value=Quarterly Report second line", "a.docx: field=Author value=Ada" });
}

TEST_CASE("carriage returns and tabs inside a value become single spaces", "[MetadataReport]")
{
	std::vector<std::string> lines = report(std::string(1, startOfMeta) + "Comments: one\ttwo\r\nthree");

	CHECK(lines == std::vector<std::string>{ "a.docx: field=Comments value=one two three" });
}

TEST_CASE("a label containing colons is not cut apart", "[MetadataReport]")
{
	// Splitting on the first ": " rather than the first ':' is what keeps an
	// XMP label whole --- none of its internal colons is followed by a space.
	std::vector<std::string> lines = report(std::string(1, startOfMeta) + "xmp:dc:creator: slop001");

	CHECK(lines == std::vector<std::string>{ "a.docx: field=xmp:dc:creator value=slop001" });
}

TEST_CASE("a document with no metadata reports nothing", "[MetadataReport]")
{
	CHECK(report("").empty());
	CHECK(report(std::string(1, startOfMeta) + endOfMeta).empty());
}

TEST_CASE("a continuation with no field before it is ignored", "[MetadataReport]")
{
	// Nothing to attach it to, so it is dropped rather than inventing a field.
	CHECK(report("loose text with no label").empty());
}
