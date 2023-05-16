/*
   (c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved.

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

using System;
using System.IO;

namespace Hyland.DocumentFilters
{
	public class CustomStream
		: IGRStream
	{
		private System.IO.Stream m_stream;

		public CustomStream(System.IO.Stream stream)
		{
			m_stream = stream;
		}

		public override uint Read(uint Size, IGRStream_Data Dest)
		{
			byte[] data = new byte[Size];
			int res = m_stream.Read(data, 0, (int) Size);
			Dest.write(data, res);
			return (uint) res;
		}

		public override uint Seek(long Offset, int Origin)
		{
			return (uint)m_stream.Seek(Offset, (System.IO.SeekOrigin)Origin);
		}

		public override uint Write(byte[] bytes, uint size)
		{
			m_stream.Write(bytes, 0, (int)size);
			return (uint) size;
		}

		public long writeTo(BinaryWriter outputStream)
		{
			m_stream.Position = 0;
			byte[] buffer = new byte[4096];
			int bytesRead;
			long totalBytesWritten = 0;
			while ((bytesRead = m_stream.Read(buffer, 0, buffer.Length)) > 0)
			{
				outputStream.Write(buffer, 0, bytesRead);
				totalBytesWritten += bytesRead;
			}
			return totalBytesWritten;
		}
	}
}