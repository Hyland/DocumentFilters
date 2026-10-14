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

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Planning;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The open flags each command and format is opened with.
	/// </summary>
	/// <remarks>
	/// Worth pinning by literal value rather than by enum name. The
	/// <c>IGR_FORMAT_*</c> constants are an enumeration packed into bits 16 to 19
	/// (<c>Text</c> 0x1, <c>HTML</c> 0x2, <c>Image</c> 0x5, <c>Markdown</c> 0x8, each
	/// shifted by 16), not independent bits, so two of them OR'd together silently
	/// make a third — and the numbers are the contract the Python, Java and C++ ports
	/// have to reproduce.
	/// </remarks>
	public class ExtractorOpenSettingsTests
	{
		private const int BodyAndMeta = 2;
		private const int MetaOnly = 1;
		private const int FormatText = 0x00010000;
		private const int FormatHTML = 0x00020000;
		private const int FormatImage = 0x00050000;
		private const int FormatMarkdown = 0x00080000;

		private static int Bit(string token, ConvertOptions options = null)
		{
			return ExtractorOpenSettings.FormatBit(FormatCatalog.Resolve(token), options ?? new ConvertOptions());
		}

		[Theory]
		[InlineData("png")]
		[InlineData("pdf")]
		[InlineData("html5")]
		[InlineData("tiff")]
		[InlineData("mdast")]
		public void A_canvas_format_opens_paginated(string token)
		{
			Assert.Equal(FormatImage, Bit(token));
		}

		[Fact]
		public void Flow_html_asks_for_the_html_reader_not_the_text_one()
		{
			// FormatText here means --to html quietly emits plain text, which looks
			// plausible enough to ship.
			Assert.Equal(FormatHTML, Bit("html"));
		}

		[Fact]
		public void Text_mode_markdown_asks_for_the_markdown_reader()
		{
			Assert.Equal(FormatMarkdown, Bit("md-text"));
		}

		[Fact]
		public void Plain_text_asks_for_the_text_reader()
		{
			Assert.Equal(FormatText, Bit("text"));
		}

		[Fact]
		public void Include_control_codes_asks_for_no_reader_at_all()
		{
			// Omitting IGR_FORMAT_TEXT is the whole mechanism: it leaves the ISYS
			// control codes in the stream rather than having the engine strip them.
			Assert.Equal(0, Bit("text", new ConvertOptions { IncludeControlCodes = true }));
		}

		[Fact]
		public void Docpack_opens_with_no_format_bit()
		{
			// Matches doc2text, which opens --docpack with a bare IGR_BODY_AND_META.
			Assert.Equal(0, Bit("docpack"));
		}

		[Fact]
		public void A_structure_report_does_not_change_the_format_bit()
		{
			// Forcing paginated mode here was tried and does not work: it takes the
			// text reader away from the conversion opened in the same call. The pairing
			// is refused by OptionsValidator.CheckReportsNeedCanvasFormat instead, so
			// by the time this runs a report implies a canvas format already.
			var options = new ConvertOptions();
			options.Reports.Dimensions = true;
			options.Reports.Bookmarks = true;

			Assert.Equal(FormatText, Bit("text", options));
			Assert.Equal(FormatMarkdown, Bit("md-text", options));
			Assert.Equal(FormatImage, Bit("pdf", options));
		}

		[Fact]
		public void Only_one_format_bit_is_ever_set()
		{
			// They are an enumeration in bits 16-19, so two OR'd together make a third.
			foreach (FormatSpec format in FormatCatalog.All())
			{
				int bit = ExtractorOpenSettings.FormatBit(format, new ConvertOptions());
				Assert.True(bit == 0 || bit == FormatText || bit == FormatHTML || bit == FormatImage || bit == FormatMarkdown,
					$"{format.Token} produced an unrecognised format bit 0x{bit:X}");
			}
		}

		[Fact]
		public void The_content_selector_is_carried_alongside_the_format_bit()
		{
			var settings = ExtractorOpenSettings.ForFormat(FormatCatalog.Resolve("html"), new ConvertOptions(), new SharedOptions());

			Assert.Equal(BodyAndMeta | FormatHTML, settings.Flags);
		}

		[Fact]
		public void Inspection_commands_open_body_and_metadata_as_text()
		{
			Assert.Equal(BodyAndMeta | FormatText, ExtractorOpenSettings.ForInspection(new SharedOptions()).Flags);
		}

		[Fact]
		public void Extract_images_asks_the_engine_to_enumerate_them()
		{
			// ENUMERATE_IMAGES is read at open and nowhere else, so --images iterating
			// IDocument.Images without it finds an empty collection every time.
			Assert.Equal("ENUMERATE_IMAGES=ON;",
				ExtractorOpenSettings.ForInspection(new SharedOptions(), enumerateImages: true).Options);

			Assert.Equal(string.Empty, ExtractorOpenSettings.ForInspection(new SharedOptions()).Options);
		}

		[Fact]
		public void The_users_engine_options_come_before_the_one_we_add()
		{
			// First occurrence wins in the engine, so --options ENUMERATE_IMAGES=OFF
			// still overrides what --images would otherwise turn on.
			var shared = new SharedOptions();
			shared.Processing.EngineOptions = "ENUMERATE_IMAGES=OFF";

			Assert.Equal("ENUMERATE_IMAGES=OFF;ENUMERATE_IMAGES=ON;",
				ExtractorOpenSettings.ForInspection(shared, enumerateImages: true).Options);
		}

		[Fact]
		public void Meta_opens_metadata_only()
		{
			// MetaOnly replaces BodyAndMeta rather than adding to it — the two are
			// values of the same low-bit selector, not flags.
			Assert.Equal(MetaOnly | FormatText, ExtractorOpenSettings.ForMetadataOnly(new SharedOptions()).Flags);
		}
	}
}
