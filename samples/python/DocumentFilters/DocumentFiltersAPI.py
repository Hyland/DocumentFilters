# (c) 2022 Hyland Software, Inc. and its affiliates. All rights reserved.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os, sys, ctypes

# Simple Types
IGR_UCS2 = ctypes.c_uint16
IGR_LONG = ctypes.c_int32
IGR_ULONG = ctypes.c_uint32
IGR_BYTE = ctypes.c_ubyte
IGR_SHORT = ctypes.c_int
IGR_USHORT = ctypes.c_uint
IGR_LONGLONG = ctypes.c_int64
IGR_ULONGLONG = ctypes.c_uint64
IGR_HANDLE = ctypes.POINTER(None)
IGR_RETURN_CODE = IGR_LONG
HPAGE = IGR_LONG
HCANVAS = IGR_LONG
HSUBFILES = ctypes.POINTER(None)

# Constants
PERCEPTIVE_SZ_MD5_HEX_LEN = 33
PERCEPTIVE_SZ_SHA1_HEX_LEN = 41
IGR_Multi_Set_Code_Page = 1
IGR_Multi_Set_Temp_Path = 186
IGR_BODY_ONLY = 0
IGR_META_ONLY = 1
IGR_BODY_AND_META = 2
IGR_FORMAT_TEXT = 65536
IGR_FORMAT_HTML = 131072
IGR_FORMAT_XML = 196608
IGR_FORMAT_HDHTML_IFYOUCAN = 262144
IGR_FORMAT_IMAGE = 327680
IGR_FORMAT_HTML_NO_FALLBACK = 393216
IGR_FILE_SUPPORTS_TEXT = 1
IGR_FILE_SUPPORTS_SUBFILES = 2
IGR_FILE_SUPPORTS_HDHTML = 4
IGR_OK = 0
IGR_E_OPEN_ERROR = 1
IGR_E_WRONG_TYPE = 2
IGR_E_IN_USE = 3
IGR_E_NOT_READABLE = 4
IGR_E_PASSWORD = 5
IGR_E_NOT_FOUND = 10
IGR_E_WRITE_ERROR = 11
IGR_E_NOT_VALID_FOR_THIS_CLASS = 12
IGR_E_ERROR = 13
IGR_E_INVALID_HANDLE = 14
IGR_E_INVALID_POINTER = 15
IGR_E_INVALID_PARAMETER = 16
IGR_E_FILE_CORRUPT = 17
IGR_NO_MORE = 19
IGR_E_OUT_OF_MEMORY = 20
IGR_E_BAD_ERROR = 21
IGR_E_WRONG_THREAD = 22
IGR_E_TOO_MANY_HANDLES = 23
IGR_E_FONTS_NOT_FOUND = 24
IGR_E_VERSION_MISMATCH = 25
IGR_E_ACTION_NOT_SUPPORTED = 26
IGR_CANCELLED = 27
IGR_CHAR_SOFT_SPACE = 1
IGR_CHAR_STYLE_NORMAL = 2
IGR_CHAR_STYLE_BOLD = 3
IGR_CHAR_STYLE_ITALICS = 4
IGR_CHAR_STYLE_UNDERLINED = 5
IGR_CHAR_TAB = 9
IGR_CHAR_PAGE_BREAK = 12
IGR_CHAR_LINE_BREAK = 13
IGR_CHAR_PARA_BREAK = 14
IGR_CHAR_START_META = 15
IGR_CHAR_END_META = 16
IGR_DEVICE_IMAGE_PNG = 0
IGR_DEVICE_IMAGE_JPG = 1
IGR_DEVICE_IMAGE_PDF = 2
IGR_DEVICE_IMAGE_TIF = 3
IGR_DEVICE_IMAGE_BMP = 4
IGR_DEVICE_IMAGE_PBM = 7
IGR_DEVICE_IMAGE_PGM = 8
IGR_DEVICE_IMAGE_PPM = 9
IGR_DEVICE_IMAGE_WEBP = 10
IGR_DEVICE_IMAGE_XPS = 11
IGR_DEVICE_IMAGE_SVG = 12
IGR_DEVICE_IMAGE_EPS = 13
IGR_DEVICE_IMAGE_PS = 14
IGR_DEVICE_XML = 5
IGR_DEVICE_HTML = 6
IGR_TEXT_ALIGN_LEFT = 0
IGR_TEXT_ALIGN_RIGHT = 1
IGR_TEXT_ALIGN_CENTER = 2
IGR_TEXT_ALIGN_TOP = 16
IGR_TEXT_ALIGN_MIDDLE = 17
IGR_TEXT_ALIGN_BOTTOM = 18
IGR_TEXT_STYLE_BOLD = 1
IGR_TEXT_STYLE_ITALIC = 2
IGR_TEXT_STYLE_UNDERLINE = 4
IGR_BRUSH_NONE = 0
IGR_BRUSH_SOLID = 1
IGR_PEN_NONE = 0
IGR_PEN_SOLID = 1
IGR_ACTION_GET_STREAM_PART = 10
IGR_PAGE_FORM_ELEMENT_TYPE_BUTTON = 0
IGR_PAGE_FORM_ELEMENT_TYPE_RADIOBUTTON = 1
IGR_PAGE_FORM_ELEMENT_TYPE_CHECKBOX = 2
IGR_PAGE_FORM_ELEMENT_TYPE_FILESELECT = 3
IGR_PAGE_FORM_ELEMENT_TYPE_MULTILINE_TEXT = 4
IGR_PAGE_FORM_ELEMENT_TYPE_SINGLELINE_TEXT = 5
IGR_PAGE_FORM_ELEMENT_TYPE_COMBOBOX = 6
IGR_PAGE_FORM_ELEMENT_TYPE_LISTBOX = 7
IGR_PAGE_FORM_ELEMENT_TYPE_SIGNATURE = 8
IGR_PAGE_FORM_ELEMENT_TYPE_PASSWORD = 9
IGR_PAGE_FORM_ELEMENT_FLAG_READONLY = (1 << 0)
IGR_PAGE_FORM_ELEMENT_FLAG_REQUIRED = (1 << 1)
IGR_PAGE_FORM_ELEMENT_FLAG_COMB = (1 << 2)
IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED = (1 << 3)
IGR_PAGE_FORM_ELEMENT_ALIGN_LEFT = 0
IGR_PAGE_FORM_ELEMENT_ALIGN_CENTER = 1
IGR_PAGE_FORM_ELEMENT_ALIGN_RIGHT = 2
IGR_PAGE_FORM_ELEMENT_TEXT_STYLE_BOLD = 1
IGR_PAGE_FORM_ELEMENT_TEXT_STYLE_ITALICS = 2

