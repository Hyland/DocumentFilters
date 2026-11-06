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

import unittest

from docfilters_cli.engine.document_filters_engine import DocumentFiltersEngine
from docfilters_cli.engine.engine_exception import EngineException


class DocumentFiltersEngineTests(unittest.TestCase):
    """
    Python-only: the one thing below the seam that is not the reference's behaviour.

    The real engine is otherwise untestable here and deliberately untested -- it needs the
    binding, a native library and a license, which is the whole reason every outcome is
    tested against the fake instead. close() is the exception, because what it has to
    survive is a defect in the binding's Python wrapper rather than anything the engine
    does, and that is reachable with no binding present at all.

    DocumentFilters.Close() is `_Call(self.API.Close_Instance)`, and _Call checks the value
    the entry point returned -- but Close_Instance is declared `restype = None`, so there is
    nothing to check and the check always fails. Every Close() raises. Nothing in the
    reference corresponds to the tolerance that costs, which is why these tests exist: to
    say what it is for, and to pin that it stays narrow.
    """

    def test_close_tolerates_the_bindings_missing_return_code(self):
        engine, api = _initialised_engine("Unknown Error: None")

        engine.close()

        # Nothing raised, and the instance really was closed: the entry point ran, and
        # only the wrapper's reading of what it returned was wrong.
        self.assertEqual(1, api.closes)

    def test_close_still_raises_when_the_engine_really_failed(self):
        """
        The tolerance is for the one message the wrapper synthesises, nothing wider.

        A genuine failure of Close_Instance arrives by the same route and with the same
        absent code -- the message is the only thing that separates the two -- so a
        tolerance keyed on anything less exact would swallow it.
        """
        engine, api = _initialised_engine("The document handle is invalid")

        with self.assertRaises(EngineException) as caught:
            engine.close()

        self.assertIn("The document handle is invalid", caught.exception.message)
        self.assertEqual(1, api.closes)

    def test_close_asks_nothing_of_an_engine_that_never_came_up(self):
        # The guard that was there before any of this: reaching Close_Instance loads the
        # native library, so closing an engine whose initialize failed for want of that
        # library would fail a second time and replace the real error with a confusing one.
        engine, api = _initialised_engine("Unknown Error: None")
        engine._initialised = False

        engine.close()

        self.assertEqual(0, api.closes)


class _ApiWhoseCloseRaises:
    """
    The binding's DocumentFilters class, reduced to the one method these tests call.

    Not a fake engine -- fakes.FakeDocumentFiltersEngine stands in for the seam, one level
    above this. This stands in for what the seam talks to.
    """

    def __init__(self, message: str):
        self._message = message

        self.closes = 0
        """How many times Close() was reached, so a swallowed failure is still a real call."""

    def Close(self):
        """PascalCase because it is the binding's name for it, not this project's."""
        self.closes += 1
        raise RuntimeError(self._message)


def _initialised_engine(close_message: str):
    """
    An engine that has come up, over an API whose Close() raises close_message.

    __new__ rather than the constructor, which asks for the binding: the import is lazy
    precisely so this module can be imported without one, and constructing an engine would
    undo that. The two fields the constructor sets are the two set here.

    RuntimeError rather than the binding's IGRException, and for the same reason: with no
    binding loaded there is no IGRException to raise, and _translate falls back to
    EngineError.ERROR with str(error) as the message -- which is exactly the shape the real
    IGRException arrives in here, since the code the wrapper passes is None and cannot be
    named either.
    """
    api = _ApiWhoseCloseRaises(close_message)

    engine = DocumentFiltersEngine.__new__(DocumentFiltersEngine)
    engine._api = api
    engine._initialised = True

    return engine, api


if __name__ == '__main__':
    unittest.main()
