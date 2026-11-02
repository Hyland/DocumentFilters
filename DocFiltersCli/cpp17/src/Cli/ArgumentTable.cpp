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

#include "ArgumentTable.h"

#include "Configuration/ValueParsers.h"
#include "Planning/FormatCatalog.h"
#include "Planning/OutputNameTemplate.h"
#include "Planning/PageRange.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Cli
			{
				namespace ArgumentTable
				{
					namespace
					{
						using Shared = Configuration::SharedOptions;

						/// <summary>Moves every element of <paramref name="source"/> onto the end of <paramref name="target"/>.</summary>
						template <typename T>
						void appendAll(std::vector<ArgumentDefinition<T>>& target, std::vector<ArgumentDefinition<T>> source)
						{
							for (ArgumentDefinition<T>& item : source)
								target.push_back(std::move(item));
						}

						/// <summary>
						/// The shared groups every input-processing command takes regardless of
						/// whether it also writes output or descends into subfiles: name
						/// selection, type filters, the walk, passwords, processing, batch
						/// control, console, the global switches, diagnostics and the engine
						/// location.
						/// </summary>
						std::vector<ArgumentDefinition<Shared>> coreArguments()
						{
							std::vector<ArgumentDefinition<Shared>> result;
							appendAll(result, matchArguments());
							appendAll(result, typeArguments());
							appendAll(result, walkArguments());
							appendAll(result, passwordArguments());
							appendAll(result, processingArguments());
							appendAll(result, batchArguments());
							appendAll(result, consoleArguments());
							appendAll(result, global());
							appendAll(result, diagnosticArguments());
							appendAll(result, libraryArguments());
							return result;
						}
					}

					const std::vector<std::string>& commands()
					{
						static const std::vector<std::string> value = {
							"convert", "extract", "list", "identify", "meta", "formats", "options"
						};
						return value;
					}

					const std::vector<std::string>& allVerbs()
					{
						static const std::vector<std::string> value = [] {
							std::vector<std::string> result = commands();
							result.push_back("help");
							result.push_back("version");
							return result;
						}();
						return value;
					}

					/// <remarks>
					/// Composed from the group functions below rather than filtered out of one
					/// flat list. A verb simply does not call the groups it has no use for:
					/// <c>identify</c> writes no files, so it never calls
					/// <see cref="outputArguments"/>; <c>formats</c> and <c>options</c> take no
					/// input files, so they call neither the walk nor the selection groups.
					///
					/// Expressing it as composition rather than a group tag on each argument
					/// means there is no group name to mistype and no filter to get wrong --- an
					/// argument is on a command because that command asked for its group.
					/// </remarks>
					std::vector<ArgumentDefinition<Shared>> sharedFor(const std::string& command)
					{
						std::vector<ArgumentDefinition<Shared>> result;

						if (command == "convert" || command == "extract" || command == "list")
						{
							appendAll(result, outputArguments());
							appendAll(result, subfileArguments());
							appendAll(result, coreArguments());
						}
						else if (command == "identify")
						{
							appendAll(result, coreArguments());
						}
						else if (command == "meta")
						{
							appendAll(result, outputArguments());
							appendAll(result, coreArguments());
						}
						else if (command == "formats" || command == "options")
						{
							appendAll(result, consoleArguments());
							appendAll(result, global());
							appendAll(result, libraryArguments());
						}
						else
						{
							throw std::invalid_argument("unknown command '" + command + "'");
						}

						return result;
					}

					std::vector<ArgumentDefinition<Shared>> outputArguments()
					{
						return {
							ArgumentDefinition<Shared>("-o|--out-dir", "Write into a directory; safe for any format",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.output.outDir = *value; }),

							ArgumentDefinition<Shared>("--out-file", "Write one artifact to exactly this path; '-' means stdout",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									if (*value == "-")
										shared.output.useStdout = true;
									else
										shared.output.outFile = *value;
								}),

							ArgumentDefinition<Shared>("--out-name", "Filename template inside --out-dir, e.g. {stem}-{page:04}.{ext}",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.output.nameTemplate = Planning::OutputNameTemplate::parse(*value);
								}),

							ArgumentDefinition<Shared>("--stdout", "Stream a single-file format to stdout",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.output.useStdout = true; }),

							ArgumentDefinition<Shared>("--overwrite", "Replace existing outputs instead of erroring",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.output.overwrite = true; }),
						};
					}

					std::vector<ArgumentDefinition<Shared>> subfileArguments()
					{
						return {
							ArgumentDefinition<Shared>("-s|--subfiles", "Descend into subfiles, attachments and embedded documents",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.subfiles.enabled = true; }),

							ArgumentDefinition<Shared>("--subfile-depth", "How deep to recurse into nested subfiles",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.subfiles.maxDepth = Configuration::ValueParsers::parseCount("--subfile-depth", *value);
								}),
						};
					}

					std::vector<ArgumentDefinition<Shared>> matchArguments()
					{
						return {
							ArgumentDefinition<Shared>("-g|--glob", "Keep paths matching a glob, e.g. **/*.pdf; prefix ! to exclude",
								ArgumentArity::MultipleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.input.globPatterns.push_back(*value); }),

							ArgumentDefinition<Shared>("--regex", "Same selection, but match the path with a regular expression",
								ArgumentArity::MultipleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.input.regexPatterns.push_back(*value); }),
						};
					}

					std::vector<ArgumentDefinition<Shared>> typeArguments()
					{
						return {
							ArgumentDefinition<Shared>("--include-types", "Semicolon list of format ids to keep",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.input.includeTypes = Configuration::ValueParsers::parseIdList(*value);
								}),

							ArgumentDefinition<Shared>("--exclude-types", "Semicolon list of format ids to drop",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.input.excludeTypes = Configuration::ValueParsers::parseIdList(*value);
								}),
						};
					}

					std::vector<ArgumentDefinition<Shared>> walkArguments()
					{
						return {
							ArgumentDefinition<Shared>("--files-from", "Read input paths from a file, one per line; - for stdin",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.input.filesFrom = *value; }),

							ArgumentDefinition<Shared>("--max-depth", "Limit walk depth; --max-depth 1 stays in the given directory",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.input.maxDepth = Configuration::ValueParsers::parseCount("--max-depth", *value);
								}),

							ArgumentDefinition<Shared>("--no-recurse", "Don't descend into subdirectories (= --max-depth 1)",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.input.noRecurseWasGiven = true; }),

							ArgumentDefinition<Shared>("-L|--follow", "Follow symlinked directories",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.input.followSymlinks = true; }),

							ArgumentDefinition<Shared>("--sort", "Sort entries before processing",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.input.sort = true; }),

							ArgumentDefinition<Shared>("--include-hidden", "Include hidden and system files",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.input.includeHidden = true; }),
						};
					}

					std::vector<ArgumentDefinition<Shared>> passwordArguments()
					{
						return {
							ArgumentDefinition<Shared>("-p|--password", "A candidate password, repeatable",
								ArgumentArity::MultipleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.passwords.passwords.push_back(*value); }),

							ArgumentDefinition<Shared>("--password-file", "One candidate password per line",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.passwords.passwordFile = *value; }),

							ArgumentDefinition<Shared>("--password-prompt", "Prompt for passwords interactively",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.passwords.prompt = true; }),
						};
					}

					std::vector<ArgumentDefinition<Shared>> processingArguments()
					{
						return {
							ArgumentDefinition<Shared>("--options", "Semicolon-separated engine options, e.g. BW_MODE=3;CODEPAGE=1252",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.processing.engineOptions = *value; }),

							ArgumentDefinition<Shared>("--codepage", "Default codepage when the document does not declare one",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.processing.codepage = *value; }),

							ArgumentDefinition<Shared>("--temp-dir", "Temp directory for scratch files",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.processing.tempDir = *value; }),
						};
					}

					std::vector<ArgumentDefinition<Shared>> batchArguments()
					{
						return {
							ArgumentDefinition<Shared>("--on-error", "On a per-file failure: skip (default) or stop the run",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.batch.onError =
										Configuration::ValueParsers::parseOnError(Configuration::ValueParsers::foldToken(*value));
									shared.batch.onErrorWasExplicit = true;
								}),

							ArgumentDefinition<Shared>("--fail-fast", "Stop at the first failure (= --on-error stop)",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&)
								{
									// Guarded rather than unconditional: --on-error's own setter
									// always has the last word on onError/onErrorWasExplicit
									// whenever it runs, regardless of which of the two the parsing
									// library applies first, so leaving an explicit value alone
									// here is what makes the pair's resolution independent of
									// argument order rather than of registration order in this
									// table.
									if (!shared.batch.onErrorWasExplicit)
										shared.batch.onError = Configuration::OnErrorBehaviour::Stop;
									shared.batch.failFastWasGiven = true;
								}),
						};
					}

					std::vector<ArgumentDefinition<Shared>> consoleArguments()
					{
						return {
							ArgumentDefinition<Shared>("--color", "Colour errors and verbose output; ignored on a redirected stream",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.console.color = true; }),

							ArgumentDefinition<Shared>("--verbose", "Print more about what the run is doing",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.console.verbose = true; }),

							ArgumentDefinition<Shared>("--silent", "Print nothing but errors",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.console.silent = true; }),
						};
					}

					std::vector<ArgumentDefinition<Shared>> diagnosticArguments()
					{
						return {
							ArgumentDefinition<Shared>("--time", "Report how long the run took",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.diagnostics.time = true; }),

							ArgumentDefinition<Shared>("--logtime", "Log timing for each stage",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.diagnostics.logTime = true; }),

							ArgumentDefinition<Shared>("--logmem", "Sample and report peak process memory use",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.diagnostics.logMemory = true; }),

							ArgumentDefinition<Shared>("--progress", "Print a progress indicator",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.diagnostics.progress = true; }),

							ArgumentDefinition<Shared>("--pause", "Pause before exiting",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.diagnostics.pause = true; }),

							ArgumentDefinition<Shared>("--max-memory", "Abort the run if it exceeds this many megabytes",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.diagnostics.maxMemory = Configuration::ValueParsers::parseMegabytes("--max-memory", *value);
								}),

							ArgumentDefinition<Shared>("--max-time", "Abort the run after this many seconds",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.diagnostics.maxTime = Configuration::ValueParsers::parseSeconds("--max-time", *value);
								}),

							ArgumentDefinition<Shared>("--timeout", "Abort a single document after this many seconds",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.diagnostics.timeout = Configuration::ValueParsers::parseSeconds("--timeout", *value);
								}),

							ArgumentDefinition<Shared>("-x", "Report engine memory status once, at the end of the run",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&)
								{
									shared.diagnostics.memoryStatusLevel = std::max(shared.diagnostics.memoryStatusLevel, 1);
								}),

							ArgumentDefinition<Shared>("--xx", "Report engine memory status after every document too",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&)
								{
									shared.diagnostics.memoryStatusLevel = std::max(shared.diagnostics.memoryStatusLevel, 2);
								}),

							ArgumentDefinition<Shared>("--log-level", "How much the low-level readers log",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.diagnostics.logLevel = Configuration::ValueParsers::foldToken(*value);
								}),

							ArgumentDefinition<Shared>("--debug-log-level", "--log-level setting under its original name",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.diagnostics.debugLogLevel = Configuration::ValueParsers::foldToken(*value);
								}),

							ArgumentDefinition<Shared>("--log-modules", "Which modules log",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.diagnostics.logModules = *value; }),

							ArgumentDefinition<Shared>("--debug-log-modules", "--log-modules setting under its original name",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.diagnostics.debugLogModules = *value; }),

							ArgumentDefinition<Shared>("--debug-use-memory-stream", "Read each document through a memory stream",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.diagnostics.useMemoryStream = true; }),

							ArgumentDefinition<Shared>("--debug-measure-io", "Report I/O counts and volumes per document",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.diagnostics.measureIo = true; }),

							ArgumentDefinition<Shared>("--debug-truncate", "Read only the first N bytes of each input",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value)
								{
									shared.diagnostics.truncateInputBytes =
										Configuration::ValueParsers::parseCount("--debug-truncate", *value);
								}),
						};
					}

					std::vector<ArgumentDefinition<Shared>> libraryArguments()
					{
						return {
							ArgumentDefinition<Shared>("--license-key", "Document Filters license key; falls back to DF_LICENSE_KEY",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.library.licenseKey = *value; }),

							ArgumentDefinition<Shared>("--library", "Path to the Document Filters engine; falls back to DF_PATH",
								ArgumentArity::SingleValue,
								[](Shared& shared, const std::optional<std::string>& value) { shared.library.libraryPath = *value; }),
						};
					}

					std::vector<ArgumentDefinition<Shared>> global()
					{
						return {
							ArgumentDefinition<Shared>("--dry-run", "Plan the run and print it, without opening a document",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.dryRun = true; }),

							ArgumentDefinition<Shared>("--print-plan", "Alias for --dry-run",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.dryRun = true; }),

							ArgumentDefinition<Shared>("--print-contract", "Print the whole argument surface as flat text, and stop",
								ArgumentArity::NoValue,
								[](Shared& shared, const std::optional<std::string>&) { shared.printContract = true; }),
						};
					}

					std::vector<ArgumentDefinition<Configuration::ConvertOptions>> convert()
					{
						using Options = Configuration::ConvertOptions;
						return {
							ArgumentDefinition<Options>("--to", "Target format: text, md, html5, pdf, png, json, xml, tiff ...",
								ArgumentArity::SingleValue,
								[](Options& options, const std::optional<std::string>& value)
								{
									options.format = &Planning::FormatCatalog::resolve(Configuration::ValueParsers::foldToken(*value));
								}),

							ArgumentDefinition<Options>("--combine", "Merge every input into one output document",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.combine = true; }),

							ArgumentDefinition<Options>("--canvas-options", "Override canvas defaults, passed to the engine untouched",
								ArgumentArity::SingleValue,
								[](Options& options, const std::optional<std::string>& value) { options.canvasOptions = *value; }),

							ArgumentDefinition<Options>("--render-options", "Override render-page defaults, passed to the engine untouched",
								ArgumentArity::SingleValue,
								[](Options& options, const std::optional<std::string>& value) { options.renderOptions = *value; }),

							ArgumentDefinition<Options>("--no-images", "Skip embedded images during conversion",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.noImages = true; }),

							ArgumentDefinition<Options>("--include-control-codes", "Keep ISYS control codes in text output",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.includeControlCodes = true; }),

							ArgumentDefinition<Options>("--fingerprint", "Emit page fingerprints",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.fingerprint = true; }),

							ArgumentDefinition<Options>("--watermark", "Stamp each page; a file path or a literal string",
								ArgumentArity::SingleValue,
								[](Options& options, const std::optional<std::string>& value) { options.watermark = *value; }),

							ArgumentDefinition<Options>("--redact", "Black out a region: page,x,y,w,h",
								ArgumentArity::MultipleValue,
								[](Options& options, const std::optional<std::string>& value)
								{
									options.redactions.push_back(Configuration::ValueParsers::parseRedaction(*value));
								}),

							ArgumentDefinition<Options>("--form", "Set a form field value: page,name=value",
								ArgumentArity::MultipleValue,
								[](Options& options, const std::optional<std::string>& value)
								{
									options.formValues.push_back(Configuration::ValueParsers::parseFormValue(*value));
								}),

							ArgumentDefinition<Options>("--view", "Open the result when done",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.view = true; }),

							ArgumentDefinition<Options>("--fonts", "Report fonts referenced in the document",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.fonts = true; }),

							ArgumentDefinition<Options>("--font-mappings", "Report the document's font mappings",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.fontMappings = true; }),

							ArgumentDefinition<Options>("--page-fonts", "Report fonts per page",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.pageFonts = true; }),

							ArgumentDefinition<Options>("--page-font-mappings", "Report font mappings per page",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.pageFontMappings = true; }),

							ArgumentDefinition<Options>("--dimensions", "Report each page's dimensions",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.dimensions = true; }),

							ArgumentDefinition<Options>("--page-elements", "Report each page's page elements",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.pageElements = true; }),

							ArgumentDefinition<Options>("--form-elements", "Report each page's form elements",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.formElements = true; }),

							ArgumentDefinition<Options>("--hyperlinks", "Report each page's hyperlink elements",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.hyperlinks = true; }),

							ArgumentDefinition<Options>("--bookmarks", "Report the document's bookmarks",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.bookmarks = true; }),

							ArgumentDefinition<Options>("--annotations", "Report the document's annotations",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.reports.annotations = true; }),

							ArgumentDefinition<Options>("--pages", "Page ranges to render, e.g. 1,3-5,8",
								ArgumentArity::SingleValue,
								[](Options& options, const std::optional<std::string>& value)
								{
									options.pages = Planning::PageRange::parse(*value);
								}),
						};
					}

					std::vector<ArgumentDefinition<Configuration::ExtractOptions>> extract()
					{
						using Options = Configuration::ExtractOptions;
						return {
							ArgumentDefinition<Options>("-i|--images", "Also pull embedded images, not just subfiles",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.images = true; }),

							ArgumentDefinition<Options>("--include", "Keep subfiles whose name matches this glob, repeatable",
								ArgumentArity::MultipleValue,
								[](Options& options, const std::optional<std::string>& value) { options.includePatterns.push_back(*value); }),

							ArgumentDefinition<Options>("--exclude", "Drop subfiles whose name matches this glob, repeatable",
								ArgumentArity::MultipleValue,
								[](Options& options, const std::optional<std::string>& value) { options.excludePatterns.push_back(*value); }),

							ArgumentDefinition<Options>("--make-dirs", "Replicate the source tree under the output directory",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.makeDirsWasGiven = true; }),

							ArgumentDefinition<Options>("--flat", "Put every extracted file straight into the output directory",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.flatWasGiven = true; }),

							ArgumentDefinition<Options>("--sub-id", "Append the subfile id to extracted filenames",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.appendSubfileId = true; }),
						};
					}

					std::vector<ArgumentDefinition<Configuration::ListOptions>> list()
					{
						using Options = Configuration::ListOptions;
						return {
							ArgumentDefinition<Options>("--json", "Machine-readable listing",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.json = true; }),
						};
					}

					std::vector<ArgumentDefinition<Configuration::IdentifyOptions>> identify()
					{
						using Options = Configuration::IdentifyOptions;
						return {
							ArgumentDefinition<Options>("--json", "Machine-readable output",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.json = true; }),

							ArgumentDefinition<Options>("--capabilities", "Also print the format's capability flags",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.capabilities = true; }),
						};
					}

					std::vector<ArgumentDefinition<Configuration::MetaOptions>> meta()
					{
						using Options = Configuration::MetaOptions;
						return {
							ArgumentDefinition<Options>("--json", "Metadata as JSON instead of plain text",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.json = true; }),
						};
					}

					std::vector<ArgumentDefinition<Configuration::FormatsOptions>> formats()
					{
						using Options = Configuration::FormatsOptions;
						return {
							ArgumentDefinition<Options>("--json", "Machine-readable catalog",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.json = true; }),

							ArgumentDefinition<Options>("--filter", "Only formats whose id or name matches",
								ArgumentArity::SingleValue,
								[](Options& options, const std::optional<std::string>& value) { options.filter = *value; }),
						};
					}

					std::vector<ArgumentDefinition<Configuration::OptionCatalogOptions>> optionCatalog()
					{
						using Options = Configuration::OptionCatalogOptions;
						return {
							ArgumentDefinition<Options>("--json", "Full catalog: name, description, default, type, appliesTo",
								ArgumentArity::NoValue,
								[](Options& options, const std::optional<std::string>&) { options.json = true; }),

							ArgumentDefinition<Options>("--filter", "Only options whose name or description matches",
								ArgumentArity::SingleValue,
								[](Options& options, const std::optional<std::string>& value) { options.filter = *value; }),

							ArgumentDefinition<Options>("--applies-to", "Filter by scope: document, page, canvas, render ...",
								ArgumentArity::SingleValue,
								[](Options& options, const std::optional<std::string>& value)
								{
									options.appliesTo = Configuration::ValueParsers::foldToken(*value);
								}),
						};
					}

					std::vector<std::shared_ptr<const IArgumentDescriptor>> describeCommand(const std::string& command)
					{
						std::vector<std::shared_ptr<const IArgumentDescriptor>> result;

						auto addAll = [&result](auto definitions)
						{
							for (auto& definition : definitions)
								result.push_back(std::make_shared<std::decay_t<decltype(definition)>>(std::move(definition)));
						};

						if (command == "convert")
							addAll(convert());
						else if (command == "extract")
							addAll(extract());
						else if (command == "list")
							addAll(list());
						else if (command == "identify")
							addAll(identify());
						else if (command == "meta")
							addAll(meta());
						else if (command == "formats")
							addAll(formats());
						else if (command == "options")
							addAll(optionCatalog());
						else
							throw std::invalid_argument("unknown command '" + command + "'");

						addAll(sharedFor(command));
						return result;
					}
				}
			}
		}
	}
}
