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

import fnmatch
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
PACKAGE_ROOT = os.path.join(HERE, '..', '..', 'python')

_EVERY_TEST_FILE = '*_tests.py'

# What discovery calls a module it could not import. Kept out of every filter below:
# a file that will not import is the one result nobody meant to exclude, and dropping
# it would let a filtered run report OK while a module is broken.
_LOAD_FAILURE = 'unittest.loader._FailedTest'


def main(argv):
    # Both roots on the path so a test can import the package under test and the fakes
    # beside it without an editable install, a PYTHONPATH or any packaging step: the
    # tool is a published sample that has to run from a clone.
    sys.path.insert(0, os.path.abspath(PACKAGE_ROOT))
    sys.path.insert(0, HERE)

    listing = '--list-tests' in argv[1:]
    pattern = next((a for a in argv[1:] if not a.startswith('-')), None)
    suite = _select(pattern)

    if suite.countTestCases() == 0:
        # Not a pass. An empty suite is successful as far as unittest is concerned, so
        # reporting the run would exit 0 on a pattern that selected nothing -- which is
        # what a typo looks like, and is worth failing over rather than printing OK.
        sys.stderr.write("no test matches '%s': tried it against test file names, then "
                         "against test ids\n" % (pattern or _EVERY_TEST_FILE))
        return 1

    if listing:
        # Names only, nothing run. tests/parity/compare_inventory.py reads this to
        # check that every test the C# reference has exists here too -- the one
        # failure a passing suite cannot show, because a case dropped in translation
        # leaves no trace behind.
        #
        # The header is what tells that script which suite produced the listing, so
        # it can pick a parser without being told.
        sys.stdout.write('The following Python tests are available:\n')
        for test_id in sorted(_every_id(suite)):
            sys.stdout.write('    %s\n' % test_id)
        return 0

    result = unittest.TextTestRunner(verbosity=2).run(suite)

    return 0 if result.wasSuccessful() else 1


def _every_id(suite):
    """Each test's id, flattening the tree of suites discovery returns."""
    for item in suite:
        if isinstance(item, unittest.TestSuite):
            for found in _every_id(item):
                yield found
        else:
            yield item.id()


def _select(pattern):
    """
    The tests a pattern asks for, by file name or by test name.

    Discovery only filters file names, which is the coarse half of what anyone typing a
    pattern means: '*plan*' finds output_planner_tests.py, and the name of a single test
    finds nothing at all. So a pattern that matches no file is tried again against test
    ids, where a method name, a class name or a dotted prefix of either all work.

    File names are tried first because that is the cheaper question -- it imports only
    the modules it selects -- and because the two spellings can only disagree when a
    pattern matches a file, in which case the file is what the pattern looks like.
    """
    if pattern is None:
        return _discover(_EVERY_TEST_FILE)

    by_file = _discover(pattern)
    if by_file.countTestCases() > 0:
        return by_file

    return _matching_ids(_discover(_EVERY_TEST_FILE), pattern)


def _discover(pattern):
    return unittest.defaultTestLoader.discover(start_dir=HERE, top_level_dir=HERE, pattern=pattern)


def _matching_ids(suite, pattern):
    """
    The tests in a suite whose id matches, as a flat suite.

    A pattern carrying no wildcard is taken as a substring rather than as a whole id:
    nobody types a full 'unit.page_range_tests.PageRangeTests.test_...' to run one test,
    and a bare name matching nothing exactly would be a worse answer than matching it
    where it appears.
    """
    if not any(character in pattern for character in '*?['):
        pattern = '*%s*' % pattern

    selected = unittest.TestSuite()
    for test in _flatten(suite):
        identifier = test.id()
        if identifier.startswith(_LOAD_FAILURE) or fnmatch.fnmatchcase(identifier, pattern):
            selected.addTest(test)

    return selected


def _flatten(suite):
    """Every test case in a suite of suites, in the order discovery found them."""
    for item in suite:
        if isinstance(item, unittest.TestSuite):
            for test in _flatten(item):
                yield test
        else:
            yield item


if __name__ == '__main__':
    sys.exit(main(sys.argv))
