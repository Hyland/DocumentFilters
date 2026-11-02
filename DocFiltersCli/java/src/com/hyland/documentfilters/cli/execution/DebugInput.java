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

package com.hyland.documentfilters.cli.execution;

import com.hyland.documentfilters.cli.configuration.DiagnosticOptions;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.MemoryChannel;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.channels.SeekableByteChannel;

/**
 * The three debug switches that change how a document's bytes reach the engine.
 *
 * <p>{@code --debug-use-memory-stream}, {@code --debug-truncate} and
 * {@code --debug-measure-io} are each the input stream behaving differently, so
 * none of them can be an option on the ordinary open-by-path --- the engine has to
 * be handed a stream this tool controls.
 */
public final class DebugInput
{
	private DebugInput()
	{
	}

	/**
	 * The channel to open {@code path} through, or null to let the engine open the
	 * path itself.
	 *
	 * <p>Two independent questions --- where the bytes come from, and whether the
	 * asking is counted --- so they compose rather than exclude each other.
	 *
	 * <p>{@code --debug-measure-io} on its own still buffers the file: the reference
	 * counts a file stream, but {@link IFileSystem#openRead} hands back a stream that
	 * cannot seek. The figures are unaffected, because they count what the
	 * <em>engine requested</em>, not what the disk served.
	 */
	public static SeekableByteChannel forInput(String path, DiagnosticOptions options, IFileSystem fileSystem,
		IConsole console, String displayName) throws IOException
	{
		SeekableByteChannel source = null;

		if (options.truncateInputBytes != null || options.useMemoryStream || options.measureIo)
			source = inMemory(path, options.truncateInputBytes, fileSystem);

		if (options.measureIo)
			source = new MeasuredChannel(source, console, displayName);

		return source;
	}

	/**
	 * The document's bytes in memory, optionally only the first {@code limit} of
	 * them. {@code --debug-truncate} implies the memory stream: reading a prefix
	 * means deciding where the bytes stop, which a file handed to the engine by name
	 * cannot express.
	 */
	private static MemoryChannel inMemory(String path, Long limit, IFileSystem fileSystem) throws IOException
	{
		try (InputStream source = fileSystem.openRead(path))
		{
			ByteArrayOutputStream buffer = new ByteArrayOutputStream();
			byte[] chunk = new byte[64 * 1024];
			long remaining = limit != null ? limit : Long.MAX_VALUE;
			while (remaining > 0)
			{
				int wanted = (int) Math.min(chunk.length, remaining);
				int read = source.read(chunk, 0, wanted);
				if (read <= 0)
					break;

				buffer.write(chunk, 0, read);
				remaining -= read;
			}

			return new MemoryChannel(buffer.toByteArray(), buffer.size());
		}
	}
}
