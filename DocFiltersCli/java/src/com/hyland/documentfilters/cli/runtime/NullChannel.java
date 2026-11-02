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
import java.nio.ByteBuffer;
import java.nio.channels.SeekableByteChannel;

/**
 * A seekable channel that discards everything written to it.
 *
 * <p>The Java counterpart of the reference's {@code Stream.Null}, for
 * {@code --fingerprint}, which renders a page only to read an attribute off it.
 * Unlike {@code Stream.Null} it keeps a position and a size, because a canvas
 * seeks back over what it has written and would be confused by a channel that
 * claims to be empty; reading always reports end of stream.
 */
public final class NullChannel implements SeekableByteChannel
{
	private long _position;
	private long _size;
	private boolean _open = true;

	@Override
	public int read(ByteBuffer destination)
	{
		return -1;
	}

	@Override
	public int write(ByteBuffer source)
	{
		int count = source.remaining();
		source.position(source.limit());
		_position += count;
		_size = Math.max(_size, _position);
		return count;
	}

	@Override
	public long position()
	{
		return _position;
	}

	@Override
	public SeekableByteChannel position(long newPosition) throws IOException
	{
		if (newPosition < 0)
			throw new IOException("position out of range: " + newPosition);
		_position = newPosition;
		return this;
	}

	@Override
	public long size()
	{
		return _size;
	}

	@Override
	public SeekableByteChannel truncate(long size)
	{
		_size = Math.min(_size, size);
		_position = Math.min(_position, size);
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
}
