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

using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Hyland.DocumentFilters.Cli.Runtime
{
	/// <summary>
	/// <c>--view</c>: hand the finished output to whatever the desktop opens it with.
	/// </summary>
	/// <remarks>
	/// Best effort, and deliberately so — A7 settled that, and it is why <c>--view</c>
	/// is excluded from the conformance corpus: what opens is the machine's business,
	/// there is no output of our own to compare, and a headless box has nothing to
	/// open it with at all. A failure to launch is reported and ignored rather than
	/// failing the run, because the conversion itself already succeeded.
	///
	/// Three platforms, three commands, the same three doc2text uses: <c>ShellExecute</c>
	/// on Windows, <c>open</c> on macOS, <c>xdg-open</c> elsewhere. Windows goes through
	/// the shell (<c>UseShellExecute</c>), which is what makes a file association apply
	/// rather than trying to execute the document.
	/// </remarks>
	public static class Viewer
	{
		/// <summary>Opens <paramref name="path"/>, reporting but not failing on trouble.</summary>
		public static void Open(string path, IConsole console)
		{
			if (string.IsNullOrEmpty(path))
				return;

			try
			{
				Start(path);
			}
			catch (Exception error)
			{
				// Not a DocumentError: nothing about the document failed, and the output
				// is on disk either way.
				console.WriteError($"--view could not open {path}: {error.Message}");
			}
		}

		/// <remarks>
		/// The path goes in <see cref="ProcessStartInfo.ArgumentList"/>, never into an
		/// argument <em>string</em>. A string is re-parsed into arguments by rules this
		/// code does not control, so hand-quoting it was wrong for any filename
		/// containing a quote: <c>my"file.pdf</c> split into two arguments and the
		/// viewer opened neither. <c>ArgumentList</c> passes each entry through as one
		/// exact argv slot, with no quoting to get right.
		///
		/// Output names come from input names, so they are as arbitrary as whatever the
		/// run walked over — a filename is not something this tool chooses.
		/// </remarks>
		private static void Start(string path)
		{
			if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
			{
				// The only case that needs the shell: UseShellExecute is what makes the
				// file association apply. It takes the path as the verb's target rather
				// than as a command line, so there is nothing to quote here either.
				Process.Start(new ProcessStartInfo(path) { UseShellExecute = true });
				return;
			}

			string launcher = RuntimeInformation.IsOSPlatform(OSPlatform.OSX) ? "open" : "xdg-open";
			var start = new ProcessStartInfo(launcher) { UseShellExecute = false };
			start.ArgumentList.Add(path);

			Process.Start(start);
		}
	}
}
