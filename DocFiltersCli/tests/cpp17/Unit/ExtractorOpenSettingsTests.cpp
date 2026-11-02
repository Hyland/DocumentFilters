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

// The open flags each command and format is opened with. Worth pinning by
// literal value rather than by enum name: the IGR_FORMAT_* constants are an
// enumeration packed into bits 16 to 19 (Text 0x1, HTML 0x2, Image 0x5,
// Markdown 0x8, each shifted by 16), not independent bits, so two of them
// OR'd together silently make a third --- and the numbers are the contract
// the Python, Java and C# ports have to reproduce.

#include "Configuration/CommandOptions.h"
#include "Configuration/SharedOptions.h"
#include "Execution/ExtractorOpenSettings.h"
#include "Planning/FormatCatalog.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli;

namespace
{
	constexpr int bodyAndMeta = 2;
	constexpr int metaOnly = 1;
	constexpr int formatText = 0x00010000;
	constexpr int formatHtml = 0x00020000;
	constexpr int formatImage = 0x00050000;
	constexpr int formatMarkdown = 0x00080000;

	int bit(const std::string& token, const Configuration::ConvertOptions& options = Configuration::ConvertOptions())
	{
		return Execution::ExtractorOpenSettings::formatBit(Planning::FormatCatalog::resolve(token), options);
	}
}

TEST_CASE("a canvas format opens paginated", "[ExtractorOpenSettings]")
{
	for (const char* token : { "png", "pdf", "html5", "tiff", "mdast" })
		CHECK(bit(token) == formatImage);
}

TEST_CASE("flow html asks for the html reader not the text one", "[ExtractorOpenSettings]")
{
	// FormatText here means --to html quietly emits plain text, which looks
	// plausible enough to ship.
	CHECK(bit("html") == formatHtml);
}

TEST_CASE("text mode markdown asks for the markdown reader", "[ExtractorOpenSettings]")
{
	CHECK(bit("md-text") == formatMarkdown);
}

TEST_CASE("plain text asks for the text reader", "[ExtractorOpenSettings]")
{
	CHECK(bit("text") == formatText);
}

TEST_CASE("include control codes asks for no reader at all", "[ExtractorOpenSettings]")
{
	// Omitting IGR_FORMAT_TEXT is the whole mechanism: it leaves the ISYS
	// control codes in the stream rather than having the engine strip them.
	Configuration::ConvertOptions options;
	options.includeControlCodes = true;
	CHECK(bit("text", options) == 0);
}

TEST_CASE("docpack opens with no format bit", "[ExtractorOpenSettings]")
{
	// Matches doc2text, which opens --docpack with a bare IGR_BODY_AND_META.
	CHECK(bit("docpack") == 0);
}

TEST_CASE("a structure report does not change the format bit", "[ExtractorOpenSettings]")
{
	// Forcing paginated mode here was tried and does not work: it takes the
	// text reader away from the conversion opened in the same call. The
	// pairing is refused by OptionsValidator::checkReportsNeedCanvasFormat
	// instead, so by the time this runs a report implies a canvas format
	// already.
	Configuration::ConvertOptions options;
	options.reports.dimensions = true;
	options.reports.bookmarks = true;

	CHECK(bit("text", options) == formatText);
	CHECK(bit("md-text", options) == formatMarkdown);
	CHECK(bit("pdf", options) == formatImage);
}

TEST_CASE("only one format bit is ever set", "[ExtractorOpenSettings]")
{
	// They are an enumeration in bits 16-19, so two OR'd together make a third.
	for (const Planning::FormatSpec& format : Planning::FormatCatalog::all())
	{
		int value = Execution::ExtractorOpenSettings::formatBit(format, Configuration::ConvertOptions());
		CHECK((value == 0 || value == formatText || value == formatHtml || value == formatImage || value == formatMarkdown));
	}
}

TEST_CASE("the content selector is carried alongside the format bit", "[ExtractorOpenSettings]")
{
	Execution::ExtractorOpenSettings settings =
		Execution::ExtractorOpenSettings::forFormat(Planning::FormatCatalog::resolve("html"), Configuration::ConvertOptions(), Configuration::SharedOptions());

	CHECK(settings.flags == (bodyAndMeta | formatHtml));
}

TEST_CASE("inspection commands open body and metadata as text", "[ExtractorOpenSettings]")
{
	CHECK(Execution::ExtractorOpenSettings::forInspection(Configuration::SharedOptions()).flags == (bodyAndMeta | formatText));
}

TEST_CASE("extract images asks the engine to enumerate them", "[ExtractorOpenSettings]")
{
	// ENUMERATE_IMAGES is read at open and nowhere else, so --images iterating
	// IDocument::images() without it finds an empty collection every time.
	CHECK(Execution::ExtractorOpenSettings::forInspection(Configuration::SharedOptions(), true).options == "ENUMERATE_IMAGES=ON;");
	CHECK(Execution::ExtractorOpenSettings::forInspection(Configuration::SharedOptions()).options == "");
}

TEST_CASE("the users engine options come before the one we add", "[ExtractorOpenSettings]")
{
	// First occurrence wins in the engine, so --options ENUMERATE_IMAGES=OFF
	// still overrides what --images would otherwise turn on.
	Configuration::SharedOptions shared;
	shared.processing.engineOptions = "ENUMERATE_IMAGES=OFF";

	CHECK(Execution::ExtractorOpenSettings::forInspection(shared, true).options == "ENUMERATE_IMAGES=OFF;ENUMERATE_IMAGES=ON;");
}

TEST_CASE("meta opens metadata only", "[ExtractorOpenSettings]")
{
	// MetaOnly replaces BodyAndMeta rather than adding to it --- the two are
	// values of the same low-bit selector, not flags.
	CHECK(Execution::ExtractorOpenSettings::forMetadataOnly(Configuration::SharedOptions()).flags == (metaOnly | formatText));
}
