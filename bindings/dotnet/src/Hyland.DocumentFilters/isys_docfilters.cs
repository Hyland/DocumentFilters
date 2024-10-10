//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

namespace Hyland.DocumentFilters
{
#pragma warning disable 1591
    public class isys_docfilters
    {
        public static readonly int IGR_BODY_ONLY = ISYS11dfConstants.IGR_BODY_ONLY;
        public static readonly int IGR_META_ONLY = ISYS11dfConstants.IGR_META_ONLY;
        public static readonly int IGR_BODY_AND_META = ISYS11dfConstants.IGR_BODY_AND_META;
        public static readonly int IGR_FORMAT_TEXT = ISYS11dfConstants.IGR_FORMAT_TEXT;
        public static readonly int IGR_FORMAT_HTML = ISYS11dfConstants.IGR_FORMAT_HTML;
        public static readonly int IGR_FORMAT_XML = ISYS11dfConstants.IGR_FORMAT_XML;
        public static readonly int IGR_FORMAT_IMAGE = ISYS11dfConstants.IGR_FORMAT_IMAGE;
        public static readonly int IGR_FILE_SUPPORTS_TEXT = ISYS11dfConstants.IGR_FILE_SUPPORTS_TEXT;
        public static readonly int IGR_FILE_SUPPORTS_SUBFILES = ISYS11dfConstants.IGR_FILE_SUPPORTS_SUBFILES;
        public static readonly int IGR_FILE_SUPPORTS_HDHTML = ISYS11dfConstants.IGR_FILE_SUPPORTS_HDHTML;
        public static readonly int IGR_OK = ISYS11dfConstants.IGR_OK;
        public static readonly int IGR_E_OPEN_ERROR = ISYS11dfConstants.IGR_E_OPEN_ERROR;
        public static readonly int IGR_E_WRONG_TYPE = ISYS11dfConstants.IGR_E_WRONG_TYPE;
        public static readonly int IGR_E_IN_USE = ISYS11dfConstants.IGR_E_IN_USE;
        public static readonly int IGR_E_NOT_READABLE = ISYS11dfConstants.IGR_E_NOT_READABLE;
        public static readonly int IGR_E_PASSWORD = ISYS11dfConstants.IGR_E_PASSWORD;
        public static readonly int IGR_E_NOT_FOUND = ISYS11dfConstants.IGR_E_NOT_FOUND;
        public static readonly int IGR_E_WRITE_ERROR = ISYS11dfConstants.IGR_E_WRITE_ERROR;
        public static readonly int IGR_E_NOT_VALID_FOR_THIS_CLASS = ISYS11dfConstants.IGR_E_NOT_VALID_FOR_THIS_CLASS;
        public static readonly int IGR_E_ERROR = ISYS11dfConstants.IGR_E_ERROR;
        public static readonly int IGR_E_INVALID_HANDLE = ISYS11dfConstants.IGR_E_INVALID_HANDLE;
        public static readonly int IGR_E_INVALID_POINTER = ISYS11dfConstants.IGR_E_INVALID_POINTER;
        public static readonly int IGR_E_INVALID_PARAMETER = ISYS11dfConstants.IGR_E_INVALID_PARAMETER;
        public static readonly int IGR_NO_MORE = ISYS11dfConstants.IGR_NO_MORE;
        public static readonly int IGR_DEVICE_IMAGE_PNG = ISYS11dfConstants.IGR_DEVICE_IMAGE_PNG;
        public static readonly int IGR_DEVICE_IMAGE_JPG = ISYS11dfConstants.IGR_DEVICE_IMAGE_JPG;
        public static readonly int IGR_DEVICE_IMAGE_PDF = ISYS11dfConstants.IGR_DEVICE_IMAGE_PDF;
        public static readonly int IGR_DEVICE_IMAGE_TIF = ISYS11dfConstants.IGR_DEVICE_IMAGE_TIF;
        public static readonly int IGR_DEVICE_IMAGE_BMP = ISYS11dfConstants.IGR_DEVICE_IMAGE_BMP;
        public static readonly int IGR_DEVICE_XML = ISYS11dfConstants.IGR_DEVICE_XML;
        public static readonly int IGR_DEVICE_HTML = ISYS11dfConstants.IGR_DEVICE_HTML;
        public static readonly int IGR_DEVICE_IMAGE_PBM = ISYS11dfConstants.IGR_DEVICE_IMAGE_PBM;
        public static readonly int IGR_DEVICE_IMAGE_PGM = ISYS11dfConstants.IGR_DEVICE_IMAGE_PGM;
        public static readonly int IGR_DEVICE_IMAGE_PPM = ISYS11dfConstants.IGR_DEVICE_IMAGE_PPM;
        public static readonly int IGR_DEVICE_IMAGE_WEBP = ISYS11dfConstants.IGR_DEVICE_IMAGE_WEBP;
        public static readonly int IGR_DEVICE_IMAGE_XPS = ISYS11dfConstants.IGR_DEVICE_IMAGE_XPS;
        public static readonly int IGR_DEVICE_IMAGE_SVG = ISYS11dfConstants.IGR_DEVICE_IMAGE_SVG;
        public static readonly int IGR_DEVICE_IMAGE_EPS = ISYS11dfConstants.IGR_DEVICE_IMAGE_EPS;
        public static readonly int IGR_DEVICE_IMAGE_PS = ISYS11dfConstants.IGR_DEVICE_IMAGE_PS;
        public static readonly int IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL = 0;
        public static readonly int IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT = 1;
        public static readonly int IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE = 2;
        public static readonly int IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING = 3;
        public static readonly int IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH = 0xFF;

        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS = 0x1;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_MOVES = 0x10;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING = 0x20;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS = 0x40;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE = 0x80;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE = 0x100;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION = 0x200;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES = 0x400;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS = 0x800;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS = 0x1000;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES = 0x2000;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES = 0x4000;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS = 0x8000;
        public static readonly int IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS_FOOTERS = IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS | IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS;

        public static readonly int IGR_COMPARE_DOCUMENTS_COMPARE_WORDS = 0x0;

        public static readonly int IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_ORIGINAL = 0x0;
        public static readonly int IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_REVISED = 0x1;
        public static readonly int IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_BOTH = 0x2;
    }
}
