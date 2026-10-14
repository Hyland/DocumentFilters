/*
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
 */

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// The three debug switches that change how a document's bytes reach the engine.
	/// </summary>
	/// <remarks>
	/// <c>--debug-use-memory-stream</c>, <c>--debug-truncate</c> and
	/// <c>--debug-measure-io</c> are each the input stream behaving differently, so
	/// none of them can be an option on the ordinary open-by-path — the engine has to
	/// be handed a stream this tool controls. doc2text does the same, swapping in its
	/// own stream before <c>IGR_Open_Ex</c>.
	///
	/// They exist to answer questions about the engine rather than about a document:
	/// whether a failure depends on file-backed I/O, how much of a file is really
	/// read before it fails, and how many reads a format costs.
	/// </remarks>
	public static class DebugInput
	{
		/// <summary>
		/// The stream to open <paramref name="path"/> through, or null to let the engine
		/// open the path itself.
		/// </summary>
		/// <remarks>
		/// Null in the ordinary case, so a normal run never buffers a document or wraps
		/// anything — the debug switches cost nothing when nobody asks for them.
		/// </remarks>
		public static Stream For(string path, DiagnosticOptions options, IFileSystem fileSystem,
								 IConsole console, string displayName)
		{
			// Two independent questions — where the bytes come from, and whether the
			// asking is counted — so they compose rather than exclude each other. An
			// if/else here silently dropped --debug-measure-io whenever a truncated or
			// buffered source was also asked for, which is the same "accepted and
			// ignored" failure these switches exist to expose.
			Stream source = null;

			if (options.TruncateInputBytes.HasValue || options.UseMemoryStream)
				source = InMemory(path, options.TruncateInputBytes, fileSystem);

			if (options.MeasureIo)
			{
				// Counting a buffer rather than a file still answers the question the
				// switch asks: the figures are what the *engine requested*, not what the
				// disk served. Combined with --debug-truncate that is the more useful
				// pairing of the two — how much of a prefix the engine consumed before
				// it gave up.
				source = new MeasuredStream(source ?? fileSystem.OpenRead(path), console, displayName);
			}

			return source;
		}

		/// <summary>
		/// The document's bytes in memory, optionally only the first
		/// <paramref name="limit"/> of them.
		/// </summary>
		/// <remarks>
		/// <c>--debug-truncate</c> implies the memory stream: reading a prefix means
		/// deciding where the bytes stop, which a file handed to the engine by name
		/// cannot express.
		/// </remarks>
		private static Stream InMemory(string path, long? limit, IFileSystem fileSystem)
		{
			using Stream source = fileSystem.OpenRead(path);
			var buffer = new MemoryStream();

			if (limit.HasValue)
			{
				var chunk = new byte[64 * 1024];
				long remaining = limit.Value;
				while (remaining > 0)
				{
					int wanted = (int)Math.Min(chunk.Length, remaining);
					int read = source.Read(chunk, 0, wanted);
					if (read <= 0)
						break;

					buffer.Write(chunk, 0, read);
					remaining -= read;
				}
			}
			else
			{
				source.CopyTo(buffer);
			}

			buffer.Position = 0;
			return buffer;
		}
	}

	/// <summary>
	/// A read-through stream that counts what the engine asks for — <c>--debug-measure-io</c>.
	/// </summary>
	/// <remarks>
	/// Reports on close, so the figures cover the whole document rather than whatever
	/// had happened by the time anyone thought to look. Counts reads and seeks
	/// separately because they answer different questions: a format that seeks
	/// constantly is a different problem from one that reads the file twice.
	/// </remarks>
	public sealed class MeasuredStream : Stream
	{
		private readonly Stream _inner;
		private readonly IConsole _console;
		private readonly string _displayName;

		private long _bytesRead;
		private long _reads;
		private long _seeks;
		private bool _reported;

		public MeasuredStream(Stream inner, IConsole console, string displayName)
		{
			_inner = inner;
			_console = console;
			_displayName = displayName;
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			int read = _inner.Read(buffer, offset, count);
			_reads++;
			_bytesRead += read;
			return read;
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			_seeks++;
			return _inner.Seek(offset, origin);
		}

		public override bool CanRead => _inner.CanRead;
		public override bool CanSeek => _inner.CanSeek;
		public override bool CanWrite => false;
		public override long Length => _inner.Length;

		public override long Position
		{
			get => _inner.Position;
			set => _inner.Position = value;
		}

		public override void Flush() => _inner.Flush();
		public override void SetLength(long value) => throw new NotSupportedException();
		public override void Write(byte[] buffer, int offset, int count) => throw new NotSupportedException();

		protected override void Dispose(bool disposing)
		{
			if (disposing && !_reported)
			{
				_reported = true;
				_console.WriteLine($"{_displayName}: reads={_reads} seeks={_seeks} bytes={_bytesRead}");
				_inner.Dispose();
			}

			base.Dispose(disposing);
		}
	}
}
