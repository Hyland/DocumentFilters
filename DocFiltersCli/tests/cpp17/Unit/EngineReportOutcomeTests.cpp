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

// The ten structure reports, against what the engine gives them. Every one
// of these reads something only an opened, laid-out document has, so none of
// them could be tested before the engine had a seam. What is checked here is
// the shape of each record --- these tests are the authority on that --- and,
// as much, what each does with nothing: a document with no bookmarks, a page
// with no hyperlinks, an attribute the engine returns empty. A report that
// printed a header for an empty collection, or crashed on a null, would be
// wrong in a way no conversion test would notice.

#include "Configuration/CommandOptions.h"
#include "Execution/CatalogReports.h"
#include "Execution/DocumentFiltersSession.h"
#include "Execution/StructureReport.h"
#include "Fakes/FakeDocumentFiltersEngine.h"
#include "Fakes/RecordingConsole.h"

#include <catch2/catch_test_macros.hpp>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocument;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeDocumentFiltersEngine;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakePage;
using Hyland::DocumentFilters::Cli::Tests::Fakes::FakeSubFile;
using Hyland::DocumentFilters::Cli::Tests::Fakes::RecordingConsole;

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
			// EngineError::none is not a failure, and EngineError::cancelled is
			// not the engine's --- see EngineOutcomeTests::EveryEngineError.
		};
	}

	Engine::EngineRectF rectF(float x, float y, float width, float height) { return Engine::EngineRectF{ x, y, x + width, y + height }; }

}

// ============================================ document-level

TEST_CASE("fonts come from the last page attribute and split on semicolons", "[EngineReportOutcome]")
{
	// Fonts are not a collection in the binding: they arrive as the
	// semicolon-separated DOCUMENT_FONTS attribute of the last page, which is
	// the same route doc2text takes. That makes the splitting this report's
	// own job, and worth pinning.
	FakeDocument document("sample.docx");
	document.state()->pageCount = 2;
	document.state()->pages.push_back(std::make_shared<Tests::Fakes::FakePageState>());
	document.state()->pages.push_back(std::make_shared<Tests::Fakes::FakePageState>());
	document.state()->pages[1]->attributes["DOCUMENT_FONTS"] = "Calibri;Cambria Math";

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.fonts = true;
	Execution::StructureReport(options, console).reportDocument(document, "sample.docx");

	CHECK(console.outLines == std::vector<std::string>{ "sample.docx: font=Calibri", "sample.docx: font=Cambria Math" });
}

TEST_CASE("a document with no fonts prints nothing", "[EngineReportOutcome]")
{
	FakeDocument document("sample.docx");
	document.state()->pageCount = 1;

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.fonts = true;
	Execution::StructureReport(options, console).reportDocument(document, "sample.docx");

	CHECK(console.outLines.empty());
}

TEST_CASE("a document with no pages at all reports no fonts rather than failing", "[EngineReportOutcome]")
{
	// A text-only format has no laid-out pages, so there is no last page to
	// ask.
	FakeDocument document("plain.txt");
	document.state()->pageCount = 0;

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.fonts = true;
	Execution::StructureReport(options, console).reportDocument(document, "plain.txt");

	CHECK(console.outLines.empty());
}

TEST_CASE("bookmarks are walked depth first with their level and one based page", "[EngineReportOutcome]")
{
	FakeDocument document("sample.docx");
	// A bookmark's page number only exists once the document has been laid
	// out, so writeBookmarks skips a document with no pages at all.
	document.state()->pageCount = 4;

	Engine::EngineBookmark sampling;
	sampling.title = "Sampling";
	sampling.level = 1;
	sampling.pageIndex = 3;

	Engine::EngineBookmark methodology;
	methodology.title = "Methodology";
	methodology.level = 0;
	methodology.pageIndex = 2;
	methodology.children.push_back(sampling);

	Engine::EngineBookmark introduction;
	introduction.title = "Introduction";
	introduction.level = 0;
	introduction.pageIndex = 0;

	Engine::EngineBookmark root;
	root.title = "root";
	root.children.push_back(introduction);
	root.children.push_back(methodology);

	document.state()->bookmarks = root;

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.bookmarks = true;
	Execution::StructureReport(options, console).reportDocument(document, "sample.docx");

	CHECK(console.outLines
		== std::vector<std::string>{ "sample.docx: bookmark=1 level=0 page=1 title=Introduction", "sample.docx: bookmark=2 level=0 page=3 title=Methodology",
			  "sample.docx: bookmark=3 level=1 page=4 title=Sampling" });
}

