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

using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// How <c>meta</c> turns the engine's metadata blob into one record per field.
	/// </summary>
	/// <remarks>
	/// There is no structured metadata API — <c>GetText</c> returns the fields as
	/// one run of text, and the record separator is not a newline. The engine emits
	/// <c>0x0F</c> before the block and <c>0x0E</c> between records, which is why
	/// these fixtures are built from control codes rather than <c>\n</c>: a parser
	/// that splits on newlines finds no boundary at all and reports every field of a
	/// document as one.
	/// </remarks>
	public class MetadataReportTests
	{
		private const char StartOfMeta = '\u000F';
		private const char Separator = '\u000E';
		private const char EndOfMeta = '\u0010';

		private static IReadOnlyList<string> Report(string metadata)
		{
			var engine = new FakeDocumentFiltersEngine();
			engine.AddDocument("a.docx", metadata);
			var console = new RecordingConsole();

			using IDocument document = engine.OpenDocument("a.docx", 0, string.Empty, () => null);
			new MetadataReport().WriteText(document, "a.docx", console);

			return console.OutLines;
		}

		[Fact]
		public void Records_separated_by_control_codes_each_become_one_line()
		{
			// The real shape, from a PDF: 0x0F opens the block, 0x0E divides it, 0x10
			// closes it. Splitting this on '\n' yields a single bogus field whose label
			// still carries the 0x0F.
			var lines = Report(StartOfMeta + "Title: Report" + Separator + "Author: Ada" + Separator
							   + "Producer: Quartz" + EndOfMeta);

			Assert.Equal(new[]
			{
				"a.docx: field=Title value=Report",
				"a.docx: field=Author value=Ada",
				"a.docx: field=Producer value=Quartz",
			}, lines);
		}

		[Fact]
		public void A_value_carrying_a_newline_stays_one_record()
		{
			// A Word Title or
			// Comments field routinely has one; the continuation used to be dropped,
			// silently truncating the value.
			var lines = Report(StartOfMeta + "Title: Quarterly Report\nsecond line" + Separator + "Author: Ada");

			Assert.Equal(new[]
			{
				"a.docx: field=Title value=Quarterly Report second line",
				"a.docx: field=Author value=Ada",
			}, lines);
		}

		[Fact]
		public void Carriage_returns_and_tabs_inside_a_value_become_single_spaces()
		{
			var lines = Report(StartOfMeta + "Comments: one\ttwo\r\nthree");

			Assert.Equal(new[] { "a.docx: field=Comments value=one two three" }, lines);
		}

		[Fact]
		public void A_label_containing_colons_is_not_cut_apart()
		{
			// Splitting on the first ": " rather than the first ':' is what keeps an
			// XMP label whole — none of its internal colons is followed by a space.
			var lines = Report(StartOfMeta + "xmp:dc:creator: slop001");

			Assert.Equal(new[] { "a.docx: field=xmp:dc:creator value=slop001" }, lines);
		}

		[Fact]
		public void A_document_with_no_metadata_reports_nothing()
		{
			Assert.Empty(Report(string.Empty));
			Assert.Empty(Report(StartOfMeta.ToString() + EndOfMeta));
		}

		[Fact]
		public void A_continuation_with_no_field_before_it_is_ignored()
		{
			// Nothing to attach it to, so it is dropped rather than inventing a field.
			Assert.Empty(Report("loose text with no label"));
		}
	}
}
