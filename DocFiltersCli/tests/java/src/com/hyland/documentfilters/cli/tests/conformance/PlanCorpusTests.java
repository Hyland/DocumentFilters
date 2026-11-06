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

import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;
import static com.hyland.documentfilters.cli.tests.Assert.fail;

import com.hyland.documentfilters.cli.cli.ArgumentTable;
import com.hyland.documentfilters.cli.cli.BoundCommandLine;
import com.hyland.documentfilters.cli.cli.CommandLineApp;
import com.hyland.documentfilters.cli.commands.CommandContext;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The cross-language contract, executed.
 *
 * <p>Each case runs stages 1 to 3 (parse, bind, plan) over the filesystem it is
 * given, and compares the canonical plan text against the shared fixture. No document
 * is opened, so these need neither a license nor the native library.
 *
 * <p>The C#, Python and C++17 suites run the same fixture through their own
 * implementations. A case passing in all of them is the definition of the ports
 * agreeing.
 */
public final class PlanCorpusTests
{
	/**
	 * Every case in the corpus, run in turn.
	 *
	 * <p>The runner has no parameterised tests, so the cases are collected here and
	 * every failure is reported together rather than stopping at the first.
	 */
	@Test
	public void caseMatchesTheSharedCorpus()
	{
		List<String> failures = new ArrayList<>();
		for (FixtureCase testCase : PlanCorpus.load())
		{
			String failure = check(testCase);
			if (failure != null)
				failures.add(failure);
		}
		if (!failures.isEmpty())
			fail(failures.size() + " corpus case(s) failed:\n" + String.join("\n", failures));
	}

	/** Runs one case; returns null when it passes, or the failure message. */
	private static String check(FixtureCase testCase)
	{
		String[] args = PlanCorpus.splitArguments(testCase.subject);
		int expectedExit = PlanCorpus.expectedExit(testCase);
		InMemoryFileSystem fileSystem = PlanCorpus.buildFileSystem(testCase);

		if (testCase.getError() != null)
		{
			CliError error;
			try
			{
				plan(args, fileSystem);
				return testCase.describe("expected error " + testCase.getError() + ", nothing was thrown");
			}
			catch (CliError thrown)
			{
				error = thrown;
			}

			if (!testCase.getError().equals(error.getCode()))
				return testCase.describe("expected error " + testCase.getError() + ", got " + error.getCode() + ": " + error.getMessage());
			if (error.getExitCode() != expectedExit)
				return testCase.describe("expected exit " + expectedExit + ", got " + error.getExitCode());

			// Error text is part of the contract, so the ports cannot each phrase the
			// same failure differently and all pass.
			String expectedMessage = testCase.get("message");
			if (expectedMessage == null || !normalise(error.getMessage()).equals(normalise(expectedMessage)))
				return testCase.describe("message was '" + error.getMessage() + "', expected '" + expectedMessage + "'");
			return null;
		}

		// The whole document, in order, not a search for fragments. Substring matching
		// would let extra lines, reordered fields and duplicate targets pass in one
		// implementation and fail in another, which is exactly the drift this corpus
		// exists to catch.
		List<String> expected = new ArrayList<>();
		for (Map.Entry<String, String> expectation : testCase.expectations)
		{
			if (!expectation.getKey().equals("exit"))
				expected.add(normalise(expectation.getKey() + " " + expectation.getValue()));
		}

		String plan = plan(args, fileSystem);
		List<String> actual = new ArrayList<>();
		for (String line : plan.split("\n", -1))
		{
			String normalised = normalise(line);
			if (!normalised.isEmpty())
				actual.add(normalised);
		}

		if (!expected.equals(actual))
			return testCase.describe("plan does not match. Plan as produced:\n" + indent(plan));
		return null;
	}

	/**
	 * Runs stages 1 to 3 (parse, bind, plan) against the filesystem the case is given,
	 * with the input walk feeding the planner.
	 *
	 * <p>The walk runs, which is what makes {@code = inputs} and {@code = target} real
	 * assertions rather than an echo of the arguments: a case naming a directory states
	 * the files that come out of it, so --max-depth, --glob and --include-hidden are
	 * contract rather than one language's own tests.
	 */
	private static String plan(String[] args, IFileSystem fileSystem)
	{
		CommandLineApp app = new CommandLineApp(new RecordingConsole(), fileSystem);
		BoundCommandLine bound = app.bind(args);
		CommandContext context = new CommandContext();
		context.sharedOptions = bound.getSharedOptions();
		context.console = new RecordingConsole();
		context.fileSystem = fileSystem;
		return bound.getCommand().describePlan(context);
	}

	/** Collapses runs of spaces so fixture alignment is not significant. */
	private static String normalise(String text)
	{
		while (text.contains("  "))
			text = text.replace("  ", " ");
		return text.trim();
	}

	private static String indent(String text)
	{
		StringBuilder result = new StringBuilder();
		for (String line : text.split("\n", -1))
		{
			if (result.length() > 0)
				result.append('\n');
			result.append("      ").append(line);
		}
		return result.toString();
	}

