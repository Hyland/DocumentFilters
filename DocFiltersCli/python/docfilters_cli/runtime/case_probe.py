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

from typing import Callable, Iterable, Optional

# Chooses the spelling to look for when asking a directory whether it tells names apart by
# case.
#
# Separate from system_file_system and pure, because the one thing that has to be right
# here is *where* the question is asked, and a real filesystem cannot show that in a test:
# on a single volume every answer agrees, so a probe aimed one directory too high passes
# every test a machine can run. What a test can check is that the name handed back lies
# inside the directory being measured -- which is the whole of the rule.
#
# Why it matters: the lookup rules belong to the directory, not to the host and not to the
# volume above it. /mnt/ntfs is case-insensitive while the /mnt holding it is not, and
# Windows can mark a single directory case-sensitive inside an otherwise insensitive
# volume. Measuring the parent gets both of those backwards.


def measure(directory: str, entries: Iterable[str], exists: Callable[[str], bool]) -> Optional[bool]:
    """
    Whether directory tells names apart by case, or None when it has nothing to answer
    with -- empty, or holding nothing with a letter in its name -- so the caller falls back
    to the host default.

    entries is what is already in it, consumed lazily and only as far as the first name
    with a letter in it, so a directory of a hundred thousand files costs one entry rather
    than a hundred thousand. exists answers whether a path is there, file or directory;
    anything it cannot answer reads as absent.
    """
    for entry in entries:
        sibling = flipped_sibling(directory, entry)
        if sibling is None:
            continue

        return not exists(sibling)

    return None


def flipped_sibling(directory: str, entry: str) -> Optional[str]:
    """
    The path to look for inside directory to measure how it matches names, or None when
    entry has no letter to flip and so tells the caller nothing.

    entry is something already in the directory, named or full-path. Its name is flipped
    letter by letter, so Report.txt is looked for as rEPORT.TXT: if that is found, the
    directory matched two spellings to one file and is case-insensitive.
    """
    name = _name_of(entry)
    flipped = _flip_case(name)
    if flipped == name:
        return None

    parent = directory.rstrip('/\\')
    return flipped if len(parent) == 0 else parent + "/" + flipped


def _name_of(path: str) -> str:
    trimmed = path.rstrip('/\\')
    slash = max(trimmed.rfind('/'), trimmed.rfind('\\'))
    return trimmed if slash < 0 else trimmed[slash + 1:]


def _flip_case(text: str) -> str:
    """
    ASCII-only, and not str.swapcase(), which flips the whole of Unicode and turns some
    letters into two: the reference flips A-Z and a-z alone, so every port probes for the
    same name.
    """
    return "".join(chr(ord(c) - 32) if "a" <= c <= "z"
                   else chr(ord(c) + 32) if "A" <= c <= "Z"
                   else c
                   for c in text)