IGR_PAGE_FORM_ELEMENT_GET_NAME = 0
IGR_PAGE_FORM_ELEMENT_GET_VALUE = 1
IGR_PAGE_FORM_ELEMENT_GET_CAPTION = 2
IGR_PAGE_FORM_ELEMENT_GET_ACTION = 3
IGR_PAGE_FORM_ELEMENT_GET_ACTION_DEST = 4
IGR_PAGE_FORM_ELEMENT_GET_SELECTED = 5
IGR_PAGE_FORM_ELEMENT_GET_FONT_NAME = 6

IGR_BOOKMARK_FIT_NONE = 0
IGR_BOOKMARK_FIT_BOTH = 1
IGR_BOOKMARK_FIT_WIDTH = 2
IGR_BOOKMARK_FIT_HEIGHT = 3
IGR_BOOKMARK_FIT_RECT = 4

IGR_BOOKMARK_ACTION_GOTO = 0
IGR_BOOKMARK_ACTION_LINK = 1

IGR_HYPERLINK_GET_VALUE_REF = 0
IGR_HYPERLINK_GET_VALUE_URI = 1

IGR_HYPERLINK_ACTION_GOTO = 0x1
IGR_HYPERLINK_ACTION_URI =  0x2

IGR_HYPERLINK_FLAGS_CHANGE_LEFT = 0x1
IGR_HYPERLINK_FLAGS_CHANGE_TOP = 0x2
IGR_HYPERLINK_FLAGS_CHANGE_ZOOM = 0x4

IGR_HYPERLINK_FIT_XYZ = 0
IGR_HYPERLINK_FIT_FIT = 1
IGR_HYPERLINK_FIT_FITH = 2
IGR_HYPERLINK_FIT_FITV = 3
IGR_HYPERLINK_FIT_FITR = 4
IGR_HYPERLINK_FIT_FITB = 5
IGR_HYPERLINK_FIT_FITBH = 6
IGR_HYPERLINK_FIT_FITBV = 7

IGR_HYPERLINK_GET_VALUE_REF = 0
IGR_HYPERLINK_GET_VALUE_URI = 1

IGR_FORMAT_LONG_NAME = 0
IGR_FORMAT_SHORT_NAME = 1
IGR_FORMAT_CONFIG_NAME = 2
IGR_FORMAT_CLASS_NAME = 3
IGR_FORMAT_LEGACY = 4

IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_USECDECL = 1

IGR_ANNOTATION_UNKNOWN = 0
IGR_ANNOTATION_TEXT = 1
IGR_ANNOTATION_LINK = 2
IGR_ANNOTATION_FREETEXT = 3
IGR_ANNOTATION_LINE = 4
IGR_ANNOTATION_SQUARE = 5
IGR_ANNOTATION_CIRCLE = 6
IGR_ANNOTATION_POLYGON = 7
IGR_ANNOTATION_POLYLINE = 8
IGR_ANNOTATION_HIGHLIGHT = 9
IGR_ANNOTATION_UNDERLINE = 10
IGR_ANNOTATION_SQUIGGLE = 11
IGR_ANNOTATION_STRIKEOUT = 12
IGR_ANNOTATION_STAMP = 13
IGR_ANNOTATION_CARET = 14
IGR_ANNOTATION_INK = 15
IGR_ANNOTATION_POPUP = 16
IGR_ANNOTATION_FILEATTACHMENT = 17
IGR_ANNOTATION_SOUND = 18
IGR_ANNOTATION_MOVIE = 19
IGR_ANNOTATION_WIDGET = 20
IGR_ANNOTATION_SCREEN = 21
IGR_ANNOTATION_PRINTERMARK = 22
IGR_ANNOTATION_TRAPNET = 23
IGR_ANNOTATION_WATERMARK = 24
IGR_ANNOTATION_3D = 25
IGR_ANNOTATION_BARCODE = 26
IGR_ANNOTATION_NAMED_DESTINATION = 27

IGR_ANNOTATION_FLAGS_INVISIBLE = (1 << 0)
IGR_ANNOTATION_FLAGS_HIDDEN = (1 << 1)
IGR_ANNOTATION_FLAGS_PRINT = (1 << 2)
IGR_ANNOTATION_FLAGS_NOZOOM = (1 << 3)
IGR_ANNOTATION_FLAGS_NOROTATE = (1 << 4)
IGR_ANNOTATION_FLAGS_NOVIEW = (1 << 5)
IGR_ANNOTATION_FLAGS_READONLY = (1 << 6)
IGR_ANNOTATION_FLAGS_LOCKED = (1 << 7)
IGR_ANNOTATION_FLAGS_TOGGLENOVIEW = (1 << 8)
IGR_ANNOTATION_FLAGS_LOCKEDCONTENTS = (1 << 9)

IGR_ANNOTATION_BORDER_STYLE_UNKNOWN             = 0
IGR_ANNOTATION_BORDER_STYLE_SOLID               = 1
IGR_ANNOTATION_BORDER_STYLE_DASHED              = 2
IGR_ANNOTATION_BORDER_STYLE_BEVELED             = 3
IGR_ANNOTATION_BORDER_STYLE_INSET               = 4
IGR_ANNOTATION_BORDER_STYLE_UNDERLINE           = 5

IGR_ANNOTATION_LINE_ENDING_NONE                 = 0
IGR_ANNOTATION_LINE_ENDING_SQUARE               = 1
IGR_ANNOTATION_LINE_ENDING_CIRCLE               = 2
IGR_ANNOTATION_LINE_ENDING_DIAMOND              = 3
IGR_ANNOTATION_LINE_ENDING_OPENARROW            = 4
IGR_ANNOTATION_LINE_ENDING_CLOSEDARROW          = 5
IGR_ANNOTATION_LINE_ENDING_BUTT                 = 6
IGR_ANNOTATION_LINE_ENDING_RIGHTOPENARROW       = 7
IGR_ANNOTATION_LINE_ENDING_RIGHTCLOSEDARROW     = 8
IGR_ANNOTATION_LINE_ENDING_SLASH                = 9

IGR_ANNOTATION_ACTION_UNKNOWN                   = 0
IGR_ANNOTATION_ACTION_GOTO                      = 1
IGR_ANNOTATION_ACTION_GOTO_REMOTE               = 2
IGR_ANNOTATION_ACTION_URI                       = 3
IGR_ANNOTATION_ACTION_NAMED                     = 4

