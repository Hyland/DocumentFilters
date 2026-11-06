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

// --fingerprint's line formatting, the one part of it that does not need a
// real page: CanvasConversion::writeFingerprint reads the two page
// attributes and hands them to CanvasConversion::formatFingerprint, which is
// a pure function of the two strings it already read.
//
// Confirmed against a real isys_doc2text.exe --fingerprint run:
// IMAGE_FINGERPRINT_V2 is tried first, IMAGE_FINGERPRINT is the fallback,
// and the value itself (e.g. c:QlpoOTFB...) is opaque to this tool --- it is
// printed exactly as the engine returns it.

#include "Configuration/CommandOptions.h"
#include "Execution/CanvasConversion.h"
#include "Execution/DocumentFiltersSession.h"
#include "Fakes/FakeDocumentFiltersEngine.h"
#include "Fakes/RecordingConsole.h"
#include "Planning/FormatCatalog.h"
#include "Planning/PageRange.h"
#include "Planning/RunPlan.h"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocumentFiltersEngine;
using Hyland::DocumentFilters::Cli::Tests::Fakes::RecordingConsole;

TEST_CASE("the v2 fingerprint is used when present", "[CanvasConversion]")
{
	std::optional<std::string> line = Execution::CanvasConversion::formatFingerprint("sample.docx", 1, "c:v2value", "c:v1value");
	CHECK(line == std::optional<std::string>("sample.docx: page=1 fingerprint=c:v2value"));
}

TEST_CASE("the v1 fingerprint is the fallback", "[CanvasConversion]")
{
	std::optional<std::string> line = Execution::CanvasConversion::formatFingerprint("sample.docx", 2, "", "c:v1value");
	CHECK(line == std::optional<std::string>("sample.docx: page=2 fingerprint=c:v1value"));
}

TEST_CASE("nothing is written when neither attribute has a value", "[CanvasConversion]")
{
	CHECK_FALSE(Execution::CanvasConversion::formatFingerprint("sample.docx", 1, "", "").has_value());
}

TEST_CASE("the page number is one based and the display name is used verbatim", "[CanvasConversion]")
{
	// A subfile's display name already carries its container, e.g.
	// "bundle.zip>inner.docx" --- this only ever echoes it, never rebuilds
	// it.
	std::optional<std::string> line = Execution::CanvasConversion::formatFingerprint("bundle.zip>inner.docx", 3, "c:abc", "");
	CHECK(line == std::optional<std::string>("bundle.zip>inner.docx: page=3 fingerprint=c:abc"));
}

// === canvas option composition =========================================
//
// The engine resolves a repeated key to its first occurrence, so the order
// here is behaviour, not formatting: --canvas-options leads and the
// format's own options fill in behind it.

TEST_CASE("a format with no options of its own passes the users through unchanged", "[CanvasConversion]")
{
	const Planning::FormatSpec& png = Planning::FormatCatalog::resolve("png");
	CHECK(Execution::CanvasConversion::composeCanvasOptions(png, std::string("GRAPHIC_DPI=300")) == "GRAPHIC_DPI=300");
}

TEST_CASE("a format with no options and no user options composes to nothing", "[CanvasConversion]")
{
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("png"), std::nullopt) == "");
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("png"), std::string("")) == "");
}

TEST_CASE("a formats own options apply when the user gives none", "[CanvasConversion]")
{
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("html5"), std::nullopt) == "HTML_INLINE_IMAGES=ON");
}

TEST_CASE("the users options come first so they win the keys they name", "[CanvasConversion]")
{
	// Not cosmetic: first occurrence wins in the engine, so this is what
	// makes --canvas-options HTML_INLINE_IMAGES=OFF actually take effect.
	std::string composed = Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("html5"), std::string("HTML_INLINE_IMAGES=OFF"));

	CHECK(composed == "HTML_INLINE_IMAGES=OFF;HTML_INLINE_IMAGES=ON");
	CHECK(composed.rfind("HTML_INLINE_IMAGES=OFF", 0) == 0);
}

