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

namespace Hyland.DocumentFilters.Cli.Cli
{
	/// <summary>
	/// Binds a command line against <see cref="ArgumentTable"/> and dispatches to the
	/// chosen command.
	/// </summary>
	/// <remarks>
	/// The whole of the stage 1 to stage 2 boundary, and the only class that touches
	/// argv at all.
	///
	/// There is no parsing library behind it. There was: this tool was built on
	/// McMaster.Extensions.CommandLineUtils, and the ports on argparse, picocli and
	/// CLI11, with this class as the wrapper that hid the differences. The four
	/// disagreed on enough of the surface --- which tokens are options, what a
	/// repeated option means, whether a value may look like a path, what an unknown
	/// argument costs --- that every one of those decisions ended up here, hand
	/// written and shared, with the library left holding only the matching of a
	/// token to a row of the table. Holding four libraries in that role bought
	/// nothing and cost a dependency each, so the matching is written out too. See
	/// <see cref="PartitionTokens"/>, which is all of it.
	///
	/// What that means for a reader who came for the API: this class is plumbing,
	/// and deliberately dull. <c>Execution/</c> is the sample.
	/// </remarks>
	public sealed class CommandLineApp
	{
		private readonly IConsole _console;
		private readonly Interruption _interruption;
		private readonly IFileSystem _fileSystem;
		private readonly IDocumentFiltersEngine _engine;

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
		/// Two passes over the same descriptors, and the split between them is the
		/// point. <see cref="ValidateTokens"/> decides whether the line is well formed
		/// and says so in this tool's own words; <see cref="PartitionTokens"/> then
		/// walks a line already known to be good, so every lookup in it succeeds and
		/// it has no error path of its own to get wrong. A parsing library could not
		/// be given that division: it validates as it parses, in its own vocabulary,
		/// and the message a user sees is the thing four implementations have to
		/// agree on down to the byte.
		/// </remarks>
		private static void BindVerb<TOptions>(
			string command,
			string[] args,
			IReadOnlyList<ArgumentDefinition<SharedOptions>> sharedDefinitions,
			IReadOnlyList<ArgumentDefinition<TOptions>> ownDefinitions,
			SharedOptions sharedOptions,
			TOptions ownOptions)
		{
			IReadOnlyList<IArgumentDescriptor> descriptors =
				sharedDefinitions.Cast<IArgumentDescriptor>().Concat(ownDefinitions).ToList();

			ValidateTokens(command, args, descriptors);

			var values = new Dictionary<IArgumentDescriptor, List<string>>();
			var paths = new List<string>();
			PartitionTokens(args, descriptors, values, paths);

			Apply(sharedDefinitions, sharedOptions, values);
			Apply(ownDefinitions, ownOptions, values);
			sharedOptions.Input.Paths.AddRange(paths);
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
		/// in this tool's own words.
		/// </summary>
		/// <remarks>
		/// Separate from <see cref="PartitionTokens"/> so that exactly one of the two
		/// has an error path: everything malformed is turned down here, and the walk
		/// that follows can assume a good line. The messages are contract, checked
		/// byte for byte against <c>plan-corpus.txt</c> in all four implementations.
		/// </remarks>
		private static void ValidateTokens(string command, IReadOnlyList<string> args, IReadOnlyList<IArgumentDescriptor> descriptors)
		{
			var byToken = DescriptorsByToken(descriptors);

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
				// positional. When this tool still had a parsing library under it, that
				// library then threw its own parse exception, which was not a CliError,
				// so `--overwrite=false` ended the process with an unhandled exception
				// rather than a usage error. The check stays because the message is
				// contract, not because there is still an exception to head off.
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
		/// Splits the command line into each argument's values and the positional
		/// paths.
		/// </summary>
		/// <remarks>
		/// The whole of what a parsing library used to do here, and the reason none is
		/// needed: match a token to a row of the table, take its value from the token
		/// itself after an <c>=</c> or a <c>:</c> or else from the token after it, and
		/// put everything else in the path list.
		///
		/// Walks by the same rules as <see cref="ValidateTokens"/>, which has already
		/// run and rejected everything malformed — so the lookup below is known to
		/// succeed, and anything <see cref="LooksLikeOption"/> turns down here is a
		/// path by that method's own definition rather than by elimination. A value
		/// may itself look like a path, or like stdin (<c>--out-file -</c>); what
		/// follows a value-taking option is its value either way. Paths keep their
		/// positions relative to each other, which <c>--combine</c> depends on.
		///
		/// A switch is recorded with an empty list rather than being skipped: the
		/// entry is what says it was given at all, and <see cref="Apply{T}"/> reads
		/// presence, not count. Dropping it would make <c>--silent</c> indistinguishable
		/// from an absent one.
		/// </remarks>
		private static void PartitionTokens(IReadOnlyList<string> args, IReadOnlyList<IArgumentDescriptor> descriptors,
											Dictionary<IArgumentDescriptor, List<string>> values, List<string> paths)
		{
			var byToken = DescriptorsByToken(descriptors);

			char[] optionValueSeparators = { '=', ':' };
			int i = 0;
			while (i < args.Count)
			{
				string token = args[i];
				int kvSeparator = token.IndexOfAny(optionValueSeparators);
				string name = kvSeparator < 0 ? token : token.Substring(0, kvSeparator);
				if (!LooksLikeOption(name))
				{
					paths.Add(token);
					i += 1;
					continue;
				}

				IArgumentDescriptor descriptor = byToken[name];
				if (!values.TryGetValue(descriptor, out List<string> given))
					values[descriptor] = given = new List<string>();

				if (descriptor.Arity == ArgumentArity.NoValue)
				{
					i += 1;
					continue;
				}

				if (kvSeparator >= 0)
				{   // the value is in the token itself
					given.Add(token.Substring(kvSeparator + 1));
					i += 1;
					continue;
				}

				given.Add(args[i + 1]);
				i += 2;
			}
		}

		/// <summary>Indexes a verb's arguments by every token that selects one.</summary>
		private static Dictionary<string, IArgumentDescriptor> DescriptorsByToken(IReadOnlyList<IArgumentDescriptor> descriptors)
		{
			var byToken = new Dictionary<string, IArgumentDescriptor>(StringComparer.Ordinal);
			foreach (IArgumentDescriptor descriptor in descriptors)
			{
				byToken["--" + descriptor.LongName] = descriptor;
				if (!string.IsNullOrEmpty(descriptor.ShortName))
					byToken["-" + descriptor.ShortName] = descriptor;
			}
			return byToken;
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
		/// Runs the setter for every argument the user actually supplied, writing into
		/// <paramref name="target"/>.
		/// </summary>
		/// <remarks>
		/// This plus <see cref="PartitionTokens"/> is the entire translation from
		/// command line text to the options model. Everything fallible in that
		/// translation happens inside the setters, in <c>ValueParsers</c>, which is
		/// shared with the other three language implementations.
		///
		/// A <see cref="ArgumentArity.NoValue"/> argument calls its setter once with null; a
		/// <see cref="ArgumentArity.SingleValue"/> argument once with the value; a
		/// <see cref="ArgumentArity.MultipleValue"/> argument once per occurrence, which is how
		/// <c>--redact</c> and <c>-p</c> accumulate.
		///
		/// "One value, last one wins" for <see cref="ArgumentArity.SingleValue"/> is
		/// not enforced anywhere: calling a plain assignment setter once per
		/// occurrence, in the order given, leaves it holding the last one.
		///
		/// Two arguments that write the same setting never race: a contradictory pair
		/// is rejected by <c>OptionsValidator</c> rather than resolved by precedence.
		/// No precedence rule is possible here, because <paramref name="values"/> is
		/// keyed by argument and records each one's occurrences, not the relative
		/// order of two different arguments. Recording that order would be easy now
		/// and is still not done: the contract says a contradiction is an error, and a
		/// tie-break would quietly answer a question the other three ports would each
		/// have to be taught to answer the same way.
		/// </remarks>
		/// <exception cref="UsageError">A value failed to parse.</exception>
		private static void Apply<T>(IReadOnlyList<ArgumentDefinition<T>> definitions, T target,
									 Dictionary<IArgumentDescriptor, List<string>> values)
		{
			foreach (ArgumentDefinition<T> definition in definitions)
			{
				if (!values.TryGetValue(definition, out List<string> given))
					continue;

				if (definition.Arity == ArgumentArity.NoValue)
					definition.Setter(target, null);
				else
					foreach (string value in given)
						definition.Setter(target, value);
			}
		}

	}
}
