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

package com.hyland.documentfilters.cli.configuration;

import java.util.ArrayList;
import java.util.List;

/**
 * The document and page structure aspects {@code convert} can report while it
 * works, emitted during a conversion run rather than by a separate command.
 */
public final class StructureReportOptions
{
	public boolean fonts;

	public boolean fontMappings;

	public boolean pageFonts;

	public boolean pageFontMappings;

	public boolean dimensions;

	public boolean pageElements;

	public boolean formElements;

	public boolean hyperlinks;

	public boolean bookmarks;

	public boolean annotations;

	/** True when any report was requested. */
	public boolean any()
	{
		return fonts || fontMappings || pageFonts || pageFontMappings || dimensions
			|| pageElements || formElements || hyperlinks || bookmarks || annotations;
	}

	/**
	 * The report flags asked for, spelled as the user typed them, in a fixed
	 * order.
	 *
	 * <p>So that a message naming one report names the same one in all four
	 * implementations: {@code --fonts --bookmarks} and {@code --bookmarks --fonts}
	 * are the same request and must produce the same error text. Hand-written
	 * beside the fields, in the same idiom as {@code ErrorCodes.ALL} and for
	 * the same reason --- C++17 has no reflection.
	 */
	public List<String> requested()
	{
		List<String> requested = new ArrayList<>();
		if (fonts)
			requested.add("--fonts");
		if (fontMappings)
			requested.add("--font-mappings");
		if (pageFonts)
			requested.add("--page-fonts");
		if (pageFontMappings)
			requested.add("--page-font-mappings");
		if (dimensions)
			requested.add("--dimensions");
		if (pageElements)
			requested.add("--page-elements");
		if (formElements)
			requested.add("--form-elements");
		if (hyperlinks)
			requested.add("--hyperlinks");
		if (bookmarks)
			requested.add("--bookmarks");
		if (annotations)
			requested.add("--annotations");
		return requested;
	}
}
