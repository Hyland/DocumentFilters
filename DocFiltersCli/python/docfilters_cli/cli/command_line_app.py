"""
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
"""

import argparse
from typing import Callable, Dict, List, Optional, Tuple

from ..commands.convert_command import ConvertCommand
from ..commands.extract_command import ExtractCommand
from ..commands.formats_command import FormatsCommand
from ..commands.help_command import HelpCommand
from ..commands.i_command import CommandContext, ICommand
from ..commands.identify_command import IdentifyCommand
from ..commands.list_command import ListCommand
from ..commands.meta_command import MetaCommand
from ..commands.options_command import OptionsCommand
from ..commands.version_command import VersionCommand
from ..configuration.command_options import (ConvertOptions, ExtractOptions, FormatsOptions,
                                             IdentifyOptions, ListOptions, MetaOptions,
                                             OptionCatalogOptions)
from ..configuration.shared_options import SharedOptions
from ..planning.name_matcher import NameMatcher
from ..runtime import exit_code
from ..runtime.cli_error import ErrorCodes, UsageError
from .argument_definition import ArgumentArity, ArgumentDefinition
from .argument_table import ArgumentTable
from .bound_command_line import BoundCommandLine
from .contract_dump import ContractDump

# The characters that may join an option to its value in one token, so that --to=text and
# --to:text mean what --to text means. Both are the reference's, and both are exercised
# against the built C# binary rather than assumed.
_OPTION_VALUE_SEPARATORS = "=:"


