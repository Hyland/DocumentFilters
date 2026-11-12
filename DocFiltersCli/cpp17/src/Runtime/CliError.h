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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_CLIERROR_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_CLIERROR_H

#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// Stable machine-readable identifiers for every error this tool can
				/// report.
				/// </summary>
				/// <remarks>
				/// These strings appear in the conformance corpus, so all four language
				/// implementations must raise the same code for the same situation. The
				/// human-readable message is also part of the contract; it is authored
				/// here rather than left to the CLI parsing library, so that it can be
				/// identical everywhere.
				/// </remarks>
				namespace ErrorCodes
				{
					constexpr const char* unknownCommand = "E_UNKNOWN_COMMAND";
					constexpr const char* unknownArgument = "E_UNKNOWN_ARGUMENT";
					constexpr const char* missingValue = "E_MISSING_VALUE";
					constexpr const char* missingArgument = "E_MISSING_ARGUMENT";
					constexpr const char* badValue = "E_BAD_VALUE";
					constexpr const char* badPageRange = "E_BAD_PAGE_RANGE";
					constexpr const char* badTemplate = "E_BAD_TEMPLATE";
					constexpr const char* unknownFormat = "E_UNKNOWN_FORMAT";
					constexpr const char* mutuallyExclusive = "E_MUTUALLY_EXCLUSIVE";
					constexpr const char* combineUnsupported = "E_COMBINE_UNSUPPORTED";
					constexpr const char* stdoutNeedsSingleFile = "E_STDOUT_NEEDS_SINGLE_FILE";
					constexpr const char* outFileNeedsSingleFile = "E_OUTFILE_NEEDS_SINGLE_FILE";
					constexpr const char* outNameNeedsOutDir = "E_OUTNAME_NEEDS_OUTDIR";
					constexpr const char* templateNeedsPage = "E_TEMPLATE_NEEDS_PAGE";
					constexpr const char* reportNeedsHiDef = "E_REPORT_NEEDS_HIDEF";
					constexpr const char* subfileDepthNeedsSubfiles = "E_SUBFILE_DEPTH_NEEDS_SUBFILES";
					constexpr const char* outFileMultiInput = "E_OUTFILE_MULTI_INPUT";
					constexpr const char* outputExists = "E_OUTPUT_EXISTS";
					constexpr const char* outputCollision = "E_OUTPUT_COLLISION";
					constexpr const char* unsafeOutput = "E_UNSAFE_OUTPUT";
					constexpr const char* outputIsInput = "E_OUTPUT_IS_INPUT";
					constexpr const char* noInputs = "E_NO_INPUTS";
					constexpr const char* initFailed = "E_INIT_FAILED";
					constexpr const char* limitExceeded = "E_LIMIT_EXCEEDED";
					constexpr const char* documentFailed = "E_DOCUMENT_FAILED";
					constexpr const char* interrupted = "E_INTERRUPTED";

					/// <summary>
					/// Every code above, so that the tool's error vocabulary can be
					/// enumerated without reflection.
					/// </summary>
					/// <remarks>
					/// A hand-written list beside the declarations, in the same idiom as
					/// <c>ArgumentTable::commands</c>, and for the same reason: C++17 has no
					/// reflection, and the Python, Java and C++ ports translate these
					/// declarations rather than rewriting them.
					/// </remarks>
					const std::vector<std::string>& all();
				}

				/// <summary>
				/// Base class for every error the CLI reports to the user. Thrown from
				/// stages 2 to 4 and caught once, in <c>main</c>.
				/// </summary>
				class CliError : public std::runtime_error
				{
				public:
					CliError(std::string code, const std::string& message, std::optional<std::string> path = std::nullopt)
						: std::runtime_error(message), _code(std::move(code)), _path(std::move(path))
					{
					}

					/// <summary>Stable identifier from <see cref="ErrorCodes"/>.</summary>
					const std::string& code() const { return _code; }

					/// <summary>
					/// The input this failure belongs to, or nullopt when it belongs to the
					/// run.
					/// </summary>
					/// <remarks>
					/// What decides which of the two stderr shapes the failure gets:
					/// <c>path: CODE: message</c> when an input is named, <c>CODE: message</c>
					/// when none is. On the base rather than on <see cref="DocumentError"/>
					/// alone because a per-document failure is not always a document
					/// failure --- <c>--timeout</c> applies to one input but exits 6, not 1,
					/// so it could never have been a <see cref="DocumentError"/> and would
					/// otherwise have had to fold the filename into its own message to say
					/// which input it meant.
					/// </remarks>
					const std::optional<std::string>& path() const { return _path; }

					/// <summary>The process exit code this error produces.</summary>
					virtual int exitCode() const = 0;

				private:
					std::string _code;
					std::optional<std::string> _path;
				};

				/// <summary>Bad command line: unknown argument or verb, missing or malformed value.</summary>
				class UsageError : public CliError
				{
				public:
					UsageError(const std::string& code, const std::string& message) : CliError(code, message) {}

					int exitCode() const override;
				};

				/// <summary>The walk plus filters selected nothing to process.</summary>
				class NoInputsError : public CliError
				{
				public:
					explicit NoInputsError(const std::string& message) : CliError(ErrorCodes::noInputs, message) {}

					int exitCode() const override;
				};

				/// <summary>An output target cannot be resolved without guessing, or already exists.</summary>
				class OutputConflictError : public CliError
				{
				public:
					OutputConflictError(const std::string& code, const std::string& message) : CliError(code, message) {}

					int exitCode() const override;
				};

				/// <summary>Document Filters could not be initialised.</summary>
				class InitialisationError : public CliError
				{
				public:
					explicit InitialisationError(const std::string& message) : CliError(ErrorCodes::initFailed, message) {}

					int exitCode() const override;
				};

				/// <summary>A run limit was exceeded.</summary>
				class LimitExceededError : public CliError
				{
				public:
					/// <param name="path">
					/// The input the limit applied to, for <c>--timeout</c>; nullopt for
					/// <c>--max-time</c> and <c>--max-memory</c>, which bound the whole run.
					/// </param>
					explicit LimitExceededError(const std::string& message, std::optional<std::string> path = std::nullopt)
						: CliError(ErrorCodes::limitExceeded, message, std::move(path))
					{
					}

					int exitCode() const override;
				};

				/// <summary>
				/// A single document failed. Collected by <c>BatchRunner</c> rather than
				/// ending the run, unless <c>--on-error stop</c> is in effect.
				/// </summary>
				class DocumentError : public CliError
				{
				public:
					explicit DocumentError(const std::string& path, const std::string& message, std::exception_ptr inner = nullptr)
						: CliError(ErrorCodes::documentFailed, message, path), _innerError(inner)
					{
					}

					/// <summary>The underlying exception, when the failure came from the engine.</summary>
					std::exception_ptr innerError() const { return _innerError; }

					int exitCode() const override;

				private:
					std::exception_ptr _innerError;
				};
			}
		}
	}
}

#endif
