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
using Hyland.DocumentFilters.Cli.Tests.Fakes;

namespace Hyland.DocumentFilters.Cli.Tests.Conformance
{
	/// <summary>
	/// The plan corpus: <see cref="Fixture"/> plus the one thing specific to it,
	/// splitting a subject line into command line arguments.
	/// </summary>
	public static class PlanCorpus
	{
		public const string FileName = "plan-corpus.txt";

		/// <summary>Every case in the corpus.</summary>
		public static IReadOnlyList<FixtureCase> Load() => Fixture.Load(FileName);

		/// <summary>The expected exit code for a case, defaulting to success.</summary>
		public static int ExpectedExit(FixtureCase testCase)
		{
			string value = testCase.Get("exit");
			return value == null ? 0 : int.Parse(value);
		}

		/// <summary>
		/// Builds the filesystem a case is given by its <c>&lt;</c> lines.
		/// </summary>
		/// <remarks>
		/// Never the real one. A real directory listing has no defined order and differs
		/// between ext4, NTFS and APFS, so a corpus backed by real files could not be
		/// byte-identical across four languages and three platforms. The fake also models
		/// symlinks and hidden entries as plain sets, which is what lets --follow and
		/// --include-hidden be contract at all: creating a symlink on Windows needs a
		/// privilege most checkouts do not have, and "hidden" is a dot-prefix on POSIX
		/// and a file attribute on Windows.
		/// </remarks>
		public static InMemoryFileSystem BuildFileSystem(FixtureCase testCase)
		{
			var fileSystem = new InMemoryFileSystem();

			foreach (string row in testCase.GivenAll("tree"))
			{
				foreach (string path in SplitPaths(row))
				{
					if (path.EndsWith("/", StringComparison.Ordinal))
						fileSystem.AddDirectory(path.Substring(0, path.Length - 1));
					else
						fileSystem.AddFile(path);
				}
			}

			// Links after the tree, so a link may point at a directory the tree created.
			foreach (string row in testCase.GivenAll("link"))
			{
				string[] parts = SplitPaths(row);

				if (parts.Length != 2)
					throw new InvalidDataException(testCase.Describe("'< link' needs '<path> <target>': " + row));

				fileSystem.MarkSymlink(parts[0], parts[1]);
			}

			// Hidden last: it marks an entry the two above have already created.
			foreach (string row in testCase.GivenAll("hidden"))
			{
				foreach (string path in SplitPaths(row))
					fileSystem.MarkHidden(path);
			}

			return fileSystem;
		}

		private static string[] SplitPaths(string row)
		{
			return row.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
		}

		/// <summary>
		/// Splits a corpus subject into arguments on whitespace, honouring double
		/// quotes so a value may contain spaces.
		/// </summary>
		public static string[] SplitArguments(string commandLine)
		{
			var args = new List<string>();
			var current = new StringBuilder();
			bool quoted = false;

			foreach (char c in commandLine)
			{
				if (c == '"')
				{
					quoted = !quoted;
				}
				else if (char.IsWhiteSpace(c) && !quoted)
				{
					if (current.Length > 0)
					{
						args.Add(current.ToString());
						current.Clear();
					}
				}
				else
				{
					current.Append(c);
				}
			}

			if (current.Length > 0)
				args.Add(current.ToString());

			return args.ToArray();
		}
	}
}
