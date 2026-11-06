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

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.planning.FormatSpec;
import java.io.IOException;
import java.io.Writer;

/**
 * The text path: formats produced straight from the extractor, without a canvas
 * --- {@code text}, {@code md-text} and flow {@code html}.
 */
public final class TextConversion
{
	/**
	 * Writes one document's text to {@code destination}.
	 *
	 * <p><b>Precondition:</b> {@code document} must already have been opened with
	 * {@link ExtractorOpenSettings#forFormat} for this same {@code format}.
	 *
	 * <p>Control codes are stripped unless {@code --include-control-codes} asks for
	 * them; nothing else is rewritten, because the payload is the document.
	 */
	public void convert(IDocument document, FormatSpec format, ConvertOptions options, Writer destination) throws IOException
	{
		boolean isFlowHtml = format != null && format.getToken().equals("html");
		while (!document.getEOF())
		{
			String chunk = document.getText(4096);
			destination.write(options.includeControlCodes ? chunk : stripControlCodes(chunk, isFlowHtml));
		}
	}

	/** {@link #stripControlCodes(String, boolean)} for anything but flow HTML. */
	public static String stripControlCodes(String chunk)
	{
		return stripControlCodes(chunk, false);
	}

	/**
	 * Replaces the ISYS control codes the extractor emits with what they mean, as
	 * doc2text's {@code processOutput} does:
	 * <ul>
	 * <li>{@code 0x01} start-of-heading becomes a space.</li>
	 * <li>{@code 0x02}--{@code 0x06}, {@code 0x0C} soft break and {@code 0x10}
	 * end-of-meta are dropped.</li>
	 * <li>{@code 0x0E} paragraph and {@code 0x0F} start-of-meta become newlines.</li>
	 * </ul>
	 * Safe on a chunk boundary because every code is a single character.
	 *
	 * @param isFlowHtml true for flow {@code html} only, which wants no newline from
	 *        a paragraph mark.
	 */
	public static String stripControlCodes(String chunk, boolean isFlowHtml)
	{
		StringBuilder stripped = new StringBuilder(chunk.length());

		for (int i = 0; i < chunk.length(); ++i)
		{
			char character = chunk.charAt(i);
			switch (character)
			{
				case '\u0001':
					stripped.append(' ');
					break;

				case '\u0002':
				case '\u0003':
				case '\u0004':
				case '\u0005':
				case '\u0006':
				case '\u000C':
				case '\u0010':
					break;

				case '\u000E':
					if (!isFlowHtml)
						stripped.append('\n');
					break;

				case '\u000F':
					stripped.append('\n');
					break;

				default:
					stripped.append(character);
					break;
			}
		}

		return stripped.toString();
	}
}
