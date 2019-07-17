/*
   Copyright (c) 1988-2013, ISYS Search Sofware. All rights reserved.

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

import com.perceptive.documentfilters.*;
import java.io.*;
import java.io.File;
import java.util.Arrays;

class CustomStream extends IGRStream
{
	SeekableByteArrayOutputStream m_outs;

	CustomStream()
	{
		m_outs = new SeekableByteArrayOutputStream();
	}

	public long Seek(long Offset, int Origin)
	{
		if (Offset == 0 && Origin == 1)
		{ //--> Asking for current location
			return m_outs.position();
		}
		else if (Origin == 0)
		{ //-->Seek Set
			if (Offset < 0)
			{
				return -1;
			}
			else
			{
				return m_outs.position((int) Offset);
			}
		}
		else if (Origin == 1)
		{ //-->Seek Current
			return m_outs.position(m_outs.position() + (int) Offset);
		}
		else if (Origin == 2)
		{ //-->Seek End
			long test = m_outs.size() + (int) Offset;
			return m_outs.position(m_outs.size() + (int) Offset);
		}
		return m_outs.position();
	}

	public long Write(byte[] paramArrayOfByte)
	{
		m_outs.write(paramArrayOfByte, 0, paramArrayOfByte.length);
		return paramArrayOfByte.length;
	}

	public long Read(long Size, IGRStream_Data Dest)
	{
		byte[] d = new byte[(int) Size];
		long ret = m_outs.read(d, 0, (int) Size);

		// If the amount read is different that what was asked for, make a copy
		if (ret != Size)
		{
			byte[] arrDest = new byte[(int) ret];
			System.arraycopy(d, 0, arrDest, 0, (int) ret);
			Dest.write(arrDest);
		}
		else
		{
			Dest.write(d);
		}
		return ret;
	}

	public byte[] toByteArray()
	{
		return m_outs.toByteArray();
	}
	public long writeTo(OutputStream stream) throws IOException
	{
		m_outs.writeTo(stream);
		return m_outs.size();
	}

	class SeekableByteArrayOutputStream extends ByteArrayOutputStream
	{
		protected int m_position;

		public synchronized void write(int b)
		{
			// Fill in any blanks
			while (m_position > count)
			{
				super.write((byte) 0);
			}

			// Now check if we're updating existing data or adding new data
			if (m_position < count)
			{
				buf[m_position] = (byte) b;
				m_position++;
			}
			else
			{
				super.write(b);
				m_position = count;
			}
		}
		public synchronized void write(byte b[], int off, int len)
		{
			for (int i = 0; i < len; i++)
			{
				write(b[off + i]);
			}
		}
		public synchronized int position()
		{
			return m_position;
		}
		public synchronized int position(int pos)
		{
			m_position = pos;
			return m_position;
		}
		public synchronized int read(byte b[], int offset, int len)
		{
			int i = 0;
			while (len > 0 && m_position < count)
			{
				b[offset + i] = buf[m_position];
				m_position++;
				len--;
				i++;
			}
			return i;
		}
	}
}

