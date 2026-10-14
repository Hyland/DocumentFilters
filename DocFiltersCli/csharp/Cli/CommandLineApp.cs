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

using Hyland.DocumentFilters.Cli.Commands;
using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Execution;
using Hyland.DocumentFilters.Cli.Runtime;
using McMaster.Extensions.CommandLineUtils;

// McMaster ships its own IConsole, and this is the one file that imports both.
// The alias keeps our name unqualified here; nothing else in the project sees the
// clash, because nothing else knows the parsing library exists.
using IConsole = Hyland.DocumentFilters.Cli.Runtime.IConsole;

namespace Hyland.DocumentFilters.Cli.Cli
{
	/// <summary>
	/// Builds the McMaster command line application from <see cref="ArgumentTable"/> and
	/// dispatches to the chosen command.
	/// </summary>
	/// <remarks>
	/// This is the only class in the project that knows which CLI parsing library is
	/// in use, and the whole of the stage 1 to stage 2 boundary. The Python, Java and
	/// C++17 ports replace it with an equivalent over argparse, picocli and CLI11;
	/// everything below it is shared logic.
	/// </remarks>
	public sealed class CommandLineApp
	{
		private readonly IConsole _console;
		private readonly Interruption _interruption;
		private readonly IFileSystem _fileSystem;
		private readonly IDocumentFiltersEngine _engine;

		/// <summary>
		/// Deferred setter calls, one per registered argument, collected during
		/// <see cref="RegisterArguments{T}"/> and run by <see cref="ApplyArguments"/> once the
		/// command line has been parsed.
		/// </summary>
		/// <remarks>
		/// Holding them as closures is what lets one non-generic list carry arguments that
		/// write into different targets — <c>SharedOptions</c> for the shared options, a
		/// per-verb options object for the rest.
		/// </remarks>
		private readonly List<Action> _appliers = new List<Action>();

		/// <param name="engine">
		/// The Document Filters engine, or null to build the real one on first use.
		/// </param>
		/// <remarks>
		/// Passed in by hand, like the console and the filesystem, rather than resolved
		/// from anywhere: that is the rule the whole sample follows, and it is what lets
		/// a test drive a real command line from argv to exit code without a license or
		/// the native library.
		/// </remarks>
		public CommandLineApp(IConsole console, IFileSystem fileSystem, IDocumentFiltersEngine engine = null,
							  Interruption interruption = null, Func<TimeSpan> clock = null, Action<string, IConsole> openViewer = null)
		{
			_console = console;
			_fileSystem = fileSystem;
			_engine = engine;
			_interruption = interruption;
			_clock = clock;
			_openViewer = openViewer;
		}

		private readonly Func<TimeSpan> _clock;

		private readonly Action<string, IConsole> _openViewer;

		/// <summary>
		/// Parses <paramref name="args"/> and runs the selected command.
		/// </summary>
		/// <remarks>
		/// Any <see cref="CliError"/> propagates rather than being caught here:
		/// <c>Program</c> is the one place that turns an error into stderr text and an
		/// exit code, via <c>Program.ReportError</c>, and it needs the console bound to
		/// the run's own <c>--silent</c>/<c>--verbose</c>/<c>--color</c> settings —
		/// which <see cref="Bind"/> has not resolved yet when this method is entered.
		/// </remarks>
		/// <returns>A value from <see cref="ExitCode"/>.</returns>
		/// <exception cref="CliError">Unknown verb or argument, a missing value, or anything <see cref="ICommand.Execute"/> itself throws.</exception>
		public int Run(string[] args)
		{
			BoundCommandLine bound = Bind(args);

			if (bound.SharedOptions.PrintContract)
			{
				_console.WriteLine(ContractDump.Render());
				return ExitCode.Success;
			}

			var context = new CommandContext
			{
				SharedOptions = bound.SharedOptions,
				Console = _console,
				FileSystem = _fileSystem,

				// Null leaves CommandExecution to create — and dispose — the real engine
				// on first use, which is what a real run does.
				Session = _engine == null ? null : DocumentFiltersSession.For(_engine),
				Interruption = _interruption,
				Clock = _clock,
				OpenViewer = _openViewer
			};

			if (bound.SharedOptions.DryRun)
			{
				_console.WriteLine(bound.Command.DescribePlan(context));
				return ExitCode.Success;
			}

			return bound.Command.Execute(context);
		}

