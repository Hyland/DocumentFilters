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

package com.hyland.documentfilters.cli.cli;

import com.hyland.documentfilters.cli.commands.CommandContext;
import com.hyland.documentfilters.cli.commands.ConvertCommand;
import com.hyland.documentfilters.cli.commands.ExtractCommand;
import com.hyland.documentfilters.cli.commands.FormatsCommand;
import com.hyland.documentfilters.cli.commands.HelpCommand;
import com.hyland.documentfilters.cli.commands.ICommand;
import com.hyland.documentfilters.cli.commands.IdentifyCommand;
import com.hyland.documentfilters.cli.commands.ListCommand;
import com.hyland.documentfilters.cli.commands.MetaCommand;
import com.hyland.documentfilters.cli.commands.OptionsCommand;
import com.hyland.documentfilters.cli.commands.VersionCommand;
import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.ExtractOptions;
import com.hyland.documentfilters.cli.configuration.FormatsOptions;
import com.hyland.documentfilters.cli.configuration.IdentifyOptions;
import com.hyland.documentfilters.cli.configuration.ListOptions;
import com.hyland.documentfilters.cli.configuration.MetaOptions;
import com.hyland.documentfilters.cli.configuration.OptionCatalogOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.engine.IDocumentFiltersEngine;
import com.hyland.documentfilters.cli.execution.DocumentFiltersSession;
import com.hyland.documentfilters.cli.planning.NameMatcher;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.Interruption;
import com.hyland.documentfilters.cli.runtime.UsageError;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;
import java.util.function.Supplier;

/**
 * Binds a command line against {@link ArgumentTable} and dispatches to the chosen
 * command.
 *
 * <p>The whole of the stage 1 to stage 2 boundary, and the only class that touches
 * {@code argv} at all.
 *
 * <p>There is no parsing library behind it, and this port never had one: the
 * published Java samples use nothing beyond the JDK and the binding, so picocli was
 * never available to it. The reference was built on
 * McMaster.Extensions.CommandLineUtils and the other two ports on argparse and
 * CLI11, with this class as the wrapper that hid the differences. The four disagreed
 * on enough of the surface --- which tokens are options, what a repeated option
 * means, whether a value may look like a path, what an unknown argument costs ---
 * that every one of those decisions ended up here, hand written and shared, leaving
 * each library holding only the matching of a token to a row of the table. That
 * matching is now written out in every implementation. See {@link #partitionTokens},
 * which is all of it.
 *
 * <p>Values are applied argument by argument in table order, every occurrence of
 * one argument in the order given. That is not the order they were typed in, and
 * does not need to be: a contradictory pair is rejected by {@code OptionsValidator}
 * rather than resolved by precedence.
 *
 * <p>What that means for a reader who came for the API: this class is plumbing, and
 * deliberately dull. {@code Execution/} is the sample.
 */
public final class CommandLineApp
{
	private static final String OPTION_VALUE_SEPARATORS = "=:";

	private final IConsole _console;
	private final IFileSystem _fileSystem;
	private final IDocumentFiltersEngine _engine;
	private final Interruption _interruption;
	private final Supplier<Duration> _clock;
	private final BiConsumer<String, IConsole> _openViewer;

	public CommandLineApp(IConsole console, IFileSystem fileSystem)
	{
		this(console, fileSystem, null, null, null, null);
	}

	/**
	 * @param engine null for the real engine, created on first use; a fake for tests.
	 * @param interruption null when nothing is watching for Ctrl+C.
	 * @param clock null for the real monotonic clock.
	 * @param openViewer null for the desktop's own handler.
	 */
	public CommandLineApp(IConsole console, IFileSystem fileSystem, IDocumentFiltersEngine engine, Interruption interruption,
		Supplier<Duration> clock, BiConsumer<String, IConsole> openViewer)
	{
		_console = console;
		_fileSystem = fileSystem;
		_engine = engine;
		_interruption = interruption;
		_clock = clock;
		_openViewer = openViewer;
	}

