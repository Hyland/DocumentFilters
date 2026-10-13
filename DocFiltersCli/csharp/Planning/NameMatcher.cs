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
using System.Text.RegularExpressions;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Planning
{
	/// <summary>
	/// Path selection from <c>-g/--glob</c> or <c>--regex</c>.
	/// </summary>
	/// <remarks>
	/// Glob is the default idiom. Patterns are matched against the path as written,
	/// using forward slashes on every platform so that a pattern is portable.
	/// <c>*</c> matches within one path segment, <c>**</c> crosses segments, and a
	/// leading <c>!</c> turns a pattern into an exclusion.
	///
	/// Multiple patterns combine as: an entry is kept when it matches at least one
	/// include pattern (or there are none) and matches no exclude pattern.
	/// </remarks>
	public sealed class NameMatcher
	{
		private List<string> _globPatterns;
		private List<string> _regexPatterns;

		private readonly List<Regex> _includes = new List<Regex>();
		private readonly List<Regex> _excludes = new List<Regex>();

        /// <summary>Builds a matcher from one or more <c>-g/--glob</c> patterns.</summary>
        /// <exception cref="Runtime.UsageError">A pattern is malformed.</exception>
        public static NameMatcher FromGlobs(IReadOnlyList<string> patterns)
		{
			var matcher = new NameMatcher { _globPatterns = new List<string>(patterns) };
			foreach (string raw in patterns)
			{
				bool isExclude = raw.StartsWith("!", StringComparison.Ordinal);
				string body = isExclude ? raw.Substring(1) : raw;
				Regex regex = CompileGlob(body, raw);
				(isExclude ? matcher._excludes : matcher._includes).Add(regex);
			}
            return matcher;
        }

		/// <summary>Builds a matcher from one or more <c>--regex</c> patterns.</summary>
		/// <exception cref="Runtime.UsageError">A pattern is not a valid regular expression.</exception>
		public static NameMatcher FromRegexes(IReadOnlyList<string> patterns)
		{
			var matcher = new NameMatcher { _regexPatterns = new List<string>(patterns) };
			foreach (string raw in patterns)
			{
				try
				{
                    // wrap the regex in ^(?: ... )\z so that it is anchored to the beginning and end of the string,
                    // and it's in a non-capturing group, which is useful if the user has a regex with alternation (|) at the top level.
                    matcher._includes.Add(new Regex($"^(?:{raw})\\z", RegexOptions.Compiled));
                }
				catch (ArgumentException)
				{
					throw new UsageError(ErrorCodes.BadValue, $"invalid regular expression '{raw}'");
				}
			}
			return matcher;
		}

		/// <summary>True when the path should be processed.</summary>
		public bool Matches(string path)
		{
			string normalized = path.Replace('\\', '/');

			if (_excludes.Any(regex => regex.IsMatch(normalized)))
				return false;

            return _includes.Count == 0 || _includes.Any(regex => regex.IsMatch(normalized));
		}

		/// <summary>Translates one <c>-g/--glob</c> pattern (minus any leading <c>!</c>) into an anchored regex.</summary>
		private static Regex CompileGlob(string pattern, string original)
		{
			string[] rawSegments = pattern.Split('/');
			var segments = new List<string>();
			foreach (string segment in rawSegments)
			{
				if (segment == "**" && segments.Count > 0 && segments[^1] == "**")
					continue;
				segments.Add(segment);
			}

			var sb = new StringBuilder("^");
			for (int i = 0; i < segments.Count; i++)
			{
				string segment = segments[i];
				bool isFirst = i == 0;
				bool isLast = i == segments.Count - 1;
				bool previousIsDoubleStar = !isFirst && segments[i - 1] == "**";

				if (segment == "**")
				{
					if (!isFirst)
						sb.Append('/');
					sb.Append(isLast ? ".*" : "(?:.*/)?");
				}
				else
				{
					if (!isFirst && !previousIsDoubleStar)
						sb.Append('/');
					sb.Append(TranslateGlobSegment(segment, original));
				}
			}
			sb.Append("\\z$");

			try
			{
				return new Regex(sb.ToString(), RegexOptions.Compiled);
			}
			catch (ArgumentException)
			{
				throw new UsageError(ErrorCodes.BadValue, $"invalid glob pattern '{original}'");
			}
		}

		/// <summary>
		/// Translates one path segment: <c>*</c> and <c>?</c> stay within the segment,
		/// everything else is literal — including a bracket pair, which this tool's
		/// glob dialect gives no matching meaning to, so <c>[abc]</c> matches only that
		/// literal five-character text. An unterminated <c>[</c> is still rejected as
		/// malformed rather than silently taken as a literal character: it is almost
		/// certainly a typo, not an intentional five-character filename fragment.
		/// </summary>
		private static string TranslateGlobSegment(string segment, string original)
		{
			var sb = new StringBuilder();
			int i = 0;
			while (i < segment.Length)
			{
				char c = segment[i];
				switch (c)
				{
					case '*':
						sb.Append("[^/]*");
						i += 1;
						break;
					case '?':
						sb.Append("[^/]");
                        i += 1;
						break;
					case '[':
						int close = segment.IndexOf(']', i + 1);
						if (close < 0)
							throw new UsageError(ErrorCodes.BadValue, $"invalid glob pattern '{original}'");
						sb.Append(Regex.Escape(segment.Substring(i, close - i + 1)));
						i = close + 1;
						break;
					default:
						sb.Append(Regex.Escape(c.ToString()));
                        i += 1;
						break;
				}
			}
			return sb.ToString();
		}

		/// <summary>The canonical text form, used by the conformance corpus.</summary>
		public override string ToString()
		{
			if (_globPatterns != null)
                return $"glob {String.Join(", ", _globPatterns)}";
			if (_regexPatterns != null)
                return $"regex {String.Join(", ", _regexPatterns)}";
            return String.Empty;
        }
	}
}
