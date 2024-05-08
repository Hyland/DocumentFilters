/*
(c) 2020 Hyland Software, Inc. and its affiliates. All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/****************************************************************************
* Document Filters C API
****************************************************************************/

#ifndef PERCEPTIVE_DOCFILTERS_H_DEF
#define PERCEPTIVE_DOCFILTERS_H_DEF

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
# define DOCUMENT_FILTERS_PLATFORM_WINDOWS
# define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__linux__) && defined (__powerpc) && !defined(_LITTLE_ENDIAN)
# define DOCUMENT_FILTERS_PLATFORM_LINUX
# define DOCUMENT_FILTERS_PLATFORM_BIG_ENDIAN
#elif defined(__linux__)
# define DOCUMENT_FILTERS_PLATFORM_LINUX
# define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__FreeBSD__)
# define DOCUMENT_FILTERS_PLATFORM_FREEBSD
# define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__APPLE__)
# define DOCUMENT_FILTERS_PLATFORM_MACOSX
# define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__sun) && defined (__sparc)
# define DOCUMENT_FILTERS_PLATFORM_SOLARIS
# define DOCUMENT_FILTERS_PLATFORM_SOLARIS_SPARC
# define DOCUMENT_FILTERS_PLATFORM_BIG_ENDIAN
#elif defined(__sun)
# define DOCUMENT_FILTERS_PLATFORM_SOLARIS
# define DOCUMENT_FILTERS_PLATFORM_SOLARIS_INTEL
# define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__hpux)
# define DOCUMENT_FILTERS_PLATFORM_HPUX
# define DOCUMENT_FILTERS_PLATFORM_BIG_ENDIAN
#elif defined(_AIX)
# define DOCUMENT_FILTERS_PLATFORM_AIX
# define DOCUMENT_FILTERS_PLATFORM_BIG_ENDIAN
#elif defined(__wasm32__)
# define DOCUMENT_FILTERS_PLATFORM_WASM
# define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#else
# error "Document Filters: Unsupported platform"
#endif

#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32
	typedef wchar_t            IGR_UCS2;
	typedef long               IGR_LONG;
	typedef unsigned long      IGR_ULONG;
#else
	typedef unsigned short int IGR_UCS2;
	typedef int32_t            IGR_LONG;
	typedef uint32_t           IGR_ULONG;
#endif

	typedef unsigned char      IGR_BYTE;
	typedef short int          IGR_SHORT;
	typedef unsigned short int IGR_USHORT;
	typedef int64_t            IGR_LONGLONG;
	typedef uint64_t           IGR_ULONGLONG;
	typedef void* IGR_HANDLE;
	typedef float              IGR_FLOAT;
	typedef double             IGR_DOUBLE;

#if defined(DOCUMENT_FILTERS_PLATFORM_WINDOWS)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  define IGR_EXPORT  WINAPI
#  define IGR_HBITMAP HBITMAP
#  define IGR_HPALETTE HPALETTE
#else
#  include <stdint.h>

#  define IGR_EXPORT
#  define CALLBACK
#  define IGR_HBITMAP void*
#  define IGR_HPALETTE void*

#  ifndef WCHAR
#    define WCHAR IGR_UCS2
#  endif
#  ifndef BOOL
#    define BOOL int
#  endif
#  ifndef SHORT
#    define SHORT IGR_SHORT
#  endif
#  ifndef USHORT
#    define USHORT IGR_USHORT
#  endif
#  ifndef LPCWSTR
#    define LPCWSTR const WCHAR*
#  endif
#  ifndef LONG
#    define LONG IGR_LONG
#  endif
#  ifndef ULONG
#    define ULONG IGR_ULONG
#  endif
#  ifndef BYTE
#    define BYTE IGR_BYTE
#  endif
#  ifndef LONGLONG
#    define LONGLONG IGR_LONGLONG
#  endif
#  ifndef ULONGLONG
#    define ULONGLONG IGR_ULONGLONG
#  endif
#  ifndef PLONGLONG
#    define PLONGLONG *LONGLONG
#  endif
#  ifndef HANDLE
#    define HANDLE IGR_HANDLE
#  endif
#  ifndef HINSTANCE
#    define HINSTANCE IGR_HANDLE
#  endif
#  ifndef HMODULE
#    define HMODULE IGR_HANDLE
#  endif
#  ifndef HWND
#    define HWND IGR_HANDLE
#  endif
#  ifndef LPWSTR
#    define LPWSTR WCHAR*
#  endif
#  ifndef LPSTR
#    define LPSTR char*
#  endif
#  ifndef LPCSTR
#    define LPCSTR const LPSTR
#  endif
#endif

#ifndef IGR_EXPORT
#	define IGR_EXPORT
#endif

#define PERCEPTIVE_SZ_MD5_HEX_LEN  33
#define PERCEPTIVE_SZ_SHA1_HEX_LEN 41

#define IGR_Multi_Set_Code_Page         1
#define IGR_Multi_Set_Temp_Path         186

#define IGR_BODY_ONLY                   0
#define IGR_META_ONLY                   1
#define IGR_BODY_AND_META               2
#define IGR_FORMAT_TEXT                 0x00010000L
#define IGR_FORMAT_HTML                 0x00020000L
#define IGR_FORMAT_XML                  0x00030000L
#define IGR_FORMAT_HDHTML_IFYOUCAN      0x00040000L
#define IGR_FORMAT_IMAGE                0x00050000L
#define IGR_FORMAT_HTML_NO_FALLBACK     0x00060000L

#define IGR_FILE_SUPPORTS_TEXT          1
#define IGR_FILE_SUPPORTS_SUBFILES      2
#define IGR_FILE_SUPPORTS_HDHTML        4

	typedef IGR_LONG IGR_RETURN_CODE;
#define IGR_OK                          0
#define IGR_E_OPEN_ERROR                1
#define IGR_E_WRONG_TYPE                2
#define IGR_E_IN_USE                    3
#define IGR_E_NOT_READABLE              4
#define IGR_E_PASSWORD                  5
#define IGR_E_NOT_FOUND                 10
#define IGR_E_WRITE_ERROR               11
#define IGR_E_NOT_VALID_FOR_THIS_CLASS  12
#define IGR_E_ERROR                     13
#define IGR_E_INVALID_HANDLE            14
#define IGR_E_INVALID_POINTER           15
#define IGR_E_INVALID_PARAMETER         16
#define IGR_E_FILE_CORRUPT              17
#define IGR_NO_MORE                     19
#define IGR_E_OUT_OF_MEMORY             20
#define IGR_E_BAD_ERROR                 21
#define IGR_E_WRONG_THREAD              22
#define IGR_E_TOO_MANY_HANDLES          23
#define IGR_E_FONTS_NOT_FOUND           24
#define IGR_E_VERSION_MISMATCH          25
#define IGR_E_ACTION_NOT_SUPPORTED      26
#define IGR_CANCELLED                   27


#define IGR_CHAR_SOFT_SPACE             1
#define IGR_CHAR_STYLE_NORMAL           2
#define IGR_CHAR_STYLE_BOLD             3
#define IGR_CHAR_STYLE_ITALICS          4
#define IGR_CHAR_STYLE_UNDERLINED       5
#define IGR_CHAR_TAB                    9
#define IGR_CHAR_PAGE_BREAK             12
#define IGR_CHAR_LINE_BREAK             13
#define IGR_CHAR_PARA_BREAK             14
#define IGR_CHAR_START_META             15
#define IGR_CHAR_END_META               16

	/**
	*  Canvas output devices
	*/
#define IGR_DEVICE_IMAGE_PNG      0
#define IGR_DEVICE_IMAGE_JPG      1
#define IGR_DEVICE_IMAGE_PDF      2
#define IGR_DEVICE_IMAGE_TIF      3
#define IGR_DEVICE_IMAGE_BMP      4
#define IGR_DEVICE_IMAGE_PBM      7
#define IGR_DEVICE_IMAGE_PGM      8
#define IGR_DEVICE_IMAGE_PPM      9
#define IGR_DEVICE_IMAGE_WEBP     10
#define IGR_DEVICE_IMAGE_XPS      11
#define IGR_DEVICE_IMAGE_SVG      12
#define IGR_DEVICE_IMAGE_EPS      13
#define IGR_DEVICE_IMAGE_PS       14
#define IGR_DEVICE_IMAGE_WEBSAFE  15
#define IGR_DEVICE_IMAGE_TGA      16
#define IGR_DEVICE_IMAGE_BRK      17
#define IGR_DEVICE_IMAGE_PCX      18
#define IGR_DEVICE_IMAGE_DCX      19
#define IGR_DEVICE_IMAGE_JPEG2000 20
#define IGR_DEVICE_IMAGE_GIF      21
#define IGR_DEVICE_PDF            IGR_DEVICE_IMAGE_PDF
#define IGR_DEVICE_XML            5
#define IGR_DEVICE_HTML           6

	/**
	*  File type details 
	*/
#define IGR_FILETYPE_UNKNOWN              0
#define IGR_FILETYPE_TEXT                 100
#define IGR_FILETYPE_TEXT_MARKUP          101
#define IGR_FILETYPE_WORD_PROCESSOR       200
#define IGR_FILETYPE_SPREADSHEET          300
#define IGR_FILETYPE_PRESENTATION         400
#define IGR_FILETYPE_OFFICE_OTHER         500
#define IGR_FILETYPE_CAD                  600
#define IGR_FILETYPE_ARCHIVE              700
#define IGR_FILETYPE_SYSTEM               800
#define IGR_FILETYPE_DATABASE             900
#define IGR_FILETYPE_GRAPHIC              1000
#define IGR_FILETYPE_GRAPHIC_RASTER       1001
#define IGR_FILETYPE_GRAPHIC_VECTOR       1002
#define IGR_FILETYPE_EMAIL                1100
#define IGR_FILETYPE_EMAIL_MESSAGE        1101
#define IGR_FILETYPE_EMAIL_CONTAINER      1102
#define IGR_FILETYPE_EMAIL_CONTACT        1103
#define IGR_FILETYPE_EMAIL_CALENDAR       1104
#define IGR_FILETYPE_EMAIL_OTHER          1105
#define IGR_FILETYPE_MULTIMEDIA           1200
#define IGR_FILETYPE_MULTIMEDIA_VIDEO     1201
#define IGR_FILETYPE_MULTIMEDIA_AUDIO     1202
#define IGR_FILETYPE_PUBLISHING           1300
#define IGR_FILETYPE_APPDATA              2000

	/**
	*  Canvas drawing methods
	*/
#define IGR_TEXT_ALIGN_LEFT       0x00
#define IGR_TEXT_ALIGN_RIGHT      0x01
#define IGR_TEXT_ALIGN_CENTER     0x02
#define IGR_TEXT_ALIGN_TOP        0x10
#define IGR_TEXT_ALIGN_MIDDLE     0x11
#define IGR_TEXT_ALIGN_BOTTOM     0x12

	/**
	*  IGR_Canvas_SetFont Options
	*/
#define IGR_TEXT_STYLE_BOLD       0x01
#define IGR_TEXT_STYLE_ITALIC     0x02
#define IGR_TEXT_STYLE_UNDERLINE  0x04

	/**
	*  IGR_Canvas_SetBrush Options
	*/
#define IGR_BRUSH_NONE            0x00
#define IGR_BRUSH_SOLID           0x01

	/**
	*  IGR_Canvas_SetPen Options
	*/
#define IGR_PEN_NONE              0x00
#define IGR_PEN_SOLID             0x01

#define IGR_TEXTRECT_FLAG_CLIP    0x01
#define IGR_TEXTRECT_FLAG_STRETCHTOFIT 0x02

	/**
	*  IGR_CALLBACK Actions
	*/
#define IGR_ACTION_GET_STREAM_PART  10