	/**
	 * Parses {@code args} and runs the selected command.
	 *
	 * <p>Any {@code CliError} propagates rather than being caught here:
	 * {@code Program} is the one place that turns an error into stderr text and an
	 * exit code.
	 *
	 * @return a value from {@link ExitCode}.
	 */
	public int run(String[] args)
	{
		BoundCommandLine bound = bind(args);

		if (bound.getSharedOptions().printContract)
		{
			_console.writeLine(ContractDump.render());
			return ExitCode.SUCCESS;
		}

		CommandContext context = new CommandContext();
		context.sharedOptions = bound.getSharedOptions();
		context.console = _console;
		context.fileSystem = _fileSystem;
		// Null leaves CommandExecution to create --- and close --- the real engine on
		// first use, which is what a real run does.
		context.session = _engine == null ? null : DocumentFiltersSession.forEngine(_engine);
		context.interruption = _interruption;
		context.clock = _clock;
		context.openViewer = _openViewer;

		if (bound.getSharedOptions().dryRun)
		{
			_console.writeLine(bound.getCommand().describePlan(context));
			return ExitCode.SUCCESS;
		}

		return bound.getCommand().execute(context);
	}

	/**
	 * Selects the command and binds every argument on it, without executing
	 * anything. Split out so that {@code --print-plan} and the unit tests can reach
	 * stages 1 to 3 without stage 4.
	 *
	 * @throws UsageError unknown verb or argument, a missing value, or a value that
	 *         failed to parse.
	 */
	public BoundCommandLine bind(String[] args)
	{
		if (args.length == 0)
			throw new UsageError(ErrorCodes.UNKNOWN_COMMAND, "no command given; run 'docfilters help' to list them");

		String verb = args[0];
		List<String> rest = Arrays.asList(args).subList(1, args.length);

		if (verb.equals("version"))
			return new BoundCommandLine(new VersionCommand(), new SharedOptions());

		if (verb.equals("help"))
			return new BoundCommandLine(bindHelp(rest), new SharedOptions());

		if (!ArgumentTable.COMMANDS.contains(verb))
			throw new UsageError(ErrorCodes.UNKNOWN_COMMAND, "unknown command '" + verb + "'; run 'docfilters help' to list them");

		SharedOptions shared = new SharedOptions();
		List<ArgumentDefinition<SharedOptions>> sharedDefinitions = ArgumentTable.sharedFor(verb);
		ICommand command;
		switch (verb)
		{
			case "convert":
			{
				ConvertOptions options = new ConvertOptions();
				bindVerb(verb, rest, sharedDefinitions, ArgumentTable.convert(), shared, options);
				buildNameMatcher(shared);
				command = new ConvertCommand(options);
				break;
			}
			case "extract":
			{
				ExtractOptions options = new ExtractOptions();
				bindVerb(verb, rest, sharedDefinitions, ArgumentTable.extract(), shared, options);
				buildNameMatcher(shared);
				buildSubfileMatchers(options);
				command = new ExtractCommand(options);
				break;
			}
			case "list":
			{
				ListOptions options = new ListOptions();
				bindVerb(verb, rest, sharedDefinitions, ArgumentTable.list(), shared, options);
				buildNameMatcher(shared);
				command = new ListCommand(options);
				break;
			}
			case "identify":
			{
				IdentifyOptions options = new IdentifyOptions();
				bindVerb(verb, rest, sharedDefinitions, ArgumentTable.identify(), shared, options);
				buildNameMatcher(shared);
				command = new IdentifyCommand(options);
				break;
			}
			case "meta":
			{
				MetaOptions options = new MetaOptions();
				bindVerb(verb, rest, sharedDefinitions, ArgumentTable.meta(), shared, options);
				buildNameMatcher(shared);
				command = new MetaCommand(options);
				break;
			}
			case "formats":
			{
				FormatsOptions options = new FormatsOptions();
				bindVerb(verb, rest, sharedDefinitions, ArgumentTable.formats(), shared, options);
				command = new FormatsCommand(options);
				break;
			}
			case "options":
			{
				OptionCatalogOptions options = new OptionCatalogOptions();
				bindVerb(verb, rest, sharedDefinitions, ArgumentTable.optionCatalog(), shared, options);
				command = new OptionsCommand(options);
				break;
			}
			default:
				throw new IllegalStateException("'" + verb + "' is in ArgumentTable.COMMANDS but bind does not know it");
		}
		return new BoundCommandLine(command, shared);
	}

