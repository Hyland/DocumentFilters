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

import datetime
from typing import Optional, Set

from ..configuration.value_parsers import fold_token, trim

LEVEL_NONE = 0
LEVEL_CRITICAL = 1
LEVEL_ERROR = 2
LEVEL_WARNING = 3
LEVEL_INFORMATION = 4
LEVEL_DEBUG = 5
LEVEL_TRACE = 6


class EngineLog:
    """
    --log-level and --log-modules: what the engine's own readers log, and where it goes.

    The engine asks, per module, how much it should log, then hands back each message. Both
    arrive as actions on the open callback, so this only reaches documents being opened —
    there is no logging of a page loop, for the same reason there is no cancelling one.

    The filtering happens here rather than being asked of the engine, exactly as doc2text
    does it: the engine has no notion of which modules the user named, so the answer to
    "what level for module X" is where the --log-modules set is applied.

    Output goes to stderr, which doc2text also does. It shares that stream with the two
    contract error shapes, which is safe because nothing is logged unless --log-level was
    passed: a run that says nothing about logging produces the same stderr it always did.

    The level constants are module-level rather than class attributes, because the C#
    declares them as consts on the class only for want of a namespace to put them in.
    """

    def __init__(self, level: int, modules: Set[str], console):
        self._level = level
        self._modules = modules
        self._console = console

    @staticmethod
    def for_options(options, console) -> Optional['EngineLog']:
        """
        The log for these options, or None when nothing was asked for.

        None rather than a disabled instance, so a run with no logging hands the engine no
        callback at all and the engine can skip composing messages nobody will print.

        Named for_options rather than for, which is a Python keyword.

        Reads effective_log_level and effective_log_modules rather than re-deriving
        `log_level or debug_log_level` here. The C# does re-derive both inline, which is a
        second home for the alias rule that DiagnosticOptions.EffectiveLogLevel's own
        docstring says it exists to prevent — "so no caller has to remember to drop one of
        the two". The two spellings are provably the same expression, so using the property
        loses no behaviour and removes the duplicate.
        """
        level = parse_level(options.effective_log_level)
        if level == LEVEL_NONE:
            return None

        modules = options.effective_log_modules
        named = set()
        if modules and modules != "*":
            for module in modules.replace(';', ',').split(','):
                module = trim(module)
                if module:
                    # The C# set is built with StringComparer.OrdinalIgnoreCase. Folding
                    # both sides with fold_token is what replaces it, rather than .lower():
                    # a module name is an ASCII identifier, and the two answers differ only
                    # for a name outside ASCII, where OrdinalIgnoreCase would apply full
                    # Unicode case folding and this deliberately does not.
                    named.add(fold_token(module))

        return EngineLog(level, named, console)

    def level_for(self, module: Optional[str]) -> int:
        """
        Answers the engine's "how much should module log".

        LEVEL_NONE for a module the user did not name, which is how --log-modules is
        enforced — the engine then composes nothing for it rather than composing messages
        this side throws away.
        """
        if len(self._modules) == 0 or fold_token(module if module is not None else "") in self._modules:
            return self._level
        return LEVEL_NONE

    def write(self, level: int, module: str, message: str):
        """Writes one message the engine produced."""
        self._console.write_error("{0} {1} [{2}] {3}".format(_timestamp(), name(level), module, message))


def name(level: int) -> str:
    """
    The fixed-width level name, so the module and message columns line up.

    Same five names and the same padding doc2text uses, minus the colour: this tool writes
    no escape codes of its own anywhere else.
    """
    if level == LEVEL_CRITICAL or level == LEVEL_ERROR:
        return "ERROR"
    if level == LEVEL_WARNING:
        return "WARN "
    if level == LEVEL_INFORMATION:
        return "INFO "
    if level == LEVEL_DEBUG:
        return "DEBUG"
    if level == LEVEL_TRACE:
        return "TRACE"
    return "     "


def parse_level(value: Optional[str]) -> int:
    """
    --log-level's vocabulary, folded and mapped onto the engine's scale.

    Anything unrecognised is LEVEL_NONE rather than an error, matching doc2text, whose own
    parse falls through to none. That covers None, which the C# reaches as switch's default
    case.
    """
    if value == "critical":
        return LEVEL_CRITICAL
    if value == "error":
        return LEVEL_ERROR
    if value == "warning":
        return LEVEL_WARNING
    if value == "information":
        return LEVEL_INFORMATION
    if value == "debug":
        return LEVEL_DEBUG
    if value == "trace":
        return LEVEL_TRACE
    return LEVEL_NONE


def _timestamp() -> str:
    """
    The C# format string is yyyy-MM-dd'T'HH:mm:ss.fff. strftime has no milliseconds
    directive, so %f gives microseconds and the last three digits are cut — truncating,
    which is what .fff does, not rounding.
    """
    return datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S.%f')[:-3]
