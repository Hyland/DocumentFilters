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

#include "ConvertCommand.h"

#include "CommandExecution.h"
#include "Configuration/OptionsValidator.h"
#include "Discovery/InputEnumerator.h"
#include "Engine/EngineException.h"
#include "Execution/CanvasConversion.h"
#include "Execution/DebugInput.h"
#include "Execution/Diagnostics.h"
#include "Execution/EngineLog.h"
#include "Execution/ExtractorOpenSettings.h"
#include "Execution/PasswordProvider.h"
#include "Execution/StructureReport.h"
#include "Execution/TextConversion.h"
#include "PlanFormat.h"
#include "Planning/OutputPlanner.h"
#include "Planning/PageRange.h"
#include "Runtime/BatchRunner.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"
#include "Runtime/IFileSystem.h"
#include "Runtime/Viewer.h"

#include <filesystem>
#include <sstream>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				namespace
				{
					/// <summary>
					/// Creates the directory an output is about to be written into, if it
					/// is not already there.
					/// </summary>
					/// <remarks>
					/// Here, in the orchestrator, rather than at each of the three places
					/// that open an output, because only one of those three is ours to
					/// change: the text path writes through <c>IFileSystem</c>, but a
					/// canvas and a docpack are opened by the engine, which will not
					/// create a directory and reports a missing one as a per-document
					/// error that reads as though the input were missing. Doing it per
					/// format meant <c>--out-dir out/</c> worked for <c>--to text</c> and
					/// failed for everything else.
					/// </remarks>
					void ensureOutputDirectory(CommandContext& context, const std::optional<std::string>& directory)
					{
						if (directory.has_value() && !directory->empty())
							context.fileSystem->createDirectory(*directory);
					}

					void convertToText(CommandContext& context, const Configuration::SharedOptions& shared, const Planning::PlannedItem& item,
						const Planning::FormatSpec* format, Engine::IDocument& document, const Configuration::ConvertOptions& options,
						std::ostream* combinedText)
					{
						if (combinedText != nullptr)
						{
							// --combine: every document appends to the one writer execute()
							// owns, so it is not closed here.
							Execution::TextConversion::convert(document, format, options, *combinedText);
						}
						else if (shared.output.useStdout)
						{
							std::ostream& destination = context.console->openStandardOutput();
							Execution::TextConversion::convert(document, format, options, destination);
							destination.flush();
						}
						else
						{
							// Looked at again here, not only when the plan was made. The planner
							// checks every target before the run starts, against a directory none of
							// this run's outputs are in yet, so two inputs whose targets the
							// filesystem treats as one name --- which the collision check's ASCII
							// fold cannot see for a pair differing outside ASCII --- both passed, and
							// the second write replaced the first with no --overwrite and exit 0.
							// Measured on NTFS with the two spellings of a-ring. Asking immediately
							// before the write puts the question to the filesystem, which is the
							// authority on whether two names are one file.
							Planning::OutputPlanner::verifyPageTarget(*item.target, shared, *context.fileSystem);

							std::unique_ptr<std::ostream> destination = context.fileSystem->createFile(*item.target);
							Execution::TextConversion::convert(document, format, options, *destination);
						}
					}

					/// <summary>
					/// <c>--stdout</c> for <c>docpack</c>. <c>item.target</c> is absent
					/// under <see cref="Planning::OutputCardinality::Stream"/>, the same
					/// gap <c>CanvasConversion::convertToStream</c> closed for canvas
					/// formats.
					/// </summary>
					/// <remarks>
					/// Renders into an in-memory buffer first rather than handing
					/// <c>--stdout</c>'s own stream to <c>makeDocPack</c> directly, for the
					/// same reason as the canvas path: a docpack is a binary container
					/// whose finished bytes can only be known once <c>makeDocPack</c> is
					/// done, and the console's own stream is forward-only.
					/// </remarks>
					void convertToDocPack(
						CommandContext& context, const Configuration::SharedOptions& shared, const Planning::PlannedItem& item, Engine::IDocument& document,
						const Configuration::ConvertOptions& options)
					{
						std::string canvasOptions = options.canvasOptions.value_or(std::string());
						if (!shared.output.useStdout)
						{
							document.makeDocPack(*item.target, canvasOptions);
							return;
						}

						std::ostringstream buffer;
						document.makeDocPack(buffer, canvasOptions);

						std::string bytes = buffer.str();
						std::ostream& destination = context.console->openStandardOutput();
						destination.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
						destination.flush();
					}

					/// <summary>
					/// Resolves the format, opens the document once with settings
					/// matching that format, converts, and closes it. Failures become a
					/// <see cref="Runtime::DocumentError"/> for <c>BatchRunner</c> to
					/// count rather than ending the run.
					/// </summary>
					/// <remarks>
					/// The format is settled before the document is opened, which it has
					/// to be: <c>ExtractorOpenSettings</c> depends on it, and the engine
					/// applies those settings during <c>Open</c>. It comes from <c>--to</c>
					/// and nothing else, so there is nothing to discover from the document
					/// and no reason to open one twice.
					/// </remarks>
					void processDocument(CommandContext& context, const Configuration::SharedOptions& shared, const Planning::PlannedItem& item,
						Engine::IOutputCanvas* combinedCanvas, std::ostream* combinedText, Execution::CanvasConversion& canvasConversion,
						Execution::Diagnostics& diagnostics, const Execution::EngineLog* log, const Configuration::ConvertOptions& options)
					{
						const Planning::FormatSpec* format = options.format;
						Execution::PasswordProvider passwords(shared.passwords, *context.console, *context.fileSystem);
						Execution::ExtractorOpenSettings openSettings = Execution::ExtractorOpenSettings::forFormat(*format, options, shared);

						// Once per item, before anything opens an output. --fingerprint is
						// skipped because its target is a plan's guess that is never
						// written to.
						if (!options.fingerprint)
							ensureOutputDirectory(context, item.outputDirectory());

						// A debug switch that changes how the bytes arrive hands the engine
						// a stream instead of the path; otherwise the engine opens the file
						// itself. Declared out here, not inside the try, so it outlives the
						// document that reads from it --- disposing it at the end of the
						// open would close the file mid-read.
						std::unique_ptr<std::istream> debugInput =
							Execution::DebugInput::for_(item.sourcePath, shared.diagnostics, *context.fileSystem, *context.console, item.displayName);

						std::unique_ptr<Engine::IDocument> document;
						Execution::StopSignal stop = CommandExecution::watchForStop(diagnostics, context.interruption);
						try
						{
							document = debugInput == nullptr
								? context.session->openDocument(
									  item.sourcePath, openSettings, passwords, [&stop]() { return stop.shouldStop(); }, log)
								: context.session->openDocument(*debugInput, item.displayName, openSettings, passwords,
									  [&stop]() { return stop.shouldStop(); }, log);
						}
						catch (const Engine::EngineException& error)
						{
							if (error.error() == Engine::EngineError::cancelled)
								stop.raise(item.displayName);
							throw Runtime::DocumentError(item.sourcePath, "could not open document");
						}
						catch (const Runtime::CliError&)
						{
							throw;
						}
						catch (const std::exception&)
						{
							throw Runtime::DocumentError(item.sourcePath, "could not open document");
						}

						{
							// Closes the document here, whether conversion succeeds or
							// fails, matching the C# reference's `using` scope.
							std::unique_ptr<Engine::IDocument> owned = std::move(document);
							try
							{
								Execution::StructureReport structure(options.reports, *context.console);
								structure.reportDocument(*owned, item.displayName);

								if (options.reports.any())
								{
									int pageCount = owned->getPageCount();
									for (int pageIndex = 0; pageIndex < pageCount; pageIndex++)
									{
										std::unique_ptr<Engine::IDocumentPage> page = owned->getPage(pageIndex);
										structure.reportPage(*page, pageIndex + 1, item.displayName);
									}
								}

								switch (format->kind)
								{
								case Planning::FormatKind::Text:
									convertToText(context, shared, item, format, *owned, options, combinedText);
									break;

								case Planning::FormatKind::DocPack:
									convertToDocPack(context, shared, item, *owned, options);
									break;

								default:
									if (combinedCanvas != nullptr)
										canvasConversion.appendTo(*combinedCanvas, *owned, options.pages.value_or(Planning::PageRange::all()), options, item.displayName);
									else if (shared.output.useStdout)
										canvasConversion.convertToStream(*owned, context.console->openStandardOutput(), item, *format, options);
									else
										canvasConversion.convert(*owned, item, *format, options);
									break;
								}
							}
							catch (const Runtime::CliError&)
							{
								throw;
							}
							catch (const std::exception& error)
							{
								throw Runtime::DocumentError(item.sourcePath, error.what());
							}
						}
					}
				}

				int ConvertCommand::execute(CommandContext& context) const
				{
					return CommandExecution::withSession(context, [&context, this]()
						{
							Configuration::SharedOptions& shared = context.sharedOptions;
							Configuration::OptionsValidator::validateConvert(shared, _options);

							Discovery::InputEnumerator enumerator(*context.fileSystem, *context.console);
							std::vector<Discovery::InputSource> inputs = enumerator.enumerate(shared.input);

							Planning::RunPlan plan = Planning::OutputPlanner::plan(shared, _options, inputs, *context.fileSystem);
							// --fingerprint never writes to plan.items' resolved targets ---
							// see CanvasConversion::makeCanvas --- so a real file already
							// sitting at one of those paths is not a conflict to report.
							if (!_options.fingerprint)
								Planning::OutputPlanner::verifyTargets(plan, shared, *context.fileSystem);

							Execution::Diagnostics diagnostics(shared.diagnostics, *context.console, &context.session->api(), context.clock);
							std::optional<Execution::EngineLog> log = Execution::EngineLog::for_(shared.diagnostics, *context.console);
							diagnostics.beginRun();

							// --combine is allowed for text formats too, which have no
							// canvas: their documents concatenate into one writer instead.
							// Keyed off the format's kind rather than the cardinality
							// alone, because reading canvasType for a text format has none
							// to read.
							const Planning::FormatSpec* combinedFormat = _options.format;
							bool combining = plan.cardinality == Planning::OutputCardinality::Combined && !shared.output.useStdout;
							if (combining)
								ensureOutputDirectory(context, Runtime::FilePath::toUtf8(Runtime::FilePath::of(*plan.combinedTarget).parent_path()));

							// A scope rather than the enclosing one, because the combined output has to
							// be closed before --view below: it is not a finished file until it is, a
							// PDF's trailer being written on close. Left to the enclosing scope, both of
							// these lived until the lambda returned, so --combine --view handed the
							// viewer a half-written document while the comment under it said the
							// opposite. The scope now says when they close, which is not something a
							// later edit can quietly move.
							int exitCode;
							{
								std::unique_ptr<Engine::IOutputCanvas> combinedCanvas;
								if (combining && combinedFormat->kind == Planning::FormatKind::Canvas)
								{
									combinedCanvas = context.session->api().makeCanvas(*plan.combinedTarget, *combinedFormat->canvasType,
										Execution::CanvasConversion::composeCanvasOptions(*combinedFormat, _options.canvasOptions, _options.noImages));
								}

								std::unique_ptr<std::ostream> combinedText;
								if (combining && combinedFormat->kind == Planning::FormatKind::Text)
									combinedText = context.fileSystem->createFile(*plan.combinedTarget);

								Runtime::BatchRunner batch(shared.batch, *context.console, context.interruption);
								Execution::CanvasConversion canvasConversion(*context.session, *context.console, &shared, context.fileSystem);
								int processed = 0;
								exitCode = batch.run<Planning::PlannedItem>(plan.items,
									[&](const Planning::PlannedItem& item)
									{
										processed++;
										diagnostics.beginDocument(item.displayName, processed, static_cast<int>(plan.items.size()));

										processDocument(context, shared, item, combinedCanvas.get(), combinedText.get(), canvasConversion, diagnostics,
											log.has_value() ? &*log : nullptr, _options);

										diagnostics.endDocument(item.displayName);
										diagnostics.checkDocumentTimeout(item.displayName);
										diagnostics.checkLimits();
										diagnostics.pauseIfRequested();
									});

								diagnostics.endRun(batch.successCount(), batch.failureCount());
							}

							// --view opens what was produced, once the run is over and the
							// file is closed. Only a run with one output has something
							// unambiguous to open, which is what the spec means by "single
							// output".
							if (_options.view && exitCode == Runtime::ExitCode::success)
							{
								if (context.openViewer)
									context.openViewer(singleOutputOf(plan), *context.console);
								else
									Runtime::Viewer::open(singleOutputOf(plan), *context.console);
							}

							return exitCode;
						});
				}

				std::string ConvertCommand::describePlan(CommandContext& context) const
				{
					Configuration::SharedOptions& shared = context.sharedOptions;
					Configuration::OptionsValidator::validateConvert(shared, _options);

					Discovery::InputEnumerator enumerator(*context.fileSystem, *context.console);
					std::vector<Discovery::InputSource> inputs = enumerator.enumerate(shared.input);

					Planning::RunPlan plan = Planning::OutputPlanner::plan(shared, _options, inputs, *context.fileSystem);
					if (!_options.fingerprint)
						Planning::OutputPlanner::verifyTargets(plan, shared, *context.fileSystem);

					Planning::PageRange pages = _options.pages.value_or(Planning::PageRange::all());

					std::vector<std::string> lines = {
						"command convert",
						"to " + _options.format->toCanonicalString(),
						PlanFormat::inputs(inputs),
						shared.toCanonicalString(),
						"pages " + pages.toCanonicalString(),
						plan.toCanonicalString()
					};
					return PlanFormat::joinLines(lines);
				}

				std::optional<std::string> ConvertCommand::singleOutputOf(const Planning::RunPlan& plan)
				{
					if (plan.cardinality == Planning::OutputCardinality::Combined)
						return plan.combinedTarget;

					return plan.items.size() == 1 ? plan.items[0].target : std::nullopt;
				}
			}
		}
	}
}
