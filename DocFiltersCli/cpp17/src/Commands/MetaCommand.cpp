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

#include "MetaCommand.h"

#include "CommandExecution.h"
#include "Configuration/OptionsValidator.h"
#include "Discovery/InputEnumerator.h"
#include "Engine/EngineException.h"
#include "Execution/DebugInput.h"
#include "Execution/Diagnostics.h"
#include "Execution/EngineLog.h"
#include "Execution/ExtractorOpenSettings.h"
#include "Execution/MetadataReport.h"
#include "Execution/PasswordProvider.h"
#include "PlanFormat.h"
#include "Planning/OutputPlanner.h"
#include "Planning/RunPlan.h"
#include "Runtime/BatchRunner.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"
#include "Runtime/JsonWriter.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				int MetaCommand::execute(CommandContext& context) const
				{
					return CommandExecution::withSession(context, [&context, this]()
						{
							Configuration::SharedOptions& shared = context.sharedOptions;
							Configuration::OptionsValidator::validate(shared);

							Discovery::InputEnumerator enumerator(*context.fileSystem, *context.console);
							std::vector<Discovery::InputSource> inputs = enumerator.enumerate(shared.input);

							Execution::PasswordProvider passwords(shared.passwords, *context.console, *context.fileSystem);
							Execution::Diagnostics diagnostics(shared.diagnostics, *context.console, &context.session->api(), context.clock);
							std::optional<Execution::EngineLog> log = Execution::EngineLog::for_(shared.diagnostics, *context.console);
							Runtime::BatchRunner batch(shared.batch, *context.console, context.interruption);
							std::optional<Runtime::JsonWriter> json;
							if (_options.json)
							{
								json.emplace();
								json->beginArray();
							}

							diagnostics.beginRun();

							int processed = 0;
							int exitCode = batch.run<Discovery::InputSource>(inputs,
								[&](const Discovery::InputSource& input)
								{
									processed++;
									diagnostics.beginDocument(input.path, processed, static_cast<int>(inputs.size()));
									passwords.reset();

									// Declared out here so it outlives the document that reads
									// from it.
									std::unique_ptr<std::istream> debugInput =
										Execution::DebugInput::for_(input.path, shared.diagnostics, *context.fileSystem, *context.console, input.path);

									{
										std::unique_ptr<Engine::IDocument> document;
										Execution::StopSignal stop = CommandExecution::watchForStop(diagnostics, context.interruption);
										try
										{
											Execution::ExtractorOpenSettings open = Execution::ExtractorOpenSettings::forMetadataOnly(shared);
											document = debugInput == nullptr
												? context.session->openDocument(input.path, open, passwords,
														[&stop]() { return stop.shouldStop(); },
														log.has_value() ? &*log : nullptr)
												: context.session->openDocument(*debugInput, input.path, open, passwords,
														[&stop]() { return stop.shouldStop(); },
														log.has_value() ? &*log : nullptr);
										}
										catch (const Engine::EngineException& error)
										{
											if (error.error() == Engine::EngineError::cancelled)
												stop.raise(input.path);
											throw Runtime::DocumentError(input.path, "could not open document");
										}
										catch (const Runtime::CliError&)
										{
											throw;
										}
										catch (const std::exception&)
										{
											throw Runtime::DocumentError(input.path, "could not open document");
										}

										try
										{
											if (_options.json)
												Execution::MetadataReport::writeJson(*document, input.path, *json);
											else
												Execution::MetadataReport::writeText(*document, input.path, *context.console);
										}
										catch (const Runtime::CliError&)
										{
											throw;
										}
										catch (const std::exception& error)
										{
											throw Runtime::DocumentError(input.path, error.what());
										}
									} // document closes here, matching the C# reference's `using` scope

									diagnostics.endDocument(input.path);
									diagnostics.checkDocumentTimeout(input.path);
									diagnostics.checkLimits();
									diagnostics.pauseIfRequested();
								});

							if (_options.json)
								context.console->write(json->endArray().toString());

							diagnostics.endRun(batch.successCount(), batch.failureCount());
							return exitCode;
						});
				}

				std::string MetaCommand::describePlan(CommandContext& context) const
				{
					Configuration::SharedOptions& shared = context.sharedOptions;
					Configuration::OptionsValidator::validate(shared);

					Discovery::InputEnumerator enumerator(*context.fileSystem, *context.console);
					std::vector<Discovery::InputSource> inputs = enumerator.enumerate(shared.input);

					Planning::OutputCardinality cardinality =
						Planning::OutputPlanner::resolveCardinality(shared, static_cast<int>(inputs.size()), false);

					std::vector<std::string> lines = {
						"command meta",
						PlanFormat::inputs(inputs),
						shared.toCanonicalString(false),
						"json " + PlanFormat::yesNo(_options.json),
						"cardinality " + Planning::cardinalityText(cardinality)
					};
					return PlanFormat::joinLines(lines);
				}
			}
		}
	}
}
