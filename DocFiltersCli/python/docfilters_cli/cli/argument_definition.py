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

from enum import Enum
from typing import Callable, Optional


class ArgumentArity(Enum):
    """How many values an argument takes."""

    NO_VALUE = 0
    """A switch: --overwrite. The setter receives None."""

    SINGLE_VALUE = 1
    """One value: --out-dir out/. Later occurrences replace earlier ones."""

    MULTIPLE_VALUE = 2
    """Repeatable: --redact, -p. The setter is called once per occurrence."""


class ArgumentDefinition:
    """
    One argument, paired with the setter that writes its value into the target — the
    shared options for a shared argument, or a per-command options object such as
    ConvertOptions.

    The C# splits this in two: an IArgumentDescriptor interface holding the parts that
    do not depend on what the argument writes into, and a generic
    ArgumentDefinition<T> that adds the setter. The interface exists only so that
    --print-contract and the coverage tests can hold a heterogeneous collection of
    definitions that differ in T. Python's names are not typed that way, so one class
    serves both roles; nothing is lost.
    """

    def __init__(self,
                 template: str,
                 description: str,
                 arity: ArgumentArity,
                 setter: Callable[[object, Optional[str]], None]):
        self.template = template
        """McMaster-style template, e.g. -o|--out-dir. Kept verbatim: it is what the
        contract dump prints, so the spelling is part of the contract even though the
        library it was named for is not used here."""

        self.description = description
        """Help text."""

        self.arity = arity
        """How many values it takes."""

        self.setter = setter
        """Writes the parsed value into the target. For NO_VALUE the value is None."""

        self._long_name: Optional[str] = None
        self._short_name: Optional[str] = None

        for form in self.template.split('|'):
            if form.startswith("--"):
                self._long_name = form[2:]
            elif form.startswith("-"):
                self._short_name = form[1:]

    @property
    def long_name(self) -> str:
        """
        The long name without dashes, e.g. out-dir. Used to sort the contract dump.

        An argument written only in short form, such as -x, is named by that form —
        there is no long spelling to prefer.
        """
        return self._long_name if self._long_name is not None else self.short_name

    @property
    def short_name(self) -> Optional[str]:
        """The short name without its dash, e.g. o, or None when there is none."""
        return self._short_name