		/// <summary>
		/// Selects the command and binds every argument on it, without executing anything.
		/// Split out so that <c>--print-plan</c> and the unit tests can reach stages 1
		/// to 3 without stage 4.
		/// </summary>
		/// <exception cref="UsageError">Unknown verb or argument, a missing value, or a value that failed to parse.</exception>
		public BoundCommandLine Bind(string[] args)
		{
			if (args.Length == 0)
				throw new UsageError(ErrorCodes.UnknownCommand, "no command given; run 'docfilters help' to list them");

			string verb = args[0];
			string[] rest = args.Skip(1).ToArray();

			if (verb == "version")
				return new BoundCommandLine(new VersionCommand(), new SharedOptions());

			if (verb == "help")
				return new BoundCommandLine(BindHelp(rest), new SharedOptions());

			if (!ArgumentTable.Commands.Contains(verb))
				throw new UsageError(ErrorCodes.UnknownCommand, $"unknown command '{verb}'; run 'docfilters help' to list them");

			var shared = new SharedOptions();
			IReadOnlyList<ArgumentDefinition<SharedOptions>> sharedDefinitions = ArgumentTable.SharedFor(verb);
			ICommand command;
			switch (verb)
			{
				case "convert":
				{
					var options = new ConvertOptions();
					BindVerb(verb, rest, sharedDefinitions, ArgumentTable.Convert(), shared, options);
					BuildNameMatcher(shared);
					command = new ConvertCommand(options);
					break;
				}
				case "extract":
				{
					var options = new ExtractOptions();
					BindVerb(verb, rest, sharedDefinitions, ArgumentTable.Extract(), shared, options);
					BuildNameMatcher(shared);
					BuildSubfileMatchers(options);
					command = new ExtractCommand(options);
					break;
				}
				case "list":
				{
					var options = new ListOptions();
					BindVerb(verb, rest, sharedDefinitions, ArgumentTable.List(), shared, options);
					BuildNameMatcher(shared);
					command = new ListCommand(options);
					break;
				}
				case "identify":
				{
					var options = new IdentifyOptions();
					BindVerb(verb, rest, sharedDefinitions, ArgumentTable.Identify(), shared, options);
					BuildNameMatcher(shared);
					command = new IdentifyCommand(options);
					break;
				}
				case "meta":
				{
					var options = new MetaOptions();
					BindVerb(verb, rest, sharedDefinitions, ArgumentTable.Meta(), shared, options);
					BuildNameMatcher(shared);
					command = new MetaCommand(options);
					break;
				}
				case "formats":
				{
					var options = new FormatsOptions();
					BindVerb(verb, rest, sharedDefinitions, ArgumentTable.Formats(), shared, options);
					command = new FormatsCommand(options);
					break;
				}
				case "options":
				{
					var options = new OptionCatalogOptions();
					BindVerb(verb, rest, sharedDefinitions, ArgumentTable.OptionCatalog(), shared, options);
					command = new OptionsCommand(options);
					break;
				}
				default:
					throw new InvalidOperationException($"'{verb}' is in ArgumentTable.Commands but Bind does not know it");
			}
			return new BoundCommandLine(command, shared);
		}

