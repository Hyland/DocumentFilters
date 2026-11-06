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

package com.hyland.documentfilters.cli.execution;

import com.hyland.documentfilters.cli.configuration.StructureReportOptions;
import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.engine.EngineAnnotation;
import com.hyland.documentfilters.cli.engine.EngineBookmark;
import com.hyland.documentfilters.cli.engine.EngineFormElement;
import com.hyland.documentfilters.cli.engine.EngineHyperlink;
import com.hyland.documentfilters.cli.engine.EnginePageElement;
import com.hyland.documentfilters.cli.engine.EngineRectangle;
import com.hyland.documentfilters.cli.engine.FormElementType;
import com.hyland.documentfilters.cli.engine.HyperlinkActionType;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.IDocumentPage;
import com.hyland.documentfilters.cli.runtime.IConsole;
import java.util.ArrayList;
import java.util.List;

/**
 * The document and page structure aspects {@code convert} reports while it
 * works: fonts, dimensions, page elements, form elements, hyperlinks, bookmarks
 * and annotations.
 *
 * <p>These are emitted during a conversion run rather than by a separate command,
 * which matters most for high-definition output, where the structure is only known
 * once the page has been laid out.
 *
 * <p>Output is minimal and line-oriented, one fact per line, and is compared as
 * golden files by the conformance suite.
 */
public final class StructureReport
{
	private final StructureReportOptions _options;
	private final IConsole _console;

	public StructureReport(StructureReportOptions options, IConsole console)
	{
		_options = options;
		_console = console;
	}

	/** Reports the document-level aspects: fonts, font mappings, bookmarks, annotations. */
	public void reportDocument(IDocument document, String displayName)
	{
		if (_options.fonts)
			writeFonts(document, displayName);

		if (_options.fontMappings)
			writeFontMappings(document, displayName);

		if (_options.bookmarks)
			writeBookmarks(document, displayName);

		if (_options.annotations)
			writeAnnotations(document, displayName);
	}

	/** Reports the page-level aspects for one page. */
	public void reportPage(IDocumentPage page, int pageNumber, String displayName)
	{
		if (_options.dimensions)
			writeDimensions(page, pageNumber, displayName);

		if (_options.pageFonts)
			writePageFonts(page, pageNumber, displayName);

		if (_options.pageFontMappings)
			writePageFontMappings(page, pageNumber, displayName);

		if (_options.pageElements)
			writePageElements(page, pageNumber, displayName);

		if (_options.formElements)
			writeFormElements(page, pageNumber, displayName);

		if (_options.hyperlinks)
			writeHyperlinks(page, pageNumber, displayName);
	}

	/**
	 * Fonts are not a first-class collection anywhere in the binding; doc2text
	 * reads them the same way, as the semicolon-separated {@code DOCUMENT_FONTS}
	 * page attribute, queried on the last page once every page has been laid out.
	 */
	public void writeFonts(IDocument document, String displayName)
	{
		writeFontList(getLastPageAttribute(document, "DOCUMENT_FONTS"), displayName, null);
	}

	/** Same source as {@link #writeFonts}, via {@code DOCUMENT_FONT_MAPPINGS}. */
	public void writeFontMappings(IDocument document, String displayName)
	{
		writeFontMappingList(getLastPageAttribute(document, "DOCUMENT_FONT_MAPPINGS"), displayName, null);
	}

	/**
	 * Guarded on the page count: a bookmark's page number only exists once the
	 * document has been laid out into pages, and a handle opened without
	 * {@code IGR_FORMAT_IMAGE} --- {@code --to text --bookmarks} --- was never laid
	 * out at all, where asking for the root bookmark fails with an invalid handle
	 * rather than answering nothing.
	 */
	public void writeBookmarks(IDocument document, String displayName)
	{
		if (document.getPageCount() == 0)
			return;

		EngineBookmark root = document.getRootBookmark();
		if (root == null)
			return;

		int[] index = { 0 };
		for (EngineBookmark child : root.children)
			writeBookmarksRecursive(child, displayName, index);
	}

	private void writeBookmarksRecursive(EngineBookmark bookmark, String displayName, int[] index)
	{
		index[0]++;
		writeRecord(displayName, "bookmark=" + index[0] + " level=" + bookmark.level + " page=" + (bookmark.pageIndex + 1)
			+ " title=" + text(bookmark.title));

		for (EngineBookmark child : bookmark.children)
			writeBookmarksRecursive(child, displayName, index);
	}

	/**
	 * A document-level report even though every annotation belongs to a page:
	 * numbering runs across the whole document, so it has to be gathered in one
	 * pass rather than reset per {@link #reportPage} call.
	 */
	public void writeAnnotations(IDocument document, String displayName)
	{
		int index = 0;
		int pageCount = document.getPageCount();
		for (int pageIndex = 0; pageIndex < pageCount; pageIndex++)
		{
			try (IDocumentPage page = document.getPage(pageIndex))
			{
				for (EngineAnnotation annotation : page.getAnnotations())
				{
					index++;
					writeRecord(displayName,
						"annotation=" + index + " page=" + (pageIndex + 1) + " kind=" + kindOf(annotation.type)
						+ " author=" + text(annotation.title) + " text=" + text(annotation.text));
				}
			}
		}
	}

