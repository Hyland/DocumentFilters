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
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.execution.CanvasConversion;
import com.hyland.documentfilters.cli.execution.DocumentFiltersSession;
import com.hyland.documentfilters.cli.planning.FormatCatalog;
import com.hyland.documentfilters.cli.planning.PageRange;
import com.hyland.documentfilters.cli.planning.PlannedItem;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocumentFiltersEngine;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.io.IOException;
import java.io.OutputStream;

/**
 * {@code --fingerprint}'s line formatting, the one part of it that does not need a
 * real page: {@link CanvasConversion#writeFingerprint} reads the two page
 * attributes and hands them to {@link CanvasConversion#formatFingerprint}, which is
 * a pure function of the two strings it already read.
 *
 * <p>Confirmed against a real {@code isys_doc2text --fingerprint} run:
 * {@code IMAGE_FINGERPRINT_V2} is tried first, {@code IMAGE_FINGERPRINT} is the
 * fallback, and the value itself (e.g. {@code c:QlpoOTFB...}) is opaque to this
 * tool --- it is printed exactly as the engine returns it.
 */
public final class CanvasConversionTests
{
	@Test
	public void theV2FingerprintIsUsedWhenPresent()
	{
		String line = CanvasConversion.formatFingerprint("sample.docx", 1, "c:v2value", "c:v1value");

		assertEquals("sample.docx: page=1 fingerprint=c:v2value", line);
	}

	@Test
	public void theV1FingerprintIsTheFallback()
	{
		String line = CanvasConversion.formatFingerprint("sample.docx", 2, "", "c:v1value");

		assertEquals("sample.docx: page=2 fingerprint=c:v1value", line);
	}

	@Test
	public void aNullV2FallsBackToV1TheSameWayAsEmpty()
	{
		String line = CanvasConversion.formatFingerprint("sample.docx", 1, null, "c:v1value");

		assertEquals("sample.docx: page=1 fingerprint=c:v1value", line);
	}

	@Test
	public void nothingIsWrittenWhenNeitherAttributeHasAValue()
	{
		assertNull(CanvasConversion.formatFingerprint("sample.docx", 1, "", ""));
		assertNull(CanvasConversion.formatFingerprint("sample.docx", 1, null, null));
	}

	@Test
	public void thePageNumberIsOneBasedAndTheDisplayNameIsUsedVerbatim()
	{
		// A subfile's display name already carries its container, e.g.
		// "bundle.zip>inner.docx" --- this only ever echoes it, never rebuilds it.
		String line = CanvasConversion.formatFingerprint("bundle.zip>inner.docx", 3, "c:abc", null);

		assertEquals("bundle.zip>inner.docx: page=3 fingerprint=c:abc", line);
	}

	// === canvas option composition =========================================
	//
	// The engine resolves a repeated key to its first occurrence, so the order here
	// is behaviour, not formatting: --canvas-options leads and the format's own
	// options fill in behind it.

	@Test
	public void aFormatWithNoOptionsOfItsOwnPassesTheUsersThroughUnchanged()
	{
		assertEquals("GRAPHIC_DPI=300", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("png"), "GRAPHIC_DPI=300"));
	}

	@Test
	public void aFormatWithNoOptionsAndNoUserOptionsComposesToNothing()
	{
		assertEquals("", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("png"), null));
		assertEquals("", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("png"), ""));
	}

	@Test
	public void aFormatsOwnOptionsApplyWhenTheUserGivesNone()
	{
		assertEquals("HTML_INLINE_IMAGES=ON", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("html5"), null));
	}

	@Test
	public void theUsersOptionsComeFirstSoTheyWinTheKeysTheyName()
	{
		// Not cosmetic: first occurrence wins in the engine, so this is what makes
		// --canvas-options HTML_INLINE_IMAGES=OFF actually take effect.
		String composed = CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("html5"), "HTML_INLINE_IMAGES=OFF");

		assertEquals("HTML_INLINE_IMAGES=OFF;HTML_INLINE_IMAGES=ON", composed);
		assertTrue(composed.startsWith("HTML_INLINE_IMAGES=OFF"));
	}

	@Test
	public void anUnrelatedUserOptionLeavesTheFormatsOwnInPlace()
	{
		assertEquals("GRAPHIC_DPI=300;HTML_INLINE_IMAGES=ON",
			CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("html5"), "GRAPHIC_DPI=300"));
	}

	@Test
	public void trailingSeparatorsDoNotProduceAnEmptySetting()
	{
		// The catalogue writes its options with a trailing ';' and users often do
		// too; an empty setting between two ';' is not worth relying on the engine
		// to ignore.
		assertEquals("GRAPHIC_DPI=300;HTML_INLINE_IMAGES=ON",
			CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("html5"), "GRAPHIC_DPI=300;"));
	}

	@Test
	public void theInliningFormatsCarryTheirSwitch()
	{
		// Both canvases otherwise write the page raster to a sibling file that this
		// tool never emits, leaving a transparent text layer over nothing. Confirmed
		// against isys_doc2text, which forces the same two and reproduces the blank
		// output exactly when told HTML_INLINE_IMAGES=OFF.
		assertEquals("HTML_INLINE_IMAGES=ON", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("html5"), null));
		assertEquals("SVG_INLINE_IMAGES=ON", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("svg"), null));
	}

	@Test
	public void mdastStillCarriesTheSchemaThatTellsItApartFromJson()
	{
		assertEquals("JSON_OUTPUT_SCHEMA=MDAST", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("mdast"), null));
		assertEquals("", CanvasConversion.composeCanvasOptions(FormatCatalog.resolve("json"), null));
	}

	// -------------------------------------------------------------- --stdout

	/**
	 * A write-only, forward-only sink, the way standard output actually behaves.
	 *
	 * <p>An {@link OutputStream} cannot be seeked in Java at all, so the property the
	 * other ports pin with a stream that throws on seek holds here by construction;
	 * what is left to check is that the bytes arrive and the sink is flushed.
	 */
	private static final class ForwardOnlySink extends OutputStream
	{
		int flushCount;
		int bytesWritten;

		@Override
		public void write(int b)
		{
			++bytesWritten;
		}

		@Override
		public void write(byte[] buffer, int offset, int count)
		{
			bytesWritten += count;
		}

		@Override
		public void flush()
		{
			++flushCount;
		}
	}

	@Test
	public void streamingToStdoutNeverSeeksTheDestinationAndFlushesItOnceDone() throws IOException
	{
		// Reproduces a real bug found right after the --stdout fix: handing standard
		// output straight to the canvas produced no error and no output. Every
		// published sample that streams a canvas renders into a memory buffer
		// instead, because finishing a binary format means seeking back to patch a
		// header computed only once rendering is done --- and the console's own
		// stream is forward-only. convertToStream renders into its own seekable
		// buffer and only copies the finished bytes across.
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.addDocument("a.docx", "text");
		CanvasConversion conversion = new CanvasConversion(DocumentFiltersSession.forEngine(engine), new RecordingConsole());
		ForwardOnlySink destination = new ForwardOnlySink();
		PlannedItem item = new PlannedItem();
		item.sourcePath = "a.docx";
		item.displayName = "a.docx";
		item.pages = PageRange.ALL;

		try (IDocument document = engine.openDocument("a.docx", 0, "", () -> null, null, null, null))
		{
			conversion.convertToStream(document, destination, item, FormatCatalog.resolve("html5"), new ConvertOptions());
		}

		assertTrue(destination.flushCount > 0);
	}
}
