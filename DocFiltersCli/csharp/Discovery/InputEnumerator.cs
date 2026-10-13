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

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Discovery
{
	/// <summary>
	/// Turns the positional paths and <c>--files-from</c> list into the concrete set
	/// of files to process.
	/// </summary>
	/// <remarks>
	/// Directories are walked recursively by default, the way <c>fd</c> and
	/// <c>ripgrep</c> behave; <c>--max-depth</c> and <c>--no-recurse</c> limit that.
	/// Directory depth is entirely separate from subfile depth, which controls
	/// recursion into documents nested inside other documents.
	///
	/// <c>-g/--glob</c> and <c>--regex</c> match against a walked entry's path
	/// relative to the root it was found under, not the root-qualified path: an
	/// anchored pattern such as <c>^report-\d{4}\.pdf$</c> would otherwise never
	/// match anything once the walked root itself has more than one path segment.
	/// An explicit file argument is never checked against either — naming a file
	/// is an instruction, not a suggestion.
	/// </remarks>
	public sealed class InputEnumerator
	{
		private readonly IFileSystem _fileSystem;
		private readonly IConsole _console;

		public InputEnumerator(IFileSystem fileSystem, IConsole console)
		{
			_fileSystem = fileSystem;
			_console = console;
		}

		/// <summary>Directory identities already entered by the in-progress <see cref="WalkDirectory"/> call.</summary>
		/// <remarks>
		/// Reset per root rather than per <see cref="InputEnumerator"/> instance:
		/// two different explicit roots are free to share a directory, but a link
		/// cycle within one root must not loop forever. Keyed by identity, not path,
		/// because a link to an ancestor produces an endless supply of distinct path
		/// strings for what is really one directory.
		/// </remarks>
		private HashSet<string> _visited;

		/// <summary>
		/// Produces the input list, in processing order. Selection by detected format
		/// (<c>--include-types</c> / <c>--exclude-types</c>) is not applied here,
		/// because it needs the engine to identify each file; that happens during
		/// execution.
		/// </summary>
		/// <exception cref="NoInputsError">Nothing matched.</exception>
		public IReadOnlyList<InputSource> Enumerate(InputOptions options)
		{
			var rawPaths = options.Paths.ToList();
			if (options.FilesFrom != null)
				rawPaths.AddRange(ReadFileList(options.FilesFrom));
			if (rawPaths.Count == 0)
				throw new NoInputsError("no inputs given; name a file or a directory to process");

			var results = new List<InputSource>();
			foreach (string path in rawPaths)
			{
				if (_fileSystem.DirectoryExists(path))
				{
					results.AddRange(WalkDirectory(path, options));
				}
				else if (_fileSystem.FileExists(path))
				{
					results.Add(new InputSource { Path = path, RelativePath = FileNameOf(path), Root = null });
				}
				else
				{
					throw new NoInputsError($"{path}: no such file or directory");
				}
			}

			if (options.Sort)
				results.Sort((a, b) => string.CompareOrdinal(a.Path, b.Path));

			if (results.Count == 0)
				throw new NoInputsError("no inputs matched; the walk and filters selected nothing");

			return results;
		}

		/// <summary>Reads a <c>--files-from</c> list. A path of "-" reads from stdin.</summary>
		internal IReadOnlyList<string> ReadFileList(string path)
		{
			IEnumerable<string> lines = path == "-" ? _console.ReadAllInputLines() : _fileSystem.ReadAllLines(path);
			return lines.Where(line => !string.IsNullOrWhiteSpace(line)).ToList();
		}

		/// <summary>
		/// Walks one directory tree, honouring depth, hidden entries and symlinks.
		/// </summary>
		internal IReadOnlyList<InputSource> WalkDirectory(string root, InputOptions options)
		{
			var results = new List<InputSource>();
			_visited = new HashSet<string>(StringComparer.Ordinal);
			WalkInto(root, root, 1, options, results);
			return results;
		}

		/// <summary>
		/// The recursive worker behind <see cref="WalkDirectory"/>, accumulating into a
		/// caller-owned list.
		/// </summary>
		/// <remarks>
		/// Accumulating rather than returning a list per level is deliberate: the
		/// portability rules rule out <c>yield return</c>, because Java has no clean
		/// equivalent and C++17 has none at all, and merging a list at every level of
		/// the recursion is four different idioms in four languages. Passing one list
		/// down ports unchanged.
		/// </remarks>
		private void WalkInto(string root, string current, int depth, InputOptions options, List<InputSource> results)
		{
			if (!_visited.Add(_fileSystem.GetDirectoryIdentity(current)))
				return;

			IReadOnlyList<string> entries;
			try
			{
				entries = _fileSystem.ListDirectory(current);
			}
			catch (Exception error) when (!(error is CliError))
			{
				// A directory the walk cannot read is reported, not skipped. Skipping it
				// silently produces a shorter input list that looks like a complete one —
				// and reporting nothing at all is what an unchecked listing used to do in
				// the C++ port, while this one let the platform's exception escape and end
				// the process.
				//
				// The message is fixed rather than the platform's own: .NET says "Access to
				// the path ... is denied", libstdc++ says "Permission denied", and error
				// text is part of the cross-language contract.
				throw new NoInputsError($"{current}: could not be read");
			}

			foreach (string entry in entries)
			{
				if (!options.IncludeHidden && _fileSystem.IsHidden(entry))
					continue;

				if (_fileSystem.DirectoryExists(entry))
				{
					if (_fileSystem.IsSymbolicLink(entry) && !options.FollowSymlinks)
						continue;

					int nextDepth = depth + 1;
					if (!options.NoRecurseWasGiven && (!options.MaxDepth.HasValue || nextDepth <= options.MaxDepth.Value))
						WalkInto(root, entry, nextDepth, options, results);
				}
				else
				{
					string relativePath = entry.Substring(root.Length).TrimStart('/');
					if (options.Matcher != null && !options.Matcher.Matches(relativePath))
						continue;

					results.Add(new InputSource
					{
						Path = entry,
						RelativePath = relativePath,
						Root = root
					});
				}
			}
		}

		/// <summary>The filename portion of an explicitly named file argument.</summary>
		/// <remarks>
		/// Unlike a walked entry's <see cref="InputSource.RelativePath"/> — already
		/// forward-slash because <see cref="WalkInto"/> built it from <see cref="IFileSystem.ListDirectory"/>
		/// output — an explicit argument is whatever the user typed on their own
		/// shell, backslashes included on Windows. <see cref="Planning.OutputPlanner.DirectoryOf"/>
		/// normalises before splitting for exactly this reason; skipping it here left
		/// a bare <c>C:\tmp\a.pdf</c> unrecognised as having a directory at all, so its
		/// full path — not just its filename — became "the relative path", and
		/// <c>--out-dir</c> nested the source drive underneath itself instead of
		/// flattening to the filename.
		/// </remarks>
		private static string FileNameOf(string path)
		{
			string normalised = path.Replace('\\', '/');
			int slash = normalised.LastIndexOf('/');
			return slash < 0 ? normalised : normalised.Substring(slash + 1);
		}
	}
}
