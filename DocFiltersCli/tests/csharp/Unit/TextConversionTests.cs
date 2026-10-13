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

using Hyland.DocumentFilters.Cli.Execution;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The ISYS control codes the extractor emits, and what <c>--to text</c> does
	/// with them.
	/// </summary>
	/// <remarks>
	/// Ported from doc2text's <c>processOutput</c>, which is the only implementation
	/// of this table there is. It has to live in the tool: a text-mode open resolves
	/// to the same reader whether <c>IGR_FORMAT_TEXT</c> is named or omitted, so no
	/// open flag can switch it.
	/// </remarks>
	public class TextConversionTests
	{
		[Fact]
		public void Ordinary_text_is_untouched()
		{
			Assert.Equal("Hello, world.", TextConversion.StripControlCodes("Hello, world."));
		}

		[Fact]
		public void Start_of_heading_becomes_a_space()
		{
			// \u form throughout: C#'s \x escape is variable length, so "a\x0Eb" is the
			// single character U+0EB, not U+000E followed by 'b'.
			Assert.Equal("Title Body", TextConversion.StripControlCodes("Title\u0001Body"));
		}

		[Theory]
		[InlineData('\x02')]  // normal
		[InlineData('\x03')]  // bold
		[InlineData('\x04')]  // italic
		[InlineData('\x05')]  // underline
		[InlineData('\x06')]  // start-of-hit
		[InlineData('\x0C')]  // soft break
		[InlineData('\x10')]  // end-of-meta
		public void Styling_marks_are_dropped(char code)
		{
			Assert.Equal("ab", TextConversion.StripControlCodes("a" + code + "b"));
		}

		[Fact]
		public void Paragraph_and_start_of_meta_become_newlines()
		{
			Assert.Equal("a\nb", TextConversion.StripControlCodes("a\u000Eb"));
			Assert.Equal("a\nb", TextConversion.StripControlCodes("a\u000Fb"));
		}

		[Fact]
		public void Flow_html_keeps_its_paragraph_marks_as_nothing()
		{
			// The markup already carries the structure, so doc2text suppresses the
			// newline when the open flags say HTML; a newline there would only add
			// whitespace between tags.
			Assert.Equal("<p>a</p><p>b</p>",
				TextConversion.StripControlCodes("<p>a</p>\u000E<p>b</p>", isFlowHtml: true));
		}

		[Fact]
		public void Start_of_meta_still_breaks_a_line_in_flow_html()
		{
			Assert.Equal("a\nb", TextConversion.StripControlCodes("a\u000Fb", isFlowHtml: true));
		}

		[Fact]
		public void Real_newlines_and_tabs_survive()
		{
			// Only the ISYS codes go; line endings are the document's, not ours.
			Assert.Equal("a\r\nb\tc", TextConversion.StripControlCodes("a\r\nb\tc"));
		}

		[Fact]
		public void Every_code_in_one_string_is_handled_together()
		{
			string chunk = "\u0001H\u0002e\u0003l\u0004l\u0005o\u0006\u000E\u000Fw\u000Co\u0010rld";

			Assert.Equal(" Hello\n\nworld", TextConversion.StripControlCodes(chunk));
		}

		[Fact]
		public void A_chunk_of_nothing_but_codes_strips_to_nothing()
		{
			Assert.Equal(string.Empty, TextConversion.StripControlCodes("\u0002\u0003\u0004\u000C\u0010"));
			Assert.Equal(string.Empty, TextConversion.StripControlCodes(string.Empty));
		}

		[Fact]
		public void Stripping_is_safe_across_a_chunk_boundary()
		{
			// GetText hands back 4096 characters at a time, so a code can land at
			// either edge. Every one is a single character that does not depend on its
			// neighbours, which is what makes stripping per chunk equivalent to
			// stripping the whole stream.
			const string whole = "one\u000Etwo\u000Ethree";

			for (int split = 0; split <= whole.Length; split++)
			{
				string joined = TextConversion.StripControlCodes(whole.Substring(0, split))
							  + TextConversion.StripControlCodes(whole.Substring(split));

				Assert.Equal(TextConversion.StripControlCodes(whole), joined);
			}
		}
	}
}
