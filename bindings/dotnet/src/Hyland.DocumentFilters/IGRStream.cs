//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Hyland.DocumentFilters
{
#if !NETSTANDARD
    internal class Tuple<T1, T2>
    {
        public T1 Item1 { get; set; }
        public T2 Item2 { get; set; }
        public Tuple(T1 item1, T2 item2)
        {
            this.Item1 = item1;
            this.Item2 = item2;
        }
    }
#endif

    internal class Marshaler
    {
        public static T PtrToStructure<T>(IntPtr ptr)
        {
#if NETSTANDARD
            return Marshal.PtrToStructure<T>(ptr);
#else
            return (T)Marshal.PtrToStructure(ptr, typeof(T));
#endif
        }
        public static void StructureToPtr<T>(T structure, IntPtr ptr, bool deleteOld)
        {
#if NETSTANDARD
            Marshal.StructureToPtr<T>(structure, ptr, deleteOld);
#else
            Marshal.StructureToPtr(structure, ptr, deleteOld);
#endif
        }
        public static int SizeOf<T>()
        {
#if NETSTANDARD
            return Marshal.SizeOf<T>();
#else
            return Marshal.SizeOf(typeof(T));
#endif
        }

        public static IntPtr StructureToPtr<T>(T structure)
        {
            if (structure == null)
                return IntPtr.Zero;

            int size = SizeOf<T>();
            IntPtr ptr = Marshal.AllocHGlobal(size);
            StructureToPtr(structure, ptr, false);
            return ptr;
        }

    }

    /// <summary>
    /// Represents a stream used by the Document Filters API.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public abstract class IGRStream : IDisposable
    {
        private class StreamHolder
        {
            private static readonly StreamHolder _instance = new StreamHolder();
            private readonly Dictionary<long, IGRStream> _streams = new Dictionary<long, IGRStream>();
            private long _lastid = 0;

            public long Register(IGRStream stream)
            {
                lock(_streams)
                {
                    long retval = ++_lastid;
                    _streams.Add(retval, stream);
                    return retval;
                }
            }
            public void Release(long cookie)
            {
                lock(_streams)
                {
                    _streams.Remove(cookie);
                }
            }
            public IGRStream Get(long cookie)
            {
                lock(_streams)
                {
                    IGRStream retval;
                    if (_streams.TryGetValue(cookie, out retval))
                        return retval;
                    throw new Exception("No stream");
                }
            }
            public static StreamHolder Instance => _instance;
        }
        class Methods
        {
            internal static readonly Methods Instance = new Methods();

            public Tuple<IGR_Stream_Open, GCHandle> Open;
            public Tuple<IGR_Stream_Read, GCHandle> Read;
            public Tuple<IGR_Stream_Seek, GCHandle> Seek;
            public Tuple<IGR_Stream_Close, GCHandle> Close;
            public Tuple<IGR_Writable_Stream_Write, GCHandle> Write;
            public Tuple<IGR_CALLBACK, GCHandle> StreamCallback;

            public Methods()
            {
                Open = Bind<IGR_Stream_Open>(new IGR_Stream_Open(_open));
                Close = Bind<IGR_Stream_Close>(_close);
                Seek = Bind<IGR_Stream_Seek>(_seek);
                Read = Bind<IGR_Stream_Read>(_read);
                Write = Bind<IGR_Writable_Stream_Write>(_write);
                StreamCallback = Bind<IGR_CALLBACK>(_stream_callback);
            }
            ~Methods()
            {
                Open.Item2.Free();
                Read.Item2.Free();
                Seek.Item2.Free();
                Close.Item2.Free();
                Write.Item2.Free();
                StreamCallback.Item2.Free();
            }
            private Tuple<T, GCHandle> Bind<T>(T method)
            {
                return new Tuple<T, GCHandle>(method, GCHandle.Alloc(method));
            }
            private static Payload MarshalPayload(IntPtr stream)
            {
                return Marshaler.PtrToStructure<Payload>(stream);
            }

            private static UInt32 _read(IntPtr stream, byte[] buffer, UInt32 bufferSize)
            {
                Payload payload = MarshalPayload(stream);
                try
                {
                    return payload.GetStream().Read(bufferSize, buffer);
                }
                catch
                {
                    // Sentinel value used to notify the calling C code of an error condition
                    return UInt32.MaxValue;
                }
            }
            private static UInt32 _write(IntPtr stream, Byte[] buffer, UInt32 bufferSize)
            {
                Payload payload = MarshalPayload(stream);
                try
                {
                    return payload.GetStream().Write(buffer, bufferSize);
                }
                catch
                {
                    // Sentinel value used to notify the calling C code of an error condition
                    return UInt32.MaxValue;
                }
            }
            private static Int64 _seek(IntPtr stream, Int64 offset, uint whence)
            {
                Payload payload = MarshalPayload(stream);
                try
                {
                    return (Int64)payload.GetStream().Seek(offset, (int)whence);
                }
                catch
                {
                    // Sentinel value used to notify the calling C code of an error condition
                    return -1;
                }
            }
            private static UInt32 _open(IntPtr stream)
            {
                MarshalPayload(stream);

                return 0;
            }
            private static void _close(IntPtr stream)
            {
                Payload payload = MarshalPayload(stream);
                payload.Destroy(stream);
            }
            private static int _stream_callback(int actionID, System.IntPtr actionData, System.IntPtr context)
            {
                var payload = MarshalPayload(context);

                if (actionID == ISYS11dfConstants.IGR_ACTION_GET_STREAM_PART)
                {
                    var part = Marshaler.PtrToStructure<IGR_T_ACTION_GET_STREAM_PART>(actionData);
                    var res = payload.GetStream().GetStreamPart(part.partName, part.partFullName, part.partIndex);
                    if (res != null)
                    {
                        part.istr = res.NewStreamPtr();
                        Marshaler.StructureToPtr<IGR_T_ACTION_GET_STREAM_PART>(part, actionData, false);
                        return 0;
                    }
                }
                return -1;
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct Payload
        {
            public IGR_Stream_Open Open;
            public IGR_Stream_Read Read;
            public IGR_Stream_Seek Seek;
            public IGR_Stream_Close Close;
            public IGR_Writable_Stream_Write Write;
            public long StreamObjectRef;

            public IGRStream GetStream()
            {
                return StreamHolder.Instance.Get(StreamObjectRef);
            }
            public void Destroy(IntPtr handle)
            {
                GetStream().Close();
                StreamHolder.Instance.Release(StreamObjectRef);
                Marshal.FreeHGlobal(handle);
            }
            public static IntPtr Alloc(IGRStream stream, bool writable)
            {
                var payloadSize = Marshaler.SizeOf<Payload>();
                var retval = Marshal.AllocHGlobal(payloadSize);
                var self = StreamHolder.Instance.Register(stream);
                var payload = new Payload
                {
                    Open = Methods.Instance.Open.Item1,
                    Read = Methods.Instance.Read.Item1,
                    Seek = Methods.Instance.Seek.Item1,
                    Close = Methods.Instance.Close.Item1,
                    Write = Methods.Instance.Write.Item1,
                    StreamObjectRef = self
                };
                Marshaler.StructureToPtr<Payload>(payload, retval, false);

                if (!writable)
                {
                    Error_Control_Block ecb = new Error_Control_Block();
                    DocumentFiltersBase.Check(ISYS11df.IGR_Extend_Stream(retval, Methods.Instance.StreamCallback.Item1, retval, ref retval, ref ecb), ecb);
                }

                return retval;
            }
           

        }

        internal long _id;
        internal virtual string GetName()
        {
            return ToString();
        }

        /// <summary>
        /// Constructs a new IGRStream.
        /// </summary>
        public IGRStream()
        {
            _id = System.DateTime.Now.Ticks;
        }

        /// <summary>
        /// Destroys the IGRStream.
        /// </summary>
        ~IGRStream()
        {
            Dispose();
        }

        /// <summary>
        /// Disposes of the IGRStream.
        /// </summary>
        public virtual void Dispose()
        {
        }

        /// <summary>
        /// Reads up to Size bytes from the stream into the buffer.
        /// </summary>
        /// <param name="Size">The number of bytes to read.</param>
        /// <param name="buffer">The buffer where the bytes are to written.</param>
        /// <returns>The number of bytes read.</returns>
        public virtual uint Read(uint Size, byte[] buffer)
        {
            return Read(Size, new IGRStream_Data((byte[] bytes, int length) =>
            {
                if (length > 0)
                {
                    Array.Copy(bytes, 0, buffer, 0, length);
                }
            }));
        }

        /// <summary>
        /// Reads up to Size bytes from the stream into the destination stream.
        /// </summary>
        /// <param name="Size">The number of bytes to read.</param>
        /// <param name="Dest">The destination where the bytes are to be written.</param>
        /// <returns>The number of bytes read.</returns>
        public virtual uint Read(uint Size, IGRStream_Data Dest)
        {
            return 0;
        }

        /// <summary>
        /// Seek to the specified location.
        /// </summary>
        /// <param name="Offset">The offset where to seek.</param>
        /// <param name="Origin">The origin of the seek.</param>
        /// <returns></returns>
        public abstract uint Seek(long Offset, int Origin);

        /// <summary>
        /// Writes up to Size bytes from the buffer to the stream.
        /// </summary>
        /// <param name="bytes">The bytes to be written.</param>
        /// <param name="size">The number of bytes to write.</param>
        /// <returns>The number of bytes to write.</returns>
        public virtual uint Write([global::System.Runtime.InteropServices.In, global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] bytes, uint size)
        {
            return 0;
        }

        /// <summary>
        /// Closes the stream.
        /// </summary>
        public virtual void Close()
        {
        }

        /// <summary>
        /// Gets a part of the stream.
        /// </summary>
        public virtual IGRStream GetStreamPart(string partName, string partFullName, int partIndex)
        {
            return null;
        }

        /// <summary>
        /// Create a new writable stream.
        /// </summary>
        public IntPtr NewWritableStreamPtr()
        {
            return Payload.Alloc(this, true);
        }

        /// <summary>
        /// Create a new readable stream.
        /// </summary>
        public IntPtr NewStreamPtr()
        {
            return Payload.Alloc(this, false);
        }

        /// <summary>
        /// Destroy the stream.
        /// </summary>
        public static void DestroyStreamPtr(IntPtr stream)
        {
#if NETSTANDARD
            IGR_Stream igr = Marshal.PtrToStructure<IGR_Stream>(stream);
#else
            IGR_Stream igr = (IGR_Stream) Marshal.PtrToStructure(stream, typeof(IGR_Stream));
#endif

            igr.Close(stream);
        }

        /// <summary>
        /// Convert a stream to a System.IO.Stream.
        /// </summary>
        public static System.IO.Stream ToStream(IntPtr stream)
        {
            return new StreamBridge(stream);
        }
    }
}