TEST_CASE("a document with no outline prints no bookmarks", "[EngineReportOutcome]")
{
	// rootBookmark is absent for a document with no outline, which is most of
	// them. pageCount is set so this exercises that path specifically, rather
	// than the page-count guard below for the same empty result.
	FakeDocument document("plain.txt");
	document.state()->pageCount = 1;

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.bookmarks = true;
	Execution::StructureReport(options, console).reportDocument(document, "plain.txt");

	CHECK(console.outLines.empty());
}

TEST_CASE("bookmarks are skipped for a document with no pages", "[EngineReportOutcome]")
{
	// Reproduces a real bug: a bookmark's page number only exists once the
	// document has been laid out, so a handle opened without
	// IGR_FORMAT_IMAGE --- --to text --bookmarks, where the text reader needs
	// the format bit bookmarks would otherwise take --- was never laid out at
	// all. rootBookmark does not degrade to "nothing" on its own for that
	// case the way getLastPageAttribute does; it throws the binding's own
	// "Invalid DocHandle". This guard applies the same "reports nothing
	// rather than failing" rule the binding does not.
	FakeDocument document("sample.docx");
	document.state()->pageCount = 0;

	Engine::EngineBookmark introduction;
	introduction.title = "Introduction";
	introduction.level = 0;
	introduction.pageIndex = 0;

	Engine::EngineBookmark root;
	root.title = "root";
	root.children.push_back(introduction);
	document.state()->bookmarks = root;

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.bookmarks = true;
	Execution::StructureReport(options, console).reportDocument(document, "sample.docx");

	CHECK(console.outLines.empty());
}

TEST_CASE("annotations are reported with their page kind author and text", "[EngineReportOutcome]")
{
	FakeDocument document("sample.docx");
	document.state()->pageCount = 2;
	document.state()->pages.push_back(std::make_shared<Tests::Fakes::FakePageState>());
	document.state()->pages.push_back(std::make_shared<Tests::Fakes::FakePageState>());

	Engine::EngineAnnotation annotation;
	annotation.type = Hyland::DocFilters::Annotation::Type::Text;
	annotation.title = "Jane Smith";
	annotation.text = "Check this figure";
	document.state()->pages[1]->annotations.push_back(annotation);

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.annotations = true;
	Execution::StructureReport(options, console).reportDocument(document, "sample.docx");

	REQUIRE(console.outLines.size() == 1);
	const std::string& line = console.outLines[0];
	CHECK(line.find("annotation=1") != std::string::npos);
	CHECK(line.find("page=2") != std::string::npos);
	CHECK(line.find("author=Jane Smith") != std::string::npos);
	CHECK(line.find("text=Check this figure") != std::string::npos);
}

// ============================================ page-level

TEST_CASE("dimensions come from the page itself", "[EngineReportOutcome]")
{
	RecordingConsole console;
	FakePage page;
	page.state()->width = 612;
	page.state()->height = 792;

	Configuration::StructureReportOptions options;
	options.dimensions = true;
	Execution::StructureReport(options, console).reportPage(page, 1, "sample.docx");

	REQUIRE(console.outLines.size() == 1);
	CHECK(console.outLines[0] == "sample.docx: page=1 width=612 height=792");
}

TEST_CASE("hyperlinks report a uri target or a ref target but never both", "[EngineReportOutcome]")
{
	FakePage page;

	Engine::EngineHyperlink uriLink;
	uriLink.type = Hyland::DocFilters::Hyperlink::Type::Uri;
	uriLink.uri = "https://example.com";
	uriLink.ref = "ignored";
	uriLink.x = 100;
	uriLink.y = 200;
	uriLink.width = 80;
	uriLink.height = 12;
	page.state()->hyperlinks.push_back(uriLink);

	Engine::EngineHyperlink goToLink;
	goToLink.type = Hyland::DocFilters::Hyperlink::Type::GoTo;
	goToLink.uri = "ignored";
	goToLink.ref = "page-3";
	goToLink.x = 1;
	goToLink.y = 2;
	goToLink.width = 3;
	goToLink.height = 4;
	page.state()->hyperlinks.push_back(goToLink);

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.hyperlinks = true;
	Execution::StructureReport(options, console).reportPage(page, 1, "sample.docx");

	REQUIRE(console.outLines.size() == 2);
	CHECK(console.outLines[0].find("uri=https://example.com") != std::string::npos);
	CHECK(console.outLines[0].find("ref=") == std::string::npos);
	CHECK(console.outLines[1].find("ref=page-3") != std::string::npos);
	CHECK(console.outLines[1].find("uri=") == std::string::npos);
}