class CommandLineApp:
    """
    Builds the argparse command line application from ArgumentTable and dispatches to the
    chosen command.

    This is the only class in the project that knows which CLI parsing library is in use,
    and the whole of the stage 1 to stage 2 boundary. The C#, Java and C++17 ports replace
    it with an equivalent over McMaster.Extensions.CommandLineUtils, picocli and CLI11;
    everything below it is shared logic.

    argparse rather than a third-party parser: it is the standard library, and it is what
    the published Document Filters Python samples already use. No new dependency.
    """

    def __init__(self, console, file_system, engine=None, interruption=None, clock=None,
                 open_viewer=None):
        """
        Args:
            engine: The Document Filters engine, or None to build the real one on first use.

        Passed in by hand, like the console and the filesystem, rather than resolved from
        anywhere: that is the rule the whole sample follows, and it is what lets a test
        drive a real command line from argv to exit code without a license or the native
        library.

        engine is left unannotated rather than typed IDocumentFiltersEngine. Nothing above
        the seam may import the DocumentFilters binding at module scope, and while the
        engine interface module does not itself reach the binding, leaving the annotation
        off keeps this file's imports to the ones it actually calls -- the same choice
        execution/engine_log.py makes for its console.
        """
        self._console = console
        self._file_system = file_system
        self._engine = engine
        self._interruption = interruption
        self._clock = clock
        self._open_viewer = open_viewer

        self._appliers: List[Callable[[], None]] = []
        """
        Deferred setter calls, one per registered argument, collected during
        _register_arguments and run by _apply_arguments once the command line has been
        parsed.

        Holding them as closures is what lets one list carry arguments that write into
        different targets -- SharedOptions for the shared options, a per-verb options
        object for the rest. The C# needs this to escape its own generics; Python does
        not, but the deferral is still what orders the setters correctly.
        """

    def run(self, args: List[str]) -> int:
        """
        Parses args and runs the selected command.

        Any CliError propagates rather than being caught here: program.py is the one place
        that turns an error into stderr text and an exit code, via program.report_error,
        and it needs the console bound to the run's own --silent/--verbose/--color
        settings -- which bind has not resolved yet when this method is entered.

        Returns:
            A value from runtime.exit_code.

        Raises:
            CliError: Unknown verb or argument, a missing value, or anything
                ICommand.execute itself throws.
        """
        bound = self.bind(args)

        if bound.shared_options.print_contract:
            self._console.write_line(ContractDump.render())
            return exit_code.SUCCESS

        context = CommandContext()
        context.shared_options = bound.shared_options
        context.console = self._console
        context.file_system = self._file_system
        context.interruption = self._interruption
        context.clock = self._clock
        context.open_viewer = self._open_viewer

        # None leaves command_execution to create -- and close -- the real engine on first
        # use, which is what a real run does.
        #
        # document_filters_session is imported here rather than at the top of the module,
        # for the reason commands/command_execution.py gives at its own deferred import of
        # the same module: --print-plan, --print-contract, help and version must all work
        # in a clone with no native library, and none of them reaches the execution
        # package unless something drags it in at import time.
        context.session = None
        if self._engine is not None:
            from ..execution.document_filters_session import DocumentFiltersSession

            context.session = DocumentFiltersSession.for_engine(self._engine)

        if bound.shared_options.dry_run:
            self._console.write_line(bound.command.describe_plan(context))
            return exit_code.SUCCESS

        return bound.command.execute(context)

    def bind(self, args: List[str]) -> BoundCommandLine:
        """
        Selects the command and binds every argument on it, without executing anything.
        Split out so that --print-plan and the unit tests can reach stages 1 to 3 without
        stage 4.

        Raises:
            UsageError: Unknown verb or argument, a missing value, or a value that failed
                to parse.
        """
        if len(args) == 0:
            raise UsageError(ErrorCodes.UNKNOWN_COMMAND,
                             "no command given; run 'docfilters help' to list them")

        verb = args[0]
        rest = list(args[1:])

        if verb == "version":
            return BoundCommandLine(VersionCommand(), SharedOptions())

        if verb == "help":
            return BoundCommandLine(_bind_help(rest), SharedOptions())

        if verb not in ArgumentTable.COMMANDS:
            raise UsageError(ErrorCodes.UNKNOWN_COMMAND,
                             "unknown command '" + verb + "'; run 'docfilters help' to list them")

        shared = SharedOptions()
        shared_definitions = ArgumentTable.shared_for(verb)

        # The C# switch, as an if/elif chain: Python's match needs 3.10 and this port
        # targets 3.8. A dispatch dict was the other option and is rejected because each
        # arm does three different things -- build the options, bind, compile the matchers
        # -- so the table would be a table of closures that read no better than this.
        if verb == "convert":
            convert_options = ConvertOptions()
            self._bind_verb(verb, rest, shared_definitions, ArgumentTable.convert(),
                            shared, convert_options)
            _build_name_matcher(shared)
            command: ICommand = ConvertCommand(convert_options)
        elif verb == "extract":
            extract_options = ExtractOptions()
            self._bind_verb(verb, rest, shared_definitions, ArgumentTable.extract(),
                            shared, extract_options)
            _build_name_matcher(shared)
            _build_subfile_matchers(extract_options)
            command = ExtractCommand(extract_options)
        elif verb == "list":
            list_options = ListOptions()
            self._bind_verb(verb, rest, shared_definitions, ArgumentTable.list(),
                            shared, list_options)
            _build_name_matcher(shared)
            command = ListCommand(list_options)
        elif verb == "identify":
            identify_options = IdentifyOptions()
            self._bind_verb(verb, rest, shared_definitions, ArgumentTable.identify(),
                            shared, identify_options)
            _build_name_matcher(shared)
            command = IdentifyCommand(identify_options)
        elif verb == "meta":
            meta_options = MetaOptions()
            self._bind_verb(verb, rest, shared_definitions, ArgumentTable.meta(),
                            shared, meta_options)
            _build_name_matcher(shared)
            command = MetaCommand(meta_options)
        elif verb == "formats":
            formats_options = FormatsOptions()
            self._bind_verb(verb, rest, shared_definitions, ArgumentTable.formats(),
                            shared, formats_options)
            command = FormatsCommand(formats_options)
        elif verb == "options":
            catalog_options = OptionCatalogOptions()
            self._bind_verb(verb, rest, shared_definitions, ArgumentTable.option_catalog(),
                            shared, catalog_options)
            command = OptionsCommand(catalog_options)
        else:
            # RuntimeError for the C#'s InvalidOperationException: a programming error in
            # this file, not anything the user typed, so deliberately not a CliError.
            raise RuntimeError("'" + verb + "' is in ArgumentTable.COMMANDS but bind does "
                               "not know it")

        return BoundCommandLine(command, shared)

    def _bind_verb(self, command: str, args: List[str],
                   shared_definitions: List[ArgumentDefinition],
                   own_definitions: List[ArgumentDefinition],
                   shared_options: SharedOptions, own_options) -> None:
        """
        Binds one processing verb's shared and own arguments, and every positional path,
        into shared_options and own_options.

        Tokens are validated by hand, against the same descriptors that get registered
        with argparse, before parse_args ever sees them. The reference does this because
        two of McMaster's default behaviours do not match this tool's contract closely
        enough to rely on; the same hand-validation is what lets argparse be used here
        without inheriting its own answers to the same questions, and it is shared logic
        rather than a Python workaround.
        """
        descriptors = list(shared_definitions) + list(own_definitions)
        _validate_tokens(command, args, descriptors)

        # The positional paths and the options are separated here rather than by argparse.
        # Three reasons, each measured against the built C# binary rather than assumed:
        #
        #   * argparse cannot collect positionals that are interspersed with options.
        #     `convert a.pdf --to text b.pdf` binds both paths in the reference -- its plan
        #     dump reads `inputs a.pdf, b.pdf` -- and a `nargs='*'` positional would drop
        #     b.pdf. Any tool invoked on a shell glob meets this on the first run.
        #   * argparse has its own opinion about several tokens _looks_like_option has
        #     already classified as paths -- it consumes a bare `--` as the end-of-options
        #     separator, and would read `-ab` as a cluster of short options. The contract
        #     is _looks_like_option, so a path is what each of them has to stay.
        #   * A value may itself look like an option -- `--out-file -` means stdout -- and
        #     normalising every occurrence to one `--long=value` token settles that here
        #     instead of relying on argparse's own precedence.
        option_tokens, paths = _partition_tokens(args, descriptors)

        parser = _Parser(prog=command, add_help=False, allow_abbrev=False)

        self._appliers = []
        self._register_arguments(parser, shared_definitions, shared_options)
        self._register_arguments(parser, own_definitions, own_options)

        namespace = parser.parse_args(option_tokens)

        self._apply_arguments(namespace)
        shared_options.input.paths.extend(paths)

    def _register_arguments(self, parser: argparse.ArgumentParser,
                            definitions: List[ArgumentDefinition], target) -> None:
        """
        Registers a table of arguments against an argparse parser, remembering how to
        write each one into target after parsing.

        This plus _apply_arguments is the entire translation from command line text to the
        options model. Everything fallible in that translation happens inside the setters,
        in value_parsers, which is shared with the other three language implementations.

        Every value-taking argument is registered with action='append' regardless of its
        own arity -- see to_argparse_action -- so _apply_arguments is what actually
        enforces "one value, last one wins" for SINGLE_VALUE, simply by calling the setter
        once per occurrence in the order given: a plain assignment setter naturally ends
        up holding the last one.
        """
        for index, definition in enumerate(definitions):
            forms = definition.template.split('|')

            # An explicit dest, rather than letting argparse derive one from the option
            # name. Its own rule -- strip the dashes and turn the rest into underscores --
            # would map --out-dir to out_dir, which reads well right up until a future
            # argument's long name collides with another's or lands on a Python keyword.
            # The index is unique by construction and nothing ever reads these names
            # except the applier built directly below, so their spelling is private.
            #
            # id(target) is part of the name because this method is called twice against
            # one parser, once for the shared table and once for the verb's own, and the
            # two index ranges overlap.
            dest = "arg{0}_{1}".format(id(target), index)

            parser.add_argument(*forms, dest=dest, help=definition.description,
                                **CommandLineApp.to_argparse_action(definition.arity))

            self._appliers.append(_make_applier(definition, dest, target))

    def _apply_arguments(self, namespace: argparse.Namespace) -> None:
        """
        Runs the setter for every argument the user actually supplied.

        A NO_VALUE argument calls its setter once with None; a SINGLE_VALUE argument once
        with the value; a MULTIPLE_VALUE argument once per occurrence, which is how
        --redact and -p accumulate.

        Two arguments that write the same setting never race: a contradictory pair is
        rejected by options_validator rather than resolved by precedence. No precedence
        rule is possible here -- argparse records values per option, not the relative
        order of different options, and neither do all of McMaster, picocli and CLI11.

        Raises:
            UsageError: A value failed to parse.
        """
        for applier in self._appliers:
            applier(namespace)

    @staticmethod
    def to_argparse_action(arity: ArgumentArity) -> Dict[str, object]:
        """
        Translates our arity to the argparse keywords that implement it.

        SINGLE_VALUE maps to action='append', not argparse's default of storing one value:
        argparse's default silently keeps the last occurrence, which happens to be this
        tool's contract, but collecting every occurrence and letting _apply_arguments
        replay them in order says so explicitly and is what the other three ports do. The
        reference has the same line to write for the opposite reason -- McMaster's
        SingleValue throws if the option is given twice, where this tool's contract says
        the later one silently wins.

        A dict of keywords rather than the single enum member the C# returns, because
        argparse spells "takes no value" and "takes values" as different keyword
        combinations rather than as one value of one type.
        """
        if arity == ArgumentArity.NO_VALUE:
            # default=False, so _make_applier can ask whether the switch was given at all.
            # store_true also collapses repeats, which is McMaster's HasValue() exactly:
            # --overwrite --overwrite calls the setter once in both ports.
            return {'action': 'store_true', 'default': False}
        if arity in (ArgumentArity.SINGLE_VALUE, ArgumentArity.MULTIPLE_VALUE):
            return {'action': 'append', 'default': None}

        # The C#'s ArgumentOutOfRangeException. ValueError is Python's equivalent for an
        # argument whose value is outside the set a function accepts.
        raise ValueError("unknown arity: " + str(arity))

    def show_help(self, command: Optional[str]) -> None:
        """Prints top-level usage, or usage for one command."""
        self._console.write_line(ContractDump.render() if command is None
                                 else ContractDump.render_command(command))

    def show_version(self) -> None:
        """Prints the library version and build information."""
        context = CommandContext()
        context.shared_options = SharedOptions()
        context.console = self._console
        context.file_system = self._file_system

        VersionCommand().execute(context)


