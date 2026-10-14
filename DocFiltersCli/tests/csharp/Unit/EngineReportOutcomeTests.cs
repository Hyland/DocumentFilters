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

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;
using AnnotationKind = Hyland.DocumentFilters.Annotations.Type;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The ten structure reports, against what the engine gives them.
	/// </summary>
	/// <remarks>
	/// Every one of these reads something only an opened, laid-out document has, so
	/// none of them could be tested before the engine had a seam. What is checked
	/// here is the shape of each record — these tests are the authority on that
	/// — and, as much, what each does with nothing: a document with no bookmarks, a
	/// page with no hyperlinks, an attribute the engine returns empty. A report that
	/// printed a header for an empty collection, or crashed on a null, would be wrong
	/// in a way no conversion test would notice.
	/// </remarks>
	public class EngineReportOutcomeTests
	{
		private static StructureReport Report(RecordingConsole console, Action<StructureReportOptions> configure)
		{
			var options = new StructureReportOptions();
			configure(options);
			return new StructureReport(options, console);
		}

		// ============================================ document-level

		/// <remarks>
		/// Fonts are not a collection in the binding: they arrive as the
		/// semicolon-separated DOCUMENT_FONTS attribute of the last page, which is the
		/// same route doc2text takes. That makes the splitting this report's own job,
		/// and worth pinning.
		/// </remarks>
		[Fact]
		public void Fonts_come_from_the_last_page_attribute_and_split_on_semicolons()
		{
			var document = new FakeDocument("sample.docx") { PageCount = 2 };
			document.Pages.Add(new FakePage());
			document.Pages.Add(new FakePage());
			document.Pages[1].Attributes["DOCUMENT_FONTS"] = "Calibri;Cambria Math";

			var console = new RecordingConsole();
			Report(console, o => o.Fonts = true).ReportDocument(document, "sample.docx");

			Assert.Equal(
				new[] { "sample.docx: font=Calibri", "sample.docx: font=Cambria Math" },
				console.OutLines);
		}

		[Fact]
		public void A_document_with_no_fonts_prints_nothing()
		{
			var document = new FakeDocument("sample.docx") { PageCount = 1 };

			var console = new RecordingConsole();
			Report(console, o => o.Fonts = true).ReportDocument(document, "sample.docx");

			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void A_document_with_no_pages_at_all_reports_no_fonts_rather_than_failing()
		{
			// A text-only format has no laid-out pages, so there is no last page to ask.
			var document = new FakeDocument("plain.txt") { PageCount = 0 };

			var console = new RecordingConsole();
			Report(console, o => o.Fonts = true).ReportDocument(document, "plain.txt");

			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void Bookmarks_are_walked_depth_first_with_their_level_and_one_based_page()
		{
			var document = new FakeDocument("sample.docx")
			{
				// A bookmark's page number only exists once the document has been laid
				// out, so WriteBookmarks skips a document with no pages at all — see
				// Bookmarks_are_skipped_for_a_document_with_no_pages below.
				PageCount = 4,
				Bookmarks = new EngineBookmark
				{
					Title = "root",
					Children = new List<EngineBookmark>
					{
						new EngineBookmark { Title = "Introduction", Level = 0, PageIndex = 0 },
						new EngineBookmark
						{
							Title = "Methodology",
							Level = 0,
							PageIndex = 2,
							Children = new List<EngineBookmark>
							{
								new EngineBookmark { Title = "Sampling", Level = 1, PageIndex = 3 }
							}
						}
					}
				}
			};

			var console = new RecordingConsole();
			Report(console, o => o.Bookmarks = true).ReportDocument(document, "sample.docx");

			Assert.Equal(
				new[]
				{
					"sample.docx: bookmark=1 level=0 page=1 title=Introduction",
					"sample.docx: bookmark=2 level=0 page=3 title=Methodology",
					"sample.docx: bookmark=3 level=1 page=4 title=Sampling"
				},
				console.OutLines);
		}

		[Fact]
		public void A_document_with_no_outline_prints_no_bookmarks()
		{
			// RootBookmark is null for a document with no outline, which is most of
			// them. PageCount is set so this exercises that path specifically, rather
			// than the page-count guard below for the same empty result.
			var document = new FakeDocument("plain.txt") { PageCount = 1 };

			var console = new RecordingConsole();
			Report(console, o => o.Bookmarks = true).ReportDocument(document, "plain.txt");

			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void Bookmarks_are_skipped_for_a_document_with_no_pages()
		{
			// Reproduces a real bug: a bookmark's page number only exists once the
			// document has been laid out, so a handle opened without IGR_FORMAT_IMAGE
			// -- --to text --bookmarks, where the text reader needs the format bit
			// bookmarks would otherwise take -- was never laid out at all.
			// IDocument.RootBookmark does not degrade to "nothing" on its own for that
			// case the way GetLastPageAttribute does; it throws the binding's own
			// "Invalid DocHandle". This guard applies the same "reports nothing rather
			// than failing" rule the binding does not.
			var document = new FakeDocument("sample.docx")
			{
				PageCount = 0,
				Bookmarks = new EngineBookmark
				{
					Title = "root",
					Children = new List<EngineBookmark>
					{
						new EngineBookmark { Title = "Introduction", Level = 0, PageIndex = 0 }
					}
				}
			};

			var console = new RecordingConsole();
			Report(console, o => o.Bookmarks = true).ReportDocument(document, "sample.docx");

			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void Annotations_are_reported_with_their_page_kind_author_and_text()
		{
			var document = new FakeDocument("sample.docx") { PageCount = 2 };
			document.Pages.Add(new FakePage());
			document.Pages.Add(new FakePage());
			document.Pages[1].AnnotationList.Add(new EngineAnnotation
			{
				Type = AnnotationKind.Text,
				Title = "Jane Smith",
				Text = "Check this figure"
			});

			var console = new RecordingConsole();
			Report(console, o => o.Annotations = true).ReportDocument(document, "sample.docx");

			string line = Assert.Single(console.OutLines);
			Assert.Contains("annotation=1", line);
			Assert.Contains("page=2", line);
			Assert.Contains("author=Jane Smith", line);
			Assert.Contains("text=Check this figure", line);
		}

		// ============================================ page-level

		[Fact]
		public void Dimensions_come_from_the_page_itself()
		{
			var console = new RecordingConsole();
			var page = new FakePage { Width = 612, Height = 792 };

			Report(console, o => o.Dimensions = true).ReportPage(page, 1, "sample.docx");

			Assert.Equal("sample.docx: page=1 width=612 height=792", Assert.Single(console.OutLines));
		}

		[Fact]
		public void Hyperlinks_report_a_uri_target_or_a_ref_target_but_never_both()
		{
			var page = new FakePage();
			page.HyperlinkList.Add(new EngineHyperlink
			{
				Type = Hyperlink.ActionType.URI,
				Uri = "https://example.com",
				Ref = "ignored",
				X = 100,
				Y = 200,
				Width = 80,
				Height = 12
			});
			page.HyperlinkList.Add(new EngineHyperlink
			{
				Type = Hyperlink.ActionType.GoTo,
				Uri = "ignored",
				Ref = "page-3",
				X = 1,
				Y = 2,
				Width = 3,
				Height = 4
			});

			var console = new RecordingConsole();
			Report(console, o => o.Hyperlinks = true).ReportPage(page, 1, "sample.docx");

			Assert.Contains("uri=https://example.com", console.OutLines[0]);
			Assert.DoesNotContain("ref=", console.OutLines[0]);
			Assert.Contains("ref=page-3", console.OutLines[1]);
			Assert.DoesNotContain("uri=", console.OutLines[1]);
		}

		[Fact]
		public void Form_elements_report_their_geometry_and_name()
		{
			var page = new FakePage();
			page.FormElementList.Add(new EngineFormElement
			{
				Type = FormElement.ElementType.SingleLineText,
				Name = "surname",
				X = 72,
				Y = 144,
				Width = 200,
				Height = 20
			});

			var console = new RecordingConsole();
			Report(console, o => o.FormElements = true).ReportPage(page, 1, "sample.docx");

			string line = Assert.Single(console.OutLines);
			Assert.Contains("form-element=1", line);
			Assert.Contains("x=72 y=144 w=200 h=20", line);
			Assert.Contains("name=surname", line);
		}

		/// <remarks>
		/// The element tree is reported flattened, depth first, with each node's own
		/// depth — so a nested node appears once, after its parent, not inside it.
		/// </remarks>
		[Fact]
		public void Page_elements_are_flattened_depth_first_with_their_depth()
		{
			var page = new FakePage
			{
				RootElement = new EnginePageElement
				{
					Depth = 0,
					Bounds = new RectangleF(0, 0, 612, 792),
					Children = new List<EnginePageElement>
					{
						new EnginePageElement
						{
							Type = PageElementType.Paragraph,
							Depth = 1,
							Bounds = new RectangleF(72, 72, 468, 14),
							Children = new List<EnginePageElement>
							{
								new EnginePageElement
								{
									Type = PageElementType.Run,
									Depth = 2,
									Bounds = new RectangleF(72, 72, 100, 14)
								}
							}
						}
					}
				}
			};

			var console = new RecordingConsole();
			Report(console, o => o.PageElements = true).ReportPage(page, 1, "sample.docx");

			Assert.Equal(2, console.OutLines.Count);
			Assert.Contains("element=1", console.OutLines[0]);
			Assert.Contains("depth=1", console.OutLines[0]);
			Assert.Contains("x=72 y=72 w=468 h=14", console.OutLines[0]);
			Assert.Contains("element=2", console.OutLines[1]);
			Assert.Contains("depth=2", console.OutLines[1]);
		}

		[Fact]
		public void A_page_that_exposes_no_element_tree_prints_nothing()
		{
			// GetRootPageElement returns null for a format with no layout tree.
			var console = new RecordingConsole();
			Report(console, o => o.PageElements = true).ReportPage(new FakePage(), 1, "sample.docx");

			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void An_empty_page_prints_nothing_for_any_page_report()
		{
			var console = new RecordingConsole();
			var page = new FakePage();

			Report(console, o =>
			{
				o.PageFonts = true;
				o.PageFontMappings = true;
				o.PageElements = true;
				o.FormElements = true;
				o.Hyperlinks = true;
			}).ReportPage(page, 1, "sample.docx");

			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void No_report_requested_reads_nothing_from_the_document()
		{
			// The reports are opt-in, so a plain conversion must not pay for them —
			// asking a page for its hyperlinks is work the engine has to do.
			var document = new FakeDocument("sample.docx") { ReadFailure = EngineError.ActionNotSupported };

			var console = new RecordingConsole();
			Report(console, o => { }).ReportDocument(document, "sample.docx");

			Assert.Empty(console.OutLines);
		}

		// ============================================ the catalogues

		private static CatalogReports Catalog(FakeDocumentFiltersEngine engine, RecordingConsole console)
		{
			return new CatalogReports(DocumentFiltersSession.For(engine), console);
		}

		[Fact]
		public void Formats_lists_what_the_engine_reports()
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.Formats.Add(new EngineFormat { Id = 254, ShortName = "docx", DisplayName = "Microsoft Word 2007" });
			engine.Formats.Add(new EngineFormat { Id = 112, ShortName = "doc", DisplayName = "Microsoft Word for Windows" });

			var console = new RecordingConsole();
			Catalog(engine, console).WriteFormats(new FormatsOptions());

			Assert.Equal(
				new[]
				{
					"id=254 ext=docx name=Microsoft Word 2007",
					"id=112 ext=doc name=Microsoft Word for Windows"
				},
				console.OutLines);
		}

		[Fact]
		public void The_formats_filter_matches_on_name_and_short_name()
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.Formats.Add(new EngineFormat { Id = 254, ShortName = "docx", DisplayName = "Microsoft Word 2007" });
			engine.Formats.Add(new EngineFormat { Id = 200, ShortName = "pdf", DisplayName = "Adobe Portable Document Format" });

			var console = new RecordingConsole();
			Catalog(engine, console).WriteFormats(new FormatsOptions { Filter = "word" });

			Assert.Equal("id=254 ext=docx name=Microsoft Word 2007", Assert.Single(console.OutLines));
		}

		[Fact]
		public void A_filter_matching_nothing_prints_nothing_rather_than_failing()
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.Formats.Add(new EngineFormat { Id = 254, ShortName = "docx", DisplayName = "Microsoft Word 2007" });

			var console = new RecordingConsole();
			Catalog(engine, console).WriteFormats(new FormatsOptions { Filter = "nothing-matches-this" });

			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void A_build_that_reports_no_formats_prints_nothing()
		{
			var console = new RecordingConsole();
			Catalog(new FakeDocumentFiltersEngine(), console).WriteFormats(new FormatsOptions());

			Assert.Empty(console.OutLines);
		}

		[Fact]
		public void Options_lists_what_the_engine_reports()
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.Options.Add(new EngineOption
			{
				DisplayName = "BW_MODE",
				Type = "int",
				DefaultValue = "0",
				Description = "Black and white rendering mode"
			});

			var console = new RecordingConsole();
			Catalog(engine, console).WriteOptions(new OptionCatalogOptions());

			string line = Assert.Single(console.OutLines);
			Assert.Contains("name=BW_MODE", line);
			Assert.Contains("type=int", line);
			Assert.Contains("default=0", line);
			Assert.Contains("description=Black and white rendering mode", line);
		}

		[Fact]
		public void An_options_filter_matching_nothing_prints_nothing()
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.Options.Add(new EngineOption { DisplayName = "BW_MODE", Type = "int", Description = "mode" });

			var console = new RecordingConsole();
			Catalog(engine, console).WriteOptions(new OptionCatalogOptions { Filter = "nothing-matches-this" });

			Assert.Empty(console.OutLines);
		}

		/// <remarks>
		/// The catalogues are the two commands that read no document, so a failure in
		/// either is a failure of the run rather than of an input — there is no input to
		/// blame. What matters is that the engine's error reaches the caller intact.
		/// </remarks>
		[Theory]
		[MemberData(nameof(EngineOutcomeTests.EveryEngineError), MemberType = typeof(EngineOutcomeTests))]
		public void A_catalogue_that_cannot_be_read_surfaces_the_engine_error(EngineError error)
		{
			var engine = new FakeDocumentFiltersEngine { CatalogFailure = error };

			EngineException fromFormats = Assert.Throws<EngineException>(
				() => Catalog(engine, new RecordingConsole()).WriteFormats(new FormatsOptions()));

			EngineException fromOptions = Assert.Throws<EngineException>(
				() => Catalog(engine, new RecordingConsole()).WriteOptions(new OptionCatalogOptions()));

			Assert.Equal(error, fromFormats.Error);
			Assert.Equal(error, fromOptions.Error);
		}
	}
}
