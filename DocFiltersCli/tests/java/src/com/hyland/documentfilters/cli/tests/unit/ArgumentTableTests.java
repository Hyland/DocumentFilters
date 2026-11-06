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
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.cli.ArgumentArity;
import com.hyland.documentfilters.cli.cli.ArgumentTable;
import com.hyland.documentfilters.cli.cli.IArgumentDescriptor;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.conformance.Fixture;
import com.hyland.documentfilters.cli.tests.conformance.FixtureCase;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The command line surface, checked against the shared {@code arguments.txt}
 * fixture.
 *
 * <p>The fixture is written from the CLI design document, not dumped from any
 * implementation. That direction is the point: a file generated from one build could
 * only prove the others agree with it, whereas this proves all four agree with the
 * specification --- so an argument nobody implemented still fails.
 *
 * <p>This asks "does the right set of arguments exist, with the right arity?".
 * {@link ArgumentBindingTests} asks "does each one do what it says?".
 */
public final class ArgumentTableTests
{
	/**
	 * The seven processing commands, written out rather than read from
	 * {@link ArgumentTable}. The duplication is deliberate: a command quietly dropped
	 * from the table still gets tested here, and
	 * {@link #theSevenProcessingCommandsAreDeclaredInHelpOrder} ties the two lists
	 * back together.
	 */
	private static final List<String> COMMANDS =
		Arrays.asList("convert", "extract", "list", "identify", "meta", "formats", "options");

	@Test
	public void commandDeclaresExactlyTheArgumentsInTheSharedFixture()
	{
		for (FixtureCase testCase : Fixture.load("arguments.txt"))
		{
			String command = testCase.subject;

			List<ArgumentRow> expected = new ArrayList<>();
			for (String row : testCase.getAll("argument"))
				expected.add(ArgumentRow.parse(row, testCase));

			Map<String, IArgumentDescriptor> declared = new HashMap<>();
			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
				declared.put(argument.getLongName(), argument);

			// Nothing missing.
			for (ArgumentRow row : expected)
			{
				assertTrue(declared.containsKey(row.longName), testCase.describe("'" + command + "' does not declare " + row.template));

				IArgumentDescriptor actual = declared.get(row.longName);

				assertTrue(actual.getArity() == row.arity,
					testCase.describe(row.template + ": arity is " + actual.getArity() + ", fixture says " + row.arity));

				assertTrue(orEmpty(actual.getShortName()).equals(orEmpty(row.shortName)),
					testCase.describe(row.template + ": short form is '" + actual.getShortName() + "', fixture says '" + row.shortName + "'"));
			}

			// And nothing extra: an undocumented argument is drift just as surely as a
			// missing one, and would never be implemented in the other three.
			Set<String> expectedNames = new HashSet<>();
			for (ArgumentRow row : expected)
				expectedNames.add(row.longName);

			for (IArgumentDescriptor actual : ArgumentTable.describeCommand(command))
			{
				assertTrue(expectedNames.contains(actual.getLongName()),
					testCase.describe("'" + command + "' declares --" + actual.getLongName() + ", which arguments.txt does not list"));
			}
		}
	}

	// ------------------------------------------- behaviour the fixture cannot state

	@Test
	public void theFixtureCoversEveryProcessingCommand()
	{
		Set<String> covered = new HashSet<>();
		for (FixtureCase testCase : Fixture.load("arguments.txt"))
			covered.add(testCase.subject);

		for (String command : ArgumentTable.COMMANDS)
			assertTrue(covered.contains(command), "arguments.txt has no section for: " + command);
	}

	@Test
	public void theFixtureListsNoCommandTheToolDoesNotHave()
	{
		for (FixtureCase testCase : Fixture.load("arguments.txt"))
		{
			assertTrue(ArgumentTable.COMMANDS.contains(testCase.subject),
				testCase.describe("arguments.txt describes a command the tool does not declare"));
		}
	}

	@Test
	public void theSevenProcessingCommandsAreDeclaredInHelpOrder()
	{
		assertEquals(COMMANDS, ArgumentTable.COMMANDS);
	}

	@Test
	public void helpAndVersionAreVerbsButNotProcessingCommands()
	{
		// They accept no shared options and process no documents, so they are
		// dispatchable without appearing in the applicability table.
		assertTrue(ArgumentTable.ALL_VERBS.contains("help"));
		assertTrue(ArgumentTable.ALL_VERBS.contains("version"));
		assertFalse(ArgumentTable.COMMANDS.contains("help"));
		assertFalse(ArgumentTable.COMMANDS.contains("version"));
	}

	@Test
	public void noCommandDeclaresTheSameArgumentTwice()
	{
		for (String command : COMMANDS)
		{
			Set<String> seen = new HashSet<>();

			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
				assertTrue(seen.add(argument.getLongName()), command + " declares --" + argument.getLongName() + " twice");
		}
	}