	// ------------------------------------------------------ the corpus itself

	@Test
	public void theCorpusIsPresentAndNotEmpty()
	{
		// A missing fixture would otherwise make every case above vanish silently
		// rather than fail.
		assertFalse(PlanCorpus.load().isEmpty(), "the plan corpus has no cases");
	}

	@Test
	public void everyCaseStatesAnExpectedExitCode()
	{
		for (FixtureCase testCase : PlanCorpus.load())
			assertTrue(testCase.has("exit"), testCase.describe("no '= exit' expectation"));
	}

	@Test
	public void everyFailingCaseExpectsANonZeroExit()
	{
		for (FixtureCase testCase : failing())
			assertTrue(PlanCorpus.expectedExit(testCase) != 0, testCase.describe("expects an error but exit 0"));
	}

	@Test
	public void everyExpectedErrorCodeIsOneTheToolCanRaise()
	{
		// Catches a typo in the fixture, which would otherwise look like an
		// implementation bug in whichever language ran it next.
		Set<String> known = new HashSet<>(ErrorCodes.ALL);
		for (FixtureCase testCase : failing())
			assertTrue(known.contains(testCase.getError()), testCase.describe("unknown error code"));
	}

	@Test
	public void everyFailingCaseStatesItsMessage()
	{
		// stderr is contract. A case that gives only a code would let the ports phrase
		// the same failure several ways and all pass.
		for (FixtureCase testCase : failing())
			assertTrue(testCase.has("message"), testCase.describe("no '= message' expectation"));
	}

	@Test
	public void everySucceedingCaseStatesACompletePlan()
	{
		// A case listing only '= exit 0' would pass against any plan whatsoever.
		for (FixtureCase testCase : succeeding())
			assertTrue(testCase.has("command"), testCase.describe("a succeeding case must state 'command'"));
	}

	/**
	 * Commands that declare the output group state a cardinality; the rest do not.
	 *
	 * <p>Cardinality answers "who owns the output target". {@code identify} declares no
	 * output group at all, and {@code formats} and {@code options} read no inputs and
	 * write no files, so for those three the question has no answer and printing one
	 * would be inventing it.
	 */
	@Test
	public void onlyCommandsWithAnOutputGroupStateACardinality()
	{
		Set<String> owns = new HashSet<>(Arrays.asList("convert", "extract", "list", "meta"));
		for (FixtureCase testCase : succeeding())
		{
			String command = testCase.get("command");
			if (command == null)
				continue;

			assertTrue(
				testCase.has("cardinality") == owns.contains(command),
				testCase.describe(owns.contains(command)
					? command + " owns its output and must state a cardinality"
					: command + " owns no output target and must not state a cardinality"));
		}
	}

	/**
	 * Every succeeding case for one command emits the same lines, in the same order.
	 *
	 * <p>The corpus is compared as a whole document, so which lines appear has to be a
	 * property of the command and not of the arguments. Two cases with the same argv
	 * and different line sets cannot both be satisfied, in any language.
	 */
	@Test
	public void allCasesForOneCommandEmitTheSameLineSet()
	{
		Map<String, String> shapes = new HashMap<>();
		Map<String, FixtureCase> firstSeen = new HashMap<>();
		for (FixtureCase testCase : succeeding())
		{
			String command = testCase.get("command");
			if (command == null)
				continue;

			List<String> keys = new ArrayList<>();
			for (Map.Entry<String, String> expectation : testCase.expectations)
			{
				// target and pattern are the one part of the plan that legitimately
				// varies: one per input for a resolved path, one per input for a
				// per-page pattern, and none at all when the output is streamed to
				// stdout. Their placement is checked separately below.
				if (isVariableKey(expectation.getKey()))
					continue;
				keys.add(expectation.getKey());
			}

			String shape = String.join(" ", keys);
			String existing = shapes.get(command);
			if (existing != null)
			{
				assertTrue(
					existing.equals(shape),
					testCase.describe(
						"every '" + command + "' case must emit the same lines in the same order.\n"
						+ "      this case: " + shape + "\n"
						+ "      " + firstSeen.get(command) + ": " + existing));
			}
			else
			{
				shapes.put(command, shape);
				firstSeen.put(command, testCase);
			}
		}
	}

	/**
	 * Keys that are not a fixed plan line: {@code exit}, which is the process result,
	 * and the target keys, whose count varies by input and by format.
	 */
	private static boolean isVariableKey(String key)
	{
		return key.equals("exit") || key.equals("target") || key.equals("pattern");
	}

	/**
	 * Every case whose error comes from the planner declares the inputs it needs.
	 *
	 * <p>The walk runs before planning, so a case meaning to provoke
	 * {@code E_OUTFILE_MULTI_INPUT} with no tree gets {@code E_NO_INPUTS} instead: it
	 * passes for the wrong reason and tests nothing.
	 */
	@Test
	public void plannerErrorsDeclareTheInputsTheyNeed()
	{
		Set<String> afterDiscovery = new HashSet<>(Arrays.asList(
			ErrorCodes.OUT_FILE_MULTI_INPUT, ErrorCodes.OUTPUT_COLLISION, ErrorCodes.OUTPUT_EXISTS));
		for (FixtureCase testCase : failing())
		{
			if (!afterDiscovery.contains(testCase.getError()))
				continue;
			assertTrue(
				testCase.hasGiven("tree"),
				testCase.describe(testCase.getError() + " is raised after the walk, so the case needs a '< tree'"));
		}
	}

