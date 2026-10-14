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
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// The conversions that can fail. These live in shared code precisely so that a
	/// bad value produces the same message and the same exit code in the C#, Python,
	/// Java and C++17 builds, rather than four different parsing libraries' wording.
	/// </summary>
	public class ValueParsersTests
	{
		[Theory]
		[InlineData("0", 0)]
		[InlineData("1", 1)]
		[InlineData("42", 42)]
		public void Counts_parse(string text, int expected)
		{
			Assert.Equal(expected, ValueParsers.ParseCount("--max-depth", text));
		}

		[Theory]
		[InlineData("abc")]
		[InlineData("")]
		[InlineData("-1")]
		[InlineData("1.5")]
		[InlineData("1e3")]
		[InlineData(" 1")]
		public void A_bad_count_is_a_usage_error(string text)
		{
			UsageError error = Assert.Throws<UsageError>(() => ValueParsers.ParseCount("--max-depth", text));

			Assert.Equal(ErrorCodes.BadValue, error.Code);
		}

		[Fact]
		public void The_error_names_the_argument_that_was_wrong()
		{
			// Without the argument name, a batch of options makes the message useless.
			UsageError error = Assert.Throws<UsageError>(() => ValueParsers.ParseCount("--max-depth", "abc"));

			Assert.Contains("--max-depth", error.Message);
		}

		// -------------------------------------------------------------------- seconds

		[Theory]
		[InlineData("1", 1)]
		[InlineData("60", 60)]
		[InlineData("86400", 86400)]
		public void Durations_parse(string text, int expected)
		{
			Assert.Equal(expected, ValueParsers.ParseSeconds("--timeout", text));
		}

		[Theory]
		[InlineData("0")]
		[InlineData("-1")]
		[InlineData("abc")]
		[InlineData("")]
		[InlineData("1.5")]
		[InlineData("30s")]
		public void A_bad_duration_is_a_usage_error(string text)
		{
			// Zero is rejected as well as negative: a timeout of nothing is a mistake,
			// not a request to finish instantly.
			UsageError error = Assert.Throws<UsageError>(() => ValueParsers.ParseSeconds("--timeout", text));

			Assert.Equal(ErrorCodes.BadValue, error.Code);
		}

		// ----------------------------------------------------------------- megabytes

		[Theory]
		[InlineData("1", 1L)]
		[InlineData("512", 512L)]
		[InlineData("8192", 8192L)]
		public void Memory_limits_parse(string text, long expected)
		{
			Assert.Equal(expected, ValueParsers.ParseMegabytes("--max-memory", text));
		}

		[Fact]
		public void A_memory_limit_beyond_int_range_still_parses()
		{
			// The value is megabytes and the field is a long, so a limit larger than
			// int.MaxValue is legitimate rather than an overflow.
			Assert.Equal(4194304L, ValueParsers.ParseMegabytes("--max-memory", "4194304"));
		}

		[Theory]
		[InlineData("0")]
		[InlineData("-1")]
		[InlineData("abc")]
		[InlineData("")]
		[InlineData("512MB")]
		[InlineData("1.5")]
		public void A_bad_memory_limit_is_a_usage_error(string text)
		{
			UsageError error = Assert.Throws<UsageError>(() => ValueParsers.ParseMegabytes("--max-memory", text));

			Assert.Equal(ErrorCodes.BadValue, error.Code);
		}

		[Fact]
		public void Both_limit_parsers_name_the_argument_that_was_wrong()
		{
			Assert.Contains("--timeout", Assert.Throws<UsageError>(
				() => ValueParsers.ParseSeconds("--timeout", "x")).Message);

			Assert.Contains("--max-memory", Assert.Throws<UsageError>(
				() => ValueParsers.ParseMegabytes("--max-memory", "x")).Message);
		}

		// ------------------------------------------------------------------- id lists

		[Fact]
		public void Id_lists_split_on_semicolons()
		{
			Assert.Equal(new[] { "PDF", "DOCX" }, ValueParsers.ParseIdList("PDF;DOCX"));
		}

		[Fact]
		public void Id_lists_trim_whitespace_and_drop_empty_entries()
		{
			Assert.Equal(new[] { "PDF", "DOCX" }, ValueParsers.ParseIdList(" PDF ; ; DOCX ;"));
		}

		[Fact]
		public void A_single_id_needs_no_separator()
		{
			Assert.Equal(new[] { "PDF" }, ValueParsers.ParseIdList("PDF"));
		}

		// ------------------------------------------------------------------ redactions

		[Fact]
		public void A_redaction_parses_as_page_then_rectangle()
		{
			RedactSpec spec = ValueParsers.ParseRedaction("2,10,20,100,50");

			Assert.Equal(2, spec.Page);
			Assert.Equal(10, spec.X);
			Assert.Equal(20, spec.Y);
			Assert.Equal(100, spec.Width);
			Assert.Equal(50, spec.Height);
		}

		[Theory]
		[InlineData("2,10,20,100")]
		[InlineData("2,10,20,100,50,60")]
		[InlineData("2,10,20,100,abc")]
		[InlineData("")]
		public void A_malformed_redaction_is_a_usage_error(string text)
		{
			Assert.Throws<UsageError>(() => ValueParsers.ParseRedaction(text));
		}

		// ----------------------------------------------------------------- form values

		[Fact]
		public void A_form_value_parses_as_page_then_name_equals_value()
		{
			FormValue value = ValueParsers.ParseFormValue("1,surname=Smith");

			Assert.Equal(1, value.Page);
			Assert.Equal("surname", value.Name);
			Assert.Equal("Smith", value.Value);
		}

		[Fact]
		public void Only_the_first_equals_separates_name_from_value()
		{
			// Form values legitimately contain '=', for instance a base64 payload.
			FormValue value = ValueParsers.ParseFormValue("1,token=abc=def=");

			Assert.Equal("token", value.Name);
			Assert.Equal("abc=def=", value.Value);
		}

		[Fact]
		public void An_empty_form_value_is_allowed()
		{
			// Clearing a field is a legitimate operation.
			FormValue value = ValueParsers.ParseFormValue("1,surname=");

			Assert.Equal("", value.Value);
		}

		[Theory]
		[InlineData("1,surname")]
		[InlineData("surname=Smith")]
		[InlineData("")]
		public void A_malformed_form_value_is_a_usage_error(string text)
		{
			Assert.Throws<UsageError>(() => ValueParsers.ParseFormValue(text));
		}

		// --------------------------------------------------------------------- on-error

		[Theory]
		[InlineData("skip", OnErrorBehaviour.Skip)]
		[InlineData("stop", OnErrorBehaviour.Stop)]
		public void On_error_accepts_exactly_the_two_documented_values(string text, OnErrorBehaviour expected)
		{
			Assert.Equal(expected, ValueParsers.ParseOnError(text));
		}

		[Theory]
		[InlineData("SKIP")]
		[InlineData("Skip")]
		[InlineData("continue")]
		[InlineData("")]
		public void Anything_else_for_on_error_is_a_usage_error(string text)
		{
			// Case sensitive on purpose: if one implementation accepted "SKIP" and
			// another did not, the four would diverge on what is a valid command line.
			Assert.Throws<UsageError>(() => ValueParsers.ParseOnError(text));
		}
	}
}