	private static ICommand bindHelp(List<String> args)
	{
		String topic = null;
		boolean json = false;
		for (String arg : args)
		{
			if (arg.equals("--json"))
				json = true;
			// COMMANDS, not ALL_VERBS: 'help' and 'version' are verbs with no argument
			// surface of their own, so there is nothing to describe for them.
			else if (ArgumentTable.COMMANDS.contains(arg))
			{
				if (topic == null)
					topic = arg;
				else
					throw new UsageError(ErrorCodes.MUTUALLY_EXCLUSIVE, "multiple commands cannot be used together for help");
			}
			else if (ArgumentTable.ALL_VERBS.contains(arg))
			{
				throw new UsageError(ErrorCodes.BAD_VALUE,
					"'" + arg + "' has no arguments of its own; run 'docfilters help' to list the commands");
			}
			else
				throw new UsageError(ErrorCodes.UNKNOWN_ARGUMENT, "unknown argument '" + arg + "'; run 'docfilters help' to list them");
		}
		return new HelpCommand(topic, json);
	}

	/**
	 * Binds one processing verb's shared and own arguments, and every positional
	 * path, into {@code sharedOptions} and {@code ownOptions}.
	 */
	private static <T> void bindVerb(String command, List<String> args, List<ArgumentDefinition<SharedOptions>> sharedDefinitions,
		List<ArgumentDefinition<T>> ownDefinitions, SharedOptions sharedOptions, T ownOptions)
	{
		List<IArgumentDescriptor> descriptors = new ArrayList<>();
		descriptors.addAll(sharedDefinitions);
		descriptors.addAll(ownDefinitions);
		validateTokens(command, args, descriptors);

		Map<IArgumentDescriptor, List<String>> values = new IdentityHashMap<>();
		List<String> paths = new ArrayList<>();
		partitionTokens(args, descriptors, values, paths);

		apply(sharedDefinitions, sharedOptions, values);
		apply(ownDefinitions, ownOptions, values);
		sharedOptions.input.paths.addAll(paths);
	}

	private static void buildNameMatcher(SharedOptions shared)
	{
		if (!shared.input.globPatterns.isEmpty())
			shared.input.matcher = NameMatcher.fromGlobs(shared.input.globPatterns);
		else if (!shared.input.regexPatterns.isEmpty())
			shared.input.matcher = NameMatcher.fromRegexes(shared.input.regexPatterns);
	}

	private static void buildSubfileMatchers(ExtractOptions options)
	{
		if (!options.includePatterns.isEmpty())
			options.subfileInclude = NameMatcher.fromGlobs(options.includePatterns);

		if (!options.excludePatterns.isEmpty())
			options.subfileExclude = NameMatcher.fromGlobs(options.excludePatterns);
	}

	/**
	 * Rejects an unknown argument, a value given to a switch, or an argument missing
	 * the value its arity requires, with this tool's own message.
	 */
	static void validateTokens(String command, List<String> args, List<IArgumentDescriptor> descriptors)
	{
		Map<String, IArgumentDescriptor> byToken = descriptorsByToken(descriptors);

		int i = 0;
		while (i < args.size())
		{
			String token = args.get(i);
			int kvSeparator = indexOfAny(token, OPTION_VALUE_SEPARATORS);
			String name = kvSeparator < 0 ? token : token.substring(0, kvSeparator);
			if (!looksLikeOption(name))
			{
				// A positional path, whatever it looks like inside.
				i += 1;
				continue;
			}

			IArgumentDescriptor descriptor = byToken.get(name);
			if (descriptor == null)
				throw new UsageError(ErrorCodes.UNKNOWN_ARGUMENT,
					"unknown argument '" + name + "'; run 'docfilters help " + command + "' to list them");

			// A switch takes no value, so an attached one is a mistake and not a value
			// to ignore.
			if (descriptor.getArity() == ArgumentArity.NO_VALUE && kvSeparator >= 0)
				throw new UsageError(ErrorCodes.BAD_VALUE, "--" + descriptor.getLongName() + " takes no value");

			if (descriptor.getArity() == ArgumentArity.NO_VALUE || kvSeparator >= 0)
			{
				// No value needed, or it is already in the token itself.
				i += 1;
				continue;
			}

			if (i + 1 >= args.size() || looksLikeOption(args.get(i + 1)))
				throw new UsageError(ErrorCodes.MISSING_VALUE, "--" + descriptor.getLongName() + " requires a value");

			i += 2;
		}
	}

