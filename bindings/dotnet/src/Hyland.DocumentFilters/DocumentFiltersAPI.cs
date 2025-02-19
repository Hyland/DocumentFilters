//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Runtime.CompilerServices;

#if NETSTANDARD
[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory | DllImportSearchPath.ApplicationDirectory | DllImportSearchPath.SafeDirectories | DllImportSearchPath.UseDllDirectoryForDependencies)]
#endif

#pragma warning disable 1591

namespace Hyland.DocumentFilters
{
    using IGR_ULONG = System.UInt32;
    using IGR_LONG = System.Int32;
    using IGR_HPAGE = System.Int32;
    using IGR_UCS2 = System.UInt16;
    using IGR_RETURN_CODE = System.Int32;

    public partial class ISYS11dfConstants
    {
        // IGR_Multi_Set_Code_Page -> 1
        public const int IGR_Multi_Set_Code_Page = 1;

        // IGR_Multi_Set_Temp_Path -> 186
        public const int IGR_Multi_Set_Temp_Path = 186;

        // IGR_BODY_ONLY -> 0
        public const int IGR_BODY_ONLY = 0;

        // IGR_META_ONLY -> 1
        public const int IGR_META_ONLY = 1;

        // IGR_BODY_AND_META -> 2
        public const int IGR_BODY_AND_META = 2;

        // IGR_FORMAT_TEXT -> 0x00010000L
        public const int IGR_FORMAT_TEXT = 65536;

        // IGR_FORMAT_HTML -> 0x00020000L
        public const int IGR_FORMAT_HTML = 131072;

        // IGR_FORMAT_XML -> 0x00030000L
        public const int IGR_FORMAT_XML = 196608;

        // IGR_FORMAT_HDHTML_IFYOUCAN -> 0x00040000L
        public const int IGR_FORMAT_HDHTML_IFYOUCAN = 262144;

        // IGR_FORMAT_IMAGE -> 0x00050000L
        public const int IGR_FORMAT_IMAGE = 327680;

        // IGR_FORMAT_HTML_NO_FALLBACK -> 0x00060000L
        public const int IGR_FORMAT_HTML_NO_FALLBACK = 393216;

        public const int IGR_FORMAT_MARKDOWN = 0x00080000;

        // IGR_FILE_SUPPORTS_TEXT -> 1
        public const int IGR_FILE_SUPPORTS_TEXT = 1;

        // IGR_FILE_SUPPORTS_SUBFILES -> 2
        public const int IGR_FILE_SUPPORTS_SUBFILES = 2;

        // IGR_FILE_SUPPORTS_HDHTML -> 4
        public const int IGR_FILE_SUPPORTS_HDHTML = 4;

        // IGR_FILE_CORRUPT_OR_TRUNCATED -> 0x2000
        public const int IGR_FILE_CORRUPT_OR_TRUNCATED = 0x2000;

        // IGR_OK -> 0
        public const int IGR_OK = 0;

        // IGR_E_OPEN_ERROR -> 1
        public const int IGR_E_OPEN_ERROR = 1;

        // IGR_E_WRONG_TYPE -> 2
        public const int IGR_E_WRONG_TYPE = 2;

        // IGR_E_IN_USE -> 3
        public const int IGR_E_IN_USE = 3;

        // IGR_E_NOT_READABLE -> 4
        public const int IGR_E_NOT_READABLE = 4;

        // IGR_E_PASSWORD -> 5
        public const int IGR_E_PASSWORD = 5;

        // IGR_E_NOT_FOUND -> 10
        public const int IGR_E_NOT_FOUND = 10;

        // IGR_E_WRITE_ERROR -> 11
        public const int IGR_E_WRITE_ERROR = 11;

        // IGR_E_NOT_VALID_FOR_THIS_CLASS -> 12
        public const int IGR_E_NOT_VALID_FOR_THIS_CLASS = 12;

        // IGR_E_ERROR -> 13
        public const int IGR_E_ERROR = 13;

        // IGR_E_INVALID_HANDLE -> 14
        public const int IGR_E_INVALID_HANDLE = 14;

        // IGR_E_INVALID_POINTER -> 15
        public const int IGR_E_INVALID_POINTER = 15;

        // IGR_E_INVALID_PARAMETER -> 16
        public const int IGR_E_INVALID_PARAMETER = 16;

        // IGR_E_FILE_CORRUPT -> 17
        public const int IGR_E_FILE_CORRUPT = 17;

        // IGR_NO_MORE -> 19
        public const int IGR_NO_MORE = 19;

        // IGR_E_OUT_OF_MEMORY -> 20
        public const int IGR_E_OUT_OF_MEMORY = 20;

        // IGR_E_BAD_ERROR -> 21
        public const int IGR_E_BAD_ERROR = 21;

        // IGR_E_WRONG_THREAD -> 22
        public const int IGR_E_WRONG_THREAD = 22;

        // IGR_E_TOO_MANY_HANDLES -> 23
        public const int IGR_E_TOO_MANY_HANDLES = 23;

        // IGR_E_FONTS_NOT_FOUND -> 24
        public const int IGR_E_FONTS_NOT_FOUND = 24;

        // IGR_E_VERSION_MISMATCH -> 25
        public const int IGR_E_VERSION_MISMATCH = 25;

        // IGR_E_ACTION_NOT_SUPPORTED -> 26
        public const int IGR_E_ACTION_NOT_SUPPORTED = 26;

        // IGR_CANCELLED -> 27
        public const int IGR_CANCELLED = 27;

        // IGR_CHAR_SOFT_SPACE -> 1
        public const int IGR_CHAR_SOFT_SPACE = 1;

        // IGR_CHAR_STYLE_NORMAL -> 2
        public const int IGR_CHAR_STYLE_NORMAL = 2;

        // IGR_CHAR_STYLE_BOLD -> 3
        public const int IGR_CHAR_STYLE_BOLD = 3;

        // IGR_CHAR_STYLE_ITALICS -> 4
        public const int IGR_CHAR_STYLE_ITALICS = 4;

        // IGR_CHAR_STYLE_UNDERLINED -> 5
        public const int IGR_CHAR_STYLE_UNDERLINED = 5;

        // IGR_CHAR_TAB -> 9
        public const int IGR_CHAR_TAB = 9;

        // IGR_CHAR_PAGE_BREAK -> 12
        public const int IGR_CHAR_PAGE_BREAK = 12;

        // IGR_CHAR_LINE_BREAK -> 13
        public const int IGR_CHAR_LINE_BREAK = 13;

        // IGR_CHAR_PARA_BREAK -> 14
        public const int IGR_CHAR_PARA_BREAK = 14;

        // IGR_CHAR_START_META -> 15
        public const int IGR_CHAR_START_META = 15;

        // IGR_CHAR_END_META -> 16
        public const int IGR_CHAR_END_META = 16;

        // IGR_DEVICE_IMAGE_PNG -> 0
        public const int IGR_DEVICE_IMAGE_PNG = 0;

        // IGR_DEVICE_IMAGE_JPG -> 1
        public const int IGR_DEVICE_IMAGE_JPG = 1;

        // IGR_DEVICE_IMAGE_PDF -> 2
        public const int IGR_DEVICE_IMAGE_PDF = 2;

        // IGR_DEVICE_IMAGE_TIF -> 3
        public const int IGR_DEVICE_IMAGE_TIF = 3;

        // IGR_DEVICE_IMAGE_BMP -> 4
        public const int IGR_DEVICE_IMAGE_BMP = 4;

        // IGR_DEVICE_IMAGE_PBM -> 7
        public const int IGR_DEVICE_IMAGE_PBM = 7;

        // IGR_DEVICE_IMAGE_PGM -> 8
        public const int IGR_DEVICE_IMAGE_PGM = 8;

        // IGR_DEVICE_IMAGE_PPM -> 9
        public const int IGR_DEVICE_IMAGE_PPM = 9;

        // IGR_DEVICE_IMAGE_WEBP -> 10
        public const int IGR_DEVICE_IMAGE_WEBP = 10;

        // IGR_DEVICE_IMAGE_XPS -> 11
        public const int IGR_DEVICE_IMAGE_XPS = 11;

        // IGR_DEVICE_IMAGE_SVG -> 12
        public const int IGR_DEVICE_IMAGE_SVG = 12;

        // IGR_DEVICE_IMAGE_EPS -> 13
        public const int IGR_DEVICE_IMAGE_EPS = 13;

        // IGR_DEVICE_IMAGE_PS -> 14
        public const int IGR_DEVICE_IMAGE_PS = 14;

        public const int IGR_DEVICE_IMAGE_WEBSAFE = 15;
        public const int IGR_DEVICE_IMAGE_TGA = 16;
        public const int IGR_DEVICE_IMAGE_BRK = 17;
        public const int IGR_DEVICE_IMAGE_PCX = 18;
        public const int IGR_DEVICE_IMAGE_DCX = 19;
        public const int IGR_DEVICE_IMAGE_JPEG2000 = 20;
        public const int IGR_DEVICE_IMAGE_GIF = 21;
        public const int IGR_DEVICE_JSON = 22;
        public const int IGR_DEVICE_MARKDOWN = 23;

