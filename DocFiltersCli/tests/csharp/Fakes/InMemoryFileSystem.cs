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
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Tests.Fakes
{
	/// <summary>
	/// A filesystem that lives entirely in a dictionary, so the directory walk and
	/// the output planner can be tested without touching disk.
	/// </summary>
	/// <remarks>
	/// Paths are normalised to forward slashes throughout, which is also how the
	/// real implementation matches globs, so a test written here means the same
	/// thing on Windows and Linux.
	///
	/// This is test infrastructure, so unlike the project under test it is fully
	/// implemented.
	/// </remarks>
	public sealed class InMemoryFileSystem : IFileSystem
	{
		private readonly Dictionary<string, byte[]> _files = new Dictionary<string, byte[]>(StringComparer.Ordinal);
		private readonly HashSet<string> _directories = new HashSet<string>(StringComparer.Ordinal);
		private readonly HashSet<string> _hidden = new HashSet<string>(StringComparer.Ordinal);
		private readonly HashSet<string> _symlinks = new HashSet<string>(StringComparer.Ordinal);
		private readonly Dictionary<string, string> _linkTargets = new Dictionary<string, string>(StringComparer.Ordinal);

		/// <summary>
		/// Names this filesystem matches to one file, beyond matching them exactly.
		/// </summary>
		/// <remarks>
		/// Stated case by case rather than folded by a rule, because the rule is the
		/// thing under test. A real volume decides for itself which spellings are one
		/// file --- NTFS matches Å.txt to å.txt, measured --- and the tool's own
		/// collision key cannot always predict it. Writing the pair down here lets a
		/// test say "these two reach one file" without this fake acquiring a Unicode
		/// table that the thing it stands in for would not agree with anyway.
		/// </remarks>
		private readonly Dictionary<string, string> _sameFile = new Dictionary<string, string>(StringComparer.Ordinal);

		/// <summary>Declares that two names reach one file.</summary>
		public InMemoryFileSystem TreatAsSameFile(string path, string sameAs)
		{
			_sameFile[Normalise(path)] = Normalise(sameAs);
			return this;
		}

		/// <summary>The one name a file is stored under, whatever spelling was asked for.</summary>
		private string Key(string path)
		{
			string normalised = Normalise(path);
			return _sameFile.TryGetValue(normalised, out string canonical) ? canonical : normalised;
		}

		/// <summary>Everything written through <see cref="CreateFile"/>, for assertions.</summary>
		public List<string> Created { get; } = new List<string>();

		/// <summary>Adds a file, creating its parent directories.</summary>
		public InMemoryFileSystem AddFile(string path, string contents = "")
		{
			path = Key(path);
			_files[path] = System.Text.Encoding.UTF8.GetBytes(contents);
			AddParents(path);
			return this;
		}

		/// <summary>Adds an empty directory.</summary>
		public InMemoryFileSystem AddDirectory(string path)
		{
			path = Normalise(path);
			_directories.Add(path);
			AddParents(path);
			return this;
		}

		/// <summary>Marks an existing entry hidden, so the walk skips it without <c>--include-hidden</c>.</summary>
		public InMemoryFileSystem MarkHidden(string path)
		{
			_hidden.Add(Normalise(path));
			return this;
		}

		/// <summary>
		/// Marks a directory as a symlink, so the walk enters it only under <c>-L</c>.
		/// </summary>
		/// <param name="path">The link itself.</param>
		/// <param name="target">
		/// What it resolves to. Point this at an ancestor to build the cycle that
		/// <c>--follow</c> has to survive; leave it null when the link's destination
		/// does not matter to the test.
		/// </param>
		public InMemoryFileSystem MarkSymlink(string path, string target = null)
		{
			string key = Normalise(path);
			_symlinks.Add(key);

			// The link is a real directory entry, so a walk of its parent finds it.
			// Without this the walk never sees the link at all and any test about
			// following one passes without exercising anything.
			_directories.Add(key);
			AddParents(key);

			if (target != null)
				_linkTargets[key] = Normalise(target);

			return this;
		}

		public bool FileExists(string path) => _files.ContainsKey(Key(path));

		public bool DirectoryExists(string path) => _directories.Contains(Normalise(path));

		public IReadOnlyList<string> ListDirectory(string path)
		{
			string requested = Normalise(path);

			// The same shape of failure a real filesystem raises for a directory the
			// process may not read: an exception, not an empty listing.
			if (Unreadable.Contains(requested))
				throw new UnauthorizedAccessException($"Access to the path '{requested}' is denied.");

			// Listing through a link yields the target's children, re-rooted under the
			// path that was asked for — which is what makes a link to an ancestor
			// generate the endless corpus/loop/loop/… that --follow has to survive.
			string real = ResolveThroughLinks(requested);

			if (real != requested)
			{
				var throughLink = new List<string>();

				foreach (string child in ListDirectory(real))
					throughLink.Add(requested + child.Substring(real.Length));

				return throughLink;
			}

			string prefix = requested;
			if (prefix.Length > 0 && !prefix.EndsWith("/", StringComparison.Ordinal))
				prefix += "/";

			var children = new HashSet<string>(StringComparer.Ordinal);
			foreach (string candidate in _files.Keys.Concat(_directories))
			{
				if (!candidate.StartsWith(prefix, StringComparison.Ordinal) || candidate.Length == prefix.Length)
					continue;

				string rest = candidate.Substring(prefix.Length);
				int slash = rest.IndexOf('/');
				children.Add(prefix + (slash < 0 ? rest : rest.Substring(0, slash)));
			}

			// Deliberately unordered: IFileSystem promises no order and a real directory
			// listing gives none. A fake that sorted would let a walk ignoring --sort
			// pass the sorting tests.
			var result = children.ToList();
			result.Sort(StringComparer.Ordinal);
			result.Reverse();
			return result;
		}

		public bool IsHidden(string path) => _hidden.Contains(Normalise(path));

		public bool IsSymbolicLink(string path) => _symlinks.Contains(Normalise(path));

		/// <summary>
		/// The directory a path resolves to, following any link. Two paths reaching the
		/// same directory return the same identity, which is what lets the walk detect
		/// a cycle.
		/// </summary>
		public string GetDirectoryIdentity(string path) => ResolveThroughLinks(Normalise(path));

		/// <summary>
		/// What this filesystem claims about case — <c>true</c> unless a test says
		/// otherwise.
		/// </summary>
		/// <remarks>
		/// Case-sensitive by default because this fake stores paths in an ordinal
		/// dictionary and so behaves that way; a default of anything else would have
		/// the fake's own lookups disagree with its answer here. Tests covering
		/// Windows and macOS set it false. The shared plan corpus leaves it alone, so
		/// its cases read the same on every platform.
		/// </remarks>
		public bool CaseSensitive { get; set; } = true;

		public bool IsCaseSensitive(string path) => CaseSensitive;

		/// <summary>Directories this filesystem refuses to list, as a real one may.</summary>
		public HashSet<string> Unreadable { get; } = new HashSet<string>(StringComparer.Ordinal);

		/// <summary>
		/// Rewrites a path through however many links stand in it, so that every route
		/// to one directory produces the same answer.
		/// </summary>
		/// <remarks>
		/// <c>corpus/loop/loop</c> resolves through <c>corpus/loop → corpus</c> twice
		/// and lands back at <c>corpus</c>. The iteration is bounded because a link to
		/// its own ancestor never settles — that is the whole point of the case, and
		/// stopping it is the walk's job, not the filesystem's.
		/// </remarks>
		private string ResolveThroughLinks(string path)
		{
			for (int hop = 0; hop < 64; hop++)
			{
				string rewritten = RewriteLongestLinkPrefix(path);

				if (rewritten == path)
					return path;

				path = rewritten;
			}

			return path;
		}

		private string RewriteLongestLinkPrefix(string path)
		{
			string bestLink = null;

			foreach (string link in _linkTargets.Keys)
			{
				bool matches = path == link || path.StartsWith(link + "/", StringComparison.Ordinal);

				if (matches && (bestLink == null || link.Length > bestLink.Length))
					bestLink = link;
			}

			return bestLink == null ? path : _linkTargets[bestLink] + path.Substring(bestLink.Length);
		}

		public Stream OpenRead(string path)
		{
			string key = Key(path);
			if (!_files.TryGetValue(key, out byte[] bytes))
				throw new FileNotFoundException("No such file in the fake filesystem: " + key, key);

			return new MemoryStream(bytes, writable: false);
		}

		public Stream CreateFile(string path)
		{
			string key = Key(path);
			Created.Add(key);
			AddParents(key);
			_files[key] = Array.Empty<byte>();
			return new CommittingStream(this, key);
		}

		/// <summary>Reads back what was written to a created file.</summary>
		public string ReadText(string path)
		{
			return System.Text.Encoding.UTF8.GetString(_files[Key(path)]);
		}

		/// <summary>
		/// A stream that writes its contents back into the fake when disposed.
		/// </summary>
		/// <remarks>
		/// Without this the stream returned by <see cref="CreateFile"/> would be
		/// detached: everything written to it would be discarded, and a later
		/// <see cref="OpenRead"/> would see an empty file. Conversion output could
		/// then never be asserted on.
		/// </remarks>
		private sealed class CommittingStream : MemoryStream
		{
			private readonly InMemoryFileSystem _fileSystem;
			private readonly string _path;

			public CommittingStream(InMemoryFileSystem fileSystem, string path)
			{
				_fileSystem = fileSystem;
				_path = path;
			}

			protected override void Dispose(bool disposing)
			{
				if (disposing)
					_fileSystem._files[_path] = ToArray();

				base.Dispose(disposing);
			}
		}

		public void CreateDirectory(string path) => AddDirectory(path);

		public void MoveFile(string from, string to)
		{
			string source = Key(from);
			string destination = Key(to);

			if (!_files.TryGetValue(source, out byte[] content))
				throw new FileNotFoundException("no such file: " + source);

			_files.Remove(source);
			_files[destination] = content;
			Created.Add(destination);
		}

		public IReadOnlyList<string> ReadAllLines(string path)
		{
			using (var reader = new StreamReader(OpenRead(path)))
			{
				var lines = new List<string>();
				string line;
				while ((line = reader.ReadLine()) != null)
					lines.Add(line);

				return lines;
			}
		}

		private void AddParents(string path)
		{
			int slash = path.LastIndexOf('/');
			while (slash > 0)
			{
				path = path.Substring(0, slash);
				_directories.Add(path);
				slash = path.LastIndexOf('/');
			}
		}

		private static string Normalise(string path) => path.Replace('\\', '/').TrimEnd('/');
	}
}
