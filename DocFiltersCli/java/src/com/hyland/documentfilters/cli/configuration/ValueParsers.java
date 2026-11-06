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

import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.UsageError;
import java.util.ArrayList;
import java.util.List;

/**
 * Turns command line text into typed values.
 *
 * <p>Every conversion that can fail lives here rather than in the argument
 * parser, so that a bad value produces the same message and the same exit code
 * in the C#, Python, Java and C++17 builds.
 *
 * <p><b>Not {@code Integer.parseInt}.</b> That accepts a leading {@code +} or
 * {@code -} and any Unicode decimal digit, so {@code --max-depth +3} and an
 * Arabic-Indic three would both be accepted where the reference rejects them.
 * {@link #tryParseDigits} accepts ASCII digits and nothing else.
 */
public final class ValueParsers
{
	private ValueParsers()
	{
	}

	/** Parses a non-negative integer, e.g. for {@code --max-depth}. */
	public static int parseCount(String argument, String text)
	{
		Long value = tryParseDigits(text, Integer.MAX_VALUE);
		if (value == null)
			throw badValue(argument, text);
		return value.intValue();
	}

	/** Parses a positive integer of seconds, e.g. for {@code --timeout}. */
	public static int parseSeconds(String argument, String text)
	{
		Long value = tryParseDigits(text, Integer.MAX_VALUE);
		if (value == null || value <= 0)
			throw badValue(argument, text);
		return value.intValue();
	}

	/** Parses a size in megabytes, e.g. for {@code --max-memory}. */
	public static long parseMegabytes(String argument, String text)
	{
		Long value = tryParseDigits(text, Long.MAX_VALUE);
		if (value == null || value <= 0)
			throw badValue(argument, text);
		return value;
	}

	/**
	 * Splits a semicolon-separated id list, e.g. {@code --include-types "PDF;DOCX"}.
	 * Empty entries are dropped; surrounding whitespace is trimmed.
	 */
	public static List<String> parseIdList(String text)
	{
		List<String> result = new ArrayList<>();
		for (String part : split(text, ';'))
		{
			String trimmed = trim(part);
			if (!trimmed.isEmpty())
				result.add(trimmed);
		}
		return result;
	}

	/** Parses one {@code --redact page,x,y,w,h} value. */
	public static RedactSpec parseRedaction(String text)
	{
		List<String> parts = split(text, ',');
		if (parts.size() != 5)
			throw badValue("--redact", text);

		int[] values = new int[5];
		for (int i = 0; i < 5; i++)
		{
			Long value = tryParseDigits(parts.get(i), Integer.MAX_VALUE);
			if (value == null)
				throw badValue("--redact", text);
			values[i] = value.intValue();
		}

		RedactSpec spec = new RedactSpec();
		spec.page = values[0];
		spec.x = values[1];
		spec.y = values[2];
		spec.width = values[3];
		spec.height = values[4];
		return spec;
	}

	/**
	 * Parses one {@code --form page,name=value} value.
	 *
	 * <p>Only the first {@code =} separates name from value, so values may contain {@code =}.
	 */
	public static FormValue parseFormValue(String text)
	{
		int comma = text.indexOf(',');
		if (comma < 0)
			throw badValue("--form", text);

		String pageText = text.substring(0, comma);
		String rest = text.substring(comma + 1);

		int equals = rest.indexOf('=');
		if (equals < 0)
			throw badValue("--form", text);

		Long page = tryParseDigits(pageText, Integer.MAX_VALUE);
		if (page == null)
			throw badValue("--form", text);

		FormValue value = new FormValue();
		value.page = page.intValue();
		value.name = rest.substring(0, equals);
		value.value = rest.substring(equals + 1);
		return value;
	}

	/** Parses {@code --on-error}: "skip" or "stop", after {@link #foldToken}. */
	public static OnErrorBehaviour parseOnError(String text)
	{
		switch (text)
		{
			case "skip":
				return OnErrorBehaviour.SKIP;
			case "stop":
				return OnErrorBehaviour.STOP;
			default:
				throw badValue("--on-error", text);
		}
	}