        // IGR_DEVICE_XML -> 5
        public const int IGR_DEVICE_XML = 5;

        // IGR_DEVICE_HTML -> 6
        public const int IGR_DEVICE_HTML = 6;

        // IGR_TEXT_ALIGN_LEFT -> 0x00
        public const int IGR_TEXT_ALIGN_LEFT = 0;

        // IGR_TEXT_ALIGN_RIGHT -> 0x01
        public const int IGR_TEXT_ALIGN_RIGHT = 1;

        // IGR_TEXT_ALIGN_CENTER -> 0x02
        public const int IGR_TEXT_ALIGN_CENTER = 2;

        // IGR_TEXT_ALIGN_TOP -> 0x10
        public const int IGR_TEXT_ALIGN_TOP = 16;

        // IGR_TEXT_ALIGN_MIDDLE -> 0x11
        public const int IGR_TEXT_ALIGN_MIDDLE = 17;

        // IGR_TEXT_ALIGN_BOTTOM -> 0x12
        public const int IGR_TEXT_ALIGN_BOTTOM = 18;

        // IGR_TEXT_STYLE_BOLD -> 0x01
        public const int IGR_TEXT_STYLE_BOLD = 1;

        // IGR_TEXT_STYLE_ITALIC -> 0x02
        public const int IGR_TEXT_STYLE_ITALIC = 2;

        // IGR_TEXT_STYLE_UNDERLINE -> 0x04
        public const int IGR_TEXT_STYLE_UNDERLINE = 4;

        // IGR_BRUSH_NONE -> 0x00
        public const int IGR_BRUSH_NONE = 0;

        // IGR_BRUSH_SOLID -> 0x01
        public const int IGR_BRUSH_SOLID = 1;

        // IGR_PEN_NONE -> 0x00
        public const int IGR_PEN_NONE = 0;

        // IGR_PEN_SOLID -> 0x01
        public const int IGR_PEN_SOLID = 1;

        public const int IGR_TEXTRECT_FLAG_CLIP = 0x01;

        public const int IGR_TEXTRECT_FLAG_STRETCHTOFIT = 0x02;

        // IGR_ACTION_GET_STREAM_PART -> 10
        public const int IGR_ACTION_GET_STREAM_PART = 10;

        // Warning: Generation of Method Macros is not supported at this time
        // MAKE_COLOR -> "(R,G,B) ((int) R | ((int) G << 8) | ((int) B << 16))"
        public const string MAKE_COLOR = "(R,G,B) ((int) R | ((int) G << 8) | ((int) B << 16))";

        // IGR_PAGE_FORM_ELEMENT_TYPE_BUTTON -> 0
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_BUTTON = 0;

        // IGR_PAGE_FORM_ELEMENT_TYPE_RADIOBUTTON -> 1
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_RADIOBUTTON = 1;

        // IGR_PAGE_FORM_ELEMENT_TYPE_CHECKBOX -> 2
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_CHECKBOX = 2;

        // IGR_PAGE_FORM_ELEMENT_TYPE_FILESELECT -> 3
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_FILESELECT = 3;

        // IGR_PAGE_FORM_ELEMENT_TYPE_MULTILINE_TEXT -> 4
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_MULTILINE_TEXT = 4;

        // IGR_PAGE_FORM_ELEMENT_TYPE_SINGLELINE_TEXT -> 5
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_SINGLELINE_TEXT = 5;

        // IGR_PAGE_FORM_ELEMENT_TYPE_COMBOBOX -> 6
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_COMBOBOX = 6;

        // IGR_PAGE_FORM_ELEMENT_TYPE_LISTBOX -> 7
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_LISTBOX = 7;

        // IGR_PAGE_FORM_ELEMENT_TYPE_SIGNATURE -> 8
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_SIGNATURE = 8;

        // IGR_PAGE_FORM_ELEMENT_TYPE_PASSWORD -> 9
        public const int IGR_PAGE_FORM_ELEMENT_TYPE_PASSWORD = 9;

        // IGR_PAGE_FORM_ELEMENT_FLAG_READONLY -> (1 << 0)
        public const int IGR_PAGE_FORM_ELEMENT_FLAG_READONLY = (1) << (0);

        // IGR_PAGE_FORM_ELEMENT_FLAG_REQUIRED -> (1 << 1)
        public const int IGR_PAGE_FORM_ELEMENT_FLAG_REQUIRED = (1) << (1);

        // IGR_PAGE_FORM_ELEMENT_FLAG_COMB -> (1 << 2)
        public const int IGR_PAGE_FORM_ELEMENT_FLAG_COMB = (1) << (2);

        // IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED -> (1 << 3)
        public const int IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED = (1) << (3);

        // IGR_PAGE_FORM_ELEMENT_ALIGN_LEFT -> 0

        public const int IGR_BOOKMARK_ACTION_GOTO = 0;
        public const int IGR_BOOKMARK_ACTION_LINK = 1;
        public const int IGR_BOOKMARK_FIT_NONE = 0;
        public const int IGR_BOOKMARK_FIT_BOTH = 1;
        public const int IGR_BOOKMARK_FIT_WIDTH = 2;
        public const int IGR_BOOKMARK_FIT_HEIGHT = 3;
        public const int IGR_BOOKMARK_FIT_RECT = 4;
        public const int IGR_PAGE_FORM_ELEMENT_GET_NAME = 0;
        public const int IGR_PAGE_FORM_ELEMENT_GET_VALUE = 1;
        public const int IGR_PAGE_FORM_ELEMENT_GET_CAPTION = 2;
        public const int IGR_PAGE_FORM_ELEMENT_GET_ACTION = 3;
        public const int IGR_PAGE_FORM_ELEMENT_GET_ACTION_DEST = 4;
        public const int IGR_PAGE_FORM_ELEMENT_GET_SELECTED = 5;
        public const int IGR_PAGE_FORM_ELEMENT_GET_FONT_NAME = 6;

        public const int IGR_HYPERLINK_ACTION_GOTO = 1;
        public const int IGR_HYPERLINK_ACTION_URI = 2;
        public const int IGR_HYPERLINK_FLAGS_CHANGE_LEFT = 1;
        public const int IGR_HYPERLINK_FLAGS_CHANGE_TOP = 2;
        public const int IGR_HYPERLINK_FLAGS_CHANGE_ZOOM = 4;
        public const int IGR_HYPERLINK_FIT_XYZ = 0;
        public const int IGR_HYPERLINK_FIT_FIT = 1;
        public const int IGR_HYPERLINK_FIT_FITH = 2;
        public const int IGR_HYPERLINK_FIT_FITV = 3;
        public const int IGR_HYPERLINK_FIT_FITR = 4;
        public const int IGR_HYPERLINK_FIT_FITB = 5;
        public const int IGR_HYPERLINK_FIT_FITBH = 6;
        public const int IGR_HYPERLINK_FIT_FITBV = 7;
        public const int IGR_HYPERLINK_GET_VALUE_REF = 0;
        public const int IGR_HYPERLINK_GET_VALUE_URI = 1;

        public const int IGR_ANNOTATION_UNKNOWN = 0;
        public const int IGR_ANNOTATION_TEXT = 1;
        public const int IGR_ANNOTATION_LINK = 2;
        public const int IGR_ANNOTATION_FREETEXT = 3;
        public const int IGR_ANNOTATION_LINE = 4;
        public const int IGR_ANNOTATION_SQUARE = 5;
        public const int IGR_ANNOTATION_CIRCLE = 6;
        public const int IGR_ANNOTATION_POLYGON = 7;
        public const int IGR_ANNOTATION_POLYLINE = 8;
        public const int IGR_ANNOTATION_HIGHLIGHT = 9;
        public const int IGR_ANNOTATION_UNDERLINE = 10;
        public const int IGR_ANNOTATION_SQUIGGLE = 11;
        public const int IGR_ANNOTATION_STRIKEOUT = 12;
        public const int IGR_ANNOTATION_STAMP = 13;
        public const int IGR_ANNOTATION_CARET = 14;
        public const int IGR_ANNOTATION_INK = 15;
        public const int IGR_ANNOTATION_POPUP = 16;
        public const int IGR_ANNOTATION_FILEATTACHMENT = 17;
        public const int IGR_ANNOTATION_SOUND = 18;
        public const int IGR_ANNOTATION_MOVIE = 19;
        public const int IGR_ANNOTATION_WIDGET = 20;
        public const int IGR_ANNOTATION_SCREEN = 21;
        public const int IGR_ANNOTATION_PRINTERMARK = 22;
        public const int IGR_ANNOTATION_TRAPNET = 23;
        public const int IGR_ANNOTATION_WATERMARK = 24;
        public const int IGR_ANNOTATION_3D = 25;
        public const int IGR_ANNOTATION_BARCODE = 26;
        public const int IGR_ANNOTATION_NAMED_DESTINATION = 27;

