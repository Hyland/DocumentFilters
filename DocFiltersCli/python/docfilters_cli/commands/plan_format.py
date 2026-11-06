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

from typing import List, Optional

# The handful of rendering rules every command's ICommand.describe_plan shares, kept in one
# place so the conformance corpus's vocabulary — "yes"/"no", "none" for an absent value —
# is spelled the same way everywhere.
#
# A module of functions rather than a class: the C# is a static class, which is what C# has
# instead of a module.


def yes_no(value: bool) -> str:
    return "yes" if value else "no"


def none_or(value: Optional[str]) -> str:
    """
    The C# overloads NoneOr for a string and for a list. Python has no overloading, so the
    list form is spelled none_or_list; keeping one name and dispatching on the argument's
    type would read as the shorter code while being the easier one to pass the wrong thing
    to, since an empty string and an empty list would then differ only by which branch
    happened to run first.
    """
    return "none" if not value else value


def none_or_list(values: List[str]) -> str:
    return "none" if len(values) == 0 else ", ".join(values)


def inputs(sources: List) -> str:
    return "inputs " + ", ".join(source.path for source in sources)
