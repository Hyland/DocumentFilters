//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;

namespace Hyland.DocumentFilters
{
#pragma warning disable 1591

    /// <summary>
    /// Represents the type of canvas to create.
    /// </summary>
    public enum CanvasType
    {
        PNG = ISYS11dfConstants.IGR_DEVICE_IMAGE_PNG,
        PDF = ISYS11dfConstants.IGR_DEVICE_IMAGE_PDF,
        TIF = ISYS11dfConstants.IGR_DEVICE_IMAGE_TIF,
        BMP = ISYS11dfConstants.IGR_DEVICE_IMAGE_BMP,
        PBM = ISYS11dfConstants.IGR_DEVICE_IMAGE_PBM,
        PGM = ISYS11dfConstants.IGR_DEVICE_IMAGE_PGM,
        PPM = ISYS11dfConstants.IGR_DEVICE_IMAGE_PPM,
        WEBP = ISYS11dfConstants.IGR_DEVICE_IMAGE_WEBP,
        XPS = ISYS11dfConstants.IGR_DEVICE_IMAGE_XPS,
        SVG = ISYS11dfConstants.IGR_DEVICE_IMAGE_SVG,
        EPS = ISYS11dfConstants.IGR_DEVICE_IMAGE_EPS,
        PS = ISYS11dfConstants.IGR_DEVICE_IMAGE_PS,
        XML = ISYS11dfConstants.IGR_DEVICE_XML,
        HTML = ISYS11dfConstants.IGR_DEVICE_HTML,
        JPG = ISYS11dfConstants.IGR_DEVICE_IMAGE_JPG,
        WebSafe = ISYS11dfConstants.IGR_DEVICE_IMAGE_WEBSAFE,
        TGA = ISYS11dfConstants.IGR_DEVICE_IMAGE_TGA,
        BRK = ISYS11dfConstants.IGR_DEVICE_IMAGE_BRK,
        PCX = ISYS11dfConstants.IGR_DEVICE_IMAGE_PCX,
        DCX = ISYS11dfConstants.IGR_DEVICE_IMAGE_DCX,
        JPEG2000 = ISYS11dfConstants.IGR_DEVICE_IMAGE_JPEG2000,
        GIF = ISYS11dfConstants.IGR_DEVICE_IMAGE_GIF,
        JSON = ISYS11dfConstants.IGR_DEVICE_JSON,
        MARKDOWN = ISYS11dfConstants.IGR_DEVICE_MARKDOWN,
    }
#pragma warning restore 1591

    /// <summary>
    /// DocumentFilters is the primary factory object in the Document Filters Object Library. You will need to create 
    /// and initialize an instance of this object to start using the API. It is recommended to define the object in the 
    /// application scope and create and initialize it only once.
    /// </summary>
    public class DocumentFilters : DocumentFiltersBase, System.IDisposable
    {
        private short _handle = 0;

        /// <summary>
        /// Destructor
        /// </summary>
        ~DocumentFilters()
        {
            Dispose();
        }

        /// <summary>
        /// Disposes of the DocumentFilters object and releases any resources it holds.
        /// </summary>
        public virtual void Dispose()
        {
            lock (this)
            {
                Close();
            }
        }

        /// <summary>
        /// Internal method to prepare the object for use.
        /// </summary>
        private DocumentFilters prepare()
        {
            if (_handle <= 0)
                throw new IGRException(4, "Initialize has not been called");
            return this;
        }

        /// <summary>
        /// The Initialize method initializes and authorizes the Document Filters API. It is the first method that your application must call.
        /// </summary>
        /// <param name="license">Document Filters License Code.</param>
        /// <param name="path">Path to Document Filters resources, such as configuration files and fonts.</param>
        public void Initialize(string license, string path = "")
        {
            Instance_Status_Block isb = new Instance_Status_Block();
            Error_Control_Block ecb = new Error_Control_Block();

            if (!string.IsNullOrEmpty(license) && license != "******")
            {
                isb.Licensee_ID1 = license;
            }
            else
            {
                isb.Licensee_ID1 = System.Environment.GetEnvironmentVariable("DOCFILTERS_LICENSE_KEY");
            }

            ISYS11df.Runtime.Prepare();

            if (string.IsNullOrEmpty(path) || path == ".")
            {
                path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetCallingAssembly().Location);
            }

            ISYS11df.Init_Instance(0, path, ref isb, ref _handle, ref ecb);
            IGRException.Check(ecb);
        }

        /// <summary>
        /// Close the Document Filters API.
        /// </summary>
        public void Close()
        {
            if (_handle > 0)
            {
                Error_Control_Block ecb = new Error_Control_Block();
                ISYS11df.Close_Instance(ref ecb);
                IGRException.Check(ecb);
                _handle = 0;
            }
        }