        public const int IGR_ANNOTATION_FLAGS_INVISIBLE = (1 << 0);
        public const int IGR_ANNOTATION_FLAGS_HIDDEN = (1 << 1);
        public const int IGR_ANNOTATION_FLAGS_PRINT = (1 << 2);
        public const int IGR_ANNOTATION_FLAGS_NOZOOM = (1 << 3);
        public const int IGR_ANNOTATION_FLAGS_NOROTATE = (1 << 4);
        public const int IGR_ANNOTATION_FLAGS_NOVIEW = (1 << 5);
        public const int IGR_ANNOTATION_FLAGS_READONLY = (1 << 6);
        public const int IGR_ANNOTATION_FLAGS_LOCKED = (1 << 7);
        public const int IGR_ANNOTATION_FLAGS_TOGGLENOVIEW = (1 << 8);
        public const int IGR_ANNOTATION_FLAGS_LOCKEDCONTENTS = (1 << 9);

        public const int IGR_GET_PAGE_PIXELS_FLAGS_BUFFER_ALLOCATED = 0x00010000;

        public const int IGR_OPEN_BITMAP_PIXEL_1BPP_INDEXED = 1;
        public const int IGR_OPEN_BITMAP_PIXEL_4BPP_INDEXED = 2;
        public const int IGR_OPEN_BITMAP_PIXEL_8BPP_INDEXED = 3;
        public const int IGR_OPEN_BITMAP_PIXEL_16BPP_565_RGB = 4;
        public const int IGR_OPEN_BITMAP_PIXEL_16BPP_565_BGR = 5;
        public const int IGR_OPEN_BITMAP_PIXEL_16BPP_4444_ARGB = 6;
        public const int IGR_OPEN_BITMAP_PIXEL_16BPP_4444_BGRA = 7;
        public const int IGR_OPEN_BITMAP_PIXEL_24BPP_888_RGB = 8;
        public const int IGR_OPEN_BITMAP_PIXEL_24BPP_888_BGR = 9;
        public const int IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ARGB = 10;
        public const int IGR_OPEN_BITMAP_PIXEL_32BPP_8888_BGRA = 11;
        public const int IGR_OPEN_BITMAP_PIXEL_32BPP_8888_RGBA = 12;
        public const int IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ABGR = 13;
        public const int IGR_OPEN_BITMAP_PIXEL_AUTO = 0;

        public const int IGR_OPEN_BITMAP_FLAGS_MIN_IS_BLACK = 0x01;
        public const int IGR_OPEN_BITMAP_FLAGS_BOTTOM_UP = 0x02;

        public const int IGR_OPEN_FROM_FILENAME_UTF16 = 0;
        public const int IGR_OPEN_FROM_STREAM = 1;

        public const int IGR_OPEN_CALLBACK_ACTION_HEARTBEAT = 0;
        public const int IGR_OPEN_CALLBACK_ACTION_PASSWORD = 1;
        public const int IGR_OPEN_CALLBACK_ACTION_LOCALIZE = 2;
        public const int IGR_OPEN_CALLBACK_ACTION_LOG_LEVEL = 3;
        public const int IGR_OPEN_CALLBACK_ACTION_LOG_MESSAGE = 4;

        public const int IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL = 0;
        public const int IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT = 1;
        public const int IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE = 2;
        public const int IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING = 3;
        public const int IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH = 0xFF;

        public const int IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS = 0x1;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_MOVES = 0x10;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING = 0x20;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS = 0x40;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE = 0x80;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE = 0x100;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION = 0x200;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES = 0x400;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS = 0x800;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS = 0x1000;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES = 0x2000;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES = 0x4000;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS = 0x8000;
        public const int IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS_FOOTERS = IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS | IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS;

        public const int IGR_COMPARE_DOCUMENTS_COMPARE_WORDS = 0x0;

        public const int IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_ORIGINAL = 0x0;
        public const int IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_REVISED = 0x1;
        public const int IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_BOTH = 0x2;

        public const int IGR_FILETYPE_UNKNOWN = 0;
        public const int IGR_FILETYPE_TEXT = 100;
        public const int IGR_FILETYPE_TEXT_MARKUP = 101;
        public const int IGR_FILETYPE_WORD_PROCESSOR = 200;
        public const int IGR_FILETYPE_SPREADSHEET = 300;
        public const int IGR_FILETYPE_PRESENTATION = 400;
        public const int IGR_FILETYPE_OFFICE_OTHER = 500;
        public const int IGR_FILETYPE_CAD = 600;
        public const int IGR_FILETYPE_ARCHIVE = 700;
        public const int IGR_FILETYPE_SYSTEM = 800;
        public const int IGR_FILETYPE_DATABASE = 900;
        public const int IGR_FILETYPE_GRAPHIC = 1000;
        public const int IGR_FILETYPE_GRAPHIC_RASTER = 1001;
        public const int IGR_FILETYPE_GRAPHIC_VECTOR = 1002;
        public const int IGR_FILETYPE_EMAIL = 1100;
        public const int IGR_FILETYPE_EMAIL_MESSAGE = 1101;
        public const int IGR_FILETYPE_EMAIL_CONTAINER = 1102;
        public const int IGR_FILETYPE_EMAIL_CONTACT = 1103;
        public const int IGR_FILETYPE_EMAIL_CALENDAR = 1104;
        public const int IGR_FILETYPE_EMAIL_OTHER = 1105;
        public const int IGR_FILETYPE_MULTIMEDIA = 1200;
        public const int IGR_FILETYPE_MULTIMEDIA_VIDEO = 1201;
        public const int IGR_FILETYPE_MULTIMEDIA_AUDIO = 1202;
        public const int IGR_FILETYPE_PUBLISHING = 1300;
        public const int IGR_FILETYPE_APPDATA = 2000;

        public const int IGR_PAGE_ELEMENT_TYPE_PAGE = 1;
        public const int IGR_PAGE_ELEMENT_TYPE_SECTION = 2;
        public const int IGR_PAGE_ELEMENT_TYPE_COLUMN = 3;
        public const int IGR_PAGE_ELEMENT_TYPE_PARAGRAPH = 4;
        public const int IGR_PAGE_ELEMENT_TYPE_LINE = 5;
        public const int IGR_PAGE_ELEMENT_TYPE_RUN = 6;
        public const int IGR_PAGE_ELEMENT_TYPE_WORD = 7;
        public const int IGR_PAGE_ELEMENT_TYPE_HEADER = 8;
        public const int IGR_PAGE_ELEMENT_TYPE_FOOTER = 9;
        public const int IGR_PAGE_ELEMENT_TYPE_IMAGE = 10;
        public const int IGR_PAGE_ELEMENT_TYPE_TABLE = 11;
        public const int IGR_PAGE_ELEMENT_TYPE_TABLE_ROW = 12;
        public const int IGR_PAGE_ELEMENT_TYPE_TABLE_CELL = 13;
        public const int IGR_PAGE_ELEMENT_TYPE_FORM = 14;
        public const int IGR_PAGE_ELEMENT_TYPE_FORM_ELEMENT = 15;
        public const int IGR_PAGE_ELEMENT_TYPE_FLOAT = 16;
        public const int IGR_PAGE_ELEMENT_TYPE_GRAPHIC = 17;
        public const int IGR_PAGE_ELEMENT_TYPE_TEXT_BOX = 18;
        public const int IGR_PAGE_ELEMENT_TYPE_NOTES = 19;

        public const int IGR_PAGE_ELEMENT_FLAG_DYNAMIC_CONTENT  = 0x01;
        public const int IGR_PAGE_ELEMENT_FLAG_INJECTED_CONTENT = 0x02;

        public const int IGR_SUBFILE_INFO_FLAG_PASSWORD_PROTECTED = 0x0002;
        public const int IGR_SUBFILE_INFO_FLAG_HAS_COMMENT = 0x0004;

