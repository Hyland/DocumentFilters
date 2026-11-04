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
					/// requires, in this tool's own words.
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
					/// Splits the command line into each argument's values and the
					/// positional paths.
					/// </summary>
					/// <remarks>
					/// The whole of what a parsing library used to do here, and the reason none
					/// is needed: match a token to a row of the table, take its value from the
					/// token itself after an <c>=</c> or a <c>:</c> or else from the token after
					/// it, and put everything else in the path list.
					///
					/// Walks by the same rules as <see cref="validateTokens"/>, which has
					/// already run and rejected everything malformed --- so the lookup below
					/// is known to succeed, and anything <see cref="looksLikeOption"/> turns
					/// down here is a path by that function's own definition rather than by
					/// elimination. A value may itself look like a path, or like stdin
					/// (<c>--out-file -</c>); what follows a value-taking option is its value
					/// either way. Paths keep their positions relative to each other, which
					/// <c>--combine</c> depends on.
					///
					/// The map is keyed by descriptor address, not by name. **The reference
					/// keys a Dictionary by the descriptor object**, which in C# is reference
					/// equality for a class that does not override Equals; the pointer is the
					/// same thing spelled for C++. Every descriptor outlives the call --- they
					/// are the ArgumentDefinition objects the caller holds --- so the address
					/// is stable for as long as the map is.
					///
					/// A switch is recorded with an empty vector rather than being skipped:
					/// the entry is what says it was given at all, and <see cref="apply"/>
					/// reads presence, not count.
					/// </remarks>
					void partitionTokens(const std::vector<std::string>& args,
						const std::vector<const IArgumentDescriptor*>& descriptors,
						std::unordered_map<const IArgumentDescriptor*, std::vector<std::string>>& values,
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

							const IArgumentDescriptor* descriptor = byToken.at(name);
							std::vector<std::string>& given = values[descriptor];

							if (descriptor->arity() == ArgumentArity::NoValue)
							{
								i += 1;
								continue;
							}

							if (kvSeparator != std::string::npos)
							{ // the value is in the token itself
								given.push_back(token.substr(kvSeparator + 1));
								i += 1;
								continue;
							}

							given.push_back(args[i + 1]);
							i += 2;
						}
					}

					/// <summary>
					/// Runs the setter for every argument the user actually supplied, writing
					/// into <paramref name="target"/>.
					/// </summary>
					/// <remarks>
					/// This plus <see cref="partitionTokens"/> is the entire translation from
					/// command line text to the options model. Everything fallible in that
					/// translation happens inside the setters, in <c>ValueParsers</c>, which
					/// is shared with the other three language implementations.
					///
					/// Setters run in this table's order, which is what the loop below is: not
					/// the order the arguments were typed in, and it does not need to be.
					///
					/// "One value, last one wins" for <see cref="ArgumentArity::SingleValue"/>
					/// falls out of calling a plain assignment setter once per occurrence.
					/// </remarks>
					template <typename T>
					void apply(const std::vector<ArgumentDefinition<T>>& definitions, T& target,
						const std::unordered_map<const IArgumentDescriptor*, std::vector<std::string>>& values)
					{
						for (const ArgumentDefinition<T>& definition : definitions)
						{
							auto found = values.find(&definition);
							if (found == values.end())
								continue;

							if (definition.arity() == ArgumentArity::NoValue)
								definition.setter()(target, std::nullopt);
							else
								for (const std::string& value : found->second)
									definition.setter()(target, value);
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
					/// Two passes over the same descriptors: <see cref="validateTokens"/>
					/// decides whether the line is well formed and says so in this tool's own
					/// words, and <see cref="partitionTokens"/> then walks a line already
					/// known to be good.
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

						std::unordered_map<const IArgumentDescriptor*, std::vector<std::string>> values;
						std::vector<std::string> paths;
						partitionTokens(args, descriptors, values, paths);

						apply(sharedDefinitions, sharedOptions, values);
						apply(ownDefinitions, ownOptions, values);

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
