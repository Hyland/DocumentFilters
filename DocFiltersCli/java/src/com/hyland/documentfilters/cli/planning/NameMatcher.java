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

package com.hyland.documentfilters.cli.planning;

import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.UsageError;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

/**
 * Path selection from {@code -g/--glob} or {@code --regex}.
 *
 * <p>Glob is the default idiom. Patterns are matched against the path as written,
 * using forward slashes on every platform so that a pattern is portable.
 * {@code *} matches within one path segment, {@code **} crosses segments, and a
 * leading {@code !} turns a pattern into an exclusion.
 *
 * <p>Multiple patterns combine as: an entry is kept when it matches at least one
 * include pattern (or there are none) and matches no exclude pattern.
 *
 * <p><b>The flags are what make a pattern mean what it means in the reference.</b>
 * {@link Pattern#UNICODE_CHARACTER_CLASS}, because .NET's {@code \d}, {@code \w},
 * {@code \s} and {@code \b} are Unicode-aware and Java's are ASCII by default;
 * {@link Pattern#UNIX_LINES}, because .NET's {@code .} excludes only {@code \n} and
 * Java's also excludes {@code \r}, U+0085, U+2028 and U+2029. The anchor is
 * {@code \z}, as there: {@code $} would accept a trailing newline.
 */
public final class NameMatcher
{
	private static final int FLAGS = Pattern.UNICODE_CHARACTER_CLASS | Pattern.UNIX_LINES;

	private List<String> _globPatterns;
	private List<String> _regexPatterns;

	private final List<Pattern> _includes = new ArrayList<>();
	private final List<Pattern> _excludes = new ArrayList<>();

	private NameMatcher()
	{
	}

	/**
	 * Builds a matcher from one or more {@code -g/--glob} patterns.
	 *
	 * @throws UsageError a pattern is malformed.
	 */
	public static NameMatcher fromGlobs(List<String> patterns)
	{
		NameMatcher matcher = new NameMatcher();
		matcher._globPatterns = new ArrayList<>(patterns);
		for (String raw : patterns)
		{
			boolean isExclude = raw.startsWith("!");
			String body = isExclude ? raw.substring(1) : raw;
			Pattern regex = compileGlob(body, raw);
			(isExclude ? matcher._excludes : matcher._includes).add(regex);
		}
		return matcher;
	}

	/**
	 * Builds a matcher from one or more {@code --regex} patterns.
	 *
	 * @throws UsageError a pattern is not a valid regular expression.
	 */
	public static NameMatcher fromRegexes(List<String> patterns)
	{
		NameMatcher matcher = new NameMatcher();
		matcher._regexPatterns = new ArrayList<>(patterns);
		for (String raw : patterns)
		{
			try
			{
				// Wrapped in ^(?: ... )\z so that it is anchored to the beginning and end
				// of the string, in a non-capturing group so that a top-level
				// alternation (|) stays inside the anchors.
				matcher._includes.add(Pattern.compile("^(?:" + raw + ")\\z", FLAGS));
			}
			catch (PatternSyntaxException e)
			{
				throw new UsageError(ErrorCodes.BAD_VALUE, "invalid regular expression '" + raw + "'");
			}
		}
		return matcher;
	}

	/** True when the path should be processed. */
	public boolean matches(String path)
	{
		String normalized = path.replace('\\', '/');

		for (Pattern regex : _excludes)
		{
			if (regex.matcher(normalized).find())
				return false;
		}

		if (_includes.isEmpty())
			return true;
		for (Pattern regex : _includes)
		{
			if (regex.matcher(normalized).find())
				return true;
		}
		return false;
	}

	/** Translates one {@code -g/--glob} pattern (minus any leading {@code !}) into an anchored regex. */
	private static Pattern compileGlob(String pattern, String original)
	{
		List<String> segments = new ArrayList<>();
		for (String segment : ValueParsers.split(pattern, '/'))
		{
			if (segment.equals("**") && !segments.isEmpty() && segments.get(segments.size() - 1).equals("**"))
				continue;
			segments.add(segment);
		}

		StringBuilder sb = new StringBuilder("^");
		for (int i = 0; i < segments.size(); i++)
		{
			String segment = segments.get(i);
			boolean isFirst = i == 0;
			boolean isLast = i == segments.size() - 1;
			boolean previousIsDoubleStar = !isFirst && segments.get(i - 1).equals("**");

			if (segment.equals("**"))
			{
				if (!isFirst)
					sb.append('/');
				sb.append(isLast ? ".*" : "(?:.*/)?");
			}
			else
			{
				if (!isFirst && !previousIsDoubleStar)
					sb.append('/');
				sb.append(translateGlobSegment(segment, original));
			}
		}
		sb.append("\\z$");

		try
		{
			return Pattern.compile(sb.toString(), FLAGS);
		}
		catch (PatternSyntaxException e)
		{
			throw new UsageError(ErrorCodes.BAD_VALUE, "invalid glob pattern '" + original + "'");
		}
	}

	/**
	 * Translates one path segment: {@code *} and {@code ?} stay within the segment,
	 * everything else is literal --- including a bracket pair, which this tool's
	 * glob dialect gives no matching meaning to, so {@code [abc]} matches only that
	 * literal five-character text. An unterminated {@code [} is still rejected as
	 * malformed rather than silently taken as a literal character: it is almost
	 * certainly a typo, not an intentional five-character filename fragment.
	 */
	private static String translateGlobSegment(String segment, String original)
	{
		StringBuilder sb = new StringBuilder();
		int i = 0;
		while (i < segment.length())
		{
			char c = segment.charAt(i);
			switch (c)
			{
				case '*':
					sb.append("[^/]*");
					i += 1;
					break;
				case '?':
					sb.append("[^/]");
					i += 1;
					break;
				case '[':
					int close = segment.indexOf(']', i + 1);
					if (close < 0)
						throw new UsageError(ErrorCodes.BAD_VALUE, "invalid glob pattern '" + original + "'");
					sb.append(escape(segment.substring(i, close + 1)));
					i = close + 1;
					break;
				default:
					sb.append(escape(String.valueOf(c)));
					i += 1;
					break;
			}
		}
		return sb.toString();
	}

	/**
	 * Literal text as a regex. Every character that is not a letter or a digit is
	 * backslash-escaped, which Java reads as that character --- unlike
	 * {@link Pattern#quote}, whose {@code \Q...\E} would need care where a
	 * fragment itself contains {@code \E}.
	 */
	private static String escape(String text)
	{
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < text.length(); i++)
		{
			char c = text.charAt(i);
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c > 0x7f)
				sb.append(c);
			else
				sb.append('\\').append(c);
		}
		return sb.toString();
	}

	/** The canonical text form, used by the conformance corpus. */
	@Override
	public String toString()
	{
		if (_globPatterns != null)
			return "glob " + String.join(", ", _globPatterns);
		if (_regexPatterns != null)
			return "regex " + String.join(", ", _regexPatterns);
		return "";
	}
}
