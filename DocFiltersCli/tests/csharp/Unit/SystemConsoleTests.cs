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
using System.IO;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Runtime;
using Xunit;

namespace Hyland.DocumentFilters.Cli.Tests.Unit
{
	/// <summary>
	/// <c>--color</c>, asserted against the real console rather than a fake.
	/// </summary>
	/// <remarks>
	/// <see cref="Fakes.RecordingConsole"/> deliberately knows nothing about colour:
	/// its <c>ErrorLines</c> are the cross-language contract and are asserted
	/// character for character, so letting escape sequences into them would mean every
	/// one of those assertions had to strip ANSI first — and a wrong sequence would
	/// then pass. Colour is presentation, the fake captures content. That leaves
	/// <see cref="SystemConsole"/> as the only thing that can be asserted on here,
	/// which is the point: a flag tested only through "the option got set" is what let
	/// nine others sit unimplemented.
	///
	/// These tests redirect <c>System.Console</c>, which is process-wide state. xUnit
	/// runs the tests within one class serially, and nothing else in this suite writes
	/// to the real console, so the redirection cannot be observed by another test.
	/// </remarks>
	public class SystemConsoleTests
	{
		private static readonly string Escape = ((char)27).ToString();

		private static ConsoleOptions Options(bool verbose = false)
		{
			return new ConsoleOptions { Color = true, Verbose = verbose };
		}

		/// <summary>Runs <paramref name="write"/> and returns what landed on stderr.</summary>
		private static string CaptureError(SystemConsole console, Action<SystemConsole> write)
		{
			TextWriter original = Console.Error;
			var captured = new StringWriter();
			try
			{
				Console.SetError(captured);
				write(console);
			}
			finally
			{
				Console.SetError(original);
			}

			return captured.ToString();
		}

		/// <summary>Runs <paramref name="write"/> and returns what landed on stdout.</summary>
		private static string CaptureOut(SystemConsole console, Action<SystemConsole> write)
		{
			TextWriter original = Console.Out;
			var captured = new StringWriter();
			try
			{
				Console.SetOut(captured);
				write(console);
			}
			finally
			{
				Console.SetOut(original);
			}

			return captured.ToString();
		}

		// ------------------------------------------------------------ the rule

		[Theory]
		[InlineData(true, false, true)]   // asked for, terminal
		[InlineData(true, true, false)]   // asked for, but redirected
		[InlineData(false, false, false)] // not asked for
		[InlineData(false, true, false)]
		public void Colour_is_used_only_when_asked_for_and_the_stream_can_show_it(
			bool requested, bool redirected, bool expected)
		{
			Assert.Equal(expected, SystemConsole.ShouldColor(requested, redirected));
		}

		// ------------------------------------------------------- what is written

		[Fact]
		public void An_error_is_wrapped_in_red()
		{
			var console = new SystemConsole(Options(), colorErrors: true, colorVerbose: true);

			string written = CaptureError(console, c => c.WriteError("E_DOCUMENT_FAILED: boom"));

			Assert.Equal(Escape + "[31m" + "E_DOCUMENT_FAILED: boom" + Escape + "[0m", written.TrimEnd('\r', '\n'));
		}

		[Fact]
		public void An_error_carries_no_escapes_when_colour_is_off()
		{
			var console = new SystemConsole(Options(), colorErrors: false, colorVerbose: false);

			string written = CaptureError(console, c => c.WriteError("E_DOCUMENT_FAILED: boom"));

			// Asserted as an exact match rather than "contains no escape": xUnit's
			// DoesNotContain reports a match at position 0 when the needle is a control
			// character, so it would pass here for the wrong reason.
			Assert.Equal("E_DOCUMENT_FAILED: boom", written.TrimEnd('\r', '\n'));
		}

		[Fact]
		public void Verbose_chatter_is_dimmed()
		{
			var console = new SystemConsole(Options(verbose: true), colorErrors: true, colorVerbose: true);

			string written = CaptureOut(console, c => c.WriteVerbose("opening a.pdf"));

			Assert.Equal(Escape + "[2m" + "opening a.pdf" + Escape + "[0m", written.TrimEnd('\r', '\n'));
		}

		/// <summary>
		/// The deliberate half of the decision: <c>--color</c> must not reach the answer
		/// the user asked for, which is routinely piped into something that would choke
		/// on escapes. A change that colours reports uniformly would pass every test
		/// above and fail this one.
		/// </summary>
		[Fact]
		public void Report_output_is_never_coloured()
		{
			var console = new SystemConsole(Options(), colorErrors: true, colorVerbose: true);

			string line = CaptureOut(console, c => c.WriteLine("a.pdf: format=51"));
			string chunk = CaptureOut(console, c => c.Write("a.pdf: format=51"));

			Assert.Equal("a.pdf: format=51", line.TrimEnd('\r', '\n'));
			Assert.Equal("a.pdf: format=51", chunk);
		}

		/// <summary>
		/// <c>--silent</c> still wins over <c>--color</c>: an error is the one thing that
		/// survives it, and it survives coloured.
		/// </summary>
		[Fact]
		public void Silent_suppresses_coloured_report_output_but_not_coloured_errors()
		{
			var console = new SystemConsole(
				new ConsoleOptions { Color = true, Silent = true, Verbose = true },
				colorErrors: true, colorVerbose: true);

			Assert.Equal(string.Empty, CaptureOut(console, c => c.WriteLine("a.pdf: format=51")));
			Assert.Equal(string.Empty, CaptureOut(console, c => c.WriteVerbose("opening a.pdf")));

			string error = CaptureError(console, c => c.WriteError("E_DOCUMENT_FAILED: boom"));
			Assert.Equal(Escape + "[31m" + "E_DOCUMENT_FAILED: boom" + Escape + "[0m", error.TrimEnd('\r', '\n'));
		}
	}
}
