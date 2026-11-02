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

import com.hyland.documentfilters.cli.runtime.IConsole;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.NonWritableChannelException;
import java.nio.channels.SeekableByteChannel;

/**
 * A read-through channel that counts what the engine asks for ---
 * {@code --debug-measure-io}.
 *
 * <p>Reports on close, so the figures cover the whole document. Counts reads and
 * seeks separately because they answer different questions. A seek is a call to
 * {@link #position(long)}, which is what the engine adapter makes for each of the
 * engine's own seeks.
 */
public final class MeasuredChannel implements SeekableByteChannel
{
	private final SeekableByteChannel _inner;
	private final IConsole _console;
	private final String _displayName;

	private long _bytesRead;
	private long _reads;
	private long _seeks;
	private boolean _reported;

	public MeasuredChannel(SeekableByteChannel inner, IConsole console, String displayName)
	{
		_inner = inner;
		_console = console;
		_displayName = displayName;
	}

	@Override
	public int read(ByteBuffer destination) throws IOException
	{
		int read = _inner.read(destination);
		_reads++;
		// End of stream is -1 here and 0 in the reference; neither moved any bytes.
		_bytesRead += Math.max(read, 0);
		return read;
	}

	@Override
	public SeekableByteChannel position(long newPosition) throws IOException
	{
		_seeks++;
		_inner.position(newPosition);
		return this;
	}

	@Override
	public long position() throws IOException
	{
		return _inner.position();
	}

	@Override
	public long size() throws IOException
	{
		return _inner.size();
	}

	@Override
	public int write(ByteBuffer source)
	{
		throw new NonWritableChannelException();
	}

	@Override
	public SeekableByteChannel truncate(long size)
	{
		throw new NonWritableChannelException();
	}

	@Override
	public boolean isOpen()
	{
		return _inner.isOpen();
	}

	@Override
	public void close() throws IOException
	{
		if (_reported)
			return;

		_reported = true;
		_console.writeLine(_displayName + ": reads=" + _reads + " seeks=" + _seeks + " bytes=" + _bytesRead);
		_inner.close();
	}
}