	public void writeDimensions(IDocumentPage page, int pageNumber, String displayName)
	{
		writeRecord(displayName, "page=" + pageNumber + " width=" + page.getWidth() + " height=" + page.getHeight());
	}

	public void writePageFonts(IDocumentPage page, int pageNumber, String displayName)
	{
		writeFontList(page.getAttribute("FONTS"), displayName, pageNumber);
	}

	/** {@code --page-font-mappings}: how each page's fonts were substituted. */
	public void writePageFontMappings(IDocumentPage page, int pageNumber, String displayName)
	{
		writeFontMappingList(page.getAttribute("FONT_MAPPINGS"), displayName, pageNumber);
	}

	public void writePageElements(IDocumentPage page, int pageNumber, String displayName)
	{
		EnginePageElement root = page.getRootPageElement();
		if (root == null)
			return;

		int index = 0;
		for (EnginePageElement element : root.allChildren())
		{
			index++;
			EngineRectangle bounds = element.bounds;
			// The cast truncates toward zero, as the reference's does.
			writeRecord(displayName,
				"page=" + pageNumber + " element=" + index + " kind=" + kindOf(element.type) + " depth=" + element.depth
				+ " x=" + (int) bounds.x + " y=" + (int) bounds.y + " w=" + (int) bounds.width + " h=" + (int) bounds.height);
		}
	}

	public void writeFormElements(IDocumentPage page, int pageNumber, String displayName)
	{
		int index = 0;
		for (EngineFormElement element : page.getFormElements())
		{
			index++;
			writeRecord(displayName,
				"page=" + pageNumber + " form-element=" + index + " kind=" + formElementKind(element.type)
				+ " x=" + element.x + " y=" + element.y + " w=" + element.width + " h=" + element.height
				+ " name=" + text(element.name));
		}
	}

	public void writeHyperlinks(IDocumentPage page, int pageNumber, String displayName)
	{
		int index = 0;
		for (EngineHyperlink link : page.getHyperlinks())
		{
			index++;
			String target = link.type == HyperlinkActionType.URI ? "uri=" + text(link.uri) : "ref=" + text(link.ref);
			writeRecord(displayName,
				"page=" + pageNumber + " hyperlink=" + index + " kind=" + kindOf(link.type)
				+ " x=" + link.x + " y=" + link.y + " w=" + link.width + " h=" + link.height + " " + target);
		}
	}

	private void writeRecord(String displayName, String fields)
	{
		_console.writeLine(displayName + ": " + fields);
	}

	private void writeFontList(String attributeValue, String displayName, Integer pageNumber)
	{
		for (String font : splitAttribute(attributeValue))
		{
			String fields = (pageNumber != null ? "page=" + pageNumber + " " : "") + "font=" + font;
			writeRecord(displayName, fields);
		}
	}

	/** Each entry is {@code original=mapped}, as the engine builds the attribute text. */
	private void writeFontMappingList(String attributeValue, String displayName, Integer pageNumber)
	{
		for (String entry : splitAttribute(attributeValue))
		{
			int split = entry.indexOf('=');
			String original = split < 0 ? entry : entry.substring(0, split);
			String mapped = split < 0 ? entry : entry.substring(split + 1);

			String fields = (pageNumber != null ? "page=" + pageNumber + " " : "") + "font=" + original + " mapped=" + mapped;
			writeRecord(displayName, fields);
		}
	}

	private static List<String> splitAttribute(String value)
	{
		List<String> entries = new ArrayList<>();
		if (value == null || value.isEmpty())
			return entries;

		for (String entry : value.split(";", -1))
		{
			if (!entry.isEmpty())
				entries.add(entry);
		}
		return entries;
	}

	private static String getLastPageAttribute(IDocument document, String attributeName)
	{
		int pageCount = document.getPageCount();
		if (pageCount == 0)
			return "";

		try (IDocumentPage page = document.getPage(pageCount - 1))
		{
			String value = page.getAttribute(attributeName);
			return value != null ? value : "";
		}
	}

	/**
	 * The value's name as the reference prints it: its enum name lowercased. The
	 * Java names separate words with underscores where the reference's run them
	 * together, so those are dropped.
	 */
	static String kindOf(Enum<?> value)
	{
		return ValueParsers.foldToken(value.name().replace("_", ""));
	}

	/**
	 * Short, CLI-facing labels rather than the binding's own enum names --- only
	 * single-line text to {@code text} is confirmed against a real fixture; the rest
	 * follow the same short-label convention.
	 */
	static String formElementKind(FormElementType type)
	{
		switch (type)
		{
			case SINGLELINE_TEXT:
				return "text";
			case MULTILINE_TEXT:
				return "multiline";
			case CHECKBOX:
				return "checkbox";
			case RADIOBUTTON:
				return "radio";
			case COMBOBOX:
				return "combo";
			case LISTBOX:
				return "list";
			case FILESELECT:
				return "file";
			case SIGNATURE:
				return "signature";
			case PASSWORD:
				return "password";
			case BUTTON:
				return "button";
			default:
				return kindOf(type);
		}
	}

	/** {@code value}, or empty for null, as the reference's string interpolation renders it. */
	private static String text(String value)
	{
		return value != null ? value : "";
	}
}