IGR_ANNOTATION_ALIGN_LEFT                       = 0
IGR_ANNOTATION_ALIGN_CENTER                     = 1
IGR_ANNOTATION_ALIGN_RIGHT                      = 2

IGR_ANNOTATION_HIGHLIGHT_UNKNOWN                = 0
IGR_ANNOTATION_HIGHLIGHT_INVERT                 = 1
IGR_ANNOTATION_HIGHLIGHT_OUTLINE                = 2
IGR_ANNOTATION_HIGHLIGHT_PUSH                   = 3

IGR_GET_PAGE_PIXELS_FLAGS_TYPE = IGR_LONG
IGR_GET_PAGE_PIXELS_FLAGS_BUFFER_ALLOCATED = 0x00010000


IGR_OPEN_BITMAP_PIXEL_1BPP_INDEXED = 1
IGR_OPEN_BITMAP_PIXEL_4BPP_INDEXED = 2
IGR_OPEN_BITMAP_PIXEL_8BPP_INDEXED = 3
IGR_OPEN_BITMAP_PIXEL_16BPP_565_RGB = 4
IGR_OPEN_BITMAP_PIXEL_16BPP_565_BGR = 5
IGR_OPEN_BITMAP_PIXEL_16BPP_4444_ARGB = 6
IGR_OPEN_BITMAP_PIXEL_16BPP_4444_BGRA = 7
IGR_OPEN_BITMAP_PIXEL_24BPP_888_RGB = 8
IGR_OPEN_BITMAP_PIXEL_24BPP_888_BGR = 9
IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ARGB = 10
IGR_OPEN_BITMAP_PIXEL_32BPP_8888_BGRA = 11
IGR_OPEN_BITMAP_PIXEL_AUTO = 0

IGR_OPEN_BITMAP_FLAGS_MIN_IS_BLACK = 0x01
IGR_OPEN_BITMAP_FLAGS_BOTTOM_UP = 0x02

IGR_OPEN_FROM = IGR_LONG
IGR_OPEN_FROM_FILENAME_UTF16 = 0
IGR_OPEN_FROM_STREAM = 1

IGR_OPEN_CALLBACK_ACTION = IGR_LONG
IGR_OPEN_CALLBACK_ACTION_HEARTBEAT = 0
IGR_OPEN_CALLBACK_ACTION_PASSWORD = 1
IGR_OPEN_CALLBACK_ACTION_LOCALIZE = 2

def UNCHECKED(type):
    if hasattr(type, "_type_") and isinstance(type._type_, str) and type._type_ != "P":
        return type
    else:
        return ctypes.c_void_p

def IGRFUNCTYPE(*arg):
    if os.name == 'nt':
        return ctypes.WINFUNCTYPE(*arg)
    else:
        return ctypes.CFUNCTYPE(*arg)

class Error_Control_Block(ctypes.Structure):
    _fields_ = [
        ('Msg', ctypes.c_char * int(238)),
        ('MsgId', IGR_SHORT),
    ]

class Instance_Status_Block(ctypes.Structure):
    _fields_ = [
        ('DLL_Version', ctypes.c_char * int(16)),
        ('RReserved', IGR_LONG),
        ('Licensee_ID1', ctypes.c_char * int(40)),
        ('Licensee_ID2', ctypes.c_char * int(40)),
    ]

IGR_Stream_Open = IGRFUNCTYPE(UNCHECKED(IGR_ULONG), ctypes.POINTER(None))
IGR_Stream_Read = IGRFUNCTYPE(UNCHECKED(IGR_ULONG), ctypes.POINTER(None), ctypes.POINTER(None), IGR_ULONG)
IGR_Stream_Seek = IGRFUNCTYPE(UNCHECKED(IGR_LONGLONG), ctypes.POINTER(None), IGR_LONGLONG, IGR_ULONG)
IGR_Stream_Close = IGRFUNCTYPE(UNCHECKED(None), ctypes.POINTER(None))
IGR_Stream_Write = IGRFUNCTYPE(UNCHECKED(IGR_ULONG), ctypes.POINTER(None), ctypes.POINTER(None), IGR_ULONG)
IGR_Stream_ActionCallback = IGRFUNCTYPE(UNCHECKED(ctypes.c_int), ctypes.c_int, ctypes.POINTER(None), ctypes.POINTER(None))

IGR_Make_Stream_Read = ctypes.CFUNCTYPE(UNCHECKED(IGR_ULONG), ctypes.POINTER(None), ctypes.POINTER(None), IGR_ULONG)
IGR_Make_Stream_Seek = ctypes.CFUNCTYPE(UNCHECKED(IGR_LONGLONG), ctypes.POINTER(None), IGR_LONGLONG, IGR_ULONG)
IGR_Make_Stream_Write = ctypes.CFUNCTYPE(UNCHECKED(IGR_ULONG), ctypes.POINTER(None), ctypes.POINTER(None), IGR_ULONG)
IGR_Make_Stream_Action = ctypes.CFUNCTYPE(UNCHECKED(IGR_LONG), ctypes.POINTER(None), IGR_LONG, ctypes.POINTER(None))
IGR_Make_Stream_Destruct = ctypes.CFUNCTYPE(UNCHECKED(None), ctypes.POINTER(None))

if sys.byteorder == 'big':
    IGR_Make_Stream_Seek = ctypes.CFUNCTYPE(UNCHECKED(IGR_ULONG), ctypes.POINTER(None), IGR_LONGLONG, IGR_ULONG)

class IGR_Stream(ctypes.Structure):
    _fields_ = [
        ('Open', IGR_Stream_Open),
        ('Read', IGR_Stream_Read),
        ('Seek', IGR_Stream_Seek),
        ('Close', IGR_Stream_Close),
        ('Write', IGR_Stream_Write),
        ('Context', IGR_ULONG),
    ]

IGR_Writable_Stream = IGR_Stream

class IGR_Rect(ctypes.Structure):
    _fields_ = [
        ('left', IGR_ULONG),
        ('top', IGR_ULONG),
        ('right', IGR_ULONG),
        ('bottom', IGR_ULONG),
    ]

class IGR_Size(ctypes.Structure):
    _fields_ = [
        ('width', IGR_ULONG),
        ('height', IGR_ULONG),
    ]

class IGR_Page_Word(ctypes.Structure):
    _fields_ = [
        ('x', ctypes.c_int),
        ('y', ctypes.c_int),
        ('width', ctypes.c_int),
        ('height', ctypes.c_int),
        ('style', ctypes.c_int),
        ('word', ctypes.POINTER(IGR_UCS2)),
        ('wordLength', ctypes.c_int),
        ('charoffset', ctypes.c_int),
        ('reserved', ctypes.c_int * int(8)),
    ]