#define MAKE_COLOR(R, G, B)       ((int) R | ((int) G << 8) | ((int) B << 16))

#define IGR_MAKE_COLOR_WITH_ALPHA(A, R, G, B) (((int) A << 24) | ((int) R << 16) | ((int) G << 8) | ((int) B << 0))

#define IGR_MAKE_COLOR(R, G, B) IGR_MAKE_COLOR_WITH_ALPHA(0xff, R, G, B)

	/**
	 * Structure representing an error control block.
	 * It includes properties such as a message and a message ID.
	 */
	typedef struct tagError_Control_Block
	{
	    char Msg[238];      ///< Error message.
	    IGR_SHORT MsgId;    ///< Error message ID.
	} Error_Control_Block;

	typedef Error_Control_Block* lpECB_Type;
	typedef Error_Control_Block ECB_Type;

	/**
	 * Structure representing an instance status block.
	 * It includes properties such as DLL version, reserved fields,
	 * license type, and licensee IDs.
	 */
	typedef struct tagInstance_Status_Block
	{
	    char DLL_Version[16];        ///< DLL version information.
	    IGR_SHORT RReserved;         ///< Reserved field.
	    IGR_SHORT License_Type;      ///< Type of license.
	    char Licensee_ID1[40];       ///< First part of the licensee ID.
	    char Licensee_ID2[40];       ///< Second part of the licensee ID.
	} Instance_Status_Block, * LPInstance_Status_Block;

	typedef struct IGR_Stream* PIGR_Stream;
#ifndef _IGR_Stream
	struct IGR_Stream
	{
		ULONG(CALLBACK* Open) (struct IGR_Stream* stream);
		ULONG(CALLBACK* Read) (struct IGR_Stream* stream, void* buffer, ULONG bufferSize);
		IGR_LONGLONG(CALLBACK* Seek) (struct IGR_Stream* stream, IGR_LONGLONG offset, ULONG whence);
		void (CALLBACK* Close) (struct IGR_Stream* stream);
	};
#endif

	typedef struct IGR_Writable_Stream* PIGR_Writable_Stream;
	struct IGR_Writable_Stream
	{
		struct IGR_Stream base;
		ULONG(CALLBACK* Write) (struct IGR_Writable_Stream* stream, void* buffer, ULONG bufferSize);
	};

	typedef IGR_LONG HPAGE;
	typedef IGR_LONG HCANVAS;
	typedef HPAGE IGR_HPAGE;
	typedef HCANVAS IGR_HCANVAS;

	typedef IGR_LONG IGR_HDOC;

	/**
	 * Structure representing a rectangle with unsigned integer coordinates.
	 * The coordinates are defined by the left, top, right, and bottom edges.
	 */
	struct IGR_Rect
	{
	    IGR_ULONG left;     ///< Left edge coordinate.
	    IGR_ULONG top;      ///< Top edge coordinate.
	    IGR_ULONG right;    ///< Right edge coordinate.
	    IGR_ULONG bottom;   ///< Bottom edge coordinate.
	};

	/**
	 * Structure representing a rectangle with signed integer coordinates.
	 * The coordinates are defined by the left, top, right, and bottom edges.
	 */
	struct IGR_SRect
	{
	    IGR_LONG left;      ///< Left edge coordinate.
	    IGR_LONG top;       ///< Top edge coordinate.
	    IGR_LONG right;     ///< Right edge coordinate.
	    IGR_LONG bottom;    ///< Bottom edge coordinate.
	};

	/**
	 * Structure representing a rectangle with floating-point coordinates.
	 * The coordinates are defined by the left, top, right, and bottom edges.
	 */
	struct IGR_FRect
	{
	    IGR_FLOAT left;     ///< Left edge coordinate.
	    IGR_FLOAT top;      ///< Top edge coordinate.
	    IGR_FLOAT right;    ///< Right edge coordinate.
	    IGR_FLOAT bottom;   ///< Bottom edge coordinate.
	};

	/**
	 * Structure representing a size with unsigned integer width and height.
	 */
	struct IGR_Size
	{
	    IGR_ULONG width;    ///< Width dimension.
	    IGR_ULONG height;   ///< Height dimension.
	};


	/**
	 * Structure containing positional information about each word on a page. Units are pixels.
	 */
	struct IGR_Page_Word
	{
	    IGR_LONG x;                 ///< X position of the word.
	    IGR_LONG y;                 ///< Y position of the word.
	    IGR_LONG width;             ///< Width of the word.
	    IGR_LONG height;            ///< Height of the word.
	    IGR_LONG style;             ///< Reserved for future use. Style information.
	    const IGR_UCS2* word;       ///< Pointer to a UCS2 string representing the word.
	    IGR_LONG wordLength;        ///< Length of the word.
	    IGR_LONG charoffset;        ///< Character offset of the word.
	    IGR_LONG reserved[8];       ///< Reserved data for future use.
	};


#define IGR_PAGE_FORM_ELEMENT_TYPE_BUTTON             0
#define IGR_PAGE_FORM_ELEMENT_TYPE_RADIOBUTTON        1
#define IGR_PAGE_FORM_ELEMENT_TYPE_CHECKBOX           2
#define IGR_PAGE_FORM_ELEMENT_TYPE_FILESELECT         3
#define IGR_PAGE_FORM_ELEMENT_TYPE_MULTILINE_TEXT     4
#define IGR_PAGE_FORM_ELEMENT_TYPE_SINGLELINE_TEXT    5
#define IGR_PAGE_FORM_ELEMENT_TYPE_COMBOBOX           6
#define IGR_PAGE_FORM_ELEMENT_TYPE_LISTBOX            7
#define IGR_PAGE_FORM_ELEMENT_TYPE_SIGNATURE          8
#define IGR_PAGE_FORM_ELEMENT_TYPE_PASSWORD           9

#define IGR_PAGE_FORM_ELEMENT_FLAG_READONLY          (1 << 0)
#define IGR_PAGE_FORM_ELEMENT_FLAG_REQUIRED          (1 << 1)
#define IGR_PAGE_FORM_ELEMENT_FLAG_COMB              (1 << 2)
#define IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED           (1 << 3)

#define IGR_PAGE_FORM_ELEMENT_GET_NAME               0
#define IGR_PAGE_FORM_ELEMENT_GET_VALUE              1
#define IGR_PAGE_FORM_ELEMENT_GET_CAPTION            2
#define IGR_PAGE_FORM_ELEMENT_GET_ACTION             3
#define IGR_PAGE_FORM_ELEMENT_GET_ACTION_DEST        4
#define IGR_PAGE_FORM_ELEMENT_GET_SELECTED           5
#define IGR_PAGE_FORM_ELEMENT_GET_FONT_NAME          6

	/**
	 * Structure representing a form element on a page.
	 * It includes properties such as reserved data, name, position, size,
	 * type, flags, font size, font style, rotation, text alignment,
	 * selected item, option count, and reserved data for future use.
	 */
	struct IGR_Page_Form_Element
	{
	    const void* reserved;       ///< Reserved data for future use.
	    IGR_UCS2 name[255];         ///< Element form element name.
	    IGR_LONG x;                 ///< X pixel position of the form element.
	    IGR_LONG y;                 ///< Y pixel position of the form element.
	    IGR_LONG width;             ///< Width in pixels of the form element.
	    IGR_LONG height;            ///< Height in pixels of the form element.
	    IGR_LONG type;              ///< Element type, one of IGR_PAGE_FORM_ELEMENT_TYPE_X.
	    IGR_LONG flags;             ///< Element flags, bit set of IGR_PAGE_FORM_ELEMENT_FLAG_X.
	    IGR_LONG fontSize;          ///< Font size of field text in points.
	    IGR_LONG fontStyle;         ///< Bit set of IGR_TEXT_STYLE_XYZ.
	    IGR_LONG rotation;          ///< Field rotation in degrees (0, 90, 180, 270).
	    IGR_LONG textAlignment;     ///< Text alignment inside field, one of IGR_TEXT_ALIGN_X.
	    IGR_LONG selectedItem;      ///< Selected item for combo boxes or single select list boxes.
	    IGR_LONG option_count;      ///< Number of items available in a combobox or list box.
	    IGR_LONG reserved2[8];      ///< Reserved data for future use.
	};

#define IGR_BOOKMARK_FIT_NONE    0
#define IGR_BOOKMARK_FIT_BOTH    1
#define IGR_BOOKMARK_FIT_WIDTH   2
#define IGR_BOOKMARK_FIT_HEIGHT  3
#define IGR_BOOKMARK_FIT_RECT    4

#define IGR_BOOKMARK_ACTION_GOTO 0
#define IGR_BOOKMARK_ACTION_LINK 1

	/**
	 * Structure representing a bookmark item.
	 * It includes properties such as reserved data, flags, title, action,
	 * destination, level, page fit method, zoom factor, position, size,
	 * color, text styles, and reserved data for future use.
	 */
	struct IGR_Bookmark
	{
	    const void* reserved;           ///< Reserved data for future use.
	    IGR_LONG flags;                 ///< Flags indicating specific characteristics.
	    IGR_UCS2 title[255];            ///< Display name for the bookmark.
	    IGR_LONG action;                ///< Action to occur when the bookmark is selected.
	    IGR_UCS2 dest[1024];            ///< Destination for the bookmark.
	    IGR_LONG level;                 ///< Depth/level of the bookmark in the hierarchy.
	    IGR_LONG fit;                   ///< Page fit method to use when navigating.
	    IGR_LONG zoom;                  ///< Zoom factor to apply when navigating.
	    IGR_LONG x;                     ///< X position in pixels of the bookmark destination.
	    IGR_LONG y;                     ///< Y position in pixels of the bookmark destination.
	    IGR_LONG width;                 ///< Width in pixels of the bookmark destination.
	    IGR_LONG height;                ///< Height in pixels of the bookmark destination.
	    IGR_LONG color;                 ///< Color used when displaying the bookmark item.
	    IGR_LONG text_style;            ///< Text styles used when displaying the bookmark item.
	    IGR_LONG reserved2[8];          ///< Reserved data for future use.
	};

	typedef IGR_LONG(IGR_EXPORT* IGR_CALLBACK_BOOKMARK)(struct IGR_Bookmark* element, void* context);

	typedef IGR_LONG(*IGR_CALLBACK)(int actionID, void* actionData, void* context);

	/**
	 * Structure passed to IGR_CALLBACK's actionData when actionID is IGR_ACTION_GET_STREAM_PART.
	 * It includes properties such as size, part name, part full name, part index, and output stream.
	 */
	struct IGR_T_ACTION_GET_STREAM_PART
	{
	    IGR_ULONG size;                    ///< Size of this structure.
	    const wchar_t* partName;           ///< Input: Part name (e.g., foo.ra01).
	    const wchar_t* partFullName;       ///< Input: Full part name (e.g., c:\foo.ra01).
	    IGR_LONG partIndex;                ///< Input: Part index.
	    struct IGR_Stream* istr;           ///< Output: Pointer to the output stream.
	};


	/**
	 * Structure representing form values for rendering a page.
	 * It includes properties such as the name, value, and selection state.
	 */
	struct IGR_Render_Page_Form_Values
	{
	    const IGR_UCS2* name;    ///< Pointer to the name of the form value.
	    const IGR_UCS2* value;   ///< Pointer to the value of the form value.
	    IGR_LONG selected;       ///< Selection state of the form value.
	};


#define IGR_COLOR_TYPE IGR_ULONG

	/**
	 * Structure representing a redaction area for rendering on a page.
	 * The redaction area is defined by its position (x, y) and size (width, height),
	 * along with the color to use for rendering and additional reserved data.
	 */
	struct IGR_Render_Page_Redactions
	{
	    IGR_LONG x;             ///< X-coordinate of the redaction area.
	    IGR_LONG y;             ///< Y-coordinate of the redaction area.
	    IGR_LONG width;         ///< Width of the redaction area.
	    IGR_LONG height;        ///< Height of the redaction area.
	    IGR_COLOR_TYPE color;   ///< Color type used for rendering the redaction.
	    IGR_ULONG reserved;     ///< Reserved data for future use.
	};

