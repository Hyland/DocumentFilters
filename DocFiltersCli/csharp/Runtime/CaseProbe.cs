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
	/// Chooses the spelling to look for when asking a directory whether it tells
	/// names apart by case.
	/// </summary>
	/// <remarks>
	/// Separate from <see cref="SystemFileSystem"/> and pure, because the one thing
	/// that has to be right here is <em>where</em> the question is asked, and a real
	/// filesystem cannot show that in a test: on a single volume every answer agrees,
	/// so a probe aimed one directory too high passes every test a machine can run.
	/// What a test can check is that the name handed back lies inside the directory
	/// being measured — which is the whole of the rule.
	///
	/// Why it matters: the lookup rules belong to the directory, not to the host and
	/// not to the volume above it. <c>/mnt/ntfs</c> is case-insensitive while the
	/// <c>/mnt</c> holding it is not, and Windows can mark a single directory
	/// case-sensitive inside an otherwise insensitive volume. Measuring the parent
	/// gets both of those backwards.
	/// </remarks>
	public static class CaseProbe
	{
		/// <summary>
		/// Whether <paramref name="directory"/> tells names apart by case, or
		/// <c>null</c> when it has nothing to answer with.
		/// </summary>
		/// <param name="directory">The directory whose rules are in question.</param>
		/// <param name="entries">
		/// What is already in it. Enumerated lazily and only as far as the first name
		/// with a letter in it, so a directory of a hundred thousand files costs one
		/// entry rather than a hundred thousand.
		/// </param>
		/// <param name="exists">
		/// Whether a path is there, file or directory. Anything it cannot answer reads
		/// as absent, which is what <see cref="File.Exists"/> does with an unreadable
		/// path and therefore what every port does.
		/// </param>
		/// <returns>
		/// <c>null</c> when the directory is empty or holds nothing with a letter in
		/// its name — the caller falls back to the host default.
		/// </returns>
		public static bool? Measure(string directory, IEnumerable<string> entries, Func<string, bool> exists)
		{
			foreach (string entry in entries)
			{
				string sibling = FlippedSibling(directory, entry);
				if (sibling == null)
					continue;

				return !exists(sibling);
			}

			return null;
		}

		/// <summary>
		/// The path to look for inside <paramref name="directory"/> to measure how it
		/// matches names, or <c>null</c> when <paramref name="entry"/> cannot answer.
		/// </summary>
		/// <param name="directory">The directory whose rules are in question.</param>
		/// <param name="entry">
		/// Something already in it, named or full-path. Its name is flipped letter by
		/// letter, so <c>Report.txt</c> is looked for as <c>rEPORT.TXT</c>: if that is
		/// found, the directory matched two spellings to one file and is
		/// case-insensitive.
		/// </param>
		/// <returns>
		/// A sibling path under <paramref name="directory"/>, or <c>null</c> when the
		/// name has no letter to flip and so tells the caller nothing.
		/// </returns>
		public static string FlippedSibling(string directory, string entry)
		{
			string name = NameOf(entry);
			string flipped = FlipCase(name);
			if (flipped == name)
				return null;

			string parent = directory.TrimEnd('/', '\\');
			return parent.Length == 0 ? flipped : parent + "/" + flipped;
		}

		private static string NameOf(string path)
		{
			string trimmed = path.TrimEnd('/', '\\');
			int slash = trimmed.LastIndexOfAny(new[] { '/', '\\' });
			return slash < 0 ? trimmed : trimmed.Substring(slash + 1);
		}

		/// <summary>
		/// ASCII-only, and every letter rather than a lowercasing, for the same reason
		/// <c>ValueParsers.FoldToken</c> folds ASCII only: a locale-aware fold turns
		/// 'I' into a dotless 'ı' under a Turkish locale and would probe for a name
		/// that exists under neither rule.
		/// </summary>
		private static string FlipCase(string text)
		{
			char[] chars = text.ToCharArray();
			for (int i = 0; i < chars.Length; i++)
			{
				if (chars[i] >= 'a' && chars[i] <= 'z')
					chars[i] = (char)(chars[i] - 'a' + 'A');
				else if (chars[i] >= 'A' && chars[i] <= 'Z')
					chars[i] = (char)(chars[i] - 'A' + 'a');
			}

			return new string(chars);
		}
	}
}
