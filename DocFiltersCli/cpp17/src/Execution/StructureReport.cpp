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

#include "Execution/StructureReport.h"

#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				namespace
				{
					// -----------------------------------------------------------------
					//  KindOf<TEnum>, by hand.
					//
					//  The C# reference names these with `value.ToString().ToLowerInvariant()`
					//  --- the enum member's own name, lowercased. C++ has no reflection,
					//  so each table below is that same lowering, done once by hand
					//  against the C# member list (confirmed identical, member-for-member,
					//  to the C++ binding's own Annotation type; the binding's
					//  Hyperlink::Type differs from the C# ActionType in name and order
					//  --- Uri/GoTo vs GoTo/URI --- but lowers to the same two strings
					///  either way).
					// -----------------------------------------------------------------

					std::string annotationKindOf(Hyland::DocFilters::Annotation::Type type)
					{
						using T = Hyland::DocFilters::Annotation::Type;
						switch (type)
						{
						case T::Unknown:
							return "unknown";
						case T::Text:
							return "text";
						case T::Link:
							return "link";
						case T::Freetext:
							return "freetext";
						case T::Line:
							return "line";
						case T::Rectangle:
							return "rectangle";
						case T::Ellipse:
							return "ellipse";
						case T::Polygon:
							return "polygon";
						case T::Polyline:
							return "polyline";
						case T::Highlight:
							return "highlight";
						case T::Underline:
							return "underline";
						case T::Squiggly:
							return "squiggly";
						case T::Strikeout:
							return "strikeout";
						case T::Stamp:
							return "stamp";
						case T::Caret:
							return "caret";
						case T::Ink:
							return "ink";
						case T::Popup:
							return "popup";
						case T::FileAttachment:
							return "fileattachment";
						case T::Sound:
							return "sound";
						case T::Movie:
							return "movie";
						case T::Widget:
							return "widget";
						case T::Screen:
							return "screen";
						case T::Printermark:
							return "printermark";
						case T::Trapnet:
							return "trapnet";
						case T::Watermark:
							return "watermark";
						case T::ThreeD:
							return "threed";
						case T::Barcode:
							return "barcode";
						case T::NamedDestination:
							return "nameddestination";
						default:
							return "unknown";
						}
					}

					std::string pageElementKindOf(Hyland::DocFilters::PageElementType type)
					{
						using T = Hyland::DocFilters::PageElementType;
						switch (type)
						{
						case T::Page:
							return "page";
						case T::Section:
							return "section";
						case T::Column:
							return "column";
						case T::Paragraph:
							return "paragraph";
						case T::Line:
							return "line";
						case T::Run:
							return "run";
						case T::Word:
							return "word";
						case T::Header:
							return "header";
						case T::Footer:
							return "footer";
						case T::Image:
							return "image";
						case T::Table:
							return "table";
						case T::TableRow:
							return "tablerow";
						case T::TableCell:
							return "tablecell";
						case T::Form:
							return "form";
						case T::FormElement:
							return "formelement";
						case T::Float:
							return "float";
						case T::Graphic:
							return "graphic";
						case T::TextBox:
							return "textbox";
						case T::Notes:
							return "notes";
						default:
							// T::None has no C# equivalent --- the binding's own extra
							// leading member --- and is never produced by a real element,
							// only by a default-constructed one.
							return "none";
						}
					}

					std::string hyperlinkKindOf(Hyland::DocFilters::Hyperlink::Type type)
					{
						using T = Hyland::DocFilters::Hyperlink::Type;
						switch (type)
						{
						case T::Uri:
							return "uri";
						case T::GoTo:
							return "goto";
						default:
							return "uri";
						}
					}

					/// <remarks>
					/// Short, CLI-facing labels rather than the binding's own enum names
					/// --- only <c>SingleLineText</c> -&gt; <c>text</c> is confirmed
					/// against a real fixture; the rest follow the same short-label
					/// convention. The switch matches on the C++ binding's own member
					/// spellings (<c>Checkbox</c>, <c>Combobox</c>, <c>Listbox</c>, ...),
					/// which differ in casing from the C# reference's, but produces the
					/// identical output text.
					/// </remarks>
					std::string formElementKind(Hyland::DocFilters::FormElement::Type type)
					{
						using T = Hyland::DocFilters::FormElement::Type;
						switch (type)
						{
						case T::SinglelineText:
							return "text";
						case T::MultilineText:
							return "multiline";
						case T::Checkbox:
							return "checkbox";
						case T::RadioButton:
							return "radio";
						case T::Combobox:
							return "combo";
						case T::Listbox:
							return "list";
						case T::FileSelect:
							return "file";
						case T::Signature:
							return "signature";
						case T::Password:
							return "password";
						case T::Button:
							return "button";
						default:
							return "text";
						}
					}

					std::vector<std::string> splitAttribute(const std::string& value)
					{
						std::vector<std::string> result;
						std::string current;
						for (char ch : value)
						{
							if (ch == ';')
							{
								if (!current.empty())
									result.push_back(current);
								current.clear();
							}
							else
							{
								current += ch;
							}
						}
						if (!current.empty())
							result.push_back(current);
						return result;
					}

					std::string getLastPageAttribute(Engine::IDocument& document, const std::string& attributeName)
					{
						int pageCount = document.getPageCount();
						if (pageCount == 0)
							return std::string();

						std::unique_ptr<Engine::IDocumentPage> page = document.getPage(pageCount - 1);
						return page->getAttribute(attributeName);
					}

					/// <summary>This node's descendants, depth first --- the node itself is not included.</summary>
					void collectAllChildren(const Engine::EnginePageElement& node, std::vector<const Engine::EnginePageElement*>& out)
					{
						for (const Engine::EnginePageElement& child : node.children)
						{
							out.push_back(&child);
							collectAllChildren(child, out);
						}
					}
				}

				void StructureReport::reportDocument(Engine::IDocument& document, const std::string& displayName)
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

				void StructureReport::reportPage(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName)
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

				void StructureReport::writeFonts(Engine::IDocument& document, const std::string& displayName)
				{
					writeFontList(getLastPageAttribute(document, "DOCUMENT_FONTS"), displayName, std::nullopt);
				}

				void StructureReport::writeFontMappings(Engine::IDocument& document, const std::string& displayName)
				{
					writeFontMappingList(getLastPageAttribute(document, "DOCUMENT_FONT_MAPPINGS"), displayName, std::nullopt);
				}

				namespace
				{
					void writeBookmarksRecursive(
						Runtime::IConsole& console, const Engine::EngineBookmark& bookmark, const std::string& displayName, int& index)
					{
						index++;
						console.writeLine(displayName + ": bookmark=" + std::to_string(index) + " level=" + std::to_string(bookmark.level)
							+ " page=" + std::to_string(bookmark.pageIndex + 1) + " title=" + bookmark.title);

						for (const Engine::EngineBookmark& child : bookmark.children)
							writeBookmarksRecursive(console, child, displayName, index);
					}
				}

				void StructureReport::writeBookmarks(Engine::IDocument& document, const std::string& displayName)
				{
					if (document.getPageCount() == 0)
						return;

					std::optional<Engine::EngineBookmark> root = document.rootBookmark();
					if (!root.has_value())
						return;

					int index = 0;
					for (const Engine::EngineBookmark& child : root->children)
						writeBookmarksRecursive(_console, child, displayName, index);
				}

				void StructureReport::writeAnnotations(Engine::IDocument& document, const std::string& displayName)
				{
					int index = 0;
					int pageCount = document.getPageCount();
					for (int pageIndex = 0; pageIndex < pageCount; pageIndex++)
					{
						std::unique_ptr<Engine::IDocumentPage> page = document.getPage(pageIndex);
						for (const Engine::EngineAnnotation& annotation : page->annotations())
						{
							index++;
							writeRecord(displayName,
								"annotation=" + std::to_string(index) + " page=" + std::to_string(pageIndex + 1)
									+ " kind=" + annotationKindOf(annotation.type) + " author=" + annotation.title + " text=" + annotation.text);
						}
					}
				}

				void StructureReport::writeDimensions(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName)
				{
					writeRecord(displayName,
						"page=" + std::to_string(pageNumber) + " width=" + std::to_string(page.getWidth())
							+ " height=" + std::to_string(page.getHeight()));
				}

				void StructureReport::writePageFonts(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName)
				{
					writeFontList(page.getAttribute("FONTS"), displayName, pageNumber);
				}

				void StructureReport::writePageFontMappings(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName)
				{
					writeFontMappingList(page.getAttribute("FONT_MAPPINGS"), displayName, pageNumber);
				}

				void StructureReport::writePageElements(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName)
				{
					std::optional<Engine::EnginePageElement> root = page.getRootPageElement();
					if (!root.has_value())
						return;

					std::vector<const Engine::EnginePageElement*> all;
					collectAllChildren(*root, all);

					int index = 0;
					for (const Engine::EnginePageElement* element : all)
					{
						index++;
						const Engine::EngineRectF& bounds = element->bounds;
						writeRecord(displayName,
							"page=" + std::to_string(pageNumber) + " element=" + std::to_string(index) + " kind="
								+ pageElementKindOf(element->type) + " depth=" + std::to_string(element->depth)
								+ " x=" + std::to_string(static_cast<int>(bounds.left)) + " y=" + std::to_string(static_cast<int>(bounds.top))
								+ " w=" + std::to_string(static_cast<int>(bounds.right - bounds.left))
								+ " h=" + std::to_string(static_cast<int>(bounds.bottom - bounds.top)));
					}
				}

				void StructureReport::writeFormElements(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName)
				{
					int index = 0;
					for (const Engine::EngineFormElement& element : page.formElements())
					{
						index++;
						writeRecord(displayName,
							"page=" + std::to_string(pageNumber) + " form-element=" + std::to_string(index)
								+ " kind=" + formElementKind(element.type) + " x=" + std::to_string(element.x) + " y=" + std::to_string(element.y)
								+ " w=" + std::to_string(element.width) + " h=" + std::to_string(element.height) + " name=" + element.name);
					}
				}

				void StructureReport::writeHyperlinks(Engine::IDocumentPage& page, int pageNumber, const std::string& displayName)
				{
					int index = 0;
					for (const Engine::EngineHyperlink& link : page.hyperlinks())
					{
						index++;
						std::string target = link.type == Hyland::DocFilters::Hyperlink::Type::Uri ? "uri=" + link.uri : "ref=" + link.ref;
						writeRecord(displayName,
							"page=" + std::to_string(pageNumber) + " hyperlink=" + std::to_string(index) + " kind=" + hyperlinkKindOf(link.type)
								+ " x=" + std::to_string(link.x) + " y=" + std::to_string(link.y) + " w=" + std::to_string(link.width)
								+ " h=" + std::to_string(link.height) + " " + target);
					}
				}

				void StructureReport::writeRecord(const std::string& displayName, const std::string& fields)
				{
					_console.writeLine(displayName + ": " + fields);
				}

				void StructureReport::writeFontList(const std::string& attributeValue, const std::string& displayName, std::optional<int> pageNumber)
				{
					for (const std::string& font : splitAttribute(attributeValue))
					{
						std::string fields = (pageNumber.has_value() ? "page=" + std::to_string(*pageNumber) + " " : std::string()) + "font=" + font;
						writeRecord(displayName, fields);
					}
				}

				void StructureReport::writeFontMappingList(
					const std::string& attributeValue, const std::string& displayName, std::optional<int> pageNumber)
				{
					for (const std::string& entry : splitAttribute(attributeValue))
					{
						std::size_t split = entry.find('=');
						std::string original = split == std::string::npos ? entry : entry.substr(0, split);
						std::string mapped = split == std::string::npos ? entry : entry.substr(split + 1);

						std::string fields = (pageNumber.has_value() ? "page=" + std::to_string(*pageNumber) + " " : std::string()) + "font="
							+ original + " mapped=" + mapped;
						writeRecord(displayName, fields);
					}
				}
			}
		}
	}
}
