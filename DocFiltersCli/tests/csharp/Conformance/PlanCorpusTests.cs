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
using System.Text;
using Hyland.DocumentFilters.Cli.Cli;
using Hyland.DocumentFilters.Cli.Commands;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Conformance
{
	/// <summary>
	/// The cross-language contract, executed.
	/// </summary>
	/// <remarks>
	/// Each case runs stages 1 to 3 — parse, bind, plan — over the filesystem it is
	/// given, and compares the canonical plan text against the shared fixture. No
	/// document is opened, so these need neither a license nor the native library.
	///
	/// The Python, Java and C++17 suites run the same fixture through their own
	/// implementations. A case passing in all four is the definition of the four
	/// agreeing.
	/// </remarks>
	public class PlanCorpusTests
	{
		public static IEnumerable<object[]> Cases()
		{
			return PlanCorpus.Load().Select(c => new object[] { c });
		}

		[Theory]
		[MemberData(nameof(Cases))]
		public void Case_matches_the_shared_corpus(FixtureCase testCase)
		{
			string[] args = PlanCorpus.SplitArguments(testCase.Subject);
			int expectedExit = PlanCorpus.ExpectedExit(testCase);

			InMemoryFileSystem fileSystem = PlanCorpus.BuildFileSystem(testCase);

			if (testCase.Error != null)
			{
				CliError error = Assert.ThrowsAny<CliError>(() => Plan(args, fileSystem));

				Assert.True(
					error.Code == testCase.Error,
					testCase.Describe($"expected error {testCase.Error}, got {error.Code}: {error.Message}"));

				Assert.True(
					error.ExitCode == expectedExit,
					testCase.Describe($"expected exit {expectedExit}, got {error.ExitCode}"));

				// Error text is part of the contract, so four ports cannot each phrase
				// the same failure differently and all pass.
				string expectedMessage = testCase.Get("message");

				Assert.True(
					Normalise(error.Message) == Normalise(expectedMessage),
					testCase.Describe($"message was '{error.Message}', expected '{expectedMessage}'"));
				return;
			}

			// The whole document, in order — not a search for fragments. Substring
			// matching would let extra lines, reordered fields and duplicate targets
			// pass in one implementation and fail in another, which is exactly the
			// drift this corpus exists to catch.
			string[] expected = testCase.Expectations
				.Where(e => e.Key != "exit")
				.Select(e => Normalise(e.Key + " " + e.Value))
				.ToArray();

			string plan = Plan(args, fileSystem);

			string[] actual = plan
				.Split('\n')
				.Select(Normalise)
				.Where(line => line.Length > 0)
				.ToArray();

			Assert.True(
				expected.SequenceEqual(actual),
				testCase.Describe($"plan does not match. Plan as produced:\n{Indent(plan)}"));
		}

		/// <summary>
		/// Runs stages 1 to 3 — parse, bind, plan — against the filesystem the case is
		/// given, with the input walk feeding the planner.
		/// </summary>
		/// <remarks>
		/// The walk runs, which is what makes <c>= inputs</c> and <c>= target</c> real
		/// assertions rather than an echo of the arguments: a case naming a directory
		/// states the files that come out of it, so <c>--max-depth</c>, <c>--glob</c> and
		/// <c>--include-hidden</c> are contract rather than one language's own tests.
		/// </remarks>
		private static string Plan(string[] args, IFileSystem fileSystem)
		{
			var app = new CommandLineApp(new RecordingConsole(), fileSystem);
			BoundCommandLine bound = app.Bind(args);

			return bound.Command.DescribePlan(new CommandContext
			{
				SharedOptions = bound.SharedOptions,
				Console = new RecordingConsole(),
				FileSystem = fileSystem
			});
		}

		/// <summary>Collapses runs of spaces so fixture alignment is not significant.</summary>
		private static string Normalise(string text)
		{
			while (text.Contains("  "))
				text = text.Replace("  ", " ");

			return text.Trim();
		}

		private static string Indent(string text)
		{
			return string.Join("\n", text.Split('\n').Select(l => "      " + l));
		}

		// ------------------------------------------------------ the corpus itself

		[Fact]
		public void The_corpus_is_present_and_not_empty()
		{
			// A missing fixture would otherwise make every case above vanish silently
			// rather than fail.
			Assert.NotEmpty(PlanCorpus.Load());
		}

		[Fact]
		public void Every_case_states_an_expected_exit_code()
		{
			foreach (FixtureCase testCase in PlanCorpus.Load())
				Assert.True(testCase.Has("exit"), testCase.Describe("no '= exit' expectation"));
		}

		[Fact]
		public void Every_failing_case_expects_a_non_zero_exit()
		{
			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error != null))
			{
				Assert.True(
					PlanCorpus.ExpectedExit(testCase) != 0,
					testCase.Describe("expects an error but exit 0"));
			}
		}

		[Fact]
		public void Every_expected_error_code_is_one_the_tool_can_raise()
		{
			// Catches a typo in the fixture, which would otherwise look like an
			// implementation bug in whichever language ran it next.
			var known = new HashSet<string>(ErrorCodes.All);

			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error != null))
				Assert.True(known.Contains(testCase.Error), testCase.Describe("unknown error code"));
		}

		[Fact]
		public void Every_failing_case_states_its_message()
		{
			// stderr is contract. A case that gives only a code would let the four
			// implementations phrase the same failure four ways and all pass.
			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error != null))
				Assert.True(testCase.Has("message"), testCase.Describe("no '= message' expectation"));
		}

		[Fact]
		public void Every_succeeding_case_states_a_complete_plan()
		{
			// A case listing only '= exit 0' would pass against any plan whatsoever.
			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error == null))
				Assert.True(testCase.Has("command"), testCase.Describe("a succeeding case must state 'command'"));
		}

		/// <summary>
		/// Commands that declare the output group state a cardinality; the rest do not.
		/// </summary>
		/// <remarks>
		/// Cardinality answers "who owns the output target". <c>identify</c> declares no
		/// output group at all, and <c>formats</c> and <c>options</c> read no inputs and
		/// write no files, so for those three the question has no answer and printing one
		/// would be inventing it.
		/// </remarks>
		[Fact]
		public void Only_commands_with_an_output_group_state_a_cardinality()
		{
			var owns = new HashSet<string> { "convert", "extract", "list", "meta" };

			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error == null))
			{
				string command = testCase.Get("command");
				if (command == null)
					continue;

				Assert.True(
					testCase.Has("cardinality") == owns.Contains(command),
					testCase.Describe(owns.Contains(command)
						? command + " owns its output and must state a cardinality"
						: command + " owns no output target and must not state a cardinality"));
			}
		}

		/// <summary>
		/// Every succeeding case for one command emits the same lines, in the same order.
		/// </summary>
		/// <remarks>
		/// The corpus is compared as a whole document (D14), so which lines appear has to
		/// be a property of the command and not of the arguments. Two cases with the same
		/// argv and different line sets cannot both be satisfied, in any language.
		/// </remarks>
		[Fact]
		public void All_cases_for_one_command_emit_the_same_line_set()
		{
			var shapes = new Dictionary<string, string>();
			var firstSeen = new Dictionary<string, FixtureCase>();

			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error == null))
			{
				string command = testCase.Get("command");
				if (command == null)
					continue;

				var keys = new List<string>();
				foreach (KeyValuePair<string, string> expectation in testCase.Expectations)
				{
					// target and pattern are the one part of the plan that legitimately
					// varies: one per input for a resolved path, one per input for a
					// per-page pattern (D13), and none at all when the output is streamed
					// to stdout. Their placement is checked separately below.
					if (IsVariableKey(expectation.Key))
						continue;

					keys.Add(expectation.Key);
				}

				string shape = string.Join(" ", keys);

				if (shapes.TryGetValue(command, out string existing))
				{
					Assert.True(
						existing == shape,
						testCase.Describe(
							$"every '{command}' case must emit the same lines in the same order.\n" +
							$"      this case: {shape}\n" +
							$"      {firstSeen[command]}: {existing}"));
				}
				else
				{
					shapes[command] = shape;
					firstSeen[command] = testCase;
				}
			}
		}

		/// <summary>
		/// Keys that are not a fixed plan line: <c>exit</c>, which is the process result,
		/// and the target keys, whose count varies by input and by format.
		/// </summary>
		private static bool IsVariableKey(string key)
		{
			return key == "exit" || key == "target" || key == "pattern";
		}

		/// <summary>
		/// Every case whose error comes from the planner declares the inputs it needs.
		/// </summary>
		/// <remarks>
		/// The walk runs before planning, so a case meaning to provoke
		/// <c>E_OUTFILE_MULTI_INPUT</c> with no tree gets <c>E_NO_INPUTS</c> instead: it
		/// passes for the wrong reason and tests nothing.
		/// </remarks>
		[Fact]
		public void Planner_errors_declare_the_inputs_they_need()
		{
			var afterDiscovery = new HashSet<string>
			{
				ErrorCodes.OutFileMultiInput, ErrorCodes.OutputCollision, ErrorCodes.OutputExists
			};

			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => afterDiscovery.Contains(c.Error ?? string.Empty)))
			{
				Assert.True(
					testCase.HasGiven("tree"),
					testCase.Describe(testCase.Error + " is raised after the walk, so the case needs a '< tree'"));
			}
		}

		/// <summary>A case that walks a directory sorts, because directory order is otherwise undefined.</summary>
		[Fact]
		public void Cases_that_walk_a_directory_ask_for_a_defined_order()
		{
			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error == null))
			{
				// More than one file under a common root means the walk chose an order.
				bool walksMoreThanOneFile = testCase.GivenAll("tree")
					.SelectMany(row => row.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries))
					.Count(path => path.Contains("/")) > 1;

				if (!walksMoreThanOneFile)
					continue;

				Assert.True(
					(testCase.Get("walk") ?? string.Empty).Contains("sort=yes"),
					testCase.Describe("a case walking several files must pass --sort: a listing's "
						+ "order is unspecified without it, and this corpus compares order"));
			}
		}

		/// <summary>
		/// Targets come last, in one run, and a case never mixes the two spellings.
		/// </summary>
		/// <remarks>
		/// The line set test above ignores them, because a streamed run has no target at
		/// all. This is their guard: wherever they appear they are contiguous, they sit
		/// after every fixed line, and an item carries either a resolved target or a
		/// deferred pattern, never both (D13).
		/// </remarks>
		[Fact]
		public void Target_lines_are_contiguous_last_and_of_one_kind()
		{
			foreach (FixtureCase testCase in PlanCorpus.Load().Where(c => c.Error == null))
			{
				var keys = testCase.Expectations
					.Select(e => e.Key)
					.Where(k => k != "exit")
					.ToList();

				int first = keys.FindIndex(k => k == "target" || k == "pattern");
				if (first < 0)
					continue;

				var tail = keys.Skip(first).ToList();

				Assert.True(
					tail.All(k => k == "target" || k == "pattern"),
					testCase.Describe("target and pattern lines must come after every other plan line"));

				Assert.True(
					tail.Distinct().Count() == 1,
					testCase.Describe("a case states either targets or patterns, never both"));
			}
		}

		[Fact]
		public void No_two_cases_share_a_subject_and_a_filesystem()
		{
			// Two cases with the same input and different expectations cannot both pass.
			// The input is argv *and* the tree it runs against: several cases reuse one
			// command line to show what a hidden entry or a symlink changes, and those
			// are different runs rather than a contradiction.
			var seen = new Dictionary<string, FixtureCase>();

			foreach (FixtureCase testCase in PlanCorpus.Load())
			{
				var key = new StringBuilder(testCase.Subject);

				foreach (KeyValuePair<string, string> given in testCase.Given)
					key.Append('\n').Append(given.Key).Append(' ').Append(given.Value.Trim());

				string identity = key.ToString();

				Assert.True(
					!seen.ContainsKey(identity),
					testCase.Describe("same command line and same filesystem as "
						+ (seen.TryGetValue(identity, out FixtureCase first) ? first.ToString() : "?")));

				seen[identity] = testCase;
			}
		}

		[Fact]
		public void The_dry_run_switch_itself_is_exercised()
		{
			// Every other case reaches the planner directly, so without this the
			// advertised switch could be missing entirely and the corpus still pass.
			Assert.Contains(PlanCorpus.Load(), c => c.Subject.Contains("--dry-run"));
			Assert.Contains(PlanCorpus.Load(), c => c.Subject.Contains("--print-plan"));
		}

		[Fact]
		public void The_corpus_covers_every_command()
		{
			// Coverage guard: a new verb must arrive with at least one case.
			var covered = new HashSet<string>(PlanCorpus.Load()
				.Select(c => PlanCorpus.SplitArguments(c.Subject))
				.Where(a => a.Length > 0)
				.Select(a => a[0]));

			foreach (string command in ArgumentTable.Commands)
				Assert.True(covered.Contains(command), "no corpus case exercises: " + command);
		}
	}

	/// <summary>
	/// The shared fixture reader, which the Python, Java and C++17 suites each
	/// reimplement in about twenty lines.
	/// </summary>
	public class FixtureReaderTests
	{
		[Fact]
		public void Comments_and_blank_lines_are_ignored()
		{
			var lines = new[] { "# a comment", "", "$ identify a.bin", "= exit 0" };

			Assert.Single(Fixture.Parse("test.txt", lines));
		}

		[Fact]
		public void A_case_collects_its_expectations_in_order()
		{
			var lines = new[] { "$ identify a.bin", "= command identify", "= exit 0" };

			FixtureCase parsed = Fixture.Parse("test.txt", lines)[0];

			Assert.Equal("identify a.bin", parsed.Subject);
			Assert.Equal(2, parsed.Expectations.Count);
			Assert.Equal("identify", parsed.Get("command"));
		}

		[Fact]
		public void A_repeated_key_keeps_every_value()
		{
			// How multi-target and multi-argument cases are expressed.
			var lines = new[] { "$ convert", "= target out/a.pdf", "= target out/b.pdf" };

			Assert.Equal(new[] { "out/a.pdf", "out/b.pdf" }, Fixture.Parse("test.txt", lines)[0].GetAll("target"));
		}

		[Fact]
		public void An_error_expectation_is_recognised()
		{
			var lines = new[] { "$ convert --to pdxf a.docx", "= error E_UNKNOWN_FORMAT", "= exit 2" };

			Assert.Equal("E_UNKNOWN_FORMAT", Fixture.Parse("test.txt", lines)[0].Error);
		}

		[Fact]
		public void Yes_and_no_expectations_parse_as_booleans()
		{
			var lines = new[] { "$ pdf", "= single-file yes", "= hidef no" };

			FixtureCase parsed = Fixture.Parse("test.txt", lines)[0];

			Assert.True(parsed.GetBool("single-file"));
			Assert.False(parsed.GetBool("hidef"));
		}

		[Fact]
		public void Anything_other_than_yes_or_no_is_rejected()
		{
			var lines = new[] { "$ pdf", "= single-file true" };

			Assert.ThrowsAny<System.IO.InvalidDataException>(() => Fixture.Parse("test.txt", lines)[0].GetBool("single-file"));
		}

		[Fact]
		public void An_empty_subject_is_allowed()
		{
			// formats.txt uses one to assert that an empty --to is rejected.
			var lines = new[] { "$", "= error E_UNKNOWN_FORMAT" };

			Assert.Equal(string.Empty, Fixture.Parse("test.txt", lines)[0].Subject);
		}

		[Fact]
		public void An_expectation_before_any_case_is_an_error()
		{
			Assert.ThrowsAny<System.IO.InvalidDataException>(
				() => Fixture.Parse("test.txt", new[] { "= command convert" }));
		}

		[Fact]
		public void A_case_keeps_what_it_is_given_apart_from_what_it_expects()
		{
			var lines = new[] { "$ convert a.docx", "< tree a.docx", "= command convert" };

			FixtureCase parsed = Fixture.Parse("test.txt", lines)[0];

			Assert.Equal(new[] { "a.docx" }, parsed.GivenAll("tree"));
			Assert.Single(parsed.Expectations);
			Assert.False(parsed.Has("tree"));
		}

		[Fact]
		public void What_a_case_is_given_comes_before_what_it_expects()
		{
			// '<' describes the world before the run and '=' what the run must produce, so
			// a fixture that interleaved them would read as though the order meant
			// something.
			Assert.ThrowsAny<System.IO.InvalidDataException>(
				() => Fixture.Parse("test.txt", new[] { "$ convert a.docx", "= command convert", "< tree a.docx" }));
		}

		[Fact]
		public void A_given_line_before_any_case_is_an_error()
		{
			Assert.ThrowsAny<System.IO.InvalidDataException>(
				() => Fixture.Parse("test.txt", new[] { "< tree a.docx" }));
		}

		[Fact]
		public void An_unrecognised_line_is_an_error()
		{
			// Rather than being skipped, which would silently drop expectations.
			Assert.ThrowsAny<System.IO.InvalidDataException>(
				() => Fixture.Parse("test.txt", new[] { "$ convert", "command convert" }));
		}

		[Fact]
		public void A_failure_message_names_the_file_the_line_and_the_whole_case()
		{
			// The reason externalising these cases is affordable at all: a failure has
			// to be readable without opening the fixture.
			var lines = new[] { "# header", "$ pdf", "= kind canvas", "= ext pdf" };

			string message = Fixture.Parse("formats.txt", lines)[0].Describe("kind was text");

			Assert.Contains("formats.txt:2", message);
			Assert.Contains("$ pdf", message);
			Assert.Contains("kind was text", message);
			Assert.Contains("= kind canvas", message);
			Assert.Contains("= ext pdf", message);
		}

		[Fact]
		public void Quoted_arguments_may_contain_spaces()
		{
			Assert.Equal(
				new[] { "convert", "--options", "BW_MODE=3;X=1", "a.docx" },
				PlanCorpus.SplitArguments("convert --options \"BW_MODE=3;X=1\" a.docx"));
		}
	}
}