#define IGR_REDACT_FLAGS_TYPE						IGR_ULONG
#define IGR_REDACT_FLAGS_DO_NOT_REDACT_TEXT			0x1				// Do not redact text
#define IGR_REDACT_FLAGS_DO_NOT_REDACT_IMAGES		0x2				// Do not redact images
#define IGR_REDACT_FLAGS_ALWAYS_RASTER				0x4				// Page should always be rasterized and then redacted pixels replaced with the specified color. If this flag is set and redaction cannot be performed an error will occur.
#define IGR_REDACT_FLAGS_NEVER_RASTER				0x8				// Page should never be rasterized. If this flag is set and redaction cannot be performed an error will occur.
#define IGR_REDACT_FLAGS_DO_NOT_DRAW_REGION			0x10			// Do not draw redaction regions on the canvas, redacted images will still use the redaction regions and colors

	/**
	 * Structure passed to IGR_Render_Page_Ex function.
	 * It includes properties such as structure size, form values, redaction flags,
	 * redactions, source rectangle, and destination rectangle in pixels.
	 */
	struct IGR_Render_Page_Properties
	{
	    IGR_ULONG struct_size;                          ///< Size of this structure.
	    IGR_ULONG form_value_count;                     ///< Number of items in the form_values array.
	    const struct IGR_Render_Page_Form_Values* form_values; ///< Pointer to the first form value item.
	    IGR_REDACT_FLAGS_TYPE redaction_flags;          ///< Redaction flags.
	    IGR_ULONG redaction_count;                      ///< Number of items in the redaction array.
	    const struct IGR_Render_Page_Redactions* redactions; ///< Pointer to the first redaction item.
	    struct IGR_Rect source_rect;                    ///< Source rectangle in pixels.
	    struct IGR_Rect dest_rect;                      ///< Destination rectangle in pixels.
	};


#define IGR_HYPERLINK_ACTION_TYPE IGR_LONG
#define IGR_HYPERLINK_ACTION_GOTO 0x1 // 
#define IGR_HYPERLINK_ACTION_URI  0x2 // Destination is either a qualified or relative link. 

#define IGR_HYPERLINK_FLAGS_TYPE IGR_LONG
#define IGR_HYPERLINK_FLAGS_CHANGE_LEFT 0x1
#define IGR_HYPERLINK_FLAGS_CHANGE_TOP 0x2
#define IGR_HYPERLINK_FLAGS_CHANGE_ZOOM 0x4

#define IGR_HYPERLINK_FIT_TYPE IGR_LONG
#define IGR_HYPERLINK_FIT_XYZ 0
#define IGR_HYPERLINK_FIT_FIT 1
#define IGR_HYPERLINK_FIT_FITH 2
#define IGR_HYPERLINK_FIT_FITV 3
#define IGR_HYPERLINK_FIT_FITR 4
#define IGR_HYPERLINK_FIT_FITB 5
#define IGR_HYPERLINK_FIT_FITBH 6
#define IGR_HYPERLINK_FIT_FITBV 7

#define IGR_HYPERLINK_GET_VALUE_TYPE IGR_LONG
#define IGR_HYPERLINK_GET_VALUE_REF 0
#define IGR_HYPERLINK_GET_VALUE_URI 1

	/**
	 * Structure representing a hyperlink element.
	 * It includes details such as type of action, position, size,
	 * destination page number, zoom factor, destination position,
	 * flags, and reserved data for internal use.
	 */
	struct IGR_Hyperlink
	{
	    const void* reserved;               ///< Reserved data for internal use.
	    IGR_HYPERLINK_ACTION_TYPE type;     ///< Type of action to execute when the link is clicked.
	    IGR_LONG x;                         ///< X pixel position of the hyperlink element.
	    IGR_LONG y;                         ///< Y pixel position of the hyperlink element.
	    IGR_LONG width;                     ///< Width in pixels of the hyperlink element.
	    IGR_LONG height;                    ///< Height in pixels of the hyperlink element.
	    IGR_LONG page_number;               ///< Page number to show if type is GOTO.
	    IGR_HYPERLINK_FIT_TYPE dest_fit;    ///< Page fit to apply when navigating to the destination.
	    IGR_LONG dest_zoom;                 ///< Zoom factor to apply when navigating.
	    IGR_LONG dest_left;                 ///< Left position in pixels for the destination, fit value specific.
	    IGR_LONG dest_top;                  ///< Top position in pixels for the destination, fit value specific.
	    IGR_LONG dest_right;                ///< Right position in pixels for the destination, fit value specific.
	    IGR_LONG dest_bottom;               ///< Bottom position in pixels for the destination, fit value specific.
	    IGR_HYPERLINK_FLAGS_TYPE flags;     ///< Flags associated with the hyperlink.
	    IGR_LONG reserved2[8];              ///< Reserved data for internal use.
	};

#define IGR_OPEN_BITMAP_PIXEL_TYPE IGR_LONG
#define IGR_OPEN_BITMAP_PIXEL_1BPP_INDEXED    1
#define IGR_OPEN_BITMAP_PIXEL_4BPP_INDEXED    2
#define IGR_OPEN_BITMAP_PIXEL_8BPP_INDEXED    3
#define IGR_OPEN_BITMAP_PIXEL_16BPP_565_RGB   4
#define IGR_OPEN_BITMAP_PIXEL_16BPP_565_BGR   5
#define IGR_OPEN_BITMAP_PIXEL_16BPP_4444_ARGB 6
#define IGR_OPEN_BITMAP_PIXEL_16BPP_4444_BGRA 7
#define IGR_OPEN_BITMAP_PIXEL_24BPP_888_RGB   8
#define IGR_OPEN_BITMAP_PIXEL_24BPP_888_BGR   9
#define IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ARGB 10 
#define IGR_OPEN_BITMAP_PIXEL_32BPP_8888_BGRA 11
#define IGR_OPEN_BITMAP_PIXEL_32BPP_8888_RGBA 12
#define IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ABGR 13
#define IGR_OPEN_BITMAP_PIXEL_AUTO            0

#define IGR_OPEN_BITMAP_FLAGS_TYPE IGR_LONG
#define IGR_OPEN_BITMAP_FLAGS_MIN_IS_BLACK    0x01
#define IGR_OPEN_BITMAP_FLAGS_BOTTOM_UP       0x02


	/**
	 * Structure representing information about an open Device Independent Bitmap (DIB).
	 * It includes details such as size, flags, width, height, stride, pixel format,
	 * pixel data, palette information, and palette count.
	 */
	struct IGR_Open_DIB_Info
	{
	    IGR_ULONG struct_size;                  ///< Size of this structure, must be populated.
	    IGR_ULONG flags;                        ///< Flags indicating specific characteristics.
	    IGR_ULONG width;                        ///< Width of the bitmap, in pixels.
	    IGR_ULONG height;                       ///< Height of the bitmap, in pixels.
	    IGR_ULONG stride;                       ///< Number of bytes per scanline.
	    IGR_OPEN_BITMAP_PIXEL_TYPE pixel_format;///< Format of the pixel data.
	    const void* pixel_data;                 ///< Pointer to the raw pixel data.
	    IGR_ULONG pixel_data_size;              ///< Size of the buffer in pixel_data.
	    const IGR_LONG* palette;                ///< Pointer to the RGB palette information.
	    IGR_ULONG palette_count;                ///< Number of colors in the palette.
	};


#define IGR_GET_PAGE_PIXELS_FLAGS_TYPE IGR_LONG
#define IGR_GET_PAGE_PIXELS_FLAGS_BUFFER_ALLOCATED 0x00010000

	/**
	 * Structure representing extracted pixels from a page.
	 * It includes information such as width, height, stride, pixel format,
	 * pixel format flags, palette information, and reserved data.
	 */
	struct IGR_Page_Pixels
	{
	    IGR_ULONG width;                        ///< Number of pixels in one scan line of the extracted pixels.
	    IGR_ULONG height;                       ///< Number of scanlines extracted.
	    IGR_LONG stride;                        ///< Offset, in bytes, between consecutive scan lines of the page.
	                                            ///< If positive, scan lines are top-down; if negative, they are bottom-up.
	    IGR_OPEN_BITMAP_PIXEL_TYPE pixel_format;///< Pixel format of the extracted pixels.
	    IGR_OPEN_BITMAP_FLAGS_TYPE flags;       ///< Pixel format flags.
	    void* scanline0;                        ///< Pointer to the first scan line extracted.
	    IGR_ULONG palette_count;                ///< Number of palette colors extracted, if any.
	    IGR_LONG palette[256];                  ///< Palette colors extracted.
	    void* reserved;                         ///< Reserved for internal use.
	};


	/**
	 * Structure representing information about a subfile or sub-document.
	 * It includes details such as size, flags, identifiers, names, comments,
	 * size in bytes, and date and time of the subfile.
	 */
	struct IGR_Subfile_Info
	{
	    IGR_ULONG struct_size;      ///< Size of this structure, must be populated.
	    IGR_ULONG flags;            ///< Flags indicating specific characteristics.
	    IGR_UCS2* id;               ///< Buffer to be populated with the id.
	    IGR_ULONG id_size;          ///< Number of characters pointed to by id.
	    IGR_UCS2* name;             ///< Buffer to be populated with the name.
	    IGR_ULONG name_size;        ///< Number of characters pointed to by name.
	    IGR_UCS2* comment;          ///< Buffer to be populated with the comment.
	    IGR_ULONG comment_size;     ///< Number of characters pointed to by comment.
	    IGR_ULONGLONG size;         ///< Size in bytes of the sub-document.
	    IGR_ULONGLONG date;         ///< Date and time of the sub-document in FILETIME format.
	};

	struct IGR_Subfile_Enumerator;
	typedef struct IGR_Subfile_Enumerator* HSUBFILES;

#define IGR_ANNOTATION_TYPE IGR_LONG
#define IGR_ANNOTATION_UNKNOWN			0
#define IGR_ANNOTATION_TEXT				1
#define IGR_ANNOTATION_LINK				2
#define IGR_ANNOTATION_FREETEXT			3
#define IGR_ANNOTATION_LINE				4
#define IGR_ANNOTATION_SQUARE			5
#define IGR_ANNOTATION_CIRCLE			6
#define IGR_ANNOTATION_POLYGON			7
#define IGR_ANNOTATION_POLYLINE			8
#define IGR_ANNOTATION_HIGHLIGHT		9
#define IGR_ANNOTATION_UNDERLINE		10
#define IGR_ANNOTATION_SQUIGGLE			11
#define IGR_ANNOTATION_STRIKEOUT		12
#define IGR_ANNOTATION_STAMP			13
#define IGR_ANNOTATION_CARET			14
#define IGR_ANNOTATION_INK				15
#define IGR_ANNOTATION_POPUP			16
#define IGR_ANNOTATION_FILEATTACHMENT	17
#define IGR_ANNOTATION_SOUND			18
#define IGR_ANNOTATION_MOVIE			19
#define IGR_ANNOTATION_WIDGET			20
#define IGR_ANNOTATION_SCREEN			21
#define IGR_ANNOTATION_PRINTERMARK		22
#define IGR_ANNOTATION_TRAPNET			23
#define IGR_ANNOTATION_WATERMARK		24
#define IGR_ANNOTATION_3D				25
#define IGR_ANNOTATION_BARCODE			26
#define IGR_ANNOTATION_NAMED_DESTINATION 27


