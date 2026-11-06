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

package com.hyland.documentfilters.cli.tests.unit;

import static com.hyland.documentfilters.cli.tests.Assert.assertContains;
import static com.hyland.documentfilters.cli.tests.Assert.assertDoesNotContain;
import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.configuration.FormatsOptions;
import com.hyland.documentfilters.cli.configuration.OptionCatalogOptions;
import com.hyland.documentfilters.cli.configuration.StructureReportOptions;
import com.hyland.documentfilters.cli.engine.AnnotationKind;
import com.hyland.documentfilters.cli.engine.EngineAnnotation;
import com.hyland.documentfilters.cli.engine.EngineBookmark;
import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.engine.EngineException;
import com.hyland.documentfilters.cli.engine.EngineFormElement;
import com.hyland.documentfilters.cli.engine.EngineFormat;
import com.hyland.documentfilters.cli.engine.EngineHyperlink;
import com.hyland.documentfilters.cli.engine.EngineOption;
import com.hyland.documentfilters.cli.engine.EnginePageElement;
import com.hyland.documentfilters.cli.engine.EngineRectangle;
import com.hyland.documentfilters.cli.engine.FormElementType;
import com.hyland.documentfilters.cli.engine.HyperlinkActionType;
import com.hyland.documentfilters.cli.engine.PageElementType;
import com.hyland.documentfilters.cli.execution.CatalogReports;
import com.hyland.documentfilters.cli.execution.DocumentFiltersSession;
import com.hyland.documentfilters.cli.execution.StructureReport;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocument;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocumentFiltersEngine;
import com.hyland.documentfilters.cli.tests.fakes.FakePage;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.util.Arrays;
import java.util.Collections;
import java.util.function.Consumer;

/**
 * The structure reports and the catalogues, against every shape the engine can
 * hand them.
 *
 * <p>Each report is opt-in and each reads something the engine may simply not
 * have --- no fonts, no outline, no layout tree --- so the property checked
 * throughout is that an absence prints nothing rather than failing the document.
 */
public final class EngineReportOutcomeTests
{
	private static StructureReport report(RecordingConsole console, Consumer<StructureReportOptions> configure)
	{
		StructureReportOptions options = new StructureReportOptions();
		configure.accept(options);
		return new StructureReport(options, console);
	}

	private static EngineBookmark bookmark(String title, int level, int pageIndex, EngineBookmark... children)
	{
		EngineBookmark bookmark = new EngineBookmark();
		bookmark.title = title;
		bookmark.level = level;
		bookmark.pageIndex = pageIndex;
		bookmark.children.addAll(Arrays.asList(children));
		return bookmark;
	}

	private static EnginePageElement element(PageElementType type, int depth, EngineRectangle bounds, EnginePageElement... children)
	{
		EnginePageElement element = new EnginePageElement();
		element.type = type;
		element.depth = depth;
		element.bounds = bounds;
		element.children.addAll(Arrays.asList(children));
		return element;
	}

	private static EngineFormat format(int id, String shortName, String displayName)
	{
		EngineFormat format = new EngineFormat();
		format.id = id;
		format.shortName = shortName;
		format.displayName = displayName;
		return format;
	}

	private static String single(java.util.List<String> lines)
	{
		assertEquals(1, lines.size());
		return lines.get(0);
	}

	// ============================================ document-level

	@Test
	public void fontsComeFromTheLastPageAttributeAndSplitOnSemicolons()
	{
		FakeDocument document = new FakeDocument("sample.docx");
		document.pageCount = 2;
		document.pages.add(new FakePage());
		document.pages.add(new FakePage());
		document.pages.get(1).attributes.put("DOCUMENT_FONTS", "Calibri;Cambria Math");

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.fonts = true).reportDocument(document, "sample.docx");

