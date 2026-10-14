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

using System;
using System.Collections.Generic;
using System.Linq;
using Hyland.DocumentFilters.Cli.Cli;
using Hyland.DocumentFilters.Cli.Tests.Conformance;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The command line surface, checked against the shared <c>arguments.txt</c>
	/// fixture.
	/// </summary>
	/// <remarks>
	/// The fixture is written from the CLI design document, not dumped from any
	/// implementation. That direction is the point: a file generated from the C#
	/// build could only prove the other three agree with C#, whereas this proves
	/// all four agree with the specification — so an argument nobody implemented still
	/// fails.
	///
	/// This asks "does the right set of arguments exist, with the right arity?".
	/// <see cref="ArgumentBindingTests"/> asks "does each one do what it says?".
	/// </remarks>
	public class ArgumentTableTests
	{
		public static IEnumerable<object[]> Commands()
		{
			return Fixture.Load("arguments.txt").Select(c => new object[] { c });
		}

		/// <summary>One <c>= argument template arity</c> row.</summary>
		private sealed class ArgumentRow
		{
			public string Template { get; private set; }

			public ArgumentArity Arity { get; private set; }

			/// <summary>Long name without dashes, e.g. <c>out-dir</c>.</summary>
			public string LongName { get; private set; }

			/// <summary>Short name without its dash, or null.</summary>
			public string ShortName { get; private set; }

			public static ArgumentRow Parse(string row, FixtureCase testCase)
			{
				string[] parts = row.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);

				if (parts.Length != 2)
					throw new System.IO.InvalidDataException(testCase.Describe($"argument row needs 'template arity': {row}"));

				var result = new ArgumentRow { Template = parts[0], Arity = ParseArity(parts[1], testCase) };

				foreach (string form in parts[0].Split('|'))
				{
					if (form.StartsWith("--", StringComparison.Ordinal))
						result.LongName = form.Substring(2);
					else if (form.StartsWith("-", StringComparison.Ordinal))
						result.ShortName = form.Substring(1);
				}

				// An argument written only in short form, such as -x, is named by that form.
				if (result.LongName == null)
					result.LongName = result.ShortName;

				return result;
			}

			private static ArgumentArity ParseArity(string text, FixtureCase testCase)
			{
				switch (text)
				{
					case "none": return ArgumentArity.NoValue;
					case "single": return ArgumentArity.SingleValue;
					case "multiple": return ArgumentArity.MultipleValue;
					default:
						throw new System.IO.InvalidDataException(testCase.Describe($"unknown arity '{text}'"));
				}
			}
		}

		[Theory]
		[MemberData(nameof(Commands))]
		public void Command_declares_exactly_the_arguments_in_the_shared_fixture(FixtureCase testCase)
		{
			string command = testCase.Subject;

			var expected = testCase.GetAll("argument")
				.Select(row => ArgumentRow.Parse(row, testCase))
				.ToList();

			var declared = ArgumentTable.DescribeCommand(command)
				.ToDictionary(f => f.LongName, f => f);

			// Nothing missing.
			foreach (ArgumentRow row in expected)
			{
				Assert.True(
					declared.ContainsKey(row.LongName),
					testCase.Describe($"'{command}' does not declare {row.Template}"));

				IArgumentDescriptor actual = declared[row.LongName];

				Assert.True(
					actual.Arity == row.Arity,
					testCase.Describe($"{row.Template}: arity is {actual.Arity}, fixture says {row.Arity}"));

				Assert.True(
					(actual.ShortName ?? string.Empty) == (row.ShortName ?? string.Empty),
					testCase.Describe(
						$"{row.Template}: short form is '{actual.ShortName}', fixture says '{row.ShortName}'"));
			}

			// And nothing extra: an undocumented argument is drift just as surely as a
			// missing one, and would never be implemented in the other three.
			var expectedNames = new HashSet<string>(expected.Select(r => r.LongName));

			foreach (IArgumentDescriptor actual in ArgumentTable.DescribeCommand(command))
			{
				Assert.True(
					expectedNames.Contains(actual.LongName),
					testCase.Describe($"'{command}' declares --{actual.LongName}, which arguments.txt does not list"));
			}
		}

		// ------------------------------------------- behaviour the fixture cannot state

		[Fact]
		public void The_fixture_covers_every_processing_command()
		{
			var covered = new HashSet<string>(Fixture.Load("arguments.txt").Select(c => c.Subject));

			foreach (string command in ArgumentTable.Commands)
				Assert.True(covered.Contains(command), "arguments.txt has no section for: " + command);
		}

		[Fact]
		public void The_fixture_lists_no_command_the_tool_does_not_have()
		{
			foreach (FixtureCase testCase in Fixture.Load("arguments.txt"))
			{
				Assert.True(
					ArgumentTable.Commands.Contains(testCase.Subject),
					testCase.Describe("arguments.txt describes a command the tool does not declare"));
			}
		}

		[Fact]
		public void The_seven_processing_commands_are_declared_in_help_order()
		{
			Assert.Equal(
				new[] { "convert", "extract", "list", "identify", "meta", "formats", "options" },
				ArgumentTable.Commands.ToArray());
		}

		[Fact]
		public void Help_and_version_are_verbs_but_not_processing_commands()
		{
			// They accept no shared options and process no documents, so they are
			// dispatchable without appearing in the applicability table.
			Assert.Contains("help", ArgumentTable.AllVerbs);
			Assert.Contains("version", ArgumentTable.AllVerbs);
			Assert.DoesNotContain("help", ArgumentTable.Commands);
			Assert.DoesNotContain("version", ArgumentTable.Commands);
		}

		[Theory]
		[InlineData("convert")]
		[InlineData("extract")]
		[InlineData("list")]
		[InlineData("identify")]
		[InlineData("meta")]
		[InlineData("formats")]
		[InlineData("options")]
		public void No_command_declares_the_same_argument_twice(string command)
		{
			var seen = new HashSet<string>();

			foreach (IArgumentDescriptor argument in ArgumentTable.DescribeCommand(command))
				Assert.True(seen.Add(argument.LongName), command + " declares --" + argument.LongName + " twice");
		}

		[Theory]
		[InlineData("convert")]
		[InlineData("extract")]
		[InlineData("list")]
		[InlineData("identify")]
		[InlineData("meta")]
		[InlineData("formats")]
		[InlineData("options")]
		public void No_command_reuses_a_short_name(string command)
		{
			var seen = new HashSet<string>();

			foreach (IArgumentDescriptor argument in ArgumentTable.DescribeCommand(command))
			{
				if (string.IsNullOrEmpty(argument.ShortName))
					continue;

				Assert.True(seen.Add(argument.ShortName), command + " reuses -" + argument.ShortName);
			}
		}

		[Theory]
		[InlineData("convert")]
		[InlineData("extract")]
		[InlineData("list")]
		[InlineData("identify")]
		[InlineData("meta")]
		[InlineData("formats")]
		[InlineData("options")]
		public void Every_argument_has_help_text(string command)
		{
			// Prose, so it is not in the fixture — help text may differ in wording
			// between the four implementations. It may not be absent.
			foreach (IArgumentDescriptor argument in ArgumentTable.DescribeCommand(command))
				Assert.False(string.IsNullOrWhiteSpace(argument.Description), "--" + argument.LongName + " has no description");
		}

		[Fact]
		public void A_shared_argument_keeps_the_same_arity_on_every_command_that_accepts_it()
		{
			// The defining property of a shared option, and true without exception:
			// --glob cannot be repeatable on convert and single-valued on list, and
			// --regex cannot be a path pattern on one command and a mode switch on
			// another.
			var arities = new Dictionary<string, ArgumentArity>();
			var firstSeenOn = new Dictionary<string, string>();

			foreach (string command in ArgumentTable.Commands)
			{
				foreach (IArgumentDescriptor argument in ArgumentTable.DescribeCommand(command))
				{
					if (arities.TryGetValue(argument.LongName, out ArgumentArity existing))
					{
						Assert.True(
							existing == argument.Arity,
							$"--{argument.LongName} is {existing} on {firstSeenOn[argument.LongName]} but {argument.Arity} on {command}");
					}
					else
					{
						arities[argument.LongName] = argument.Arity;
						firstSeenOn[argument.LongName] = command;
					}
				}
			}
		}

		[Fact]
		public void Extract_accepts_the_shared_regex_like_every_other_walking_command()
		{
			// extract walks directories like the others, so it selects paths the same
			// way: -g/--glob and --regex, with --include/--exclude picking subfiles.
			var declared = ArgumentTable.DescribeCommand("extract").Select(a => a.LongName).ToList();

			Assert.Contains("regex", declared);
			Assert.Contains("include", declared);
			Assert.DoesNotContain("filter", declared);
		}

		[Fact]
		public void Include_and_exclude_are_a_matched_pair_on_extract()
		{
			// A matched pair: same arity, opposite direction.
			IArgumentDescriptor include = ArgumentTable.DescribeCommand("extract").Single(a => a.LongName == "include");
			IArgumentDescriptor exclude = ArgumentTable.DescribeCommand("extract").Single(a => a.LongName == "exclude");

			Assert.Equal(exclude.Arity, include.Arity);
		}
	}
}