#define IGR_ANNOTATION_FLAGS_TYPE IGR_LONG
#define IGR_ANNOTATION_FLAGS_INVISIBLE			(1 << 0)
#define IGR_ANNOTATION_FLAGS_HIDDEN				(1 << 1)
#define IGR_ANNOTATION_FLAGS_PRINT				(1 << 2)
#define IGR_ANNOTATION_FLAGS_NOZOOM				(1 << 3)
#define IGR_ANNOTATION_FLAGS_NOROTATE			(1 << 4)
#define IGR_ANNOTATION_FLAGS_NOVIEW				(1 << 5)
#define IGR_ANNOTATION_FLAGS_READONLY			(1 << 6)
#define IGR_ANNOTATION_FLAGS_LOCKED				(1 << 7)
#define IGR_ANNOTATION_FLAGS_TOGGLENOVIEW		(1 << 8)
#define IGR_ANNOTATION_FLAGS_LOCKEDCONTENTS		(1 << 9)

	/**
	 * Structure representing an annotation.
	 * It includes details such as type, flags, position, size,
	 * and reserved data for internal use.
	 */
	struct IGR_Annotation
	{
	    const void* reserved;               ///< Reserved data for internal use.
	    IGR_ANNOTATION_TYPE type;           ///< Type of the annotation.
	    IGR_ANNOTATION_FLAGS_TYPE flags;    ///< Flags on this annotation.
	    IGR_LONG x;                         ///< X pixel position of the annotation's bounding box.
	    IGR_LONG y;                         ///< Y pixel position of the annotation's bounding box.
	    IGR_LONG width;                     ///< Width in pixels of the annotation's bounding box.
	    IGR_LONG height;                    ///< Height in pixels of the annotation's bounding box.
	    IGR_LONG reserved2[8];              ///< Reserved data for internal use.
	};

