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

import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.execution.MetadataReport;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocumentFiltersEngine;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * How {@code meta} turns the engine's metadata blob into one record per field.
 *
 * <p>There is no structured metadata API --- {@code getText} returns the fields as
 * one run of text, and the record separator is not a newline. The engine emits
 * {@code 0x0F} before the block and {@code 0x0E} between records, which is why
 * these fixtures are built from control codes rather than {@code \n}: a parser
 * that splits on newlines finds no boundary at all and reports every field of a
 * document as one.
 */
public final class MetadataReportTests
{
	private static final char START_OF_META = '\u000F';
	private static final char SEPARATOR = '\u000E';
	private static final char END_OF_META = '\u0010';

	private static List<String> report(String metadata)
	{
		FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
		engine.addDocument("a.docx", metadata);
		RecordingConsole console = new RecordingConsole();

		try (IDocument document = engine.openDocument("a.docx", 0, "", () -> null, null, null, null))
		{
			new MetadataReport().writeText(document, "a.docx", console);
		}

		return console.outLines;
	}

	@Test
	public void recordsSeparatedByControlCodesEachBecomeOneLine()
	{
		// The real shape, from a PDF: 0x0F opens the block, 0x0E divides it, 0x10
		// closes it. Splitting this on '\n' yields a single bogus field whose label
		// still carries the 0x0F.
		List<String> lines = report(START_OF_META + "Title: Report" + SEPARATOR + "Author: Ada" + SEPARATOR
			+ "Producer: Quartz" + END_OF_META);

		assertEquals(Arrays.asList(
			"a.docx: field=Title value=Report",
			"a.docx: field=Author value=Ada",
			"a.docx: field=Producer value=Quartz"), lines);
	}

	@Test
	public void aValueCarryingANewlineStaysOneRecord()
	{
		// A Word Title or Comments field routinely has one; the continuation used to
		// be dropped, silently truncating the value.
		List<String> lines = report(START_OF_META + "Title: Quarterly Report\nsecond line" + SEPARATOR + "Author: Ada");

		assertEquals(Arrays.asList(
			"a.docx: field=Title value=Quarterly Report second line",
			"a.docx: field=Author value=Ada"), lines);
	}

	@Test
	public void carriageReturnsAndTabsInsideAValueBecomeSingleSpaces()
	{
		List<String> lines = report(START_OF_META + "Comments: one\ttwo\r\nthree");

		assertEquals(Collections.singletonList("a.docx: field=Comments value=one two three"), lines);
	}

	@Test
	public void aLabelContainingColonsIsNotCutApart()
	{
		// Splitting on the first ": " rather than the first ':' is what keeps an XMP
		// label whole --- none of its internal colons is followed by a space.
		List<String> lines = report(START_OF_META + "xmp:dc:creator: slop001");

		assertEquals(Collections.singletonList("a.docx: field=xmp:dc:creator value=slop001"), lines);
	}

	@Test
	public void aDocumentWithNoMetadataReportsNothing()
	{
		assertEquals(Collections.emptyList(), report(""));
		assertEquals(Collections.emptyList(), report(String.valueOf(START_OF_META) + END_OF_META));
	}

	@Test
	public void aContinuationWithNoFieldBeforeItIsIgnored()
	{
		// Nothing to attach it to, so it is dropped rather than inventing a field.
		assertEquals(Collections.emptyList(), report("loose text with no label"));
	}
}
