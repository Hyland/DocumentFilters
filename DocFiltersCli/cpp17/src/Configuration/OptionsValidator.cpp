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

#include "OptionsValidator.h"

#include "Runtime/CliError.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Configuration
			{
				namespace OptionsValidator
				{
					namespace
					{
						namespace ErrorCodes = Runtime::ErrorCodes;
						using Runtime::UsageError;

						/// <summary>
						/// Builds the "cannot be used together" message every contradictory
						/// pair shares, naming each side by its long form --- with its
						/// current value attached when the caller passes one --- in
						/// argument-table order.
						/// </summary>
						[[noreturn]] void throwContradiction(const std::string& first, const std::string& second)
						{
							throw UsageError(ErrorCodes::mutuallyExclusive, first + " and " + second + " cannot be used together");
						}

						/// <summary><c>--out-dir ""</c> names no directory, and is refused rather than read as one.</summary>
						/// <remarks>
						/// Every output path is the directory, a slash and a name, so an empty
						/// directory does not mean "here": it turns <c>a.pdf</c> into <c>/a.pdf</c>,
						/// and an archive's <c>tmp/file</c> into <c>/tmp/file</c> --- a path relative
						/// to the root of the drive, chosen by the container. The current directory
						/// is <c>.</c>.
						/// </remarks>
						void checkOutDirNamesADirectory(const SharedOptions& shared)
						{
							if (shared.output.outDir.has_value() && shared.output.outDir->empty())
								throw UsageError(ErrorCodes::badValue, "--out-dir does not accept the value ''");
						}

						/// <summary><c>--out-file</c> and <c>--out-dir</c> are mutually exclusive.</summary>
						void checkOutputTargetsExclusive(const SharedOptions& shared)
						{
							if (shared.output.outDir.has_value() && shared.output.outFile.has_value())
								throwContradiction("--out-dir", "--out-file");
						}

						/// <summary><c>--out-name</c> is a template for <c>--out-dir</c> and means nothing without it.</summary>
						void checkOutNameNeedsOutDir(const SharedOptions& shared)
						{
							if (shared.output.nameTemplate.has_value() && !shared.output.outDir.has_value())
							{
								throw UsageError(ErrorCodes::outNameNeedsOutDir,
									"--out-name is a template for --out-dir and has no meaning without it");
							}
						}

						/// <summary><c>--stdout</c> cannot be combined with a directory target.</summary>
						void checkStdoutHasNoDirectory(const SharedOptions& shared)
						{
							if (shared.output.outDir.has_value() && shared.output.useStdout)
								throwContradiction("--out-dir", "--stdout");
						}

						/// <summary>
						/// <c>--stdout</c> cannot be combined with a file target either.
						/// </summary>
						/// <remarks>
						/// <c>--out-file -</c> is the spelling that means stdout, so it is
						/// not caught here; <c>--stdout --out-file book.pdf</c> is, because
						/// output cannot go to both places. Neither
						/// <c>checkOutputTargetsExclusive</c> nor
						/// <c>checkStdoutHasNoDirectory</c> covers this pair.
						/// </remarks>
						void checkStdoutHasNoFileTarget(const SharedOptions& shared)
						{
							if (shared.output.outFile.has_value() && shared.output.useStdout)
								throwContradiction("--out-file", "--stdout");
						}

						/// <summary>
						/// <c>--log-level</c> and <c>--debug-log-level</c> are the same
						/// setting, so giving both with different values is a
						/// contradiction. Likewise <c>--log-modules</c> and
						/// <c>--debug-log-modules</c>.
						/// </summary>
						/// <remarks>
						/// Agreeing values are allowed, as with <c>--fail-fast</c>
						/// alongside an explicit <c>--on-error stop</c>: redundant is not
						/// wrong.
						/// </remarks>
						void checkLogAliasesConsistent(const SharedOptions& shared)
						{
							const DiagnosticOptions& diagnostics = shared.diagnostics;
							if (diagnostics.logLevel.has_value() && diagnostics.debugLogLevel.has_value()
								&& *diagnostics.logLevel != *diagnostics.debugLogLevel)
							{
								throwContradiction("--log-level " + *diagnostics.logLevel, "--debug-log-level " + *diagnostics.debugLogLevel);
							}
							if (diagnostics.logModules.has_value() && diagnostics.debugLogModules.has_value()
								&& *diagnostics.logModules != *diagnostics.debugLogModules)
							{
								throwContradiction(
									"--log-modules " + *diagnostics.logModules, "--debug-log-modules " + *diagnostics.debugLogModules);
							}
						}

						/// <summary><c>--no-recurse</c> contradicts an explicit <c>--max-depth</c> other than 1.</summary>
						/// <remarks>
						/// Decidable only because the two arguments write separate fields;
						/// if <c>--no-recurse</c> wrote the depth, this rule could not
						/// exist.
						/// </remarks>
						void checkRecursionDepthConsistent(const SharedOptions& shared)
						{
							if (shared.input.maxDepth.has_value() && *shared.input.maxDepth != 1 && shared.input.noRecurseWasGiven)
								throwContradiction("--max-depth", "--no-recurse");
						}

						/// <summary><c>--verbose</c> and <c>--silent</c> contradict each other.</summary>
						void checkVerbosityConsistent(const SharedOptions& shared)
						{
							if (shared.console.verbose && shared.console.silent)
								throwContradiction("--verbose", "--silent");
						}

						/// <summary>
						/// <c>-g/--glob</c> and <c>--regex</c> are two ways of saying the
						/// same thing, and cannot both be given.
						/// </summary>
						/// <remarks>
						/// Glob is the idiom for ordinary selection and regex the power
						/// mode for patterns a glob cannot express; mixing them in one
						/// selection has no coherent meaning, and combining them silently
						/// would make the result depend on argument order.
						/// </remarks>
						void checkPatternLanguagesExclusive(const SharedOptions& shared)
						{
							if (!shared.input.globPatterns.empty() && !shared.input.regexPatterns.empty())
								throwContradiction("--glob", "--regex");
						}

						/// <summary>
						/// <c>--fail-fast</c> contradicts an explicit <c>--on-error skip</c>.
						/// </summary>
						/// <remarks>
						/// Rejected rather than resolved by precedence: the two arguments
						/// write the same setting, and the binder records each argument's
						/// occurrences rather than the relative order of two different
						/// arguments. All four implementations could be taught to record
						/// that order now that none of them delegates to a parsing library,
						/// and none is: a tie-break is a rule four ports would have to agree
						/// on for ever, where an error is a rule they already agree on.
						/// </remarks>
						void checkErrorPolicyConsistent(const SharedOptions& shared)
						{
							if (shared.batch.onErrorWasExplicit && shared.batch.onError == OnErrorBehaviour::Skip
								&& shared.batch.failFastWasGiven)
							{
								throwContradiction("--on-error skip", "--fail-fast");
							}
						}

						/// <summary><c>--combine</c> needs a format that can hold several documents.</summary>
						/// <remarks>
						/// Keyed off <c>FormatSpec::supportsCombine</c>, not
						/// <c>isSingleFile</c>. Two different formats fail here for two
						/// different reasons, and the message says which: a per-page
						/// format writes one file per page and has nothing to merge into,
						/// while <c>docpack</c> writes a single file but holds exactly one
						/// document, because <c>Extractor::MakeDocPack</c> takes one
						/// extractor.
						/// </remarks>
						void checkCombineUnsupportedFormat(const ConvertOptions& options)
						{
							if (!options.combine || options.format->supportsCombine)
								return;

							if (!options.format->isSingleFile)
							{
								throw UsageError(ErrorCodes::combineUnsupported,
									"--combine needs a format that writes one file; '" + options.format->token + "' writes one per page");
							}

							throw UsageError(ErrorCodes::combineUnsupported,
								"--combine cannot merge documents into '" + options.format->token + "'; each document makes its own pack");
						}

						/// <summary><c>--combine</c> needs an output file to combine into.</summary>
						/// <remarks>
						/// Execution combines only when it has a target to combine into, so with
						/// <c>--stdout</c> it silently did not combine: each document was converted
						/// on its own and the results ran into one another down the pipe, which for
						/// a single-file format like PDF is several complete documents end to end
						/// rather than one. The user asked for one document and got a stream no
						/// reader can open, with exit 0 to say it worked.
						///
						/// Refused rather than implemented: combining into a stream would mean a
						/// canvas that writes to stdout and outlives every document, which is a
						/// feature, not a fix. <c>--out-file</c> names the file to combine into, and
						/// the result can be piped from there.
						///
						/// Streaming several documents is still allowed without <c>--combine</c>:
						/// for text that is concatenation and exactly what a pipeline wants.
						/// </remarks>
						void checkCombineHasAFileToCombineInto(const SharedOptions& shared, const ConvertOptions& options)
						{
							if (options.combine && shared.output.useStdout)
								throwContradiction("--combine", "--stdout");
						}

						/// <summary><c>--stdout</c> can only stream a format that writes a single file.</summary>
						void checkStdoutNeedsSingleFileFormat(const SharedOptions& shared, const ConvertOptions& options)
						{
							if (shared.output.useStdout && !options.format->isSingleFile)
							{
								throw UsageError(ErrorCodes::stdoutNeedsSingleFile,
									"--stdout can only stream a format that writes one file; '" + options.format->token
										+ "' writes one per page");
							}
						}

						/// <summary><c>--out-file</c> can only name a format that writes a single file.</summary>
						/// <remarks>
						/// The same rule as the one above, for the other way of naming one
						/// output. It is decidable from the command line alone, so it
						/// belongs here rather than in the planner --- which matters,
						/// because the planner's <c>E_OUTFILE_MULTI_INPUT</c> only fires
						/// once there is more than one input and would otherwise leave
						/// <c>--to png --out-file one.png a.docx</c> unreported.
						/// </remarks>
						void checkOutFileNeedsSingleFileFormat(const SharedOptions& shared, const ConvertOptions& options)
						{
							if (shared.output.outFile.has_value() && !options.format->isSingleFile)
							{
								throw UsageError(ErrorCodes::outFileNeedsSingleFile,
									"--out-file can only name a format that writes one file; '" + options.format->token
										+ "' writes one per page");
							}
						}

						/// <summary>
						/// A per-page format needs a <c>--out-name</c> template containing
						/// <c>{page}</c>, or every page resolves to the same path.
						/// </summary>
						/// <remarks>
						/// Without this, <c>--to png --out-name "{stem}.{ext}"</c> either
						/// fails after writing page one, or --- with <c>--overwrite</c> ---
						/// silently leaves only the last page. The default template already
						/// includes the page number; this rule covers the case where the
						/// user supplied their own.
						/// </remarks>
						void checkTemplateHasPageForPerPageFormat(const SharedOptions& shared, const ConvertOptions& options)
						{
							if (options.format->isSingleFile)
								return;

							// The fallback is only constructed when it is actually needed,
							// matching C#'s lazily-evaluated shared.Output.NameTemplate ?? ....
							bool usesPage = shared.output.nameTemplate.has_value() ? shared.output.nameTemplate->usesPage()
																					: Planning::OutputNameTemplate::defaultTemplate(false).usesPage();

							if (!usesPage)
							{
								throw UsageError(ErrorCodes::templateNeedsPage,
									"--out-name must contain {page} when the format writes one file per page");
							}
						}

						/// <summary>
						/// <c>--fingerprint</c> renders a page only to read a fingerprint
						/// attribute off it; the render itself is never kept, so every
						/// argument whose entire purpose is to control or expose that
						/// now-discarded content contradicts it.
						/// </summary>
						/// <remarks>
						/// Confirmed against doc2text's own <c>--fingerprint</c>
						/// (<c>main.cpp</c>): it forces a throwaway image render and throws
						/// it away through a temporary stream rather than the requested
						/// target, whenever the selected format is not already an image
						/// format. <c>--redact</c>, <c>--form</c> and <c>--watermark</c>
						/// are unaffected: they decorate the same throwaway render
						/// fingerprinting already needs, so there is nothing for them to
						/// contradict.
						/// </remarks>
						void checkFingerprintHasNoNormalOutput(const SharedOptions& shared, const ConvertOptions& options)
						{
							if (!options.fingerprint)
								return;

							if (shared.output.useStdout)
								throwContradiction("--fingerprint", "--stdout");
							if (options.view)
								throwContradiction("--fingerprint", "--view");
							if (options.combine)
								throwContradiction("--fingerprint", "--combine");
							if (shared.output.outFile.has_value())
								throwContradiction("--fingerprint", "--out-file");
							if (shared.output.outDir.has_value())
								throwContradiction("--fingerprint", "--out-dir");
						}

						/// <summary>
						/// A structure report needs a format that opens the document
						/// paginated.
						/// </summary>
						/// <remarks>
						/// Not a preference --- one document open cannot serve both. Page
						/// structures only exist on a paginated handle, and text
						/// extraction only works on a text one, so pairing a report with a
						/// text format fails whichever way the format bit goes. So the
						/// combination is refused before anything opens, which is also
						/// what doc2text does --- its report flags and its conversions are
						/// alternative output modes, never both at once. One code, two
						/// messages, because <c>text</c> and <c>docpack</c> are unusable
						/// here for different reasons.
						///
						/// Reads <c>--to</c>, which is the only thing a conversion format comes from: the
						/// planner resolves targets from <c>--to</c> alone, so that is the
						/// format the run is described as producing, and a rule that
						/// judged a different one would contradict the plan it validates.
						/// </remarks>
						void checkReportsNeedCanvasFormat(const ConvertOptions& options)
						{
							if (!options.reports.any())
								return;

							const Planning::FormatSpec* format = options.format;
							if (format == nullptr || format->kind == Planning::FormatKind::Canvas)
								return;

							std::string report = options.reports.requested()[0];
							throw UsageError(ErrorCodes::reportNeedsHiDef,
								format->kind == Planning::FormatKind::DocPack
									? report + " needs a high-definition format; '" + format->token + "' writes a document pack, not pages"
									: report + " needs a high-definition format; '" + format->token + "' is text-mode only");
						}

						/// <summary><c>--to</c> is <c>convert</c>'s one required argument.</summary>
						/// <remarks>
						/// Every other rule in <c>validateConvert</c> reads
						/// <c>options.format</c> unconditionally, so this has to run
						/// first: otherwise the first symptom of a missing <c>--to</c>
						/// would be a null-pointer dereference rather than the usage error
						/// the mistake actually is.
						/// </remarks>
						void checkFormatIsProvided(const ConvertOptions& options)
						{
							if (options.format == nullptr)
								throw UsageError(ErrorCodes::missingArgument, "--to is required");
						}

						/// <summary><c>--make-dirs</c> and <c>--flat</c> contradict each other.</summary>
						void checkDirectoryLayoutConsistent(const ExtractOptions& options)
						{
							if (options.makeDirsWasGiven && options.flatWasGiven)
								throwContradiction("--make-dirs", "--flat");
						}
					}

					void validate(const SharedOptions& shared)
					{
						checkOutDirNamesADirectory(shared);
						checkOutputTargetsExclusive(shared);
						checkOutNameNeedsOutDir(shared);
						checkStdoutHasNoDirectory(shared);
						checkStdoutHasNoFileTarget(shared);
						checkLogAliasesConsistent(shared);
						checkRecursionDepthConsistent(shared);
						checkVerbosityConsistent(shared);
						checkPatternLanguagesExclusive(shared);
						checkErrorPolicyConsistent(shared);
					}

					void validateConvert(const SharedOptions& shared, const ConvertOptions& options)
					{
						validate(shared);
						checkFormatIsProvided(options);
						checkCombineUnsupportedFormat(options);
						checkStdoutNeedsSingleFileFormat(shared, options);
						checkCombineHasAFileToCombineInto(shared, options);
						checkOutFileNeedsSingleFileFormat(shared, options);
						checkTemplateHasPageForPerPageFormat(shared, options);
						checkFingerprintHasNoNormalOutput(shared, options);
						checkReportsNeedCanvasFormat(options);
					}

					void validateExtract(const SharedOptions& shared, const ExtractOptions& options)
					{
						validate(shared);
						checkDirectoryLayoutConsistent(options);
					}
				}
			}
		}
	}
}
