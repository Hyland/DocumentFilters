//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.IO;
using System.Runtime.InteropServices;

namespace Hyland.DocumentFilters
{
#pragma warning disable 1591
    public class IGRStreamBridge : IGRStream
    {
        private System.IO.Stream _stream;
        public IGRStreamBridge(System.IO.Stream stream)
            : base()
        {
            _stream = stream;
        }
        ~IGRStreamBridge()
        {
        }
        internal override string GetName()
        {
            return _stream is FileStream ? ((FileStream)_stream).Name : _stream.ToString();
        }

        public override uint Read(uint Size, IGRStream_Data Dest)
        {
            byte[] data = new byte[Size];
            int res = _stream.Read(data, 0, (int) Size);
            Dest.write(data, res);
            return (uint) res;
        }

        public override uint Seek(long Offset, int Origin)
        {
            // Some streams throw if the seek past the end, but the expected behavior is to return the end offset
            long dest = Offset;
            SeekOrigin origin = (System.IO.SeekOrigin)Origin;
            switch (origin)
            {
                case SeekOrigin.Begin:
                    dest = Offset;
                    break;
                case SeekOrigin.End:
                    dest = _stream.Length + Offset;
                    break;
                case SeekOrigin.Current:
                    dest = _stream.Position + Offset;
                    break;
            }
            if (!_stream.CanWrite)
                dest = Math.Min(dest, _stream.Length);
            return (uint)_stream.Seek(dest, SeekOrigin.Begin);
        }

        public override uint Write(byte[] bytes, uint size)
        {
            _stream.Write(bytes, 0, (int)size);
            return size;
        }
        public override void Close()
        {
            //_stream.Close();
        }
    }
    public class StreamBridge : System.IO.Stream
    {
        IntPtr _stream;
        IGR_Stream _methods;

        public StreamBridge(IntPtr stream)
        {
            _stream = stream;
#if NETSTANDARD
            _methods = Marshal.PtrToStructure<IGR_Stream>(_stream);
#else
            _methods = (IGR_Stream) Marshal.PtrToStructure(_stream, typeof(IGR_Stream));
#endif            
        }

        public override bool CanRead => true;

        public override bool CanSeek => true;

        public override bool CanWrite => false;

        public override long Length
        {
            get
            {
                long current = Seek(0, SeekOrigin.Current);
                long retval = Seek(0, SeekOrigin.End);
                Seek(current, SeekOrigin.Begin);
                return retval;
            }
        }

        public override long Position
        {
            get
            {
                return Seek(0, SeekOrigin.Current);
            }
            set
            {
                Seek(value, SeekOrigin.Begin);
            }
        }
        
        public override void Flush()
        {
            
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            return (int) _methods.Read(_stream, buffer, (uint) count);
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            return _methods.Seek(_stream, offset, (uint)origin);
        }

        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotImplementedException();
        }
    }
}
