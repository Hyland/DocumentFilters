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

"""
The process exit codes, identical across the C#, Python, Java and C++17
implementations of this tool.

Only rule that matters to existing scripts: non-zero means something went
wrong. The distinct codes let a caller tell "three files failed" apart from
"you typed an argument wrong" apart from "your glob matched nothing".
"""

# Every selected input was processed.
SUCCESS = 0

# One or more files failed while --on-error skip was in effect.
PARTIAL_FAILURE = 1

# Unknown argument or verb, missing required value, malformed value.
USAGE_ERROR = 2

# The filesystem walk plus filters selected nothing to do.
NO_INPUTS_MATCHED = 3

# Output target conflict: multiple inputs into one file, or an existing file without --overwrite.
OUTPUT_CONFLICT = 4

# Document Filters could not be initialised: library not found, or license rejected.
INITIALISATION_FAILURE = 5

# A run limit was hit: --max-time, --max-memory or --timeout.
LIMIT_EXCEEDED = 6

# The run was interrupted (Ctrl-C).
INTERRUPTED = 130
