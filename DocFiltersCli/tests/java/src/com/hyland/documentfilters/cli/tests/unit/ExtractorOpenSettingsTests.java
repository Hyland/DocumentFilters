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
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.execution.ExtractorOpenSettings;
import com.hyland.documentfilters.cli.planning.FormatCatalog;
import com.hyland.documentfilters.cli.planning.FormatSpec;
import com.hyland.documentfilters.cli.tests.Test;
import java.util.Arrays;

/**
 * The open flags each command and format is opened with.
 *
 * <p>Worth pinning by literal value rather than by constant name. The
 * {@code IGR_FORMAT_*} constants are an enumeration packed into bits 16 to 19 (Text
 * 0x1, HTML 0x2, Image 0x5, Markdown 0x8, each shifted by 16), not independent bits,
 * so two of them OR'd together silently make a third --- and the numbers are the
 * contract the Python, Java and C++ ports have to reproduce.
 */
public final class ExtractorOpenSettingsTests
{
	// Written as literals on purpose: reading them from the binding would make the
	// test agree with the binding rather than with the ABI the four ports reproduce.
	private static final int BODY_AND_META = 2;
	private static final int META_ONLY = 1;
	private static final int FORMAT_TEXT = 0x00010000;
	private static final int FORMAT_HTML = 0x00020000;
	private static final int FORMAT_IMAGE = 0x00050000;
	private static final int FORMAT_MARKDOWN = 0x00080000;

	private static int bit(String token)
	{
		return bit(token, new ConvertOptions());
	}

	private static int bit(String token, ConvertOptions options)
	{
		return ExtractorOpenSettings.formatBit(FormatCatalog.resolve(token), options);
	}

	@Test
	public void aCanvasFormatOpensPaginated()
	{
		for (String token : Arrays.asList("png", "pdf", "html5", "tiff", "mdast"))
			assertEquals(FORMAT_IMAGE, bit(token), token);
	}

	@Test
	public void flowHtmlAsksForTheHtmlReaderNotTheTextOne()
	{
		// FORMAT_TEXT here means --to html quietly emits plain text, which looks
		// plausible enough to ship.
		assertEquals(FORMAT_HTML, bit("html"));
	}

	@Test
	public void textModeMarkdownAsksForTheMarkdownReader()
	{
		assertEquals(FORMAT_MARKDOWN, bit("md-text"));
	}

	@Test
	public void plainTextAsksForTheTextReader()
	{
		assertEquals(FORMAT_TEXT, bit("text"));
	}

	@Test
	public void includeControlCodesAsksForNoReaderAtAll()
	{
		// Omitting IGR_FORMAT_TEXT is the whole mechanism: it leaves the ISYS
		// control codes in the stream rather than having the engine strip them.
		ConvertOptions options = new ConvertOptions();
		options.includeControlCodes = true;

		assertEquals(0, bit("text", options));
	}

	@Test
	public void docpackOpensWithNoFormatBit()
	{
		// Matches doc2text, which opens --docpack with a bare IGR_BODY_AND_META.
		assertEquals(0, bit("docpack"));
	}

	@Test
	public void aStructureReportDoesNotChangeTheFormatBit()
	{
		// Forcing paginated mode here was tried and does not work: it takes the
		// text reader away from the conversion opened in the same call. The pairing
		// is refused by the options validator instead, so by the time this runs a
		// report implies a canvas format already.
		ConvertOptions options = new ConvertOptions();
		options.reports.dimensions = true;
		options.reports.bookmarks = true;

		assertEquals(FORMAT_TEXT, bit("text", options));
		assertEquals(FORMAT_MARKDOWN, bit("md-text", options));
		assertEquals(FORMAT_IMAGE, bit("pdf", options));
	}

	@Test
	public void onlyOneFormatBitIsEverSet()
	{
		// They are an enumeration in bits 16-19, so two OR'd together make a third.
		for (FormatSpec format : FormatCatalog.all())
		{
			int bit = ExtractorOpenSettings.formatBit(format, new ConvertOptions());

			assertTrue(
				bit == 0 || bit == FORMAT_TEXT || bit == FORMAT_HTML || bit == FORMAT_IMAGE || bit == FORMAT_MARKDOWN,
				format.getToken() + " produced an unrecognised format bit 0x" + Integer.toHexString(bit).toUpperCase());
		}
	}

	@Test
	public void theContentSelectorIsCarriedAlongsideTheFormatBit()
	{
		ExtractorOpenSettings settings = ExtractorOpenSettings.forFormat(FormatCatalog.resolve("html"), new ConvertOptions(), new SharedOptions());

		assertEquals(BODY_AND_META | FORMAT_HTML, settings.flags);
	}

	@Test
	public void inspectionCommandsOpenBodyAndMetadataAsText()
	{
		assertEquals(BODY_AND_META | FORMAT_TEXT, ExtractorOpenSettings.forInspection(new SharedOptions()).flags);
	}

	@Test
	public void extractImagesAsksTheEngineToEnumerateThem()
	{
		// ENUMERATE_IMAGES is read at open and nowhere else, so --images iterating the
		// document's images without it finds an empty collection every time.
		assertEquals("ENUMERATE_IMAGES=ON;", ExtractorOpenSettings.forInspection(new SharedOptions(), true).options);

		assertEquals("", ExtractorOpenSettings.forInspection(new SharedOptions()).options);
	}

	@Test
	public void theUsersEngineOptionsComeBeforeTheOneWeAdd()
	{
		// First occurrence wins in the engine, so --options ENUMERATE_IMAGES=OFF
		// still overrides what --images would otherwise turn on.
		SharedOptions shared = new SharedOptions();
		shared.processing.engineOptions = "ENUMERATE_IMAGES=OFF";

		assertEquals("ENUMERATE_IMAGES=OFF;ENUMERATE_IMAGES=ON;", ExtractorOpenSettings.forInspection(shared, true).options);
	}

	@Test
	public void metaOpensMetadataOnly()
	{
		// META_ONLY replaces BODY_AND_META rather than adding to it --- the two are
		// values of the same low-bit selector, not flags.
		assertEquals(META_ONLY | FORMAT_TEXT, ExtractorOpenSettings.forMetadataOnly(new SharedOptions()).flags);
	}
}
