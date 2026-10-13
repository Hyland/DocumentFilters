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

using System.Globalization;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Configuration
{
	/// <summary>
	/// Turns command line text into typed values.
	/// </summary>
	/// <remarks>
	/// Every conversion that can fail lives here rather than in the CLI parsing
	/// library, so that a bad value produces the same message and the same exit code
	/// in the C#, Python, Java and C++17 builds. The parsers themselves are only ever
	/// asked for <c>bool</c>, <c>string</c> and <c>List&lt;string&gt;</c>, none of
	/// which can fail to convert.
	/// </remarks>
	public static class ValueParsers
	{
        /// <summary>Parses a non-negative integer, e.g. for <c>--max-depth</c>.</summary>
        /// <exception cref="Runtime.UsageError">The text is not a non-negative integer.</exception>
        public static int ParseCount(string argument, string text)
        {
            if (!int.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out int value) || value < 0)
                Throw(argument, text);
            return value;
        }

		/// <summary>Parses a positive integer of seconds, e.g. for <c>--timeout</c>.</summary>
		public static int ParseSeconds(string argument, string text)
        {
            if (!int.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out int value) || value <= 0)
                Throw(argument, text);
            return value;
        }

        /// <summary>Parses a size in megabytes, e.g. for <c>--max-memory</c>.</summary>
        public static long ParseMegabytes(string argument, string text)
        {
            if (!long.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out long value) || value <= 0)
                Throw(argument, text);
            return value;
        }

        /// <summary>
        /// Splits a semicolon-separated id list, e.g. <c>--include-types "PDF;DOCX"</c>.
        /// Empty entries are dropped; surrounding whitespace is trimmed.
        /// </summary>
        public static List<string> ParseIdList(string text)
		{
			var result = new List<string>();
			foreach (string part in text.Split(';'))
			{
				string trimmed = part.Trim();
				if (trimmed.Length > 0)
					result.Add(trimmed);
			}
			return result;
		}

		/// <summary>Parses one <c>--redact page,x,y,w,h</c> value.</summary>
		/// <exception cref="Runtime.UsageError">Wrong number of parts, or a part is not an integer.</exception>
		public static RedactSpec ParseRedaction(string text)
		{
			string[] parts = text.Split(',');
			if (parts.Length != 5)
				Throw("--redact", text);

			int[] values = new int[5];
			for (int i = 0; i < 5; i++)
			{
				if (!int.TryParse(parts[i], NumberStyles.None, CultureInfo.InvariantCulture, out values[i]))
					Throw("--redact", text);
			}

			return new RedactSpec { Page = values[0], X = values[1], Y = values[2], Width = values[3], Height = values[4] };
		}

		/// <summary>Parses one <c>--form page,name=value</c> value.</summary>
		/// <remarks>Only the first <c>=</c> separates name from value, so values may contain <c>=</c>.</remarks>
		public static FormValue ParseFormValue(string text)
		{
			int comma = text.IndexOf(',');
			if (comma < 0)
				Throw("--form", text);

			string pageText = text.Substring(0, comma);
			string rest = text.Substring(comma + 1);

			int equals = rest.IndexOf('=');
			if (equals < 0)
				Throw("--form", text);

			if (!int.TryParse(pageText, NumberStyles.None, CultureInfo.InvariantCulture, out int page))
				Throw("--form", text);

			return new FormValue
			{
				Page = page,
				Name = rest.Substring(0, equals),
				Value = rest.Substring(equals + 1)
			};
		}

		/// <summary>
		/// Parses <c>--on-error</c>: "skip" or "stop", after <see cref="FoldToken"/>.
		/// </summary>
		public static OnErrorBehaviour ParseOnError(string text)
		{
			switch (text)
			{
				case "skip":
					return OnErrorBehaviour.Skip;
				case "stop":
					return OnErrorBehaviour.Stop;
				default:
					Throw("--on-error", text);
					return default;
			}
		}

		/// <summary>
		/// Folds a value drawn from a vocabulary this tool publishes, so that PDF, Pdf
		/// and pdf are one token.
		/// </summary>
		/// <remarks>
		/// Applied by <c>--to</c>, <c>--on-error</c>,
		/// <c>--applies-to</c> and the two log-level arguments — every
		/// argument whose valid values are enumerated here rather than supplied by the
		/// engine or the user. Paths, globs, regular expressions, <c>--out-name</c>
		/// templates, <c>--filter</c> text, passwords, <c>--codepage</c> and the
		/// <c>--options</c> string are used exactly as typed.
		///
		/// <b>ASCII only, and that is the whole point.</b> A locale-aware fold maps I to
		/// the dotless ı under a Turkish locale, so <c>--to TIFF</c> would stop resolving
		/// on a Turkish machine — identically in C#, Java and Python, and in C++ where
		/// <c>std::tolower</c> is locale-sensitive too. This maps A-Z to a-z and leaves
		/// every other character alone.
		/// </remarks>
		public static string FoldToken(string text)
		{
			char[] chars = text.ToCharArray();
			for (int i = 0; i < chars.Length; i++)
			{
				if (chars[i] >= 'A' && chars[i] <= 'Z')
					chars[i] = (char)(chars[i] - 'A' + 'a');
			}
			return new string(chars);
		}

		private static void Throw(string argument, string text)
		{
			throw new UsageError(ErrorCodes.BadValue, $"{argument} does not accept the value '{text}'");
		}
	}
}
