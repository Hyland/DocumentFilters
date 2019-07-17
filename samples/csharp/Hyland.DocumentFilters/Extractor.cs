//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Hyland.DocumentFilters
{
    [Flags]
    public enum OpenType
    {
        BodyOnly = ISYS11dfConstants.IGR_BODY_ONLY,
        MetaOnly = ISYS11dfConstants.IGR_META_ONLY,
        BodyAndMeta = ISYS11dfConstants.IGR_BODY_AND_META,
        FormatText = ISYS11dfConstants.IGR_FORMAT_TEXT,
        FormatHTML = ISYS11dfConstants.IGR_FORMAT_HTML,
        FormatXML = ISYS11dfConstants.IGR_FORMAT_XML,
        FormatHTMLIfYouCan = ISYS11dfConstants.IGR_FORMAT_HDHTML_IFYOUCAN,
        FormatImage = ISYS11dfConstants.IGR_FORMAT_IMAGE,
        FormatHTMLNoFallback = ISYS11dfConstants.IGR_FORMAT_HTML_NO_FALLBACK,
    }
    public enum OpenMode
    {
        Text,
        Paginated,
        ClassicHTML
    }
    public class Extractor : DocumentFiltersBase, IDisposable
    {
        private IGRStream _stream = null;
        private System.IntPtr _streamHandle = IntPtr.Zero;
        private int _handle = 0;
        private OpenType _flags = 0;
        private int _capabilities = 0;
        private int _filetype = 0;
        private bool _eof = false;
        private int _pageIndex = 0;
        private int _pageCount = 0;
        private GCHandle _bytes;
        private uint _bytesLength;
        internal delegate SubFile SubfileDelegate();
        /// <summary>
        /// Returns an enumerable set of pages
        /// </summary>
        public ReadOnlyList<Page> Pages => new PageCollection(this);
        public IEnumerable<SubFile> SubFiles => new SubFileCollection(this, ()=>this.GetFirstSubFile(), ()=>this.GetNextSubFile());
        public IEnumerable<SubFile> Images => new SubFileCollection(this, ()=>this.GetFirstImage(), ()=>this.GetNextImage());

        public Page FirstPage => GetFirstPage();
        public Page NextPage => GetNextPage();
        public SubFile FirstSubFile => GetFirstSubFile();
        public SubFile NextSubFile => GetNextSubFile();
        public int Handle => NeedHandle();
        public int PageCount => GetPageCount();
        public int FileType => getFileType();
        public int Capabilities => getFileCapabilities();
        public string HashMD5 => getHashMD5();
        public string HashSHA1 => getHashSHA1();
        public bool EndOfStream => getEOF();

        internal Extractor(DocumentFiltersBase parent)
        {

        }
        internal Extractor(DocumentFiltersBase parent, string filename)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            Check(ISYS11df.IGR_Make_Stream_From_File(filename, 0, ref _streamHandle, ref ecb), ecb);
        }
        internal Extractor(DocumentFiltersBase parent, IGRStream stream)
        {
            _stream = stream;
            _streamHandle = stream.NewStreamPtr();
        }
        internal Extractor(DocumentFiltersBase parent, byte[] byteArray)
        {
            _bytes = GCHandle.Alloc(byteArray, GCHandleType.Pinned);
            _bytesLength = (uint)byteArray.Length;

            Error_Control_Block ecb = new Error_Control_Block();
            Check(ISYS11df.IGR_Make_Stream_From_Memory(_bytes.AddrOfPinnedObject(), new System.IntPtr(_bytesLength), IntPtr.Zero, ref _streamHandle, ref ecb), ecb);
        }
        internal Extractor(DocumentFiltersBase parent, System.IO.Stream stream)
        {
            _stream = new IGRStreamBridge(stream);
            _streamHandle = _stream.NewStreamPtr();
        }
        ~Extractor()
        {
            Dispose();
        }
        public virtual void Dispose()
        {
            lock (this)
            {
                Close();
                if (_bytes != null && _bytes.IsAllocated)
                {
                    _bytes.Free();
                }
            }
        }
        protected virtual System.IntPtr ResolveStreamPtr()
        {
            return _streamHandle;
        }
        protected System.IntPtr NeedStream()
        {
            if (_streamHandle == IntPtr.Zero)
            {
                _streamHandle = ResolveStreamPtr();
            }
            if (_streamHandle != IntPtr.Zero)
            {
                return _streamHandle;
            }
            else
            {
                throw new IGRException(4, "Stream is not available");
            }
        }
        protected int NeedHandle()
        {
            if (_handle <= 0)
            {
                Open(OpenMode.Text, OpenType.BodyAndMeta);
            }
            if (_handle > 0)
            {
                return _handle;
            }
            else
            {
                throw new IGRException(4, "Handle is not available");
            }
        }

        void global::System.IDisposable.Dispose()
        {
            Close();
        }
        virtual public void Close(bool closeStream = true)
        {
            if (_handle > 0)
            {
                Check(ISYS11df.IGR_Close_File(_handle, ref ecb));
                _handle = 0;
            }
            if (closeStream && _streamHandle != IntPtr.Zero)
            {
                IGRStream.DestroyStreamPtr(_streamHandle);
                _streamHandle = IntPtr.Zero;
            }

        }
        public void Open(OpenType flags, string options = "")
        {
            if (_handle > 0)
            {
                Close(false);
            }

            Check(ISYS11df.IGR_Open_Stream_Ex(NeedStream(), (int)flags, options, ref _capabilities, ref _filetype, ref _handle, ref ecb), ecb);
            if (((int)(flags) & 0xffff0000) == (int)OpenType.FormatImage)
                Check(ISYS11df.IGR_Get_Page_Count(_handle, ref _pageCount, ref ecb));
            _flags = flags;
        }
        public void Open(OpenMode mode, OpenType flags = 0, string options = "")
        {
            unchecked
            {
                if (mode == OpenMode.Paginated)
                    flags = (flags & ~((OpenType)0xffff0000)) | OpenType.FormatImage;
                else if (mode == OpenMode.ClassicHTML)
                    flags = (flags & ~((OpenType)0xffff0000)) | OpenType.FormatHTML;
                else if (mode == OpenMode.Text)
                    flags = (flags & ~((OpenType)0xffff0000)) | OpenType.FormatText;
            }
            Open(flags, options);
        }
        public void Open(int flags, string options = "")
        {
            Open((OpenType)flags, options);
        }

        /// <summary>
        /// Returns the file-type code for the document
        /// </summary>
        /// <returns></returns>
        public int getFileType()
        {
            if (_handle != 0)
            {
                return _filetype;
            }
            else
            {
                Error_Control_Block ecb = new Error_Control_Block();
                int caps = 0, filetype = 0;
                Check(ISYS11df.IGR_Get_Stream_Type(NeedStream(), ref caps, ref filetype, ref ecb), ecb);
                return filetype;
            }
        }

        /// <summary>
        /// Returns the capabilities mask for the document
        /// </summary>
        /// <returns></returns>
        public int getFileCapabilities()
        {
            if (_handle != 0)
            {
                return _capabilities;
            }
            else
            {
                Error_Control_Block ecb = new Error_Control_Block();
                int caps = 0, filetype = 0;
                Check(ISYS11df.IGR_Get_Stream_Type(NeedStream(), ref caps, ref filetype, ref ecb), ecb);
                return caps;
            }
        }

        /// <summary>
        /// Returns the file-type information for the document
        /// </summary>
        /// <param name="what"></param>
        /// <returns></returns>
        public string getFileType(IGRFormatWhat what)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder buffer = new StringBuilder(255);

            Check(ISYS11df.IGR_Get_Format_Attribute(getFileType(), (int)what, buffer, ref ecb), ecb);
            return buffer.ToString();
        }

        /// <summary>
        /// Indicates whether the document supports text extraction
        /// </summary>
        /// <returns></returns>
        public bool getSupportsText()
        {
            return (getFileCapabilities() & ISYS11dfConstants.IGR_FILE_SUPPORTS_TEXT) != 0;
        }

        /// <summary>
        /// Indicates whether the document supports sub-file extraction
        /// </summary>
        /// <returns></returns>
        public bool getSupportsSubFiles()
        {
            return (getFileCapabilities() & ISYS11dfConstants.IGR_FILE_SUPPORTS_SUBFILES) != 0;
        }

        /// <summary>
        /// Indicates whether the document supports HD conversion
        /// </summary>
        /// <returns></returns>
        public bool getSupportsHTML()
        {
            return (getFileCapabilities() & ISYS11dfConstants.IGR_FILE_SUPPORTS_HDHTML) != 0;
        }

        /// <summary>
        /// The EOF property is only valid for documents where the SupportsText property is TRUE. The EOF property will be 
        /// set to TRUE when no more text can be extracted from the document with calls to GetText. If the document needs to 
        /// be re-read, call Close and Open first.
        /// </summary>
        /// <returns></returns>
        public bool getEOF()
        {
            return _eof;
        }

        /// <summary>
        /// The GetText method extracts the next portion of text content from the document.
        /// </summary>
        /// <param name="maxLength"></param>
        /// <returns></returns>
        public string GetText(int maxLength)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder buffer = new StringBuilder(maxLength);
            int retval = maxLength;

            Check(ISYS11df.IGR_Get_Text(NeedHandle(), buffer, ref retval, ref ecb), ecb);
            _eof = retval == 0;
            return buffer.ToString().Substring(0, retval);
        }

        /// <summary>
        /// The SaveTo method extracts the entire text content of the document in a single call and writes it to the destination stream using the specified encoding.
        /// </summary>
        /// <param name="stream">Destination stream</param>
        /// <param name="encoding">Text encoding method</param>
        public void SaveTo(System.IO.Stream stream, System.Text.Encoding encoding)
        {
            VerifyArgumentNotNull(stream, "stream");
            using (System.IO.StreamWriter writer = new System.IO.StreamWriter(stream, encoding))
            {
                while (!getEOF())
                {
                    string text = GetText(1024);
                    writer.Write(text);
                }
            }
        }

        /// <summary>
        /// The SaveTo method extracts the entire text content of the document in a single call and writes it to the destination stream as UTF-8
        /// </summary>
        /// <param name="stream">Destination stream</param>
        public void SaveTo(System.IO.Stream stream)
        {
            SaveTo(stream, Encoding.UTF8);
        }

        /// <summary>
        /// The SaveTo method extracts the entire text content of the document in a single call and writes it to the destination file with the given encoding
        /// </summary>
        /// <param name="filename">The filename to create, existing files will be overwitten</param>
        /// <param name="encoding">The text encoding method</param>
        public void SaveTo(string filename, System.Text.Encoding encoding)
        {
            SaveTo(filename, Encoding.UTF8);
        }

        /// <summary>
        /// The SaveTo method extracts the entire text content of the document in a single call and writes it to the destination file as UTF-8
        /// </summary>
        /// <param name="filename">The filename to create, existing files will be overwitten</param>
        public void SaveTo(string filename)
        {
            VerifyArgumentNotEmpty(filename, "filename");
            using (System.IO.FileStream stream = new System.IO.FileStream(filename, System.IO.FileMode.Create))
            {
                SaveTo(stream);
            }
        }

        /// <summary>
        /// The getHashMD5 method obtain a string representing the calculated hash of the current document for unique identification.
        /// </summary>
        /// <returns></returns>
        public string getHashMD5()
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder buffer = new StringBuilder(33);

            Check(ISYS11df.IGR_Calculate_MD5(NeedStream(), buffer, ref ecb), ecb);
            return buffer.ToString();
        }

        /// <summary>
        /// The getHashSHA1 method obtain a string representing the calculated hash of the current document for unique identification.
        /// </summary>
        /// <returns></returns>
        public string getHashSHA1()
        {
            StringBuilder buffer = new StringBuilder(41);

            Check(ISYS11df.IGR_Calculate_SHA1(NeedStream(), buffer, ref ecb), ecb);
            return buffer.ToString();
        }

        /// <summary>
        /// The GetFirstSubFile and GetFirstImage methods obtains a SubFile object representing the first sub-document or attached image (if using HTML conversion) of the current document.
        /// </summary>
        /// <returns></returns>
        public SubFile GetFirstSubFile()
        {
            return GetNextSubFile();
        }

        /// <summary>
        /// The GetFirstSubFile and GetFirstImage methods obtains a SubFile object representing the first sub-document or attached image (if using HTML conversion) of the current document.
        /// </summary>
        /// <returns></returns>
        public SubFile GetNextSubFile()
        {
            return GetNext(NeedHandle(), ISYS11df.IGR_Get_Subfile_Entry, ISYS11df.IGR_Extract_Subfile_Stream);
        }

        /// <summary>
        /// The GetSubFile method obtains a SubFile object representing the nominated sub-file of the current document.
        /// </summary>
        /// <param name="id"></param>
        /// <returns></returns>
        public SubFile GetSubFile(string id)
        {
            VerifyArgumentNotEmpty(id, "id");
            return new SubFile(this, NeedHandle(), id, null, 0, 0, ISYS11df.IGR_Extract_Subfile_Stream);
        }

        protected SubFile GetNext(int handle, IGR_Get_Entry GetNext, IGR_Extract_Stream extractor)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder id = new StringBuilder(4096);
            StringBuilder name = new StringBuilder(1024);
            long date = 0, size = 0;
            if (Check(GetNext(handle, id, name, ref date, ref size, ref ecb), ecb) != ISYS11dfConstants.IGR_NO_MORE)
            {
                return new SubFile(this, handle, id.ToString(), name.ToString(), date, size, extractor);
            }
            return (SubFile)null;
        }

        /// <summary>
        /// The GetFirstImage method obtains a SubFile object representing the next sub-document or attached image (if using HTML conversion) of the current document.
        /// </summary>
        /// <returns></returns>
        public SubFile GetFirstImage()
        {
            return GetNextImage();
        }

        /// <summary>
        /// The GetNextImage method obtains a SubFile object representing the next sub-document or attached image (if using HTML conversion) of the current document.
        /// </summary>
        /// <returns></returns>
        public SubFile GetNextImage()
        {
            return GetNext(NeedHandle(), ISYS11df.IGR_Get_Image_Entry, ISYS11df.IGR_Extract_Image_Stream);
        }

        /// <summary>
        /// The GetImage method obtains a SubFile object representing the nominated sub-file of the current document.
        /// </summary>
        /// <param name="id"></param>
        /// <returns></returns>
        public SubFile GetImage(string id)
        {
            VerifyArgumentNotEmpty(id, "id");
            return new SubFile(this, NeedHandle(), id, null, 0, 0, ISYS11df.IGR_Extract_Image_Stream);
        }


        /// <summary>
        /// The CopyTo method extracts the binary content of the sub-document to a file.
        /// </summary>
        /// <param name="dest">The destination stream</param>
        public void CopyTo(System.IO.Stream dest)
        {
            VerifyArgumentNotNull(dest, "dest");
            using (var src = new StreamBridge(NeedStream()))
            {
                src.Seek(0, System.IO.SeekOrigin.Begin);

                byte[] buffer = new byte[1024];
                int len;
                
                while((len = src.Read(buffer, 0, buffer.Length)) > 0)
                {
                    dest.Write(buffer, 0, len);
                }
            }
        }

        /// <summary>
        /// The CopyTo method extracts the binary content of the sub-document to a file.
        /// </summary>
        /// <param name="filename">The name of the file to create, existing file will be overwritten.</param>
        public void CopyTo(string filename)
        {
            VerifyArgumentNotEmpty(filename, "filename");
            using (var dest = new System.IO.FileStream(filename, System.IO.FileMode.Create))
            {
                CopyTo(dest);
            }
        }

        /// <summary>
        /// The GetFirstPage methods enumerate over the pages of an opened document. Alternativily, use Pages enumeration.
        /// </summary>
        /// <returns></returns>
        public Page GetFirstPage()
        {
            _pageIndex = 0;
            return GetNextPage();
        }

        /// <summary>
        /// The GetNextPage methods enumerate over the pages of an opened document. Alternativily, use Pages enumeration.
        /// </summary>
        /// <returns></returns>
        public Page GetNextPage()
        {
            if (_pageIndex < _pageCount)
            {
                Error_Control_Block ecb = new Error_Control_Block();
                int pageHandle = 0;
                Check(ISYS11df.IGR_Open_Page(NeedHandle(), _pageIndex, ref pageHandle, ref ecb), ecb);

                _pageIndex++;

                return new Page(NeedHandle(), pageHandle);
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// The GetPage method returns the page at the given index, where the page index is 0-based. An exception is raised if the index is invalid.
        /// </summary>
        /// <param name="page"></param>
        /// <returns></returns>
        public Page GetPage(int page)
        {
            VerifyArgumentInRange(page, 0, _pageCount - 1, "page");
            Error_Control_Block ecb = new Error_Control_Block();
            int pageHandle = 0;
            Check(ISYS11df.IGR_Open_Page(NeedHandle(), page, ref pageHandle, ref ecb), ecb);

            return new Page(NeedHandle(), pageHandle);
        }

        /// <summary>
        /// Returns the number of pages in the current document, the document must be opened in image mode for the page count to be populated.
        /// </summary>
        /// <returns></returns>
        public int GetPageCount()
        {
            NeedHandle();
            return _pageCount;
        }

        internal class PageCollection : ReadOnlyList<Page>
        {
            private Extractor _extractor;

            public PageCollection(Extractor extractor)
            {
                _extractor = extractor;
            }

            protected override int GetCount()
            {
                return _extractor.GetPageCount();
            }

            protected override Page GetItem(int index)
            {
                VerifyArgumentInRange(index, 0, GetCount(), "index");
                return _extractor.GetPage(index);
            }
        }

        internal class SubFileEnumerator : IEnumerator<SubFile>
        {
            private readonly Object _extractor;
            private SubFile _current;
            private bool _is_first;
            private readonly SubfileDelegate _first, _next;

            internal SubFileEnumerator(Object extractor, SubfileDelegate first, SubfileDelegate next)
            {
                _extractor = extractor;
                _first = first;
                _next = next;
                _is_first = true;
            }
            public SubFile Current => _current;

            object IEnumerator.Current => _current;

            public void Dispose()
            {
            }

            public bool MoveNext()
            {
                if (_is_first)
                {
                    _current = _first();
                    _is_first = false;
                }
                else
                {
                    _current = _next();
                }
                return _current != null;
            }

            public void Reset()
            {
                _is_first = true;
            }
        }

        internal class SubFileCollection : IEnumerable<SubFile>
        {
            private readonly Object _extractor;
            private readonly SubfileDelegate _first, _next;

            internal SubFileCollection(Object extractor, SubfileDelegate first, SubfileDelegate next)
            {
                _extractor = extractor;
                _first = first;
                _next = next;
            }

            public IEnumerator<SubFile> GetEnumerator()
            {
                return new SubFileEnumerator(_extractor, _first, _next);
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return new SubFileEnumerator(_extractor, _first, _next);
            }
        }
    }
}
