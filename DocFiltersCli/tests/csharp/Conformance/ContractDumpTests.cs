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
using System.Text.Json;
using Hyland.DocumentFilters.Cli.Cli;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Conformance
{
	/// <summary>
	/// <c>--print-contract</c>: the tool's own account of its command line surface.
	/// </summary>
	/// <remarks>
	/// A debugging aid and the machine-readable answer to <c>help --json</c>, so what
	/// is tested here is that it faithfully reflects the argument table.
	///
	/// The surface itself is checked against <c>arguments.txt</c> in
	/// <see cref="Unit.ArgumentTableTests"/>.
	/// </remarks>
	public class ContractDumpTests
	{
		[Fact]
		public void Every_verb_appears_in_the_dump()
		{
			string dump = ContractDump.Render();

			foreach (string command in ArgumentTable.Commands)
				Assert.Contains("command " + command, dump);
		}

		[Fact]
		public void Every_argument_on_every_command_appears_in_the_dump()
		{
			foreach (string command in ArgumentTable.Commands)
			{
				string section = ContractDump.RenderCommand(command);

				foreach (IArgumentDescriptor argument in ArgumentTable.DescribeCommand(command))
					Assert.True(section.Contains("--" + argument.LongName), $"{command}: --{argument.LongName} missing from the dump");
			}
		}

		[Fact]
		public void The_dump_declares_nothing_the_argument_table_does_not_have()
		{
			// Without this a stale line in the dump would send the other three chasing a
			// argument that no longer exists.
			foreach (string command in ArgumentTable.Commands)
			{
				var declared = new HashSet<string>(ArgumentTable.DescribeCommand(command).Select(f => f.LongName));

				foreach (string name in LongNamesIn(ContractDump.RenderCommand(command)))
					Assert.True(declared.Contains(name), $"{command}: --{name} is in the dump but not in the argument table");
			}
		}

		[Fact]
		public void The_dump_agrees_with_the_shared_fixture()
		{
			// Transitively true given the two tests above and ArgumentTableTests, but worth
			// asserting directly: this is the pair of files a reader will diff by hand
			// when something looks wrong.
			foreach (FixtureCase testCase in Fixture.Load("arguments.txt"))
			{
				string section = ContractDump.RenderCommand(testCase.Subject);

				foreach (string row in testCase.GetAll("argument"))
				{
					string template = row.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries)[0];
					string longForm = template.Split('|').Last();

					Assert.True(
						section.Contains(longForm),
						testCase.Describe($"{longForm} is in arguments.txt but not in the contract dump"));
				}
			}
		}

		[Fact]
		public void Each_argument_records_its_arity()
		{
			// The other three need this to register an argument correctly: a switch declared
			// as taking a value silently swallows the next argument.
			string section = ContractDump.RenderCommand("convert");

			Assert.Contains("none", section);
			Assert.Contains("single", section);
			Assert.Contains("multiple", section);
		}

		[Fact]
		public void The_dump_is_deterministic()
		{
			Assert.Equal(ContractDump.Render(), ContractDump.Render());
		}

		[Fact]
		public void The_dump_is_line_oriented_and_free_of_json()
		{
			// Not "contains no brace" - a description is free to mention template
			// placeholders like {stem} or another flag's name. The actual property
			// that matters is that the dump, as a whole, is not itself JSON: valid
			// JSON permits nothing but whitespace outside its one outermost value,
			// so a document with "command convert" before any brace, English prose,
			// and multiple top-level lines can never parse as one JSON document -
			// unlike RenderJson's output, which always does.
			string dump = ContractDump.Render();

			Assert.ThrowsAny<JsonException>(() => JsonDocument.Parse(dump));
		}

		[Fact]
		public void The_dump_uses_lf_line_endings()
		{
			// Otherwise it differs between a Windows and a Linux build.
			Assert.DoesNotContain("\r", ContractDump.Render());
		}

		// ------------------------------------------------------------- help --json

		[Fact]
		public void The_json_rendering_is_actually_json()
		{
			// The whole reason it is a separate renderer: the flat dump is asserted
			// brace-free just above, so an argument named --json could not share it.
			string json = ContractDump.RenderJson();

			Assert.StartsWith("[", json.TrimStart());
			Assert.EndsWith("\n", json);
			Assert.DoesNotContain("\r", json);
		}

		[Fact]
		public void The_json_rendering_names_every_command()
		{
			string json = ContractDump.RenderJson();

			foreach (string command in ArgumentTable.Commands)
				Assert.Contains("\"" + command + "\"", json);
		}

		[Fact]
		public void The_json_rendering_names_every_argument_on_every_command()
		{
			string json = ContractDump.RenderJson();

			foreach (string command in ArgumentTable.Commands)
			{
				foreach (IArgumentDescriptor argument in ArgumentTable.DescribeCommand(command))
					Assert.True(json.Contains("--" + argument.LongName), $"{command}: --{argument.LongName} missing from the JSON");
			}
		}

		[Fact]
		public void The_json_rendering_carries_arity_and_help_text()
		{
			// A port registering arguments from this needs the arity; a person reading
			// it wants the description. Both have to survive the rendering.
			string json = ContractDump.RenderJson();

			Assert.Contains("\"arity\"", json);
			Assert.Contains("\"description\"", json);
			Assert.Contains("\"template\"", json);
		}

		[Fact]
		public void The_json_rendering_is_deterministic()
		{
			Assert.Equal(ContractDump.RenderJson(), ContractDump.RenderJson());
		}

		[Fact]
		public void The_two_renderings_describe_the_same_surface()
		{
			// They share the argument table rather than the renderer, so nothing but a
			// test keeps them agreeing.
			string json = ContractDump.RenderJson();

			foreach (string command in ArgumentTable.Commands)
			{
				foreach (string name in LongNamesIn(ContractDump.RenderCommand(command)))
					Assert.True(json.Contains("--" + name), $"--{name} is in the flat dump but not in the JSON");
			}
		}

		/// <summary>
		/// Pulls the long argument names back out of a rendered section by reading
		/// only the template column, never the free-form description — a line's
		/// tab-separated fields are <c>argument</c>, template, arity, description, and
		/// only the template is safe to search for <c>--name</c> tokens. A word
		/// split over the whole line would just as happily find one inside prose
		/// that happens to mention another flag by name.
		/// </summary>
		private static IEnumerable<string> LongNamesIn(string section)
		{
			foreach (string line in section.Split('\n'))
			{
				string trimmed = line.Trim();
				if (!trimmed.StartsWith("argument", StringComparison.Ordinal))
					continue;

				string[] fields = trimmed.Split('\t');
				if (fields.Length < 2)
					continue;

				foreach (string form in fields[1].Split('|'))
				{
					if (form.StartsWith("--", StringComparison.Ordinal))
						yield return form.Substring(2);
				}
			}
		}
	}
}