class _Parser(argparse.ArgumentParser):
    """
    An ArgumentParser that reports a problem the way the rest of this tool does.

    Not in the C#, and it exists because argparse's defaults are wrong for a library this
    tool only wants the mechanical half of. On a parse problem argparse writes usage text
    to stderr itself and calls sys.exit(2), which would bypass both seams the project is
    built on: runtime.i_console owns everything the user sees, and program.py owns the
    mapping from error to exit code. exit() is overridden as well as error(), because
    argparse calls it directly rather than always going through error().

    Reaching either of these means _validate_tokens and this parser disagreed about the
    same token, which is a bug in this file rather than anything the user typed -- every
    rejection the contract defines has already happened by now, and _partition_tokens
    hands over only tokens it has itself recognised. The code is UNKNOWN_ARGUMENT because
    that is the nearest of the codes the fixtures pin; a new one is deliberately not
    added, since ErrorCodes.all_codes is checked against the corpus in both directions
    and a code no case provokes fails the conformance suite.
    """

    def error(self, message: str) -> None:
        raise UsageError(ErrorCodes.UNKNOWN_ARGUMENT, message)

    def exit(self, status: int = 0, message: Optional[str] = None) -> None:
        raise UsageError(ErrorCodes.UNKNOWN_ARGUMENT,
                         message if message is not None else "could not parse the command line")


