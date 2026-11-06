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

from typing import Dict, List, Optional, Tuple

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
    Binds a command line against ArgumentTable and dispatches to the chosen command.

    The whole of the stage 1 to stage 2 boundary, and the only class that touches argv at
    all.

    There is no parsing library behind it. There was: this port was built on argparse, the
    reference on McMaster.Extensions.CommandLineUtils, and the other two on picocli and
    CLI11, with this class as the wrapper that hid the differences. The four disagreed on
    enough of the surface -- which tokens are options, what a repeated option means,
    whether a value may look like a path, what an unknown argument costs -- that every one
    of those decisions ended up here, hand written and shared, with the library left
    holding only the matching of a token to a row of the table. Holding four libraries in
    that role bought nothing, so the matching is written out too. See _partition_tokens,
    which is all of it.

    argparse cost nothing to keep, being the standard library, and was dropped with the
    other three anyway: the point of the change was that four implementations stopped
    having four different things to go wrong, and keeping one of them would have left the
    reference and this port needing separate explanations again.

    What that means for a reader who came for the API: this class is plumbing, and
    deliberately dull. execution/ is the sample.
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
            _bind_verb(verb, rest, shared_definitions, ArgumentTable.convert(),
                       shared, convert_options)
            _build_name_matcher(shared)
            command: ICommand = ConvertCommand(convert_options)
        elif verb == "extract":
            extract_options = ExtractOptions()
            _bind_verb(verb, rest, shared_definitions, ArgumentTable.extract(),
                       shared, extract_options)
            _build_name_matcher(shared)
            _build_subfile_matchers(extract_options)
            command = ExtractCommand(extract_options)
        elif verb == "list":
            list_options = ListOptions()
            _bind_verb(verb, rest, shared_definitions, ArgumentTable.list(),
                       shared, list_options)
            _build_name_matcher(shared)
            command = ListCommand(list_options)
        elif verb == "identify":
            identify_options = IdentifyOptions()
            _bind_verb(verb, rest, shared_definitions, ArgumentTable.identify(),
                       shared, identify_options)
            _build_name_matcher(shared)
            command = IdentifyCommand(identify_options)
        elif verb == "meta":
            meta_options = MetaOptions()
            _bind_verb(verb, rest, shared_definitions, ArgumentTable.meta(),
                       shared, meta_options)
            _build_name_matcher(shared)
            command = MetaCommand(meta_options)
        elif verb == "formats":
            formats_options = FormatsOptions()
            _bind_verb(verb, rest, shared_definitions, ArgumentTable.formats(),
                       shared, formats_options)
            command = FormatsCommand(formats_options)
        elif verb == "options":
            catalog_options = OptionCatalogOptions()
            _bind_verb(verb, rest, shared_definitions, ArgumentTable.option_catalog(),
                       shared, catalog_options)
            command = OptionsCommand(catalog_options)
        else:
            # RuntimeError for the C#'s InvalidOperationException: a programming error in
            # this file, not anything the user typed, so deliberately not a CliError.
            raise RuntimeError("'" + verb + "' is in ArgumentTable.COMMANDS but bind does "
                               "not know it")

        return BoundCommandLine(command, shared)


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


def _bind_verb(command: str, args: List[str],
               shared_definitions: List[ArgumentDefinition],
               own_definitions: List[ArgumentDefinition],
               shared_options: SharedOptions, own_options) -> None:
    """
    Binds one processing verb's shared and own arguments, and every positional path,
    into shared_options and own_options.

    Two passes over the same descriptors, as in the reference: _validate_tokens
    decides whether the line is well formed and says so in this tool's own words, and
    _partition_tokens then walks a line already known to be good, so every lookup in
    it succeeds and it has no error path of its own.

    See the class docstring for why neither pass delegates to a parsing library.
    """
    descriptors = list(shared_definitions) + list(own_definitions)
    _validate_tokens(command, args, descriptors)

    values, paths = _partition_tokens(args, descriptors)

    _apply(shared_definitions, shared_options, values)
    _apply(own_definitions, own_options, values)
    shared_options.input.paths.extend(paths)


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
    Rejects an unknown argument, or one missing the value its arity requires, in this
    tool's own words.

    Separate from _partition_tokens so that exactly one of the two has an error path:
    everything malformed is turned down here, and the walk that follows can assume a good
    line. The messages are contract, checked byte for byte against plan-corpus.txt in all
    four implementations.
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


def _partition_tokens(args: List[str], descriptors: List[ArgumentDefinition]
                      ) -> Tuple[Dict[ArgumentDefinition, List[str]], List[str]]:
    """
    Splits the command line into each argument's values and the positional paths.

    The whole of what a parsing library used to do here, and the reason none is imported:
    match a token to a row of the table, take its value from the token itself after an =
    or a : or else from the token after it, and put everything else in the path list.

    Walks by the same rules as _validate_tokens, which has already run and has already
    rejected everything malformed -- so every lookup here is known to succeed, and the
    absence of error handling below is a consequence of that order rather than an
    oversight. The walk is written twice rather than folded into one pass so that
    _validate_tokens stays a line-for-line port of the reference's own method, with the
    reference's own control flow, and this stays separable from it.

    **The reference keys a Dictionary by the descriptor object**, which in C# is reference
    equality for a class that does not override Equals. A dict keyed by the definition is
    the same thing here: ArgumentDefinition defines neither __eq__ nor __hash__, so it
    hashes by identity. Two definitions that happened to be spelled identically would
    still be two keys, which is what the Java port spells out with an IdentityHashMap.

    A switch is recorded with an empty list rather than being skipped: the entry is what
    says it was given at all, and _apply reads presence, not count. Dropping it would make
    --silent indistinguishable from an absent one.
    """
    by_token = _descriptors_by_token(descriptors)
    values: Dict[ArgumentDefinition, List[str]] = {}
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
        given = values.setdefault(descriptor, [])

        if descriptor.arity == ArgumentArity.NO_VALUE:
            index += 1
            continue

        if separator >= 0:
            # the value is in the token itself
            given.append(token[separator + 1:])
            index += 1
            continue

        given.append(args[index + 1])
        index += 2

    return values, paths


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
    path by this rule, in all four implementations.
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


def _apply(definitions: List[ArgumentDefinition], target,
           values: Dict[ArgumentDefinition, List[str]]) -> None:
    """
    Runs the setter for every argument the user actually supplied, writing into target.

    This plus _partition_tokens is the entire translation from command line text to the
    options model. Everything fallible in that translation happens inside the setters, in
    value_parsers, which is shared with the other three language implementations.

    A NO_VALUE argument calls its setter once with None; a SINGLE_VALUE argument once with
    the value; a MULTIPLE_VALUE argument once per occurrence, which is how --redact and -p
    accumulate.

    A module-level function, where the reference has a private method. It needs nothing
    from the instance, and the reference's own equivalent is static for the same reason;
    C# simply has no place to put a free function.

    No closure is built per argument, which removes the one trap this file used to carry:
    a lambda written inside `for definition in definitions` captures the variable rather
    than its value, so every deferred applier would have set whichever definition the loop
    finished on. The factory function that worked around it is gone with the deferral.

    Raises:
        UsageError: A value failed to parse.
    """
    for definition in definitions:
        given = values.get(definition)
        if given is None:
            continue

        if definition.arity == ArgumentArity.NO_VALUE:
            definition.setter(target, None)
        else:
            for value in given:
                definition.setter(target, value)


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


