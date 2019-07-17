//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System.Text;

namespace Hyland.DocumentFilters
{
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
        HTML = ISYS11dfConstants.IGR_DEVICE_HTML
    }
    public class DocumentFilters : DocumentFiltersBase, System.IDisposable
    {
        private short _handle = 0;

        ~DocumentFilters()
        {
            Dispose();
        }

        public virtual void Dispose()
        {
            lock (this)
            {
                Close();
            }
        }
        private DocumentFilters prepare()
        {
            if (_handle <= 0)
                throw new IGRException(4, "Initialize has not been called");
            return this;
        }
        public void Initialize(string license, string path = "")
        {
            VerifyArgumentNotEmpty(license, "license");

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

            ISYS11df.Init_Instance(0, path, ref isb, ref _handle, ref ecb);
            IGRException.Check(ecb);
        }
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
        public Extractor GetExtractor(string filename)
        {
            VerifyArgumentNotEmpty(filename, "filename");
            return new Extractor(prepare(), filename);
        }
        public Extractor GetExtractor(byte[] byteArray)
        {
            return new Extractor(prepare(), byteArray);            
        }
        public Extractor GetExtractor(IGRStream stream)
        {
            VerifyArgumentNotNull(stream, "stream");
            return new Extractor(prepare(), stream);
        }
        public Extractor GetExtractor(System.IO.Stream stream)
        {
            VerifyArgumentNotNull(stream, "stream");
            return new Extractor(prepare(), stream);
        }
        public string GetMemoryStatus()
        {
            StringBuilder buffer = new StringBuilder(1024);

            return buffer.ToString(0, (int) ISYS11df.IGR_Get_Memory_Status(buffer, (uint) buffer.Capacity));
        }
        public Canvas MakeOutputCanvas(string filename, CanvasType type, string options = "")
        {
            VerifyArgumentNotEmpty(filename, "filename");
            return new Canvas(prepare(), filename, type, options);
        }
        public Canvas MakeOutputCanvas(IGRStream stream, CanvasType type, string options = "")
        {
            VerifyArgumentNotNull(stream, "stream");
            return new Canvas(prepare(), stream, type, options);
        }
        public Canvas MakeOutputCanvas(System.IO.Stream stream, CanvasType type, string options = "")
        {
            VerifyArgumentNotNull(stream, "stream");
            return new Canvas(prepare(), stream, type, options);
        }

        public Canvas MakeOutputCanvas(string filename, int type, string options)
        {
            VerifyArgumentNotEmpty(filename, "filename");
            return MakeOutputCanvas(filename, (CanvasType)type, options);
        }
        public Canvas MakeOutputCanvas(IGRStream stream, int type, string options)
        {
            VerifyArgumentNotNull(stream, "stream");
            return MakeOutputCanvas(stream, (CanvasType)type, options);
        }
        public Canvas MakeOutputCanvas(System.IO.Stream stream, int type, string options)
        {
            VerifyArgumentNotNull(stream, "stream");
            return MakeOutputCanvas(stream, (CanvasType)type, options);
        }
    }
}