        public const int IGR_LOG_LEVEL_NONE = 0;
        public const int IGR_LOG_LEVEL_CRITICAL = 1;
        public const int IGR_LOG_LEVEL_ERROR = 2;
        public const int IGR_LOG_LEVEL_WARNING = 3;
        public const int IGR_LOG_LEVEL_INFORMATION = 4;
        public const int IGR_LOG_LEVEL_DEBUG = 5;
        public const int IGR_LOG_LEVEL_TRACE = 6;

    }


    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct Error_Control_Block
    {

        // char[238]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 238)]
        public string Msg;

        // SHORT->short
        public short MsgId;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct Instance_Status_Block
    {

        // char[16]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string DLL_Version;

        // SHORT->short
        public short RReserved;

        // SHORT->short
        public short License_Type;

        // char[40]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 40)]
        public string Licensee_ID1;

        // char[40]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 40)]
        public string Licensee_ID2;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Page_Form_Element
    {

        // void*
        public System.IntPtr reserved;

        // IGR_UCS2[255]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 255)]
        public string name;

        // IGR_LONG->int
        public int x;

        // IGR_LONG->int
        public int y;

        // IGR_LONG->int
        public int width;

        // IGR_LONG->int
        public int height;

        // IGR_LONG->int
        public int type;

        // IGR_LONG->int
        public int flags;

        // IGR_LONG->int
        public int fontSize;

        // IGR_LONG->int
        public int fontStyle;

        // IGR_LONG->int
        public int rotation;

        // IGR_LONG->int
        public int textAlignment;

        // IGR_LONG->int
        public int selectedItem;

        // IGR_LONG->int
        public int option_count;

        // IGR_LONG[8]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst = 8, ArraySubType = System.Runtime.InteropServices.UnmanagedType.I4)]
        public int[] reserved2;
    }

    public delegate int IGR_Get_Entry(int handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);
    public delegate int IGR_Extract_File(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, [In()] [MarshalAs(UnmanagedType.LPWStr)] string destination, ref Error_Control_Block error);
    public delegate int IGR_Extract_Stream(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, ref System.IntPtr stream, ref Error_Control_Block error);

    // Return Type: ULONG->unsigned int
    ///stream: IGR_Stream*
    ///
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate UInt32 IGR_Stream_Open(IntPtr stream);

    // Return Type: ULONG->unsigned int
    ///stream: IGR_Stream*
    ///buffer: void*
    ///bufferSize: ULONG->unsigned int
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate UInt32 IGR_Stream_Read(IntPtr stream, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex=2), Out] byte[] buffer, UInt32 bufferSize);

    // Return Type: int
    ///stream: IGR_Stream*
    ///offset: int
    ///whence: ULONG->unsigned int
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate Int64 IGR_Stream_Seek(IntPtr stream, Int64 offset, uint whence);

    // Return Type: void
    ///stream: IGR_Stream*
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void IGR_Stream_Close(IntPtr stream);

