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

using System.IO;
using System.Linq;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// <c>--fingerprint</c>'s line formatting, the one part of it that does not need a
	/// real page: <see cref="CanvasConversion.WriteFingerprint"/> reads the two page
	/// attributes and hands them to <see cref="CanvasConversion.FormatFingerprint"/>,
	/// which is a pure function of the two strings it already read.
	/// </summary>
	/// <remarks>
	/// Confirmed against a real <c>isys_doc2text.exe --fingerprint</c> run:
	/// <c>IMAGE_FINGERPRINT_V2</c> is tried first, <c>IMAGE_FINGERPRINT</c> is the
	/// fallback, and the value itself (e.g. <c>c:QlpoOTFB...</c>) is opaque to this
	/// tool — it is printed exactly as the engine returns it.
	/// </remarks>
	public class CanvasConversionTests
	{
		[Fact]
		public void The_v2_fingerprint_is_used_when_present()
		{
			string line = CanvasConversion.FormatFingerprint("sample.docx", 1, "c:v2value", "c:v1value");

			Assert.Equal("sample.docx: page=1 fingerprint=c:v2value", line);
		}

		[Fact]
		public void The_v1_fingerprint_is_the_fallback()
		{
			string line = CanvasConversion.FormatFingerprint("sample.docx", 2, "", "c:v1value");

			Assert.Equal("sample.docx: page=2 fingerprint=c:v1value", line);
		}

		[Fact]
		public void A_null_v2_falls_back_to_v1_the_same_way_as_empty()
		{
			string line = CanvasConversion.FormatFingerprint("sample.docx", 1, null, "c:v1value");

			Assert.Equal("sample.docx: page=1 fingerprint=c:v1value", line);
		}

		[Fact]
		public void Nothing_is_written_when_neither_attribute_has_a_value()
		{
			Assert.Null(CanvasConversion.FormatFingerprint("sample.docx", 1, "", ""));
			Assert.Null(CanvasConversion.FormatFingerprint("sample.docx", 1, null, null));
		}

		[Fact]
		public void The_page_number_is_one_based_and_the_display_name_is_used_verbatim()
		{
			// A subfile's display name already carries its container, e.g.
			// "bundle.zip>inner.docx" — this only ever echoes it, never rebuilds it.
			string line = CanvasConversion.FormatFingerprint("bundle.zip>inner.docx", 3, "c:abc", null);

			Assert.Equal("bundle.zip>inner.docx: page=3 fingerprint=c:abc", line);
		}

		// === canvas option composition =========================================
		//
		// The engine resolves a repeated key to its first occurrence, so the order
		// here is behaviour, not formatting: --canvas-options leads and the format's
		// own options fill in behind it.

		[Fact]
		public void A_format_with_no_options_of_its_own_passes_the_users_through_unchanged()
		{
			var png = FormatCatalog.Resolve("png");

			Assert.Equal("GRAPHIC_DPI=300", CanvasConversion.ComposeCanvasOptions(png, "GRAPHIC_DPI=300"));
		}

		[Fact]
		public void A_format_with_no_options_and_no_user_options_composes_to_nothing()
		{
			Assert.Equal(string.Empty, CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("png"), null));
			Assert.Equal(string.Empty, CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("png"), string.Empty));
		}

		[Fact]
		public void A_formats_own_options_apply_when_the_user_gives_none()
		{
			Assert.Equal("HTML_INLINE_IMAGES=ON", CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("html5"), null));
		}

		[Fact]
		public void The_users_options_come_first_so_they_win_the_keys_they_name()
		{
			// Not cosmetic: first occurrence wins in the engine, so this is what makes
			// --canvas-options HTML_INLINE_IMAGES=OFF actually take effect.
			string composed = CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("html5"), "HTML_INLINE_IMAGES=OFF");

			Assert.Equal("HTML_INLINE_IMAGES=OFF;HTML_INLINE_IMAGES=ON", composed);
			Assert.StartsWith("HTML_INLINE_IMAGES=OFF", composed);
		}

		[Fact]
		public void An_unrelated_user_option_leaves_the_formats_own_in_place()
		{
			Assert.Equal("GRAPHIC_DPI=300;HTML_INLINE_IMAGES=ON",
				CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("html5"), "GRAPHIC_DPI=300"));
		}

		[Fact]
		public void Trailing_separators_do_not_produce_an_empty_setting()
		{
			// The catalogue writes its options with a trailing ';' and users often do
			// too; an empty setting between two ';' is not worth relying on the engine
			// to ignore.
			Assert.Equal("GRAPHIC_DPI=300;HTML_INLINE_IMAGES=ON",
				CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("html5"), "GRAPHIC_DPI=300;"));
		}

		[Theory]
		[InlineData("html5", "HTML_INLINE_IMAGES=ON")]
		[InlineData("svg", "SVG_INLINE_IMAGES=ON")]
		public void The_inlining_formats_carry_their_switch(string token, string expected)
		{
			// Both canvases otherwise write the page raster to a sibling file that this
			// tool never emits, leaving a transparent text layer over nothing. Confirmed
			// against isys_doc2text.exe, which forces the same two and reproduces the
			// blank output exactly when told HTML_INLINE_IMAGES=OFF.
			Assert.Equal(expected, CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve(token), null));
		}

		[Fact]
		public void Mdast_still_carries_the_schema_that_tells_it_apart_from_json()
		{
			Assert.Equal("JSON_OUTPUT_SCHEMA=MDAST", CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("mdast"), null));
			Assert.Equal(string.Empty, CanvasConversion.ComposeCanvasOptions(FormatCatalog.Resolve("json"), null));
		}

		// -------------------------------------------------------------- --stdout

		/// <summary>
		/// A write-only, forward-only sink, the way <c>Console.OpenStandardOutput()</c>
		/// actually behaves: real console output cannot be seeked or read back, unlike
		/// <see cref="MemoryStream"/>, which would let a bug that seeks the destination
		/// directly pass silently.
		/// </summary>
		private sealed class ForwardOnlySink : Stream
		{
			public int FlushCount { get; private set; }
			public int BytesWritten { get; private set; }

			public override bool CanRead => false;
			public override bool CanSeek => false;
			public override bool CanWrite => true;
			public override long Length => throw new NotSupportedException();
			public override long Position
			{
				get => throw new NotSupportedException();
				set => throw new NotSupportedException();
			}

			public override void Write(byte[] buffer, int offset, int count) => BytesWritten += count;
			public override void Flush() => FlushCount++;
			public override long Seek(long offset, SeekOrigin origin) => throw new NotSupportedException();
			public override void SetLength(long value) => throw new NotSupportedException();
			public override int Read(byte[] buffer, int offset, int count) => throw new NotSupportedException();
		}

		[Fact]
		public void Streaming_to_stdout_never_seeks_the_destination_and_flushes_it_once_done()
		{
			// Reproduces a real bug found right after the --stdout NRE fix: handing
			// Console.OpenStandardOutput() straight to MakeCanvas produced no error and
			// no output. Every published sample that streams a canvas
			// (ConvertDocumentToTIFFStream, ConvertDocumentToInteractiveHDHTML) renders
			// into a MemoryStream instead, because finishing a binary format means
			// seeking back to patch a header computed only once rendering is done — and
			// the console's own stream is forward-only. ConvertToStream now renders into
			// its own seekable buffer and only copies the finished bytes across, so a
			// destination that would throw on any seek attempt must still work.
			var engine = new FakeDocumentFiltersEngine();
			engine.AddDocument("a.docx", "text");
			var conversion = new CanvasConversion(DocumentFiltersSession.For(engine), new RecordingConsole());
			var destination = new ForwardOnlySink();
			var item = new PlannedItem { SourcePath = "a.docx", DisplayName = "a.docx", Pages = PageRange.All };

			IDocument document = engine.OpenDocument("a.docx", 0, string.Empty, () => null);
			try
			{
				conversion.ConvertToStream(document, destination, item, FormatCatalog.Resolve("html5"), new ConvertOptions());
			}
			finally
			{
				document.Dispose();
			}

			Assert.True(destination.FlushCount > 0);
		}
	}
}
