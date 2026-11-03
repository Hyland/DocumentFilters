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

#include "CommandLineApp.h"

#include "ArgumentTable.h"
#include "Commands/ConvertCommand.h"
#include "Commands/ExtractCommand.h"
#include "Commands/FormatsCommand.h"
#include "Commands/HelpCommand.h"
#include "Commands/IdentifyCommand.h"
#include "Commands/ListCommand.h"
#include "Commands/MetaCommand.h"
#include "Commands/OptionsCommand.h"
#include "Commands/VersionCommand.h"
#include "Planning/NameMatcher.h"
#include "ContractDump.h"
#include "Execution/DocumentFiltersSession.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Cli
			{
				namespace
				{
					namespace ErrorCodes = Runtime::ErrorCodes;
					using Runtime::UsageError;

					/// <summary>
					/// True for a token shaped like one of this tool's own options: a single
					/// dash plus exactly one character (a short form, e.g. <c>-o</c>), or a
					/// double dash plus more than one character (a long form, e.g.
					/// <c>--out-dir</c>).
					/// </summary>
					/// <remarks>
					/// Deliberately a grammar check, not "does this look like a plausible
					/// filename" --- almost any string, including a mistyped <c>--out-d</c>
					/// or an entirely unknown <c>--nonsense</c>, is also a syntactically valid
					/// relative path, so that test would accept exactly the malformed
					/// arguments this function exists to reject. It also excludes a bare
					/// <c>-</c>: that spelling means stdin or stdout, and must still work as a
					/// plain value.
					///
					/// ASCII-only, like every other classification in this tool: a
					/// locale-sensitive check would let a Turkish locale decide whether
					/// <c>-i</c> looks like an option.
					/// </remarks>
					bool looksLikeOption(const std::string& token)
					{
						if (token.size() < 2 || token[0] != '-')
							return false;

						auto isAsciiLetterOrDigit = [](char c) {
							return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
						};

						if (token[1] == '-')
							return token.size() > 2 && token[2] != '-';
						else
							return token.size() == 2 && isAsciiLetterOrDigit(token[1]);
					}

					/// <summary>Indexes a verb's arguments by every token that selects one.</summary>
					std::unordered_map<std::string, const IArgumentDescriptor*> descriptorsByToken(
						const std::vector<const IArgumentDescriptor*>& descriptors)
					{
						std::unordered_map<std::string, const IArgumentDescriptor*> byToken;
						for (const IArgumentDescriptor* descriptor : descriptors)
						{
							byToken["--" + descriptor->longName()] = descriptor;
							if (descriptor->shortName().has_value() && !descriptor->shortName()->empty())
								byToken["-" + *descriptor->shortName()] = descriptor;
						}
						return byToken;
					}

					/// <summary>
					/// Rejects an unknown argument, or one missing the value its arity
					/// requires, with this tool's own message rather than CLI11's.
					/// </summary>
					void validateTokens(const std::string& command, const std::vector<std::string>& args,
						const std::vector<const IArgumentDescriptor*>& descriptors)
					{
						std::unordered_map<std::string, const IArgumentDescriptor*> byToken = descriptorsByToken(descriptors);

						size_t i = 0;
						while (i < args.size())
						{
							const std::string& token = args[i];
							size_t kvSeparator = token.find_first_of("=:");
							std::string name = kvSeparator == std::string::npos ? token : token.substr(0, kvSeparator);

							if (!looksLikeOption(name))
							{ // a positional path, whatever it looks like inside
								i += 1;
								continue;
							}

							auto found = byToken.find(name);
							if (found == byToken.end())
								throw UsageError(ErrorCodes::unknownArgument,
									"unknown argument '" + name + "'; run 'docfilters help " + command + "' to list them");

							const IArgumentDescriptor& descriptor = *found->second;

							// A switch takes no value, so an attached one is a mistake and not a
							// value to ignore. Accepting it meant --overwrite=false enabled the
							// flag and then left "false" behind as a positional, which the walk
							// read as an input path: the opposite of what was typed, reported as
							// a missing file.
							if (descriptor.arity() == ArgumentArity::NoValue && kvSeparator != std::string::npos)
								throw UsageError(ErrorCodes::badValue,
									"--" + descriptor.longName() + " takes no value");

							if (descriptor.arity() == ArgumentArity::NoValue || kvSeparator != std::string::npos)
							{ // we don't need a value or we already have one in the token itself
								i += 1;
								continue;
							}

							if (i + 1 >= args.size() || looksLikeOption(args[i + 1])) // it's the last argument or the next one is another option
								throw UsageError(ErrorCodes::missingValue, "--" + descriptor.longName() + " requires a value");

							i += 2;
						}
					}

					/// <summary>
					/// Splits the command line into the tokens CLI11 is to parse and the
					/// positional paths it must never see.
					/// </summary>
					/// <remarks>
					/// Walks by the same rules as <see cref="validateTokens"/>, which has
					/// already run and rejected everything malformed --- so the lookup below
					/// is known to succeed, and anything <see cref="looksLikeOption"/> turns
					/// down here is a path by that function's own definition rather than by
					/// elimination. A value may itself look like a path, or like stdin
					/// (<c>--out-file -</c>); what follows a value-taking option is its value
					/// either way. Paths keep their positions relative to each other, which
					/// <c>--combine</c> depends on.
					/// </remarks>
					void partitionTokens(const std::vector<std::string>& args,
						const std::vector<const IArgumentDescriptor*>& descriptors, std::vector<std::string>& optionTokens,
						std::vector<std::string>& paths)
					{
						std::unordered_map<std::string, const IArgumentDescriptor*> byToken = descriptorsByToken(descriptors);

						size_t i = 0;
						while (i < args.size())
						{
							const std::string& token = args[i];
							size_t kvSeparator = token.find_first_of("=:");
							std::string name = kvSeparator == std::string::npos ? token : token.substr(0, kvSeparator);
							if (!looksLikeOption(name))
							{
								paths.push_back(token);
								i += 1;
								continue;
							}

							const IArgumentDescriptor& descriptor = *byToken.at(name);
							optionTokens.push_back(token);

							if (descriptor.arity() == ArgumentArity::NoValue || kvSeparator != std::string::npos)
							{ // we don't need a value or we already have one in the token itself
								i += 1;
								continue;
							}

							optionTokens.push_back(args[i + 1]);
							i += 2;
						}
					}

					/// <summary>
					/// Splits every <c>name=value</c> or <c>name:value</c> token into two,
					/// so CLI11 --- which only understands <c>=</c> on a long option, and
					/// nothing on a short one --- sees the same "name, then its value" shape
					/// this tool accepts either way.
					/// </summary>
					std::vector<std::string> normalizeForCli11(const std::vector<std::string>& args)
					{
						std::vector<std::string> result;
						for (const std::string& token : args)
						{
							size_t separator = token.find_first_of("=:");
							std::string name = separator == std::string::npos ? token : token.substr(0, separator);

							if (separator == std::string::npos || !looksLikeOption(name))
							{
								result.push_back(token);
								continue;
							}

							result.push_back(name);
							result.push_back(token.substr(separator + 1));
						}
						return result;
					}

					/// <summary>McMaster's <c>-o|--out-dir</c> template, spelled the way CLI11 expects: <c>-o,--out-dir</c>.</summary>
					std::string toCli11Names(const std::string& templateSpec)
					{
						std::string result = templateSpec;
						std::replace(result.begin(), result.end(), '|', ',');
						return result;
					}

					/// <summary>
					/// Registers a table of arguments against a CLI11 command, remembering how
					/// to write each one into <paramref name="target"/> once parsing finishes.
					/// </summary>
					/// <remarks>
					/// Every value-taking argument is registered to accept any number of
					/// occurrences, regardless of its own <see cref="ArgumentArity"/> --- the
					/// same trick <c>CommandLineApp.cs</c> plays on McMaster's
					/// <c>CommandOptionType</c> --- so that replaying the collected values in
					/// occurrence order is what actually enforces "one value, last one wins"
					/// for <see cref="ArgumentArity::SingleValue"/>: a plain assignment setter
					/// naturally ends up holding the last one.
					///
					/// Deferred into <paramref name="appliers"/> rather than applied from
					/// inside CLI11's own callback: this is what lets every argument's setter
					/// run in this table's registration order rather than in whatever order
					/// CLI11 happens to fire callbacks, matching the C# reference exactly.
					/// </remarks>
					template <typename T>
					void registerArguments(::CLI::App& app, const std::vector<ArgumentDefinition<T>>& definitions, T& target,
						std::vector<std::function<void()>>& appliers)
					{
						for (const ArgumentDefinition<T>& definition : definitions)
						{
							std::string names = toCli11Names(definition.templateText());

							if (definition.arity() == ArgumentArity::NoValue)
							{
								auto given = std::make_shared<bool>(false);
								app.add_flag_function(
									names, [given](std::int64_t count) { *given = count > 0; }, definition.description());

								appliers.push_back([&target, &definition, given]()
									{
										if (*given)
											definition.setter()(target, std::nullopt);
									});
							}
							else
							{
								auto values = std::make_shared<std::vector<std::string>>();
								::CLI::callback_t callback = [values](const ::CLI::results_t& res)
								{
									for (const std::string& value : res)
										values->push_back(value);
									return true;
								};

								::CLI::Option* option = app.add_option(names, callback, definition.description(), false);
								option->multi_option_policy(::CLI::MultiOptionPolicy::TakeAll);

								appliers.push_back([&target, &definition, values]()
									{
										for (const std::string& value : *values)
											definition.setter()(target, value);
									});
							}
						}
					}

					/// <summary>
					/// Compiles <c>InputOptions::globPatterns</c> or <c>InputOptions::regexPatterns</c>
					/// into <c>InputOptions::matcher</c>.
					/// </summary>
					/// <remarks>
					/// A stage 2 compile step, not a validation rule: <c>OptionsValidator</c>
					/// has already rejected the case where both lists are populated by the
					/// time this runs, so at most one branch below ever does anything.
					/// </remarks>
					void buildNameMatcher(Configuration::SharedOptions& shared)
					{
						if (!shared.input.globPatterns.empty())
							shared.input.matcher = Planning::NameMatcher::fromGlobs(shared.input.globPatterns);
						else if (!shared.input.regexPatterns.empty())
							shared.input.matcher = Planning::NameMatcher::fromRegexes(shared.input.regexPatterns);
					}

					/// <summary>
					/// Compiles <c>extract</c>'s <c>--include</c> and <c>--exclude</c> globs into
					/// <c>ExtractOptions::subfileInclude</c> and <c>ExtractOptions::subfileExclude</c>.
					/// </summary>
					void buildSubfileMatchers(Configuration::ExtractOptions& options)
					{
						if (!options.includePatterns.empty())
							options.subfileInclude = Planning::NameMatcher::fromGlobs(options.includePatterns);

						if (!options.excludePatterns.empty())
							options.subfileExclude = Planning::NameMatcher::fromGlobs(options.excludePatterns);
					}

					/// <summary>
					/// Binds one processing verb's shared and own arguments, and every
					/// positional path, into <paramref name="sharedOptions"/> and
					/// <paramref name="ownOptions"/>.
					/// </summary>
					/// <remarks>
					/// Tokens are validated by hand, against the same descriptors that get
					/// registered with CLI11, before <c>CLI::App::parse</c> ever sees them ---
					/// see <see cref="validateTokens"/>.
					/// </remarks>
					template <typename TOptions>
					void bindVerb(const std::string& command, const std::vector<std::string>& args,
						const std::vector<ArgumentDefinition<Configuration::SharedOptions>>& sharedDefinitions,
						const std::vector<ArgumentDefinition<TOptions>>& ownDefinitions, Configuration::SharedOptions& sharedOptions,
						TOptions& ownOptions)
					{
						std::vector<const IArgumentDescriptor*> descriptors;
						for (const ArgumentDefinition<Configuration::SharedOptions>& definition : sharedDefinitions)
							descriptors.push_back(&definition);
						for (const ArgumentDefinition<TOptions>& definition : ownDefinitions)
							descriptors.push_back(&definition);

						validateTokens(command, args, descriptors);

						// The paths are taken out here rather than registered as a CLI11
						// positional, so that CLI11 never has to decide which tokens are
						// paths: looksLikeOption is the contract, and by it "-tsv", "--" and
						// "---weird" are paths, which CLI11 would read as a short-option
						// cluster, the end-of-options separator and a long option. Handing it
						// only the tokens this tool calls options leaves nothing to disagree
						// about --- the same split the C# reference now makes for McMaster.
						std::vector<std::string> optionTokens;
						std::vector<std::string> paths;
						partitionTokens(args, descriptors, optionTokens, paths);

						::CLI::App app{command};
						std::vector<std::function<void()>> appliers;
						registerArguments(app, sharedDefinitions, sharedOptions, appliers);
						registerArguments(app, ownDefinitions, ownOptions, appliers);

						std::vector<std::string> reversed = normalizeForCli11(optionTokens);
						std::reverse(reversed.begin(), reversed.end()); // CLI::App::parse's vector overload consumes from the back
						app.parse(reversed);

						for (const std::function<void()>& applier : appliers)
							applier();

						for (const std::string& path : paths)
							sharedOptions.input.paths.push_back(path);
					}

					/// <summary><c>help [command] [--json]</c>. No engine, no shared options, so it is bound by hand.</summary>
					std::unique_ptr<Commands::ICommand> bindHelp(const std::vector<std::string>& args)
					{
						std::optional<std::string> topic;
						bool json = false;
						// Commands, not allVerbs. 'help' and 'version' are verbs with no
						// argument surface of their own --- they are deliberately absent
						// from commands(), which is what drives sharedFor and the contract
						// dump. Accepting them here sent an undescribable topic to
						// describeCommand, which throws std::invalid_argument; that is not a
						// CliError, so 'docfilters help help' died in std::terminate rather
						// than reporting anything.
						const std::vector<std::string>& commands = ArgumentTable::commands();
						const std::vector<std::string>& verbs = ArgumentTable::allVerbs();

						for (const std::string& arg : args)
						{
							if (arg == "--json")
								json = true;
							else if (std::find(commands.begin(), commands.end(), arg) != commands.end())
							{
								if (!topic.has_value())
									topic = arg;
								else
									throw UsageError(ErrorCodes::mutuallyExclusive, "multiple commands cannot be used together for help");
							}
							else if (std::find(verbs.begin(), verbs.end(), arg) != verbs.end())
							{
								throw UsageError(ErrorCodes::badValue,
									"'" + arg + "' has no arguments of its own; run 'docfilters help' to list the commands");
							}
							else
								throw UsageError(
									ErrorCodes::unknownArgument, "unknown argument '" + arg + "'; run 'docfilters help' to list them");
						}

						return std::make_unique<Commands::HelpCommand>(topic, json);
					}
				}

				int CommandLineApp::run(const std::vector<std::string>& args)
				{
					BoundCommandLine bound = bind(args);

					if (bound.sharedOptions().printContract)
					{
						_console.writeLine(ContractDump::render());
						return Runtime::ExitCode::success;
					}

					Commands::CommandContext context;
					context.sharedOptions = bound.sharedOptions();
					context.console = &_console;
					context.fileSystem = &_fileSystem;
					context.interruption = _interruption;
					context.clock = _clock;
					context.openViewer = _openViewer;

					// nullopt/nullptr leaves CommandExecution to create --- and dispose ---
					// the real engine on first use, which is what a real run does.
					std::optional<Execution::DocumentFiltersSession> session;
					if (_engine != nullptr)
					{
						session.emplace(Execution::DocumentFiltersSession::forEngine(*_engine));
						context.session = &*session;
					}

					if (bound.sharedOptions().dryRun)
					{
						_console.writeLine(bound.command().describePlan(context));
						return Runtime::ExitCode::success;
					}

					return bound.command().execute(context);
				}

				BoundCommandLine CommandLineApp::bind(const std::vector<std::string>& args)
				{
					if (args.empty())
						throw UsageError(ErrorCodes::unknownCommand, "no command given; run 'docfilters help' to list them");

					std::string verb = args[0];
					std::vector<std::string> rest(args.begin() + 1, args.end());

					if (verb == "version")
						return BoundCommandLine(std::make_unique<Commands::VersionCommand>(), Configuration::SharedOptions());

					if (verb == "help")
						return BoundCommandLine(bindHelp(rest), Configuration::SharedOptions());

					const std::vector<std::string>& commands = ArgumentTable::commands();
					if (std::find(commands.begin(), commands.end(), verb) == commands.end())
						throw UsageError(
							ErrorCodes::unknownCommand, "unknown command '" + verb + "'; run 'docfilters help' to list them");

					Configuration::SharedOptions shared;
					std::vector<ArgumentDefinition<Configuration::SharedOptions>> sharedDefinitions = ArgumentTable::sharedFor(verb);
					std::unique_ptr<Commands::ICommand> command;

					if (verb == "convert")
					{
						Configuration::ConvertOptions options;
						bindVerb(verb, rest, sharedDefinitions, ArgumentTable::convert(), shared, options);
						buildNameMatcher(shared);
						command = std::make_unique<Commands::ConvertCommand>(std::move(options));
					}
					else if (verb == "extract")
					{
						Configuration::ExtractOptions options;
						bindVerb(verb, rest, sharedDefinitions, ArgumentTable::extract(), shared, options);
						buildNameMatcher(shared);
						buildSubfileMatchers(options);
						command = std::make_unique<Commands::ExtractCommand>(std::move(options));
					}
					else if (verb == "list")
					{
						Configuration::ListOptions options;
						bindVerb(verb, rest, sharedDefinitions, ArgumentTable::list(), shared, options);
						buildNameMatcher(shared);
						command = std::make_unique<Commands::ListCommand>(std::move(options));
					}
					else if (verb == "identify")
					{
						Configuration::IdentifyOptions options;
						bindVerb(verb, rest, sharedDefinitions, ArgumentTable::identify(), shared, options);
						buildNameMatcher(shared);
						command = std::make_unique<Commands::IdentifyCommand>(std::move(options));
					}
					else if (verb == "meta")
					{
						Configuration::MetaOptions options;
						bindVerb(verb, rest, sharedDefinitions, ArgumentTable::meta(), shared, options);
						buildNameMatcher(shared);
						command = std::make_unique<Commands::MetaCommand>(std::move(options));
					}
					else if (verb == "formats")
					{
						Configuration::FormatsOptions options;
						bindVerb(verb, rest, sharedDefinitions, ArgumentTable::formats(), shared, options);
						command = std::make_unique<Commands::FormatsCommand>(std::move(options));
					}
					else if (verb == "options")
					{
						Configuration::OptionCatalogOptions options;
						bindVerb(verb, rest, sharedDefinitions, ArgumentTable::optionCatalog(), shared, options);
						command = std::make_unique<Commands::OptionsCommand>(std::move(options));
					}
					else
					{
						throw std::logic_error("'" + verb + "' is in ArgumentTable::commands() but bind() does not know it");
					}

					return BoundCommandLine(std::move(command), std::move(shared));
				}
			}
		}
	}
}
