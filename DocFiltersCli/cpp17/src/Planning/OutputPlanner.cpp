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

#include "OutputPlanner.h"

#include "Runtime/CliError.h"
#include "Runtime/Strings.h"

#include <set>

using Hyland::DocumentFilters::Cli::Discovery::InputSource;

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Planning
			{
				namespace OutputPlanner
				{
					namespace
					{
						namespace ErrorCodes = Runtime::ErrorCodes;
						using Runtime::NoInputsError;
						using Runtime::OutputConflictError;

						/// <summary>The directory portion of a portable, forward-slash relative path.</summary>
						std::string directoryOf(const std::string& relativePath)
						{
							std::string normalised = relativePath;
							for (char& c : normalised)
							{
								if (c == '\\')
									c = '/';
							}

							size_t slash = normalised.rfind('/');
							return slash == std::string::npos ? std::string() : normalised.substr(0, slash);
						}

						std::string combineDirectories(const std::string& outDir, const std::string& relativeDirectory)
						{
							if (relativeDirectory.empty())
								return outDir;
							if (outDir.empty())
								return relativeDirectory;
							return outDir + "/" + relativeDirectory;
						}

						std::string joinPath(const std::string& directory, const std::string& fileName)
						{
							return directory.empty() ? fileName : (directory + "/" + fileName);
						}

						/// <summary>
						/// Resolves one <see cref="OutputCardinality::PerInput"/> item's
						/// target: the source tree replicated under <c>--out-dir</c>, then
						/// the naming template applied to the filename only.
						/// </summary>
						void assignPerInputTarget(PlannedItem& item, const InputSource& input, const FormatSpec& format,
							const OutputNameTemplate& templateToUse, const std::string& outDir)
						{
							std::string relativePath = input.relativePath.empty() ? input.path : input.relativePath;
							std::string directory = combineDirectories(outDir, directoryOf(relativePath));

							if (format.isSingleFile)
							{
								item.target = joinPath(directory, templateToUse.render(item.sourcePath, format.extension, 1));
							}
							else
							{
								TargetPattern pattern;
								pattern.directory = directory;
								pattern.templateName = templateToUse;
								pattern.extension = format.extension;
								item.targetPattern = pattern;
							}
						}

						/// <summary>
						/// Detects two inputs resolving to the same output path, which
						/// happens when a flattened walk contains two files with the same
						/// stem.
						/// </summary>
						/// <remarks>
						/// For statically resolved items this compares the target paths
						/// directly. For page-count-dependent items it compares
						/// <c>TargetPattern::collisionKey</c>, which catches the real-world
						/// case --- <c>a/report.docx</c> and <c>b/report.docx</c> both
						/// flattening into one output directory --- without needing to know
						/// how many pages either has.
						/// </remarks>
						/// <summary>
						/// A planned path reduced to what two outputs must not share.
						/// </summary>
						/// <remarks>
						/// The path itself where the filesystem tells names apart by case,
						/// and a case-folded copy where it does not. Comparing exactly
						/// everywhere let <c>a/Report.docx</c> and <c>b/report.docx</c> both
						/// pass planning and then write to one file on Windows and macOS:
						/// the run exited 0, reported nothing, and the second output had
						/// silently replaced the first.
						///
						/// Folding everywhere instead would have been the other error --- on
						/// a case-sensitive filesystem those are two files, and refusing to
						/// write both declines work the tool can do correctly. So the
						/// question goes to <c>IFileSystem::isCaseSensitive</c> rather than
						/// to the host or to a fixed rule, which is also what keeps it
						/// testable: a fake filesystem states its own answer, so both
						/// behaviours are covered on every platform.
						///
						/// <c>foldAscii</c> rather than anything locale-aware, for the
						/// reason <c>Strings::foldAscii</c> already documents, and because
						/// it is the fold the C# reference applies to the same key. Two names
						/// differing only outside ASCII therefore do not collide here.
						///
						/// What that costs, measured on NTFS: the two spellings of a-ring are
						/// one file there, so two inputs named for them and flattened into one
						/// output directory pass this check and then write to the same place.
						/// What stops the loss is no longer this key but the moment of the
						/// question: each target is looked at again immediately before it is
						/// written, when the earlier output is on disk and the filesystem ---
						/// the only authority on whether two names are one file --- answers for
						/// itself. The run then fails with E_OUTPUT_EXISTS rather than silently
						/// keeping the second.
						///
						/// Under <c>--overwrite</c> it still keeps the second, because that is
						/// what <c>--overwrite</c> asks for and nothing left can tell this apart
						/// from an ordinary replacement. A pair this key does fold is refused
						/// even then.
						/// </remarks>
						std::string collisionKeyForImpl(const std::string& path, Runtime::IFileSystem& fileSystem)
						{
							return fileSystem.isCaseSensitive(path) ? path : Runtime::Strings::foldAscii(path);
						}

						void checkForCollisions(const RunPlan& runPlan, Runtime::IFileSystem& fileSystem)
						{
							std::set<std::string> seen;
							for (const PlannedItem& item : runPlan.items)
							{
								std::optional<std::string> key = item.target.has_value()
									? item.target
									: (item.targetPattern.has_value() ? std::optional<std::string>(item.targetPattern->collisionKey(item.sourcePath))
																	   : std::nullopt);

								if (key.has_value() && !seen.insert(collisionKeyForImpl(*key, fileSystem)).second)
								{
									throw OutputConflictError(
										ErrorCodes::outputCollision, "two inputs would write to the same output; use --out-name to distinguish them");
								}
							}
						}

						/// <summary>Every path the plan would write, as far as it is knowable before the run.</summary>
						std::vector<std::string> plannedTargets(const RunPlan& runPlan)
						{
							std::vector<std::string> targets;
							if (runPlan.combinedTarget.has_value())
								targets.push_back(*runPlan.combinedTarget);

							for (const PlannedItem& item : runPlan.items)
							{
								if (item.target.has_value())
									targets.push_back(*item.target);
								else if (item.targetPattern.has_value())
									targets.push_back(item.targetPattern->firstTarget(item.sourcePath, item.pages));
							}

							return targets;
						}

						/// <summary>
						/// Refuses a plan that would write over one of the documents it reads.
						/// </summary>
						/// <remarks>
						/// <c>--overwrite</c> says an existing file may be replaced, and an input
						/// is an existing file, so nothing else here stops
						/// <c>convert --to text -o . --overwrite notes.txt</c> from naming
						/// <c>notes.txt</c> as both. The combined case is the plainest: the output
						/// is created, and therefore truncated, before the first document is
						/// opened, so the source is gone and the run then fails to read it. A glob
						/// reaches the same place without a typo --- a second run over a directory
						/// picks up the first run's output as an input.
						///
						/// This is the collision check pointed at the other set: two outputs must
						/// not share a path, and an output must not share one with an input
						/// either. It uses the same key, so it folds case exactly where the
						/// filesystem does.
						///
						/// A page-count-dependent item is compared by its first target, for the
						/// reason <c>verifyTargets</c> gives: the rest of the names are not known
						/// until the document is open. Page one is the one that would be written
						/// first, and writing it is what would destroy the source.
						/// </remarks>
						void checkOutputsAreNotInputs(const RunPlan& runPlan, Runtime::IFileSystem& fileSystem)
						{
							std::set<std::string> inputs;
							for (const PlannedItem& item : runPlan.items)
								inputs.insert(collisionKeyForImpl(item.sourcePath, fileSystem));

							for (const std::string& target : plannedTargets(runPlan))
							{
								if (inputs.count(collisionKeyForImpl(target, fileSystem)) != 0)
								{
									throw OutputConflictError(ErrorCodes::outputIsInput,
										target + " is also an input; use --out-dir or --out-name to write somewhere else");
								}
							}
						}

						/// <summary>
						/// The same resolution as the public overload, for <c>convert</c>,
						/// which is the only command with a <c>--combine</c> of its own.
						/// </summary>
						OutputCardinality resolveCardinalityForConvert(
							const Configuration::SharedOptions& shared, const Configuration::ConvertOptions& convert, int inputCount)
						{
							return resolveCardinality(shared, inputCount, convert.combine);
						}
					}

					RunPlan plan(const Configuration::SharedOptions& shared, const Configuration::ConvertOptions& convert,
						const std::vector<InputSource>& inputs, Runtime::IFileSystem& fileSystem)
					{
						if (inputs.empty())
							throw NoInputsError("no inputs matched; the walk and filters selected nothing");

						RunPlan result;
						result.format = convert.format;
						result.cardinality = resolveCardinalityForConvert(shared, convert, static_cast<int>(inputs.size()));

						if (result.cardinality == OutputCardinality::Combined)
							result.combinedTarget = shared.output.outFile;

						const FormatSpec* format = convert.format;
						OutputNameTemplate templateToUse = shared.output.nameTemplate.has_value()
							? *shared.output.nameTemplate
							: OutputNameTemplate::defaultTemplate(format->isSingleFile);
						std::string outDir = shared.output.outDir.has_value() ? *shared.output.outDir : ".";
						PageRange pages = convert.pages.has_value() ? *convert.pages : PageRange::all();

						for (const InputSource& input : inputs)
						{
							PlannedItem item;
							item.sourcePath = input.path;
							item.displayName = input.path;
							item.pages = pages;

							switch (result.cardinality)
							{
								case OutputCardinality::PerInput:
									assignPerInputTarget(item, input, *format, templateToUse, outDir);
									break;

								case OutputCardinality::SingleTarget:
									item.target = shared.output.outFile;
									break;

								case OutputCardinality::Combined:
								case OutputCardinality::Stream:
									// Neither owns a path of its own: a combined item feeds
									// RunPlan::combinedTarget, and a streamed one writes to
									// stdout.
									break;
							}

							result.items.push_back(item);
						}

						checkForCollisions(result, fileSystem);
						checkOutputsAreNotInputs(result, fileSystem);

						return result;
					}

					void verifyTargets(const RunPlan& runPlan, const Configuration::SharedOptions& shared, Runtime::IFileSystem& fileSystem)
					{
						if (shared.output.overwrite)
							return;

						if (runPlan.cardinality == OutputCardinality::Combined)
						{
							verifyPageTarget(runPlan.combinedTarget.has_value() ? *runPlan.combinedTarget : std::string(), shared, fileSystem);
						}
						else
						{
							for (const PlannedItem& item : runPlan.items)
							{
								// A Stream item owns neither: it writes to stdout, not a path.
								if (!item.target.has_value() && !item.targetPattern.has_value())
									continue;

								std::string path = item.isPageCountDependent() ? item.targetPattern->firstTarget(item.sourcePath, item.pages)
																				 : *item.target;
								verifyPageTarget(path, shared, fileSystem);
							}
						}
					}

					std::string collisionKeyFor(const std::string& path, Runtime::IFileSystem& fileSystem)
					{
						return collisionKeyForImpl(path, fileSystem);
					}

					void verifyPageTarget(const std::string& path, const Configuration::SharedOptions& shared, Runtime::IFileSystem& fileSystem)
					{
						if (shared.output.overwrite)
							return;

						if (fileSystem.fileExists(path))
							throw OutputConflictError(ErrorCodes::outputExists, path + " already exists; use --overwrite to replace it");
					}

					OutputCardinality resolveCardinality(const Configuration::SharedOptions& shared, int inputCount, bool combine)
					{
						if (shared.output.useStdout)
							return OutputCardinality::Stream;

						if (shared.output.outFile.has_value())
						{
							if (combine)
								return OutputCardinality::Combined;

							if (inputCount > 1)
							{
								throw OutputConflictError(ErrorCodes::outFileMultiInput,
									"--out-file takes a single input; use --combine to merge several into one document");
							}

							return OutputCardinality::SingleTarget;
						}

						return OutputCardinality::PerInput;
					}
				}
			}
		}
	}
}
