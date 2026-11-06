"""docfilters CLI - Document Filters command line tool."""

from pathlib import Path


def _read_version() -> str:
    """
    The tool's own version, which `docfilters version` prints.

    Read from DocFiltersCli/VERSION.txt, the file the C# and C++17 ports use
    too, so all three print one answer. This port is run from a source checkout,
    where that file sits two directories above this package. If it is absent
    (the package was copied out of the tree) the answer is "unknown" rather than
    a second, hardcoded version that could drift from the file.
    """
    try:
        return (Path(__file__).resolve().parents[2] / 'VERSION.txt').read_text(encoding='utf-8').strip() or 'unknown'
    except OSError:
        return 'unknown'


__version__ = _read_version()