    // Return Type: ULONG->unsigned int
    ///stream: IGR_Writable_Stream*
    ///buffer: void*
    ///bufferSize: ULONG->unsigned int
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate UInt32 IGR_Writable_Stream_Write(IntPtr stream, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2), In] byte[] buffer, UInt32 bufferSize);

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Stream
    {

        // IGR_Stream_Open
        public IGR_Stream_Open Open;

        // IGR_Stream_Read
        public IGR_Stream_Read Read;

        // IGR_Stream_Seek
        public IGR_Stream_Seek Seek;

        // IGR_Stream_Close
        public IGR_Stream_Close Close;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Writable_Stream
    {

        // IGR_Stream
        public IGR_Stream @base;

        // IGR_Writable_Stream_Write
        public IGR_Writable_Stream_Write Write;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Page_Word
    {

        // int
        public int x;

        // int
        public int y;

        // int
        public int width;

        // int
        public int height;

        // int
        public int style;

        // WCHAR*
        //[MarshalAs(UnmanagedType.LPWStr)]
        public IntPtr _word;

        // int
        public int wordLength;

        // int
        public int charoffset;

        // int[8]
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8, ArraySubType = UnmanagedType.I4)]
        public int[] reserved;

        public string word
        {
            get
            {
                if (wordLength == 0)
                    return "";

                byte[] buffer = new byte[wordLength * 2];
                Marshal.Copy(_word, buffer, 0, wordLength * 2);
                return System.Text.Encoding.Unicode.GetString(buffer);
            }
        }
    }

    // Return Type: LONG->int
    ///actionID: int
    ///actionData: void*
    ///context: void*
    public delegate int IGR_CALLBACK(int actionID, System.IntPtr actionData, System.IntPtr context);

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_T_ACTION_GET_STREAM_PART
    {

        // unsigned int
        public uint size;

        // wchar_t*
        [MarshalAs(UnmanagedType.LPWStr)]
        public string partName;

        // wchar_t*
        [MarshalAs(UnmanagedType.LPWStr)]
        public string partFullName;

        // int
        public int partIndex;

        // IGR_Stream*
        public System.IntPtr istr;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Render_Page_Form_Values
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        public string name;

        [MarshalAs(UnmanagedType.LPWStr)]
        public string value;

        public int selected;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Render_Page_Redactions
    {
        public uint x;
        public uint y;
        public uint width;
        public uint height;
        public uint color;
        public uint reserved;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Rect
    {
        public uint left;

        public uint top;

        public uint right;

        public uint bottom;

        public IGR_Rect(Rectangle other)
        {
            left   = (uint)other.Left;
            top    = (uint)other.Top;
            right  = (uint)other.Right;
            bottom = (uint)other.Bottom;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Size
    {
        public uint width;

        public uint height;

        public IGR_Size(Size other)
        {
            width = (uint)other.Width;
            height = (uint)other.Height;
        }
    }

        
    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Render_Page_Properties
    {
        // unsigned int
        public uint size;

        public uint form_value_count;

        public IntPtr form_values;

        public uint redaction_flags;

        public uint redaction_count;

        public IntPtr redactions;

        public IGR_Rect source_rect;

        public IGR_Rect dest_rect;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Bookmark
    {

        // void*
        public System.IntPtr reserved;

        // IGR_LONG->int
        public int flags;

        // IGR_UCS2[255]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 255)]
        public string title;

        // IGR_LONG->int
        public int action;

        // IGR_UCS2[1024]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string dest;

        // IGR_LONG->int
        public int level;

        // IGR_LONG->int
        public int fit;

        // IGR_LONG->int
        public int zoom;

        // IGR_LONG->int
        public int x;

        // IGR_LONG->int
        public int y;

        // IGR_LONG->int
        public int width;

        // IGR_LONG->int
        public int height;

        // IGR_LONG->int
        public int color;

        // IGR_LONG->int
        public int text_style;

        // IGR_LONG[8]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst = 8, ArraySubType = System.Runtime.InteropServices.UnmanagedType.I4)]
        public int[] reserved2;
    }

    public delegate int IGR_CALLBACK_BOOKMARK(ref IGR_Bookmark element, System.IntPtr context);

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Hyperlink
    {
        // void*
        public System.IntPtr reserved;

        // IGR_HYPERLINK_ACTION_TYPE->IGR_LONG->int
        public int type;

        // IGR_LONG->int
        public int x;

        // IGR_LONG->int
        public int y;

        // IGR_LONG->int
        public int width;

        // IGR_LONG->int
        public int height;

        // IGR_LONG->int
        public int page_number;

        // IGR_HYPERLINK_FIT_TYPE->IGR_LONG->int
        public int dest_fit;

        // IGR_LONG->int
        public int dest_zoom;

        // IGR_LONG->int
        public int dest_left;

        // IGR_LONG->int
        public int dest_top;

        // IGR_LONG->int
        public int dest_right;

        // IGR_LONG->int
        public int dest_bottom;

        // IGR_HYPERLINK_FLAGS_TYPE->IGR_LONG->int
        public int flags;

        // IGR_LONG[8]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst = 8, ArraySubType = System.Runtime.InteropServices.UnmanagedType.I4)]
        public int[] reserved2;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Annotation
    {
        // void*
        public System.IntPtr reserved;

        // IGR_ANNOTATION_TYPE->IGR_LONG->int
        public int type;
        // IGR_ANNOTATION_FLAGS_TYPE->IGR_LONG->int
        public int flags;

        // IGR_LONG->int
        public int x;
        // IGR_LONG->int
        public int y;
        // IGR_LONG->int
        public int width;
        // IGR_LONG->int
        public int height;

        // IGR_LONG[8]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst = 8, ArraySubType = System.Runtime.InteropServices.UnmanagedType.I4)]
        public int[] reserved2;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Open_DIB_Info
    {
        // Indicate the size of this structure, must be populated
        public uint struct_size;
        public uint flags;
        // Indicate the width of the bitmap, in pixels
        public uint width;
        // Indicate the height of the bitmap, in pixels
        public uint height;
        // Indicate how may bytes per scanline
        public uint stride;
        // Indicate the format of the pixel data
        public uint pixel_format;
        // Pointer to the raw pixel data
        public IntPtr pixel_data;
        // Indicate the size of the buffer in pixel_data
        public uint pixel_data_size;
        // Pointer to the RGB palette information
        public IntPtr palette;
        // Indicate the number of colors in the palette
        public uint palette_count;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Page_Pixels
    {
        // Number of pixel in one scan line of the extracted pixels.
        public uint width;
        // Number of scanlines extracted.
        public uint height;
        // Offset, in bytes, between consecutive scan lines of the page. If the stride is positive, the scan lines are top-down. If the stride is negative, the scan lines are bottom-up.
        public int stride;
        // Integer that specifies the pixel format of the extracted pixels.
        public int pixel_format;
        // Integer than specifies and pixel format flags
        public int flags;
        // Pointer to the first scan line extracted.
        public IntPtr scanline0;
        // Indicates the number of palette colors extracted, if any
        public uint palette_count;
        // Indicates the palette colors extracted
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst = 256, ArraySubType = System.Runtime.InteropServices.UnmanagedType.I4)]
        public int[] palette;
        // Reserved for internal use
        public IntPtr reserved;
    }

    /**
    * Structure that is payload argument for IGR_OPEN_CALLBACK when action argument is IGR_OPEN_CALLBACK_ACTION_HEARTBEAT.
    */
    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public class IGR_Open_Callback_Action_Heartbeat
    { 
        // [out] Indicate the size of this structure; must be populated.
        public uint struct_size;
    };

    /**
	* Structure that is payload argument for IGR_OPEN_CALLBACK when action argument is IGR_OPEN_CALLBACK_ACTION_PASSWORD.
	*/
    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public class IGR_Open_Callback_Action_Password
    {
        // [out] Indicate the size of this structure; must be populated.
        public uint struct_size;
        // [out] Subfile Id indicating which subfile the password is requested for. May be empty to indicate the top level document password is requested. Null terminated.
        // IGR_UCS2[4096]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 4096)]
        public string id;

        // [in] Password string to be used to decode the requested document or subfile. Must be null terminated.
        // IGR_UCS2[4096]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 4096)]
        public string password;
    };

    /**
	* Structure that is payload argument for IGR_OPEN_CALLBACK when action argument is IGR_OPEN_CALLBACK_ACTION_LOCALIZE.
	*/
    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public class IGR_Open_Callback_Action_Localize
    {
        // [out] Indicate the size of this structure; must be populated.
        public uint struct_size;
        public uint string_id;
        // Original string
        // IGR_UCS2[4096]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string original;
        // Replacement string
        // IGR_UCS2[4096]
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string replacement;
    };


    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Ansi)]
    public class IGR_Open_Callback_Action_Log_Level
    {
        // [out] Indicate the size of this structure; must be populated.
        public uint struct_size;

        // Indicates the module that the log level is being requested.
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 16)]
        public string module;

        // User sets with the desired log level for the module. Defaults to IGR_LOG_LEVEL_NONE
        public uint result;
    };

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Ansi)]
    public class IGR_Open_Callback_Action_Log_Message
    {
        // Indicates the size of this structure; must be populated.
        public uint struct_size;

        // Indicates the module that generated the message.
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 16)]
        public string module;

        // Indicates the log level of the message.
        public uint log_level;

        // Contains the log message.
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst = 512)]
        public string message;
    };


    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Compare_Documents_Callback_Context
    {
        public IGR_ULONG struct_size;
        public IGR_ULONG doc_index;
        public IGR_LONG doc_handle;
        public IGR_ULONG page_index;
        public IGR_HPAGE page_handle;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_FRect
    {
        public float left;
        public float top;
        public float right;
        public float bottom;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Compare_Documents_Difference_Item
    {
        public IGR_ULONG struct_size;
        public IGR_ULONG page_index;
        public IGR_FRect bounds;
        [System.Runtime.InteropServices.MarshalAsAttribute(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
        public string text;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Compare_Documents_Difference
    {
        public IGR_ULONG struct_size;
        public IGR_ULONG type;
        public IGR_ULONG doc_source;
        public IGR_ULONG original_page_index;
        public IGR_ULONG revised_page_index;
        public IGR_ULONG item_count;
        public IntPtr items;
    }

    public delegate IGR_RETURN_CODE IGR_Text_Compare_Settings_OpenPage(IntPtr a, IGR_Compare_Documents_Callback_Context b, IGR_ULONG c, IntPtr d);
    public delegate IGR_RETURN_CODE IGR_Text_Compare_Settings_ClosePage(IntPtr a, IGR_Compare_Documents_Callback_Context b, IntPtr c);
    public delegate IGR_RETURN_CODE IGR_Text_Compare_Settings_GetPageMargins(IntPtr a, IGR_Compare_Documents_Callback_Context b, IntPtr c);
    public delegate IGR_RETURN_CODE IGR_Text_Compare_Settings_GetPageArea(IntPtr a, IGR_Compare_Documents_Callback_Context b, IntPtr c);
    public delegate IGR_RETURN_CODE IGR_Text_Compare_Settings_GetNextPageIndex(IntPtr a, IGR_Compare_Documents_Callback_Context b, IntPtr c);
    public delegate IGR_RETURN_CODE IGR_Text_Compare_Settings_HandleError(IntPtr a, IGR_Compare_Documents_Callback_Context b, IntPtr c);


    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Text_Compare_Settings
    {
        public IGR_ULONG struct_size;
        public IGR_ULONG compare_type;
        public IGR_ULONG flags;
        public IntPtr user_context;
        public IGR_ULONG comparison_window;
        public IGR_ULONG comparison_overlap;
        public IntPtr reserved1;
        public IntPtr reserved2;
        public IntPtr reserved3;
        public IntPtr reserved4;
        public IGR_Text_Compare_Settings_OpenPage OpenPage;
        public IGR_Text_Compare_Settings_ClosePage ClosePage;
        public IGR_Text_Compare_Settings_GetPageMargins GetPageMargins;
        public IGR_Text_Compare_Settings_GetPageArea GetPageArea;
        public IGR_Text_Compare_Settings_GetNextPageIndex GetNextPageIndex;
        public IGR_Text_Compare_Settings_HandleError HandleError;
    }

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Text_Compare_Document_Source
    {
        public IGR_ULONG struct_size;
        public IGR_LONG doc_handle;
        public IGR_ULONG doc_first_page;
        public IGR_ULONG doc_page_count;
        public IGR_FRect doc_margins;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Page_Element
    {
        public uint struct_size;
        public IntPtr reserved;
        public IGR_ULONG type;
        public uint depth;
        public IGR_ULONG flags;
        public IGR_FRect pos;
        public uint rotation;
    }

    public delegate int IGR_PAGE_ELEMENT_STYLES_CALLBACK([MarshalAs(UnmanagedType.LPWStr)] string name, [MarshalAs(UnmanagedType.LPWStr)] string value, IntPtr context);
    public delegate int IGR_PAGE_ELEMENT_CALLBACK(IntPtr page, ref IGR_Page_Element item, IntPtr context);

    [System.Runtime.InteropServices.StructLayoutAttribute(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
    public struct IGR_Subfile_Info
    {
        public uint struct_size;             // < Size of this structure, must be populated.
        public uint flags;                   // < Flags indicating specific characteristics.
        public IntPtr id;                    // < Buffer to be populated with the id.
        public uint id_size;                 // < Number of characters pointed to by id.
        public IntPtr name;                  // < Buffer to be populated with the name.
        public uint name_size;               // < Number of characters pointed to by name.
        public IntPtr comment;               // < Buffer to be populated with the comment.
        public uint comment_size;            // < Number of characters pointed to by comment.
        public ulong size;                   // < Size in bytes of the sub-document.
        public ulong date;                   // < Date and time of the sub-document in FILETIME
    }

    public partial class ISYS11df
    {
        internal class Runtime
        {
#if !NETSTANDARD
            static private IntPtr _libraryHandle;
            static private object _libraryLock = new object();
            static internal void Prepare()
            {
                lock (_libraryLock)
                {
                    if (_libraryHandle.ToInt64() == 0)
                    {
                        string assemblyFilename = System.Reflection.Assembly.GetCallingAssembly().Location;
                        string assemblyPath = System.IO.Path.GetDirectoryName(assemblyFilename);
                        string runtimePath = System.IO.Path.Combine(assemblyPath, ((IntPtr.Size == 8) ? "x64" : "x86"));
                        string libraryFilename = System.IO.Path.Combine(runtimePath, "ISYS11df.dll");
                        if (System.IO.File.Exists(libraryFilename))
                        {
                            _libraryHandle = LoadLibraryEx(libraryFilename
                                , IntPtr.Zero
                                , LoadLibraryFlags.LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LoadLibraryFlags.LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                        }
                    }
                }
            }
            [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool SetDllDirectory(string lpPathName);

            [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Ansi)]
            static extern IntPtr LoadLibrary([MarshalAs(UnmanagedType.LPStr)]string lpFileName);

            [DllImport("kernel32.dll", SetLastError = true)]
            static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hReservedNull, LoadLibraryFlags dwFlags);

            [System.Flags]
            enum LoadLibraryFlags : uint
            {
                None = 0,
                DONT_RESOLVE_DLL_REFERENCES = 0x00000001,
                LOAD_IGNORE_CODE_AUTHZ_LEVEL = 0x00000010,
                LOAD_LIBRARY_AS_DATAFILE = 0x00000002,
                LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE = 0x00000040,
                LOAD_LIBRARY_AS_IMAGE_RESOURCE = 0x00000020,
                LOAD_LIBRARY_SEARCH_APPLICATION_DIR = 0x00000200,
                LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = 0x00001000,
                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR = 0x00000100,
                LOAD_LIBRARY_SEARCH_SYSTEM32 = 0x00000800,
                LOAD_LIBRARY_SEARCH_USER_DIRS = 0x00000400,
                LOAD_WITH_ALTERED_SEARCH_PATH = 0x00000008
            }
#else
            static internal void Prepare()
            {

            }
#endif
        }


        // Return Type: void
        ///reserved: LONG->int
        ///path: char*
        ///init: Instance_Status_Block*
        ///handle: SHORT*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Init_Instance")]
        public static extern void Init_Instance(int reserved, [In()] [MarshalAs(UnmanagedType.LPStr)] string path, ref Instance_Status_Block init, ref short handle, ref Error_Control_Block error);


        // Return Type: void
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Close_Instance")]
        public static extern void Close_Instance(ref Error_Control_Block error);


        // Return Type: void
        ///handle: SHORT->short
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Switch_Instance")]
        public static extern void Switch_Instance(short handle, ref Error_Control_Block error);


        // Return Type: void
        ///source: LPCWSTR->WCHAR*
        ///buffer: LPSTR->CHAR*
        ///bufferSize: LONG->int
        [DllImport("ISYS11df", EntryPoint = "Widechar_to_UTF8")]
        public static extern void Widechar_to_UTF8([In()] [MarshalAs(UnmanagedType.LPWStr)] string source, [MarshalAs(UnmanagedType.LPStr)] System.Text.StringBuilder buffer, int bufferSize);


        // Return Type: void
        ///source: LPCWSTR->WCHAR*
        ///sourceLen: LONG->int
        ///buffer: LPSTR->CHAR*
        ///bufferSize: LONG->int
        [DllImport("ISYS11df", EntryPoint = "Widechar_to_UTF8_Ex")]
        public static extern void Widechar_to_UTF8_Ex([In()] [MarshalAs(UnmanagedType.LPWStr)] string source, int sourceLen, [MarshalAs(UnmanagedType.LPStr)] System.Text.StringBuilder buffer, int bufferSize);


        // Return Type: void
        ///source: LPCSTR->CHAR*
        ///buffer: LPWSTR->WCHAR*
        ///bufferSize: LONG->int
        [DllImport("ISYS11df", EntryPoint = "UTF8_to_Widechar")]
        public static extern void UTF8_to_Widechar([In()] [MarshalAs(UnmanagedType.LPStr)] string source, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, int bufferSize);


        // Return Type: void
        ///source: LPCSTR->CHAR*
        ///sourceLen: LONG->int
        ///buffer: LPWSTR->WCHAR*
        ///bufferSize: LONG->int
        [DllImport("ISYS11df", EntryPoint = "UTF8_to_Widechar_Ex")]
        public static extern void UTF8_to_Widechar_Ex([In()] [MarshalAs(UnmanagedType.LPStr)] string source, int sourceLen, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, int bufferSize);


        // Return Type: void
        ///param0: LONG->int
        ///param1: LONG->int
        ///param2: LPCSTR->CHAR*
        [DllImport("ISYS11df", EntryPoint = "Set_Engine_Call_Tracing")]
        public static extern void Set_Engine_Call_Tracing(int param0, int param1, [In()] [MarshalAs(UnmanagedType.LPStr)] string param2);


        // Return Type: void
        ///param0: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Set_API_Charset")]
        public static extern void Set_API_Charset(int param0, ref Error_Control_Block error);


        // Return Type: void
        ///param0: LPCSTR->CHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Set_DLL_Option")]
        public static extern void Set_DLL_Option([In()] [MarshalAs(UnmanagedType.LPStr)] string param0, ref Error_Control_Block error);


        // Return Type: void
        ///param0: LONG->int
        ///param1: size_t*
        ///param2: size_t*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "ISYS_Multiplex")]
        public static extern void ISYS_Multiplex(int param0, ref uint param1, ref uint param2, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_File_Type")]
        public static extern int IGR_Get_File_Type([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, ref int capabilities, ref int filetype, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Stream_Type")]
        public static extern int IGR_Get_Stream_Type(IntPtr stream, ref int capabilities, ref int filetype, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///flags: LONG->int
        ///capabilities: LONG*
        ///filetype: LONG*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_File")]
        public static extern int IGR_Open_File([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, int flags, ref int capabilities, ref int filetype, ref int handle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///flags: LONG->int
        ///capabilities: LONG*
        ///filetype: LONG*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_Stream")]
        public static extern int IGR_Open_Stream(IntPtr stream, int flags, ref int capabilities, ref int filetype, ref int handle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///source_type: IGR_OPEN_FROM->int
        ///source: void*
        ///flags: LONG->int
        ///options: LPCWSTR->WCHAR*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///reserved: void*
        ///callback: IGR_CALLBACK->void*
        ///callback_context: void*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_Ex")]
        public static extern int IGR_Open_Ex(int source_type, System.IntPtr source, int flags, [In()][MarshalAs(UnmanagedType.LPWStr)] string options, ref int capabilities, ref int filetype, System.IntPtr reserved, System.IntPtr callback, System.IntPtr callback_context, ref int handle, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///flags: LONG->int
        ///options: LPCWSTR->WCHAR*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_File_Ex")]
        public static extern int IGR_Open_File_Ex([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, int flags, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref int capabilities, ref int filetype, ref int handle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///flags: LONG->int
        ///options: LPCWSTR->WCHAR*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_Stream_Ex")]
        public static extern int IGR_Open_Stream_Ex(IntPtr stream, int flags, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref int capabilities, ref int filetype, ref int handle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///buffer: LPWSTR->WCHAR*
        ///bufferSize: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Text")]
        public static extern int IGR_Get_Text(int handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, ref int bufferSize, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPWSTR->WCHAR*
        ///name: LPWSTR->WCHAR*
        ///date: PLONGLONG->LONGLONG*
        ///size: PLONGLONG->LONGLONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Subfile_Entry")]
        public static extern int IGR_Get_Subfile_Entry(int handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPCWSTR->WCHAR*
        ///destination: LPCWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Subfile")]
        public static extern int IGR_Extract_Subfile(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, [In()] [MarshalAs(UnmanagedType.LPWStr)] string destination, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPCWSTR->WCHAR*
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Subfile_Stream")]
        public static extern int IGR_Extract_Subfile_Stream(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, ref System.IntPtr stream, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Close_File")]
        public static extern int IGR_Close_File(int handle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///function: LONG->int
        ///param1: size_t*
        ///param2: size_t*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Multiplex")]
        public static extern int IGR_Multiplex(int function, ref uint param1, ref uint param2, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///flags: LONG->int
        ///options: LPCWSTR->WCHAR*
        ///output: LPCWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Convert_File")]
        public static extern int IGR_Convert_File([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, int flags, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, [In()] [MarshalAs(UnmanagedType.LPWStr)] string output, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///flags: LONG->int
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_Stream_From_File")]
        public static extern int IGR_Make_Stream_From_File([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, int flags, ref System.IntPtr stream, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///buffer: void*
        ///bufferSize: size_t->unsigned int
        ///destruct: void*
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_Stream_From_Memory")]
        public static extern int IGR_Make_Stream_From_Memory(System.IntPtr buffer, [MarshalAs(UnmanagedType.SysUInt)] IntPtr bufferSize, System.IntPtr destruct, ref System.IntPtr stream, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pStream: IGR_Stream*
        ///cb: IGR_CALLBACK
        ///context: void*
        ///ppExtStream: IGR_Stream**
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extend_Stream")]
        public static extern int IGR_Extend_Stream(IntPtr stream, IGR_CALLBACK cb, System.IntPtr context, ref System.IntPtr ppExtStream, ref Error_Control_Block error);


        // Return Type: size_t->unsigned int
        ///buffer: LPSTR->CHAR*
        ///len: size_t->unsigned int
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Memory_Status")]
        [return: MarshalAs(UnmanagedType.SysUInt)]
        public static extern uint IGR_Get_Memory_Status([MarshalAs(UnmanagedType.LPStr)] System.Text.StringBuilder buffer, [MarshalAs(UnmanagedType.SysUInt)] uint len);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPWSTR->WCHAR*
        ///name: LPWSTR->WCHAR*
        ///date: PLONGLONG->LONGLONG*
        ///size: PLONGLONG->LONGLONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Image_Entry")]
        public static extern int IGR_Get_Image_Entry(int handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPCWSTR->WCHAR*
        ///destination: LPCWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Image")]
        public static extern int IGR_Extract_Image(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, [In()] [MarshalAs(UnmanagedType.LPWStr)] string destination, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPCWSTR->WCHAR*
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Image_Stream")]
        public static extern int IGR_Extract_Image_Stream(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, ref System.IntPtr stream, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///dest: LPWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Calculate_MD5")]
        public static extern int IGR_Calculate_MD5(IntPtr stream, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder dest, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///dest: LPWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Calculate_SHA1")]
        public static extern int IGR_Calculate_SHA1(IntPtr stream, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder dest, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///docHandle: LONG->int
        ///pageCount: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Count")]
        public static extern int IGR_Get_Page_Count(int docHandle, ref int pageCount, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///docHandle: LONG->int
        ///pageIndex: LONG->int
        ///pageHandle: HPAGE*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_Page")]
        public static extern int IGR_Open_Page(int docHandle, int pageIndex, ref int pageHandle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Close_Page")]
        public static extern int IGR_Close_Page(int pageHandle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///wordCount: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Word_Count")]
        public static extern int IGR_Get_Page_Word_Count(int pageHandle, ref int wordCount, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///index: LONG->int
        ///count: LONG*
        ///words: IGR_Page_Word*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Words")]
        public static extern int IGR_Get_Page_Words(int pageHandle, int index, ref int count, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2), Out] IGR_Page_Word[] words, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///width: LONG*
        ///height: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Dimensions")]
        public static extern int IGR_Get_Page_Dimensions(int pageHandle, ref int width, ref int height, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///buffer: WCHAR*
        ///size: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Text")]
        public static extern int IGR_Get_Page_Text(int pageHandle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, ref int size, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///id: LPWSTR->WCHAR*
        ///name: LPWSTR->WCHAR*
        ///date: PLONGLONG->LONGLONG*
        ///size: PLONGLONG->LONGLONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Image_Entry")]
        public static extern int IGR_Get_Page_Image_Entry(int pageHandle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///id: LPCWSTR->WCHAR*
        ///destination: LPCWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Page_Image")]
        public static extern int IGR_Extract_Page_Image(int pageHandle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, [In()] [MarshalAs(UnmanagedType.LPWStr)] string destination, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///id: LPCWSTR->WCHAR*
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Page_Image_Stream")]
        public static extern int IGR_Extract_Page_Image_Stream(int pageHandle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, ref System.IntPtr stream, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///type: LONG->int
        ///filename: WCHAR*
        ///options: WCHAR*
        ///canvasHandle: HCANVAS*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_Output_Canvas")]
        public static extern int IGR_Make_Output_Canvas(int type, [In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref int canvasHandle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///type: LONG->int
        ///stream: IGR_Writable_Stream*
        ///options: WCHAR*
        ///canvasHandle: HCANVAS*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_Output_Canvas_On")]
        public static extern int IGR_Make_Output_Canvas_On(int type, IntPtr stream, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref int canvasHandle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvasHandle: HCANVAS->LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Close_Canvas")]
        public static extern int IGR_Close_Canvas(int canvasHandle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///page: HPAGE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Render_Page")]
        public static extern int IGR_Render_Page(int page, int canvas, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///page: HPAGE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///options: WCHAR*
        ///properties: properties that control per page rendering
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Render_Page_Ex")]
        public static extern int IGR_Render_Page_Ex(int page, int canvas, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref IGR_Render_Page_Properties properties, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///page: HPAGE->LONG->int
        ///From: LONG->int
        ///To: LONG->int
        ///Type: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Redact_Page_Text")]
        public static extern int IGR_Redact_Page_Text(int page, int From, int To, int Type, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///page: HPAGE->LONG->int
        ///name: WCHAR*
        ///buffer: WCHAR*
        ///size: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Attribute")]
        public static extern int IGR_Get_Page_Attribute(int page, [In()] [MarshalAs(UnmanagedType.LPWStr)] string name, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, ref int size, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///formatid: LONG->int
        ///what: LONG->int
        ///buffer: char*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Format_Attribute", CharSet = CharSet.Ansi)]
        public static extern int IGR_Get_Format_Attribute(int formatid, int what, StringBuilder buffer, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///dest_filename: WCHAR*
        ///options: WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_DocPack")]
        public static extern int IGR_Make_DocPack(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string dest_filename, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///dest_stream: IGR_WRITEABLE_STREAM*
        ///options: WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_DocPack_On")]
        public static extern int IGR_Make_DocPack_On(int handle, IntPtr dest_stream, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///x3: LONG->int
        ///y3: LONG->int
        ///x4: LONG->int
        ///y4: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Arc")]
        public static extern int IGR_Canvas_Arc(int canvas, int x, int y, int x2, int y2, int x3, int y3, int x4, int y4, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///radius: LONG->int
        ///startAngle: LONG->int
        ///sweepAngle: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_AngleArc")]
        public static extern int IGR_Canvas_AngleArc(int canvas, int x, int y, int radius, int startAngle, int sweepAngle, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///x3: LONG->int
        ///y3: LONG->int
        ///x4: LONG->int
        ///y4: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Chord")]
        public static extern int IGR_Canvas_Chord(int canvas, int x, int y, int x2, int y2, int x3, int y3, int x4, int y4, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Ellipse")]
        public static extern int IGR_Canvas_Ellipse(int canvas, int x, int y, int x2, int y2, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Rect")]
        public static extern int IGR_Canvas_Rect(int canvas, int x, int y, int x2, int y2, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_LineTo")]
        public static extern int IGR_Canvas_LineTo(int canvas, int x, int y, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_MoveTo")]
        public static extern int IGR_Canvas_MoveTo(int canvas, int x, int y, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///x3: LONG->int
        ///y3: LONG->int
        ///x4: LONG->int
        ///y4: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Pie")]
        public static extern int IGR_Canvas_Pie(int canvas, int x, int y, int x2, int y2, int x3, int y3, int x4, int y4, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///radius: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_RoundRect")]
        public static extern int IGR_Canvas_RoundRect(int canvas, int x, int y, int x2, int y2, int radius, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///text: WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_TextOut")]
        public static extern int IGR_Canvas_TextOut(int canvas, int x, int y, [In()] [MarshalAs(UnmanagedType.LPWStr)] string text, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///text: WCHAR*
        ///flags: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_TextRect")]
        public static extern int IGR_Canvas_TextRect(int canvas, int x, int y, int x2, int y2, [In()] [MarshalAs(UnmanagedType.LPWStr)] string text, int flags, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///text: WCHAR*
        ///width: LONG*
        ///height: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_MeasureText")]
        public static extern int IGR_Canvas_MeasureText(int canvas, [In()] [MarshalAs(UnmanagedType.LPWStr)] string text, ref int width, ref int height, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///color: LONG->int
        ///width: LONG->int
        ///style: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_SetPen")]
        public static extern int IGR_Canvas_SetPen(int canvas, int color, int width, int style, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///color: LONG->int
        ///style: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_SetBrush")]
        public static extern int IGR_Canvas_SetBrush(int canvas, int color, int style, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///fontFamily: WCHAR*
        ///size: LONG->int
        ///style: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_SetFont")]
        public static extern int IGR_Canvas_SetFont(int canvas, [In()] [MarshalAs(UnmanagedType.LPWStr)] string fontFamily, int size, int style, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///opacity: BYTE->unsigned char
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_SetOpacity")]
        public static extern int IGR_Canvas_SetOpacity(int canvas, byte opacity, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///imagedata: void*
        ///imagesize: size_t->unsigned int
        ///mimetype: WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_DrawImage")]
        public static extern int IGR_Canvas_DrawImage(int canvas, int x, int y, Byte[] imagedata, [MarshalAs(UnmanagedType.SysUInt)] IntPtr imagesize, [In()] [MarshalAs(UnmanagedType.LPWStr)] string mimetype, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///imagedata: void*
        ///imagesize: size_t->unsigned int
        ///mimetype: WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_DrawScaleImage")]
        public static extern int IGR_Canvas_DrawScaleImage(int canvas, int x, int y, int x2, int y2, Byte[] imagedata, [MarshalAs(UnmanagedType.SysUInt)] IntPtr imagesize, [In()] [MarshalAs(UnmanagedType.LPWStr)] string mimetype, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///degrees: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Rotation")]
        public static extern int IGR_Canvas_Rotation(int canvas, int degrees, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Reset")]
        public static extern int IGR_Canvas_Reset(int canvas, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->LONG->int
        ///formatid: LONG->int
        ///what: LONG->int
        ///buffer: char*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Option_Attribute", CharSet = CharSet.Ansi)]
        public static extern int IGR_Get_Option_Attribute(int formatid, int what, StringBuilder buffer, [MarshalAs(UnmanagedType.SysUInt)] IntPtr bufferSize, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///pageHandle: HPAGE->IGR_LONG->int
        ///count: IGR_LONG*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Form_Element_Count")]
        public static extern int IGR_Get_Page_Form_Element_Count(int pageHandle, ref int count, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///pageHandle: HPAGE->IGR_LONG->int
        ///index: IGR_LONG->int
        ///count: IGR_LONG*
        ///items: IGR_Page_Form_Element*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Form_Elements")]
        public static extern int IGR_Get_Page_Form_Elements(int pageHandle, int index, ref int count, ref IGR_Page_Form_Element items, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///items: IGR_Page_Form_Element*
        ///value_type: IGR_LONG->int
        ///char_count: IGR_LONG->int
        ///buffer: IGR_UCS2*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Form_Element_Str")]
        public static extern int IGR_Get_Page_Form_Element_Str(ref IGR_Page_Form_Element items, int value_type, int char_count, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder buffer, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///items: IGR_Page_Form_Element*
        ///value_type: IGR_LONG->int
        ///option_index: IGR_LONG->int
        ///char_count: IGR_LONG->int
        ///buffer: IGR_UCS2*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Form_Element_Option_Str")]
        public static extern int IGR_Get_Page_Form_Element_Option_Str(ref IGR_Page_Form_Element items, int value_type, int option_index, int char_count, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder buffer, ref Error_Control_Block error);
        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///docHandle: IGR_LONG->int
        ///root: IGR_Bookmark*
        ///recurse_depth: IGR_LONG->int
        ///callback: IGR_CALLBACK_BOOKMARK
        ///context: void*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Enum_Bookmarks")]
        public static extern int IGR_Enum_Bookmarks(int docHandle, ref IGR_Bookmark root, int recurse_depth, IGR_CALLBACK_BOOKMARK callback, System.IntPtr context, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///docHandle: IGR_LONG->int
        ///dest: IGR_Bookmark*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Bookmark_Root")]
        public static extern int IGR_Get_Bookmark_Root(int docHandle, ref IGR_Bookmark dest, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///docHandle: IGR_LONG->int
        ///root: IGR_Bookmark*
        ///dest: IGR_Bookmark*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Bookmark_First_Child")]
        public static extern int IGR_Get_Bookmark_First_Child(int docHandle, ref IGR_Bookmark root, ref IGR_Bookmark dest, ref Error_Control_Block error);


        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///docHandle: IGR_LONG->int
        ///root: IGR_Bookmark*
        ///dest: IGR_Bookmark*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Bookmark_Next_Sibling")]
        public static extern int IGR_Get_Bookmark_Next_Sibling(int docHandle, ref IGR_Bookmark root, ref IGR_Bookmark dest, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Canvas_Annotate_JSON")]
        public static extern int IGR_Canvas_Annotate_JSON(int canvas, [In()] [MarshalAs(UnmanagedType.LPWStr)] string json, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///pageHandle: HPAGE->IGR_LONG->int
        ///count: IGR_LONG*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Hyperlink_Count")]
        public static extern int IGR_Get_Page_Hyperlink_Count(int pageHandle, ref int count, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///pageHandle: HPAGE->IGR_LONG->int
        ///index: IGR_LONG->int
        ///count: IGR_LONG*
        ///items: IGR_Hyperlink*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Hyperlinks")]
        public static extern int IGR_Get_Page_Hyperlinks(int pageHandle, int index, ref int count, ref IGR_Hyperlink items, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///items: IGR_Page_Form_Element*
        ///value_type: IGR_LONG->int
        ///char_count: IGR_LONG->int
        ///buffer: IGR_UCS2*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Hyperlink_Str")]
        public static extern int IGR_Get_Page_Hyperlink_Str(ref IGR_Hyperlink items, int value_type, int char_count, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder buffer, ref Error_Control_Block error);


        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Subfiles_Enumerator")]
        public static extern int IGR_Get_Subfiles_Enumerator(int docHandle, ref System.IntPtr enumerator, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Subfiles_Next")]
        public static extern int IGR_Subfiles_Next(System.IntPtr handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Subfiles_Next_Ex")]
        public static extern int IGR_Subfiles_Next_Ex(System.IntPtr handle, ref IGR_Subfile_Info result, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Subfiles_Reset")]
        public static extern int IGR_Subfiles_Reset(System.IntPtr handle, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Subfiles_Close")]
        public static extern int IGR_Subfiles_Close(System.IntPtr handle, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Images_Enumerator")]
        public static extern int IGR_Get_Images_Enumerator(int docHandle, ref System.IntPtr enumerator, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Images_Enumerator")]
        public static extern int IGR_Get_Page_Images_Enumerator(int docHandle, ref System.IntPtr enumerator, ref Error_Control_Block error);

        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Blank_Page")]
        public static extern int IGR_Canvas_Blank_Page(int canvas, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, int width, int height, IntPtr reserved, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///pageHandle: HPAGE->IGR_LONG->int
        ///count: IGR_LONG*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Annotation_Count")]
        public static extern int IGR_Get_Page_Annotation_Count(int pageHandle, ref int count, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///pageHandle: HPAGE->IGR_LONG->int
        ///index: IGR_LONG->int
        ///count: IGR_LONG*
        ///items: IGR_Annotation*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Annotations")]
        public static extern int IGR_Get_Page_Annotations(int pageHandle, int index, ref int count, ref IGR_Annotation items, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///item: IGR_Annotation*
        ///path: IGR_UCS2*
        ///char_count: IGR_LONG*
        ///buffer: IGR_UCS2*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Annotation_Str")]
        public static extern int IGR_Get_Page_Annotation_Str(ref IGR_Annotation item, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder path, ref int char_count, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder buffer, ref Error_Control_Block error);

        // Return Type: IGR_RETURN_CODE->IGR_LONG->int
        ///item: IGR_Annotation*
        ///path: IGR_UCS2*
        ///buffer: IGR_LONG*
        ///error: Error_Control_Block*
        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Annotation_Long")]
        public static extern int IGR_Get_Page_Annotation_Long(ref IGR_Annotation item, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder path, ref int buffer, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Pixels")]
        public static extern int IGR_Get_Page_Pixels(int pageHandle,
            [In] ref IGR_Rect sourceRect,
            [In] ref IGR_Size destRect,
            [In] int flags,
            [In, MarshalAs(UnmanagedType.LPWStr)] string options,
            [In] int pixelFormat,
            ref IGR_Page_Pixels pixel_data,
            ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Free_Page_Pixels")]
        public static extern int IGR_Free_Page_Pixels(ref IGR_Page_Pixels pixel_data, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Canvas_Bookmarks_Clear")]
        public static extern int IGR_Canvas_Bookmarks_Clear(int canvasHandle, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Canvas_Bookmarks_Append")]
        public static extern int IGR_Canvas_Bookmarks_Append(int canvasHandle, ref IGR_Bookmark bookmark, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Text_Compare_Documents")]
        public static extern int IGR_Text_Compare_Documents(
            ref IGR_Text_Compare_Document_Source doc1,
            ref IGR_Text_Compare_Document_Source doc2,
            ref IGR_Text_Compare_Settings settings,
            out IntPtr enumerator,
            ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Text_Compare_Pages")]
        public static extern int IGR_Text_Compare_Pages(
            IGR_HPAGE page1,
            IntPtr page1_margins,
            IGR_HPAGE page2,
            IntPtr page2_margins,
            ref IGR_Text_Compare_Settings settings,
            out IntPtr enumerator,
            ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Text_Compare_Elements")]
        public static extern int IGR_Text_Compare_Elements(
            IntPtr page1,
            IntPtr page1_ele_root,
            IntPtr page2,
            IntPtr page2_ele_root,
            ref IGR_Text_Compare_Settings settings,
            out IntPtr enumerator,
            ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Text_Compare_Close")]
        public static extern int IGR_Text_Compare_Close(
            IntPtr enumerator,
            ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Text_Compare_Reset")]
        public static extern int IGR_Text_Compare_Reset(
            IntPtr enumerator,
            ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Text_Compare_Next")]
        public static extern int IGR_Text_Compare_Next(
            IntPtr enumerator,
            ref IGR_Compare_Documents_Difference result,
            ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Text_Compare_Difference_Dispose")]
        public static extern int IGR_Text_Compare_Difference_Dispose(
            ref IGR_Compare_Documents_Difference result,
            ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Elements")]
        public static extern IGR_RETURN_CODE IGR_Get_Page_Elements(IntPtr page, IntPtr parent, IGR_ULONG first, ref IGR_ULONG dest_count, [In, Out] IGR_Page_Element[] dest, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Element_Root")]
        public static extern IGR_RETURN_CODE IGR_Get_Page_Element_Root(IntPtr page, [In, Out] ref IGR_Page_Element dest, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Element_First_Child")]
        public static extern IGR_RETURN_CODE IGR_Get_Page_Element_First_Child(IntPtr page, ref IGR_Page_Element parent, [In, Out] ref IGR_Page_Element dest, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Element_Next_Sibling")]
        public static extern IGR_RETURN_CODE IGR_Get_Page_Element_Next_Sibling(IntPtr page, ref IGR_Page_Element element, [In, Out] ref IGR_Page_Element dest, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Element_Text")]
        public static extern IGR_RETURN_CODE IGR_Get_Page_Element_Text(IntPtr page, ref IGR_Page_Element source, ref IGR_ULONG char_count, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Element_Styles")]
        public static extern IGR_RETURN_CODE IGR_Get_Page_Element_Styles(IntPtr page, ref IGR_Page_Element source, IGR_PAGE_ELEMENT_STYLES_CALLBACK callback, IntPtr context, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Get_Page_Element_Style")]
        public static extern IGR_RETURN_CODE IGR_Get_Page_Element_Style(IntPtr page, ref IGR_Page_Element source, [In(), MarshalAs(UnmanagedType.LPWStr)] string name, ref IGR_ULONG char_count, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, ref Error_Control_Block error);

        [System.Runtime.InteropServices.DllImportAttribute("ISYS11df", EntryPoint = "IGR_Enum_Page_Elements")]
        public static extern IGR_RETURN_CODE IGR_Enum_Page_Elements(IntPtr page, ref IGR_Page_Element source, IGR_ULONG flags, IGR_ULONG max_depth, IGR_PAGE_ELEMENT_CALLBACK callback, IntPtr context, ref Error_Control_Block error);

    }
}
