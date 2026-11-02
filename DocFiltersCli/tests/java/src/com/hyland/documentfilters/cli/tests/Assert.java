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

package com.hyland.documentfilters.cli.tests;

import java.util.Objects;

/**
 * The assertions the suite uses. A failure throws {@link AssertionError}, which
 * {@link TestRunner} reports as a failed test; any other exception is reported as
 * an error.
 */
public final class Assert
{
	/** The body of an {@link #assertThrows} call: anything that may throw. */
	public interface ThrowingRunnable
	{
		void run() throws Exception;
	}

	private Assert()
	{
	}

	public static void fail(String message)
	{
		throw new AssertionError(message);
	}

	public static void assertTrue(boolean condition)
	{
		assertTrue(condition, "expected true");
	}

	public static void assertTrue(boolean condition, String message)
	{
		if (!condition)
			fail(message);
	}

	public static void assertFalse(boolean condition)
	{
		assertFalse(condition, "expected false");
	}

	public static void assertFalse(boolean condition, String message)
	{
		if (condition)
			fail(message);
	}

	public static void assertNull(Object actual)
	{
		if (actual != null)
			fail("expected null, was " + describe(actual));
	}

	public static void assertNotNull(Object actual)
	{
		if (actual == null)
			fail("expected a value, was null");
	}

	public static void assertEquals(Object expected, Object actual)
	{
		assertEquals(expected, actual, null);
	}

	public static void assertEquals(Object expected, Object actual, String context)
	{
		// Objects.equals, so that a long compares with a long: the boxed overloads
		// below exist so that assertEquals(1, someLong) does not box the 1 as an
		// Integer and fail against a Long that holds the same value.
		if (!Objects.equals(expected, actual))
			fail((context == null ? "" : context + ": ") + "expected " + describe(expected) + ", was " + describe(actual));
	}

	public static void assertEquals(long expected, long actual)
	{
		if (expected != actual)
			fail("expected " + expected + ", was " + actual);
	}

	public static void assertNotEquals(Object unexpected, Object actual)
	{
		if (Objects.equals(unexpected, actual))
			fail("expected anything but " + describe(unexpected));
	}

	public static void assertSame(Object expected, Object actual)
	{
		if (expected != actual)
			fail("expected the same instance as " + describe(expected) + ", was " + describe(actual));
	}

	/** Asserts that {@code text} contains {@code fragment}. */
	public static void assertContains(String fragment, String text)
	{
		if (text == null || !text.contains(fragment))
			fail("expected " + describe(fragment) + " in " + describe(text));
	}

	/** Asserts that {@code text} does not contain {@code fragment}. */
	public static void assertDoesNotContain(String fragment, String text)
	{
		if (text != null && text.contains(fragment))
			fail("expected no " + describe(fragment) + " in " + describe(text));
	}

	/**
	 * Asserts that {@code body} throws exactly {@code type} --- not a subclass, as
	 * for xUnit's {@code Assert.Throws} --- and returns what it threw.
	 */
	public static <T extends Throwable> T assertThrows(Class<T> type, ThrowingRunnable body)
	{
		try
		{
			body.run();
		}
		catch (Throwable thrown)
		{
			if (thrown.getClass() == type)
				return type.cast(thrown);
			AssertionError error = new AssertionError("expected " + type.getName() + ", threw " + thrown);
			error.initCause(thrown);
			throw error;
		}
		throw new AssertionError("expected " + type.getName() + ", nothing was thrown");
	}

	private static String describe(Object value)
	{
		if (value instanceof String)
			return "'" + value + "'";
		return String.valueOf(value);
	}
}
