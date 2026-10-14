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

namespace Hyland.DocumentFilters.Cli.Runtime
{
	/// <summary>
	/// The filesystem operations this tool needs, behind one seam so the directory
	/// walk and the output planner can be unit tested entirely in memory.
	/// </summary>
	public interface IFileSystem
	{
		bool FileExists(string path);

		bool DirectoryExists(string path);

		/// <summary>Immediate children of a directory, files and directories alike, unsorted.</summary>
		IReadOnlyList<string> ListDirectory(string path);

		/// <summary>True for entries the walk skips unless <c>--include-hidden</c> is given.</summary>
		bool IsHidden(string path);

		/// <summary>True for symlinked directories, which the walk enters only under <c>-L/--follow</c>.</summary>
		bool IsSymbolicLink(string path);

		/// <summary>
		/// A stable identity for a directory, with links resolved — two paths that
		/// reach the same directory return the same value.
		/// </summary>
		/// <remarks>
		/// Needed to make <c>-L/--follow</c> terminate. Knowing that an entry is a link
		/// is not enough: a link pointing at one of its own ancestors produces an
		/// endless supply of distinct path strings, so a set of visited <em>paths</em>
		/// never detects the cycle. A set of visited identities does.
		/// </remarks>
		string GetDirectoryIdentity(string path);

		/// <summary>
		/// Whether the filesystem holding <paramref name="path"/> tells two names apart
		/// by case — true where <c>Report.txt</c> and <c>report.txt</c> are two files.
		/// </summary>
		/// <remarks>
		/// Asked of the filesystem rather than derived from the host, for the same
		/// reason <see cref="GetDirectoryIdentity"/> is: the planner cannot work it out
		/// and guessing is wrong often enough to matter. An <c>--out-dir</c> may sit on
		/// a mounted NTFS or exFAT volume under Linux, or on a case-sensitive volume
		/// under macOS, and the answer decides whether two planned outputs are one file.
		///
		/// <paramref name="path"/> need not exist: the question is about the volume it
		/// would live on, which is why the collision check can ask before anything is
		/// written and <c>--dry-run</c> can ask without touching the disk at all.
		/// </remarks>
		bool IsCaseSensitive(string path);

		Stream OpenRead(string path);

		/// <summary>Creates or truncates a file, creating parent directories as needed.</summary>
		Stream CreateFile(string path);

		void CreateDirectory(string path);

		/// <summary>
		/// Renames <paramref name="from"/> to <paramref name="to"/>, replacing an
		/// existing file at the destination.
		/// </summary>
		/// <remarks>
		/// For <c>web-safe</c>, which is the one format whose extension is not knowable
		/// until the page has been rendered: the engine picks PNG, GIF, JPEG or WebP per
		/// page, and the file is named for what it turned out to be. Whether replacing is
		/// allowed at all is settled before this is called, by the same output policy
		/// every other target goes through — so by the time it runs, the destination is
		/// one the run is entitled to write.
		/// </remarks>
		void MoveFile(string from, string to);

		/// <summary>Reads a <c>--files-from</c> list, one path per line.</summary>
		IReadOnlyList<string> ReadAllLines(string path);
	}
}