TEST_CASE("form elements report their geometry and name", "[EngineReportOutcome]")
{
	FakePage page;

	Engine::EngineFormElement element;
	element.type = Hyland::DocFilters::FormElement::Type::SinglelineText;
	element.name = "surname";
	element.x = 72;
	element.y = 144;
	element.width = 200;
	element.height = 20;
	page.state()->formElements.push_back(element);

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.formElements = true;
	Execution::StructureReport(options, console).reportPage(page, 1, "sample.docx");

	REQUIRE(console.outLines.size() == 1);
	const std::string& line = console.outLines[0];
	CHECK(line.find("form-element=1") != std::string::npos);
	CHECK(line.find("x=72 y=144 w=200 h=20") != std::string::npos);
	CHECK(line.find("name=surname") != std::string::npos);
}

TEST_CASE("page elements are flattened depth first with their depth", "[EngineReportOutcome]")
{
	// The element tree is reported flattened, depth first, with each node's
	// own depth --- so a nested node appears once, after its parent, not
	// inside it.
	Engine::EnginePageElement run;
	run.type = Hyland::DocFilters::PageElementType::Run;
	run.depth = 2;
	run.bounds = rectF(72, 72, 100, 14);

	Engine::EnginePageElement paragraph;
	paragraph.type = Hyland::DocFilters::PageElementType::Paragraph;
	paragraph.depth = 1;
	paragraph.bounds = rectF(72, 72, 468, 14);
	paragraph.children.push_back(run);

	Engine::EnginePageElement root;
	root.depth = 0;
	root.bounds = rectF(0, 0, 612, 792);
	root.children.push_back(paragraph);

	FakePage page;
	page.state()->rootElement = root;

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	options.pageElements = true;
	Execution::StructureReport(options, console).reportPage(page, 1, "sample.docx");

	REQUIRE(console.outLines.size() == 2);
	CHECK(console.outLines[0].find("element=1") != std::string::npos);
	CHECK(console.outLines[0].find("depth=1") != std::string::npos);
	CHECK(console.outLines[0].find("x=72 y=72 w=468 h=14") != std::string::npos);
	CHECK(console.outLines[1].find("element=2") != std::string::npos);
	CHECK(console.outLines[1].find("depth=2") != std::string::npos);
}

TEST_CASE("a page that exposes no element tree prints nothing", "[EngineReportOutcome]")
{
	// getRootPageElement returns absent for a format with no layout tree.
	RecordingConsole console;
	FakePage page;

	Configuration::StructureReportOptions options;
	options.pageElements = true;
	Execution::StructureReport(options, console).reportPage(page, 1, "sample.docx");

	CHECK(console.outLines.empty());
}

TEST_CASE("an empty page prints nothing for any page report", "[EngineReportOutcome]")
{
	RecordingConsole console;
	FakePage page;

	Configuration::StructureReportOptions options;
	options.pageFonts = true;
	options.pageFontMappings = true;
	options.pageElements = true;
	options.formElements = true;
	options.hyperlinks = true;
	Execution::StructureReport(options, console).reportPage(page, 1, "sample.docx");

	CHECK(console.outLines.empty());
}

TEST_CASE("no report requested reads nothing from the document", "[EngineReportOutcome]")
{
	// The reports are opt-in, so a plain conversion must not pay for them ---
	// asking a page for its hyperlinks is work the engine has to do.
	FakeDocument document("sample.docx");
	document.state()->readFailure = Engine::EngineError::actionNotSupported;

	RecordingConsole console;
	Configuration::StructureReportOptions options;
	Execution::StructureReport(options, console).reportDocument(document, "sample.docx");

	CHECK(console.outLines.empty());
}

// ============================================ the catalogues
//
// The C# reference has a Catalog(engine, console) helper here and calls it from
// every catalogue test. It has no C++ counterpart and must not: CatalogReports
// holds DocumentFiltersSession&, so a helper that builds the session inside
// itself and returns the report by value leaves that reference dangling the
// moment it returns. C# is safe because the session is a managed object the
// helper's return value keeps alive; a C++ reference member keeps nothing alive.
// Each test below therefore names its own session, which outlives the report
// built from it. A helper in the C# shape did exist here, unused -- MSVC accepts
// binding a temporary to a non-const reference, so it compiled; GCC rejects it,
// which is what surfaced it.

TEST_CASE("formats lists what the engine reports", "[EngineReportOutcome]")
{
	FakeDocumentFiltersEngine engine;
	engine.formats.push_back(Engine::EngineFormat{ 254, "docx", "Microsoft Word 2007" });
	engine.formats.push_back(Engine::EngineFormat{ 112, "doc", "Microsoft Word for Windows" });

	RecordingConsole console;
	Execution::DocumentFiltersSession session = Execution::DocumentFiltersSession::forEngine(engine);
	Execution::CatalogReports(session, console).writeFormats(Configuration::FormatsOptions());

	CHECK(console.outLines == std::vector<std::string>{ "id=254 ext=docx name=Microsoft Word 2007", "id=112 ext=doc name=Microsoft Word for Windows" });
}

