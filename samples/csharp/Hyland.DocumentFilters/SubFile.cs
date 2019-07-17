//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

namespace Hyland.DocumentFilters
{

    public class SubFile : Extractor
    {
        private int _docHandle;
        private readonly string _id;
        private readonly string _name;

        private readonly long _size;
        private long _date;
        private IGR_Extract_Stream _extractor;

        internal SubFile(DocumentFiltersBase parent, int docHandle, string id, string name, long size, long date, IGR_Extract_Stream extractor)
            : base(parent)
        {
            _docHandle = docHandle;
            _id = id;
            _name = name;
            _size = size;
            _date = date;
            _extractor = extractor;
        }
        ~SubFile()
        {
            Dispose();
        }
        public override void Dispose()
        {
            lock (this)
            {
                Close();
            }
        }
        protected override System.IntPtr ResolveStreamPtr()
        {
            Error_Control_Block ecb = new Error_Control_Block();
            System.IntPtr ret = new System.IntPtr();
            Check(_extractor(_docHandle, _id, ref ret, ref ecb), ecb);
            return ret;
        }
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
        /// <returns></returns>
        public long getFileSize() => _size;
        public string ID => _id;
        public string Name => _name;
        public long FileSize => _size;
        public System.DateTime FileDate => getFileTime();

    }
}
