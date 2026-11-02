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

package com.hyland.documentfilters.cli.runtime;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.SeekableByteChannel;
import java.util.Arrays;

/**
 * A growable, seekable, in-memory byte channel.
 *
 * <p>The Java counterpart of the reference's {@code MemoryStream}. The JDK has no
 * seekable in-memory channel, and the engine needs one in both directions: it
 * seeks while reading an input, and a canvas seeks back to patch headers it has
 * already written.
 *
 * <p>Closing only marks the channel closed; the bytes stay readable through
 * {@link #toByteArray} and {@link #writeTo}, as a closed {@code MemoryStream}'s do
 * through {@code ToArray}.
 */
public final class MemoryChannel implements SeekableByteChannel
{
	private byte[] _buffer;
	private int _size;
	private int _position;
	private boolean _open = true;

	/** An empty channel, for writing. */
	public MemoryChannel()
	{
		_buffer = new byte[4096];
	}

	/** A channel over a copy of {@code data}, positioned at the start, for reading. */
	public MemoryChannel(byte[] data, int length)
	{
		_buffer = Arrays.copyOf(data, Math.max(length, 1));
		_size = length;
	}

	@Override
	public int read(ByteBuffer destination) throws IOException
	{
		ensureOpen();
		if (_position >= _size)
			return -1;

		int count = Math.min(destination.remaining(), _size - _position);
		destination.put(_buffer, _position, count);
		_position += count;
		return count;
	}

	@Override
	public int write(ByteBuffer source) throws IOException
	{
		ensureOpen();
		int count = source.remaining();
		long end = (long) _position + count;
		if (end > Integer.MAX_VALUE)
			throw new IOException("in-memory output larger than 2GB");

		if (end > _buffer.length)
			_buffer = Arrays.copyOf(_buffer, (int) Math.min(Integer.MAX_VALUE, Math.max(end, (long) _buffer.length * 2)));

		// A write past the end leaves a zero-filled gap, as a MemoryStream does;
		// Arrays.copyOf already zero-fills, but a shrink-then-grow would not.
		if (_position > _size)
			Arrays.fill(_buffer, _size, _position, (byte) 0);

		source.get(_buffer, _position, count);
		_position = (int) end;
		_size = Math.max(_size, _position);
		return count;
	}

	@Override
	public long position() throws IOException
	{
		ensureOpen();
		return _position;
	}

	@Override
	public SeekableByteChannel position(long newPosition) throws IOException
	{
		ensureOpen();
		if (newPosition < 0 || newPosition > Integer.MAX_VALUE)
			throw new IOException("position out of range: " + newPosition);
		_position = (int) newPosition;
		return this;
	}

	@Override
	public long size() throws IOException
	{
		ensureOpen();
		return _size;
	}

	@Override
	public SeekableByteChannel truncate(long size) throws IOException
	{
		ensureOpen();
		if (size < 0)
			throw new IOException("negative size: " + size);
		if (size < _size)
			_size = (int) size;
		if (_position > size)
			_position = (int) size;
		return this;
	}

	@Override
	public boolean isOpen()
	{
		return _open;
	}

	@Override
	public void close()
	{
		_open = false;
	}

	/** A copy of everything written, whether or not the channel is still open. */
	public byte[] toByteArray()
	{
		return Arrays.copyOf(_buffer, _size);
	}

	/** Copies everything written to {@code destination}. */
	public void writeTo(OutputStream destination) throws IOException
	{
		destination.write(_buffer, 0, _size);
	}

	private void ensureOpen() throws ClosedChannelException
	{
		if (!_open)
			throw new ClosedChannelException();
	}
}
