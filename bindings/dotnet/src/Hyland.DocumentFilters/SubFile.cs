//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System.Runtime.InteropServices;

namespace Hyland.DocumentFilters
{

    /// <summary>
    /// Represents a sub-document extracted from a container document.
    /// </summary>
    public class SubFile : Extractor
    {
        private int _docHandle;
        private readonly string _id;
        private readonly string _name;
        private readonly string _comment;

        private readonly long _size;
        private long _date;
        private int _flags;
        private IGR_Extract_Stream _extractor;

        internal SubFile(DocumentFiltersBase parent, int docHandle, string id, string name, long size, long date, IGR_Extract_Stream extractor)
            : base(parent)
        {
            _docHandle = docHandle;
            _extractor = extractor;
            _id = id;
            _name = name;
            _size = size;
            _date = date;
        }

        internal SubFile(DocumentFiltersBase parent, int docHandle, IGR_Subfile_Info info, IGR_Extract_Stream extractor)
            : base(parent)
        {
            _docHandle = docHandle;
            _extractor = extractor;

            if (info.id != System.IntPtr.Zero)
                _id = Marshal.PtrToStringUni(info.id);
            if (info.name != System.IntPtr.Zero)
                _name = Marshal.PtrToStringUni(info.name);
            if (info.comment != System.IntPtr.Zero)
                _comment = Marshal.PtrToStringUni(info.comment);
            if (string.IsNullOrEmpty(_comment))
                _comment = null;

            _size = (long) info.size;
            _date = (long) info.date;
            _flags = (int) info.flags;
        }


        /// <summary>
        /// Destructor
        /// </summary>
        ~SubFile()
        {
            Dispose();
        }

        /// <summary>
        /// Disposes of the sub-document.
        /// </summary>
        public override void Dispose()
        {
            lock (this)
            {
                Close();
            }
        }

        /// <summary>
        /// Resolves the stream pointer for the sub-document.
        /// </summary>
        /// <returns></returns>
        protected override System.IntPtr ResolveStreamPtr()
        {
            Error_Control_Block ecb = new Error_Control_Block();
            System.IntPtr ret = new System.IntPtr();
            Check(_extractor(_docHandle, _id, ref ret, ref ecb), ecb);
            return ret;
        }

        /// <summary>
        /// Close the sub-document.
        /// </summary>
        override public void Close(bool closeStream=true)
        {
            base.Close(closeStream);
        }

        /// <summary>
        /// The ID property contains the unique ID of the sub-document.
        /// </summary>
        /// <returns></returns>
        public string getID()
        {
            return _id;
        }
        /// <summary>
        /// The Name property displays name of the sub-document, if available.
        /// </summary>
        /// <returns></returns>
        public string getName()
        {
            return _name;
        }

        /// <summary>
        /// The FileDate property contains last-modified date and time of the sub-document as a double-precision number (DATE).
        /// </summary>
        /// <returns></returns>
        private IGRTime getFileDate() 
        {
            return new IGRTime(getFileTime());
        }

        /// <summary>
        /// The FileTime property contains last-modified date and time of the sub-document as a .NET DateTime.
        /// </summary>
        /// <returns></returns>
        public System.DateTime getFileTime()
        {
            System.DateTime epoch = new System.DateTime(1601, 1, 1);
            return epoch.AddSeconds(_date / 10000000);
        }

        /// <summary>
        /// The FileSize property contains the size, in bytes, of the sub-document as a 64-bit number (INT64).
        /// </summary>
        public long getFileSize() => _size;

        /// <summary>
        /// Returns the unique ID of the sub-document.
        /// </summary>
        public string ID => _id;

        /// <summary>
        /// Returns the name of the sub-document.
        /// </summary>
        public string Name => _name;

        /// <summary>
        /// Returns the size of the sub-document.
        /// </summary>
        public long FileSize => _size;

        /// <summary>
        /// Returns the flags of the sub-document.
        /// </summary>
        public int Flags => _flags;

        /// <summary>
        /// Returns if the entry is password-protected.
        /// </summary>
        public bool IsEncrypted => (_flags & ISYS11dfConstants.IGR_SUBFILE_INFO_FLAG_PASSWORD_PROTECTED) != 0;

        /// <summary>
        /// Returns the comment for the sub-document, or null if one does not exist.
        /// </summary>
        public string Comment => _comment;

        /// <summary>
        /// Returns the last-modified date and time of the sub-document.
        /// </summary>
        public System.DateTime FileDate => getFileTime();

    }
}
