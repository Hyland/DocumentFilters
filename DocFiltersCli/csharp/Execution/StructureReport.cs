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

using Hyland.DocumentFilters.Annotations;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Runtime;

using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// The document and page structure aspects <c>convert</c> reports while it
	/// works: fonts, dimensions, page elements, form elements, hyperlinks, bookmarks
	/// and annotations.
	/// </summary>
	/// <remarks>
	/// These are emitted during a conversion run rather than by a separate command,
	/// matters most for high-definition output, where the structure is only known
	/// once the page has been laid out.
	///
	/// Output is minimal and line-oriented, one fact per line, and is compared as
	/// golden files by the conformance suite.
	/// </remarks>
	public sealed class StructureReport
	{
		private readonly StructureReportOptions _options;
		private readonly IConsole _console;

		public StructureReport(StructureReportOptions options, IConsole console)
		{
			_options = options;
			_console = console;
		}

		/// <summary>Reports the document-level aspects: fonts, font mappings, bookmarks, annotations.</summary>
		public void ReportDocument(IDocument document, string displayName)
		{
			if (_options.Fonts)
				WriteFonts(document, displayName);

			if (_options.FontMappings)
				WriteFontMappings(document, displayName);

			if (_options.Bookmarks)
				WriteBookmarks(document, displayName);

			if (_options.Annotations)
				WriteAnnotations(document, displayName);
		}

		/// <summary>Reports the page-level aspects for one page.</summary>
		public void ReportPage(IDocumentPage page, int pageNumber, string displayName)
		{
			if (_options.Dimensions)
				WriteDimensions(page, pageNumber, displayName);

			if (_options.PageFonts)
				WritePageFonts(page, pageNumber, displayName);

			if (_options.PageFontMappings)
				WritePageFontMappings(page, pageNumber, displayName);

			if (_options.PageElements)
				WritePageElements(page, pageNumber, displayName);

			if (_options.FormElements)
				WriteFormElements(page, pageNumber, displayName);

			if (_options.Hyperlinks)
				WriteHyperlinks(page, pageNumber, displayName);
		}

		/// <remarks>
		/// Fonts are not a first-class collection anywhere in the binding; doc2text
		/// reads them the same way, as the semicolon-separated <c>DOCUMENT_FONTS</c>
		/// page attribute (see <c>LayoutAPI.cpp</c>'s <c>IGR_Get_Page_Attribute</c>
		/// handling), queried on the last page once every page has been laid out.
		/// </remarks>
		internal void WriteFonts(IDocument document, string displayName)
		{
			WriteFontList(GetLastPageAttribute(document, "DOCUMENT_FONTS"), displayName, pageNumber: null);
		}

		/// <remarks>Same source as <see cref="WriteFonts"/>, via <c>DOCUMENT_FONT_MAPPINGS</c>.</remarks>
		internal void WriteFontMappings(IDocument document, string displayName)
		{
			WriteFontMappingList(GetLastPageAttribute(document, "DOCUMENT_FONT_MAPPINGS"), displayName, pageNumber: null);
		}

		/// <remarks>
		/// Guarded on <see cref="IDocument.GetPageCount"/> for the same reason
		/// <see cref="GetLastPageAttribute"/> is: a bookmark's page number only
		/// exists once the document has been laid out into pages, and a handle
		/// opened without <c>IGR_FORMAT_IMAGE</c> — <c>--to text --bookmarks</c>,
		/// where the text reader needs the format bit bookmarks would otherwise take
		/// — was never laid out at all. Unlike <c>GetLastPageAttribute</c>,
		/// <c>RootBookmark</c> does not degrade to "nothing" on its own for that
		/// case; it throws the binding's own <c>Invalid DocHandle</c>. This is that
		/// same "reports nothing rather than failing" rule, applied where the
		/// binding does not already apply it.
		/// </remarks>
		internal void WriteBookmarks(IDocument document, string displayName)
		{
			if (document.GetPageCount() == 0)
				return;

			EngineBookmark root = document.RootBookmark;
			if (root == null)
				return;

			int index = 0;
			foreach (EngineBookmark child in root.Children ?? Enumerable.Empty<EngineBookmark>())
				WriteBookmarksRecursive(child, displayName, ref index);
		}

		private void WriteBookmarksRecursive(EngineBookmark bookmark, string displayName, ref int index)
		{
			index++;
			WriteRecord(displayName, $"bookmark={index} level={bookmark.Level} page={bookmark.PageIndex + 1} title={bookmark.Title}");

			foreach (EngineBookmark child in bookmark.Children ?? Enumerable.Empty<EngineBookmark>())
				WriteBookmarksRecursive(child, displayName, ref index);
		}

		/// <remarks>
		/// A document-level report even though every annotation belongs to a page:
		/// numbering runs across the whole document, so it has to be gathered in one
		/// pass rather than reset per <see cref="ReportPage"/> call.
		/// </remarks>
		internal void WriteAnnotations(IDocument document, string displayName)
		{
			int index = 0;
			int pageCount = document.GetPageCount();
			for (int pageIndex = 0; pageIndex < pageCount; pageIndex++)
			{
				using IDocumentPage page = document.GetPage(pageIndex);
				foreach (EngineAnnotation annotation in page.Annotations)
				{
					index++;
					WriteRecord(displayName,
						$"annotation={index} page={pageIndex + 1} kind={KindOf(annotation.Type)} " +
						$"author={annotation.Title} text={annotation.Text}");
				}
			}
		}

		internal void WriteDimensions(IDocumentPage page, int pageNumber, string displayName)
		{
			WriteRecord(displayName, $"page={pageNumber} width={page.GetWidth()} height={page.GetHeight()}");
		}

		internal void WritePageFonts(IDocumentPage page, int pageNumber, string displayName)
		{
			WriteFontList(page.GetAttribute("FONTS"), displayName, pageNumber);
		}

		/// <summary><c>--page-font-mappings</c>: how each page's fonts were substituted.</summary>
		internal void WritePageFontMappings(IDocumentPage page, int pageNumber, string displayName)
		{
			WriteFontMappingList(page.GetAttribute("FONT_MAPPINGS"), displayName, pageNumber);
		}

		internal void WritePageElements(IDocumentPage page, int pageNumber, string displayName)
		{
			EnginePageElement root = page.GetRootPageElement();
			if (root == null)
				return;

			int index = 0;
			foreach (EnginePageElement element in root.AllChildren())
			{
				index++;
				var bounds = element.Bounds;
				WriteRecord(displayName,
					$"page={pageNumber} element={index} kind={KindOf(element.Type)} depth={element.Depth} " +
					$"x={(int)bounds.X} y={(int)bounds.Y} w={(int)bounds.Width} h={(int)bounds.Height}");
			}
		}

		internal void WriteFormElements(IDocumentPage page, int pageNumber, string displayName)
		{
			int index = 0;
			foreach (EngineFormElement element in page.FormElements)
			{
				index++;
				WriteRecord(displayName,
					$"page={pageNumber} form-element={index} kind={FormElementKind(element.Type)} " +
					$"x={element.X} y={element.Y} w={element.Width} h={element.Height} name={element.Name}");
			}
		}

		internal void WriteHyperlinks(IDocumentPage page, int pageNumber, string displayName)
		{
			int index = 0;
			foreach (EngineHyperlink link in page.Hyperlinks)
			{
				index++;
				string target = link.Type == Hyperlink.ActionType.URI ? "uri=" + link.Uri : "ref=" + link.Ref;
				WriteRecord(displayName,
					$"page={pageNumber} hyperlink={index} kind={KindOf(link.Type)} " +
					$"x={link.X} y={link.Y} w={link.Width} h={link.Height} {target}");
			}
		}

		private void WriteRecord(string displayName, string fields)
		{
			_console.WriteLine(displayName + ": " + fields);
		}

		private void WriteFontList(string attributeValue, string displayName, int? pageNumber)
		{
			foreach (string font in SplitAttribute(attributeValue))
			{
				string fields = (pageNumber.HasValue ? $"page={pageNumber} " : "") + $"font={font}";
				WriteRecord(displayName, fields);
			}
		}

		/// <remarks>Each entry is <c>original=mapped</c>, per <c>LayoutAPI.cpp</c>'s own construction of the attribute text.</remarks>
		private void WriteFontMappingList(string attributeValue, string displayName, int? pageNumber)
		{
			foreach (string entry in SplitAttribute(attributeValue))
			{
				int split = entry.IndexOf('=');
				string original = split < 0 ? entry : entry.Substring(0, split);
				string mapped = split < 0 ? entry : entry.Substring(split + 1);

				string fields = (pageNumber.HasValue ? $"page={pageNumber} " : "") + 
								$"font={original} mapped={mapped}";
				WriteRecord(displayName, fields);
			}
		}

		private static IEnumerable<string> SplitAttribute(string value)
		{
			return string.IsNullOrEmpty(value) ? Enumerable.Empty<string>() : value.Split(';').Where(entry => entry.Length > 0);
		}

		private static string GetLastPageAttribute(IDocument document, string attributeName)
		{
			int pageCount = document.GetPageCount();
			if (pageCount == 0)
				return string.Empty;

			using IDocumentPage page = document.GetPage(pageCount - 1);
			return page.GetAttribute(attributeName) ?? string.Empty;
		}

		private static string KindOf<TEnum>(TEnum value) where TEnum : struct, Enum
		{
			return value.ToString().ToLowerInvariant();
		}

		/// <remarks>
		/// Short, CLI-facing labels rather than the binding's own enum names — only
		/// <c>SingleLineText</c> -&gt; <c>text</c> is confirmed against a real fixture;
		/// the rest follow the same short-label convention.
		/// </remarks>
		private static string FormElementKind(FormElement.ElementType type)
		{
			switch (type)
			{
				case FormElement.ElementType.SingleLineText:
					return "text";
				case FormElement.ElementType.MultiLineText:
					return "multiline";
				case FormElement.ElementType.CheckBox:
					return "checkbox";
				case FormElement.ElementType.RadioButton:
					return "radio";
				case FormElement.ElementType.ComboBox:
					return "combo";
				case FormElement.ElementType.ListBox:
					return "list";
				case FormElement.ElementType.FileSelect:
					return "file";
				case FormElement.ElementType.Signature:
					return "signature";
				case FormElement.ElementType.Password:
					return "password";
				case FormElement.ElementType.Button:
					return "button";
				default:
					return type.ToString().ToLowerInvariant();
			}
		}
	}
}