	/**
	 * Splits the command line into each argument's values and the positional paths.
	 *
	 * <p>Walks by the same rules as {@link #validateTokens}, which has already run
	 * and rejected everything malformed --- so every lookup here is known to
	 * succeed. A value may itself look like a path or like stdin ({@code --out-file -});
	 * what follows a value-taking option is its value either way, and paths may be
	 * interspersed with options anywhere on the line.
	 */
	private static void partitionTokens(List<String> args, List<IArgumentDescriptor> descriptors,
		Map<IArgumentDescriptor, List<String>> values, List<String> paths)
	{
		Map<String, IArgumentDescriptor> byToken = descriptorsByToken(descriptors);

		int i = 0;
		while (i < args.size())
		{
			String token = args.get(i);
			int kvSeparator = indexOfAny(token, OPTION_VALUE_SEPARATORS);
			String name = kvSeparator < 0 ? token : token.substring(0, kvSeparator);
			if (!looksLikeOption(name))
			{
				paths.add(token);
				i += 1;
				continue;
			}

			IArgumentDescriptor descriptor = byToken.get(name);
			List<String> given = values.computeIfAbsent(descriptor, key -> new ArrayList<>());
			if (descriptor.getArity() == ArgumentArity.NO_VALUE)
			{
				i += 1;
				continue;
			}

			if (kvSeparator >= 0)
			{
				given.add(token.substring(kvSeparator + 1));
				i += 1;
				continue;
			}

			given.add(args.get(i + 1));
			i += 2;
		}
	}

	private static Map<String, IArgumentDescriptor> descriptorsByToken(List<IArgumentDescriptor> descriptors)
	{
		Map<String, IArgumentDescriptor> byToken = new HashMap<>();
		for (IArgumentDescriptor descriptor : descriptors)
		{
			byToken.put("--" + descriptor.getLongName(), descriptor);
			String shortName = descriptor.getShortName();
			if (shortName != null && !shortName.isEmpty())
				byToken.put("-" + shortName, descriptor);
		}
		return byToken;
	}

	/**
	 * True for a token shaped like one of this tool's own options: a single dash and
	 * exactly one ASCII letter or digit ({@code -o}), or a double dash and more, not
	 * starting with a third dash ({@code --out-dir}).
	 *
	 * <p>A grammar check, not "could this be a filename": almost any string,
	 * including a mistyped {@code --out-d}, is also a valid relative path. It
	 * excludes a bare "-", which means stdin or stdout and must still work as a
	 * value. ASCII only, because {@code Character.isLetterOrDigit} would accept
	 * every letter and digit in Unicode, and the reference's check does not.
	 */
	static boolean looksLikeOption(String token)
	{
		if (token.length() < 2 || token.charAt(0) != '-')
			return false;

		if (token.charAt(1) == '-')
			return token.length() > 2 && token.charAt(2) != '-';

		char character = token.charAt(1);
		return token.length() == 2 && ((character >= 'a' && character <= 'z')
			|| (character >= 'A' && character <= 'Z')
			|| (character >= '0' && character <= '9'));
	}

	/**
	 * Runs the setter for every argument the user actually supplied: once with null
	 * for a switch, once per occurrence for anything that takes a value.
	 *
	 * @throws UsageError a value failed to parse.
	 */
	private static <T> void apply(List<ArgumentDefinition<T>> definitions, T target, Map<IArgumentDescriptor, List<String>> values)
	{
		for (ArgumentDefinition<T> definition : definitions)
		{
			List<String> given = values.get(definition);
			if (given == null)
				continue;

			if (definition.getArity() == ArgumentArity.NO_VALUE)
				definition.apply(target, null);
			else
			{
				for (String value : given)
					definition.apply(target, value);
			}
		}
	}

	private static int indexOfAny(String token, String characters)
	{
		for (int i = 0; i < token.length(); i++)
		{
			if (characters.indexOf(token.charAt(i)) >= 0)
				return i;
		}
		return -1;
	}
}