		/// <summary><c>help [command] [--json]</c>. No engine, no shared options, so it is bound by hand.</summary>
		private static ICommand BindHelp(string[] args)
		{
			string topic = null;
			bool json = false;
			foreach (string arg in args)
			{
				if (arg == "--json")
					json = true;
				// Commands, not AllVerbs. 'help' and 'version' are verbs with no argument
				// surface of their own — they are deliberately absent from Commands, which
				// is what drives SharedFor and the contract dump. Accepting them here sent
				// an undescribable topic to DescribeCommand, which throws something that is
				// not a CliError, so 'docfilters help help' terminated the process instead
				// of reporting anything.
				else if (ArgumentTable.Commands.Contains(arg))
				{
					if (topic == null)
	                    topic = arg;
					else
                        throw new UsageError(ErrorCodes.MutuallyExclusive, $"multiple commands cannot be used together for help");
                }
				else if (ArgumentTable.AllVerbs.Contains(arg))
				{
					throw new UsageError(ErrorCodes.BadValue,
						$"'{arg}' has no arguments of its own; run 'docfilters help' to list the commands");
				}
				else
					throw new UsageError(ErrorCodes.UnknownArgument, $"unknown argument '{arg}'; run 'docfilters help' to list them");
			}
			return new HelpCommand(topic, json);
		}

		/// <summary>
		/// Binds one processing verb's shared and own arguments, and every positional
		/// path, into <paramref name="shared"/> and <paramref name="ownOptions"/>.
		/// </summary>
		/// <remarks>
		/// Tokens are validated by hand, against the same descriptors that get
		/// registered with McMaster, before <see cref="CommandLineApplication.Execute(string[])"/>
		/// ever sees them. Two of McMaster's default behaviours do not match this
		/// tool's contract closely enough to rely on: it accepts a value-taking option
		/// a second time by throwing rather than letting the later one win, and it
		/// does not reliably reject "--to --out-dir out" — a value-taking option
		/// followed immediately by another option with nothing in between — the way
		/// argparse and CLI11 do. Validating first, with our own messages, sidesteps
		/// both rather than post-processing whatever McMaster's own exception says.
		/// </remarks>
		private void BindVerb<TOptions>(
			string command,
			string[] args,
			IReadOnlyList<ArgumentDefinition<SharedOptions>> sharedDefinitions,
			IReadOnlyList<ArgumentDefinition<TOptions>> ownDefinitions,
			SharedOptions sharedOptions,
			TOptions ownOptions)
		{
			ValidateTokens(command, args, sharedDefinitions.Cast<IArgumentDescriptor>().Concat(ownDefinitions).ToList());

			var app = new CommandLineApplication { Name = command }; // McMaster app

			_appliers.Clear();
			RegisterArguments(app, sharedDefinitions, sharedOptions);
			RegisterArguments(app, ownDefinitions, ownOptions);
			CommandArgument paths = app.Argument("paths", "Files or directories to process", multipleValues: true);

			app.Execute(args);

			ApplyArguments();
            sharedOptions.Input.Paths.AddRange(paths.Values);
		}

		/// <summary>
		/// Compiles <see cref="InputOptions.GlobPatterns"/> or
		/// <see cref="InputOptions.RegexPatterns"/> into <see cref="InputOptions.Matcher"/>.
		/// </summary>
		/// <remarks>
		/// A stage 2 compile step, not a validation rule: <see cref="OptionsValidator"/>
		/// has already rejected the case where both lists are populated by the time this
		/// runs, so at most one branch below ever does anything.
		/// </remarks>
		/// <exception cref="UsageError">A pattern is malformed.</exception>
		private static void BuildNameMatcher(SharedOptions shared)
		{
			if (shared.Input.GlobPatterns.Count > 0)
				shared.Input.Matcher = NameMatcher.FromGlobs(shared.Input.GlobPatterns);
			else if (shared.Input.RegexPatterns.Count > 0)
				shared.Input.Matcher = NameMatcher.FromRegexes(shared.Input.RegexPatterns);
		}

