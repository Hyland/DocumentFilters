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
	/// The process exit codes, identical across the C#, Python, Java and C++17
	/// implementations of this tool.
	/// </summary>
	/// <remarks>
	/// Only rule that matters to existing scripts: non-zero means something went
	/// wrong. The distinct codes let a caller tell "three files failed" apart from
	/// "you typed an argument wrong" apart from "your glob matched nothing".
	/// </remarks>
	public static class ExitCode
	{
		/// <summary>Every selected input was processed.</summary>
		public const int Success = 0;

		/// <summary>One or more files failed while <c>--on-error skip</c> was in effect.</summary>
		public const int PartialFailure = 1;

		/// <summary>Unknown argument or verb, missing required value, malformed value.</summary>
		public const int UsageError = 2;

		/// <summary>The filesystem walk plus filters selected nothing to do.</summary>
		public const int NoInputsMatched = 3;

		/// <summary>Output target conflict: multiple inputs into one file, or an existing file without <c>--overwrite</c>.</summary>
		public const int OutputConflict = 4;

		/// <summary>Document Filters could not be initialised: library not found, or license rejected.</summary>
		public const int InitialisationFailure = 5;

		/// <summary>A run limit was hit: <c>--max-time</c>, <c>--max-memory</c> or <c>--timeout</c>.</summary>
		public const int LimitExceeded = 6;

		/// <summary>The run was interrupted (Ctrl-C).</summary>
		public const int Interrupted = 130;
	}
}