	/** A case that walks a directory sorts, because directory order is otherwise undefined. */
	@Test
	public void casesThatWalkADirectoryAskForADefinedOrder()
	{
		for (FixtureCase testCase : succeeding())
		{
			// More than one file under a common root means the walk chose an order.
			int nested = 0;
			for (String row : testCase.givenAll("tree"))
			{
				for (String path : PlanCorpus.splitPaths(row))
				{
					if (path.contains("/"))
						nested++;
				}
			}
			if (nested <= 1)
				continue;

			String walk = testCase.get("walk");
			assertTrue(
				walk != null && walk.contains("sort=yes"),
				testCase.describe("a case walking several files must pass --sort: a listing's "
					+ "order is unspecified without it, and this corpus compares order"));
		}
	}

	/**
	 * Targets come last, in one run, and a case never mixes the two spellings.
	 *
	 * <p>The line set test above ignores them, because a streamed run has no target at
	 * all. This is their guard: wherever they appear they are contiguous, they sit
	 * after every fixed line, and an item carries either a resolved target or a
	 * deferred pattern, never both.
	 */
	@Test
	public void targetLinesAreContiguousLastAndOfOneKind()
	{
		for (FixtureCase testCase : succeeding())
		{
			List<String> keys = new ArrayList<>();
			for (Map.Entry<String, String> expectation : testCase.expectations)
			{
				if (!expectation.getKey().equals("exit"))
					keys.add(expectation.getKey());
			}

			int first = -1;
			for (int i = 0; i < keys.size() && first < 0; i++)
			{
				if (keys.get(i).equals("target") || keys.get(i).equals("pattern"))
					first = i;
			}
			if (first < 0)
				continue;

			List<String> tail = keys.subList(first, keys.size());
			boolean allTargets = true;
			for (String key : tail)
				allTargets &= key.equals("target") || key.equals("pattern");
			assertTrue(allTargets, testCase.describe("target and pattern lines must come after every other plan line"));
			assertTrue(new HashSet<>(tail).size() == 1, testCase.describe("a case states either targets or patterns, never both"));
		}
	}

	@Test
	public void noTwoCasesShareASubjectAndAFilesystem()
	{
		// Two cases with the same input and different expectations cannot both pass.
		// The input is argv *and* the tree it runs against: several cases reuse one
		// command line to show what a hidden entry or a symlink changes, and those are
		// different runs rather than a contradiction.
		Map<String, FixtureCase> seen = new HashMap<>();
		for (FixtureCase testCase : PlanCorpus.load())
		{
			StringBuilder key = new StringBuilder(testCase.subject);
			for (Map.Entry<String, String> given : testCase.given)
				key.append('\n').append(given.getKey()).append(' ').append(given.getValue().trim());

			String identity = key.toString();
			FixtureCase first = seen.get(identity);
			assertTrue(first == null, testCase.describe("same command line and same filesystem as " + first));
			seen.put(identity, testCase);
		}
	}

	@Test
	public void theDryRunSwitchItselfIsExercised()
	{
		// Every other case reaches the planner directly, so without this the
		// advertised switch could be missing entirely and the corpus still pass.
		boolean dryRun = false;
		boolean printPlan = false;
		for (FixtureCase testCase : PlanCorpus.load())
		{
			dryRun |= testCase.subject.contains("--dry-run");
			printPlan |= testCase.subject.contains("--print-plan");
		}
		assertTrue(dryRun, "no corpus case uses --dry-run");
		assertTrue(printPlan, "no corpus case uses --print-plan");
	}

	@Test
	public void theCorpusCoversEveryCommand()
	{
		// Coverage guard: a new verb must arrive with at least one case.
		Set<String> covered = new HashSet<>();
		for (FixtureCase testCase : PlanCorpus.load())
		{
			String[] args = PlanCorpus.splitArguments(testCase.subject);
			if (args.length > 0)
				covered.add(args[0]);
		}
		for (String command : ArgumentTable.COMMANDS)
			assertTrue(covered.contains(command), "no corpus case exercises: " + command);
	}

	private static List<FixtureCase> failing()
	{
		List<FixtureCase> result = new ArrayList<>();
		for (FixtureCase testCase : PlanCorpus.load())
		{
			if (testCase.getError() != null)
				result.add(testCase);
		}
		return result;
	}

	private static List<FixtureCase> succeeding()
	{
		List<FixtureCase> result = new ArrayList<>();
		for (FixtureCase testCase : PlanCorpus.load())
		{
			if (testCase.getError() == null)
				result.add(testCase);
		}
		return result;
	}
}