		/// <summary>
		/// Compiles <c>extract</c>'s <c>--include</c> and <c>--exclude</c> globs into
		/// <see cref="ExtractOptions.SubfileInclude"/> and
		/// <see cref="ExtractOptions.SubfileExclude"/>.
		/// </summary>
		/// <remarks>
		/// Here rather than in the setter so a malformed pattern is reported before any
		/// archive is opened, the same way <see cref="BuildNameMatcher"/> handles the
		/// path patterns. The two are complementary rather than contradictory, so unlike
		/// <c>-g</c> and <c>--regex</c> both may be given.
		/// </remarks>
		/// <exception cref="UsageError">A pattern is malformed.</exception>
		private static void BuildSubfileMatchers(ExtractOptions options)
		{
			if (options.IncludePatterns.Count > 0)
				options.SubfileInclude = NameMatcher.FromGlobs(options.IncludePatterns);

			if (options.ExcludePatterns.Count > 0)
				options.SubfileExclude = NameMatcher.FromGlobs(options.ExcludePatterns);
		}

		/// <summary>
		/// Rejects an unknown argument, or one missing the value its arity requires,
		/// with this tool's own message rather than McMaster's.
		/// </summary>
		private static void ValidateTokens(string command, IReadOnlyList<string> args, IReadOnlyList<IArgumentDescriptor> descriptors)
		{
			var byToken = new Dictionary<string, IArgumentDescriptor>(StringComparer.Ordinal);
			foreach (IArgumentDescriptor descriptor in descriptors)
			{
				byToken["--" + descriptor.LongName] = descriptor;
				if (!string.IsNullOrEmpty(descriptor.ShortName))
					byToken["-" + descriptor.ShortName] = descriptor;
			}

            char[] optionValueSeparators = { '=', ':' };
            int i = 0;
			while (i < args.Count)
			{
				string token = args[i];
				int kvSeparator = token.IndexOfAny(optionValueSeparators);
				string name = kvSeparator < 0 ? token : token.Substring(0, kvSeparator);
				if (!LooksLikeOption(name))
				{	// a positional path, whatever it looks like inside
					i += 1;
					continue;
				}

				if (!byToken.TryGetValue(name, out IArgumentDescriptor descriptor))
					throw new UsageError(ErrorCodes.UnknownArgument, $"unknown argument '{name}'; run 'docfilters help {command}' to list them");

				// A switch takes no value, so an attached one is a mistake and not a value
				// to ignore. Accepting it left the flag enabled and "false" behind as a
				// positional — and McMaster then threw a CommandParsingException, which is
				// not a CliError, so `--overwrite=false` ended the process with an
				// unhandled exception rather than a usage error.
				if (descriptor.Arity == ArgumentArity.NoValue && kvSeparator >= 0)
					throw new UsageError(ErrorCodes.BadValue, $"--{descriptor.LongName} takes no value");

				if (descriptor.Arity == ArgumentArity.NoValue || kvSeparator >= 0)
				{   // we don't need a value or we already have one in the token itself
					i += 1;
					continue;
				}

				if (i + 1 >= args.Count || LooksLikeOption(args[i + 1])) // it's the last argument or the next one is another option
					throw new UsageError(ErrorCodes.MissingValue, $"--{descriptor.LongName} requires a value");

				i += 2;
			}
		}

		/// <summary>
		/// True for a token shaped like one of this tool's own options: a single
		/// dash plus exactly one character (a short form, e.g. <c>-o</c>), or a
		/// double dash plus more than one character (a long form, e.g.
		/// <c>--out-dir</c>).
		/// </summary>
		/// <remarks>
		/// Deliberately a grammar check, not "does this look like a plausible
		/// filename" — almost any string, including a mistyped <c>--out-d</c> or an
		/// entirely unknown <c>--nonsense</c>, is also a syntactically valid
		/// relative path, so that test would accept exactly the malformed arguments
		/// this method exists to reject. It also excludes a bare <c>-</c>: that
		/// spelling means stdin or stdout, and must still work as a plain value.
		/// </remarks>
		private static bool LooksLikeOption(string token)
		{
			if (token.Length < 2 || token[0] != '-')
				return false;

			if (token[1] == '-')
				return token.Length > 2 && token[2] != '-';
			else
				return token.Length == 2 && char.IsAsciiLetterOrDigit(token[1]);
		}

