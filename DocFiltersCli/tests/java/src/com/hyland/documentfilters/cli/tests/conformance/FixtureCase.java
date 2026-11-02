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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * One case from a shared fixture: a subject, the world it runs in, and its
 * expectations.
 */
public final class FixtureCase
{
	/** The text after {@code $}: a command line, a format token, a template. */
	public String subject;

	/**
	 * The {@code <} lines: what the case is given, in fixture order. Keys may repeat.
	 *
	 * <p>Held apart from {@link #expectations} rather than distinguished by key name,
	 * because they are the opposite kind of thing: {@code <} describes the world
	 * before the run and {@code =} describes what the run must produce. A reader that
	 * mixed them would make every consumer filter by a list of key names it had to
	 * keep in step.
	 */
	public final List<Map.Entry<String, String>> given = new ArrayList<>();

	/** The {@code =} lines, in fixture order. Keys may repeat. */
	public final List<Map.Entry<String, String>> expectations = new ArrayList<>();

	/** Fixture filename, for failure messages. */
	public String file;

	/** 1-based line of the {@code $} line. */
	public int line;

	/** The expected error code, when the case expects a failure; otherwise null. */
	public String getError()
	{
		return get("error");
	}

	/**
	 * The single value for a key, trimmed, or null when the key is absent.
	 *
	 * <p>Values are stored verbatim and trimmed here, because fixtures pad them into
	 * a readable column and almost no case cares about that padding. The exceptions
	 * use {@link #getRaw}, for a value whose own leading whitespace is part of what it
	 * asserts.
	 */
	public String get(String key)
	{
		String value = getRaw(key);
		return value == null ? null : value.trim();
	}

	/** The single value for a key exactly as written, padding included. */
	public String getRaw(String key)
	{
		for (Map.Entry<String, String> pair : expectations)
		{
			if (pair.getKey().equals(key))
				return pair.getValue();
		}
		return null;
	}

	/** Every value for a repeated key, trimmed, in fixture order. */
	public List<String> getAll(String key)
	{
		List<String> result = new ArrayList<>();
		for (String value : getAllRaw(key))
			result.add(value.trim());
		return result;
	}

	/**
	 * Every value for a repeated key exactly as written, with one separating space
	 * removed and everything beyond it kept.
	 *
	 * <p>This is what the golden output cases use. {@code = json    "a": 1} yields
	 * {@code    "a": 1}: the first space after the key separates, the rest is the
	 * value's own indentation.
	 */
	public List<String> getAllRaw(String key)
	{
		return valuesOf(expectations, key);
	}

	/** True when the fixture states an expectation for this key. */
	public boolean has(String key)
	{
		return !valuesOf(expectations, key).isEmpty();
	}

	/** Every value for a repeated {@code <} key, trimmed, in fixture order. */
	public List<String> givenAll(String key)
	{
		List<String> result = new ArrayList<>();
		for (String value : valuesOf(given, key))
			result.add(value.trim());
		return result;
	}

	/** True when the case is given anything under this key. */
	public boolean hasGiven(String key)
	{
		return !valuesOf(given, key).isEmpty();
	}

	/**
	 * A yes/no expectation. Accepts only "yes" and "no", so the fixtures stay
	 * uniform.
	 *
	 * @throws FixtureFormatException the value is anything else, or absent.
	 */
	public boolean getBool(String key)
	{
		String value = get(key);
		if ("yes".equals(value))
			return true;
		if ("no".equals(value))
			return false;
		throw new FixtureFormatException(describe("'" + key + "' must be yes or no, was '" + value + "'"));
	}

	/**
	 * A failure message that names the fixture and line, states the problem, and
	 * reproduces the whole case, so a failing run does not send the reader off to
	 * open another file.
	 */
	public String describe(String problem)
	{
		StringBuilder text = new StringBuilder();
		text.append(file).append(':').append(line).append("  $ ").append(subject).append('\n');
		text.append("  ").append(problem).append('\n');
		text.append("  case as written:\n");
		for (Map.Entry<String, String> pair : given)
			text.append("    < ").append(pair.getKey()).append(' ').append(pair.getValue()).append('\n');
		for (Map.Entry<String, String> pair : expectations)
			text.append("    = ").append(pair.getKey()).append(' ').append(pair.getValue()).append('\n');
		return text.toString();
	}

	@Override
	public String toString()
	{
		return file + ":" + line + " " + subject;
	}

	private static List<String> valuesOf(List<Map.Entry<String, String>> pairs, String key)
	{
		List<String> result = new ArrayList<>();
		for (Map.Entry<String, String> pair : pairs)
		{
			if (pair.getKey().equals(key))
				result.add(pair.getValue());
		}
		return result;
	}
}
