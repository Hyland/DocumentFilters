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

package com.hyland.documentfilters.cli.runtime;

import java.util.function.Predicate;

/**
 * Chooses the spelling to look for when asking a directory whether it tells
 * names apart by case.
 *
 * <p>Separate from {@link SystemFileSystem} and pure, because the one thing that
 * has to be right here is <em>where</em> the question is asked, and a real
 * filesystem cannot show that in a test: on a single volume every answer agrees,
 * so a probe aimed one directory too high passes every test a machine can run.
 * What a test can check is that the name handed back lies inside the directory
 * being measured --- which is the whole of the rule.
 *
 * <p>Why it matters: the lookup rules belong to the directory, not to the host
 * and not to the volume above it. {@code /mnt/ntfs} is case-insensitive while the
 * {@code /mnt} holding it is not, and Windows can mark a single directory
 * case-sensitive inside an otherwise insensitive volume. Measuring the parent
 * gets both of those backwards.
 */
public final class CaseProbe
{
	private CaseProbe()
	{
	}

	/**
	 * Whether {@code directory} tells names apart by case, or null when it has
	 * nothing to answer with --- the caller falls back to the host default.
	 *
	 * @param directory the directory whose rules are in question.
	 * @param entries what is already in it. Walked lazily and only as far as the
	 *        first name with a letter in it, so a directory of a hundred thousand
	 *        files costs one entry rather than a hundred thousand.
	 * @param exists whether a path is there, file or directory. Anything it cannot
	 *        answer reads as absent, which is what every port does with an
	 *        unreadable path.
	 */
	public static Boolean measure(String directory, Iterable<String> entries, Predicate<String> exists)
	{
		for (String entry : entries)
		{
			String sibling = flippedSibling(directory, entry);
			if (sibling == null)
				continue;

			return !exists.test(sibling);
		}

		return null;
	}

	/**
	 * The path to look for inside {@code directory} to measure how it matches
	 * names, or null when {@code entry} has no letter to flip and so tells the
	 * caller nothing.
	 *
	 * @param directory the directory whose rules are in question.
	 * @param entry something already in it, named or full-path. Its name is
	 *        flipped letter by letter, so {@code Report.txt} is looked for as
	 *        {@code rEPORT.TXT}: if that is found, the directory matched two
	 *        spellings to one file and is case-insensitive.
	 */
	public static String flippedSibling(String directory, String entry)
	{
		String name = nameOf(entry);
		String flipped = flipCase(name);
		if (flipped.equals(name))
			return null;

		String parent = trimSeparators(directory);
		return parent.isEmpty() ? flipped : parent + "/" + flipped;
	}

	private static String nameOf(String path)
	{
		String trimmed = trimSeparators(path);
		int slash = Math.max(trimmed.lastIndexOf('/'), trimmed.lastIndexOf('\\'));
		return slash < 0 ? trimmed : trimmed.substring(slash + 1);
	}

	private static String trimSeparators(String path)
	{
		int end = path.length();
		while (end > 0 && (path.charAt(end - 1) == '/' || path.charAt(end - 1) == '\\'))
			--end;
		return path.substring(0, end);
	}

	/**
	 * ASCII-only, and every letter rather than a lowercasing, for the same reason
	 * {@code ValueParsers.foldToken} folds ASCII only: a locale-aware fold turns
	 * 'I' into a dotless i under a Turkish locale and would probe for a name that
	 * exists under neither rule.
	 */
	private static String flipCase(String text)
	{
		char[] chars = text.toCharArray();
		for (int i = 0; i < chars.length; i++)
		{
			if (chars[i] >= 'a' && chars[i] <= 'z')
				chars[i] = (char)(chars[i] - 'a' + 'A');
			else if (chars[i] >= 'A' && chars[i] <= 'Z')
				chars[i] = (char)(chars[i] - 'A' + 'a');
		}

		return new String(chars);
	}
}