def _bind_help(args: List[str]) -> ICommand:
    """help [command] [--json]. No engine, no shared options, so it is bound by hand."""
    topic = None
    json = False
    for arg in args:
        if arg == "--json":
            json = True
        # COMMANDS, not ALL_VERBS. 'help' and 'version' are verbs with no argument
        # surface of their own -- they are deliberately absent from COMMANDS, which is
        # what drives shared_for and the contract dump. Accepting them here sent an
        # undescribable topic to the describer, which raises something that is not a
        # CliError, so 'docfilters help help' ended the process instead of reporting.
        elif arg in ArgumentTable.COMMANDS:
            if topic is None:
                topic = arg
            else:
                raise UsageError(ErrorCodes.MUTUALLY_EXCLUSIVE,
                                 "multiple commands cannot be used together for help")
        elif arg in ArgumentTable.ALL_VERBS:
            raise UsageError(ErrorCodes.BAD_VALUE,
                             "'" + arg + "' has no arguments of its own; run 'docfilters help' "
                             "to list the commands")
        else:
            raise UsageError(ErrorCodes.UNKNOWN_ARGUMENT,
                             "unknown argument '" + arg + "'; run 'docfilters help' to list them")
    return HelpCommand(topic, json)


def _build_name_matcher(shared: SharedOptions) -> None:
    """
    Compiles InputOptions.glob_patterns or InputOptions.regex_patterns into
    InputOptions.matcher.

    A stage 2 compile step, not a validation rule: options_validator has already rejected
    the case where both lists are populated by the time this runs, so at most one branch
    below ever does anything.

    Raises:
        UsageError: A pattern is malformed.
    """
    if len(shared.input.glob_patterns) > 0:
        shared.input.matcher = NameMatcher.from_globs(shared.input.glob_patterns)
    elif len(shared.input.regex_patterns) > 0:
        shared.input.matcher = NameMatcher.from_regexes(shared.input.regex_patterns)