		/// <summary>
		/// Registers a table of arguments against a McMaster command, remembering how to
		/// write each one into <paramref name="target"/> after parsing.
		/// </summary>
		/// <remarks>
		/// This plus <see cref="ApplyArguments"/> is the entire translation from command
		/// line text to the options model. Everything fallible in that translation
		/// happens inside the setters, in <c>ValueParsers</c>, which is shared with
		/// the other three language implementations.
		///
		/// Every value-taking argument is registered as McMaster's
		/// <see cref="CommandOptionType.MultipleValue"/> regardless of its own
		/// <see cref="ArgumentArity"/> — see <see cref="ToOptionType"/> — so
		/// <see cref="ApplyArguments"/> is what actually enforces "one value, last one
		/// wins" for <see cref="ArgumentArity.SingleValue"/>, simply by calling the
		/// setter once per occurrence in the order given: a plain assignment setter
		/// naturally ends up holding the last one.
		/// </remarks>
		private void RegisterArguments<T>(CommandLineApplication command, IReadOnlyList<ArgumentDefinition<T>> definitions, T target)
		{
			foreach (ArgumentDefinition<T> definition in definitions)
			{
				CommandOption option = command.Option(definition.Template, definition.Description, ToOptionType(definition.Arity));
				_appliers.Add(() =>
				{
					if (definition.Arity == ArgumentArity.NoValue)
					{
						if (option.HasValue())
							definition.Setter(target, null);
					}
					else
					{
						foreach (string value in option.Values)
							definition.Setter(target, value);
					}
				});
			}
		}

		/// <summary>
		/// Runs the setter for every argument the user actually supplied.
		/// </summary>
		/// <remarks>
		/// A <see cref="ArgumentArity.NoValue"/> argument calls its setter once with null; a
		/// <see cref="ArgumentArity.SingleValue"/> argument once with the value; a
		/// <see cref="ArgumentArity.MultipleValue"/> argument once per occurrence, which is how
		/// <c>--redact</c> and <c>-p</c> accumulate.
		///
		/// Two arguments that write the same setting never race: a contradictory pair
		/// is rejected by <c>OptionsValidator</c> rather than resolved by precedence.
		/// No precedence rule is possible here — McMaster records values per option,
		/// not the relative order of different options, and neither do all of argparse,
		/// picocli and CLI11.
		/// </remarks>
		/// <exception cref="UsageError">A value failed to parse.</exception>
		private void ApplyArguments()
		{
			foreach (Action applier in _appliers)
				applier();
		}

		/// <summary>Translates our arity to McMaster's equivalent.</summary>
		/// <remarks>
		/// <see cref="ArgumentArity.SingleValue"/> maps to McMaster's own
		/// <see cref="CommandOptionType.MultipleValue"/>, not its
		/// <c>SingleValue</c>: McMaster's <c>SingleValue</c> throws
		/// <c>CommandParsingException</c> if the option is given a second time, where
		/// this tool's contract says the later one silently wins. Collecting every
		/// occurrence and letting <see cref="ApplyArguments"/> replay them in order
		/// gets "last one wins" for free.
		/// </remarks>
		internal static CommandOptionType ToOptionType(ArgumentArity arity)
		{
			switch (arity)
			{
				case ArgumentArity.NoValue:
					return CommandOptionType.NoValue;
				case ArgumentArity.SingleValue:
				case ArgumentArity.MultipleValue:
					return CommandOptionType.MultipleValue;
				default:
					throw new ArgumentOutOfRangeException(nameof(arity));
			}
		}

		/// <summary>Prints top-level usage, or usage for one command.</summary>
		internal void ShowHelp(string command)
		{
			_console.WriteLine(command == null ? ContractDump.Render() : ContractDump.RenderCommand(command));
		}

		/// <summary>Prints the library version and build information.</summary>
		internal void ShowVersion()
		{
            new VersionCommand().Execute(new CommandContext
            {
                SharedOptions = new SharedOptions(),
                Console = _console,
                FileSystem = _fileSystem
            });
		}
	}
}
