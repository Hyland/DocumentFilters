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

from ..runtime.json_writer import JsonWriter
from .argument_definition import ArgumentArity
from .argument_table import ArgumentTable


class ContractDump:
    """
    Emits the complete command line surface as flat text, for --print-contract.

    A debugging aid, not the contract: the authority on which arguments exist is
    tests/conformance/arguments.txt, and this dump is checked against it.

    render_json answers `help --json` with the same information. The two are separate
    renderers over one argument table: this output is brace-free, so an argument named
    --json cannot share it.

    The flat format is line-oriented and free of JSON, so all four languages can parse
    it with nothing but their standard library. Each argument line's four columns — the
    literal word `argument`, the template, the arity, and the description — are
    tab-separated, so a reader splits once on tab and never has to guess where the
    free-form description begins. Splitting on space instead would find the words
    "and", "or" and stray flag mentions inside a help string exactly as readily as an
    argument name, since English prose and a template are otherwise indistinguishable
    to a naive whitespace split.

        command convert
        argument\\t--to\\tsingle\\tTarget format
        argument\\t-o|--out-dir\\tsingle\\tWrite into a directory
        argument\\t--overwrite\\tnone\\tReplace existing outputs
    """

    @staticmethod
    def render() -> str:
        """Renders the whole surface: every command, and every argument on it."""
        text = []
        for command in ArgumentTable.COMMANDS:
            text.append(ContractDump.render_command(command))
        return ''.join(text)

    @staticmethod
    def render_command(command: str) -> str:
        """Renders one command's section."""
        text = ["command ", command, "\n"]
        for argument in ArgumentTable.describe_command(command):
            text.extend(["argument\t",
                         argument.template, "\t",
                         ContractDump._arity_text(argument.arity), "\t",
                         argument.description,
                         "\n"])
        return ''.join(text)

    @staticmethod
    def render_json() -> str:
        """
        The same surface as JSON, for `help --json`.

        A separate renderer rather than a reformatting of render(): that output is
        deliberately line-oriented and free of braces, so an argument named --json could
        not honestly share it. Both read the same argument table, which is what keeps
        them in agreement.

        Written with the tool's own JsonWriter rather than json.dumps, for the reason
        that class gives: the escaping policy is ours, not a library's, so the four ports
        agree. The C# reaches for JsonSerializer here, which is the one place it does
        not use its own writer.
        """
        writer = JsonWriter()
        writer.begin_array()

        for command in ArgumentTable.COMMANDS:
            writer.begin_object()
            writer.property("command", command)
            writer.name("arguments")
            writer.begin_array()

            for argument in ArgumentTable.describe_command(command):
                writer.begin_object()
                writer.property("template", argument.template)
                writer.property("description", argument.description)
                writer.property("arity", ContractDump._arity_text(argument.arity))
                writer.end_object()

            writer.end_array()
            writer.end_object()

        writer.end_array()

        # to_string() already ends the document with the newline the C# appends by hand.
        return writer.to_string()

    @staticmethod
    def _arity_text(arity: ArgumentArity) -> str:
        if arity == ArgumentArity.NO_VALUE:
            return "none"
        if arity == ArgumentArity.SINGLE_VALUE:
            return "single"
        if arity == ArgumentArity.MULTIPLE_VALUE:
            return "multiple"

        raise ValueError("arity")
