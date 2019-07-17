//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
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

    }

    [StructLayout(LayoutKind.Sequential)]
    public abstract class IGRStream : IDisposable
    {
        private class StreamHolder
        {
            private static readonly StreamHolder _instance = new StreamHolder();
            private Dictionary<long, IGRStream> _streams = new Dictionary<long, IGRStream>();
            private long _lastid = 0;

            public long Register(IGRStream stream)
            {
                lock(this)
                {
                    long retval = ++_lastid;
                    _streams.Add(retval, stream);
                    return retval;
                }
            }
            public void Release(long cookie)
            {
                lock(this)
                {
                    _streams.Remove(cookie);
                }
            }
            public IGRStream Get(long cookie)
            {
                lock(this)
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
                return (UInt32)payload.GetStream().Read(bufferSize, buffer);
            }
            private static UInt32 _write(IntPtr stream, Byte[] buffer, UInt32 bufferSize)
            {
                Payload payload = MarshalPayload(stream);
                return payload.GetStream().Write(buffer, bufferSize);
            }
            private static Int64 _seek(IntPtr stream, Int64 offset, uint whence)
            {
                Payload payload = MarshalPayload(stream);
                return (Int64)payload.GetStream().Seek(offset, (int)whence);
            }
            private static UInt32 _open(IntPtr stream)
            {
                Payload payload = MarshalPayload(stream);

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

        public IGRStream()
        {
            _id = System.DateTime.Now.Ticks;
        }
        ~IGRStream()
        {
            Dispose();
        }

        public virtual void Dispose()
        {
        }
        public virtual uint Read(uint Size, byte[] buffer)
        {
            return Read(Size, new IGRStream_Data((byte[] bytes, int length) =>
            {
                Array.Copy(bytes, 0, buffer, 0, length);
            }));
        }
        public virtual uint Read(uint Size, IGRStream_Data Dest)
        {
            return 0;
        }
        public abstract uint Seek(long Offset, int Origin);

        public virtual uint Write([global::System.Runtime.InteropServices.In, global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] bytes, uint size)
        {
            return 0;
        }
        public virtual void Close()
        {

        }
        public virtual IGRStream GetStreamPart(string partName, string partFullName, int partIndex)
        {
            return null;
        }
        public IntPtr NewWritableStreamPtr()
        {
            return Payload.Alloc(this, true);
        }
        public IntPtr NewStreamPtr()
        {
            return Payload.Alloc(this, false);
        }
        public static void DestroyStreamPtr(IntPtr stream)
        {
#if NETSTANDARD
            IGR_Stream igr = Marshal.PtrToStructure<IGR_Stream>(stream);
#else
            IGR_Stream igr = (IGR_Stream) Marshal.PtrToStructure(stream, typeof(IGR_Stream));
#endif

            igr.Close(stream);
        }
        public static System.IO.Stream ToStream(IntPtr stream)
        {
            return new StreamBridge(stream);
        }
    }
}

