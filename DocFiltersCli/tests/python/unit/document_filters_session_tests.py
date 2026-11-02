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

from docfilters_cli.configuration.shared_options import LibraryOptions
from docfilters_cli.execution.document_filters_session import DocumentFiltersSession


class DocumentFiltersSessionTests(unittest.TestCase):
    """
    Where DocumentFiltersSession.create looks for the engine's own resources -- fonts
    included.

    doc2text and the other published samples default this to "." and get away with it
    because they always run from their own build output directory. This tool is invoked
    from wherever the user's documents happen to be, so "." there resolves to a directory
    with no fonts at all: the engine still initialises, but falls back to a font it cannot
    rasterise glyphs from, so rendered text comes out blank instead of missing.
    resolve_library_path is the precedence decision alone, pure and without the
    environment or the real executable path, so it is testable without either.
    """

    def test_an_explicit_library_path_wins_over_everything(self):
        # C# builds this with an object initialiser; Python has none, so the field is
        # assigned after construction. LibraryOptions' __init__ takes no arguments in
        # either port.
        options = LibraryOptions()
        options.library_path = "/opt/df"

        resolved = DocumentFiltersSession.resolve_library_path(options, "/env/df", "/exe/dir")

        self.assertEqual("/opt/df", resolved)

    def test_the_environment_variable_wins_over_the_executable_directory(self):
        options = LibraryOptions()

        resolved = DocumentFiltersSession.resolve_library_path(options, "/env/df", "/exe/dir")

        self.assertEqual("/env/df", resolved)

    def test_with_neither_argument_nor_environment_variable_it_falls_back_to_the_executable_directory(self):
        # Not the process's current directory: a general-purpose CLI is invoked
        # from wherever the user's documents are, not from its own install
        # directory, and "." would send font lookup there instead.
        options = LibraryOptions()

        resolved = DocumentFiltersSession.resolve_library_path(options, None, "/exe/dir")

        self.assertEqual("/exe/dir", resolved)