		assertEquals(Arrays.asList("sample.docx: font=Calibri", "sample.docx: font=Cambria Math"), console.outLines);
	}

	@Test
	public void aDocumentWithNoFontsPrintsNothing()
	{
		FakeDocument document = new FakeDocument("sample.docx");
		document.pageCount = 1;

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.fonts = true).reportDocument(document, "sample.docx");

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void aDocumentWithNoPagesAtAllReportsNoFontsRatherThanFailing()
	{
		// A text-only format has no laid-out pages, so there is no last page to ask.
		FakeDocument document = new FakeDocument("plain.txt");
		document.pageCount = 0;

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.fonts = true).reportDocument(document, "plain.txt");

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void bookmarksAreWalkedDepthFirstWithTheirLevelAndOneBasedPage()
	{
		FakeDocument document = new FakeDocument("sample.docx");
		// A bookmark's page number only exists once the document has been laid out,
		// so writeBookmarks skips a document with no pages at all --- see
		// bookmarksAreSkippedForADocumentWithNoPages below.
		document.pageCount = 4;
		document.bookmarks = bookmark("root", 0, 0,
			bookmark("Introduction", 0, 0),
			bookmark("Methodology", 0, 2,
				bookmark("Sampling", 1, 3)));

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.bookmarks = true).reportDocument(document, "sample.docx");

		assertEquals(Arrays.asList(
			"sample.docx: bookmark=1 level=0 page=1 title=Introduction",
			"sample.docx: bookmark=2 level=0 page=3 title=Methodology",
			"sample.docx: bookmark=3 level=1 page=4 title=Sampling"), console.outLines);
	}

	@Test
	public void aDocumentWithNoOutlinePrintsNoBookmarks()
	{
		// The root bookmark is null for a document with no outline, which is most of
		// them. The page count is set so this exercises that path specifically,
		// rather than the page-count guard below for the same empty result.
		FakeDocument document = new FakeDocument("plain.txt");
		document.pageCount = 1;

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.bookmarks = true).reportDocument(document, "plain.txt");

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void bookmarksAreSkippedForADocumentWithNoPages()
	{
		// Reproduces a real bug: a bookmark's page number only exists once the
		// document has been laid out, so a handle opened without IGR_FORMAT_IMAGE
		// --- --to text --bookmarks, where the text reader needs the format bit
		// bookmarks would otherwise take --- was never laid out at all. The root
		// bookmark does not degrade to "nothing" on its own for that case the way
		// the last page attribute does; it throws the binding's own "Invalid
		// DocHandle". This guard applies the same "reports nothing rather than
		// failing" rule the binding does not.
		FakeDocument document = new FakeDocument("sample.docx");
		document.pageCount = 0;
		document.bookmarks = bookmark("root", 0, 0, bookmark("Introduction", 0, 0));

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.bookmarks = true).reportDocument(document, "sample.docx");

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void annotationsAreReportedWithTheirPageKindAuthorAndText()
	{
		FakeDocument document = new FakeDocument("sample.docx");
		document.pageCount = 2;
		document.pages.add(new FakePage());
		document.pages.add(new FakePage());
		EngineAnnotation annotation = new EngineAnnotation();
		annotation.type = AnnotationKind.TEXT;
		annotation.title = "Jane Smith";
		annotation.text = "Check this figure";
		document.pages.get(1).annotationList.add(annotation);

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.annotations = true).reportDocument(document, "sample.docx");

		String line = single(console.outLines);
		assertContains("annotation=1", line);
		assertContains("page=2", line);
		assertContains("author=Jane Smith", line);
		assertContains("text=Check this figure", line);
	}

	// ============================================ page-level

	@Test
	public void dimensionsComeFromThePageItself()
	{
		RecordingConsole console = new RecordingConsole();
		FakePage page = new FakePage();
		page.width = 612;
		page.height = 792;

		report(console, o -> o.dimensions = true).reportPage(page, 1, "sample.docx");

		assertEquals("sample.docx: page=1 width=612 height=792", single(console.outLines));
	}

	@Test
	public void hyperlinksReportAUriTargetOrARefTargetButNeverBoth()
	{
		FakePage page = new FakePage();
		EngineHyperlink uri = new EngineHyperlink();
		uri.type = HyperlinkActionType.URI;
		uri.uri = "https://example.com";
		uri.ref = "ignored";
		uri.x = 100;
		uri.y = 200;
		uri.width = 80;
		uri.height = 12;
		page.hyperlinkList.add(uri);
		EngineHyperlink goTo = new EngineHyperlink();
		goTo.type = HyperlinkActionType.GOTO;
		goTo.uri = "ignored";
		goTo.ref = "page-3";
		goTo.x = 1;
		goTo.y = 2;
		goTo.width = 3;
		goTo.height = 4;
		page.hyperlinkList.add(goTo);

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.hyperlinks = true).reportPage(page, 1, "sample.docx");

		assertContains("uri=https://example.com", console.outLines.get(0));
		assertDoesNotContain("ref=", console.outLines.get(0));
		assertContains("ref=page-3", console.outLines.get(1));
		assertDoesNotContain("uri=", console.outLines.get(1));
	}

	@Test
	public void formElementsReportTheirGeometryAndName()
	{
		FakePage page = new FakePage();
		EngineFormElement field = new EngineFormElement();
		field.type = FormElementType.SINGLELINE_TEXT;
		field.name = "surname";
		field.x = 72;
		field.y = 144;
		field.width = 200;
		field.height = 20;
		page.formElementList.add(field);

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.formElements = true).reportPage(page, 1, "sample.docx");

		String line = single(console.outLines);
		assertContains("form-element=1", line);
		assertContains("x=72 y=144 w=200 h=20", line);
		assertContains("name=surname", line);
	}

	@Test
	public void pageElementsAreFlattenedDepthFirstWithTheirDepth()
	{
		FakePage page = new FakePage();
		page.rootElement = element(PageElementType.PAGE, 0, new EngineRectangle(0, 0, 612, 792),
			element(PageElementType.PARAGRAPH, 1, new EngineRectangle(72, 72, 468, 14),
				element(PageElementType.RUN, 2, new EngineRectangle(72, 72, 100, 14))));

		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.pageElements = true).reportPage(page, 1, "sample.docx");

		assertEquals(2, console.outLines.size());
		assertContains("element=1", console.outLines.get(0));
		assertContains("depth=1", console.outLines.get(0));
		assertContains("x=72 y=72 w=468 h=14", console.outLines.get(0));
		assertContains("element=2", console.outLines.get(1));
		assertContains("depth=2", console.outLines.get(1));
	}

	@Test
	public void aPageThatExposesNoElementTreePrintsNothing()
	{
		// The root page element is null for a format with no layout tree.
		RecordingConsole console = new RecordingConsole();
		report(console, o -> o.pageElements = true).reportPage(new FakePage(), 1, "sample.docx");

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void anEmptyPagePrintsNothingForAnyPageReport()
	{
		RecordingConsole console = new RecordingConsole();
		FakePage page = new FakePage();

		report(console, o ->
		{
			o.pageFonts = true;
			o.pageFontMappings = true;
			o.pageElements = true;
			o.formElements = true;
			o.hyperlinks = true;
		}).reportPage(page, 1, "sample.docx");

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void noReportRequestedReadsNothingFromTheDocument()
	{
		// The reports are opt-in, so a plain conversion must not pay for them ---
		// asking a page for its hyperlinks is work the engine has to do.
		FakeDocument document = new FakeDocument("sample.docx");
		document.readFailure = EngineError.ACTION_NOT_SUPPORTED;

		RecordingConsole console = new RecordingConsole();
		report(console, o ->
		{
		}).reportDocument(document, "sample.docx");

		assertEquals(Collections.emptyList(), console.outLines);
	}

	// ============================================ the catalogues

	private static CatalogReports catalog(FakeDocumentFiltersEngine engine, RecordingConsole console)
	{
		return new CatalogReports(DocumentFiltersSession.forEngine(engine), console);
	}

	@Test
	public void formatsListsWhatTheEngineReports()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.formats.add(format(254, "docx", "Microsoft Word 2007"));
		engine.formats.add(format(112, "doc", "Microsoft Word for Windows"));

		RecordingConsole console = new RecordingConsole();
		catalog(engine, console).writeFormats(new FormatsOptions());

		assertEquals(Arrays.asList(
			"id=254 ext=docx name=Microsoft Word 2007",
			"id=112 ext=doc name=Microsoft Word for Windows"), console.outLines);
	}

	@Test
	public void theFormatsFilterMatchesOnNameAndShortName()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.formats.add(format(254, "docx", "Microsoft Word 2007"));
		engine.formats.add(format(200, "pdf", "Adobe Portable Document Format"));

		RecordingConsole console = new RecordingConsole();
		FormatsOptions options = new FormatsOptions();
		options.filter = "word";
		catalog(engine, console).writeFormats(options);

		assertEquals("id=254 ext=docx name=Microsoft Word 2007", single(console.outLines));
	}

	@Test
	public void aFilterMatchingNothingPrintsNothingRatherThanFailing()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.formats.add(format(254, "docx", "Microsoft Word 2007"));

		RecordingConsole console = new RecordingConsole();
		FormatsOptions options = new FormatsOptions();
		options.filter = "nothing-matches-this";
		catalog(engine, console).writeFormats(options);

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void aBuildThatReportsNoFormatsPrintsNothing()
	{
		RecordingConsole console = new RecordingConsole();
		catalog(new FakeDocumentFiltersEngine(), console).writeFormats(new FormatsOptions());

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void optionsListsWhatTheEngineReports()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		EngineOption option = new EngineOption();
		option.displayName = "BW_MODE";
		option.type = "int";
		option.defaultValue = "0";
		option.description = "Black and white rendering mode";
		engine.options.add(option);

		RecordingConsole console = new RecordingConsole();
		catalog(engine, console).writeOptions(new OptionCatalogOptions());

		String line = single(console.outLines);
		assertContains("name=BW_MODE", line);
		assertContains("type=int", line);
		assertContains("default=0", line);
		assertContains("description=Black and white rendering mode", line);
	}

	@Test
	public void anOptionsFilterMatchingNothingPrintsNothing()
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		EngineOption option = new EngineOption();
		option.displayName = "BW_MODE";
		option.type = "int";
		option.description = "mode";
		engine.options.add(option);

		RecordingConsole console = new RecordingConsole();
		OptionCatalogOptions options = new OptionCatalogOptions();
		options.filter = "nothing-matches-this";
		catalog(engine, console).writeOptions(options);

		assertEquals(Collections.emptyList(), console.outLines);
	}

	@Test
	public void aCatalogueThatCannotBeReadSurfacesTheEngineError()
	{
		for (EngineError error : EngineOutcomeTests.everyEngineError())
		{
			FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
			engine.catalogFailure = error;

			EngineException fromFormats = assertThrows(EngineException.class,
				() -> catalog(engine, new RecordingConsole()).writeFormats(new FormatsOptions()));

			EngineException fromOptions = assertThrows(EngineException.class,
				() -> catalog(engine, new RecordingConsole()).writeOptions(new OptionCatalogOptions()));

			assertEquals(error, fromFormats.getError());
			assertEquals(error, fromOptions.getError());
		}
	}
}
