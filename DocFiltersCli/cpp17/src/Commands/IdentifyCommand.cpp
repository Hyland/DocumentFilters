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

#include "IdentifyCommand.h"

#include "CommandExecution.h"
#include "Configuration/OptionsValidator.h"
#include "Discovery/InputEnumerator.h"
#include "Execution/DebugInput.h"
#include "Execution/Diagnostics.h"
#include "PlanFormat.h"
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
				namespace
				{
					/// <remarks>
					/// The three bits doc2text itself reports: <c>IGR_FILE_SUPPORTS_TEXT</c>
					/// (1), <c>IGR_FILE_SUPPORTS_SUBFILES</c> (2), <c>IGR_FILE_SUPPORTS_HDHTML</c> (4).
					/// </remarks>
					std::string capabilitiesText(int capabilities)
					{
						std::vector<std::string> caps;
						if ((capabilities & 1) != 0)
							caps.push_back("text");
						if ((capabilities & 2) != 0)
							caps.push_back("subfiles");
						if ((capabilities & 4) != 0)
							caps.push_back("hd");

						if (caps.empty())
							return "none";

						std::string joined = caps[0];
						for (std::size_t i = 1; i < caps.size(); i++)
							joined += "," + caps[i];
						return joined;
					}
				}

				int IdentifyCommand::execute(CommandContext& context) const
				{
					return CommandExecution::withSession(context, [&context, this]()
						{
							Configuration::SharedOptions& shared = context.sharedOptions;
							Configuration::OptionsValidator::validate(shared);

							Discovery::InputEnumerator enumerator(*context.fileSystem, *context.console);
							std::vector<Discovery::InputSource> inputs = enumerator.enumerate(shared.input);

							Execution::Diagnostics diagnostics(shared.diagnostics, *context.console, &context.session->api(), context.clock);
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

									// identify never opens the document, so there is no open
									// callback and nothing to cancel or log --- but what the
									// engine makes of a truncated or buffered file is exactly
									// what the debug switches ask about.
									std::unique_ptr<std::istream> debugInput =
										Execution::DebugInput::for_(input.path, shared.diagnostics, *context.fileSystem, *context.console, input.path);

									Engine::EngineFileType identified;
									try
									{
										identified = debugInput == nullptr ? context.session->api().identifyFile(input.path)
																			: context.session->api().identifyFile(*debugInput, input.path);
									}
									catch (const Runtime::CliError&)
									{
										throw;
									}
									catch (const std::exception&)
									{
										throw Runtime::DocumentError(input.path, "could not identify document");
									}

									std::optional<std::string> capabilities =
										_options.capabilities ? std::optional<std::string>(capabilitiesText(identified.capabilities)) : std::nullopt;

									if (_options.json)
									{
										json->beginObject().property("path", input.path).property("format", identified.id);
										if (capabilities.has_value())
											json->property("capabilities", *capabilities);
										json->property("mime", identified.mimeType).property("name", identified.name).endObject();
									}
									else
									{
										std::string capabilitiesField = capabilities.has_value() ? "caps=" + *capabilities + " " : std::string();
										context.console->writeLine(
											input.path + ": format=" + std::to_string(identified.id) + " " + capabilitiesField
											+ "mime=" + identified.mimeType + " name=" + identified.name);
									}

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

				std::string IdentifyCommand::describePlan(CommandContext& context) const
				{
					Configuration::SharedOptions& shared = context.sharedOptions;
					Configuration::OptionsValidator::validate(shared);

					Discovery::InputEnumerator enumerator(*context.fileSystem, *context.console);
					std::vector<Discovery::InputSource> inputs = enumerator.enumerate(shared.input);

					std::vector<std::string> lines = {
						"command identify",
						PlanFormat::inputs(inputs),
						shared.toCanonicalString(false),
						"json " + PlanFormat::yesNo(_options.json),
						"capabilities " + PlanFormat::yesNo(_options.capabilities)
					};
					return PlanFormat::joinLines(lines);
				}
			}
		}
	}
}
