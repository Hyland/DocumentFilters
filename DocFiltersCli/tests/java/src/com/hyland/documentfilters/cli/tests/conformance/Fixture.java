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

package com.hyland.documentfilters.cli.tests.conformance;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

/**
 * Reader for the shared conformance fixtures.
 *
 * <p>Every fixture uses one format, so each language needs exactly one reader rather
 * than one per file. A case is a {@code $} subject line, then any {@code <} lines
 * saying what the case is given, then {@code =} lines saying what it must produce.
 * Blank lines separate cases and {@code #} starts a comment.
 *
 * <p>Deliberately free of JSON and YAML so that every suite can parse it with nothing
 * but its standard library.
 */
public final class Fixture
{
	private Fixture()
	{
	}

	/**
	 * Where the fixtures live: {@code tests/conformance}, found by walking up from the
	 * working directory, so the suite runs from {@code tests/java}, from the
	 * repository root, or from anywhere beneath either.
	 *
	 * @throws IllegalStateException no ancestor of the working directory holds the
	 *     fixtures.
	 */
	public static Path directory()
	{
		Path start = Paths.get(System.getProperty("user.dir")).toAbsolutePath();
		for (Path candidate = start; candidate != null; candidate = candidate.getParent())
		{
			for (Path directory : Arrays.asList(candidate.resolve("conformance"), candidate.resolve("tests").resolve("conformance")))
			{
				if (Files.isRegularFile(directory.resolve(PlanCorpus.FILE_NAME)))
					return directory;
			}
		}
		throw new IllegalStateException("conformance fixtures not found above " + start);
	}

	/** Loads a fixture by filename, e.g. {@code formats.txt}. */
	public static List<FixtureCase> load(String name)
	{
		Path path = directory().resolve(name);
		if (!Files.isRegularFile(path))
			throw new IllegalStateException("conformance fixture not found: " + path);
		try
		{
			return parse(name, Files.readAllLines(path, StandardCharsets.UTF_8));
		}
		catch (IOException e)
		{
			throw new UncheckedIOException(e);
		}
	}

	/** Parses fixture text. Exposed so the reader itself can be tested. */
	public static List<FixtureCase> parse(String file, List<String> lines)
	{
		List<FixtureCase> cases = new ArrayList<>();
		FixtureCase current = null;
		for (int i = 0; i < lines.size(); i++)
		{
			String line = lines.get(i).trim();
			if (line.startsWith("#"))
				continue;

			if (line.isEmpty())
			{
				// A blank line ends the current case, as the format documents. Merely
				// skipping it would silently attach a later '=' line to the case above
				// the gap, which a stricter reader in another language would reject,
				// so the same fixture would mean different things in different ports.
				current = null;
				continue;
			}

			if (line.startsWith("$"))
			{
				current = new FixtureCase();
				current.subject = line.substring(1).trim();
				current.file = file;
				current.line = i + 1;
				cases.add(current);
				continue;
			}

			if (line.startsWith("<"))
			{
				if (current == null)
					throw new FixtureFormatException(file + ":" + (i + 1) + ": '<' line with no preceding '$' case");
				if (!current.expectations.isEmpty())
				{
					throw new FixtureFormatException(
						file + ":" + (i + 1) + ": '<' describes what the case is given and comes before every '=' line");
				}
				current.given.add(splitKeyValue(line));
				continue;
			}

			if (line.startsWith("="))
			{
				if (current == null)
					throw new FixtureFormatException(file + ":" + (i + 1) + ": expectation with no preceding '$' case");
				current.expectations.add(splitKeyValue(line));
				continue;
			}

			throw new FixtureFormatException(file + ":" + (i + 1) + ": unrecognised line: " + line);
		}
		return cases;
	}

	/** {@link #parse(String, List)} over an array, for the reader's own tests. */
	public static List<FixtureCase> parse(String file, String... lines)
	{
		return parse(file, Arrays.asList(lines));
	}

	/**
	 * Splits a {@code <} or {@code =} line into its key and its value.
	 *
	 * <p>Exactly one space separates the key from the value; everything after it is
	 * stored verbatim. Callers that do not care about the column padding use
	 * get/getAll, which trim. getRaw/getAllRaw exist for a value whose own leading
	 * whitespace is part of what it asserts, and must survive the reader.
	 */
	private static Map.Entry<String, String> splitKeyValue(String line)
	{
		String rest = trimStart(line.substring(1));
		int space = rest.indexOf(' ');
		String key = space < 0 ? rest : rest.substring(0, space);
		String value = space < 0 ? "" : rest.substring(space + 1);
		return new AbstractMap.SimpleImmutableEntry<>(key, value);
	}

	private static String trimStart(String text)
	{
		int start = 0;
		while (start < text.length() && Character.isWhitespace(text.charAt(start)))
			start++;
		return text.substring(start);
	}
}