        /// <summary>
        /// Get an return a new Extractor object for the provided file on disk.
        /// </summary>
        /// <param name="filename">The filename to the document to be opened.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor GetExtractor(string filename)
        {
            VerifyArgumentNotEmpty(filename, "filename");
            return new Extractor(prepare(), filename);
        }

        /// <summary>
        /// The OpenExtractor is a utility method that obtains an Extractor object and immediately call's it's Open method to process a document.
        /// </summary>
        /// <param name="filename">The filename to the document to be opened.</param>
        /// <param name="openMode">The mode to open the file.</param>
        /// <param name="openType">The flags to open the file.</param>
        /// <param name="options">Semicolon delimited of name/value document processing options.</param>
        /// <param name="callback">Callback that is called during document processing.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor OpenExtractor(string filename, OpenMode openMode, OpenType openType, string options, Func<OpenCallback, int> callback)
        {
            Extractor res = GetExtractor(filename);
            res.Open(openMode, openType, options, callback);
            return res;
        }

        /// <summary>
        /// The OpenExtractor is a utility method that obtains an Extractor object and immediately call's it's Open method to process a document.
        /// </summary>
        /// <param name="filename">The filename to the document to be opened.</param>
        /// <param name="openMode">The mode to open the file.</param>
        /// <param name="openType">The flags to open the file.</param>
        /// <param name="options">Semicolon delimited of name/value document processing options.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor OpenExtractor(string filename, OpenMode openMode, OpenType openType = OpenType.BodyAndMeta, string options = "")
        {
            return OpenExtractor(filename, openMode, openType, options, null);
        }

        /// <summary>
        /// Get an return a new Extractor object for the provided file in the byte array.
        /// </summary>
        /// <param name="byteArray">A memory block containing binary document content.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor GetExtractor(byte[] byteArray)
        {
            return new Extractor(prepare(), byteArray);            
        }

        /// <summary>
        /// The OpenExtractor is a utility method that obtains an Extractor object and immediately call's it's Open method to process a document.
        /// </summary>
        /// <param name="byteArray">A memory block containing binary document content.</param>
        /// <param name="openMode">The mode to open the file.</param>
        /// <param name="openType">The flags to open the file.</param>
        /// <param name="options">Semicolon delimited of name/value document processing options.</param>
        /// <param name="callback">Callback that is called during document processing.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor OpenExtractor(byte[] byteArray, OpenMode openMode, OpenType openType, string options, Func<OpenCallback, int> callback)
        {
            Extractor res = GetExtractor(byteArray);
            res.Open(openMode, openType, options, callback);
            return res;
        }

        /// <summary>
        /// The OpenExtractor is a utility method that obtains an Extractor object and immediately call's it's Open method to process a document.
        /// </summary>
        /// <param name="byteArray">A memory block containing binary document content.</param>
        /// <param name="openMode">The mode to open the file.</param>
        /// <param name="openType">The flags to open the file.</param>
        /// <param name="options">Semicolon delimited of name/value document processing options.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor OpenExtractor(byte[] byteArray, OpenMode openMode, OpenType openType = OpenType.BodyAndMeta, string options = "")
        {
            return OpenExtractor(byteArray, openMode, openType, options, null);
        }

        /// <summary>
        /// Get an return a new Extractor object for the provided file in the IGRStream.
        /// </summary>
        /// <param name="stream">The stream containing the bytes of the file to process.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor GetExtractor(IGRStream stream)
        {
            VerifyArgumentNotNull(stream, "stream");
            return new Extractor(prepare(), stream);
        }

        /// <summary>
        /// Get an return a new Extractor object for the provided file in the Stream.
        /// </summary>
        /// <param name="stream">The stream containing the bytes of the file to process.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor GetExtractor(System.IO.Stream stream)
        {
            VerifyArgumentNotNull(stream, "stream");
            return new Extractor(prepare(), stream);
        }

        /// <summary>
        /// The OpenExtractor is a utility method that obtains an Extractor object and immediately call's it's Open method to process a document.
        /// </summary>
        /// <param name="stream">The stream containing the bytes of the file to process.</param>
        /// <param name="openMode">The mode to open the file.</param>
        /// <param name="openType">The flags to open the file.</param>
        /// <param name="options">Semicolon delimited of name/value document processing options.</param>
        /// <param name="callback">Callback that is called during document processing.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor OpenExtractor(System.IO.Stream stream, OpenMode openMode, OpenType openType, string options, Func<OpenCallback, int> callback)
        {
            Extractor res = GetExtractor(stream);
            res.Open(openMode, openType, options, callback);
            return res;
        }

