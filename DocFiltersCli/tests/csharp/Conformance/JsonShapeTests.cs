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

using System.Linq;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Runtime;
using Hyland.DocumentFilters.Cli.Tests.Cli;
using Hyland.DocumentFilters.Cli.Tests.Fakes;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Conformance
{
	/// <summary>
	/// The <c>--json</c> shape of every command that has one.
	/// </summary>
	/// <remarks>
	/// These five shapes are the tool's machine-readable interface, so a renamed key
	/// or a restructured array breaks whatever is parsing it. They used to be
	/// described only by a conformance fixture that nothing ever executed, and
	/// asserted nowhere: no test called <c>WriteJson</c> at all, and the end-to-end
	/// script only checked that the output parsed. See D52.
	///
	/// Driven through <see cref="CliHarness"/> rather than the report classes, so the
	/// assertion covers the path a user actually takes — argv, dispatch, the command,
	/// <c>JsonWriter</c> — and the engine answers from a script, which is what lets
	/// the expected document be written out in full rather than described.
	/// </remarks>
	public class JsonShapeTests
	{
		private static string Json(CliHarness cli, params string[] args)
		{
			Assert.Equal(ExitCode.Success, cli.Run(args));
			Assert.Empty(cli.Errors);

			// Raw, not the line list: a JSON report is built as one document and
			// written in a single call, so line capture misses it entirely.
			return cli.Output.Replace("\r\n", "\n").TrimEnd('\n');
		}

		[Fact]
		public void Identify_reports_an_array_of_path_format_mime_and_name()
		{
			var cli = new CliHarness().WithDocument("sample.docx");
			FakeDocument document = cli.Engine.AddDocument("sample.docx");
			document.FileType = 254;
			document.MimeType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
			document.FormatName = "Microsoft Word 2007";

			Assert.Equal(string.Join("\n",
				"[",
				"  {",
				"    \"path\": \"sample.docx\",",
				"    \"format\": 254,",
				"    \"mime\": \"application/vnd.openxmlformats-officedocument.wordprocessingml.document\",",
				"    \"name\": \"Microsoft Word 2007\"",
				"  }",
				"]"),
				Json(cli, "identify", "--json", "sample.docx"));
		}

		[Fact]
		public void Meta_reports_metadata_as_a_nested_object_keyed_by_field_name()
		{
			// Nested rather than flat, and every value a string: the engine reports
			// metadata as text, so a port that guessed at numbers would disagree about
			// "Pages".
			var cli = new CliHarness().WithDocument("sample.docx",
				"\u000FAuthor: Jane Smith\u000ETitle: Quarterly Report\u000EPages: 12\u0010");

			Assert.Equal(string.Join("\n",
				"[",
				"  {",
				"    \"path\": \"sample.docx\",",
				"    \"metadata\": {",
				"      \"Author\": \"Jane Smith\",",
				"      \"Title\": \"Quarterly Report\",",
				"      \"Pages\": \"12\"",
				"    }",
				"  }",
				"]"),
				Json(cli, "meta", "--json", "sample.docx"));
		}

		[Fact]
		public void List_reports_subfiles_as_an_array_inside_each_container()
		{
			var cli = new CliHarness().WithDocument("bundle.zip");
			FakeDocument container = cli.Engine.AddDocument("bundle.zip");
			container.SubFileEntries.Add(new FakeSubFile("readme.txt", "0001")
			{
				Size = 4096,
				Modified = new DateTime(2026, 1, 15, 9, 30, 0, DateTimeKind.Utc)
			});

			Assert.Equal(string.Join("\n",
				"[",
				"  {",
				"    \"path\": \"bundle.zip\",",
				"    \"subfiles\": [",
				"      {",
				"        \"id\": \"0001\",",
				"        \"name\": \"readme.txt\",",
				"        \"size\": 4096,",
				"        \"modified\": \"2026-01-15T09:30:00Z\",",
				"        \"depth\": 1",
				"      }",
				"    ]",
				"  }",
				"]"),
				Json(cli, "list", "--json", "bundle.zip"));
		}

		[Fact]
		public void Formats_reports_a_flat_array_of_id_ext_and_name()
		{
			var cli = new CliHarness();
			cli.Engine.Formats.Add(new EngineFormat { Id = 254, ShortName = "docx", DisplayName = "Microsoft Word 2007" });
			cli.Engine.Formats.Add(new EngineFormat { Id = 112, ShortName = "doc", DisplayName = "Microsoft Word for Windows" });

			Assert.Equal(string.Join("\n",
				"[",
				"  {",
				"    \"id\": 254,",
				"    \"ext\": \"docx\",",
				"    \"name\": \"Microsoft Word 2007\"",
				"  },",
				"  {",
				"    \"id\": 112,",
				"    \"ext\": \"doc\",",
				"    \"name\": \"Microsoft Word for Windows\"",
				"  }",
				"]"),
				Json(cli, "formats", "--json", "--filter", "word"));
		}

		[Fact]
		public void Options_reports_name_type_default_appliesTo_and_description()
		{
			// appliesTo is the one camelCase key in the whole surface — everything else
			// is a single lowercase word, so it is easy to port as "applies-to" or
			// "appliesto" by accident.
			var cli = new CliHarness();
			cli.Engine.Options.Add(new EngineOption
			{
				DisplayName = "BW_MODE",
				Type = "int",
				DefaultValue = "0",
				Flags = 0x2,   // canvas; 0x1 is document, and both join as "document,canvas"
				Description = "Black and white rendering mode"
			});

			Assert.Equal(string.Join("\n",
				"[",
				"  {",
				"    \"name\": \"BW_MODE\",",
				"    \"type\": \"int\",",
				"    \"default\": \"0\",",
				"    \"appliesTo\": \"canvas\",",
				"    \"description\": \"Black and white rendering mode\"",
				"  }",
				"]"),
				Json(cli, "options", "--json", "--filter", "BW_MODE"));
		}

		[Fact]
		public void Every_json_command_emits_one_document_even_with_nothing_to_report()
		{
			// An empty array, not empty output: whatever is parsing this should not
			// have to special-case "no results" as a parse failure.
			var cli = new CliHarness();

			Assert.Equal("[]", Json(cli, "formats", "--json", "--filter", "nothing-matches-this"));
			Assert.Equal("[]", Json(new CliHarness(), "options", "--json", "--filter", "nothing-matches-this"));
		}
	}
}
