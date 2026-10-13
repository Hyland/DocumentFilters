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

using System.Collections.Generic;
using System.IO;
using System.Text;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Tests.Fakes
{
	/// <summary>
	/// Captures everything the tool would have printed, so that output can be
	/// asserted on without a terminal.
	/// </summary>
	/// <remarks>
	/// Standard output and standard error are kept apart deliberately: error text is
	/// part of the cross-language contract and is asserted precisely, while progress
	/// chatter is not.
	/// </remarks>
	public sealed class RecordingConsole : IConsole
	{
		private readonly StringBuilder _out = new StringBuilder();
		private readonly MemoryStream _binary = new MemoryStream();

		/// <summary>Every line written to standard output, in order.</summary>
		public List<string> OutLines { get; } = new List<string>();

		/// <summary>Every line written to standard error, in order. Part of the contract.</summary>
		public List<string> ErrorLines { get; } = new List<string>();

		/// <summary>Every verbose line. Not part of the contract.</summary>
		public List<string> VerboseLines { get; } = new List<string>();

		/// <summary>Set this to exercise the <c>--password-prompt</c> paths.</summary>
		public bool IsInputInteractive { get; set; }

		/// <summary>Standard input, for <c>--files-from -</c>.</summary>
		public List<string> InputLines { get; } = new List<string>();

		/// <summary>Answers handed to successive <see cref="ReadPassword"/> calls, in order.</summary>
		public Queue<string> PasswordResponses { get; } = new Queue<string>();

		/// <summary>The prompts <see cref="ReadPassword"/> was called with.</summary>
		public List<string> PasswordPrompts { get; } = new List<string>();

		/// <summary>How many times <see cref="WaitForKey"/> was called, for <c>--pause</c>.</summary>
		public int KeyWaits { get; private set; }

        public IReadOnlyList<string> ReadAllInputLines() => InputLines;

		public string ReadPassword(string prompt)
		{
			PasswordPrompts.Add(prompt);
			return PasswordResponses.Count > 0 ? PasswordResponses.Dequeue() : null;
		}

		public void WaitForKey() => KeyWaits++;

		/// <summary>Everything written to the binary stdout stream.</summary>
		public byte[] BinaryOutput => _binary.ToArray();

		/// <summary>Standard output as one string.</summary>
		public string Output => _out.ToString();

		public void Write(string text) => _out.Append(text);

		public void WriteLine(string text)
		{
			_out.Append(text).Append('\n');
			OutLines.Add(text);
		}

		public void WriteError(string text) => ErrorLines.Add(text);

		public void WriteVerbose(string text) => VerboseLines.Add(text);

		public Stream OpenStandardOutput() => _binary;
	}
}