#define IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_USECDECL  0x1

	/**
	*  Init_Instance initializes the Document Filters engine and authenticates your license.
	*    reserved    - [in] unused, must be zero
	*    path        - [in] Path to Perceptive executables
	*    inst        - [in,out] prior to the call, populate with your application License Code. After the call, returns your licensee information.
	*    handle      - [out] Returns the instance handle
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Init_Instance(IGR_LONG reserved, const char* path, Instance_Status_Block* init, IGR_SHORT* handle, Error_Control_Block* error);

	/**
	*  Close_Instance advises the Document Filters engine that the program is finished.
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Close_Instance(Error_Control_Block* error);

	/**
	*  Switch_Instance is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Switch_Instance(IGR_SHORT handle, Error_Control_Block* error);

	/**
	*  Widechar_to_UTF8 converts a UCS2 string into UTF-8
	*    source     - [in]  contains a NULL terminated UCS2 string
	*    buffer     - [out] pointer to a buffer that will hold the converted text
	*    bufferSize - [in]  indicates the size of the buffer, the system will copy upto this size including the NULL.
	*  Returns
	*    None
	*/
	void IGR_EXPORT Widechar_to_UTF8(const IGR_UCS2* source, char* buffer, IGR_LONG bufferSize);
	void IGR_EXPORT Widechar_to_UTF8_Ex(const IGR_UCS2* source, IGR_LONG sourceLen, char* buffer, IGR_LONG bufferSize);

	/**
	*  UTF8_to_Widechar converts a UTF-* string into a UCS2 string
	*    source     - [in]  contains a NULL terminated UTF-8 string
	*    buffer     - [out] pointer to a buffer that will hold the converted text
	*    bufferSize - [in]  indicates the size of the buffer, the system will copy upto this size including the NULL.
	*  Returns
	*    None
	*/
	void IGR_EXPORT UTF8_to_Widechar(const char* source, IGR_UCS2* buffer, IGR_LONG bufferSize);
	void IGR_EXPORT UTF8_to_Widechar_Ex(const char* source, IGR_LONG sourceLen, IGR_UCS2* buffer, IGR_LONG bufferSize);

	/**
	*  Set_Engine_Call_Tracing is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Set_Engine_Call_Tracing(IGR_LONG, IGR_LONG, const char*);

	/**
	*  Set_API_Charset is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Set_API_Charset(IGR_LONG, Error_Control_Block* error);

	/**
	*  Set_DLL_Option is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Set_DLL_Option(const char*, Error_Control_Block* error);

	/**
	*  ISYS_Multiplex is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT ISYS_Multiplex(IGR_LONG, size_t*, size_t*, Error_Control_Block* error);

	/**
	*  IGR_Get_File_Type gets the type and the capabilities of a given document.
	*    filename     - [in]  Path to the document to be opened.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_File_Type(const IGR_UCS2* filename, IGR_LONG* capabilities, IGR_LONG* filetype, Error_Control_Block* error);

	/**
	*  IGR_Get_Stream_Type gets the type and the capabilities of a given stream object
	*    stream       - [in]  The stream can be either user implemented, or created using the IGR_Make_Stream_From_File and IGR_Make_Stream_From_Memory utility functions.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Stream_Type(struct IGR_Stream* stream, IGR_LONG* capabilities, IGR_LONG* filetype, Error_Control_Block* error);


	/**
	*  IGR_Open_File opens a document for text extraction or enumeration of sub-documents.
	*    filename     - [in]  Path to the document to be opened.
	*    flags        - [in]  Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags
	*                         affect the verbosity or the format of the extracted data.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    handle       - [out] Returns a handle to be used in subsequent calls.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_File(const IGR_UCS2* filename, IGR_LONG flags, IGR_LONG* capabilities, IGR_LONG* filetype, IGR_LONG* handle, Error_Control_Block* error);

	/**
	*  IGR_Open_Stream opens a document for text extraction or enumeration of sub-documents.
	*    stream       - [in]  The stream can be either user implemented, or created using the IGR_Make_Stream_From_File and IGR_Make_Stream_From_Memory utility functions.
	*    flags        - [in]  Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags
	*                         affect the verbosity or the format of the extracted data.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    handle       - [out] Returns a handle to be used in subsequent calls.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_Stream(struct IGR_Stream* stream, IGR_LONG flags, IGR_LONG* capabilities, IGR_LONG* filetype, IGR_LONG* handle, Error_Control_Block* error);

	/**
	*  IGR_Open_File_Ex opens a document for text extraction or enumeration of sub-documents and also controls the output format, including converting the source document to HTML.
	*    filename     - [in]  Path to the document to be opened.
	*    flags        - [in]  Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags
	*                         affect the verbosity or the format of the extracted data.
	*    options      - [in]  Extended processing options, used when converting the document to HTML. The Open Document Options are expressed as Name=Value with a semicolon delimiter.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    handle       - [out] Returns a handle to be used in subsequent calls.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_File_Ex(const IGR_UCS2* filename, IGR_LONG flags, const IGR_UCS2* options, IGR_LONG* capabilities, IGR_LONG* filetype, IGR_LONG* handle, Error_Control_Block* error);

	/**
	*  IGR_Open_Stream_Ex opens a document for text extraction or enumeration of sub-documents and also controls the output format, including converting the source document to HTML.
	*    stream       - [in]  The stream can be either user implemented, or created using the IGR_Make_Stream_From_File and IGR_Make_Stream_From_Memory utility functions.
	*    flags        - [in]  Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags
	*                         affect the verbosity or the format of the extracted data.
	*    options      - [in]  Extended processing options, used when converting the document to HTML. The Open Document Options are expressed as Name=Value with a semicolon delimiter.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    handle       - [out] Returns a handle to be used in subsequent calls.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_Stream_Ex(struct IGR_Stream* stream, IGR_LONG flags, const IGR_UCS2* options, IGR_LONG* capabilities, IGR_LONG* file_type, IGR_LONG* handle, Error_Control_Block* error);

	/**
	*  IGR_Get_Text extracts the text of previously opened document.
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    buffer       - [in, out]  Application allocated memory block that will be filled with the next portion of text.
	*    bufferSize   - [in, out]  Prior to the call, set to the size in Unicode characters of the buffer, After the call, indicates the actual number of Unicode characters extracted.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*   IGR_NO_MORE          - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support text extraction
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Text(IGR_LONG handle, IGR_UCS2* buffer, IGR_LONG* bufferSize, Error_Control_Block* error);

	/**
	*  IGR_Get_Subfile_Entry enumerates the sub-documents contained in a previously opened compound document, such as message documents (MSG) or archive documents (ZIP).
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    id           - [out]  Application allocated memory block of 8192 bytes that will be filled with up to 4096 Unicode characters that specify the unique ID of the next subdocument.
	*    name         - [out]  Application allocated memory block of 2048 bytes that will be filled with up to 1024 Unicode characters that specify the name of the sub-document.
	*    date         - [out]  Returns the date and time of the sub-document in FILETIME format.
	*    size         - [out]  Returns the size in bytes of the sub-document.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*   IGR_NO_MORE          - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfile_Entry(IGR_LONG handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size, Error_Control_Block* error);

	/**
	*  IGR_Get_Subfile_Entry enumerates the sub-documents contained in a previously opened compound document, such as message documents (MSG) or archive documents (ZIP).
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    result       - [in/out]  Application allocated memory block that will be populated with details of the subfile.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*   IGR_NO_MORE          - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfile_Entry_Ex(IGR_LONG handle, struct IGR_Subfile_Info* result, Error_Control_Block* error);

	/**
	*  IGR_Get_Subfiles_Enumerator returns a new enumerator for subfiles. Unlike IGR_Get_Subfile_Entry, multiple enumerators can be requested
	*  to re-process subfiles of an open document.
	*    docHandle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    enumerator      - [out] Populated with handle to new enumerator, which can be passed to the enumeration functions
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfiles_Enumerator(IGR_LONG docHandle, HSUBFILES* enumerator, Error_Control_Block* error);

	/**
	*  IGR_Extract_Subfile extracts a sub-document to disk from a compound document, given the ID of the sub-document.
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    id           - [in]  Unique ID of the sub-document to be extracted, obtained by a call to IGR_Get_Subfile_Entry.
	*    destination  - [in]  Path to a file on disk, where the binary the sub-document will be written.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*   IGR_NO_MORE          - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Subfile(IGR_LONG handle, const IGR_UCS2* id, const IGR_UCS2* destination, Error_Control_Block* error);

	/**
	*  IGR_Extract_Subfile_Stream extracts a sub-document to a stream from a compound document, given the ID of the sub-document.
	*  The sub-document ID is obtained previously by IGR_Get_Subfile_Entry from the compound document, after being opened by IGR_Open_File or IGR_Open_Stream.
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    id           - [in]  Unique ID of the sub-document to be extracted, obtained by a call to IGR_Get_Subfile_Entry.
	*    stream       - [out] A pointer to a system allocated memory stream will be returned. It is the caller's responsibility to free the
	*                         stream object by calling Stream->Close()
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*   IGR_NO_MORE          - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Subfile_Stream(IGR_LONG handle, const IGR_UCS2* id, struct IGR_Stream** stream, Error_Control_Block* error);

	/**
	*  IGR_Close_File releases the resources associated with the file handle. It must be called for every document opened by IGR_Open_File,
	*  IGR_Open_File_Ex, IGR_Open_Stream or IGR_Open_Stream_Ex.
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Close_File(IGR_LONG handle, Error_Control_Block* error);

	/**
	*  The IGR_Multiplex function is used to supply extensible functionality to the Document Filters API.
	*    function     - [in]  The function to perform, see Document Filters Manual for complete list.
	*    param1       - [in,out] Used for some functions.
	*    param2       - [in,out] Used for some functions.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if param1, param2 or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Multiplex(IGR_LONG function, size_t* param1, size_t* param2, Error_Control_Block* error);


	/**
	*  IGR_Subfiles_Next enumerates the sub-documents contained in a subfile enumeration, returned from IGR_Get_Subfiles_Enumerator or similar .
	*    handle       - [in]  Handle to an enumeration, opened by a call to IGR_Get_Subfiles_Enumerator or similar.
	*    id           - [out]  Application allocated memory block of 8192 bytes that will be filled with up to 4096 Unicode characters that specify the unique ID of the next subdocument.
	*    name         - [out]  Application allocated memory block of 2048 bytes that will be filled with up to 1024 Unicode characters that specify the name of the sub-document.
	*    date         - [out]  Returns the date and time of the sub-document in FILETIME format.
	*    size         - [out]  Returns the size in bytes of the sub-document.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*   IGR_NO_MORE          - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Next(HSUBFILES handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size, Error_Control_Block* error);

	/**
	*  IGR_Subfiles_Next enumerates the sub-documents contained in a subfile enumeration, returned from IGR_Get_Subfiles_Enumerator or similar .
	*    handle       - [in]  Handle to an enumeration, opened by a call to IGR_Get_Subfiles_Enumerator or similar.
	*    result       - [in/out]  Application allocated memory block that will be populated with details of the subfile.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*   IGR_NO_MORE          - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Next_Ex(HSUBFILES handle, struct IGR_Subfile_Info* result, Error_Control_Block* error);

	/**
	*  IGR_Subfiles_Reset resets an enumeration back to the beginning. Call IGR_Subfiles_Next to start reprocessing the document.
	*  Calling IGR_Subfiles_Reset is not required to iterate the first time.
	*    handle       - [in]  Handle to an enumeration, opened by a call to IGR_Get_Subfiles_Enumerator or similar.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Reset(HSUBFILES handle, Error_Control_Block* error);

	/**
	*  IGR_Subfiles_Close frees the resources behind an enumeration created by IGR_Get_Subfiles_Enumerator or similar.
	*	Enumerators MUST be freed before the owning document is closed.
	*    handle       - [in]  Handle to an enumeration, opened by a call to IGR_Get_Subfiles_Enumerator or similar.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Close(HSUBFILES handle, Error_Control_Block* error);

	/**
	*  IGR_Convert_File converts the specified document into a plain text or HTML file, without the need to call IGR_Open_File and IGR_Get_Text.
	*    filename     - [in]  Path to the document to be converted.
	*    flags        - [in]  Specifies processing options controlling the output. See Open Document Flags.
	*    options      - [in]  Extended processing options used when converting the document to HTML. The Open Document Options are expressed as Name=Value with a semicolon delimiter.
	*    output       - [in]  The filename where the output document is to be written.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if filename, options, output or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Convert_File(const IGR_UCS2* filename, IGR_LONG flags, const IGR_UCS2* options, const IGR_UCS2* output, Error_Control_Block* error);

	/**
	*  IGR_Make_Stream_From_File creates a Perceptive stream based on a file for use with the Perceptive document stream functions.
	*    filename     - [in]  Path to the document to be opened.
	*    flags        - [in]  A bitfield of options that affect the behavior of the stream object.
	*    stream       - [out] A system allocated memory stream structure will be returned.  It is the caller's responsibility to free
	*                         the stream object by calling stream->Close()
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if filename, stream or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_File(const IGR_UCS2* filename, IGR_LONG flags, struct IGR_Stream** stream, Error_Control_Block* error);

	/**
	*  IGR_Make_Stream_From_File creates a Perceptive stream based on a file for use with the Perceptive document stream functions.
	*    buffer       - [in]  A pointer to a user allocated memory buffer that contains the binary document you wish to work with.
	*    bufferSize   - [in]  Indicates the size of the buffer pointed to by Data.
	*    destruct     - [in]  Optional function pointer that will be called when the stream object is closed, giving your application the
	*                         ability to free the memory buffer or perform other cleanup routines. Specify NULL if unused.
	*                         The destructor must take the following form: void __cdecl Destruct(void *data);
	*    flags        - [in]  A bitfield of options that affect the behavior of the stream object.
	*    stream       - [out] A system allocated memory stream structure will be returned.  It is the caller's responsibility to free
	*                         the stream object by calling stream->Close()
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if buffer, stream or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_Memory(void* buffer, size_t bufferSize, void* destruct, struct IGR_Stream** stream, Error_Control_Block* error);

	/**
	*  IGR_Make_Stream_From_Functions creates a new IGR stream based off the context and functions. This is functionally
	*  identical to constructing the IGR_Stream structure by hand; this function can be used by languages that do not
	*  have precise control over memory layout.
	*
	*    context      - [in] Context is a user-provided block that is passed to each callback function
	*    flags        - [in] A bitfield of options that affect the behavior of the stream object
	*    seek         - [in] Function pointer to required Seek method
	*    read         - [in] Function pointer to required Read method
	*    write        - [in] Function pointer to optional Write method
	*    destruct     - [in] Function pointer to required destructor that can be used to dispose of *context* memory
	*    stream       - [out] A system allocated memory stream structure will be returned.  It is the caller's responsibility to free
	*                         the stream object by calling stream->Close()
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if buffer, stream or error are invalid*
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_Functions(void* context,
		IGR_LONG flags,
		IGR_ULONG(CALLBACK* seek)(void* context, IGR_LONGLONG offset, IGR_ULONG whence),
		IGR_ULONG(CALLBACK* read)(void* context, void* buffer, IGR_ULONG buffer_size),
		IGR_ULONG(CALLBACK* write)(void* context, const void* buffer, IGR_ULONG buffer_size),
		IGR_LONG(CALLBACK* action)(void* context, int actionId, void* action_data),
		void (CALLBACK* destruct)(void* context),
		struct IGR_Stream** stream,
		Error_Control_Block* error
	);

	/**
	*  IGR_Extend_Stream creates a custom stream that accepts callbacks.
	*    pStream      - [in]  A valid IGR_Stream instance.
	*    cb           - [in]  Pointer to the API user's function to handle callbacks generated while processing the stream.
	*    context      - [in]  API user-supplied context information.
	*    ppExtStream  - [in]  The extended stream which should be used instead of the original stream.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_ERROR - returned if the input stream, custom stream, calback, or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Extend_Stream(struct IGR_Stream* pStream, IGR_CALLBACK cb, void* context, struct IGR_Stream** ppExtStream, Error_Control_Block* error);


	/**
	*  IGR_Get_Memory_Status is a Windows-only memory diagnostic.
	*/
	size_t IGR_EXPORT IGR_Get_Memory_Status(char* buffer, size_t len);

	/**
	*  IGR_Get_Image_Entry enumerates the set of images, when HTML conversion is in effect as set by the Open Document Flags, contained in a previously opened document.
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    id           - [out]  Application allocated memory block of 8192 bytes that will be filled with up to 4096 Unicode characters that specify the unique ID of the next subdocument.
	*    name         - [out]  Application allocated memory block of 2048 bytes that will be filled with up to 1024 Unicode characters that specify the name of the sub-document.
	*    date         - [out]  Returns the date and time of the sub-document in FILETIME format.
	*    size         - [out]  Returns the size in bytes of the sub-document.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if id, name, date, size or error are invalid
	*   IGR_NO_MORE           - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Image_Entry(IGR_LONG handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size, Error_Control_Block* error);

	/**
	*  IGR_Get_Images_Enumerator returns a new enumerator for the set of images, when HTML conversion is in effect as set by the Open Document Flags, contained in a previously opened document
	*    docHandle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    enumerator      - [out] Populated with handle to new enumerator, which can be passed to the enumeration functions
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if docHandle or error are invalid
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Images_Enumerator(IGR_LONG docHandle, HSUBFILES* enumerator, Error_Control_Block* error);

	/**
	*  IGR_Extract_Image extracts an image to disk from a document opened with HTML conversion in effect.
	*
	*  The image ID is obtained previously by IGR_Get_Image_Entry from the document, after being opened by IGR_Open_File with the HTML conversion Open Document Flags set.
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    id           - [in]  Unique ID of the sub-document to be extracted, obtained by a call to IGR_Get_Subfile_Entry.
	*    destination  - [in]  Path to a file on disk, where the binary the sub-document will be written.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if id, destination or error are invalid
	*   IGR_NO_MORE           - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Image(IGR_LONG handle, const IGR_UCS2* id, const IGR_UCS2* destination, Error_Control_Block* error);

	/**
	*  IGR_Extract_Image_Stream extracts an image to a stream from a document, given the ID of the image.
	*  The image ID is obtained previously by IGR_Get_Image_Entry from the document, after being opened by IGR_Open_File or IGR_Open_Stream with the HTML conversion Open Document Flags set.
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    id           - [in]  Unique ID of the sub-document to be extracted, obtained by a call to IGR_Get_Subfile_Entry.
	*    stream       - [out] A pointer to a system allocated memory stream will be returned. It is the caller's responsibility to free the
	*                         stream object by calling Stream->Close()
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if id, stream or error are invalid
	*   IGR_NO_MORE           - there is no more text to extract
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - this document does not support sub-file enumeration
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Image_Stream(IGR_LONG handle, const IGR_UCS2* id, struct IGR_Stream** stream, Error_Control_Block* error);

	/**
	*  IGR_Calculate_MD5 will calculate the MD5 hash of an input stream for unique document identification.
	*    stream       - [in]  An open IGR_Stream
	*    dest         - [in]  A buffer to receive the nullterminated MD5 hash (as a Unicode string). Must be allocated by the
	*                         caller and be able to hold at least 33 UCS2 characters.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if stream, dest or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Calculate_MD5(struct IGR_Stream* stream, IGR_UCS2* dest, Error_Control_Block* error);

	/**
	*  IGR_Calculate_SHA1 will calculate the SHA1 hash of an input stream for unique document identification.
	*    stream       - [in]  An open IGR_Stream
	*    dest         - [in]  A buffer to receive the nullterminated SHA1 hash (as a Unicode string). Must be allocated by the
	*                         caller and be able to hold at least 41 UCS2 characters.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - returned if stream, dest or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Calculate_SHA1(struct IGR_Stream* stream, IGR_UCS2* dest, Error_Control_Block* error);

	/**
	*  Return the number of pages for a document opened in HD mode.
	*  Returns
	*   IGR_OK
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - returned when docHandle not opened in HD mode
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Count(IGR_LONG docHandle, IGR_LONG* pageCount, Error_Control_Block* error);

	/**
	*  Return a handle to the given page
	*    docHandle   - [in]  handle to a HD document
	*    pageIndex   - [in]  0-based index of the page being requested
	*    pageHandle  - [out] set to the page handle
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if pageHandle or error are invalid
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - returned when docHandle not opened in HD mode
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_Page(IGR_LONG docHandle, IGR_LONG pageIndex, HPAGE* pageHandle, Error_Control_Block* error);

	/**
	*  Close the given page handle
	*    pageHandle  - [in] set to the page handle
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Close_Page(HPAGE pageHandle, Error_Control_Block* error);

	/**
	*  Returns the number of words on this page
	*    pageHandle  - [in] set to the page handle
	*    wordCount   - [out] on success, contains the number words on page
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if wordCount or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Word_Count(HPAGE pageHandle, IGR_LONG* wordCount, Error_Control_Block* error);

	/**
	*  Returns the word references from the given index on the page
	*    pageHandle  - [in] set to the page handle
	*    index       - [in] 0-based index of the first word to return
	*    count       - [in,out] on input, contains the size of the words array. on result, contains the number of words copied.
	*    words       - [in,out] buffer that the word reference data will be written to
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if wordCount or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Words(HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Page_Word* words, Error_Control_Block* error);

	/**
	*  Returns the size (in pixels) of the specified page
	*    pageHandle  - [in] set to the page handle
	*    width       - [out] set to the width of the page, in pixels
	*    height      - [out] set to the height of the page, in pixels
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if width, height or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Dimensions(HPAGE pageHandle, IGR_LONG* width, IGR_LONG* height, Error_Control_Block* error);

	/**
	*  Returns the next piece of page text
	*    pageHandle  - [in] set to the page handle
	*    size        - [in,out] on calling, is set to the number of CHARACTERS that buffer points to. on return, is
	*                           set to the number of CHARACTERS returned.
	*    buffer      - [out] copies text up to size CHARACTERS to the buffer
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if size, buffer or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Text(HPAGE pageHandle, IGR_UCS2* buffer, IGR_LONG* size, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Image_Entry(HPAGE pageHandle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Images_Enumerator(IGR_LONG docHandle, HSUBFILES* enumerator, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Page_Image(HPAGE pageHandle, const IGR_UCS2* id, const IGR_UCS2* destination, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Page_Image_Stream(HPAGE pageHandle, const IGR_UCS2* id, struct IGR_Stream** stream, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Count(HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Elements(HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Page_Form_Element* items, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Str(const struct IGR_Page_Form_Element* item, IGR_LONG value_type, IGR_LONG char_count, IGR_UCS2* buffer, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Option_Str(const struct IGR_Page_Form_Element* item, IGR_LONG value_type, IGR_LONG option_index, IGR_LONG char_count, IGR_UCS2* buffer, Error_Control_Block* error);

	/**
	*  Enumerate all bookmarks, upto recurse_depth, to an application-defined callback function.
	*    docHandle     - [in] handle to a HD document
	*    root          - [in] optional bookmark value to enumerate, or NULL to enumerate from root
	*    recurse_depth - [in] maximum depth to scan
	*    callback      - [in] a pointer to an application-defined callback function.
	*    context       - [in] an application-defined value to be passed to the callback function.
	*    error         - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if stream, canvasHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Enum_Bookmarks(IGR_LONG docHandle, const struct IGR_Bookmark* root, IGR_LONG recurse_depth, IGR_CALLBACK_BOOKMARK callback, void* context, Error_Control_Block* error);

	/**
	*  Retrieve the root bookmark item for the document.
	*    docHandle     - [in] handle to a HD document
	*    dest          - [in] a pointer to an application provided IGR_Bookmark that will be populated with Bookmark data
	*    error         - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if stream, canvasHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_Root(IGR_LONG docHandle, struct IGR_Bookmark* dest, Error_Control_Block* error);

	/**
	*  Retrieve the first child of the provided bookmark root.
	*    docHandle     - [in] handle to a HD document.
	*    root          - [in] the item to extract the first child from, or pass NULL to get the first bookmark of the document.
	*    dest          - [in] a pointer to an application provided IGR_Bookmark that will be populated with Bookmark data.
	*    error         - [out] contains any error text.
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if stream, canvasHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_First_Child(IGR_LONG docHandle, const struct IGR_Bookmark* root, struct IGR_Bookmark* dest, Error_Control_Block* error);

	/**
	*  Retrieve the next sibling of the provided bookmark root.
	*    docHandle     - [in] handle to a HD document.
	*    root          - [in] the item to extract the first child from.
	*    dest          - [in] a pointer to an application provided IGR_Bookmark that will be populated with Bookmark data.
	*    error         - [out] contains any error text.
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if stream, canvasHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_Next_Sibling(IGR_LONG docHandle, const struct IGR_Bookmark* root, struct IGR_Bookmark* dest, Error_Control_Block* error);

	/**
	*  Returns a new canvas, used to render paginated output (pages)
	*    type        - [in] one of the following:
	*                       IGR_DEVICE_IMAGE_PNG
	*                       IGR_DEVICE_IMAGE_JPG
	*                       IGR_DEVICE_IMAGE_PDF
	*                       IGR_DEVICE_IMAGE_TIF
	*    filename    - [in] specifies the filename to save
	*    stream      - [in] specifies the stream where the content should be rendered
	*    canvasHandle- [out] contains the newly created output canvas handle, must be closed with IGR_Close_Canvas
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if stream, canvasHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Output_Canvas(IGR_LONG type, const IGR_UCS2* filename, const IGR_UCS2* options, HCANVAS* canvasHandle, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Output_Canvas_On(IGR_LONG type, struct IGR_Writable_Stream* stream, const IGR_UCS2* options, HCANVAS* canvasHandle, Error_Control_Block* error);

	/**
	*  Close the given canvas handle
	*    canvasHandle  - [in] set to the canvas handle
	*    error         - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Close_Canvas(HCANVAS canvasHandle, Error_Control_Block* error);

	/**
	*  Renders the given page onto the canvas, this will create a new page for multi-page canvases
	*    page        - [in] the handle to a page returned from IGR_Open_Page
	*    canvas      - [in] the handle to a canvas returned from IGR_Make_Output_Canvas
	*    options     - [in] Extended processing options.  The Open Document Options are expressed as Name=Value with a semicolon delimiter.
	*    properties  - [in] properties that control per page rendering
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if error is invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Render_Page(HPAGE page, HCANVAS canvas, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Render_Page_Ex(HPAGE page, HCANVAS canvas, const IGR_UCS2* options, const struct IGR_Render_Page_Properties* properties, Error_Control_Block* error);

	/**
	*  Redacts/removes the words in the specified range from the document. The text will be removed and the region blacked out.
	*    page        - [in] the handle to a page returned from IGR_Open_Page
	*    from        - [in] the first word to be redacted
	*    to          - [in] the last word to be redacted
	*    type        - [in] reserved, must be zero
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if error is invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Redact_Page_Text(HPAGE page, IGR_LONG From, IGR_LONG To, IGR_LONG Type, Error_Control_Block* error);

	/**
	*  Returns page style/attribute information
	*    page        - [in] the handle to a page returned from IGR_Open_Page
	*    name        - [in] the name of the style/attribute
	*    buffer      - [out] buffer that is filled with the requested attribute/style
	*    size        - [in/out] contains the size of the buffer, in characters. On return, is set to the size of the string
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if error is invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Attribute(HPAGE page, const IGR_UCS2* name, IGR_UCS2* buffer, IGR_LONG* size, Error_Control_Block* error);

	/**
	*  Returns information about a supported format.
	*    formatid    - [in] the format id, as returned by IGR_Get_File_Type
	*    what        - [in] indicates the information to return, may be one of:
	*                       0 - copy the long form of the format name
	*                       1 - copy the short form of the format name
	*                       2 - copy the Perceptive Search index config file form of the format name
	*                       3 - copy the class of the format
	*                       4 - indicate if the format is a legacy format
	*    buffer      - [in] a pointer to a string buffer to receive the text, must be at least 255 bytes in length.
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if error is invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Format_Attribute(IGR_LONG formatid, IGR_LONG what, char* buffer, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Option_Attribute(IGR_LONG formatid, IGR_LONG what, char* buffer, size_t bufferSize, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Arc(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3, IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_AngleArc(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG radius, IGR_LONG startAngle, IGR_LONG sweepAngle, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Chord(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3, IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Ellipse(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Rect(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_LineTo(HCANVAS canvas, IGR_LONG x, IGR_LONG y, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_MoveTo(HCANVAS canvas, IGR_LONG x, IGR_LONG y, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Pie(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3, IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_RoundRect(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG radius, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_TextOut(HCANVAS canvas, IGR_LONG x, IGR_LONG y, const IGR_UCS2* text, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_TextRect(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, const IGR_UCS2* text, IGR_LONG flags, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_MeasureText(HCANVAS canvas, const IGR_UCS2* text, IGR_LONG* width, IGR_LONG* height, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetPen(HCANVAS canvas, IGR_LONG color, IGR_LONG width, IGR_LONG style, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetBrush(HCANVAS canvas, IGR_LONG color, IGR_LONG style, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetFont(HCANVAS canvas, const IGR_UCS2* fontFamily, IGR_LONG size, IGR_LONG style, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetOpacity(HCANVAS canvas, IGR_BYTE opacity, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_DrawImage(HCANVAS canvas, IGR_LONG x, IGR_LONG y, void* imagedata, size_t imagesize, const IGR_UCS2* mimetype, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_DrawScaleImage(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, void* imagedata, size_t imagesize, const IGR_UCS2* mimetype, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Rotation(HCANVAS canvas, IGR_LONG degrees, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Reset(HCANVAS canvas, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Annotate_JSON(HCANVAS canvas, const IGR_UCS2* json, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlink_Count(HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlinks(HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Hyperlink* items, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlink_Str(const struct IGR_Hyperlink* item, IGR_HYPERLINK_GET_VALUE_TYPE value_type, IGR_LONG char_count, IGR_UCS2* buffer, Error_Control_Block* error);

	/**
	*  Insert a blank page into the canvas
	*    canvas      - [in] provide the canvas handle of a canvas created by IGR_Make_Output_Canvas
	*    options     - [in] string providing any extended processing options
	*    width       - [in] width of the new page in pixels
	*    width       - [in] height of the new page in pixels
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if error is invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Blank_Page(HCANVAS canvas, const IGR_UCS2* options, IGR_LONG width, IGR_LONG height, const void* reserved, Error_Control_Block* error);

	/**
	*  Opens a document from an OS based, device-dependant bitmap (Windows-Only).
	*    bitmap       - [in]  Provide a handle to the OS bitmap (HBITMAP)
	*    palette      - [in]  Provide a handle to an optional OS palette (HPALETTE)
	*    flags        - [in]  Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags
	*                         affect the verbosity or the format of the extracted data.
	*    options      - [in]  Extended processing options, used when converting the document to HTML. The Open Document Options are expressed as Name=Value with a semicolon delimiter.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    handle       - [out] Returns a handle to be used in subsequent calls.
	*    error        - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_DDB(IGR_HBITMAP bitmap, IGR_HPALETTE palette, IGR_LONG flags, const IGR_UCS2* options, IGR_LONG* capabilities, IGR_LONG* file_type, IGR_LONG* handle, Error_Control_Block* error);

	/**
	*  Opens a document from raw pixel information.
	*    bitmap       - [in]  Provides details on how to process the bitmap information
	*    flags        - [in]  Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags
	*                         affect the verbosity or the format of the extracted data.
	*    options      - [in]  Extended processing options, used when converting the document to HTML. The Open Document Options are expressed as Name=Value with a semicolon delimiter.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    handle       - [out] Returns a handle to be used in subsequent calls.
	*    error        - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_DIB(const struct IGR_Open_DIB_Info* bitmap, IGR_LONG flags, const IGR_UCS2* options, IGR_LONG* capabilities, IGR_LONG* file_type, IGR_LONG* handle, Error_Control_Block* error);

	/**
	*  Returns the count of annotations for a given page
	*    pageHandle   - [in]  the handle to a page returned from IGR_Open_Page
	*    count        - [out] contains the count of annotations on the provided page
	*    error        - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Count(HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);

	/**
	*  Returns annotation objects for a given page
	*    pageHandle   - [in]     the handle to a page returned from IGR_Open_Page
	*    index        - [in]     the offset to begin returning annotations
	*    count        - [in/out] the length of the array items, on return is set to the number of elements filled into the array
	*    items        - [out]    buffer to be filled with up to count number of annotations
	*    error        - [out]    contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotations(HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Annotation* items, Error_Control_Block* error);

	/**
	*  Returns a string value for a given annotation
	*    item         - [in]     an IGR_Annotation pointer returned from IGR_Get_Page_Annotations
	*    path         - [in]     the path of the annotation property to return
	*    char_count   - [in/out] contains the size of the buffer, in characters. On return, is set to the size of the string
	*    buffer       - [out]    buffer that is filled with the string value
	*    error        - [out]    contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Str(const struct IGR_Annotation* item, const IGR_UCS2* path, IGR_LONG* char_count, IGR_UCS2* buffer, Error_Control_Block* error);

	/**
	*  Returns an integer value for a given annotation
	*    item         - [in]  an IGR_Annotation pointer returned from IGR_Get_Page_Annotations
	*    path         - [in]  the path of the annotation property to return
	*    buffer       - [out] buffer that is filled with the integer value
	*    error        - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Long(const struct IGR_Annotation* item, const IGR_UCS2* path, IGR_LONG* buffer, Error_Control_Block* error);

	/**
	*  Returns the pixel data for the given page
	*    pageHandle    - [in] set to the page handle
	*    sourceRect    - [in] indicate the rectange to extract, or NULL to extract the entire page
	*    destSize      - [in] indicate the size of the output, or NULL to extract at default size
	*    flags         - [in] processing flags
	*    pixel_format  - [in] indicate the format of the pixel data to extract
	*    pixel_data    - [in/out] structure that is populated with the pixel data
	*    error         - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Pixels(
		HPAGE pageHandle,
		const struct IGR_Rect* sourceRect,
		const struct IGR_Size* destSize,
		IGR_ULONG flags,
		const IGR_UCS2* options,
		IGR_OPEN_BITMAP_PIXEL_TYPE pixel_format,
		struct IGR_Page_Pixels* pixel_data,
		Error_Control_Block* error);

	/**
	*  Releases pixel data previously allocated by IGR_Get_Page_Pixels
	*    pixel_data    - [in/out] structure that is populated with the pixel data
	*    error         - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Free_Page_Pixels(
		struct IGR_Page_Pixels* pixel_data,
		Error_Control_Block* error);

	/**
	*  Remove existing bookmarks from the canvas.
	*     canvas - [in] the handle to the canvas where bookmarks are being cleared
	*     error  - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Bookmarks_Clear(HCANVAS canvas, Error_Control_Block* error);

	/**
	*  Append a single bookmark to the canvas.
	*    canvas    - [in] the handle to the canvas where bookmarks are being placed
	*    bookmark  - [in] pointer to bookmark data to add
	*    error     - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Bookmarks_Append(HCANVAS canvas, const struct IGR_Bookmark* bookmark, Error_Control_Block* error);

#define IGR_OPEN_FROM IGR_LONG
#define IGR_OPEN_FROM_FILENAME_UTF16 0
#define IGR_OPEN_FROM_STREAM 1

#define IGR_OPEN_CALLBACK_ACTION IGR_LONG
#define IGR_OPEN_CALLBACK_ACTION_HEARTBEAT 0
#define IGR_OPEN_CALLBACK_ACTION_PASSWORD 1
#define IGR_OPEN_CALLBACK_ACTION_LOCALIZE 2

	/**
	* Callback that is passed to IGR_Open_Ex and called for specific events while processing the document.
	*   action   - The type of action that prompted the calling of the callback.
	*   payload  - A structure that can be modified to provide information that will be used to process the document.  The type
	*              of the structure corresponds to the type of action, e.g. if action is IGR_OPEN_CALLBACK_ACTION_HEARTBEAT,
	*              then payload is a IGR_Open_Callback_Action_Heartbeat structure.
	*   context	 - Contextual information that was passed to IGR_Open_Ex as the callback_context argument.
	*/
	typedef IGR_LONG(IGR_EXPORT* IGR_OPEN_CALLBACK)(IGR_OPEN_CALLBACK_ACTION action, void* payload, void* context);

	/**
	* Structure that is payload argument for IGR_OPEN_CALLBACK when action argument is IGR_OPEN_CALLBACK_ACTION_HEARTBEAT.
	*/
	struct IGR_Open_Callback_Action_Heartbeat
	{
		// [out] Indicate the size of this structure; must be populated.
		IGR_ULONG struct_size;
	};

	/**
	* Structure that is payload argument for IGR_OPEN_CALLBACK when action argument is IGR_OPEN_CALLBACK_ACTION_PASSWORD.
	*/
	struct IGR_Open_Callback_Action_Password
	{
		// [out] Indicate the size of this structure; must be populated.
		IGR_ULONG struct_size;
		// [out] Subfile Id indicating which subfile the password is requested for. May be empty to indicate the top level document password is requested. Null terminated.
		IGR_UCS2 id[4096];
		// [in] Password string to be used to decode the requested document or subfile. Must be null terminated.
		IGR_UCS2 password[4096];
	};

	/**
	* Structure that is payload argument for IGR_OPEN_CALLBACK when action argument is IGR_OPEN_CALLBACK_ACTION_LOCALIZE.
	*/
	struct IGR_Open_Callback_Action_Localize
	{
		// [out] Indicate the size of this structure; must be populated.
		IGR_ULONG struct_size;

		// Internal string resource id
		IGR_ULONG string_id;
		// Original string
		IGR_UCS2 original[1024];
		// Replacement string
		IGR_UCS2 replacement[1024];
	};

	/**
	*  IGR_Open_Ex opens a document for text extraction or enumeration of sub-documents.
	*    source_type       - [in]  The type of source, that can be either a UTF-16 string containing the filename, or an IGR_Stream record.
	*    source            - [in]  The pointer to the source based on the source_type parameter. If the source is an IGR_Stream,
								   the stream MUST remain valid for the life-type of the document handle returned.
	*    flags             - [in]  Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags
	*                              affect the verbosity or the format of the extracted data.
	*    options           - [in]  Extended processing options, used when converting the document to HTML. The Open Document Options are expressed as Name=Value with a semicolon delimiter.
	*    capabilities      - [out] Returns the Document Capabilities as a bitfield.
	*    filetype          - [out] Returns the Document Format Code of the document.
	*    reserved          - [in]  MUST be NULL. Reserved for future use.
	*    callback          - [in]  Callback that will be called for specific events while processing the document. MUST remain valid for the lifetime of the document handle returned.
	*    callback_context  - [in]  Contextual information that will be passed back when the callback is called. MUST remain valid for the lifetime of the document handle returned.
	*    handle            - [out] Returns a handle to be used in subsequent calls.
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_Ex(
		IGR_OPEN_FROM source_type,
		const void* source,
		IGR_LONG flags,
		const IGR_UCS2* options,
		IGR_LONG* capabilities,
		IGR_LONG* file_type,
		void* reserved,
		IGR_OPEN_CALLBACK callback,
		void* callback_context,
		IGR_LONG* handle,
		Error_Control_Block* error);

#define IGR_PAGE_ELEMENT_TYPE IGR_ULONG

#define IGR_PAGE_ELEMENT_TYPE_PAGE          1
#define IGR_PAGE_ELEMENT_TYPE_SECTION       2
#define IGR_PAGE_ELEMENT_TYPE_COLUMN        3
#define IGR_PAGE_ELEMENT_TYPE_PARAGRAPH     4
#define IGR_PAGE_ELEMENT_TYPE_LINE          5
#define IGR_PAGE_ELEMENT_TYPE_RUN           6
#define IGR_PAGE_ELEMENT_TYPE_WORD          7
#define IGR_PAGE_ELEMENT_TYPE_HEADER        8
#define IGR_PAGE_ELEMENT_TYPE_FOOTER        9
#define IGR_PAGE_ELEMENT_TYPE_IMAGE         10
#define IGR_PAGE_ELEMENT_TYPE_TABLE         11
#define IGR_PAGE_ELEMENT_TYPE_TABLE_ROW     12
#define IGR_PAGE_ELEMENT_TYPE_TABLE_CELL    13
#define IGR_PAGE_ELEMENT_TYPE_FORM          14
#define IGR_PAGE_ELEMENT_TYPE_FORM_ELEMENT  15
#define IGR_PAGE_ELEMENT_TYPE_FLOAT         16
#define IGR_PAGE_ELEMENT_TYPE_GRAPHIC       17
#define IGR_PAGE_ELEMENT_TYPE_TEXT_BOX      18

#define IGR_PAGE_ELEMENT_FLAG_TYPE IGR_ULONG
#define IGR_PAGE_ELEMENT_FLAG_DYNAMIC_CONTENT  0x01
#define IGR_PAGE_ELEMENT_FLAG_INJECTED_CONTENT 0x02

	struct IGR_Page_Element
	{
		IGR_ULONG struct_size;
		const void* reserved;
		IGR_PAGE_ELEMENT_TYPE type;
		IGR_ULONG depth;
		IGR_PAGE_ELEMENT_FLAG_TYPE flags;

		struct IGR_FRect pos;

		IGR_ULONG rotation;
	};
	typedef IGR_LONG(IGR_EXPORT* IGR_PAGE_ELEMENT_STYLES_CALLBACK)(const IGR_UCS2* name, const IGR_UCS2* value, void* context);
	typedef IGR_LONG(IGR_EXPORT* IGR_PAGE_ELEMENT_CALLBACK)(IGR_HPAGE page, const struct IGR_Page_Element* item, void* context);

	/**
	* IGR_Get_Page_Elements fills the {dest} buffer with {dest_count} page elements from the parent object, starting from the {first} index. If parent is NULL, the root
	* element is copied into {dest}.
	*    page              - [in] Page element containing the page elements to extract.
	*    parent            - [in] Optional parent element from which the children elements are to be extracted.
	*    index             - [in] The index of the first element to extract
	*    dest_count        - [in,out] On input, contains the number of elements in the {dest} array. On output, is set to the number of elements copied.
	*    dest              - [in,out] Pointer to {dest_count} array of IGR_Page_Element. On input, the first elements struct_size must be set. On output, the structs are filled with element data.
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Elements(IGR_HPAGE page, const struct IGR_Page_Element* parent, IGR_ULONG first, IGR_ULONG* dest_count, struct IGR_Page_Element* dest, Error_Control_Block* error);

	/**
	* IGR_Get_Page_Element_Root fills {dest} with the root element of the page.
	*    page              - [in] Page element containing the page elements to extract.
	*    dest              - [in,out] Pointer an IGR_Page_Element. On input, the elements struct_size must be set. On output, the struct is filled with element data.
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Root(IGR_HPAGE page, struct IGR_Page_Element* dest, Error_Control_Block* error);

	/**
	* IGR_Get_Page_Element_First_Child fills {dest} with the first child of the {parent} element.
	*    page              - [in] Page element containing the page elements to extract.
	*    parent            - [in] Optional parent element from which the children elements are to be extracted.
	*    dest              - [in,out] Pointer an IGR_Page_Element. On input, the elements struct_size must be set. On output, the struct is filled with element data.
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_First_Child(IGR_HPAGE page, const struct IGR_Page_Element* parent, struct IGR_Page_Element* dest, Error_Control_Block* error);

	/**
	* IGR_Get_Page_Element_Next_Sibling fills {dest} with the next sibling node of {element} element.
	*    page              - [in] Page element containing the page elements to extract.
	*    element           - [in] The element of the element where the sibling should be extracted.
	*    dest              - [in,out] Pointer an IGR_Page_Element. On input, the elements struct_size must be set. On output, the struct is filled with element data.
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Next_Sibling(IGR_HPAGE page, const struct IGR_Page_Element* element, struct IGR_Page_Element* dest, Error_Control_Block* error);

	/**
	* IGR_Get_Page_Element_Text returns any text associated with the page element.
	*    page              - [in] Page element containing the page elements to extract.
	*    source            - [in] The element of the element where the text should be extracted.
	*    char_count        - [in,out] On input, contains the number of elements in the {dest} array. On output, is set to the number of elements copied.
	*    buffer            - [in,out] Pointer an IGR_UCS2 array.
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Text(IGR_HPAGE page, const struct IGR_Page_Element* source, IGR_ULONG* char_count, IGR_UCS2* buffer, Error_Control_Block* error);

	/**
	* IGR_Get_Page_Element_Styles calls the {callback} with each style for the given {source} element.
	*    page              - [in] Page element containing the page elements to extract.
	*    source            - [in] The element of the element where the styles should be extracted.
	*    callback          - [in] User supplied callback where the style information is to be sent.
	*    context           - [in] User supplied callback context to pass when calling {callback}
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Styles(IGR_HPAGE page, const struct IGR_Page_Element* source, IGR_PAGE_ELEMENT_STYLES_CALLBACK callback, void* context, Error_Control_Block* error);

	/**
	* IGR_Get_Page_Element_Style returns the named style value from the page element.
	*    page              - [in] Page element containing the page elements to extract.
	*    source            - [in] The element of the element where the text should be extracted.
	*    source            - [in] The name of the style to extract.
	*    char_count        - [in,out] On input, contains the number of elements in the {dest} array. On output, is set to the number of elements copied.
	*    buffer            - [in,out] Pointer an IGR_UCS2 array.
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Style(IGR_HPAGE page, const struct IGR_Page_Element* source, const IGR_UCS2* name, IGR_ULONG* char_count, IGR_UCS2* buffer, Error_Control_Block* error);

	/**
	* IGR_Enum_Page_Elements recursively walks the page elements of {source} calling the user-supplied {callback}
	*    page              - [in] Page element containing the page elements to extract.
	*    source            - [in] The element of the element where the styles should be extracted.
	*    flags             - [in] Reserved for future use
	*    max_depth         - [in] The maximum depth of page elements to walk.
	*    callback          - [in] User supplied callback where the element information is to be sent.
	*    context           - [in] User supplied callback context to pass when calling {callback}
	*    error             - [out] contains any error text
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Enum_Page_Elements(IGR_HPAGE page, const struct IGR_Page_Element* source, IGR_ULONG flags, IGR_ULONG max_depth, IGR_PAGE_ELEMENT_CALLBACK callback, void* context, Error_Control_Block* error);

	/**
	*  IGR_Get_Handle_File_Type gets the type and the capabilities of an open document.
	*    handle       - [in]  Handle to an open document.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Handle_File_Type(IGR_HDOC handle, IGR_LONG* capabilities, IGR_LONG* filetype, Error_Control_Block* error);

	/**
	*  IGR_Get_Page_Handle_File_Type gets the type and the capabilities of an open page.
	*    handle       - [in]  Handle to an open document.
	*    capabilities - [out] Returns the Document Capabilities as a bitfield.
	*    filetype     - [out] Returns the Document Format Code of the document.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Handle_File_Type(IGR_HPAGE handle, IGR_LONG* capabilities, IGR_LONG* filetype, Error_Control_Block* error);


#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_TYPE            IGR_ULONG
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL           0
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT          1
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE          2
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING      3
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH      0xFF

#define IGR_COMPARE_DOCUMENTS_FLAGS_TYPE                 IGR_ULONG
#define IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS               0x1
#define IGR_COMPARE_DOCUMENTS_FLAGS_MOVES                0x10
#define IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING           0x20
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS          0x40
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE              0x80
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE        0x100
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION       0x200
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES            0x400
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS           0x800
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS           0x1000
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES         0x2000
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES         0x4000
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS            0x8000
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS_FOOTERS (IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS | IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS)

#define IGR_COMPARE_DOCUMENTS_COMPARE_TYPE               IGR_ULONG
#define IGR_COMPARE_DOCUMENTS_COMPARE_WORDS              0x0

#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_TYPE     IGR_ULONG
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_ORGINAL  0x0
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_REVISED  0x1
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_BOTH     0x2

	/**
	 * Context structure for the callback functions used in document comparison.
	 */
	struct IGR_Compare_Documents_Callback_Context
	{
		// Size of this structure
		IGR_ULONG struct_size;

		// Index to the document being processed. Will be either 0 or 1.
		IGR_ULONG doc_index;

		// The handle to the document being processed.
		IGR_LONG doc_handle;

		// The index to the current page being processed.
		IGR_ULONG page_index;

		// The handle to the current page being processed.
		IGR_HPAGE page_handle;
	};

	/**
	 * Structure representing a difference item in a compared document.
	 */
	struct IGR_Compare_Documents_Difference_Item
	{
		// Size of this structure
		IGR_ULONG struct_size;

		// The index of the page that generated the diff 
		IGR_ULONG page_index;

		// The bounding box of the word, in pixels.
		struct IGR_FRect bounds;

		// Null terminated UCS2 text buffer.
		const IGR_UCS2* text;
	};

	/**
	 * Structure representing a difference between compared documents.
	 */
	struct IGR_Compare_Documents_Difference
	{
		// Size of this structure
		IGR_ULONG struct_size;

		// The type of difference
		IGR_COMPARE_DOCUMENTS_DIFFERENCE_TYPE type;

		// The document where the difference occurred.
		IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_TYPE doc_source;

		// The page index in the original/left document of the first item in the hit list.
		IGR_ULONG original_page_index;

		// The page index in the revised/right document of the first item in the hit list.
		IGR_ULONG revised_page_index;

		// The number of difference items pointed to by items.
		IGR_ULONG item_count;

		// Pointer to the first rectangle 
		const struct IGR_Compare_Documents_Difference_Item* items;
	};

	struct IGR_Text_Compare_Enumerator;

	/**
	 * Settings structure for text comparison operations.
	 */
	struct IGR_Text_Compare_Settings
	{
		// Size of this structure
		IGR_ULONG struct_size;

		// The type of comparison to perform.
		IGR_COMPARE_DOCUMENTS_COMPARE_TYPE compare_type;

		// Flags that control the comparison logic.
		IGR_COMPARE_DOCUMENTS_FLAGS_TYPE flags;

		// Pointer to user supplied context that is passed as /context to each of the callbacks.
		void* user_context;

		// Reserved
		IGR_ULONG comparison_window;

		// Reserved
		IGR_ULONG comparison_overlap;

		// Reserved for future use
		void* reserved1;

		// Reserved for future use
		void* reserved2;

		// Reserved for future use
		void* reserved3;

		// Reserved for future use
		void* reserved4;

		// Optional callback that is called to provide when the page resource is required. 
		IGR_RETURN_CODE(CALLBACK* OpenPage) (void* context, const struct IGR_Compare_Documents_Callback_Context* info, IGR_ULONG page_index, IGR_HPAGE* page_handle);

		// Optional callback that is called to provide when the page resource can be destroyed. 
		IGR_RETURN_CODE(CALLBACK* ClosePage) (void* context, const struct IGR_Compare_Documents_Callback_Context* info, IGR_HPAGE page_handle);

		// Optional callback that is called to provide the margins for a given page. User can modify the left, top, right, margins of the page. Units are in pixels.
		IGR_RETURN_CODE(CALLBACK* GetPageMargins)(void* context, const struct IGR_Compare_Documents_Callback_Context* info, struct IGR_FRect* margins);

		// Optional callback that is called with the page area where text is to be diffed.
		IGR_RETURN_CODE(CALLBACK* GetPageArea)(void* context, const struct IGR_Compare_Documents_Callback_Context* info, struct IGR_FRect* page_area);

		// Optional callback that is called to provide the next page number to process.
		IGR_RETURN_CODE(CALLBACK* GetNextPageIndex) (void* context, const struct IGR_Compare_Documents_Callback_Context* info, IGR_ULONG* page_index);

		// Optional callback that is called to when an error is encountered
		IGR_RETURN_CODE(CALLBACK* HandleError) (void* context, const struct IGR_Compare_Documents_Callback_Context* info, const Error_Control_Block* ecb);

	};

	/**
	 * Structure representing a source document for text comparison.
	 */
	struct IGR_Text_Compare_Document_Source
	{
		// Size of this structure
		IGR_ULONG struct_size;

		// The handle to an open document
		IGR_LONG doc_handle;

		// The index to the first page to compare from document.
		IGR_ULONG doc_first_page;

		// The number of pages from doc1_first_page to compare.
		IGR_ULONG doc_page_count;

		// The margins, in pts, to exclude from the comparison for document.
		struct IGR_FRect doc_margins;
	};

	/**
	 * Pointer to a text comparison enumerator.
	 */
	typedef struct IGR_Text_Compare_Enumerator* IGR_HTEXTCOMPARE;

	/**
	 * Compares two documents and returns an enumerator for iterating through the differences.
	 *
	 * @param doc1      [in] The first document to compare.
	 * @param doc2      [in] The second document to compare.
	 * @param settings  [in] Settings for text comparison.
	 * @param enumerator[out] Pointer to store the enumerator for iterating through differences.
	 * @param error     [out] Contains any error text.
	 * @return          IGR_OK if successful, an error code otherwise.
	 */
	IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Documents(const struct IGR_Text_Compare_Document_Source* doc1
		, const struct IGR_Text_Compare_Document_Source* doc2
		, const struct IGR_Text_Compare_Settings* settings
		, IGR_HTEXTCOMPARE* enumerator
		, Error_Control_Block* error);

	/**
	 * Compares two pages within documents and returns an enumerator for iterating through the differences.
	 *
	 * @param page1         [in] The first page to compare.
	 * @param page1_margins [in] The margins of the first page.
	 * @param page2         [in] The second page to compare.
	 * @param page2_margins [in] The margins of the second page.
	 * @param settings      [in] Settings for text comparison.
	 * @param enumerator    [out] Pointer to store the enumerator for iterating through differences.
	 * @param error         [out] Contains any error text.
	 * @return              IGR_OK if successful, an error code otherwise.
	 */
	IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Pages(IGR_HPAGE page1
		, const struct IGR_FRect* page1_margins
		, IGR_HPAGE page2
		, const struct IGR_FRect* page2_margins
		, const struct IGR_Text_Compare_Settings* settings
		, IGR_HTEXTCOMPARE* enumerator
		, Error_Control_Block* error);

	/**
	 * Compares elements within pages of documents and returns an enumerator for iterating through the differences.
	 *
	 * @param page1             [in] The first page containing elements to compare.
	 * @param page1_ele_root    [in] The root element of the first page to start comparison.
	 * @param page2             [in] The second page containing elements to compare.
	 * @param page2_ele_root    [in] The root element of the second page to start comparison.
	 * @param settings          [in] Settings for text comparison.
	 * @param enumerator        [out] Pointer to store the enumerator for iterating through differences.
	 * @param error             [out] Contains any error text.
	 * @return                  IGR_OK if successful, an error code otherwise.
	 */
	IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Elements(IGR_HPAGE page1
		, const struct IGR_Page_Element* page1_ele_root
		, IGR_HPAGE page2
		, const struct IGR_Page_Element* page2_ele_root
		, const struct IGR_Text_Compare_Settings* settings
		, IGR_HTEXTCOMPARE* enumerator
		, Error_Control_Block* error);

	/**
	 * Closes the text comparison enumerator and frees associated resources.
	 *
	 * @param enumerator    [in] The text comparison enumerator to close.
	 * @param error         [out] Contains any error text.
	 * @return              IGR_OK if successful, an error code otherwise.
	 */
	IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Close(IGR_HTEXTCOMPARE enumerator, Error_Control_Block* error);

	/**
	 * Resets the text comparison enumerator to its initial state.
	 *
	 * @param enumerator    [in] The text comparison enumerator to reset.
	 * @param error         [out] Contains any error text.
	 * @return              IGR_OK if successful, an error code otherwise.
	 */
	IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Reset(IGR_HTEXTCOMPARE enumerator, Error_Control_Block* error);

	/**
	 * Retrieves the next difference from the text comparison enumerator.
	 *
	 * @param enumerator    [in] The text comparison enumerator.
	 * @param result        [out] Pointer to store the next difference.
	 * @param error         [out] Contains any error text.
	 * @return              IGR_OK if successful, an error code otherwise.
	 */
	IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Next(IGR_HTEXTCOMPARE enumerator, struct IGR_Compare_Documents_Difference* result, Error_Control_Block* error);

	/**
	 * Disposes of a difference result returned by the text comparison enumerator.
	 *
	 * @param result        [in] The difference result to dispose of.
	 * @param error         [out] Contains any error text.
	 * @return              IGR_OK if successful, an error code otherwise.
	 */
	IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Difference_Dispose(struct IGR_Compare_Documents_Difference* result, Error_Control_Block* error);

#ifdef __cplusplus
};
#endif

#endif // PERCEPTIVE_DOCFILTERS_H_DEF