def _build_subfile_matchers(options: ExtractOptions) -> None:
    """
    Compiles extract's --include and --exclude globs into ExtractOptions.subfile_include
    and ExtractOptions.subfile_exclude.

    Here rather than in the setter so a malformed pattern is reported before any archive is
    opened, the same way _build_name_matcher handles the path patterns. The two are
    complementary rather than contradictory, so unlike -g and --regex both may be given.

    Raises:
        UsageError: A pattern is malformed.
    """
    if len(options.include_patterns) > 0:
        options.subfile_include = NameMatcher.from_globs(options.include_patterns)

    if len(options.exclude_patterns) > 0:
        options.subfile_exclude = NameMatcher.from_globs(options.exclude_patterns)


def _validate_tokens(command: str, args: List[str],
                     descriptors: List[ArgumentDefinition]) -> None:
    """
    Rejects an unknown argument, or one missing the value its arity requires, with this
    tool's own message rather than argparse's.
    """
    by_token = _descriptors_by_token(descriptors)

    index = 0
    while index < len(args):
        token = args[index]
        separator = _index_of_any(token, _OPTION_VALUE_SEPARATORS)
        name = token if separator < 0 else token[:separator]
        if not _looks_like_option(name):
            # a positional path, whatever it looks like inside
            index += 1
            continue

        if name not in by_token:
            raise UsageError(ErrorCodes.UNKNOWN_ARGUMENT,
                             "unknown argument '" + name + "'; run 'docfilters help "
                             + command + "' to list them")

        descriptor = by_token[name]

        # A switch takes no value, so an attached one is a mistake and not a value to
        # ignore. Accepting it left the flag enabled and "false" behind as a positional,
        # so --overwrite=false turned overwriting on.
        if descriptor.arity == ArgumentArity.NO_VALUE and separator >= 0:
            raise UsageError(ErrorCodes.BAD_VALUE, "--" + descriptor.long_name + " takes no value")

        if descriptor.arity == ArgumentArity.NO_VALUE or separator >= 0:
            # we don't need a value or we already have one in the token itself
            index += 1
            continue

        # it's the last argument or the next one is another option
        if index + 1 >= len(args) or _looks_like_option(args[index + 1]):
            raise UsageError(ErrorCodes.MISSING_VALUE,
                             "--" + descriptor.long_name + " requires a value")

        index += 2


def _looks_like_option(token: str) -> bool:
    """
    True for a token shaped like one of this tool's own options: a single dash plus exactly
    one character (a short form, e.g. -o), or a double dash plus more than one character (a
    long form, e.g. --out-dir).

    Deliberately a grammar check, not "does this look like a plausible filename" -- almost
    any string, including a mistyped --out-d or an entirely unknown --nonsense, is also a
    syntactically valid relative path, so that test would accept exactly the malformed
    arguments this method exists to reject. It also excludes a bare -: that spelling means
    stdin or stdout, and must still work as a plain value.

    This is the contract. Every token that starts with a dash and lands on False here is a
    path by this rule. The reference used to hand such tokens to McMaster, which recognises
    a different set and threw an UnrecognizedCommandParsingException that nothing caught;
    it now partitions the paths out first, as this port always has, so the two agree.
    """
    if len(token) < 2 or token[0] != '-':
        return False

    if token[1] == '-':
        return len(token) > 2 and token[2] != '-'

    # char.IsAsciiLetterOrDigit, spelled out. Deliberately not str.isalnum, which is true
    # for every decimal digit and letter in Unicode -- '-²' and '-٠' would both
    # become options, and the reference would read them as paths.
    character = token[1]
    return len(token) == 2 and (('a' <= character <= 'z')
                                or ('A' <= character <= 'Z')
                                or ('0' <= character <= '9'))


