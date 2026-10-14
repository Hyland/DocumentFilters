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

using System.Collections.Generic;
using System.Linq;
using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Conformance
{
	/// <summary>
	/// The error vocabulary, checked against the fixtures that already state it.
	/// </summary>
	/// <remarks>
	/// There is no separate fixture listing the codes: every failing case in
	/// <c>plan-corpus.txt</c> already states its code, its message and its exit code,
	/// and a second file repeating them would be a second place to keep in step.
	///
	/// <see cref="ErrorCodes.All"/> is a hand-written list beside the constants rather
	/// than reflection, because C++17 has none and these tests are translated into the
	/// other three languages rather than rewritten there.
	/// </remarks>
	public class ErrorCodeTests
	{
		/// <summary>
		/// The codes no planning fixture can provoke.
		/// </summary>
		/// <remarks>
		/// <c>plan-corpus.txt</c> resolves argv into a plan and opens nothing, so none
		/// of these can appear there: three need a live engine — the library has to
		/// fail to initialise, a document has to fail to open, or one has to outrun
		/// <c>--timeout</c> — the fourth needs a person pressing Ctrl+C, and the fifth
		/// needs an open container, because what a subfile is named is not knowable
		/// until one is. They are exercised all the same, by
		/// <c>SubfileExtractionTests</c> and <c>CliTests</c> against the fake engine
		/// and by <c>tests/e2e/end2end.py</c> against the real one, so what is listed
		/// here is the fact that they have a home, not an exemption from having one.
		///
		/// Written out rather than derived, so that adding a code and forgetting to
		/// provoke it still fails: this list is the only way to opt out, and it is
		/// short enough to argue with.
		/// </remarks>
		private static readonly string[] CodesNoPlanCanProvoke =
		{
			ErrorCodes.InitFailed,
			ErrorCodes.DocumentFailed,
			ErrorCodes.LimitExceeded,
			ErrorCodes.Interrupted,
			ErrorCodes.UnsafeOutput
		};

		/// <summary>Every code named by a failing case in the corpus, plus those no case can reach.</summary>
		private static HashSet<string> CodesInFixtures()
		{
			var codes = new HashSet<string>(CodesNoPlanCanProvoke);

			foreach (FixtureCase testCase in PlanCorpus.Load())
			{
				if (testCase.Error != null)
					codes.Add(testCase.Error);
			}

			return codes;
		}

		[Fact]
		public void Every_code_the_tool_declares_is_exercised_by_a_fixture()
		{
			// The direction that matters: a code nobody provokes has no message, which
			// contradicts stderr being contract.
			var exercised = CodesInFixtures();

			foreach (string code in ErrorCodes.All)
			{
				Assert.True(
					exercised.Contains(code),
					code + " is declared in ErrorCodes but no fixture case provokes it, so its message is untested");
			}
		}

		[Fact]
		public void Every_code_a_fixture_names_is_one_the_tool_declares()
		{
			// Catches a typo in a fixture, which would otherwise look like an
			// implementation bug in whichever language ran it next.
			var declared = new HashSet<string>(ErrorCodes.All);

			foreach (string code in CodesInFixtures())
				Assert.True(declared.Contains(code), "a fixture names " + code + ", which ErrorCodes does not declare");
		}

		[Fact]
		public void One_code_always_means_one_exit_code()
		{
			// E_MUTUALLY_EXCLUSIVE appears in eight cases. Nothing else checks they agree,
			// so one drifting to a different exit code would reach three more
			// implementations before anyone noticed.
			var seen = new Dictionary<string, int>();
			var firstSeen = new Dictionary<string, FixtureCase>();

			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error != null))
			{
				int exit = PlanCorpus.ExpectedExit(testCase);

				if (seen.TryGetValue(testCase.Error, out int existing))
				{
					Assert.True(
						existing == exit,
						testCase.Describe($"{testCase.Error} exits {exit} here but {existing} at {firstSeen[testCase.Error]}"));
				}
				else
				{
					seen[testCase.Error] = exit;
					firstSeen[testCase.Error] = testCase;
				}
			}
		}

		[Fact]
		public void Every_exit_code_a_fixture_states_is_one_the_table_defines()
		{
			var valid = new HashSet<int>
			{
				ExitCode.Success, ExitCode.PartialFailure, ExitCode.UsageError,
				ExitCode.NoInputsMatched, ExitCode.OutputConflict,
				ExitCode.InitialisationFailure, ExitCode.LimitExceeded, ExitCode.Interrupted
			};

			foreach (FixtureCase testCase in PlanCorpus.Load())
			{
				Assert.True(
					valid.Contains(PlanCorpus.ExpectedExit(testCase)),
					testCase.Describe("states an exit code that is not in the §07 table"));
			}
		}
	}
}
