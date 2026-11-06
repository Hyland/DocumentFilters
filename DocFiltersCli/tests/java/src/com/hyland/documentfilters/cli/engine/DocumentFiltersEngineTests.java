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

package com.hyland.documentfilters.cli.engine;

import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertSame;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;

import com.hyland.documentfilters.cli.tests.Test;
import com.perceptive.documentfilters.IGRException;
import java.io.IOException;

/**
 * The parts of the real engine that are reachable with no native library loaded.
 *
 * <p>The engine is otherwise untestable here and deliberately untested --- it needs
 * the native library and a license, which is the whole reason every outcome is
 * tested against the fake instead. What is left are the pure helpers it uses to
 * talk to the binding: reading the library's version out of its own image, and
 * translating what the binding throws.
 *
 * <p>In the engine's package because those helpers are package-private, which
 * also keeps it out of the test runner's discovery: run it on its own.
 *
 * <p>Not covered: {@code capabilities}, which composes the capability bits from
 * the binding's three booleans. It needs an {@code Extractor}, and constructing one
 * --- even a subclass over a null handle --- calls into the native library.
 */
public final class DocumentFiltersEngineTests
{
	/** A little-endian {@code VS_FIXEDFILEINFO} head at {@code offset} in a buffer of {@code length}. */
	private static byte[] image(int length, int offset, long signature, long structVersion, long most, long least)
	{
		byte[] image = new byte[length];
		for (int i = 0; i < length; i++)
			image[i] = (byte) (i * 31 + 7);

		putUInt32(image, offset, signature);
		putUInt32(image, offset + 4, structVersion);
		putUInt32(image, offset + 8, most);
		putUInt32(image, offset + 12, least);
		return image;
	}

	private static void putUInt32(byte[] data, int offset, long value)
	{
		if (offset + 4 > data.length)
			return;

		for (int i = 0; i < 4; i++)
			data[offset + i] = (byte) (value >>> (8 * i));
	}

	// ============================================ the library version

	@Test
	public void theFileVersionIsReadFromTheFixedFileInfo()
	{
		byte[] library = image(256, 100, 0xFEEF04BDL, 0x00010000L, (25L << 16) | 2, (345L << 16) | 6);

		assertEquals("25.2.345.6", DocumentFiltersEngine.readFileVersion(library));
	}

	@Test
	public void eachPartIsReadUnsigned()
	{
		// The high half of each word is the more significant part; a signed read
		// would turn a part of 32768 or more negative.
		byte[] library = image(64, 8, 0xFEEF04BDL, 0x00010000L, (0xFFFFL << 16) | 0x8000, 0);

		assertEquals("65535.32768.0.0", DocumentFiltersEngine.readFileVersion(library));
	}

	@Test
	public void anImageWithNoVersionBlockHasNoVersion()
	{
		assertNull(DocumentFiltersEngine.readFileVersion(new byte[0]));
		assertNull(DocumentFiltersEngine.readFileVersion(new byte[64]));
	}

	@Test
	public void aSignatureWithTheWrongStructureVersionIsNotTakenForTheBlock()
	{
		// The signature alone can turn up in code or data; the structure version
		// that follows it is what says this really is the block.
		byte[] library = image(128, 16, 0xFEEF04BDL, 0x00020000L, (25L << 16) | 2, 0);

		assertNull(DocumentFiltersEngine.readFileVersion(library));
	}

	@Test
	public void aStrayMatchEarlierInTheImageIsSkipped()
	{
		byte[] library = image(256, 160, 0xFEEF04BDL, 0x00010000L, (25L << 16) | 2, (345L << 16) | 6);
		putUInt32(library, 40, 0xFEEF04BDL);

		assertEquals("25.2.345.6", DocumentFiltersEngine.readFileVersion(library));
	}

	@Test
	public void aBlockCutShortByTheEndOfTheImageIsNotRead()
	{
		// Twelve bytes from the end: the signature and structure version fit, the
		// version words do not.
		byte[] library = image(64, 52, 0xFEEF04BDL, 0x00010000L, (25L << 16) | 2, 0);

		assertNull(DocumentFiltersEngine.readFileVersion(library));
	}

	// ============================================ the binding's messages

	@Test
	public void theJniErrorCodeSuffixIsRemoved()
	{
		// The other bindings hand the engine's text over as it is; this one appends
		// the code, which would be the one difference in an otherwise identical line.
		IGRException error = new IGRException(5, "The document is password protected, error code 5");

		assertEquals("The document is password protected", DocumentFiltersEngine.EngineCall.messageOf(error));
	}

	@Test
	public void aSuffixNamingADifferentCodeIsLeftAlone()
	{
		// Only this exception's own code is the binding's suffix; anything else is
		// the engine's text and is not this tool's to edit.
		IGRException error = new IGRException(5, "Retry failed, error code 6");

		assertEquals("Retry failed, error code 6", DocumentFiltersEngine.EngineCall.messageOf(error));
	}

