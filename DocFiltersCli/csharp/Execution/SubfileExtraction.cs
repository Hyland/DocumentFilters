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
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;

using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// Walking into documents that contain other documents: archives, mail stores,
	/// attachments, embedded images.
	/// </summary>
	/// <remarks>
	/// The depth limit here is <c>--subfile-depth</c>, which is entirely separate
	/// from the directory walk's <c>--max-depth</c>. A .docx inside a .zip inside an
	/// .msg is three subfile levels regardless of where the .msg sits on disk.
	/// </remarks>
	public sealed class SubfileExtraction
	{
		private readonly IFileSystem _fileSystem;

		/// <summary>
		/// Takes the filesystem because extraction creates the directories a subfile's
		/// name implies.
		/// </summary>
		/// <remarks>
		/// The same split as everywhere else a file is produced: the directory is ours
		/// to create, the file itself is written by <c>ISubFileEntry.CopyTo</c> inside
		/// the engine. Creating it through <see cref="IFileSystem"/> rather than
		/// <c>System.IO.Directory</c> is what keeps the unit suite off the real disk —
		/// a subfile whose name carries a path, or <c>--make-dirs</c>, would otherwise
		/// leave directories behind wherever the tests happened to run.
		/// </remarks>
		public SubfileExtraction(IFileSystem fileSystem)
		{
			_fileSystem = fileSystem;
		}

		/// <summary>
		/// Every output path this extractor has already resolved, for the whole run.
		/// </summary>
		/// <remarks>
		/// Extraction resolves its own paths — the planner never sees them, because what
		/// a container holds is not known until it is open — so the output policy has to
		/// be applied here or not at all. It was not: an existing file was overwritten
		/// without <c>--overwrite</c>, and two subfiles whose names sanitise to one path
		/// overwrote each other, both silently and both exit 0.
		///
		/// On the extractor rather than on the call, because <c>extract</c> builds one of
		/// these and runs every input through it. Per call, two archives each holding a
		/// <c>readme.txt</c> flattened into one output directory would not collide at all
		/// — the set would have been emptied in between — and under <c>--overwrite</c>
		/// the second would silently replace the first. That is what the planner's own
		/// collision check refuses for ordinary outputs, with or without
		/// <c>--overwrite</c>, so extraction refuses it too.
		/// </remarks>
		private readonly HashSet<string> _claimed = new HashSet<string>(StringComparer.Ordinal);

		/// <summary>Extracts every subfile to disk, for the <c>extract</c> command.</summary>
		/// <remarks>
		/// <b>Every written path must be inside the output directory.</b> A subfile's
		/// name comes from the container, not from the user, so an archive can carry a
		/// name like <c>../../outside.txt</c> or an absolute path and walk out of the
		/// destination. Names are put through
		/// <see cref="ResolveSafeOutputPath"/> before anything is created; no path
		/// reaches the filesystem any other way.
		/// </remarks>
		public IReadOnlyList<string> ExtractAll(IDocument container, string containerPath, SharedOptions shared, ExtractOptions options)
		{
			var written = new List<string>();

			string outputRoot = shared.Output.OutDir ?? ".";

			void ExtractOne(ISubFileEntry subfile, string displayName, int depth)
			{
				if (!PassesSubfileFilters(subfile, options))
					return;

				string outputPath = ResolveSafeOutputPath(outputRoot, subfile.Name, options.ReplicateTree);
				if (options.AppendSubfileId)
					outputPath = InsertSubfileId(outputPath, subfile.Id);

				if (!_claimed.Add(OutputPlanner.CollisionKeyFor(outputPath, _fileSystem)))
				{
					throw new OutputConflictError(ErrorCodes.OutputCollision,
						"two subfiles would be extracted to the same output; use --sub-id to distinguish them");
				}

				OutputPlanner.VerifyPageTarget(outputPath, shared, _fileSystem);
				VerifyNoLinkedComponent(outputRoot, outputPath);

				string directory = Path.GetDirectoryName(outputPath);
				if (!string.IsNullOrEmpty(directory))
					_fileSystem.CreateDirectory(directory);

				subfile.CopyTo(outputPath);
				written.Add(outputPath);
			}

			Walk(container, containerPath, 1, shared, ExtractOne);

			if (options.Images)
			{
				foreach (ISubFileEntry image in container.Images)
				{
					using (image)
						ExtractOne(image, ComposeDisplayName(containerPath, image.Name), 1);
				}
			}

			return written;
		}

		/// <summary>
		/// Refuses an output path that reaches its destination through a symbolic link.
		/// </summary>
		/// <remarks>
		/// <see cref="ResolveSafeOutputPath"/> guarantees the path spells out as a plain
		/// name under the root, which is a statement about the string. Where it lands is
		/// a statement about the disk: if <c>out/latest</c> is a link to a web root, a
		/// container entry named <c>latest/index.html</c> is lexically contained and
		/// still overwrites a file outside the output directory entirely. The container
		/// chooses that name, so the choice is not the user's.
		///
		/// Only components below the root are examined. The root itself may well be a
		/// link — <c>--out-dir</c> is the user's own instruction, and refusing it would
		/// break ordinary setups — and so may anything above it.
		///
		/// This closes the hole rather than sealing it: a link created between this
		/// check and <c>CopyTo</c> would still be followed. Winning that race needs write
		/// access to the output directory while the run is in progress, at which point
		/// the file can simply be replaced afterwards; keeping the link open across the
		/// write is what a tool defending against a hostile filesystem would do, and that
		/// is not what this sample is.
		/// </remarks>
		private void VerifyNoLinkedComponent(string outputRoot, string outputPath)
		{
			string root = outputRoot.Replace('\\', '/').TrimEnd('/');
			string path = outputPath.Replace('\\', '/');

			if (!path.StartsWith(root + "/", StringComparison.Ordinal))
				return;

			// The file itself is a component like any other: a link named report.txt is
			// written through just as a link named reports/ is.
			int cursor = root.Length;
			while (cursor < path.Length)
			{
				int slash = path.IndexOf('/', cursor + 1);
				string component = slash < 0 ? path : path.Substring(0, slash);

				bool present = _fileSystem.DirectoryExists(component) || _fileSystem.FileExists(component);
				if (present && _fileSystem.IsSymbolicLink(component))
				{
					throw new OutputConflictError(ErrorCodes.UnsafeOutput,
						"a subfile would be written through '" + component + "', which is a link out of the output directory");
				}

				if (slash < 0)
					break;

				cursor = slash;
			}
		}

		/// <summary>Inserts a subfile's id before its extension, for <c>--sub-id</c>.</summary>
		private static string InsertSubfileId(string path, string id)
		{
			int slash = path.LastIndexOf('/');
			string directory = slash < 0 ? string.Empty : path.Substring(0, slash + 1);
			string fileName = slash < 0 ? path : path.Substring(slash + 1);

			int dot = fileName.LastIndexOf('.');
			string stem = dot < 0 ? fileName : fileName.Substring(0, dot);
			string extension = dot < 0 ? string.Empty : fileName.Substring(dot);

			return directory + stem + "-" + id + extension;
		}

		/// <summary>
		/// Turns a container-supplied subfile name into a path that is guaranteed to
		/// sit inside <paramref name="outputRoot"/>.
		/// </summary>
		/// <remarks>
		/// The name is untrusted input. Traversal segments, absolute paths and drive
		/// letters are all neutralised; when <paramref name="replicateTree"/> is false
		/// the name collapses to its final component.
		///
		/// This is the lexical half of the guarantee and all of it that is knowable
		/// without touching the disk: every segment it returns is a plain name under the
		/// root. The other half is <see cref="VerifyNoLinkedComponent"/>, because a name
		/// can be lexically contained and still land outside — a symlink already sitting
		/// in the output directory resolves wherever it points.
		///
		/// The published <c>ExtractSubfiles</c> sample does the same thing; a tool that
		/// extracts archives has to.
		/// </remarks>
		public static string ResolveSafeOutputPath(string outputRoot, string subfileName, bool replicateTree)
		{
			List<string> segments = SafeSegments(subfileName, replicateTree);
			return outputRoot.TrimEnd('/') + "/" + string.Join("/", segments);
		}

		/// <summary>
		/// Splits a subfile name into path components that are guaranteed not to
		/// traverse upward or escape via a drive letter or UNC share, never leaving the
		/// result empty.
		/// </summary>
		/// <remarks>
		/// Every <c>.</c> and <c>..</c> component is dropped outright, not merely
		/// neutralised, so a name that is nothing but traversal (<c>"../.."</c>) cannot
		/// leave the result empty either — that case falls back to a single placeholder
		/// segment. Removing rather than clamping is also what tells "..data", a
		/// legitimate filename, apart from an actual traversal component: only an exact
		/// match is dropped, never a prefix or substring.
		/// </remarks>
		private static List<string> SafeSegments(string subfileName, bool replicateTree)
		{
			string normalised = (subfileName ?? string.Empty).Replace('\\', '/');

			// A drive letter ("C:...") names something outside any output directory;
			// dropping just the colon is enough, because the leading "C" and the rest
			// of the path pass through the same segment filter as everything else.
			if (normalised.Length >= 2 && normalised[1] == ':')
				normalised = normalised.Substring(2);

			var segments = new List<string>();
			foreach (string segment in normalised.Split('/'))
			{
				if (segment.Length == 0 || segment == "." || segment == "..")
					continue;

				segments.Add(segment);
			}

			if (!replicateTree)
			{
				string last = segments.Count > 0 ? segments[^1] : null;
				segments.Clear();
				if (last != null)
					segments.Add(last);
			}

			if (segments.Count == 0)
				segments.Add("_");

			return segments;
		}

		/// <summary>Lists subfiles without extracting, for the <c>list</c> command.</summary>
		public IReadOnlyList<SubfileEntry> ListAll(IDocument container, string containerPath, SharedOptions shared)
		{
			var entries = new List<SubfileEntry>();
			Walk(container, containerPath, 1, shared, (subfile, displayName, depth) =>
			{
				entries.Add(new SubfileEntry
				{
					Id = subfile.Id,
					Name = subfile.Name,
					DisplayName = displayName,
					Size = subfile.Size,
					Modified = FormatTimestamp(subfile.Modified),
					Depth = depth
				});
			});
			return entries;
		}

		/// <summary>
		/// Invokes <paramref name="action"/> for every subfile, depth-first, honouring
		/// <c>--subfile-depth</c>. Shared by <c>convert</c>, <c>extract</c>,
		/// <c>list</c>, <c>identify</c> and <c>meta</c> when <c>-s</c> is given.
		/// </summary>
		/// <remarks>
		/// <paramref name="container"/> must already be open — recursion opens each
		/// subfile itself, the same way the published <c>ExtractSubfiles</c> sample
		/// does, since a <see cref="ISubFileEntry"/> is an <see cref="IDocument"/> and its own
		/// <c>SubFiles</c> are unavailable until it is.
		/// </remarks>
		public void Walk(IDocument container, string containerPath, int depth, SharedOptions shared, Action<ISubFileEntry, string, int> action)
		{
			foreach (ISubFileEntry subfile in container.SubFiles)
			{
				using (subfile)
				{
					string displayName = ComposeDisplayName(containerPath, subfile.Name);
					action(subfile, displayName, depth);

					bool canRecurse = shared.Subfiles.Enabled
						&& (!shared.Subfiles.MaxDepth.HasValue || depth < shared.Subfiles.MaxDepth.Value);

					if (canRecurse)
					{
						// Opening the entry yields a document sharing its handle, so the
						// entry's own using-block is what closes it.
						IDocument opened = subfile.Open((int)OpenType.BodyOnly, string.Empty);
						Walk(opened, displayName, depth + 1, shared, action);
					}
				}
			}
		}

		/// <summary>
		/// Renders a subfile timestamp in the ISO 8601 form the console contract uses,
		/// or null when the container records none.
		/// </summary>
		/// <remarks>
		/// Kept as a pure function of the nullable value so the boundary — a container
		/// with no timestamp versus one dated at the epoch — is testable without an
		/// archive.
		/// </remarks>
		public static string FormatTimestamp(DateTime? modified)
		{
			if (!modified.HasValue || modified.Value.Year <= 1970)
				return null;

			return modified.Value.ToString("yyyy-MM-ddTHH:mm:ss") + "Z";
		}

		/// <summary>Builds the display name for a subfile: <c>container.zip&gt;inner.docx</c>.</summary>
		internal static string ComposeDisplayName(string containerPath, string subfileName)
		{
			return containerPath + ">" + subfileName;
		}

		/// <summary>
		/// Applies <c>extract</c>'s <c>--include</c> and <c>--exclude</c> globs to a
		/// subfile's name.
		/// </summary>
		/// <remarks>
		/// Both are already-compiled <see cref="Planning.NameMatcher"/> instances; a malformed
		/// pattern fails when the argument is read, not here with an archive
		/// half-extracted.
		///
		/// Matching on the name lets a subfile be rejected without being opened, which
		/// matters for a large archive.
		/// </remarks>
		internal static bool PassesSubfileFilters(ISubFileEntry subfile, ExtractOptions options)
		{
			if (options.SubfileInclude != null && !options.SubfileInclude.Matches(subfile.Name))
				return false;

			if (options.SubfileExclude != null && options.SubfileExclude.Matches(subfile.Name))
				return false;

			return true;
		}
	}

	/// <summary>One subfile, as reported by <c>list</c>.</summary>
	public sealed class SubfileEntry
	{
		public string Id { get; set; }

		public string Name { get; set; }

		/// <summary>Full display path including containers.</summary>
		public string DisplayName { get; set; }

		public long Size { get; set; }

		/// <summary>Modification time in ISO 8601, or null when the container does not record one.</summary>
		public string Modified { get; set; }

		/// <summary>Nesting level; 1 for a direct child of the document named on the command line.</summary>
		public int Depth { get; set; }
	}
}