class IGR_Bookmark(ctypes.Structure):
    _fields_ = [
        ('reserved', ctypes.POINTER(None)),
        ('flags', IGR_LONG),
        ('title', IGR_UCS2 * 255),
        ('action', IGR_LONG),
        ('dest', IGR_UCS2 * 1024),
        ('level', IGR_LONG),
        ('fit', IGR_LONG),
        ('zoom', IGR_LONG),
        ('x', IGR_LONG),
        ('y', IGR_LONG),
        ('width', IGR_LONG),
        ('height', IGR_LONG),
        ('color', IGR_LONG),
        ('text_style', IGR_LONG),
        ('reserved2', IGR_LONG * 8),
    ]

class IGR_T_ACTION_GET_STREAM_PART(ctypes.Structure):
    _fields_ = [
        ('size', ctypes.c_uint),
        ('partName', ctypes.c_wchar_p),
        ('partFullName', ctypes.c_wchar_p),
        ('partIndex', ctypes.c_int),
        ('istr', ctypes.POINTER(IGR_Stream)),
    ]

class struct_IGR_Page_Form_Element(ctypes.Structure):
    pass

class IGR_Page_Form_Element(ctypes.Structure):
    _fields_ = [
        ('reserved', ctypes.POINTER(None)),
        ('name', IGR_UCS2 * int(255)),
        ('x', IGR_LONG),
        ('y', IGR_LONG),
        ('width', IGR_LONG),
        ('height', IGR_LONG),
        ('type', IGR_LONG),
        ('flags', IGR_LONG),
        ('fontSize', IGR_LONG),
        ('fontStyle', IGR_LONG),
        ('rotation', IGR_LONG),
        ('textAlignment', IGR_LONG),
        ('selectedItem', IGR_LONG),
        ('option_count', IGR_LONG),
        ('reserved2', IGR_LONG * int(8)),
    ]

class IGR_Hyperlink(ctypes.Structure):
    _fields_ = [
        ('reserved', ctypes.POINTER(None)),
        ('type', IGR_LONG),
        ('x', IGR_LONG),
        ('y', IGR_LONG),
        ('width', IGR_LONG),
        ('height', IGR_LONG),
        ('page_number', IGR_LONG),
        ('dest_fit', IGR_LONG),
        ('dest_zoom', IGR_LONG),
        ('dest_left', IGR_LONG),
        ('dest_top', IGR_LONG),
        ('dest_right', IGR_LONG),
        ('dest_bottom', IGR_LONG),
        ('flags', IGR_LONG),
        ('reserved2', IGR_LONG * int(8)),
    ]

class IGR_Annotation(ctypes.Structure):
    _fields_ = [
        ('reserved', ctypes.POINTER(None)),
        ('type', IGR_LONG),
        ('flags', IGR_LONG),
        ('x', IGR_LONG),
        ('y', IGR_LONG),
        ('width', IGR_LONG),
        ('height', IGR_LONG),
        ('reserved2', IGR_LONG * int(8)),
    ]

class IGR_Render_Page_Form_Values(ctypes.Structure):
    _fields_ = [
        ('name', ctypes.POINTER(IGR_UCS2)),
        ('value', ctypes.POINTER(IGR_UCS2)),
        ('selected', IGR_LONG),
    ]

class IGR_Render_Page_Redactions(ctypes.Structure):
    _fields_ = [
        ('x', IGR_LONG),
        ('y', IGR_LONG),
        ('width', IGR_LONG),
        ('height', IGR_LONG),
        ('color', IGR_LONG),
        ('reserved', IGR_LONG),
    ]

class IGR_Render_Page_Properties(ctypes.Structure):
    _fields_ = [
        ('struct_size', IGR_ULONG),
        ('form_value_count', IGR_ULONG),
        ('form_values', ctypes.POINTER(IGR_Render_Page_Form_Values)),
        ('redaction_flags', IGR_LONG),
        ('redaction_count', IGR_LONG),
        ('redactions', ctypes.POINTER(IGR_Render_Page_Redactions)),
        ('source_rect', IGR_Rect),
        ('dest_rect', IGR_Rect),
    ]

class IGR_Page_Pixels(ctypes.Structure):
    _fields_ = [
        ('width', IGR_ULONG),
        ('height', IGR_ULONG),
        ('stride', IGR_ULONG),
        ('pixel_format', IGR_LONG),
        ('flags', IGR_LONG),
        ('scanline0', ctypes.POINTER(None)),
        ('palette_count', IGR_ULONG),
        ('palette', IGR_ULONG * 256),
        ('reserved', ctypes.POINTER(None)),
    ]

IGR_CALLBACK = ctypes.CFUNCTYPE(UNCHECKED(IGR_LONG), ctypes.c_int, ctypes.POINTER(None), ctypes.POINTER(None))

class IGR_Open_Callback_Action_Heartbeat(ctypes.Structure):
    _fields_ = [
        ('struct_size', IGR_ULONG),
    ]

class IGR_Open_Callback_Action_Password(ctypes.Structure):
    _fields_ = [
        ('struct_size', IGR_ULONG),
        ('id', IGR_UCS2 * 4096),
        ('password', IGR_UCS2 * 4096),
    ]

class IGR_Open_Callback_Action_Localize(ctypes.Structure):
    _fields_ = [
        ('struct_size', IGR_ULONG),
        ('original', IGR_UCS2 * 1024),
        ('replacement', IGR_UCS2 * 1024),
    ]

IGR_OPEN_CALLBACK = ctypes.CFUNCTYPE(UNCHECKED(IGR_LONG), IGR_OPEN_CALLBACK_ACTION, ctypes.POINTER(None), ctypes.POINTER(None))