	@Test
	public void aMessageThatIsOnlyTheSuffixIsLeftAlone()
	{
		// Stripping it would leave nothing to print.
		IGRException error = new IGRException(5, ", error code 5");

		assertEquals(", error code 5", DocumentFiltersEngine.EngineCall.messageOf(error));
	}

	@Test
	public void aMessageWithTheSuffixInTheMiddleIsLeftAlone()
	{
		IGRException error = new IGRException(5, "Failed, error code 5 while reading");

		assertEquals("Failed, error code 5 while reading", DocumentFiltersEngine.EngineCall.messageOf(error));
	}

	@Test
	public void anExceptionWithNoMessageHasNoMessage()
	{
		assertNull(DocumentFiltersEngine.EngineCall.messageOf(new IGRException()));
	}

	// ============================================ error translation

	@Test
	public void aBindingFailureArrivesAsAnEngineExceptionCarryingItsCode()
	{
		IGRException cause = new IGRException(EngineError.PASSWORD.getValue(), "The document is password protected, error code 5");

		EngineException thrown = assertThrows(EngineException.class, () -> DocumentFiltersEngine.EngineCall.run(() ->
		{
			throw cause;
		}));

		assertEquals(EngineError.PASSWORD, thrown.getError());
		assertEquals(EngineError.PASSWORD.getValue(), thrown.getCode());
		assertEquals("The document is password protected", thrown.getMessage());
		assertSame(cause, thrown.getCause());
	}

	@Test
	public void aCodeThisBuildDoesNotKnowIsAGenericErrorThatKeepsItsNumber()
	{
		EngineException thrown = assertThrows(EngineException.class, () -> DocumentFiltersEngine.EngineCall.run(() ->
		{
			throw new IGRException(9999, "Something new");
		}));

		assertEquals(EngineError.ERROR, thrown.getError());
		assertEquals(9999, thrown.getCode());
	}

	@Test
	public void anythingElseTheBindingThrowsIsAGenericError()
	{
		// A missing native library surfaces as UnsatisfiedLinkError and a bad path as
		// an I/O exception; the caller's question is the same either way.
		IOException io = new IOException("cannot read input.docx");
		EngineException fromIo = assertThrows(EngineException.class, () -> DocumentFiltersEngine.EngineCall.run(() ->
		{
			throw io;
		}));

		assertEquals(EngineError.ERROR, fromIo.getError());
		assertEquals(EngineError.ERROR.getValue(), fromIo.getCode());
		assertEquals("cannot read input.docx", fromIo.getMessage());
		assertSame(io, fromIo.getCause());

		EngineException fromLink = assertThrows(EngineException.class, () -> DocumentFiltersEngine.EngineCall.run(() ->
		{
			throw new UnsatisfiedLinkError("no ISYS11df in java.library.path");
		}));

		assertEquals(EngineError.ERROR, fromLink.getError());
	}

	@Test
	public void anAlreadyTranslatedFailurePassesThroughUnchanged()
	{
		// A nested call inside an outer one: wrapping it again would bury the code.
		EngineException inner = new EngineException(EngineError.NOT_FOUND, EngineError.NOT_FOUND.getValue(), "missing");

		EngineException thrown = assertThrows(EngineException.class, () -> DocumentFiltersEngine.EngineCall.run(() ->
		{
			throw inner;
		}));

		assertSame(inner, thrown);
	}

	@Test
	public void aCallThatSucceedsReturnsItsValue()
	{
		assertEquals("ok", DocumentFiltersEngine.EngineCall.run(() -> "ok"));
	}

	// ============================================ bookmark destinations

	@Test
	public void aBookmarkDestinationIsAWholeDecimalPageIndexOrNothing()
	{
		assertEquals(3, DocumentFiltersEngine.DocumentAdapter.pageIndex("3"));
		assertEquals(0, DocumentFiltersEngine.DocumentAdapter.pageIndex("0"));
		assertEquals(-1, DocumentFiltersEngine.DocumentAdapter.pageIndex(null));
		assertEquals(-1, DocumentFiltersEngine.DocumentAdapter.pageIndex(""));
		assertEquals(-1, DocumentFiltersEngine.DocumentAdapter.pageIndex("3a"));
		assertEquals(-1, DocumentFiltersEngine.DocumentAdapter.pageIndex(" 3"));
		assertEquals(-1, DocumentFiltersEngine.DocumentAdapter.pageIndex("#page=3"));

		// Too large for an int: no page, rather than an overflowed one.
		assertEquals(-1, DocumentFiltersEngine.DocumentAdapter.pageIndex("99999999999"));
	}
}