	@Test
	public void noCommandReusesAShortName()
	{
		for (String command : COMMANDS)
		{
			Set<String> seen = new HashSet<>();

			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
			{
				if (orEmpty(argument.getShortName()).isEmpty())
					continue;

				assertTrue(seen.add(argument.getShortName()), command + " reuses -" + argument.getShortName());
			}
		}
	}

	@Test
	public void everyArgumentHasHelpText()
	{
		// Prose, so it is not in the fixture --- help text may differ in wording
		// between the four implementations. It may not be absent.
		for (String command : COMMANDS)
		{
			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
			{
				assertFalse(orEmpty(argument.getDescription()).trim().isEmpty(),
					command + ": --" + argument.getLongName() + " has no description");
			}
		}
	}

	@Test
	public void aSharedArgumentKeepsTheSameArityOnEveryCommandThatAcceptsIt()
	{
		// The defining property of a shared option, and true without exception:
		// --glob cannot be repeatable on convert and single-valued on list, and
		// --regex cannot be a path pattern on one command and a mode switch on
		// another.
		Map<String, ArgumentArity> arities = new HashMap<>();
		Map<String, String> firstSeenOn = new HashMap<>();

		for (String command : ArgumentTable.COMMANDS)
		{
			for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
			{
				ArgumentArity existing = arities.get(argument.getLongName());

				if (existing != null)
				{
					assertTrue(existing == argument.getArity(), "--" + argument.getLongName() + " is " + existing + " on "
						+ firstSeenOn.get(argument.getLongName()) + " but " + argument.getArity() + " on " + command);
				}
				else
				{
					arities.put(argument.getLongName(), argument.getArity());
					firstSeenOn.put(argument.getLongName(), command);
				}
			}
		}
	}

	@Test
	public void extractAcceptsTheSharedRegexLikeEveryOtherWalkingCommand()
	{
		// extract walks directories like the others, so it selects paths the same
		// way: -g/--glob and --regex, with --include/--exclude picking subfiles.
		List<String> declared = new ArrayList<>();
		for (IArgumentDescriptor argument : ArgumentTable.describeCommand("extract"))
			declared.add(argument.getLongName());

		assertTrue(declared.contains("regex"), String.valueOf(declared));
		assertTrue(declared.contains("include"), String.valueOf(declared));
		assertFalse(declared.contains("filter"), String.valueOf(declared));
	}

	@Test
	public void includeAndExcludeAreAMatchedPairOnExtract()
	{
		// A matched pair: same arity, opposite direction.
		IArgumentDescriptor include = single("extract", "include");
		IArgumentDescriptor exclude = single("extract", "exclude");

		assertEquals(exclude.getArity(), include.getArity());
	}

	// ------------------------------------------- helpers

	private static String orEmpty(String text)
	{
		return text == null ? "" : text;
	}

	/** The one argument named {@code longName} on {@code command}. */
	private static IArgumentDescriptor single(String command, String longName)
	{
		IArgumentDescriptor found = null;
		int count = 0;

		for (IArgumentDescriptor argument : ArgumentTable.describeCommand(command))
		{
			if (argument.getLongName().equals(longName))
			{
				found = argument;
				count++;
			}
		}

		assertEquals(Integer.valueOf(1), Integer.valueOf(count), command + " --" + longName);
		return found;
	}

	/** One {@code = argument template arity} row. */
	private static final class ArgumentRow
	{
		String template;
		ArgumentArity arity;

		/** Long name without dashes, e.g. {@code out-dir}. */
		String longName;

		/** Short name without its dash, or null. */
		String shortName;

		static ArgumentRow parse(String row, FixtureCase testCase)
		{
			String[] parts = row.trim().split("\\s+");

			if (parts.length != 2)
				throw new IllegalStateException(testCase.describe("argument row needs 'template arity': " + row));

			ArgumentRow result = new ArgumentRow();
			result.template = parts[0];
			result.arity = parseArity(parts[1], testCase);

			for (String form : parts[0].split("\\|"))
			{
				if (form.startsWith("--"))
					result.longName = form.substring(2);
				else if (form.startsWith("-"))
					result.shortName = form.substring(1);
			}

			// An argument written only in short form, such as -x, is named by that form.
			if (result.longName == null)
				result.longName = result.shortName;

			return result;
		}

		private static ArgumentArity parseArity(String text, FixtureCase testCase)
		{
			switch (text)
			{
				case "none":
					return ArgumentArity.NO_VALUE;
				case "single":
					return ArgumentArity.SINGLE_VALUE;
				case "multiple":
					return ArgumentArity.MULTIPLE_VALUE;
				default:
					throw new IllegalStateException(testCase.describe("unknown arity '" + text + "'"));
			}
		}
	}
}