TEST_CASE("an unrelated user option leaves the formats own in place", "[CanvasConversion]")
{
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("html5"), std::string("GRAPHIC_DPI=300"))
		== "GRAPHIC_DPI=300;HTML_INLINE_IMAGES=ON");
}

TEST_CASE("trailing separators do not produce an empty setting", "[CanvasConversion]")
{
	// The catalogue writes its options with a trailing ';' and users often do
	// too; an empty setting between two ';' is not worth relying on the
	// engine to ignore.
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("html5"), std::string("GRAPHIC_DPI=300;"))
		== "GRAPHIC_DPI=300;HTML_INLINE_IMAGES=ON");
}

TEST_CASE("the inlining formats carry their switch", "[CanvasConversion]")
{
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("html5"), std::nullopt) == "HTML_INLINE_IMAGES=ON");
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("svg"), std::nullopt) == "SVG_INLINE_IMAGES=ON");
}

TEST_CASE("mdast still carries the schema that tells it apart from json", "[CanvasConversion]")
{
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("mdast"), std::nullopt) == "JSON_OUTPUT_SCHEMA=MDAST");
	CHECK(Execution::CanvasConversion::composeCanvasOptions(Planning::FormatCatalog::resolve("json"), std::nullopt) == "");
}

// -------------------------------------------------------------- --stdout

namespace
{
	/// <summary>
	/// A write-only, forward-only sink, the way the real console's standard
	/// output stream actually behaves: it cannot be seeked or read back,
	/// unlike a plain in-memory buffer, which would let a bug that seeks the
	/// destination directly pass silently.
	/// </summary>
	class ForwardOnlySinkBuf : public std::streambuf
	{
	public:
		int flushCount = 0;
		long long bytesWritten = 0;

	protected:
		std::streamsize xsputn(const char*, std::streamsize n) override
		{
			bytesWritten += n;
			return n;
		}

		int sync() override
		{
			flushCount++;
			return 0;
		}

		pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override
		{
			throw std::runtime_error("ForwardOnlySink cannot seek");
		}

		pos_type seekpos(pos_type, std::ios_base::openmode) override { throw std::runtime_error("ForwardOnlySink cannot seek"); }

		int_type underflow() override { throw std::runtime_error("ForwardOnlySink cannot read"); }
	};

	class ForwardOnlySink : private ForwardOnlySinkBuf, public std::ostream
	{
	public:
		ForwardOnlySink() : std::ostream(this) {}

		int flushCount() const { return ForwardOnlySinkBuf::flushCount; }
	};
}

TEST_CASE("streaming to stdout never seeks the destination and flushes it once done", "[CanvasConversion]")
{
	// Reproduces a real bug found right after the --stdout NRE fix: handing
	// the console's raw output stream straight to makeCanvas produced no
	// error and no output. Every published sample that streams a canvas
	// renders into an in-memory buffer instead, because finishing a binary
	// format means seeking back to patch a header computed only once
	// rendering is done --- and the console's own stream is forward-only.
	// convertToStream now renders into its own seekable buffer and only
	// copies the finished bytes across, so a destination that would throw on
	// any seek attempt must still work.
	FakeDocumentFiltersEngine engine;
	engine.addDocument("a.docx", "text");
	RecordingConsole console;
	Execution::DocumentFiltersSession session = Execution::DocumentFiltersSession::forEngine(engine);
	Execution::CanvasConversion conversion(session, console);

	ForwardOnlySink destination;
	Planning::PlannedItem item;
	item.sourcePath = "a.docx";
	item.displayName = "a.docx";
	item.pages = Planning::PageRange::all();

	std::unique_ptr<Engine::IDocument> document = engine.openDocument(
		"a.docx", 0, std::string(), []() { return std::nullopt; }, nullptr, nullptr, nullptr);

	conversion.convertToStream(*document, destination, item, Planning::FormatCatalog::resolve("html5"), Configuration::ConvertOptions());

	CHECK(destination.flushCount() > 0);
}