TEST_CASE("the formats filter matches on name and short name", "[EngineReportOutcome]")
{
	FakeDocumentFiltersEngine engine;
	engine.formats.push_back(Engine::EngineFormat{ 254, "docx", "Microsoft Word 2007" });
	engine.formats.push_back(Engine::EngineFormat{ 200, "pdf", "Adobe Portable Document Format" });

	RecordingConsole console;
	Execution::DocumentFiltersSession session = Execution::DocumentFiltersSession::forEngine(engine);
	Configuration::FormatsOptions options;
	options.filter = "word";
	Execution::CatalogReports(session, console).writeFormats(options);

	REQUIRE(console.outLines.size() == 1);
	CHECK(console.outLines[0] == "id=254 ext=docx name=Microsoft Word 2007");
}

TEST_CASE("a filter matching nothing prints nothing rather than failing", "[EngineReportOutcome]")
{
	FakeDocumentFiltersEngine engine;
	engine.formats.push_back(Engine::EngineFormat{ 254, "docx", "Microsoft Word 2007" });

	RecordingConsole console;
	Execution::DocumentFiltersSession session = Execution::DocumentFiltersSession::forEngine(engine);
	Configuration::FormatsOptions options;
	options.filter = "nothing-matches-this";
	Execution::CatalogReports(session, console).writeFormats(options);

	CHECK(console.outLines.empty());
}

TEST_CASE("a build that reports no formats prints nothing", "[EngineReportOutcome]")
{
	FakeDocumentFiltersEngine engine;
	RecordingConsole console;
	Execution::DocumentFiltersSession session = Execution::DocumentFiltersSession::forEngine(engine);
	Execution::CatalogReports(session, console).writeFormats(Configuration::FormatsOptions());

	CHECK(console.outLines.empty());
}

TEST_CASE("options lists what the engine reports", "[EngineReportOutcome]")
{
	FakeDocumentFiltersEngine engine;
	Engine::EngineOption option;
	option.displayName = "BW_MODE";
	option.type = "int";
	option.defaultValue = "0";
	option.description = "Black and white rendering mode";
	engine.options.push_back(option);

	RecordingConsole console;
	Execution::DocumentFiltersSession session = Execution::DocumentFiltersSession::forEngine(engine);
	Execution::CatalogReports(session, console).writeOptions(Configuration::OptionCatalogOptions());

	REQUIRE(console.outLines.size() == 1);
	const std::string& line = console.outLines[0];
	CHECK(line.find("name=BW_MODE") != std::string::npos);
	CHECK(line.find("type=int") != std::string::npos);
	CHECK(line.find("default=0") != std::string::npos);
	CHECK(line.find("description=Black and white rendering mode") != std::string::npos);
}

TEST_CASE("an options filter matching nothing prints nothing", "[EngineReportOutcome]")
{
	FakeDocumentFiltersEngine engine;
	Engine::EngineOption option;
	option.displayName = "BW_MODE";
	option.type = "int";
	option.description = "mode";
	engine.options.push_back(option);

	RecordingConsole console;
	Execution::DocumentFiltersSession session = Execution::DocumentFiltersSession::forEngine(engine);
	Configuration::OptionCatalogOptions options;
	options.filter = "nothing-matches-this";
	Execution::CatalogReports(session, console).writeOptions(options);

	CHECK(console.outLines.empty());
}

TEST_CASE("a catalogue that cannot be read surfaces the engine error", "[EngineReportOutcome]")
{
	// The catalogues are the two commands that read no document, so a
	// failure in either is a failure of the run rather than of an input ---
	// there is no input to blame. What matters is that the engine's error
	// reaches the caller intact.
	for (Engine::EngineError error : everyEngineError())
	{
		FakeDocumentFiltersEngine engine;
		engine.catalogFailure = error;
		Execution::DocumentFiltersSession session = Execution::DocumentFiltersSession::forEngine(engine);

		RecordingConsole consoleForFormats;
		bool formatsThrew = false;
		try
		{
			Execution::CatalogReports(session, consoleForFormats).writeFormats(Configuration::FormatsOptions());
		}
		catch (const Engine::EngineException& thrown)
		{
			formatsThrew = true;
			CHECK(thrown.error() == error);
		}
		CHECK(formatsThrew);

		RecordingConsole consoleForOptions;
		bool optionsThrew = false;
		try
		{
			Execution::CatalogReports(session, consoleForOptions).writeOptions(Configuration::OptionCatalogOptions());
		}
		catch (const Engine::EngineException& thrown)
		{
			optionsThrew = true;
			CHECK(thrown.error() == error);
		}
		CHECK(optionsThrew);
	}
}
