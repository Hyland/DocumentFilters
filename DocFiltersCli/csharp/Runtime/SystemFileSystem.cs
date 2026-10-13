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

using System.Runtime.InteropServices;

namespace Hyland.DocumentFilters.Cli.Runtime
{
	/// <summary>The real filesystem.</summary>
	/// <remarks>
	/// Every path this returns uses forward slashes, whatever separator the OS or
	/// the caller used going in: <c>NameMatcher</c> and the rest of the portable
	/// walk logic compare paths as plain strings, so a backslash on Windows would
	/// make every glob and regex platform-specific.
	/// </remarks>
	public sealed class SystemFileSystem : IFileSystem
	{
		public bool FileExists(string path)
		{
			return File.Exists(path);
		}

		public bool DirectoryExists(string path)
		{
			return Directory.Exists(path);
		}

		public IReadOnlyList<string> ListDirectory(string path)
		{
			return Directory.EnumerateFileSystemEntries(path)
				.Select(Normalise)
				.ToList();
		}

		/// <summary>
		/// A dot-prefixed name is hidden on every platform this runs on, in addition
		/// to whatever the OS itself flags — POSIX has no hidden attribute at all,
		/// and checking only <see cref="FileAttributes.Hidden"/> would make
		/// <c>--include-hidden</c> a no-op there.
		/// </summary>
		public bool IsHidden(string path)
		{
			string name = Path.GetFileName(path.TrimEnd('/'));
			if (name.StartsWith(".", StringComparison.Ordinal))
				return true;

			return (File.GetAttributes(path) & FileAttributes.Hidden) != 0;
		}

		public bool IsSymbolicLink(string path)
		{
			// False for a path that does not exist, because extraction asks about every
			// component of a file it is about to create. A dangling link still answers
			// true: its attributes are its own, and writing through it would create its
			// target.
			try
			{
				return (File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0;
			}
			catch (Exception error) when (error is FileNotFoundException || error is DirectoryNotFoundException)
			{
				return false;
			}
		}

		public string GetDirectoryIdentity(string path)
		{
			FileSystemInfo target = Directory.ResolveLinkTarget(path, returnFinalTarget: true);
			string resolved = target?.FullName ?? Path.GetFullPath(path);
			return Normalise(resolved).TrimEnd('/');
		}

		/// <summary>
		/// Whether the volume holding <paramref name="path"/> tells names apart by case,
		/// measured where it can be and assumed where it cannot.
		/// </summary>
		/// <remarks>
		/// The host default alone is wrong in both directions, and only one of them is
		/// safe. On Windows and macOS, guessing case-insensitive for a case-sensitive
		/// volume refuses a run the user can fix with <c>--out-name</c>. On Linux,
		/// guessing case-sensitive for a mounted NTFS or exFAT volume lets two planned
		/// outputs collide into one file and overwrite each other — the very defect the
		/// collision check exists to catch.
		///
		/// So the directory is asked, and asked about itself: the probe flips the case of
		/// something already inside it and looks for that spelling in the same directory.
		/// Nothing is created, which is what lets the collision check run before any
		/// output exists and <c>--dry-run</c> still write nothing.
		///
		/// Inside, rather than the directory's own name in its parent, because the rules
		/// belong to the directory. A mount point is the case that makes the difference:
		/// probing <c>/mnt/NTFS</c> asks the Linux filesystem holding <c>/mnt</c>, which
		/// answers for itself and not for the volume mounted there, so every output under
		/// it would be planned as though case mattered. Windows can also mark one
		/// directory case-sensitive inside an insensitive volume, which the parent cannot
		/// see either.
		///
		/// Every letter is flipped rather than lowercased, so <c>Report.txt</c> is looked
		/// for as <c>rEPORT.TXT</c>. A second file really named that way would read as
		/// case-insensitive, which is the residual inaccuracy here — and it errs the safe
		/// way, because believing a volume insensitive only makes the collision check
		/// stricter.
		///
		/// A directory that cannot answer — empty, unreadable, or holding nothing with a
		/// letter in its name — falls back to the host default, which is where this
		/// started.
		/// </remarks>
		public bool IsCaseSensitive(string path)
		{
			string directory = NearestExistingDirectory(path);
			if (directory == null)
				return HostIsCaseSensitive;

			if (_caseSensitivity.TryGetValue(directory, out bool cached))
				return cached;

			bool sensitive = Probe(directory) ?? HostIsCaseSensitive;
			_caseSensitivity[directory] = sensitive;
			return sensitive;
		}

		private readonly Dictionary<string, bool> _caseSensitivity = new Dictionary<string, bool>(StringComparer.Ordinal);

		private static bool HostIsCaseSensitive =>
			!RuntimeInformation.IsOSPlatform(OSPlatform.Windows) && !RuntimeInformation.IsOSPlatform(OSPlatform.OSX);

		/// <summary>The first ancestor of <paramref name="path"/> that exists, or null.</summary>
		/// <remarks>
		/// The output directory usually does not exist yet when the plan is checked —
		/// creating it is part of the run — so the question is answered about the nearest
		/// ancestor that does, which is on the same volume.
		/// </remarks>
		private static string NearestExistingDirectory(string path)
		{
			string candidate = Path.GetDirectoryName(Path.GetFullPath(path));
			while (!string.IsNullOrEmpty(candidate))
			{
				if (Directory.Exists(candidate))
					return candidate;

				string parent = Path.GetDirectoryName(candidate);
				if (parent == candidate)
					return null;

				candidate = parent;
			}

			return null;
		}

		/// <summary>True, false, or null when the directory cannot be asked.</summary>
		/// <remarks>
		/// A directory that cannot be listed at all throws from the enumeration rather
		/// than returning nothing, and is the one case this answers for itself: no
		/// listing, no measurement, host default.
		/// </remarks>
		private static bool? Probe(string directory)
		{
			try
			{
				return CaseProbe.Measure(
					directory,
					Directory.EnumerateFileSystemEntries(directory),
					candidate => File.Exists(candidate) || Directory.Exists(candidate));
			}
			catch (IOException)
			{
				return null;
			}
			catch (UnauthorizedAccessException)
			{
				return null;
			}
		}

		public void MoveFile(string from, string to)
		{
			File.Move(from, to, overwrite: true);
		}

		public Stream OpenRead(string path)
		{
			return File.OpenRead(path);
		}

		public Stream CreateFile(string path)
		{
			string directory = Path.GetDirectoryName(path);
			if (!string.IsNullOrEmpty(directory))
                CreateDirectory(directory);

			return File.Create(path);
		}

		public void CreateDirectory(string path)
		{
			Directory.CreateDirectory(path);
		}

		public IReadOnlyList<string> ReadAllLines(string path)
		{
			return File.ReadAllLines(path);
		}

		private static string Normalise(string path)
		{
			return path.Replace('\\', '/');
		}
	}
}
