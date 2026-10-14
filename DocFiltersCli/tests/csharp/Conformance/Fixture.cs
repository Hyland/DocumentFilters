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
using System.IO;
using System.Linq;
using System.Text;

namespace Hyland.DocumentFilters.Cli.Tests.Conformance
{
	/// <summary>
	/// One case from a shared fixture: a subject, the world it runs in, and its
	/// expectations.
	/// </summary>
	public sealed class FixtureCase
	{
		/// <summary>The text after <c>$</c> — a command line, a format token, a template.</summary>
		public string Subject { get; set; }

		/// <summary>
		/// The <c>&lt;</c> lines: what the case is given, in fixture order. Keys may
		/// repeat.
		/// </summary>
		/// <remarks>
		/// Held apart from <see cref="Expectations"/> rather than distinguished by key
		/// name, because they are the opposite kind of thing: <c>&lt;</c> describes the
		/// world before the run and <c>=</c> describes what the run must produce. A
		/// reader that mixed them would make every consumer filter by a list of key
		/// names it had to keep in step.
		/// </remarks>
		public List<KeyValuePair<string, string>> Given { get; } = new List<KeyValuePair<string, string>>();

		/// <summary>The <c>=</c> lines, in fixture order. Keys may repeat.</summary>
		public List<KeyValuePair<string, string>> Expectations { get; } = new List<KeyValuePair<string, string>>();

		/// <summary>Fixture filename, for failure messages.</summary>
		public string File { get; set; }

		/// <summary>1-based line of the <c>$</c> line.</summary>
		public int Line { get; set; }

		/// <summary>The expected error code, when the case expects a failure.</summary>
		public string Error => Get("error");

		/// <summary>
		/// The single value for a key, trimmed, or null when the key is absent.
		/// </summary>
		/// <remarks>
		/// Values are stored verbatim and trimmed here, because fixtures pad them into
		/// a readable column and almost no case cares about that padding. The
		/// exceptions use <see cref="GetRaw"/>, for a value whose own leading
		/// whitespace is part of what it asserts.
		/// </remarks>
		public string Get(string key)
		{
			string value = GetRaw(key);
			return value?.Trim();
		}

		/// <summary>The single value for a key exactly as written, padding included.</summary>
		public string GetRaw(string key)
		{
			foreach (KeyValuePair<string, string> pair in Expectations)
			{
				if (pair.Key == key)
					return pair.Value;
			}

			return null;
		}

		/// <summary>Every value for a repeated key, trimmed, in fixture order.</summary>
		public IReadOnlyList<string> GetAll(string key)
		{
			return Expectations.Where(p => p.Key == key).Select(p => p.Value.Trim()).ToList();
		}

		/// <summary>
		/// Every value for a repeated key exactly as written, with one separating space
		/// removed and everything beyond it kept.
		/// </summary>
		/// <remarks>
		/// This is what the golden output cases use. <c>= json    "a": 1</c> yields
		/// <c>   "a": 1</c> — the first space after the key separates, the rest is the
		/// value's own indentation.
		/// </remarks>
		public IReadOnlyList<string> GetAllRaw(string key)
		{
			return Expectations.Where(p => p.Key == key).Select(p => p.Value).ToList();
		}

		/// <summary>True when the fixture states an expectation for this key.</summary>
		public bool Has(string key) => Expectations.Any(p => p.Key == key);

		/// <summary>Every value for a repeated <c>&lt;</c> key, trimmed, in fixture order.</summary>
		public IReadOnlyList<string> GivenAll(string key)
		{
			return Given.Where(p => p.Key == key).Select(p => p.Value.Trim()).ToList();
		}

		/// <summary>True when the case is given anything under this key.</summary>
		public bool HasGiven(string key) => Given.Any(p => p.Key == key);

		/// <summary>A yes/no expectation. Accepts only "yes" and "no", so the fixtures stay uniform.</summary>
		public bool GetBool(string key)
		{
			string value = Get(key);

			if (value == "yes") return true;
			if (value == "no") return false;

			throw new InvalidDataException(Describe($"'{key}' must be yes or no, was '{value}'"));
		}