class ISYS11dfAPI:
    @staticmethod
    def windll_name(name, dll_path = None):
        if dll_path == None:
            return name
        else:
            return os.path.join(dll_path, name + ".dll")

    @staticmethod
    def windll_load(name, dll_path = None):

        if not isinstance(dll_path, list) or dll_path == None:
            dll_path = [dll_path]

        LOAD_LIBRARY_SEARCH_APPLICATION_DIR = 0x00000200
        LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = 0x00001000
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR = 0x00000100
        LOAD_WITH_ALTERED_SEARCH_PATH = 0x00000008

        for path in dll_path:
            try:
                if path is not None:
                    try:
                        os.add_dll_directory(path)
                    except AttributeError:
                        # Python 3.7 does not support add_dll_directory. We side
                        # load the DLLs so the load library below can succeed.
                        ctypes.windll.kernel32.SetDllDirectoryW(path)
                        ctypes.windll.kernel32.LoadLibraryExW(ISYS11dfAPI.windll_name(name, path), 
                            0, 
                            LOAD_LIBRARY_SEARCH_DEFAULT_DIRS or LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)

                return ctypes.windll.LoadLibrary(ISYS11dfAPI.windll_name(name, path))
            except FileNotFoundError:
                pass
            except ImportError:
                pass
        raise ImportError("%s not found." % name)

    @staticmethod
    def unixdll_name(name):
        if sys.platform == 'darwin':
            return "lib" + name + ".dylib"
        return "lib" + name + ".so"

    @staticmethod
    def unixdll_load(name, dll_path = None):
        return ctypes.cdll.LoadLibrary(ISYS11dfAPI.unixdll_name("ISYS11df"))

    def __init__(self, dll_path = None):
        if os.name == 'nt':
            self._libISYS11df = ISYS11dfAPI.windll_load("ISYS11df", dll_path)
        else:
            self._libISYS11df = ISYS11dfAPI.unixdll_load("ISYS11df")

        self.Init_Instance = self._libISYS11df.Init_Instance
        self.Init_Instance.argtypes = [IGR_LONG, ctypes.c_char_p, ctypes.POINTER(Instance_Status_Block), ctypes.POINTER(IGR_SHORT), ctypes.POINTER(Error_Control_Block)]
        self.Init_Instance.restype = None

        self.Close_Instance = self._libISYS11df.Close_Instance
        self.Close_Instance.argtypes = [ctypes.POINTER(Error_Control_Block)]
        self.Close_Instance.restype = None

        self.Switch_Instance = self._libISYS11df.Switch_Instance
        self.Switch_Instance.argtypes = [IGR_SHORT, ctypes.POINTER(Error_Control_Block)]
        self.Switch_Instance.restype = None

        self.Widechar_to_UTF8 = self._libISYS11df.Widechar_to_UTF8
        self.Widechar_to_UTF8.argtypes = [ctypes.POINTER(IGR_UCS2), ctypes.c_char_p, IGR_LONG]
        self.Widechar_to_UTF8.restype = None

        self.Widechar_to_UTF8_Ex = self._libISYS11df.Widechar_to_UTF8_Ex
        self.Widechar_to_UTF8_Ex.argtypes = [ctypes.POINTER(IGR_UCS2), IGR_LONG, ctypes.c_char_p, IGR_LONG]
        self.Widechar_to_UTF8_Ex.restype = None

        self.UTF8_to_Widechar = self._libISYS11df.UTF8_to_Widechar
        self.UTF8_to_Widechar.argtypes = [ctypes.c_char_p, ctypes.POINTER(IGR_UCS2), IGR_LONG]
        self.UTF8_to_Widechar.restype = None

        self.UTF8_to_Widechar_Ex = self._libISYS11df.UTF8_to_Widechar_Ex
        self.UTF8_to_Widechar_Ex.argtypes = [ctypes.c_char_p, IGR_LONG, ctypes.POINTER(IGR_UCS2), IGR_LONG]
        self.UTF8_to_Widechar_Ex.restype = None

        self.IGR_Get_File_Type = self._libISYS11df.IGR_Get_File_Type
        self.IGR_Get_File_Type.argtypes = [ctypes.POINTER(IGR_UCS2), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_File_Type.restype = IGR_RETURN_CODE

        self.IGR_Get_Stream_Type = self._libISYS11df.IGR_Get_Stream_Type
        self.IGR_Get_Stream_Type.argtypes = [ctypes.POINTER(IGR_Stream), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Stream_Type.restype = IGR_RETURN_CODE

        self.IGR_Open_File = self._libISYS11df.IGR_Open_File
        self.IGR_Open_File.argtypes = [ctypes.POINTER(IGR_UCS2), IGR_LONG, ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Open_File.restype = IGR_RETURN_CODE

        self.IGR_Open_Stream = self._libISYS11df.IGR_Open_Stream
        self.IGR_Open_Stream.argtypes = [ctypes.POINTER(IGR_Stream), IGR_LONG, ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Open_Stream.restype = IGR_RETURN_CODE

        self.IGR_Open_File_Ex = self._libISYS11df.IGR_Open_File_Ex
        self.IGR_Open_File_Ex.argtypes = [ctypes.POINTER(None), IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Open_File_Ex.restype = IGR_RETURN_CODE

        self.IGR_Open_Stream_Ex = self._libISYS11df.IGR_Open_Stream_Ex
        self.IGR_Open_Stream_Ex.argtypes = [ctypes.POINTER(None), IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Open_Stream_Ex.restype = IGR_RETURN_CODE

        self.IGR_Get_Text = self._libISYS11df.IGR_Get_Text
        self.IGR_Get_Text.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Text.restype = IGR_RETURN_CODE

        self.IGR_Get_Subfile_Entry = self._libISYS11df.IGR_Get_Subfile_Entry
        self.IGR_Get_Subfile_Entry.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(IGR_LONGLONG), ctypes.POINTER(IGR_LONGLONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Subfile_Entry.restype = IGR_RETURN_CODE

        self.IGR_Extract_Subfile = self._libISYS11df.IGR_Extract_Subfile
        self.IGR_Extract_Subfile.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Extract_Subfile.restype = IGR_RETURN_CODE

        self.IGR_Extract_Subfile_Stream = self._libISYS11df.IGR_Extract_Subfile_Stream
        self.IGR_Extract_Subfile_Stream.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(ctypes.POINTER(IGR_Stream)), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Extract_Subfile_Stream.restype = IGR_RETURN_CODE

        self.IGR_Close_File = self._libISYS11df.IGR_Close_File
        self.IGR_Close_File.argtypes = [IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Close_File.restype = IGR_RETURN_CODE

        self.IGR_Multiplex = self._libISYS11df.IGR_Multiplex
        self.IGR_Multiplex.argtypes = [IGR_LONG, ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Multiplex.restype = IGR_RETURN_CODE

        self.IGR_Convert_File = self._libISYS11df.IGR_Convert_File
        self.IGR_Convert_File.argtypes = [ctypes.POINTER(None), IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Convert_File.restype = IGR_RETURN_CODE

        self.IGR_Make_Stream_From_File = self._libISYS11df.IGR_Make_Stream_From_File
        self.IGR_Make_Stream_From_File.argtypes = [ctypes.POINTER(None), IGR_LONG, ctypes.POINTER(ctypes.POINTER(IGR_Stream)), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Make_Stream_From_File.restype = IGR_RETURN_CODE

        self.IGR_Make_Stream_From_Memory = self._libISYS11df.IGR_Make_Stream_From_Memory
        self.IGR_Make_Stream_From_Memory.argtypes = [ctypes.POINTER(None), ctypes.c_size_t, ctypes.POINTER(None), ctypes.POINTER(ctypes.POINTER(IGR_Stream)), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Make_Stream_From_Memory.restype = IGR_RETURN_CODE

        try:
            self.IGR_Make_Stream_From_Functions = self._libISYS11df.IGR_Make_Stream_From_Functions
            self.IGR_Make_Stream_From_Functions.argtypes = [ctypes.POINTER(None), IGR_ULONG, IGR_Make_Stream_Seek, IGR_Make_Stream_Read, IGR_Make_Stream_Write, IGR_Make_Stream_Action, IGR_Make_Stream_Destruct, ctypes.POINTER(ctypes.POINTER(IGR_Stream)), ctypes.POINTER(Error_Control_Block)]
            self.IGR_Make_Stream_From_Functions.restype = IGR_RETURN_CODE
        except AttributeError:
            self.IGR_Make_Stream_From_Functions = None

        self.IGR_Extend_Stream = self._libISYS11df.IGR_Extend_Stream
        self.IGR_Extend_Stream.argtypes = [ctypes.POINTER(None), IGR_CALLBACK, ctypes.POINTER(None), ctypes.POINTER(ctypes.POINTER(IGR_Stream)), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Extend_Stream.restype = IGR_RETURN_CODE

        self.IGR_Get_Memory_Status = self._libISYS11df.IGR_Get_Memory_Status
        self.IGR_Get_Memory_Status.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        self.IGR_Get_Memory_Status.restype = ctypes.c_size_t

        self.IGR_Get_Image_Entry = self._libISYS11df.IGR_Get_Image_Entry
        self.IGR_Get_Image_Entry.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(IGR_LONGLONG), ctypes.POINTER(IGR_LONGLONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Image_Entry.restype = IGR_RETURN_CODE

        self.IGR_Extract_Image = self._libISYS11df.IGR_Extract_Image
        self.IGR_Extract_Image.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Extract_Image.restype = IGR_RETURN_CODE

        self.IGR_Extract_Image_Stream = self._libISYS11df.IGR_Extract_Image_Stream
        self.IGR_Extract_Image_Stream.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(ctypes.POINTER(IGR_Stream)), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Extract_Image_Stream.restype = IGR_RETURN_CODE

        self.IGR_Calculate_MD5 = self._libISYS11df.IGR_Calculate_MD5
        self.IGR_Calculate_MD5.argtypes = [ctypes.POINTER(IGR_Stream), ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Calculate_MD5.restype = IGR_RETURN_CODE

        self.IGR_Calculate_SHA1 = self._libISYS11df.IGR_Calculate_SHA1
        self.IGR_Calculate_SHA1.argtypes = [ctypes.POINTER(IGR_Stream), ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Calculate_SHA1.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Count = self._libISYS11df.IGR_Get_Page_Count
        self.IGR_Get_Page_Count.argtypes = [IGR_LONG, ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Count.restype = IGR_RETURN_CODE

        self.IGR_Open_Page = self._libISYS11df.IGR_Open_Page
        self.IGR_Open_Page.argtypes = [IGR_LONG, IGR_LONG, ctypes.POINTER(HPAGE), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Open_Page.restype = IGR_RETURN_CODE

        self.IGR_Close_Page = self._libISYS11df.IGR_Close_Page
        self.IGR_Close_Page.argtypes = [HPAGE, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Close_Page.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Word_Count = self._libISYS11df.IGR_Get_Page_Word_Count
        self.IGR_Get_Page_Word_Count.argtypes = [HPAGE, ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Word_Count.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Words = self._libISYS11df.IGR_Get_Page_Words
        self.IGR_Get_Page_Words.argtypes = [HPAGE, IGR_LONG, ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_Page_Word), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Words.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Dimensions = self._libISYS11df.IGR_Get_Page_Dimensions
        self.IGR_Get_Page_Dimensions.argtypes = [HPAGE, ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Dimensions.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Text = self._libISYS11df.IGR_Get_Page_Text
        self.IGR_Get_Page_Text.argtypes = [HPAGE, ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Text.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Image_Entry = self._libISYS11df.IGR_Get_Page_Image_Entry
        self.IGR_Get_Page_Image_Entry.argtypes = [HPAGE, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(IGR_LONGLONG), ctypes.POINTER(IGR_LONGLONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Image_Entry.restype = IGR_RETURN_CODE

        self.IGR_Extract_Page_Image = self._libISYS11df.IGR_Extract_Page_Image
        self.IGR_Extract_Page_Image.argtypes = [HPAGE, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Extract_Page_Image.restype = IGR_RETURN_CODE

        self.IGR_Extract_Page_Image_Stream = self._libISYS11df.IGR_Extract_Page_Image_Stream
        self.IGR_Extract_Page_Image_Stream.argtypes = [HPAGE, ctypes.POINTER(None), ctypes.POINTER(ctypes.POINTER(IGR_Stream)), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Extract_Page_Image_Stream.restype = IGR_RETURN_CODE

        self.IGR_Get_Bookmark_Root = self._libISYS11df.IGR_Get_Bookmark_Root
        self.IGR_Get_Bookmark_Root.argtypes = [IGR_LONG, ctypes.POINTER(IGR_Bookmark), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Bookmark_Root.restype = IGR_RETURN_CODE

        self.IGR_Get_Bookmark_First_Child = self._libISYS11df.IGR_Get_Bookmark_First_Child
        self.IGR_Get_Bookmark_First_Child.argtypes = [IGR_LONG, ctypes.POINTER(IGR_Bookmark), ctypes.POINTER(IGR_Bookmark), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Bookmark_First_Child.restype = IGR_RETURN_CODE

        self.IGR_Get_Bookmark_Next_Sibling = self._libISYS11df.IGR_Get_Bookmark_Next_Sibling
        self.IGR_Get_Bookmark_Next_Sibling.argtypes = [IGR_LONG, ctypes.POINTER(IGR_Bookmark), ctypes.POINTER(IGR_Bookmark), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Bookmark_Next_Sibling.restype = IGR_RETURN_CODE

        self.IGR_Make_Output_Canvas = self._libISYS11df.IGR_Make_Output_Canvas
        self.IGR_Make_Output_Canvas.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(HCANVAS), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Make_Output_Canvas.restype = IGR_RETURN_CODE

        self.IGR_Make_Output_Canvas_On = self._libISYS11df.IGR_Make_Output_Canvas_On
        self.IGR_Make_Output_Canvas_On.argtypes = [IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(HCANVAS), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Make_Output_Canvas_On.restype = IGR_RETURN_CODE

        self.IGR_Close_Canvas = self._libISYS11df.IGR_Close_Canvas
        self.IGR_Close_Canvas.argtypes = [HCANVAS, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Close_Canvas.restype = IGR_RETURN_CODE

        self.IGR_Render_Page = self._libISYS11df.IGR_Render_Page
        self.IGR_Render_Page.argtypes = [HPAGE, HCANVAS, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Render_Page.restype = IGR_RETURN_CODE

        self.IGR_Render_Page_Ex = self._libISYS11df.IGR_Render_Page_Ex
        self.IGR_Render_Page_Ex.argtypes = [HPAGE, HCANVAS, ctypes.POINTER(None), ctypes.POINTER(IGR_Render_Page_Properties), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Render_Page_Ex.restype = IGR_RETURN_CODE

        self.IGR_Redact_Page_Text = self._libISYS11df.IGR_Redact_Page_Text
        self.IGR_Redact_Page_Text.argtypes = [HPAGE, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Redact_Page_Text.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Attribute = self._libISYS11df.IGR_Get_Page_Attribute
        self.IGR_Get_Page_Attribute.argtypes = [HPAGE, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Attribute.restype = IGR_RETURN_CODE

        self.IGR_Get_Format_Attribute = self._libISYS11df.IGR_Get_Format_Attribute
        self.IGR_Get_Format_Attribute.argtypes = [IGR_LONG, IGR_LONG, ctypes.c_char_p, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Format_Attribute.restype = IGR_RETURN_CODE

        self.IGR_Get_Option_Attribute = self._libISYS11df.IGR_Get_Option_Attribute
        self.IGR_Get_Option_Attribute.argtypes = [IGR_LONG, IGR_LONG, ctypes.c_char_p, ctypes.c_size_t, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Option_Attribute.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Arc = self._libISYS11df.IGR_Canvas_Arc
        self.IGR_Canvas_Arc.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Arc.restype = IGR_RETURN_CODE

        self.IGR_Canvas_AngleArc = self._libISYS11df.IGR_Canvas_AngleArc
        self.IGR_Canvas_AngleArc.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_AngleArc.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Chord = self._libISYS11df.IGR_Canvas_Chord
        self.IGR_Canvas_Chord.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Chord.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Ellipse = self._libISYS11df.IGR_Canvas_Ellipse
        self.IGR_Canvas_Ellipse.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Ellipse.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Rect = self._libISYS11df.IGR_Canvas_Rect
        self.IGR_Canvas_Rect.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Rect.restype = IGR_RETURN_CODE

        self.IGR_Canvas_LineTo = self._libISYS11df.IGR_Canvas_LineTo
        self.IGR_Canvas_LineTo.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_LineTo.restype = IGR_RETURN_CODE

        self.IGR_Canvas_MoveTo = self._libISYS11df.IGR_Canvas_MoveTo
        self.IGR_Canvas_MoveTo.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_MoveTo.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Pie = self._libISYS11df.IGR_Canvas_Pie
        self.IGR_Canvas_Pie.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Pie.restype = IGR_RETURN_CODE

        self.IGR_Canvas_RoundRect = self._libISYS11df.IGR_Canvas_RoundRect
        self.IGR_Canvas_RoundRect.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_RoundRect.restype = IGR_RETURN_CODE

        self.IGR_Canvas_TextOut = self._libISYS11df.IGR_Canvas_TextOut
        self.IGR_Canvas_TextOut.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_TextOut.restype = IGR_RETURN_CODE

        self.IGR_Canvas_TextRect = self._libISYS11df.IGR_Canvas_TextRect
        self.IGR_Canvas_TextRect.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(None), IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_TextRect.restype = IGR_RETURN_CODE

        self.IGR_Canvas_MeasureText = self._libISYS11df.IGR_Canvas_MeasureText
        self.IGR_Canvas_MeasureText.argtypes = [HCANVAS, ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_MeasureText.restype = IGR_RETURN_CODE

        self.IGR_Canvas_SetPen = self._libISYS11df.IGR_Canvas_SetPen
        self.IGR_Canvas_SetPen.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_SetPen.restype = IGR_RETURN_CODE

        self.IGR_Canvas_SetBrush = self._libISYS11df.IGR_Canvas_SetBrush
        self.IGR_Canvas_SetBrush.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_SetBrush.restype = IGR_RETURN_CODE

        self.IGR_Canvas_SetFont = self._libISYS11df.IGR_Canvas_SetFont
        self.IGR_Canvas_SetFont.argtypes = [HCANVAS, ctypes.POINTER(None), IGR_LONG, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_SetFont.restype = IGR_RETURN_CODE

        self.IGR_Canvas_SetOpacity = self._libISYS11df.IGR_Canvas_SetOpacity
        self.IGR_Canvas_SetOpacity.argtypes = [HCANVAS, IGR_BYTE, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_SetOpacity.restype = IGR_RETURN_CODE

        self.IGR_Canvas_DrawImage = self._libISYS11df.IGR_Canvas_DrawImage
        self.IGR_Canvas_DrawImage.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, ctypes.POINTER(ctypes.c_char), ctypes.c_size_t, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_DrawImage.restype = IGR_RETURN_CODE

        self.IGR_Canvas_DrawScaleImage = self._libISYS11df.IGR_Canvas_DrawScaleImage
        self.IGR_Canvas_DrawScaleImage.argtypes = [HCANVAS, IGR_LONG, IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(ctypes.c_char), ctypes.c_size_t, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_DrawScaleImage.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Rotation = self._libISYS11df.IGR_Canvas_Rotation
        self.IGR_Canvas_Rotation.argtypes = [HCANVAS, IGR_LONG, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Rotation.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Reset = self._libISYS11df.IGR_Canvas_Reset
        self.IGR_Canvas_Reset.argtypes = [HCANVAS, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Reset.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Form_Element_Count = self._libISYS11df.IGR_Get_Page_Form_Element_Count
        self.IGR_Get_Page_Form_Element_Count.argtypes = [HPAGE, ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Form_Element_Count.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Form_Elements = self._libISYS11df.IGR_Get_Page_Form_Elements
        self.IGR_Get_Page_Form_Elements.argtypes = [HPAGE, IGR_LONG, ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_Page_Form_Element), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Form_Elements.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Form_Element_Str = self._libISYS11df.IGR_Get_Page_Form_Element_Str
        self.IGR_Get_Page_Form_Element_Str.argtypes = [ctypes.POINTER(IGR_Page_Form_Element), IGR_LONG, IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Form_Element_Str.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Form_Element_Option_Str = self._libISYS11df.IGR_Get_Page_Form_Element_Option_Str
        self.IGR_Get_Page_Form_Element_Option_Str.argtypes = [ctypes.POINTER(IGR_Page_Form_Element), IGR_LONG, IGR_LONG, IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Form_Element_Option_Str.restype = IGR_RETURN_CODE

        self.IGR_Get_Subfiles_Enumerator = self._libISYS11df.IGR_Get_Subfiles_Enumerator
        self.IGR_Get_Subfiles_Enumerator.argtypes = [IGR_LONG, ctypes.POINTER(HSUBFILES), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Subfiles_Enumerator.restype = IGR_RETURN_CODE

        self.IGR_Get_Images_Enumerator = self._libISYS11df.IGR_Get_Images_Enumerator
        self.IGR_Get_Images_Enumerator.argtypes = [IGR_LONG, ctypes.POINTER(HSUBFILES), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Images_Enumerator.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Images_Enumerator = self._libISYS11df.IGR_Get_Page_Images_Enumerator
        self.IGR_Get_Page_Images_Enumerator.argtypes = [IGR_LONG, ctypes.POINTER(HSUBFILES), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Images_Enumerator.restype = IGR_RETURN_CODE

        self.IGR_Subfiles_Next = self._libISYS11df.IGR_Subfiles_Next
        self.IGR_Subfiles_Next.argtypes = [HSUBFILES, ctypes.POINTER(None), ctypes.POINTER(None), ctypes.POINTER(IGR_LONGLONG), ctypes.POINTER(IGR_LONGLONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Subfiles_Next.restype = IGR_RETURN_CODE

        self.IGR_Subfiles_Reset = self._libISYS11df.IGR_Subfiles_Reset
        self.IGR_Subfiles_Reset.argtypes = [HSUBFILES, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Subfiles_Reset.restype = IGR_RETURN_CODE

        self.IGR_Subfiles_Close = self._libISYS11df.IGR_Subfiles_Close
        self.IGR_Subfiles_Close.argtypes = [HSUBFILES, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Subfiles_Close.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Blank_Page = self._libISYS11df.IGR_Canvas_Blank_Page
        self.IGR_Canvas_Blank_Page.argtypes = [HCANVAS, ctypes.POINTER(None), IGR_LONG, IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Blank_Page.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Annotate_JSON = self._libISYS11df.IGR_Canvas_Annotate_JSON
        self.IGR_Canvas_Annotate_JSON.argtypes = [HCANVAS, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Annotate_JSON.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Hyperlink_Count = self._libISYS11df.IGR_Get_Page_Hyperlink_Count
        self.IGR_Get_Page_Hyperlink_Count.argtypes = [HPAGE, ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Hyperlink_Count.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Hyperlinks = self._libISYS11df.IGR_Get_Page_Hyperlinks
        self.IGR_Get_Page_Hyperlinks.argtypes = [HPAGE, IGR_LONG, ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_Hyperlink), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Hyperlinks.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Hyperlink_Str = self._libISYS11df.IGR_Get_Page_Hyperlink_Str
        self.IGR_Get_Page_Hyperlink_Str.argtypes = [ctypes.POINTER(IGR_Hyperlink), IGR_LONG, IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Hyperlink_Str.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Annotation_Count = self._libISYS11df.IGR_Get_Page_Annotation_Count
        self.IGR_Get_Page_Annotation_Count.argtypes = [HPAGE, ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Annotation_Count.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Annotations = self._libISYS11df.IGR_Get_Page_Annotations
        self.IGR_Get_Page_Annotations.argtypes = [HPAGE, IGR_LONG, ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_Annotation), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Annotations.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Annotation_Str = self._libISYS11df.IGR_Get_Page_Annotation_Str
        self.IGR_Get_Page_Annotation_Str.argtypes = [ctypes.POINTER(IGR_Annotation), ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Annotation_Str.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Annotation_Long = self._libISYS11df.IGR_Get_Page_Annotation_Long
        self.IGR_Get_Page_Annotation_Long.argtypes = [HPAGE, ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Annotation_Long.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Bookmarks_Clear = self._libISYS11df.IGR_Canvas_Bookmarks_Clear
        self.IGR_Canvas_Bookmarks_Clear.argtypes = [HCANVAS, ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Bookmarks_Clear.restype = IGR_RETURN_CODE

        self.IGR_Canvas_Bookmarks_Append = self._libISYS11df.IGR_Canvas_Bookmarks_Append
        self.IGR_Canvas_Bookmarks_Append.argtypes = [HCANVAS, ctypes.POINTER(IGR_Bookmark), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Canvas_Bookmarks_Append.restype = IGR_RETURN_CODE

        self.IGR_Get_Page_Pixels = self._libISYS11df.IGR_Get_Page_Pixels
        self.IGR_Get_Page_Pixels.argtypes = [HPAGE, ctypes.POINTER(None), ctypes.POINTER(None), IGR_ULONG, ctypes.POINTER(None), IGR_ULONG, ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Get_Page_Pixels.restype = IGR_RETURN_CODE

        self.IGR_Free_Page_Pixels = self._libISYS11df.IGR_Free_Page_Pixels
        self.IGR_Free_Page_Pixels.argtypes = [ctypes.POINTER(None), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Free_Page_Pixels.restype = IGR_RETURN_CODE

        self.IGR_Open_Ex = self._libISYS11df.IGR_Open_Ex
        self.IGR_Free_Page_Pixels.argtypes = [IGR_OPEN_FROM, ctypes.POINTER(None), IGR_LONG, ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(IGR_LONG), ctypes.POINTER(None), IGR_OPEN_CALLBACK, ctypes.POINTER(None), ctypes.POINTER(IGR_LONG), ctypes.POINTER(Error_Control_Block)]
        self.IGR_Free_Page_Pixels.restype = IGR_RETURN_CODE

    __singleton = None

    @staticmethod
    def Singleton(dll_path = None):
        if ISYS11dfAPI.__singleton == None:
            ISYS11dfAPI.__singleton = ISYS11dfAPI(dll_path)
        return ISYS11dfAPI.__singleton