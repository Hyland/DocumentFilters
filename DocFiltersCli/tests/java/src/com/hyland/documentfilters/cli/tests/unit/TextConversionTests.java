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

import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;

import com.hyland.documentfilters.cli.execution.TextConversion;
import com.hyland.documentfilters.cli.tests.Test;

/**
 * The ISYS control codes the extractor emits, and what {@code --to text} does with
 * them.
 *
 * <p>Ported from doc2text's {@code processOutput}, which is the only implementation
 * of this table there is. It has to live in the tool: a text-mode open resolves to
 * the same reader whether {@code IGR_FORMAT_TEXT} is named or omitted, so no open
 * flag can switch it.
 */
public final class TextConversionTests
{
	@Test
	public void ordinaryTextIsUntouched()
	{
		assertEquals("Hello, world.", TextConversion.stripControlCodes("Hello, world."));
	}

	@Test
	public void startOfHeadingBecomesASpace()
	{
		assertEquals("Title Body", TextConversion.stripControlCodes("Title\u0001Body"));
	}

	@Test
	public void stylingMarksAreDropped()
	{
		char[] codes =
		{
			'\u0002', // normal
			'\u0003', // bold
			'\u0004', // italic
			'\u0005', // underline
			'\u0006', // start-of-hit
			'\u000C', // soft break
			'\u0010', // end-of-meta
		};

		for (char code : codes)
			assertEquals("ab", TextConversion.stripControlCodes("a" + code + "b"), "code " + (int) code);
	}

	@Test
	public void paragraphAndStartOfMetaBecomeNewlines()
	{
		assertEquals("a\nb", TextConversion.stripControlCodes("a\u000Eb"));
		assertEquals("a\nb", TextConversion.stripControlCodes("a\u000Fb"));
	}

	@Test
	public void flowHtmlKeepsItsParagraphMarksAsNothing()
	{
		// The markup already carries the structure, so doc2text suppresses the
		// newline when the open flags say HTML; a newline there would only add
		// whitespace between tags.
		assertEquals("<p>a</p><p>b</p>", TextConversion.stripControlCodes("<p>a</p>\u000E<p>b</p>", true));
	}

	@Test
	public void startOfMetaStillBreaksALineInFlowHtml()
	{
		assertEquals("a\nb", TextConversion.stripControlCodes("a\u000Fb", true));
	}

	@Test
	public void realNewlinesAndTabsSurvive()
	{
		// Only the ISYS codes go; line endings are the document's, not ours.
		assertEquals("a\r\nb\tc", TextConversion.stripControlCodes("a\r\nb\tc"));
	}

	@Test
	public void everyCodeInOneStringIsHandledTogether()
	{
		String chunk = "\u0001H\u0002e\u0003l\u0004l\u0005o\u0006\u000E\u000Fw\u000Co\u0010rld";

		assertEquals(" Hello\n\nworld", TextConversion.stripControlCodes(chunk));
	}

	@Test
	public void aChunkOfNothingButCodesStripsToNothing()
	{
		assertEquals("", TextConversion.stripControlCodes("\u0002\u0003\u0004\u000C\u0010"));
		assertEquals("", TextConversion.stripControlCodes(""));
	}

	@Test
	public void strippingIsSafeAcrossAChunkBoundary()
	{
		// getText hands back 4096 characters at a time, so a code can land at either
		// edge. Every one is a single character that does not depend on its
		// neighbours, which is what makes stripping per chunk equivalent to stripping
		// the whole stream.
		String whole = "one\u000Etwo\u000Ethree";

		for (int split = 0; split <= whole.length(); split++)
		{
			String joined = TextConversion.stripControlCodes(whole.substring(0, split))
				+ TextConversion.stripControlCodes(whole.substring(split));

			assertEquals(TextConversion.stripControlCodes(whole), joined, "split at " + split);
		}
	}
}
