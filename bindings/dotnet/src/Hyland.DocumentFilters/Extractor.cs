//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// The OpenType enumeration specifies the type of content to extract from a document.
    /// </summary>
    [Flags]
    public enum OpenType
    {
        /// <summary>
        /// Indicates that only the body content should be extracted.
        /// </summary>
        BodyOnly = ISYS11dfConstants.IGR_BODY_ONLY,

        /// <summary>
        /// Indicates that only the metadata should be extracted.
        /// </summary>
        MetaOnly = ISYS11dfConstants.IGR_META_ONLY,

        /// <summary>
        /// Indicates that both the body and metadata should be extracted.
        /// </summary>
        BodyAndMeta = ISYS11dfConstants.IGR_BODY_AND_META,

        /// <summary>
        /// Indicates that the content should be extracted as text.
        /// </summary>
        FormatText = ISYS11dfConstants.IGR_FORMAT_TEXT,

        /// <summary>
        /// Indicates that the content should be extracted as HTML.
        /// </summary>
        FormatHTML = ISYS11dfConstants.IGR_FORMAT_HTML,

        /// <summary>
        /// Indicates that the content should be extracted as XML.
        /// </summary>
        FormatXML = ISYS11dfConstants.IGR_FORMAT_XML,

        /// <summary>
        /// Indicates that the content should be extracted as HTML with fallback to text.
        /// </summary>
        FormatHTMLIfYouCan = ISYS11dfConstants.IGR_FORMAT_HDHTML_IFYOUCAN,

        /// <summary>
        /// Indicated that the content should be extracted as an paginated/image.
        /// </summary>
        FormatImage = ISYS11dfConstants.IGR_FORMAT_IMAGE,

        /// <summary>
        /// Indicates that the content should be extracted as HTML with no fallback to text.
        /// </summary>
        FormatHTMLNoFallback = ISYS11dfConstants.IGR_FORMAT_HTML_NO_FALLBACK,
    }

    /// <summary>
    /// The OpenMode enumeration specifies the mode in which to open a document.
    /// </summary>
    public enum OpenMode
    {
        /// <summary>
        /// Indicates that the document should be opened in text mode.
        /// </summary>
        Text,

        /// <summary>
        /// Indicates that the document should be opened in paginated mode.
        /// </summary>
        Paginated,

        /// <summary>
        /// Indicates that the document should be opened in classic HTML mode.
        /// </summary>
        ClassicHTML
    }

    /// <summary>
    /// The Extractor interface allows you to extract the content of a document and/or enumerate its sub-documents, such as email attachments and ZIP archives.
    /// 
    /// To obtain this interface, call the DocumentFilters.GetExtractor method.
    /// </summary>
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
        internal delegate IntPtr SubfileEnumeratorCreate();
        private Bookmark _rootBookmark;
        private Func<OpenCallback, int> _userCallback = null;
        private GCHandle _callbackHandle;
        private IEnumerator<SubFile> _subfileEnumerator;
        
        private IDictionary<string, string> _localized = new Dictionary<string, string>();

        /// <summary>
        /// Returns an enumerable set of pages.
        /// </summary>
        public ReadOnlyList<Page> Pages => new PageCollection(this);

        /// <summary>
        /// Returns an enumerable set of subfiles.
        /// </summary>
        public IEnumerable<SubFile> SubFiles => CreateSubFileEnumeration();

        /// <summary>
        /// Returns an enumerable set of images.
        /// </summary>
        public IEnumerable<SubFile> Images => CreateImageEnumeration();

        /// <summary>
        /// Returns the first page of the document. Returns null if there are no pages.
        /// </summary>
        public Page FirstPage => GetFirstPage();

        /// <summary>
        /// Returns the next page of the document. If there are no more pages, returns null.
        /// </summary>
        /// <remarks>
        /// FirstPage/NextPage properties store the current page index while enumerating pages. To support multiple enumerations, use the Pages property.
        /// </remarks>
        public Page NextPage => GetNextPage();

        /// <summary>
        /// Returns the first subfile of the document. Returns null if there are no subfiles.
        /// </summary>
        public SubFile FirstSubFile => GetFirstSubFile();

        /// <summary>
        /// Returns the next subfile of the document. If there are no more subfiles, returns null.
        /// </summary>
        public SubFile NextSubFile => GetNextSubFile();

        /// <summary>
        /// Returns the low-level handle to the document that can be used with the native API.
        /// </summary>
        public int Handle => NeedHandle();

        /// <summary>
        /// Indicates whether the document has been opened.
        /// </summary>
        public bool HasHandle => _handle > 0;

        /// <summary>
        /// Returns the number of pages in the document.
        /// </summary>
        public int PageCount => GetPageCount();

        /// <summary>
        /// Returns the file-type code for the document.
        /// </summary>
        public int FileType => getFileType();

        /// <summary>
        /// Returns the capabilities mask for the document.
        /// </summary>
        public int Capabilities => getFileCapabilities();

        /// <summary>
        /// Returns the MD5 hash of the binary content of the document.
        /// </summary>
        public string HashMD5 => getHashMD5();

        /// <summary>
        /// Returns the SHA1 hash of the binary content of the document.
        /// </summary>
        public string HashSHA1 => getHashSHA1();

        /// <summary>
        /// Indicates whether there is more text to extract from the document.
        /// </summary>
        public bool EndOfStream => getEOF();

        /// <summary>
        /// Returns the MIME type of the file.
        /// </summary>
        public string MimeType => getFileType(IGRFormatWhat.IGR_FORMAT_MIMETYPE);

        /// <summary>
        /// Provides a dictionary that can be used to localize the metadata strings in the document.
        /// </summary>
        public IDictionary<string, string> Localize => _localized;

        /// <summary>
        /// Provides a function that can be used to localize the metadata strings in the document.
        /// </summary>
        public Func<uint, string, string> Localizer { get; set; }

        /// <summary>
        /// Internal constructor for Extractor
        /// </summary>
        /// <param name="parent">Parent item if creating an extractor of a subfile.</param>
        internal Extractor(DocumentFiltersBase parent)
        {
        }

        /// <summary>
        /// Internal constructor for Extractor
        /// </summary>
        /// <param name="parent">Parent item if creating an extractor of a subfile.</param>
        /// <param name="filename">Filename of resource on disk to open.</param>
        internal Extractor(DocumentFiltersBase parent, string filename)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            Check(ISYS11df.IGR_Make_Stream_From_File(filename, 0, ref _streamHandle, ref ecb), ecb);
        }

        /// <summary>
        /// Internal constructor for Extractor
        /// </summary>
        /// <param name="parent">Parent item if creating an extractor of a subfile.</param>
        /// <param name="stream">IGRStream containing the binary contents of the file to process.</param>
        internal Extractor(DocumentFiltersBase parent, IGRStream stream)
        {
            _stream = stream;
            _streamHandle = stream.NewStreamPtr();
        }

        /// <summary>
        /// Internal constructor for Extractor
        /// </summary>
        /// <param name="parent">Parent item if creating an extractor of a subfile.</param>
        /// <param name="byteArray">Byte array containing the binary contents of the file to process.</param>
        internal Extractor(DocumentFiltersBase parent, byte[] byteArray)
        {
            _bytes = GCHandle.Alloc(byteArray, GCHandleType.Pinned);
            _bytesLength = (uint)byteArray.Length;

            Error_Control_Block ecb = new Error_Control_Block();
            Check(ISYS11df.IGR_Make_Stream_From_Memory(_bytes.AddrOfPinnedObject(), new System.IntPtr(_bytesLength), IntPtr.Zero, ref _streamHandle, ref ecb), ecb);
        }

        /// <summary>
        /// Internal constructor for Extractor
        /// </summary>
        /// <param name="parent">Parent item if creating an extractor of a subfile.</param>
        /// <param name="stream">Stream containing the binary contents of the file to process.</param>
        internal Extractor(DocumentFiltersBase parent, System.IO.Stream stream)
        {
            _stream = new IGRStreamBridge(stream);
            _streamHandle = _stream.NewStreamPtr();
        }

        /// <summary>
        /// Extractor Destructor
        /// </summary>
        ~Extractor()
        {
            Dispose();
        }

        /// <summary>
        /// Dispose of the Extractor and the underlying resources.
        /// </summary>
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

        /// <summary>
        /// Called the resolve the stream pointer to be used by the extractor.
        /// </summary>
        /// <returns></returns>
        protected virtual System.IntPtr ResolveStreamPtr()
        {
            return _streamHandle;
        }

        /// <summary>
        /// Called when the underlying stream is needed by the extractor.
        /// </summary>
        /// <returns>The stream handle</returns>
        /// <exception cref="IGRException">IGRException is thrown if the stream cannot be resolved.</exception>
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

        /// <summary>
        /// Called when the DF handle is needed by the extractor.
        /// </summary>
        /// <param name="mode">The processing mode requested.</param>
        /// <returns>Returns the IGR_HDOC.</returns>
        /// <exception cref="IGRException"></exception>
        protected int NeedHandle(OpenMode mode = OpenMode.Text)
        {
            if (_handle <= 0)
            {
                Open(mode, OpenType.BodyAndMeta);
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

        /// <summary>
        /// IDisposable implementation
        /// </summary>
        void global::System.IDisposable.Dispose()
        {
            Close();
        }

        /// <summary>
        /// Called to close the IGR_HDOC handle, and optionally the stream.
        /// </summary>
        /// <param name="closeStream">Indicate if the stream should also be disposed.</param>
        virtual public void Close(bool closeStream = true)
        {
            if (_subfileEnumerator != null)
            {
                _subfileEnumerator.Dispose();
                _subfileEnumerator = null;
            }
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
            if (_callbackHandle.IsAllocated)
            {
                _callbackHandle.Free();
            }
            _userCallback = null;
        }

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="flags">Specify the open-mode flags.</param>
        /// <param name="options">Semicolon separated list of name value pair options.</param>
        /// <param name="callback">Provide a user callback that is called during various operations.</param>
        public void Open(OpenType flags, string options, System.Func<OpenCallback, int> callback)
        {
            if (_handle > 0)
            {
                Close(false);
            }
            IntPtr cbPtr = IntPtr.Zero;
            _userCallback = callback;

            if (callback != null || _localized.Count > 0 || Localizer != null)
            {
                IGR_CALLBACK cb = new IGR_CALLBACK(Open_Callback);
                cbPtr = Marshal.GetFunctionPointerForDelegate(cb);

                if (_callbackHandle.IsAllocated)
                {
                    _callbackHandle.Free();
                }

                //save the handle to the callback to keep the delegate from being collected
                //by the garbage collector from managed code.
                //The garbage collector does not track references to unmanaged code.
                //It will be freed when the extractor is closed
                //https://learn.microsoft.com/en-us/cpp/dotnet/how-to-marshal-callbacks-and-delegates-by-using-cpp-interop?view=msvc-170#example-function-pointer-stored-by-unmanaged-api
                _callbackHandle = GCHandle.Alloc(cb);
            }
            //native parameter marshalling marshals a delegate to a native function pointer
            //so no need for explicit Marshal.GetFuntionPointerForDelegate
            //we use the function pointer here though instead of Open_Callback directly 
            //for explicit reference t tohe saved _callbackHandle reference
            //https://learn.microsoft.com/en-us/dotnet/standard/native-interop/type-marshalling
            Check(ISYS11df.IGR_Open_Ex(ISYS11dfConstants.IGR_OPEN_FROM_STREAM, NeedStream(), (int)flags, options, ref _capabilities, ref _filetype, IntPtr.Zero, cbPtr, IntPtr.Zero, ref _handle, ref ecb), ecb); ;
            
            if (((int)(flags) & 0xffff0000) == (int)OpenType.FormatImage)
                Check(ISYS11df.IGR_Get_Page_Count(_handle, ref _pageCount, ref ecb));
            _flags = flags;
        }

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="mode">Indicate the processing mode for the document.</param>
        /// <param name="flags">Specify the open-mode flags.</param>
        /// <param name="options">Semicolon separated list of name value pair options.</param>
        /// <param name="callback">Provide a user callback that is called during various operations.</param>
        public void Open(OpenMode mode, OpenType flags = 0, string options = "", Func<OpenCallback, int> callback = null)
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
            Open(flags, options, callback);
        }

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="flags">Specify the open-mode flags.</param>
        /// <param name="options">Semicolon separated list of name value pair options.</param>
        /// <param name="callback">Provide a user callback that is called during various operations.</param>
        public void Open(int flags, string options = "", Func<OpenCallback, int> callback = null)
        {
            Open((OpenType)flags, options, callback);
        }

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="flags">Specify the open-mode flags.</param>
        /// <param name="callback">Provide a user callback that is called during various operations.</param>
        public void Open(int flags, System.Func<OpenCallback, int> callback)
        {
            Open((OpenType)flags, "", callback);
        }

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="flags">Specify the open-mode flags.</param>
        /// <param name="callback">Provide a user callback that is called during various operations.</param>
        public void Open(OpenType flags, System.Func<OpenCallback, int> callback) => Open(flags, "", callback: callback);

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="flags">Specify the open-mode flags.</param>
        /// <param name="options">Semicolon separated list of name value pair options.</param>
        public void Open(OpenType flags, string options = "") => Open(flags, options, callback: null);

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="mode">Indicate the processing mode for the document.</param>
        /// <param name="flags">Specify the open-mode flags.</param>
        /// <param name="options">Semicolon separated list of name value pair options.</param>
        public void Open(OpenMode mode, OpenType flags, string options) => Open(mode, flags, options, callback: null);

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="flags">Specify the open-mode flags.</param>
        /// <param name="options">Semicolon separated list of name value pair options.</param>
        public void Open(int flags, string options) => Open(flags, options, callback: null);

        /// <summary>
        /// Open the document for processing.
        /// </summary>
        /// <param name="flags">Specify the open-mode flags.</param>
        public void Open(int flags) => Open(flags, "", callback: null);
        
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
        /// <param name="maxLength">The maximum number of UTF-16 characters to read.</param>
        /// <returns>A unicode string containing the requested content.</returns>
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
        /// <returns>Returns a string containing the Hash.</returns>
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
        /// <returns>Returns a string containing the Hash.</returns>
        public string getHashSHA1()
        {
            StringBuilder buffer = new StringBuilder(41);

            Check(ISYS11df.IGR_Calculate_SHA1(NeedStream(), buffer, ref ecb), ecb);
            return buffer.ToString();
        }

        /// <summary>
        /// The GetFirstSubFile and GetFirstImage methods obtains a SubFile object representing the first sub-document or attached 
        /// image (if using HTML conversion) of the current document.
        /// </summary>
        /// <returns>Returns a SubFile if more files are available, otherwise null.</returns>
        public SubFile GetFirstSubFile()
        {
            if (_subfileEnumerator == null)
                _subfileEnumerator = CreateSubFileEnumeration().GetEnumerator();
            else
                _subfileEnumerator.Reset();

            return GetNextSubFile();
        }

        /// <summary>
        /// The GetFirstSubFile and GetFirstImage methods obtains a SubFile object representing the first sub-document 
        /// or attached image (if using HTML conversion) of the current document.
        /// </summary>
        /// <returns>Returns a SubFile if more files are available, otherwise null.</returns>
        public SubFile GetNextSubFile()
        {
            if (_subfileEnumerator == null)
                _subfileEnumerator = CreateSubFileEnumeration().GetEnumerator();

            if (_subfileEnumerator.MoveNext())
                return _subfileEnumerator.Current;
            else
                return null;
        }

        /// <summary>
        /// The GetSubFile method obtains a SubFile object representing the nominated sub-file of the current document.
        /// </summary>
        /// <param name="id">An ID that was previously returned when enumerating sub files with GetFirstSubFile and GetNextSubFile.</param>
        /// <returns>A SubFile object for the nominated sub-document, or NULL if the document is not found.</returns>
        public SubFile GetSubFile(string id)
        {
            VerifyArgumentNotEmpty(id, "id");
            return new SubFile(this, NeedHandle(), id, null, 0, 0, ISYS11df.IGR_Extract_Subfile_Stream);
        }

        /// <summary>
        /// Internal method to obtain the next subfile or image from the document.
        /// </summary>
        /// <param name="handle">The IGR_HDOC of the document.</param>
        /// <param name="GetNext">The C api to call to enumerate.</param>
        /// <param name="extractor">The C api to call to extract.</param>
        /// <returns>A SubFile object, or null if no more are available.</returns>
        protected SubFile GetNext(int handle, IGR_Get_Entry GetNext, IGR_Extract_Stream extractor)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder id = new StringBuilder(4096);
            StringBuilder name = new StringBuilder(1024);
            long date = 0, size = 0;
            if (Check(GetNext(handle, id, name, ref date, ref size, ref ecb), ecb) != ISYS11dfConstants.IGR_NO_MORE)
            {
                return new SubFile(this, handle, id.ToString(), name.ToString(), size, date, extractor);
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

        /// <summary>
        /// Creates a DocPack from the current document and writes it to the provided stream
        /// </summary>
        /// <param name="destination">The destination stream the DocPack will be written to.</param>
        /// <param name="options">The options to use when requesting this DocPack</param>
        public void MakeDocPack(System.IO.Stream destination, string options)
        {
            IGRStreamBridge bridge = new IGRStreamBridge(destination);
            Error_Control_Block nkotb = new Error_Control_Block();
            Check(ISYS11df.IGR_Make_DocPack_On(NeedHandle(), bridge.NewWritableStreamPtr(), options, ref nkotb), nkotb);
        }

        /// <summary>
        /// Creates a DocPack from the current document and writes it to the provided file path.
        /// </summary>
        /// <param name="filename">The destination file location the DocPack will be written to.</param>
        /// <param name="options">The options to use when requesting this DocPack</param>
        public void MakeDocPack(string filename, string options)
        {
            Error_Control_Block nkotb = new Error_Control_Block();
            Check(ISYS11df.IGR_Make_DocPack(NeedHandle(), filename, options, ref nkotb), nkotb);
        }

        /// <summary>
        /// Create an enumeration of subfiles.
        /// </summary>
        /// <returns>An IEnumerable object of SubFiles.</returns>
        private IEnumerable<SubFile> CreateSubFileEnumeration()
        {
            return new SubFileEnumeratorCollection(this, NeedHandle(), () => CreateSubFileEnumerator(), ISYS11df.IGR_Extract_Subfile_Stream, GetFirstSubFile, GetNextSubFile);
        }

        /// <summary>
        /// Internal method to create an enumerator for subfiles in the C API.
        /// </summary>
        /// <returns>A new HENUMERATOR.</returns>
        private IntPtr CreateSubFileEnumerator()
        {
            IntPtr result = IntPtr.Zero;
            Error_Control_Block ecb = new Error_Control_Block();
            Check(ISYS11df.IGR_Get_Subfiles_Enumerator(NeedHandle(), ref result, ref ecb), ecb);
            return result;
        }

        /// <summary>
        /// Create an enumeration of images.
        /// </summary>
        /// <returns>An IEnumerable object of SubFiles.</returns>
        private IEnumerable<SubFile> CreateImageEnumeration()
        {
            return new SubFileEnumeratorCollection(this, NeedHandle(), () => CreateImageEnumerator(), ISYS11df.IGR_Extract_Image_Stream, GetFirstImage, GetNextImage);
        }

        /// <summary>
        /// Internal method to create an enumerator for images in the C API.
        /// </summary>
        /// <returns>A new HENUMERATOR.</returns>
        private IntPtr CreateImageEnumerator()
        {
            IntPtr result = IntPtr.Zero;
            Error_Control_Block ecb = new Error_Control_Block();
            Check(ISYS11df.IGR_Get_Images_Enumerator(NeedHandle(), ref result, ref ecb), ecb);
            return result;
        }

        /// <summary>
        /// Internal implementation of the IGR_Open_Ex callback.
        /// </summary>
        /// <param name="actionID">The action to be processed.</param>
        /// <param name="actionData">The data for the action.</param>
        /// <param name="context">The user-context passed to IGR_Open_Ex</param>
        /// <returns>Error state for the action.</returns>
        private int Open_Callback(int actionID, System.IntPtr actionData, System.IntPtr context)
        {
            int retval = -1;

            try
            {
                OpenCallback callback = new OpenCallback() { Action = actionID };

                //prepare for user's callback
                switch (actionID)
                {
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_HEARTBEAT:
                        {
                            callback.Heartbeat = Marshaler.PtrToStructure<IGR_Open_Callback_Action_Heartbeat>(actionData);
                            break;
                        }
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_PASSWORD:
                        {
                            callback.Password = Marshaler.PtrToStructure<IGR_Open_Callback_Action_Password>(actionData);
                            break;
                        }
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOCALIZE:
                        {
                            callback.Localize = Marshaler.PtrToStructure<IGR_Open_Callback_Action_Localize>(actionData);
                            break;
                        }
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOG_LEVEL:
                        {
                            callback.LogLevel = Marshaler.PtrToStructure<IGR_Open_Callback_Action_Log_Level>(actionData);
                            break;
                        }
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOG_MESSAGE:
                        {
                            callback.LogMessage = Marshaler.PtrToStructure<IGR_Open_Callback_Action_Log_Message>(actionData);
                            break;
                        }
                }

                if (actionID == ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOCALIZE)
                {
                    if (Localizer != null)
                    {
                        var res = Localizer(callback.Localize.string_id, callback.Localize.original);
                        if (!string.IsNullOrEmpty(res))
                        {
                            callback.Localize.replacement = res;
                            retval = ISYS11dfConstants.IGR_OK;
                        }
                    }
                    else if (_localized.TryGetValue(callback.Localize.original, out var res) && !string.IsNullOrEmpty(res))
                    {
                        callback.Localize.replacement = res;
                        retval = ISYS11dfConstants.IGR_OK;
                    }
                }

                //call user's callback and marshal the data back for unmanaged code
                if (_userCallback != null)
                    retval = _userCallback(callback);

                switch (actionID)
                {
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_HEARTBEAT:
                        {
                            if (callback.Heartbeat != null)
                                Marshaler.StructureToPtr<IGR_Open_Callback_Action_Heartbeat>(callback.Heartbeat, actionData, true);
                            break;
                        }
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_PASSWORD:
                        {
                            if (callback.Password != null)
                                Marshaler.StructureToPtr<IGR_Open_Callback_Action_Password>(callback.Password, actionData, true);
                            break;
                        }
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOCALIZE:
                        {
                            if (callback.Localize != null)
                                Marshaler.StructureToPtr<IGR_Open_Callback_Action_Localize>(callback.Localize, actionData, true);
                            break;
                        }
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOG_LEVEL:
                        {
                            if (callback.LogLevel != null)
                                Marshaler.StructureToPtr<IGR_Open_Callback_Action_Log_Level>(callback.LogLevel, actionData, true);
                            break;
                        }
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOG_MESSAGE:
                        {
                            if (callback.LogMessage != null)
                                Marshaler.StructureToPtr<IGR_Open_Callback_Action_Log_Message>(callback.LogMessage, actionData, true);
                            break;
                        }
                }
            }
            catch(Exception ex)
            {
                Trace.WriteLine($"Return value from user-provided Func<OpenCallback, int> cannot be interpreted as an integer. Handling as if IGR_E_ERROR was returned. Exception details: error={ex.Message}, type(error)={ex.GetType()}");
                //cannot throw exception on some platforms from within the DF callback
                return ISYS11dfConstants.IGR_E_ERROR;
            }

            return retval;
        }

        /// <summary>
        /// Returns the root bookmark for the document.
        /// </summary>
        public Bookmark RootBookmark
        {
            get
            {
                if (_rootBookmark == null)
                    _rootBookmark = new Bookmark(NeedHandle(OpenMode.Paginated), null);
                return _rootBookmark;
            }
        }

        /// <summary>
        /// Compares two documents and returns the results.
        /// </summary>
        /// <param name="original">The original or left document to compare.</param>
        /// <param name="revised">The revised or right document to compare.</param>
        /// <param name="settings">The settings to use when performing the compare.</param>
        /// <returns>A CompareResults object that contains the result of the compare.</returns>
        /// <exception cref="ArgumentException"></exception>
        public static CompareResults Compare(CompareDocumentSource original, CompareDocumentSource revised, CompareSettings settings = null)
        {
            _ = original == null || original.Extractor == null ? throw new ArgumentException("original is invalid") : original;
            _ = revised == null || revised.Extractor == null ? throw new ArgumentException("revised is invalid") : revised;

            IntPtr enumerator = IntPtr.Zero;
            IGR_Text_Compare_Settings wrappedSettings = (settings ?? new CompareSettings()).Make();
            IGR_Text_Compare_Document_Source wrappedDoc1 = original.Make();
            IGR_Text_Compare_Document_Source wrappedDoc2 = revised.Make();
            Error_Control_Block ecb = new Error_Control_Block();

            Check(ISYS11df.IGR_Text_Compare_Documents(
                ref wrappedDoc1,
                ref wrappedDoc2,
                ref wrappedSettings,
                out enumerator,
                ref ecb), ecb);

            return CompareResults.Make(enumerator);
        }

        /// <summary>
        /// Compares this document to another and returns the results.
        /// </summary>
        /// <param name="thisDoc">The settings to use for this document.</param>
        /// <param name="otherDocument">The revised or right document to compare against.</param>
        /// <param name="settings">The settings to use when performing the compare.</param>
        /// <returns>A CompareResults object that contains the result of the compare.</returns>
        public CompareResults Compare(CompareDocumentSettings thisDoc, CompareDocumentSource otherDocument, CompareSettings settings = null)
        {
            return Extractor.Compare(
                new CompareDocumentSource { Extractor = this, FirstPage = thisDoc.FirstPage, PageCount = thisDoc.PageCount, Margins = thisDoc.Margins },
                otherDocument,
                settings);
        }

        /// <summary>
        /// Compares this document to another and returns the results.
        /// </summary>
        /// <param name="thisDoc">The settings to use for this document.</param>
        /// <param name="otherDocument">The revised or right document to compare against.</param>
        /// <param name="otherDocSettings">The settings to use for the other document.</param>
        /// <param name="settings">The settings to use when performing the compare.</param>
        /// <returns>A CompareResults object that contains the result of the compare.</returns>
        public CompareResults Compare(CompareDocumentSettings thisDoc, Extractor otherDocument, CompareDocumentSettings otherDocSettings, CompareSettings settings = null)
        {
            return Extractor.Compare(
                new CompareDocumentSource { Extractor = this, FirstPage = thisDoc.FirstPage, PageCount = thisDoc.PageCount, Margins = thisDoc.Margins },
                new CompareDocumentSource { Extractor = otherDocument, FirstPage = otherDocSettings.FirstPage, PageCount = otherDocSettings.PageCount, Margins = otherDocSettings.Margins },
                settings);
        }

        /// <summary>
        /// Compares this document to another and returns the results.
        /// </summary>
        /// <param name="otherDocument">The revised or right document to compare against.</param>
        /// <param name="settings">The settings to use when performing the compare.</param>
        /// <returns>A CompareResults object that contains the result of the compare.</returns>
        public CompareResults Compare(Extractor otherDocument, CompareSettings settings = null)
        {
            return Extractor.Compare(
                new CompareDocumentSource { Extractor = this },
                new CompareDocumentSource { Extractor = otherDocument },
                settings);
        }

        /// <summary>
        /// Internal class to manage the collection of pages in a document.
        /// </summary>
        internal class PageCollection : ReadOnlyList<Page>
        {
            private Extractor _extractor;

            /// <summary>
            /// Construct a new PageCollection for the given Extractor.
            /// </summary>
            /// <param name="extractor"></param>
            public PageCollection(Extractor extractor)
            {
                _extractor = extractor;
            }

            /// <summary>
            /// Count the number of pages in the document.
            /// </summary>
            /// <returns>The number of pages.</returns>
            protected override int GetCount()
            {
                return _extractor.GetPageCount();
            }

            /// <summary>
            /// Return the page at the given index.
            /// </summary>
            /// <param name="index">The index of the page to return.</param>
            /// <returns>The Page object of the requested page.</returns>
            protected override Page GetItem(int index)
            {
                VerifyArgumentInRange(index, 0, GetCount(), "index");
                return _extractor.GetPage(index);
            }
        }

        /// <summary>
        /// Internal class to manage the collection of subfiles in a document.
        /// </summary>
        internal class SubFileEnumeratorCollection : IEnumerable<SubFile>
        {
            private DocumentFiltersBase _parent;
            private int _parentHandle;
            private IGR_Extract_Stream _extractMethod;
            private SubfileEnumeratorCreate _createMethod;
            private SubfileDelegate _legacyFirst, _legacyNext;

            /// <summary>
            /// Creates a collection of enumerable subfiles or images. Newer DocFilters APIs support a re-usable enumerator
            /// object, were older versions supported only a single enumeration. This class detects if the newer version
            /// is available, otherwise falls back to the old behavior.
            /// </summary>
            /// <param name="parent">The object that created the enumeration, either an Extractor or Page</param>
            /// <param name="parentHandle">The handle to the parent object</param>
            /// <param name="createMethod">Delegate to create new enumerator object</param>
            /// <param name="extractMethod">Delegate that is called to extract the subfile</param>
            /// <param name="legacyFirst">Delegate to get the first subfile</param>
            /// <param name="legacyNext">Delegate to get the next subfile</param>
            internal SubFileEnumeratorCollection(DocumentFiltersBase parent
                , int parentHandle
                , SubfileEnumeratorCreate createMethod
                , IGR_Extract_Stream extractMethod
                , SubfileDelegate legacyFirst
                , SubfileDelegate legacyNext)
            {
                _parent = parent;
                _parentHandle = parentHandle;
                _createMethod = createMethod;
                _extractMethod = extractMethod;
                _legacyFirst = legacyFirst;
                _legacyNext = legacyNext;
            }

            /// <summary>
            /// Create a new enumerator for the collection.
            /// </summary>
            public IEnumerator<SubFile> GetEnumerator()
            {
                return CreateEnumerator();
            }

            /// <summary>
            /// Create a new enumerator for the collection.
            /// </summary>
            IEnumerator IEnumerable.GetEnumerator()
            {
                return CreateEnumerator();
            }

            /// <summary>
            /// Create a new enumerator for the collection.
            /// </summary>
            private IEnumerator<SubFile> CreateEnumerator()
            {
                try
                {
                    return new Enumerator(_parent, _parentHandle, _createMethod(), _extractMethod);
                } 
                catch (EntryPointNotFoundException)
                {
                    return new LegacyEnumerator(_parent, _legacyFirst, _legacyNext);
                }
            }

            /// <summary>
            /// Internal class to enumerate over subfiles in a document.
            /// </summary>
            internal class Enumerator : IEnumerator<SubFile>
            {
                private IntPtr _handle;
                private DocumentFiltersBase _parent;
                private int _parentHandle;
                private IGR_Extract_Stream _extractMethod;
                private IGR_Subfile_Info _info;

                /// <summary>
                /// Create a new enumerator for the collection.
                /// </summary>
                /// <param name="parent">The parent object.</param>
                /// <param name="parentHandle">The handle to the parent document being enumerated.</param>
                /// <param name="handle">The handle to the document being enumerated.</param>
                /// <param name="extractMethod">The C API to extract a subfile stream.</param>
                internal Enumerator(DocumentFiltersBase parent, int parentHandle, IntPtr handle, IGR_Extract_Stream extractMethod)
                {
                    _parent = parent;
                    _parentHandle = parentHandle;
                    _handle = handle;
                    _extractMethod = extractMethod;

                    _info = new IGR_Subfile_Info();
                    _info.struct_size = (uint)Marshal.SizeOf(typeof(IGR_Subfile_Info));

                    _info.id_size = 4096;
                    _info.id = Marshal.AllocHGlobal((int) _info.id_size * sizeof(char));
                    _info.name_size = 1024;
                    _info.name = Marshal.AllocHGlobal((int)_info.name_size * sizeof(char));
                    _info.comment_size = 4096;
                    _info.comment = Marshal.AllocHGlobal((int)_info.comment_size * sizeof(char));
                }

                /// <summary>
                /// Dispose of the enumerator.
                /// </summary>
                public void Dispose()
                {
                    if (_info.id != IntPtr.Zero)
                    {
                        Marshal.FreeHGlobal(_info.id);
                        _info.id = IntPtr.Zero;
                    }
                    if (_info.name != IntPtr.Zero)
                    {
                        Marshal.FreeHGlobal(_info.name);
                        _info.name = IntPtr.Zero;
                    }
                    if (_info.comment != IntPtr.Zero)
                    {
                        Marshal.FreeHGlobal(_info.comment);
                        _info.comment = IntPtr.Zero;
                    }

                    if (_handle != IntPtr.Zero)
                    {
                        Error_Control_Block err = new Error_Control_Block();
                        Check(ISYS11df.IGR_Subfiles_Close(_handle, ref err), err);
                        _handle = IntPtr.Zero;
                    }
                }

                /// <summary>
                /// Returns the current subfile.
                /// </summary>
                public SubFile Current => MakeSubFile();

                /// <summary>
                /// Returns the current subfile.
                /// </summary>
                object IEnumerator.Current => MakeSubFile();

                /// <summary>
                /// Move to the next subfile.
                /// </summary>
                public bool MoveNext()
                {
                    CheckDisposed();

                    Error_Control_Block ecb = new Error_Control_Block();
                    int result = ISYS11df.IGR_Subfiles_Next_Ex(_handle, ref _info, ref ecb);
                    if (result == ISYS11dfConstants.IGR_OK)
                        return true;
                    else if (result != ISYS11dfConstants.IGR_NO_MORE)
                        Check(result, ecb);
                    return false;
                }

                /// <summary>
                /// Reset the enumerator.
                /// </summary>
                public void Reset()
                {
                    CheckDisposed();

                    Error_Control_Block ecb = new Error_Control_Block();
                    Check(ISYS11df.IGR_Subfiles_Reset(_handle, ref ecb), ecb);
                }

                /// <summary>
                /// Check if the enumerator has been disposed.
                /// </summary>
                private void CheckDisposed()
                {
                    if (_handle == IntPtr.Zero)
                        throw new ObjectDisposedException(GetType().FullName);
                }

                /// <summary>
                /// Create a new SubFile object for the current subfile.
                /// </summary>
                private SubFile MakeSubFile()
                {
                    return new SubFile(_parent, _parentHandle, _info, _extractMethod);
                }
            }

            /// <summary>
            /// Internal class to enumerate over subfiles in a document. Used when IGR_Get_Subfiles_Enumerator is not available.
            /// </summary>
            internal class LegacyEnumerator : IEnumerator<SubFile>
            {
                private readonly object _extractor;
                private SubFile _current;
                private bool _is_first;
                private readonly SubfileDelegate _first, _next;

                /// <summary>
                /// Create a new enumerator for the collection.
                /// </summary>
                internal LegacyEnumerator(object extractor, SubfileDelegate first, SubfileDelegate next)
                {
                    _extractor = extractor;
                    _first = first;
                    _next = next;
                    _is_first = true;
                }

                /// <summary>
                /// Returns the current subfile.
                /// </summary>
                public SubFile Current => _current;

                /// <summary>
                /// Returns the current subfile.
                /// </summary>
                object IEnumerator.Current => _current;

                /// <summary>
                /// Dispose of the enumerator.
                /// </summary>
                public void Dispose()
                {
                }

                /// <summary>
                /// Move to the next subfile.
                /// </summary>

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

                /// <summary>
                /// Reset the enumerator.
                /// </summary>
                public void Reset()
                {
                    _is_first = true;
                }
            }
        }
    }
}