def _partition_tokens(args: List[str],
                      descriptors: List[ArgumentDefinition]) -> Tuple[List[str], List[str]]:
    """
    Splits the command line into the option tokens argparse is given and the positional
    paths it never sees.

    The C# has the same method (PartitionTokens), for the same reason: McMaster misreads
    -tsv, -- and ---weird, which are paths by _looks_like_option. Python's version also
    normalises each option to one token, because argparse cannot collect interspersed
    positionals and has its own precedence for values that look like options. See
    _bind_verb.

    Walks by the same rules as _validate_tokens, which has already run and has already
    rejected everything malformed -- so every lookup here is known to succeed, and the
    absence of error handling below is a consequence of that order rather than an
    oversight. The walk is written twice rather than folded into one pass so that
    _validate_tokens stays a line-for-line port of the reference's own method, with the
    reference's own control flow, and this stays separable from it.

    Every option occurrence is normalised to a single token: --long=value for anything that
    takes a value, and the option's own spelling for a switch. The long form is used
    because argparse reads -p=secret as the value "=secret", where --password=secret is
    unambiguous; a switch keeps its own spelling because -x has no long form to use.
    """
    by_token = _descriptors_by_token(descriptors)
    option_tokens: List[str] = []
    paths: List[str] = []

    index = 0
    while index < len(args):
        token = args[index]
        separator = _index_of_any(token, _OPTION_VALUE_SEPARATORS)
        name = token if separator < 0 else token[:separator]
        if not _looks_like_option(name):
            paths.append(token)
            index += 1
            continue

        descriptor = by_token[name]
        if descriptor.arity == ArgumentArity.NO_VALUE:
            if separator >= 0:
                # --overwrite=true, which _validate_tokens waves through: it stops looking
                # once a token carries its own value, whatever the arity says. The
                # reference then hands it to McMaster, which throws a
                # CommandParsingException that nothing catches -- the built binary exits
                # 127 with a .NET stack trace. That is not a contract anyone wrote down,
                # no fixture covers it, and reproducing a crash is not parity worth
                # having, so it is reported as the usage error it is. Raised against the
                # reference; once it agrees, this comment is what says why it ever
                # differed. BAD_VALUE rather than a new code, because ErrorCodes.all_codes
                # is checked against the corpus in both directions.
                raise UsageError(ErrorCodes.BAD_VALUE,
                                 "--" + descriptor.long_name + " does not take a value")

            option_tokens.append(name)
            index += 1
            continue

        if separator >= 0:
            option_tokens.append("--" + descriptor.long_name + "=" + token[separator + 1:])
            index += 1
            continue

        option_tokens.append("--" + descriptor.long_name + "=" + args[index + 1])
        index += 2

    return option_tokens, paths


def _descriptors_by_token(descriptors: List[ArgumentDefinition]) -> Dict[str, ArgumentDefinition]:
    """
    Every spelling that names an argument, mapped to it.

    The C# builds this inline in ValidateTokens, which is the only place that needs it
    there; here _partition_tokens needs the same map, so it is a function rather than the
    same six lines twice.

    A plain dict, where the C# asks for StringComparer.Ordinal explicitly: Python string
    keys are already compared by code point, so the two agree. Nothing here folds case,
    and an option is not case-insensitive in any of the four ports.
    """
    by_token: Dict[str, ArgumentDefinition] = {}
    for descriptor in descriptors:
        by_token["--" + descriptor.long_name] = descriptor

        # Truthiness is string.IsNullOrEmpty exactly, for a string: None and '' are the
        # only falsy values short_name has.
        if descriptor.short_name:
            by_token["-" + descriptor.short_name] = descriptor
    return by_token


def _index_of_any(token: str, characters: str) -> int:
    """
    The position of the first of characters in token, or -1.

    C#'s string.IndexOfAny. Python's str.find takes one substring rather than a set, so
    the reference's one call is a loop; min() over two find() results was the alternative
    and needs the -1 results filtering out before it means anything.
    """
    for index, character in enumerate(token):
        if character in characters:
            return index
    return -1


def _make_applier(definition: ArgumentDefinition, dest: str,
                  target) -> Callable[[argparse.Namespace], None]:
    """
    Builds the deferred setter call for one argument.

    A factory rather than a closure written inline in the registration loop, which is the
    one thing this file could not port literally. A Python closure captures the variable
    and not its value, so a lambda built inside `for definition in definitions` would see
    whichever definition the loop finished on -- every applier would set the last
    argument, and every other argument on the command line would be silently ignored. C#
    has no such trap: its foreach variable is a fresh binding each iteration. Passing the
    three values as arguments to a function is what gives each applier its own.
    """
    def apply(namespace: argparse.Namespace) -> None:
        values = getattr(namespace, dest)

        if definition.arity == ArgumentArity.NO_VALUE:
            # values is the store_true flag here, so this is McMaster's HasValue().
            if values:
                definition.setter(target, None)
        elif values is not None:
            # `is not None` rather than truthiness: an append action leaves None when the
            # option never appeared, and a list is the only other thing it can hold.
            for value in values:
                definition.setter(target, value)

    return apply