		/// <summary>
		/// A failure message that names the fixture and line, states the problem, and
		/// reproduces the whole case, so a failing run does not send the reader off to
		/// open another file.
		/// </summary>
		public string Describe(string problem)
		{
			var text = new StringBuilder();
			text.Append(File).Append(':').Append(Line).Append("  $ ").Append(Subject).Append('\n');
			text.Append("  ").Append(problem).Append('\n');
			text.Append("  case as written:\n");

			foreach (KeyValuePair<string, string> pair in Given)
				text.Append("    < ").Append(pair.Key).Append(' ').Append(pair.Value).Append('\n');

			foreach (KeyValuePair<string, string> pair in Expectations)
				text.Append("    = ").Append(pair.Key).Append(' ').Append(pair.Value).Append('\n');

			return text.ToString();
		}

		public override string ToString() => File + ":" + Line + " " + Subject;
	}

	/// <summary>
	/// Reader for the shared conformance fixtures.
	/// </summary>
	/// <remarks>
	/// Every fixture uses one format, so each language needs exactly one reader
	/// rather than one per file. A case is a <c>$</c> subject line, then any
	/// <c>&lt;</c> lines saying what the case is given, then <c>=</c> lines saying what
	/// it must produce. Blank lines separate cases and <c>#</c> starts a comment.
	///
	/// Deliberately free of JSON and YAML so the C++17 and Java suites can parse it
	/// with nothing but their standard library.
	/// </remarks>
	public static class Fixture
	{
		/// <summary>Where the fixtures are copied to next to the test assembly.</summary>
		public static string Directory => Path.Combine(AppContext.BaseDirectory, "conformance");

		/// <summary>Loads a fixture by filename, e.g. <c>formats.txt</c>.</summary>
		public static IReadOnlyList<FixtureCase> Load(string name)
		{
			string path = Path.Combine(Directory, name);

			if (!System.IO.File.Exists(path))
				throw new FileNotFoundException("conformance fixture not found: " + path, path);

			return Parse(name, System.IO.File.ReadAllLines(path));
		}

		/// <summary>Parses fixture text. Exposed so the reader itself can be tested.</summary>
		public static IReadOnlyList<FixtureCase> Parse(string file, IReadOnlyList<string> lines)
		{
			var cases = new List<FixtureCase>();
			FixtureCase current = null;

			for (int i = 0; i < lines.Count; i++)
			{
				string line = lines[i].Trim();

				if (line.StartsWith("#", StringComparison.Ordinal))
					continue;

				if (line.Length == 0)
				{
					// A blank line ends the current case, as the format documents. Merely
					// skipping it would silently attach a later '=' line to the case above
					// the gap — which a stricter reader in another language would reject,
					// so the same fixture would mean different things in different ports.
					current = null;
					continue;
				}

				if (line.StartsWith("$", StringComparison.Ordinal))
				{
					current = new FixtureCase
					{
						Subject = line.Substring(1).Trim(),
						File = file,
						Line = i + 1
					};
					cases.Add(current);
					continue;
				}

				if (line.StartsWith("<", StringComparison.Ordinal))
				{
					if (current == null)
						throw new InvalidDataException($"{file}:{i + 1}: '<' line with no preceding '$' case");

					if (current.Expectations.Count > 0)
					{
						throw new InvalidDataException(
							$"{file}:{i + 1}: '<' describes what the case is given and comes before every '=' line");
					}

					current.Given.Add(SplitKeyValue(line));
					continue;
				}

				if (line.StartsWith("=", StringComparison.Ordinal))
				{
					if (current == null)
						throw new InvalidDataException($"{file}:{i + 1}: expectation with no preceding '$' case");

					current.Expectations.Add(SplitKeyValue(line));
					continue;
				}

				throw new InvalidDataException($"{file}:{i + 1}: unrecognised line: {line}");
			}

			return cases;
		}

		/// <summary>
		/// Splits a <c>&lt;</c> or <c>=</c> line into its key and its value.
		/// </summary>
		/// <remarks>
		/// Exactly one space separates the key from the value; everything after it is
		/// stored verbatim. Callers that do not care about the column padding use
		/// Get/GetAll, which trim. GetRaw/GetAllRaw exist for a value whose own leading
		/// whitespace is part of what it asserts, and must survive the reader.
		/// </remarks>
		private static KeyValuePair<string, string> SplitKeyValue(string line)
		{
			string rest = line.Substring(1).TrimStart();
			int space = rest.IndexOf(' ');
			string key = space < 0 ? rest : rest.Substring(0, space);
			string value = space < 0 ? string.Empty : rest.Substring(space + 1);

			return new KeyValuePair<string, string>(key, value);
		}
	}
}