        /// <summary>
        /// The OpenExtractor is a utility method that obtains an Extractor object and immediately call's it's Open method to process a document.
        /// </summary>
        /// <param name="stream">The stream containing the bytes of the file to process.</param>
        /// <param name="openMode">The mode to open the file.</param>
        /// <param name="openType">The flags to open the file.</param>
        /// <param name="options">Semicolon delimited of name/value document processing options.</param>
        /// <returns>Returns an Extractor interface that may be used to process a document.</returns>
        public Extractor OpenExtractor(System.IO.Stream stream, OpenMode openMode, OpenType openType = OpenType.BodyAndMeta, string options = "")
        {
            return OpenExtractor(stream, openMode, openType, options, null);
        }

        /// <summary>
        /// Returns details around memory usage.
        /// </summary>
        /// <returns></returns>
        public string GetMemoryStatus()
        {
            StringBuilder buffer = new StringBuilder(1024);

            return buffer.ToString(0, (int) ISYS11df.IGR_Get_Memory_Status(buffer, (uint) buffer.Capacity));
        }


        /// <summary>
        /// Creates a new canvas that is used for rendering page content.
        /// </summary>
        /// <param name="filename">Filename where canvas is to be created.</param>
        /// <param name="type">Indicates the type of output device to create</param>
        /// <param name="options">Semicolon separated list of name value pair canvas options</param>
        /// <returns>Canvas object where content can be rendered.</returns>
        public Canvas MakeOutputCanvas(string filename, CanvasType type, string options = "")
        {
            VerifyArgumentNotEmpty(filename, "filename");
            return new Canvas(prepare(), filename, type, options);
        }

        /// <summary>
        /// Creates a new canvas that is used for rendering page content.
        /// </summary>
        /// <param name="stream">Stream where canvas output is to be written.</param>
        /// <param name="type">Indicates the type of output device to create.</param>
        /// <param name="options">Semicolon separated list of name value pair canvas options.</param>
        /// <returns>Canvas object where content can be rendered.</returns>
        public Canvas MakeOutputCanvas(IGRStream stream, CanvasType type, string options = "")
        {
            VerifyArgumentNotNull(stream, "stream");
            return new Canvas(prepare(), stream, type, options);
        }

        /// <summary>
        /// Creates a new canvas that is used for rendering page content.
        /// </summary>
        /// <param name="stream">Stream where canvas output is to be written.</param>
        /// <param name="type">Indicates the type of output device to create.</param>
        /// <param name="options">Semicolon separated list of name value pair canvas options.</param>
        /// <returns>Canvas object where content can be rendered.</returns>
        public Canvas MakeOutputCanvas(System.IO.Stream stream, CanvasType type, string options = "")
        {
            VerifyArgumentNotNull(stream, "stream");
            return new Canvas(prepare(), stream, type, options);
        }

        /// <summary>
        /// Creates a new canvas that is used for rendering page content.
        /// </summary>
        /// <param name="filename">Filename where canvas is to be created.</param>
        /// <param name="type">Indicates the type of output device to create</param>
        /// <param name="options">Semicolon separated list of name value pair canvas options</param>
        /// <returns>Canvas object where content can be rendered.</returns>
        public Canvas MakeOutputCanvas(string filename, int type, string options)
        {
            VerifyArgumentNotEmpty(filename, "filename");
            return MakeOutputCanvas(filename, (CanvasType)type, options);
        }

        /// <summary>
        /// Creates a new canvas that is used for rendering page content.
        /// </summary>
        /// <param name="stream">Stream where canvas output is to be written.</param>
        /// <param name="type">Indicates the type of output device to create.</param>
        /// <param name="options">Semicolon separated list of name value pair canvas options.</param>
        /// <returns>Canvas object where content can be rendered.</returns>
        public Canvas MakeOutputCanvas(IGRStream stream, int type, string options)
        {
            VerifyArgumentNotNull(stream, "stream");
            return MakeOutputCanvas(stream, (CanvasType)type, options);
        }

        /// <summary>
        /// Creates a new canvas that is used for rendering page content.
        /// </summary>
        /// <param name="stream">Stream where canvas output is to be written.</param>
        /// <param name="type">Indicates the type of output device to create.</param>
        /// <param name="options">Semicolon separated list of name value pair canvas options.</param>
        /// <returns>Canvas object where content can be rendered.</returns>
        public Canvas MakeOutputCanvas(System.IO.Stream stream, int type, string options)
        {
            VerifyArgumentNotNull(stream, "stream");
            return MakeOutputCanvas(stream, (CanvasType)type, options);
        }

        /// <summary>
        /// Enumerates the available options that can be set on the DocumentFilters object.
        /// </summary>
        public IEnumerable<Option> GetAvailableOptions()
        {
            return Option.Fetch(prepare());
        }

        /// <summary>
        /// Enumerates the available formats that can be processed by the DocumentFilters object.
        /// </summary>
        public IEnumerable<Format> GetSupportedFormats()
        {
            return Format.Fetch(prepare());
        }
    }

    /// <summary>
    /// Alias for DocumentFilters.
    /// </summary>
    public class Api : DocumentFilters
    {
    }
}
