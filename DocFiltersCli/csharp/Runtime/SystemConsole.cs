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

using System.Text;
using Hyland.DocumentFilters.Cli.Configuration;

namespace Hyland.DocumentFilters.Cli.Runtime
{
	/// <summary>
	/// The real console. Honours <c>--silent</c>, <c>--verbose</c> and <c>--color</c>.
	/// </summary>
	/// <remarks>
	/// <c>--color</c> is applied here, by the sink, and only where the sink already
	/// knows what a line <em>is</em>: an error is red, verbose chatter is dim. Report
	/// output — what the user actually asked for — is never coloured, because it is
	/// routinely piped into something else and because this class cannot tell one part
	/// of a report from another. Colouring those would mean giving <see cref="IConsole"/>
	/// semantic roles for call sites to choose from, which is a wider contract for
	/// every port to mirror than a sample warrants.
	/// </remarks>
	public sealed class SystemConsole : IConsole
	{
		// Built from the code point rather than written as an escape sequence, so no
		// source file in any port carries a literal control byte — those survive a
		// copy-paste or a diff viewer badly, and C#'s own \x escape is variable-length.
		private static readonly string Escape = ((char)27).ToString();
		private static readonly string Red = Escape + "[31m";
		private static readonly string Dim = Escape + "[2m";
		private static readonly string Reset = Escape + "[0m";

		private readonly ConsoleOptions _options;
		private readonly bool _colorErrors;
		private readonly bool _colorVerbose;

		/// <summary>
		/// Decides per stream whether colour is wanted and would be seen.
		/// </summary>
		/// <remarks>
		/// Redirection is checked once, here, rather than per line: a redirected stream
		/// is a file or a pipe, and escape sequences written into one are noise in
		/// whatever reads it next. The two streams are asked separately because they
		/// are redirected separately — <c>2&gt;log</c> leaves stdout a terminal.
		/// </remarks>
		public SystemConsole(ConsoleOptions options)
			: this(options,
				   ShouldColor(options.Color, System.Console.IsErrorRedirected),
				   ShouldColor(options.Color, System.Console.IsOutputRedirected))
		{
		}

		/// <summary>
		/// Whether to colour a stream: asked for, and going somewhere that can show it.
		/// </summary>
		/// <remarks>
		/// Named and pure so the rule can be checked without a terminal, and so each
		/// port mirrors one expression rather than re-deriving it. Detecting redirection
		/// is the ambient half and stays at the call site.
		/// </remarks>
		public static bool ShouldColor(bool requested, bool streamRedirected)
		{
			return requested && !streamRedirected;
		}

		/// <summary>
		/// The same, with the colour decision supplied rather than detected.
		/// </summary>
		/// <remarks>
		/// Only tests use this. Capturing what this class writes means redirecting
		/// <c>System.Console</c>, and the ordinary constructor would then — correctly —
		/// decline to colour a redirected stream, so a test built on it could only ever
		/// observe the uncoloured case.
		/// </remarks>
		public SystemConsole(ConsoleOptions options, bool colorErrors, bool colorVerbose)
		{
			_options = options;
			_colorErrors = colorErrors;
			_colorVerbose = colorVerbose;
		}

		/// <summary><c>--silent</c> means "print nothing but errors", so this is a no-op under it.</summary>
		public void Write(string text)
		{
			if (_options.Silent)
				return;

			System.Console.Write(text);
		}

		public void WriteLine(string text)
		{
			if (_options.Silent)
				return;

			System.Console.WriteLine(text);
		}

		/// <summary>Always written, even under <c>--silent</c>: stderr is the contract.</summary>
		/// <remarks>
		/// Colour wraps the line without altering it, so the contract text is still
		/// exactly what a reader sees once the terminal has consumed the sequences —
		/// and is byte-for-byte unchanged whenever stderr is redirected, which is how
		/// the conformance tests and every script read it.
		/// </remarks>
		public void WriteError(string text)
		{
			System.Console.Error.WriteLine(_colorErrors ? Red + text + Reset : text);
		}

		public void WriteVerbose(string text)
		{
			if (!_options.Verbose || _options.Silent)
				return;

			System.Console.WriteLine(_colorVerbose ? Dim + text + Reset : text);
		}

		public Stream OpenStandardOutput()
		{
			return System.Console.OpenStandardOutput();
		}

		public bool IsInputInteractive => !System.Console.IsInputRedirected;

		public IReadOnlyList<string> ReadAllInputLines()
		{
			var lines = new List<string>();
			string line;
			while ((line = System.Console.In.ReadLine()) != null)
				lines.Add(line);
			return lines;
		}

		/// <summary>Escape cancels and returns null; Enter accepts what was typed so far.</summary>
		public string ReadPassword(string prompt)
		{
			System.Console.Write(prompt);
			var password = new StringBuilder();

			while (true)
			{
				ConsoleKeyInfo key = System.Console.ReadKey(intercept: true);

				if (key.Key == ConsoleKey.Enter)
				{
					System.Console.WriteLine();
					return password.ToString();
				}

				if (key.Key == ConsoleKey.Escape)
				{
					System.Console.WriteLine();
					return null;
				}

				if (key.Key == ConsoleKey.Backspace)
				{
					if (password.Length > 0)
					{
						password.Length -= 1;
						System.Console.Write("\b \b");
					}
					continue;
				}

				if (!char.IsControl(key.KeyChar))
				{
					password.Append(key.KeyChar);
					System.Console.Write('*');
				}
			}
		}

		public void WaitForKey()
		{
			System.Console.ReadKey(intercept: true);
		}
	}
}
