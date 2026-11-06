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

import io
from typing import BinaryIO, Optional

_CHUNK_SIZE = 64 * 1024

# The three debug switches that change how a document's bytes reach the engine.
#
# --debug-use-memory-stream, --debug-truncate and --debug-measure-io are each the input
# stream behaving differently, so none of them can be an option on the ordinary
# open-by-path — the engine has to be handed a stream this tool controls. doc2text does the
# same, swapping in its own stream before IGR_Open_Ex.
#
# They exist to answer questions about the engine rather than about a document: whether a
# failure depends on file-backed I/O, how much of a file is really read before it fails,
# and how many reads a format costs.
#
# A module of functions rather than a class: the C# is a static class, which is what C# has
# instead of a module.


def for_document(path: str, options, file_system, console, display_name) -> Optional[BinaryIO]:
    """
    The stream to open path through, or None to let the engine open the path itself.

    None in the ordinary case, so a normal run never buffers a document or wraps anything —
    the debug switches cost nothing when nobody asks for them.

    Named for_document rather than for, which is a Python keyword.
    """
    # Two independent questions — where the bytes come from, and whether the asking is
    # counted — so they compose rather than exclude each other. An if/else here silently
    # dropped --debug-measure-io whenever a truncated or buffered source was also asked
    # for, which is the same "accepted and ignored" failure these switches exist to expose.
    source = None

    if options.truncate_input_bytes is not None or options.use_memory_stream:
        source = _in_memory(path, options.truncate_input_bytes, file_system)

    if options.measure_io:
        # Counting a buffer rather than a file still answers the question the switch asks:
        # the figures are what the *engine requested*, not what the disk served. Combined
        # with --debug-truncate that is the more useful pairing of the two — how much of a
        # prefix the engine consumed before it gave up.
        source = MeasuredStream(source if source is not None else file_system.open_read(path), console, display_name)

    return source


def _in_memory(path: str, limit: Optional[int], file_system) -> BinaryIO:
    """
    The document's bytes in memory, optionally only the first limit of them.

    --debug-truncate implies the memory stream: reading a prefix means deciding where the
    bytes stop, which a file handed to the engine by name cannot express.
    """
    buffer = io.BytesIO()

    with file_system.open_read(path) as source:
        if limit is not None:
            remaining = limit
            while remaining > 0:
                chunk = source.read(min(_CHUNK_SIZE, remaining))
                if not chunk:
                    break

                buffer.write(chunk)
                remaining -= len(chunk)
        else:
            # The C# calls Stream.CopyTo, which also chunks rather than sizing one buffer to
            # the whole file.
            while True:
                chunk = source.read(_CHUNK_SIZE)
                if not chunk:
                    break
                buffer.write(chunk)

    buffer.seek(0)
    return buffer


class MeasuredStream(io.RawIOBase):
    """
    A read-through stream that counts what the engine asks for — --debug-measure-io.

    Reports on close, so the figures cover the whole document rather than whatever had
    happened by the time anyone thought to look. Counts reads and seeks separately because
    they answer different questions: a format that seeks constantly is a different problem
    from one that reads the file twice.

    The C# derives from Stream and overrides Read, Seek and Dispose; this derives from
    io.RawIOBase, whose readinto is the primitive the rest of the read API is built on, so
    overriding it counts a read however the caller spelled it. The C#'s SetLength and Write
    overrides that throw NotSupportedException need no equivalent: RawIOBase's own writable
    returning False makes write raise io.UnsupportedOperation already.
    """

    def __init__(self, inner: BinaryIO, console, display_name: str):
        super().__init__()
        self._inner = inner
        self._console = console
        self._display_name = display_name

        self._bytes_read = 0
        self._reads = 0
        self._seeks = 0
        self._reported = False

    def readinto(self, buffer) -> int:
        chunk = self._inner.read(len(buffer))
        read = len(chunk)
        buffer[:read] = chunk

        self._reads += 1
        self._bytes_read += read
        return read

    def seek(self, offset: int, whence: int = io.SEEK_SET) -> int:
        self._seeks += 1
        return self._inner.seek(offset, whence)

    def tell(self) -> int:
        return self._inner.tell()

    def readable(self) -> bool:
        return True

    def seekable(self) -> bool:
        return self._inner.seekable()

    def writable(self) -> bool:
        return False

    def close(self):
        if not self._reported:
            self._reported = True
            self._console.write_line("{0}: reads={1} seeks={2} bytes={3}".format(
                self._display_name, self._reads, self._seeks, self._bytes_read))
            self._inner.close()

        super().close()
