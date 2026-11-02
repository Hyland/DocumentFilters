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

import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.tests.Test;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The error vocabulary, checked against the fixtures that already state it.
 *
 * <p>There is no separate fixture listing the codes: every failing case in
 * {@code plan-corpus.txt} already states its code, its message and its exit code,
 * and a second file repeating them would be a second place to keep in step.
 *
 * <p>{@link ErrorCodes#ALL} is a hand-written list beside the constants rather than
 * reflection, because C++17 has none and these tests are translated between the
 * ports rather than rewritten in each.
 */
public final class ErrorCodeTests
{
	/**
	 * The codes no planning fixture can provoke.
	 *
	 * <p>{@code plan-corpus.txt} resolves argv into a plan and opens nothing, so none of
	 * these can appear there: three need a live engine (the library has to fail to
	 * initialise, a document has to fail to open, or one has to outrun
	 * {@code --timeout}), the fourth needs a person pressing Ctrl+C, and the fifth
	 * needs an open container, because what a subfile is named is not knowable until
	 * one is. They are exercised all the same, against the fake engine and by the
	 * end-to-end script against the real one, so what is listed here is the fact that
	 * they have a home, not an exemption from having one.
	 *
	 * <p>Written out rather than derived, so that adding a code and forgetting to
	 * provoke it still fails: this list is the only way to opt out, and it is short
	 * enough to argue with.
	 */
	private static final List<String> CODES_NO_PLAN_CAN_PROVOKE = Arrays.asList(
		ErrorCodes.INIT_FAILED,
		ErrorCodes.DOCUMENT_FAILED,
		ErrorCodes.LIMIT_EXCEEDED,
		ErrorCodes.INTERRUPTED,
		ErrorCodes.UNSAFE_OUTPUT);

	/** Every code named by a failing case in the corpus, plus those no case can reach. */
	private static Set<String> codesInFixtures()
	{
		Set<String> codes = new HashSet<>(CODES_NO_PLAN_CAN_PROVOKE);
		for (FixtureCase testCase : PlanCorpus.load())
		{
			if (testCase.getError() != null)
				codes.add(testCase.getError());
		}
		return codes;
	}

	@Test
	public void everyCodeTheToolDeclaresIsExercisedByAFixture()
	{
		// The direction that matters: a code nobody provokes has no message, which
		// contradicts stderr being contract.
		Set<String> exercised = codesInFixtures();
		for (String code : ErrorCodes.ALL)
		{
			assertTrue(
				exercised.contains(code),
				code + " is declared in ErrorCodes but no fixture case provokes it, so its message is untested");
		}
	}

	@Test
	public void everyCodeAFixtureNamesIsOneTheToolDeclares()
	{
		// Catches a typo in a fixture, which would otherwise look like an
		// implementation bug in whichever language ran it next.
		Set<String> declared = new HashSet<>(ErrorCodes.ALL);
		for (String code : codesInFixtures())
			assertTrue(declared.contains(code), "a fixture names " + code + ", which ErrorCodes does not declare");
	}

	@Test
	public void oneCodeAlwaysMeansOneExitCode()
	{
		// E_MUTUALLY_EXCLUSIVE appears in several cases. Nothing else checks they
		// agree, so one drifting to a different exit code would reach the other ports
		// before anyone noticed.
		Map<String, Integer> seen = new HashMap<>();
		Map<String, FixtureCase> firstSeen = new HashMap<>();
		for (FixtureCase testCase : PlanCorpus.load())
		{
			String code = testCase.getError();
			if (code == null)
				continue;

			int exit = PlanCorpus.expectedExit(testCase);
			Integer existing = seen.get(code);
			if (existing != null)
			{
				assertTrue(
					existing == exit,
					testCase.describe(code + " exits " + exit + " here but " + existing + " at " + firstSeen.get(code)));
			}
			else
			{
				seen.put(code, exit);
				firstSeen.put(code, testCase);
			}
		}
	}

	@Test
	public void everyExitCodeAFixtureStatesIsOneTheTableDefines()
	{
		Set<Integer> valid = new HashSet<>(Arrays.asList(
			ExitCode.SUCCESS, ExitCode.PARTIAL_FAILURE, ExitCode.USAGE_ERROR,
			ExitCode.NO_INPUTS_MATCHED, ExitCode.OUTPUT_CONFLICT,
			ExitCode.INITIALISATION_FAILURE, ExitCode.LIMIT_EXCEEDED, ExitCode.INTERRUPTED));
		for (FixtureCase testCase : PlanCorpus.load())
		{
			assertTrue(
				valid.contains(PlanCorpus.expectedExit(testCase)),
				testCase.describe("states an exit code that is not in the exit code table"));
		}
	}
}
