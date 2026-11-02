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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_CONFIGURATION_SHAREDOPTIONS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_CONFIGURATION_SHAREDOPTIONS_H

#include "Planning/NameMatcher.h"
#include "Planning/OutputNameTemplate.h"

#include <optional>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Configuration
			{
				/// <summary>Values for <c>--on-error</c>.</summary>
				enum class OnErrorBehaviour
				{
					/// <summary>Continue with the next file; still exit non-zero at the end.</summary>
					Skip,

					/// <summary>Stop the run at the first failure.</summary>
					Stop
				};

				/// <summary>Which inputs to process, and how to find them.</summary>
				struct InputOptions
				{
					/// <summary>Positional paths: files or directories, in the order given.</summary>
					std::vector<std::string> paths;

					/// <summary><c>--files-from &lt;file&gt;</c>; "-" means read from stdin.</summary>
					std::optional<std::string> filesFrom;

					/// <summary>
					/// <c>--max-depth</c>. Absent means it was not given; directories are
					/// walked recursively by default.
					/// </summary>
					/// <remarks>
					/// <c>--no-recurse</c> deliberately does <em>not</em> write this field.
					/// Two arguments sharing one field destroys the information validation
					/// needs: if <c>--no-recurse</c> wrote 1 here, then
					/// <c>--max-depth 5 --no-recurse</c> would be indistinguishable from a
					/// plain <c>--max-depth 1</c>, and the contradiction could never be
					/// reported. Each argument records its own presence;
					/// <c>effectiveMaxDepth</c> derives the answer.
					/// </remarks>
					std::optional<int> maxDepth;

					/// <summary><c>--no-recurse</c>, which means a depth of 1.</summary>
					bool noRecurseWasGiven = false;

					/// <summary>
					/// The depth actually used by the walk, once the two fields above are
					/// reconciled. Absent means unlimited.
					/// </summary>
					std::optional<int> effectiveMaxDepth() const { return noRecurseWasGiven ? std::optional<int>(1) : maxDepth; }

					/// <summary><c>-L, --follow</c>: enter symlinked directories.</summary>
					bool followSymlinks = false;

					/// <summary><c>--sort</c>: order entries before processing, for reproducible runs.</summary>
					bool sort = false;

					/// <summary><c>--include-hidden</c>.</summary>
					bool includeHidden = false;

					/// <summary><c>-g/--glob</c>, repeatable. A leading <c>!</c> excludes.</summary>
					/// <remarks>
					/// Kept apart from <c>regexPatterns</c> rather than both compiling into
					/// <c>matcher</c> directly: two arguments writing one field would let
					/// <c>-g "**/*.pdf" --regex ".*\.docx"</c> silently drop one of the two,
					/// and the validator could not see that both were given.
					/// </remarks>
					std::vector<std::string> globPatterns;

					/// <summary><c>--regex</c>, repeatable.</summary>
					std::vector<std::string> regexPatterns;

					/// <summary>
					/// The compiled path selection, built from whichever of the two pattern
					/// lists was given. Absent means everything matches.
					/// </summary>
					/// <remarks>
					/// Compiled during binding (<c>CommandLineApp::bind</c>), not here: by
					/// the time anything reads this, <c>OptionsValidator</c> has already
					/// rejected the case where both pattern lists were given. Reading it
					/// before that gives nothing.
					/// </remarks>
					std::optional<Planning::NameMatcher> matcher;

					/// <summary><c>--include-types</c>: format ids to keep. Empty means no restriction.</summary>
					std::vector<std::string> includeTypes;

					/// <summary><c>--exclude-types</c>: format ids to drop.</summary>
					std::vector<std::string> excludeTypes;
				};

				/// <summary>Where output goes. See <c>Planning::OutputPlanner</c> for the rules.</summary>
				struct OutputOptions
				{
					/// <summary><c>-o, --out-dir</c>. Safe for every format.</summary>
					std::optional<std::string> outDir;

					/// <summary><c>--out-file</c>. Exactly one artifact at exactly this path.</summary>
					std::optional<std::string> outFile;

					/// <summary><c>--out-name</c>: filename template used inside <c>outDir</c>.</summary>
					std::optional<Planning::OutputNameTemplate> nameTemplate;

					/// <summary><c>--stdout</c>, or <c>--out-file -</c>.</summary>
					bool useStdout = false;

					/// <summary><c>--overwrite</c>: replace existing outputs instead of erroring.</summary>
					bool overwrite = false;
				};

				/// <summary>Recursion into documents embedded in other documents.</summary>
				struct SubfileOptions
				{
					/// <summary><c>-s, --subfiles</c>.</summary>
					bool enabled = false;

					/// <summary>
					/// <c>--subfile-depth</c>. Absent means unlimited. Entirely separate
					/// from <c>InputOptions::maxDepth</c>, which is directory depth.
					/// </summary>
					std::optional<int> maxDepth;
				};

				/// <summary>How to open password-protected documents.</summary>
				struct PasswordOptions
				{
					/// <summary><c>-p, --password</c>, repeatable.</summary>
					std::vector<std::string> passwords;

					/// <summary><c>--password-file</c>: one candidate password per line.</summary>
					std::optional<std::string> passwordFile;

					/// <summary><c>--password-prompt</c>: ask interactively. Requires a terminal.</summary>
					bool prompt = false;
				};

				/// <summary>Engine tunables and text handling.</summary>
				struct ProcessingOptions
				{
					/// <summary>
					/// <c>--options</c>: the semicolon-separated string, passed to the
					/// engine untouched. The CLI does not parse or validate it --- run the
					/// <c>options</c> command to see the valid keys.
					/// </summary>
					std::optional<std::string> engineOptions;

					/// <summary><c>--codepage</c>: default codepage when the document does not declare one.</summary>
					std::optional<std::string> codepage;

					/// <summary><c>--temp-dir</c>.</summary>
					std::optional<std::string> tempDir;
				};

				/// <summary>What happens when one file in a batch fails.</summary>
				struct BatchOptions
				{
					/// <summary>
					/// <c>--on-error</c>, also written by <c>--fail-fast</c>. Skipping still
					/// exits non-zero if anything failed.
					/// </summary>
					OnErrorBehaviour onError = OnErrorBehaviour::Skip;

					/// <summary>True when <c>--on-error</c> was given explicitly.</summary>
					/// <remarks>
					/// Recorded so that a contradictory pair --- <c>--fail-fast --on-error skip</c>
					/// --- can be rejected as a usage error rather than silently resolved.
					/// None of the four parsing libraries reliably records which of two
					/// different options came first on the command line, so a precedence
					/// rule could not have been made identical across the ports.
					/// </remarks>
					bool onErrorWasExplicit = false;

					/// <summary>True when <c>--fail-fast</c> was given.</summary>
					bool failFastWasGiven = false;
				};

				/// <summary>Terminal output verbosity.</summary>
				struct ConsoleOptions
				{
					/// <summary><c>--color</c>.</summary>
					bool color = false;

					/// <summary><c>--verbose</c>.</summary>
					bool verbose = false;

					/// <summary><c>--silent</c>.</summary>
					bool silent = false;
				};

				/// <summary>
				/// Timing, limits and debugging aids. Their output is not part of the
				/// cross-language contract, because timings and memory figures vary per
				/// run.
				/// </summary>
				struct DiagnosticOptions
				{
					bool time = false;
					bool logTime = false;
					bool logMemory = false;
					bool progress = false;
					bool pause = false;

					/// <summary><c>--max-memory</c>, in megabytes. Absent means no limit.</summary>
					std::optional<long long> maxMemory;

					/// <summary><c>--max-time</c>, in seconds, for the whole run.</summary>
					std::optional<int> maxTime;

					/// <summary><c>--timeout</c>, in seconds, per document.</summary>
					std::optional<int> timeout;

					/// <summary>
					/// How often to report the engine's memory status, from
					/// <c>IGR_Get_Memory_Status</c>.
					/// </summary>
					/// <remarks>
					/// <list type="table">
					/// <item><term>0</term><description>never --- the default</description></item>
					/// <item><term>1</term><description><c>-x</c>: once, at the end of the run</description></item>
					/// <item><term>2</term><description><c>--xx</c>: after every document, and at the end of the run</description></item>
					/// </list>
					/// A level rather than two switches, so <c>--xx</c> implies <c>-x</c>
					/// without the two needing to be reconciled.
					/// </remarks>
					int memoryStatusLevel = 0;

					/// <summary>
					/// <c>--log-level</c> (or <c>--debug-log-level</c>): how much the
					/// low-level readers log. One of none, crit, error, warn, info, debug,
					/// trace.
					/// </summary>
					std::optional<std::string> logLevel;

					/// <summary><c>--debug-log-level</c>: the same setting under its original name.</summary>
					std::optional<std::string> debugLogLevel;

					/// <summary>The log level actually used, from whichever alias was given.</summary>
					std::optional<std::string> effectiveLogLevel() const { return logLevel.has_value() ? logLevel : debugLogLevel; }

					/// <summary><c>--log-modules</c>: which modules log.</summary>
					std::optional<std::string> logModules;

					/// <summary><c>--debug-log-modules</c>: the same setting under its original name.</summary>
					std::optional<std::string> debugLogModules;

					/// <summary>The module list actually used, from whichever alias was given.</summary>
					std::optional<std::string> effectiveLogModules() const { return logModules.has_value() ? logModules : debugLogModules; }

					/// <summary><c>--debug-use-memory-stream</c>: read the document through a memory stream.</summary>
					bool useMemoryStream = false;

					/// <summary><c>--debug-measure-io</c>: report I/O counts and volumes per document.</summary>
					bool measureIo = false;

					/// <summary>
					/// <c>--debug-truncate &lt;bytes&gt;</c>: read only the first N bytes of
					/// each input, for testing how gracefully a format handles a damaged or
					/// partial file.
					/// </summary>
					std::optional<long long> truncateInputBytes;
				};

				/// <summary>How to find and license the Document Filters engine.</summary>
				/// <remarks>
				/// Not in the CLI design document, but nothing runs without them. Both
				/// fall back to the environment variables the other published samples
				/// already use.
				/// </remarks>
				struct LibraryOptions
				{
					/// <summary><c>--license-key</c>, falling back to <c>DF_LICENSE_KEY</c>.</summary>
					std::optional<std::string> licenseKey;

					/// <summary>
					/// <c>--library</c>, falling back to <c>DF_PATH</c>, then to this
					/// tool's own executable directory. Absent here means neither argument
					/// nor environment variable was given, not that there is nowhere to
					/// look.
					/// </summary>
					std::optional<std::string> libraryPath;
				};

				/// <summary>
				/// Everything the shared options resolved to. One instance per run, filled
				/// in by the argument table's setters, then checked by
				/// <c>OptionsValidator</c>.
				/// </summary>
				/// <remarks>
				/// <b>Two arguments never share one field.</b> Where two arguments describe
				/// the same setting, each records its own presence and the setting is
				/// derived from both --- <c>--max-depth</c> plus <c>noRecurseWasGiven</c>
				/// gives <c>InputOptions::effectiveMaxDepth</c>, <c>--log-level</c> plus
				/// <c>--debug-log-level</c> gives <c>DiagnosticOptions::effectiveLogLevel</c>.
				///
				/// Defaults live here as member initialisers, never in the parser
				/// declarations --- four declarations of the same default drift, one does
				/// not.
				/// </remarks>
				struct SharedOptions
				{
					InputOptions input;
					OutputOptions output;
					SubfileOptions subfiles;
					PasswordOptions passwords;
					ProcessingOptions processing;
					BatchOptions batch;
					ConsoleOptions console;

					/// <summary>
					/// <c>--dry-run</c>, or its alias <c>--print-plan</c>: run stages 1 to
					/// 3, print the resulting plan, and stop without opening a document.
					/// </summary>
					bool dryRun = false;

					/// <summary>
					/// <c>--print-contract</c>: emit the whole argument surface as flat
					/// text and stop. A debugging aid, and the same information
					/// <c>help --json</c> returns.
					/// </summary>
					bool printContract = false;

					DiagnosticOptions diagnostics;
					LibraryOptions library;

					/// <summary>
					/// Renders the options in the canonical form used by the conformance
					/// corpus: the <c>walk</c>, <c>match</c>, <c>types</c>, <c>subfiles</c>
					/// and <c>on-error</c> lines of a <c>--print-plan</c> dump, in that
					/// order, one per line, with no trailing newline.
					/// </summary>
					/// <param name="includeSubfiles">Whether to include the <c>subfiles</c> line.</param>
					/// <remarks>
					/// That order is contiguous exactly as written, for every command that
					/// has these fields: a command's own fields --- <c>convert</c>'s
					/// <c>pages</c>; <c>extract</c>'s <c>include</c>/<c>exclude</c>/
					/// <c>images</c>/<c>make-dirs</c>/<c>sub-id</c>; <c>list</c>'s and
					/// <c>meta</c>'s <c>json</c>; every command's <c>cardinality</c> ---
					/// always come after this whole block, never inside it, so a caller
					/// appends this string and then its own lines.
					///
					/// All four language implementations must produce byte-identical
					/// output for each line.
					/// </remarks>
					std::string toCanonicalString(bool includeSubfiles = true) const;
				};
			}
		}
	}
}

#endif