	/**
	 * Folds a value drawn from a vocabulary this tool publishes, so that PDF, Pdf
	 * and pdf are one token.
	 *
	 * <p>Applied by {@code --to}, {@code --on-error}, {@code --applies-to} and the
	 * two log-level arguments --- every argument whose valid values are enumerated
	 * here rather than supplied by the engine or the user. Paths, globs, regular
	 * expressions, {@code --out-name} templates, {@code --filter} text, passwords,
	 * {@code --codepage} and the {@code --options} string are used exactly as typed.
	 *
	 * <p><b>ASCII only, and that is the whole point.</b> {@code String.toLowerCase()}
	 * maps I to the dotless i under a Turkish locale, so {@code --to TIFF} would stop
	 * resolving on a Turkish machine. This maps A-Z to a-z and leaves every other
	 * character alone.
	 */
	public static String foldToken(String text)
	{
		char[] chars = text.toCharArray();
		for (int i = 0; i < chars.length; i++)
		{
			if (chars[i] >= 'A' && chars[i] <= 'Z')
				chars[i] = (char)(chars[i] - 'A' + 'a');
		}
		return new String(chars);
	}

	/**
	 * The value of {@code text} when it is one or more ASCII digits and no more
	 * than {@code max}, else null. No sign, no whitespace, no other digit script:
	 * what {@code NumberStyles.None} accepts in the reference.
	 */
	public static Long tryParseDigits(String text, long max)
	{
		if (text == null || text.isEmpty())
			return null;

		long value = 0;
		for (int i = 0; i < text.length(); i++)
		{
			char c = text.charAt(i);
			if (c < '0' || c > '9')
				return null;
			int digit = c - '0';
			if (value > (max - digit) / 10)
				return null;
			value = value * 10 + digit;
		}
		return value;
	}

	/**
	 * Whether {@code c} is whitespace as the reference's {@code char.IsWhiteSpace}
	 * defines it.
	 *
	 * <p>Not {@code Character.isWhitespace}, which counts the four information
	 * separators U+001C to U+001F and leaves out the no-break spaces U+00A0, U+2007
	 * and U+202F. The definition here is the Unicode one: the space, line and
	 * paragraph separator categories, the C0 controls U+0009 to U+000D, and U+0085.
	 */
	public static boolean isWhiteSpace(char c)
	{
		if ((c >= '\t' && c <= '\r') || c == '\u0085')
			return true;
		switch (Character.getType(c))
		{
			case Character.SPACE_SEPARATOR:
			case Character.LINE_SEPARATOR:
			case Character.PARAGRAPH_SEPARATOR:
				return true;
			default:
				return false;
		}
	}

	/**
	 * {@code text} without leading and trailing {@link #isWhiteSpace} characters.
	 * Not {@code String.trim()}, which removes everything up to U+0020 and nothing
	 * above it.
	 */
	public static String trim(String text)
	{
		int start = 0;
		int end = text.length();
		while (start < end && isWhiteSpace(text.charAt(start)))
			++start;
		while (end > start && isWhiteSpace(text.charAt(end - 1)))
			--end;
		return text.substring(start, end);
	}

	/** True for null, empty, or nothing but {@link #isWhiteSpace} characters. */
	public static boolean isNullOrWhiteSpace(String text)
	{
		return text == null || trim(text).isEmpty();
	}

	/**
	 * Splits on every {@code separator}, keeping empty parts, as the reference's
	 * {@code string.Split} does. Not {@code String.split}, which takes a regular
	 * expression and drops trailing empty parts.
	 */
	public static List<String> split(String text, char separator)
	{
		List<String> parts = new ArrayList<>();
		int start = 0;
		for (int i = 0; i < text.length(); i++)
		{
			if (text.charAt(i) == separator)
			{
				parts.add(text.substring(start, i));
				start = i + 1;
			}
		}
		parts.add(text.substring(start));
		return parts;
	}

	private static UsageError badValue(String argument, String text)
	{
		return new UsageError(ErrorCodes.BAD_VALUE, argument + " does not accept the value '" + text + "'");
	}
}
