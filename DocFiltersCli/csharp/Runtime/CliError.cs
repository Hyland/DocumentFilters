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
	/// Stable machine-readable identifiers for every error this tool can report.
	/// </summary>
	/// <remarks>
	/// These strings appear in the conformance corpus, so all four language
	/// implementations must raise the same code for the same situation. The
	/// human-readable message is also part of the contract; it is authored here
	/// rather than left to the CLI parsing library, so that it can be identical
	/// everywhere.
	/// </remarks>
	public static class ErrorCodes
	{
		public const string UnknownCommand = "E_UNKNOWN_COMMAND";
		public const string UnknownArgument = "E_UNKNOWN_ARGUMENT";
		public const string MissingValue = "E_MISSING_VALUE";
		public const string MissingArgument = "E_MISSING_ARGUMENT";
		public const string BadValue = "E_BAD_VALUE";
		public const string BadPageRange = "E_BAD_PAGE_RANGE";
		public const string BadTemplate = "E_BAD_TEMPLATE";
		public const string UnknownFormat = "E_UNKNOWN_FORMAT";
		public const string MutuallyExclusive = "E_MUTUALLY_EXCLUSIVE";
		public const string CombineUnsupported = "E_COMBINE_UNSUPPORTED";
		public const string StdoutNeedsSingleFile = "E_STDOUT_NEEDS_SINGLE_FILE";
		public const string OutFileNeedsSingleFile = "E_OUTFILE_NEEDS_SINGLE_FILE";
		public const string OutNameNeedsOutDir = "E_OUTNAME_NEEDS_OUTDIR";
		public const string TemplateNeedsPage = "E_TEMPLATE_NEEDS_PAGE";
		public const string ReportNeedsHiDef = "E_REPORT_NEEDS_HIDEF";
		public const string OutFileMultiInput = "E_OUTFILE_MULTI_INPUT";
		public const string OutputExists = "E_OUTPUT_EXISTS";
		public const string OutputCollision = "E_OUTPUT_COLLISION";
		public const string UnsafeOutput = "E_UNSAFE_OUTPUT";
		public const string OutputIsInput = "E_OUTPUT_IS_INPUT";
		public const string NoInputs = "E_NO_INPUTS";
		public const string InitFailed = "E_INIT_FAILED";
		public const string LimitExceeded = "E_LIMIT_EXCEEDED";
		public const string DocumentFailed = "E_DOCUMENT_FAILED";
		public const string Interrupted = "E_INTERRUPTED";

		/// <summary>
		/// Every code above, so that the tool's error vocabulary can be enumerated
		/// without reflection.
		/// </summary>
		/// <remarks>
		/// A hand-written list beside the declarations, in the same idiom as
		/// <c>ArgumentTable.Commands</c>, and for the same reason: C++17 has no
		/// reflection, and the Python, Java and C++ ports translate these declarations
		/// rather than rewriting them.
		///
		/// <c>ErrorCodeTests</c> checks it against the fixtures in both directions, so a
		/// code declared here that no case provokes fails, and so does a case naming a
		/// code that is not here.
		/// </remarks>
		public static IReadOnlyList<string> All => new[]
		{
			UnknownCommand, UnknownArgument, MissingValue, MissingArgument, BadValue, BadPageRange,
			BadTemplate, UnknownFormat, MutuallyExclusive, CombineUnsupported,
			StdoutNeedsSingleFile, OutFileNeedsSingleFile, OutNameNeedsOutDir, TemplateNeedsPage, ReportNeedsHiDef,
			OutFileMultiInput, OutputExists, OutputCollision, UnsafeOutput, OutputIsInput, NoInputs, InitFailed,
			LimitExceeded, DocumentFailed, Interrupted
		};
	}

	/// <summary>
	/// Base class for every error the CLI reports to the user. Thrown from stages
	/// 2 to 4 and caught once, in <see cref="Program"/>.
	/// </summary>
	public abstract class CliError : Exception
	{
		protected CliError(string code, string message, string path = null)
			: base(message)
		{
			Code = code;
			Path = path;
		}

		/// <summary>Stable identifier from <see cref="ErrorCodes"/>.</summary>
		public string Code { get; }

		/// <summary>
		/// The input this failure belongs to, or null when it belongs to the run.
		/// </summary>
		/// <remarks>
		/// What decides which of the two stderr shapes the failure gets:
		/// <c>path: CODE: message</c> when an input is named, <c>CODE: message</c> when
		/// none is. On the base rather than on <see cref="DocumentError"/> alone because
		/// a per-document failure is not always a document failure — <c>--timeout</c>
		/// applies to one input but exits 6, not 1, so it could never have been a
		/// <see cref="DocumentError"/> and would otherwise have had to fold the filename
		/// into its own message to say which input it meant.
		/// </remarks>
		public string Path { get; }

		/// <summary>The process exit code this error produces.</summary>
		public abstract int ExitCode { get; }
	}

	/// <summary>Bad command line: unknown argument or verb, missing or malformed value.</summary>
	public sealed class UsageError : CliError
	{
		public UsageError(string code, string message) : base(code, message) { }

		public override int ExitCode => Runtime.ExitCode.UsageError;
	}

	/// <summary>The walk plus filters selected nothing to process.</summary>
	public sealed class NoInputsError : CliError
	{
		public NoInputsError(string message) : base(ErrorCodes.NoInputs, message) { }

		public override int ExitCode => Runtime.ExitCode.NoInputsMatched;
	}

	/// <summary>An output target cannot be resolved without guessing, or already exists.</summary>
	public sealed class OutputConflictError : CliError
	{
		public OutputConflictError(string code, string message) : base(code, message) { }

		public override int ExitCode => Runtime.ExitCode.OutputConflict;
	}

	/// <summary>Document Filters could not be initialised.</summary>
	public sealed class InitialisationError : CliError
	{
		public InitialisationError(string message) : base(ErrorCodes.InitFailed, message) { }

		public override int ExitCode => Runtime.ExitCode.InitialisationFailure;
	}

	/// <summary>A run limit was exceeded.</summary>
	public sealed class LimitExceededError : CliError
	{
		/// <param name="path">
		/// The input the limit applied to, for <c>--timeout</c>; null for
		/// <c>--max-time</c> and <c>--max-memory</c>, which bound the whole run.
		/// </param>
		public LimitExceededError(string message, string path = null)
			: base(ErrorCodes.LimitExceeded, message, path) { }

		public override int ExitCode => Runtime.ExitCode.LimitExceeded;
	}

	/// <summary>
	/// A single document failed. Collected by <see cref="BatchRunner"/> rather than
	/// ending the run, unless <c>--on-error stop</c> is in effect.
	/// </summary>
	public sealed class DocumentError : CliError
	{
		public DocumentError(string path, string message, Exception inner = null)
			: base(ErrorCodes.DocumentFailed, message, path)
		{
			InnerError = inner;
		}

		/// <summary>The underlying exception, when the failure came from the engine.</summary>
		public Exception InnerError { get; }

		public override int ExitCode => Runtime.ExitCode.PartialFailure;
	}
}
