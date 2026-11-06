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

from enum import IntEnum
from typing import Optional


class EngineError(IntEnum):
    """
    Every way a Document Filters call can fail, as the engine reports them.

    One member per IGR_E_* constant in the binding's constant module, with the
    constant's own value, so a code coming back from the binding maps across without
    a lookup table and an unrecognised one is still carried faithfully.

    This list is the axis the engine tests enumerate: for each outcome, what the CLI
    is supposed to do with it. Most become a DocumentError against one input and let
    the batch continue; a few are worth telling apart, and those are the interesting
    cases.

    An IntEnum rather than a plain Enum because the values are the engine's own
    numbers: the codes cross the ABI, and Code below has to be comparable with one
    whether or not this enum could name it.
    """

    NONE = 0
    """No error. Present so the enum can round-trip any code the engine returns."""

    OPEN_ERROR = 1
    """IGR_E_OPEN_ERROR: the file could not be opened at all."""

    WRONG_TYPE = 2
    """IGR_E_WRONG_TYPE: the call does not apply to this kind of document."""

    IN_USE = 3
    """IGR_E_IN_USE."""

    NOT_READABLE = 4
    """IGR_E_NOT_READABLE: recognised, but nothing can be read out of it."""

    PASSWORD = 5
    """IGR_E_PASSWORD: a password is needed, or the one supplied was wrong."""

    NOT_FOUND = 10
    """IGR_E_NOT_FOUND: no such file, page, subfile or option."""

    WRITE_ERROR = 11
    """IGR_E_WRITE_ERROR: the output could not be written."""

    NOT_VALID_FOR_THIS_CLASS = 12
    """IGR_E_NOT_VALID_FOR_THIS_CLASS."""

    ERROR = 13
    """IGR_E_ERROR: the general failure the engine falls back on."""

    INVALID_HANDLE = 14
    """IGR_E_INVALID_HANDLE: a handle used after it was closed."""

    INVALID_POINTER = 15
    """IGR_E_INVALID_POINTER."""

    INVALID_PARAMETER = 16
    """IGR_E_INVALID_PARAMETER: an option string or flag the engine rejected."""

    FILE_CORRUPT = 17
    """IGR_E_FILE_CORRUPT: the document is damaged."""

    OUT_OF_MEMORY = 20
    """IGR_E_OUT_OF_MEMORY."""

    BAD_ERROR = 21
    """IGR_E_BAD_ERROR."""

    WRONG_THREAD = 22
    """IGR_E_WRONG_THREAD."""

    TOO_MANY_HANDLES = 23
    """IGR_E_TOO_MANY_HANDLES: handles leaked, or a batch too large to hold open."""

    FONTS_NOT_FOUND = 24
    """IGR_E_FONTS_NOT_FOUND: rendering needs fonts the install does not have."""

    VERSION_MISMATCH = 25
    """IGR_E_VERSION_MISMATCH: the binding and the native library disagree."""

    ACTION_NOT_SUPPORTED = 26
    """IGR_E_ACTION_NOT_SUPPORTED: this format cannot do what was asked."""

    CANCELLED = 27
    """
    The open callback asked the engine to stop — IGR_CANCELLED.

    Not a failure of the document. This is the only way to interrupt the engine at
    all: the callback is raised during Open as
    IGR_OPEN_CALLBACK_ACTION_HEARTBEAT, and nothing in the API offers the same for
    rendering — IGR_Render_Page takes no callback. So a caller that asks for this
    already knows why, and translates it into its own error rather than reporting the
    document as broken.
    """


class EngineException(Exception):
    """
    A Document Filters call failed. The one exception type above the seam.

    Everything the binding can throw is translated into this, so nothing above the
    seam catches IGRException — which is what lets a fake engine reproduce any
    failure the real one can, and what stops a binding type from leaking into the
    conversion paths.

    Deliberately not a CliError, matching the C#, where it derives from Exception and
    not from that hierarchy. It carries no error code and no exit code of its own
    because it is not something the user is ever shown: execution catches it and
    restates it as a DocumentError against one input, or as an
    InitialisationError, or — for CANCELLED — as a RunInterruptedError. An
    EngineException reaching program.py uncaught is a bug in execution, and is meant
    to look like one rather than be quietly reported as a failed document.
    """

    def __init__(self, error: EngineError, code: int, message: str,
                 inner: Optional[BaseException] = None):
        super().__init__(message)

        self.error = error
        """The failure, named."""

        self.code = code
        """
        The raw IGR_E_* value, kept even when error could not name it, so a code this
        enum does not know about is still reportable.
        """

        self.message = message
        """
        The engine's own text. Named to match CliError.message, so the one place that
        formats a failure does not have to care which of the two it holds.
        """

        self.inner = inner
        """
        What the binding raised, when there was one.

        C# passes this to Exception's inner-exception constructor; Python's
        BaseException has no equivalent slot that survives being re-raised outside a
        handler, so it is kept as a field. `raise ... from` sets __cause__, which the
        interpreter prints, but only while the original is still in scope — this field
        is what a caller can read later.
        """

    @staticmethod
    def classify(code: int) -> EngineError:
        """
        Maps a raw engine code onto EngineError.

        An unknown code becomes EngineError.ERROR — the engine's own catch-all —
        rather than raising, because a newer native library returning a code this
        build has never heard of is not a reason to crash. EngineException.code still
        carries the real value.

        The non-int guard is not in the C#, which cannot be handed anything but an
        int. Here the code arrives out of a dict the binding builds, so it can be
        anything at all; and bool is checked before int because in Python True is 1,
        which would otherwise classify as OPEN_ERROR.
        """
        if isinstance(code, bool) or not isinstance(code, int):
            return EngineError.ERROR

        try:
            return EngineError(code)
        except ValueError:
            return EngineError.ERROR
