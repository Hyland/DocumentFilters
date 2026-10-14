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
using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The hand-rolled JSON writer.
	/// </summary>
	/// <remarks>
	/// These tests exist because Python has a JSON module and the other three do
	/// not: the Python port must reproduce exactly this output rather than accept
	/// whatever <c>json.dumps</c> defaults to. Key order, indent, escaping and the
	/// trailing newline are all contract.
	/// </remarks>
	public class JsonWriterTests
	{
		[Fact]
		public void An_empty_object_renders_on_one_line()
		{
			string json = new JsonWriter().BeginObject().EndObject().ToString();

			Assert.Equal("{}\n", json);
		}

		[Fact]
		public void Properties_are_indented_by_two_spaces()
		{
			string json = new JsonWriter()
				.BeginObject()
				.Property("name", "report.pdf")
				.EndObject()
				.ToString();

			Assert.Equal("{\n  \"name\": \"report.pdf\"\n}\n", json);
		}

		[Fact]
		public void Keys_stay_in_insertion_order_and_are_never_sorted()
		{
			// Sorting would be a gratuitous difference from the order the engine
			// reports fields in, and Python's json module sorts only on request.
			string json = new JsonWriter()
				.BeginObject()
				.Property("zebra", 1)
				.Property("apple", 2)
				.EndObject()
				.ToString();

			Assert.True(json.IndexOf("zebra", StringComparison.Ordinal) < json.IndexOf("apple", StringComparison.Ordinal));
		}

		[Fact]
		public void Nested_objects_indent_cumulatively()
		{
			string json = new JsonWriter()
				.BeginObject()
				.Name("inner").BeginObject().Property("a", 1).EndObject()
				.EndObject()
				.ToString();

			Assert.Equal("{\n  \"inner\": {\n    \"a\": 1\n  }\n}\n", json);
		}

		[Fact]
		public void Arrays_render_one_element_per_line()
		{
			string json = new JsonWriter()
				.BeginArray()
				.Value("a")
				.Value("b")
				.EndArray()
				.ToString();

			Assert.Equal("[\n  \"a\",\n  \"b\"\n]\n", json);
		}

		[Fact]
		public void An_empty_array_renders_on_one_line()
		{
			Assert.Equal("[]\n", new JsonWriter().BeginArray().EndArray().ToString());
		}

		[Fact]
		public void Booleans_and_numbers_are_unquoted()
		{
			string json = new JsonWriter()
				.BeginObject()
				.Property("count", 42)
				.Property("ok", true)
				.EndObject()
				.ToString();

			Assert.Contains("\"count\": 42", json);
			Assert.Contains("\"ok\": true", json);
		}

		[Fact]
		public void A_null_string_renders_as_null()
		{
			string json = new JsonWriter().BeginObject().Property("modified", (string)null).EndObject().ToString();

			Assert.Contains("\"modified\": null", json);
		}

		[Theory]
		[InlineData("a\"b", "a\\\"b")]
		[InlineData("a\\b", "a\\\\b")]
		[InlineData("a\nb", "a\\nb")]
		[InlineData("a\tb", "a\\tb")]
		[InlineData("a\rb", "a\\rb")]
		public void The_mandatory_escapes_are_applied(string input, string expected)
		{
			Assert.Equal(expected, JsonWriter.Escape(input));
		}

		[Fact]
		public void Control_characters_use_the_six_character_form()
		{
			Assert.Equal("\\u0000", JsonWriter.Escape("\0"));
		}

		[Fact]
		public void Non_ascii_is_written_literally_rather_than_escaped()
		{
			// Python's json.dumps escapes non-ASCII by default. It must not here, or the
			// four implementations produce different bytes for the same document.
			Assert.Equal("Ünïcödé", JsonWriter.Escape("Ünïcödé"));
		}

		[Fact]
		public void A_forward_slash_is_not_escaped()
		{
			// Legal either way in JSON, so it has to be pinned down.
			Assert.Equal("a/b", JsonWriter.Escape("a/b"));
		}

		[Fact]
		public void The_document_ends_with_exactly_one_newline()
		{
			string json = new JsonWriter().BeginObject().Property("a", 1).EndObject().ToString();

			Assert.EndsWith("}\n", json);
			Assert.DoesNotContain("\n\n", json);
		}

		[Fact]
		public void Line_endings_are_always_lf_even_on_windows()
		{
			string json = new JsonWriter().BeginObject().Property("a", 1).EndObject().ToString();

			Assert.DoesNotContain("\r", json);
		}

		[Fact]
		public void An_unclosed_object_is_a_programming_error()
		{
			Assert.Throws<InvalidOperationException>(() => new JsonWriter().BeginObject().ToString());
		}
	}
}
