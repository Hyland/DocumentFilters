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

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// Supplies passwords to the engine as it encounters protected documents.
	/// </summary>
	/// <remarks>
	/// Candidates come from <c>-p</c>, then <c>--password-file</c>, in order. When
	/// none of them work and <c>--password-prompt</c> was given, the user is asked.
	///
	/// <b>Open, and deliberately so:</b> what <c>--password-prompt</c> should do when
	/// stdin is not a terminal — fail with a usage error, or fall through as if it
	/// were not given. §09 of the design document carries it as one of the calls to
	/// be made while building rather than settled in advance, so the four ports must
	/// agree on an answer before any of them relies on one.
	/// </remarks>
	public sealed class PasswordProvider
	{
		private readonly PasswordOptions _options;
		private readonly IConsole _console;
		private readonly IFileSystem _fileSystem;

		private List<string> _fileCandidates;
		private int _nextIndex;
		private bool _promptExhausted;

		public PasswordProvider(PasswordOptions options, IConsole console, IFileSystem fileSystem)
		{
			_options = options;
			_console = console;
			_fileSystem = fileSystem;
		}

		/// <summary>
		/// Returns the next password to try for the given document, or null when the
		/// candidates are exhausted.
		/// </summary>
		/// <remarks>
		/// <c>-p</c> candidates are tried in the order given, then every line of
		/// <c>--password-file</c>, and finally — once those are exhausted and
		/// <c>--password-prompt</c> was given — one interactive prompt. The prompt is
		/// asked at most once per document: an engine that rejects it moves on to the
		/// next document rather than asking again forever.
		/// </remarks>
		public string GetPassword(string documentId)
		{
			if (_fileCandidates == null)
				LoadPasswordFile();

			List<string> candidates = _options.Passwords;
			if (_nextIndex < candidates.Count)
				return candidates[_nextIndex++];

			int fileIndex = _nextIndex - candidates.Count;
			if (fileIndex < _fileCandidates.Count)
			{
				_nextIndex++;
				return _fileCandidates[fileIndex];
			}

			if (_options.Prompt && !_promptExhausted && _console.IsInputInteractive)
			{
				_promptExhausted = true;
				return _console.ReadPassword($"Password for {documentId}: ");
			}

			return null;
		}

		/// <summary>Forgets which candidates have been tried, when moving to a new document.</summary>
		public void Reset()
		{
			_nextIndex = 0;
			_promptExhausted = false;
		}

		/// <summary>Reads <c>--password-file</c>, one candidate per line.</summary>
		internal void LoadPasswordFile()
		{
			_fileCandidates = string.IsNullOrEmpty(_options.PasswordFile)
				? new List<string>()
				: _fileSystem.ReadAllLines(_options.PasswordFile).Where(line => line.Length > 0).ToList();
		}
	}
}
