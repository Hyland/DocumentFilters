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

#ifndef HYLAND_DOCFILTERS_H_DEF
#define HYLAND_DOCFILTERS_H_DEF

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ****************************************************************************
// * Platform and architecture specific definitions
// ****************************************************************************
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

// ****************************************************************************
// * Simple types definitions
// ****************************************************************************
#ifdef _WIN32
	typedef wchar_t            IGR_UCS2;
	typedef long               IGR_LONG;
	typedef unsigned long      IGR_ULONG;
#else
	typedef uint16_t           IGR_UCS2;
	typedef int32_t            IGR_LONG;
	typedef uint32_t           IGR_ULONG;
#endif

typedef unsigned char      IGR_BYTE;
typedef short int          IGR_SHORT;
typedef unsigned short int IGR_USHORT;
typedef int64_t            IGR_LONGLONG;
typedef uint64_t           IGR_ULONGLONG;
typedef void*              IGR_HANDLE;
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

/// @brief Page handle type.
typedef IGR_LONG HPAGE;

/// @brief Canvas handle type for rendering page output.
typedef IGR_LONG HCANVAS;

/// @brief Alias for \ref HPAGE page handle type.
typedef HPAGE IGR_HPAGE;

/// @brief Alias for \ref HCANVAS canvas handle type.
typedef HCANVAS IGR_HCANVAS;

/// @brief Document handle type.
typedef IGR_LONG IGR_HDOC;

// ****************************************************************************
// * Constants
// ****************************************************************************

// ****************************************************************************
// * IGR_RETURN_CODE type and constants

/// @brief Return type for API functions.
typedef IGR_LONG IGR_RETURN_CODE;

/// @brief Operation completed successfully.
#define IGR_OK                          0

/// @brief Error opening the file/resource.
#define IGR_E_OPEN_ERROR                1

/// @brief Wrong type was used for operation.
#define IGR_E_WRONG_TYPE                2

/// @brief Resource is already in use.
#define IGR_E_IN_USE                    3

/// @brief Resource is not readable.
#define IGR_E_NOT_READABLE              4

/// @brief Password required or incorrect password provided.
#define IGR_E_PASSWORD                  5

/// @brief Resource was not found.
#define IGR_E_NOT_FOUND                 10

/// @brief Error writing to file/resource.
#define IGR_E_WRITE_ERROR               11

/// @brief Operation not valid for this class/type.
#define IGR_E_NOT_VALID_FOR_THIS_CLASS  12

/// @brief General error occurred.
#define IGR_E_ERROR                     13

/// @brief Invalid handle was provided.
#define IGR_E_INVALID_HANDLE            14

/// @brief Invalid pointer was provided.
#define IGR_E_INVALID_POINTER           15

/// @brief Invalid parameter was provided.
#define IGR_E_INVALID_PARAMETER         16

/// @brief File is corrupt or truncated.
#define IGR_E_FILE_CORRUPT              17

/// @brief No more data available.
#define IGR_NO_MORE                     19

/// @brief Out of memory error.
#define IGR_E_OUT_OF_MEMORY             20

/// @brief Severe error occurred.
#define IGR_E_BAD_ERROR                 21

/// @brief Operation called from wrong thread.
#define IGR_E_WRONG_THREAD              22

/// @brief Too many handles are open.
#define IGR_E_TOO_MANY_HANDLES          23

/// @brief Required fonts were not found.
#define IGR_E_FONTS_NOT_FOUND           24

/// @brief Version mismatch between components.
#define IGR_E_VERSION_MISMATCH          25

/// @brief Requested action is not supported.
#define IGR_E_ACTION_NOT_SUPPORTED      26

/// @brief Operation was cancelled.
#define IGR_CANCELLED                   27



/// @brief Length of an MD5 hex string including null terminator.
#define PERCEPTIVE_SZ_MD5_HEX_LEN       33

/// @brief Length of a SHA1 hex string including null terminator.
#define PERCEPTIVE_SZ_SHA1_HEX_LEN      41

/// @brief Legacy multi-function code to set code page.
#define IGR_Multi_Set_Code_Page         1

/// @brief Legacy multi-function code to set temporary path.
#define IGR_Multi_Set_Temp_Path         186

// ****************************************************************************
// * IGR_ACTION_GET_STREAM type and constants

/// @brief IGR_ACTION_GET_STREAM_PART is used to indicate an action to retrieve part of
/// a stream. Used with \ref IGR_T_ACTION_GET_STREAM_PART in callbacks.
#define IGR_ACTION_GET_STREAM_PART  10

// ****************************************************************************
// * IGR_ANNOTATION type and constants

/// @brief Type definition for annotation types.
#define IGR_ANNOTATION_TYPE IGR_LONG

/// @brief Unknown annotation type.
#define IGR_ANNOTATION_UNKNOWN 0

/// @brief Text annotation.
#define IGR_ANNOTATION_TEXT 1

/// @brief Link annotation.
#define IGR_ANNOTATION_LINK 2

/// @brief Free text annotation.
#define IGR_ANNOTATION_FREETEXT 3

/// @brief Line annotation.
#define IGR_ANNOTATION_LINE 4

/// @brief Square/rectangle annotation.
#define IGR_ANNOTATION_SQUARE 5

/// @brief Circle/ellipse annotation.
#define IGR_ANNOTATION_CIRCLE 6

/// @brief Polygon annotation.
#define IGR_ANNOTATION_POLYGON 7

/// @brief Polyline annotation.
#define IGR_ANNOTATION_POLYLINE 8

/// @brief Highlight annotation.
#define IGR_ANNOTATION_HIGHLIGHT 9

/// @brief Underline annotation.
#define IGR_ANNOTATION_UNDERLINE 10

/// @brief Squiggly underline annotation.
#define IGR_ANNOTATION_SQUIGGLE 11

/// @brief Strikeout annotation.
#define IGR_ANNOTATION_STRIKEOUT 12

/// @brief Stamp annotation.
#define IGR_ANNOTATION_STAMP 13

/// @brief Caret annotation.
#define IGR_ANNOTATION_CARET 14

/// @brief Ink annotation.
#define IGR_ANNOTATION_INK 15

/// @brief Popup annotation.
#define IGR_ANNOTATION_POPUP 16

/// @brief File attachment annotation.
#define IGR_ANNOTATION_FILEATTACHMENT 17

/// @brief Sound annotation.
#define IGR_ANNOTATION_SOUND 18

/// @brief Movie/video annotation.
#define IGR_ANNOTATION_MOVIE 19

/// @brief Form widget annotation.
#define IGR_ANNOTATION_WIDGET 20

/// @brief Screen annotation.
#define IGR_ANNOTATION_SCREEN 21

/// @brief Printer mark annotation.
#define IGR_ANNOTATION_PRINTERMARK 22

/// @brief Trap network annotation.
#define IGR_ANNOTATION_TRAPNET 23

/// @brief Watermark annotation.
#define IGR_ANNOTATION_WATERMARK 24

/// @brief 3D annotation.
#define IGR_ANNOTATION_3D 25

/// @brief Barcode annotation.
#define IGR_ANNOTATION_BARCODE 26

/// @brief Named destination annotation.
#define IGR_ANNOTATION_NAMED_DESTINATION 27

/// @brief Type definition for annotation flags.
#define IGR_ANNOTATION_FLAGS_TYPE IGR_LONG

/// @brief Annotation is invisible.
#define IGR_ANNOTATION_FLAGS_INVISIBLE (1 << 0)

/// @brief Annotation is hidden.
#define IGR_ANNOTATION_FLAGS_HIDDEN (1 << 1)

/// @brief Annotation should be printed.
#define IGR_ANNOTATION_FLAGS_PRINT (1 << 2)

/// @brief Annotation should not be zoomed.
#define IGR_ANNOTATION_FLAGS_NOZOOM (1 << 3)

/// @brief Annotation should not be rotated.
#define IGR_ANNOTATION_FLAGS_NOROTATE (1 << 4)

/// @brief Annotation should not be viewed.
#define IGR_ANNOTATION_FLAGS_NOVIEW (1 << 5)

/// @brief Annotation is read-only.
#define IGR_ANNOTATION_FLAGS_READONLY (1 << 6)

/// @brief Annotation is locked.
#define IGR_ANNOTATION_FLAGS_LOCKED (1 << 7)

/// @brief Toggle no view state.
#define IGR_ANNOTATION_FLAGS_TOGGLENOVIEW (1 << 8)

/// @brief Annotation contents are locked.
#define IGR_ANNOTATION_FLAGS_LOCKEDCONTENTS (1 << 9)

// ****************************************************************************
// * IGR_BOOKMARK_ACTION type and constants

/// @brief Type definition for bookmark action types.
#define IGR_BOOKMARK_ACTION_TYPE IGR_LONG

/// @brief Goto action for bookmark.
#define IGR_BOOKMARK_ACTION_GOTO 0

/// @brief Link action for bookmark.
#define IGR_BOOKMARK_ACTION_LINK 1

// ****************************************************************************
// * IGR_BOOKMARK_FIT type and constants

/// @brief Type definition for bookmark fit modes.
#define IGR_BOOKMARK_FIT_TYPE IGR_LONG

/// @brief No fit mode specified.
#define IGR_BOOKMARK_FIT_NONE    0

/// @brief Fit both width and height.
#define IGR_BOOKMARK_FIT_BOTH    1

/// @brief Fit width only.
#define IGR_BOOKMARK_FIT_WIDTH   2

/// @brief Fit height only.
#define IGR_BOOKMARK_FIT_HEIGHT  3

/// @brief Fit to rectangle.
#define IGR_BOOKMARK_FIT_RECT    4

// ****************************************************************************
// * IGR_BRUSH type and constants

/// @brief Canvas brush type definition.
#define IGR_BRUSH_TYPE IGR_LONG

/// @brief No brush style.
#define IGR_BRUSH_NONE            0x00

/// @brief Solid brush style.
#define IGR_BRUSH_SOLID           0x01

// ****************************************************************************
// * IGR_CHAR_TYPE and constants

/// @brief Type definition for character type, using UCS2 encoding.
#define IGR_CHAR_TYPE IGR_UCS2

/// @brief Special character code indicating a soft space character.
#define IGR_CHAR_SOFT_SPACE             1

/// @brief Special character code for normal text style.
#define IGR_CHAR_STYLE_NORMAL           2

/// @brief Special character code for bold text style.
#define IGR_CHAR_STYLE_BOLD             3

/// @brief Special character code for italics text style.
#define IGR_CHAR_STYLE_ITALICS          4

/// @brief Special character code for underlined text style.
#define IGR_CHAR_STYLE_UNDERLINED       5

/// @brief Special character code for tab character.
#define IGR_CHAR_TAB                    9

/// @brief Special character code for page break character.
#define IGR_CHAR_PAGE_BREAK             12

/// @brief Special character code for line break character.
#define IGR_CHAR_LINE_BREAK             13

/// @brief Special character code for paragraph break character.
#define IGR_CHAR_PARA_BREAK             14

/// @brief Special character code marking start of metadata.
#define IGR_CHAR_START_META             15

/// @brief Special character code marking end of metadata.
#define IGR_CHAR_END_META               16

// ****************************************************************************
// * IGR_COLOR type, constants and macros

/// @brief Type definition for color values.
#define IGR_COLOR_TYPE IGR_ULONG

/// @brief Creates an RGB color value.
///
/// @param R Red color component (0-255).
/// @param G Green color component (0-255).
/// @param B Blue color component (0-255).
///
/// @return Combined RGB color value
#define MAKE_COLOR(R, G, B)       ((int) R | ((int) G << 8) | ((int) B << 16))

/// @brief Creates an ARGB color value with alpha.
///
/// @param A Alpha transparency component (0-255).
/// @param R Red color component (0-255).
/// @param G Green color component (0-255).
/// @param B Blue color component (0-255).
///
/// @return Combined ARGB color value
#define IGR_MAKE_COLOR_WITH_ALPHA(A, R, G, B) (((int) A << 24) | ((int) R << 16) | ((int) G << 8) | ((int) B << 0))

/// @brief Creates an RGB color value with full opacity.
///
/// @param R Red color component (0-255).
/// @param G Green color component (0-255).
/// @param B Blue color component (0-255).
///
/// @return Combined RGB color value with alpha=255
#define IGR_MAKE_COLOR(R, G, B) IGR_MAKE_COLOR_WITH_ALPHA(0xff, R, G, B)


// ****************************************************************************
// * IGR_COMPARE_DOCUMENTS_DIFFERENCE type and constants

/// @brief Type definition for difference types between compared documents.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_TYPE            IGR_ULONG

/// @brief No difference between compared documents.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL           0

/// @brief Content was inserted in the revised document.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT          1

/// @brief Content was deleted from the original document.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE          2

/// @brief Formatting changes between documents.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING      3

/// @brief More differences are available in next batch.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH      0xFF


// ****************************************************************************
// * IGR_COMPARE_DOCUMENTS_FLAGS type and constants

/// @brief Type definition for document comparison flags.
#define IGR_COMPARE_DOCUMENTS_FLAGS_TYPE                 IGR_ULONG

/// @brief Include equal/matching content in results.
#define IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS               0x1

/// @brief Detect moved content between documents.
#define IGR_COMPARE_DOCUMENTS_FLAGS_MOVES                0x10

/// @brief Detect formatting changes between documents.
#define IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING           0x20

/// @brief Ignore comments when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS          0x40

/// @brief Ignore case when comparing text.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE              0x80

/// @brief Ignore whitespace when comparing text.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE        0x100

/// @brief Ignore punctuation when comparing text.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION       0x200

/// @brief Ignore table content when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES            0x400

/// @brief Ignore headers when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS           0x800

/// @brief Ignore footers when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS           0x1000

/// @brief Ignore footnotes when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES         0x2000

/// @brief Ignore textbox content when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES         0x4000

/// @brief Ignore field content when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS            0x8000

/// @brief Ignore both headers and footers when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS_FOOTERS (IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS | IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS)

/// @brief Type definition for document comparison methods.
#define IGR_COMPARE_DOCUMENTS_COMPARE_TYPE               IGR_ULONG

/// @brief Compare documents by words.
#define IGR_COMPARE_DOCUMENTS_COMPARE_WORDS              0x0

/// @brief Type definition for difference source types.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_TYPE     IGR_ULONG

/// @brief Difference found in original document.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_ORIGINAL 0x0

/// @brief Difference found in revised document - Legacy spelling.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_ORGINAL 0x0 

/// @brief Difference found in revised document.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_REVISED  0x1

/// @brief Difference found in both documents.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_BOTH     0x2


// ****************************************************************************
// * IGR_DEVICE_TYPE type and constants

/// @brief Canvas output device type definition.
#define IGR_DEVICE_TYPE IGR_LONG

/// @brief PNG image output device.
#define IGR_DEVICE_IMAGE_PNG      0

/// @brief JPEG image output device.
#define IGR_DEVICE_IMAGE_JPG      1

/// @brief PDF document output device.
#define IGR_DEVICE_IMAGE_PDF      2

/// @brief TIFF image output device.
#define IGR_DEVICE_IMAGE_TIF      3

/// @brief BMP image output device.
#define IGR_DEVICE_IMAGE_BMP      4

/// @brief XML output device.
#define IGR_DEVICE_XML            5

/// @brief HTML output device.
#define IGR_DEVICE_HTML           6

/// @brief PBM image output device.
#define IGR_DEVICE_IMAGE_PBM      7

/// @brief PGM image output device.
#define IGR_DEVICE_IMAGE_PGM      8

/// @brief PPM image output device.
#define IGR_DEVICE_IMAGE_PPM      9

/// @brief WebP image output device.
#define IGR_DEVICE_IMAGE_WEBP     10

/// @brief XPS document output device.
#define IGR_DEVICE_IMAGE_XPS      11

/// @brief SVG image output device.
#define IGR_DEVICE_IMAGE_SVG      12

/// @brief EPS image output device.
#define IGR_DEVICE_IMAGE_EPS      13

/// @brief PostScript output device.
#define IGR_DEVICE_IMAGE_PS       14

/// @brief Web-safe image output device.
#define IGR_DEVICE_IMAGE_WEBSAFE  15

/// @brief TGA image output device.
#define IGR_DEVICE_IMAGE_TGA      16

/// @brief BRK image output device.
#define IGR_DEVICE_IMAGE_BRK      17

/// @brief PCX image output device.
#define IGR_DEVICE_IMAGE_PCX      18

/// @brief DCX image output device.
#define IGR_DEVICE_IMAGE_DCX      19

/// @brief JPEG 2000 image output device.
#define IGR_DEVICE_IMAGE_JPEG2000 20

/// @brief GIF image output device.
#define IGR_DEVICE_IMAGE_GIF      21

/// @brief JSON output device.
#define IGR_DEVICE_JSON           22

/// @brief Markdown output device.
#define IGR_DEVICE_MARKDOWN       23

/// @brief Alias for PDF output device.
#define IGR_DEVICE_PDF            IGR_DEVICE_IMAGE_PDF


// ****************************************************************************
// * IGR_FILE_SUPPORTS type and constants

/// @brief Type definition for indicating file support type compatibility.
#define IGR_FILE_SUPPORTS_TYPE IGR_LONG

/// @brief Document supports text extraction capability.
#define IGR_FILE_SUPPORTS_TEXT          1

/// @brief Document supports sub-file enumeration capability.
#define IGR_FILE_SUPPORTS_SUBFILES      2

/// @brief Document supports HD HTML conversion capability.
#define IGR_FILE_SUPPORTS_HDHTML        4

/// @brief Document is corrupt or truncated.
#define IGR_FILE_CORRUPT_OR_TRUNCATED	0x2000


// ****************************************************************************
// * IGR_FILETYPE type and constants

/// @brief Unknown file type.
#define IGR_FILETYPE_UNKNOWN              0

/// @brief Plain text document.
#define IGR_FILETYPE_TEXT                 100

/// @brief Text document with markup (e.g. HTML, XML).
#define IGR_FILETYPE_TEXT_MARKUP          101

/// @brief Word processor document.
#define IGR_FILETYPE_WORD_PROCESSOR       200

/// @brief Spreadsheet document.
#define IGR_FILETYPE_SPREADSHEET          300

/// @brief Presentation document.
#define IGR_FILETYPE_PRESENTATION         400

/// @brief Other office document type.
#define IGR_FILETYPE_OFFICE_OTHER         500

/// @brief Computer aided design (CAD) document.
#define IGR_FILETYPE_CAD                  600

/// @brief Archive file (compressed container).
#define IGR_FILETYPE_ARCHIVE              700

/// @brief System/configuration file.
#define IGR_FILETYPE_SYSTEM               800

/// @brief Database file.
#define IGR_FILETYPE_DATABASE             900

/// @brief Generic graphic file.
#define IGR_FILETYPE_GRAPHIC              1000

/// @brief Raster graphic file (bitmap).
#define IGR_FILETYPE_GRAPHIC_RASTER       1001

/// @brief Vector graphic file.
#define IGR_FILETYPE_GRAPHIC_VECTOR       1002

/// @brief Generic email file.
#define IGR_FILETYPE_EMAIL                1100

/// @brief Individual email message.
#define IGR_FILETYPE_EMAIL_MESSAGE        1101

/// @brief Email container/folder.
#define IGR_FILETYPE_EMAIL_CONTAINER      1102

/// @brief Email contact.
#define IGR_FILETYPE_EMAIL_CONTACT        1103

/// @brief Email calendar item.
#define IGR_FILETYPE_EMAIL_CALENDAR       1104

/// @brief Other email-related file.
#define IGR_FILETYPE_EMAIL_OTHER          1105

/// @brief Generic multimedia file.
#define IGR_FILETYPE_MULTIMEDIA           1200

/// @brief Video file.
#define IGR_FILETYPE_MULTIMEDIA_VIDEO     1201

/// @brief Audio file.
#define IGR_FILETYPE_MULTIMEDIA_AUDIO     1202

/// @brief Desktop publishing file.
#define IGR_FILETYPE_PUBLISHING           1300

/// @brief Application data file.
#define IGR_FILETYPE_APPDATA              2000


// ****************************************************************************
// * IGR_HYPERLINK_ACTION type and constants

/// @brief Type definition for hyperlink action types.
#define IGR_HYPERLINK_ACTION_TYPE IGR_LONG

/// @brief Action type for navigation within document.
#define IGR_HYPERLINK_ACTION_GOTO 0x1

/// @brief Action type for external/URI links. Destination is either a qualified
/// or relative link.
#define IGR_HYPERLINK_ACTION_URI 0x2

/// @brief Type definition for hyperlink flags.
#define IGR_HYPERLINK_FLAGS_TYPE IGR_LONG

/// @brief Change left coordinate when navigating.
#define IGR_HYPERLINK_FLAGS_CHANGE_LEFT 0x1

/// @brief Change top coordinate when navigating.
#define IGR_HYPERLINK_FLAGS_CHANGE_TOP 0x2

/// @brief Change zoom level when navigating.
#define IGR_HYPERLINK_FLAGS_CHANGE_ZOOM 0x4


// ****************************************************************************
// * IGR_HYPERLINK_FIT type and constants

/// @brief Type definition for hyperlink fit modes.
#define IGR_HYPERLINK_FIT_TYPE IGR_LONG

/// @brief Fit at specific coordinates with zoom.
#define IGR_HYPERLINK_FIT_XYZ 0

/// @brief Fit page to window.
#define IGR_HYPERLINK_FIT_FIT 1

/// @brief Fit page horizontally.
#define IGR_HYPERLINK_FIT_FITH 2

/// @brief Fit page vertically.
#define IGR_HYPERLINK_FIT_FITV 3

/// @brief Fit page to rectangle.
#define IGR_HYPERLINK_FIT_FITR 4

/// @brief Fit page bounds.
#define IGR_HYPERLINK_FIT_FITB 5

/// @brief Fit page bounds horizontally.
#define IGR_HYPERLINK_FIT_FITBH 6

/// @brief Fit page bounds vertically.
#define IGR_HYPERLINK_FIT_FITBV 7


// ****************************************************************************
// * IGR_HYPERLINK_GET_VALUE type and constants

/// @brief Type definition for hyperlink value types.
#define IGR_HYPERLINK_GET_VALUE_TYPE IGR_LONG

/// @brief Get reference value.
#define IGR_HYPERLINK_GET_VALUE_REF 0

/// @brief Get URI value.
#define IGR_HYPERLINK_GET_VALUE_URI 1

/// @brief Type definition for page pixel flags.
#define IGR_GET_PAGE_PIXELS_FLAGS_TYPE IGR_LONG

/// @brief Buffer has been allocated.
#define IGR_GET_PAGE_PIXELS_FLAGS_BUFFER_ALLOCATED 0x00010000


// ****************************************************************************
// * IGR_LOG_LEVEL type and constants

/// @brief Type definition for log level values.
#define IGR_LOG_LEVEL_TYPE IGR_ULONG

/// @brief No logging.
#define IGR_LOG_LEVEL_NONE 0

/// @brief Critical error logging level.
#define IGR_LOG_LEVEL_CRITICAL 1

/// @brief Error logging level.
#define IGR_LOG_LEVEL_ERROR 2

/// @brief Warning logging level.
#define IGR_LOG_LEVEL_WARNING 3

/// @brief Information logging level.
#define IGR_LOG_LEVEL_INFORMATION 4

/// @brief Debug logging level.
#define IGR_LOG_LEVEL_DEBUG 5

/// @brief Trace logging level.
#define IGR_LOG_LEVEL_TRACE 6


// ****************************************************************************
// * IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS type and constants

/// @brief Type definition for defining flags used in IGR_Make_Stream_From_Functions.
#define IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_TYPE IGR_LONG

/// @brief No flags set for stream functions.
#define IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_NONE      0x0

/// @brief Use cdecl calling convention for stream functions.
#define IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_USECDECL  0x1


// ****************************************************************************
// * IGR_OPEN_BITMAP_FLAGS type and constants

/// @brief Flags type definition for bitmap processing options.
#define IGR_OPEN_BITMAP_FLAGS_TYPE IGR_LONG

/// @brief Flag indicating minimum value represents black.
#define IGR_OPEN_BITMAP_FLAGS_MIN_IS_BLACK    0x01

/// @brief Flag indicating bitmap data is stored bottom-up.
#define IGR_OPEN_BITMAP_FLAGS_BOTTOM_UP       0x02


// ****************************************************************************
// * IGR_OPEN_BITMAP_ORIENTATION type and constants

/// @brief Type definition for bitmap orientation specifications.
#define IGR_OPEN_BITMAP_ORIENTATION_TYPE IGR_LONG

/// @brief Image oriented top-left.
#define IGR_OPEN_BITMAP_ORIENTATION_TOPLEFT   1

/// @brief Image oriented top-right.
#define IGR_OPEN_BITMAP_ORIENTATION_TOPRIGHT  2

/// @brief Image oriented bottom-right.
#define IGR_OPEN_BITMAP_ORIENTATION_BOTRIGHT  3

/// @brief Image oriented bottom-left.
#define IGR_OPEN_BITMAP_ORIENTATION_BOTLEFT   4

/// @brief Image oriented left-top.
#define IGR_OPEN_BITMAP_ORIENTATION_LEFTTOP   5

/// @brief Image oriented right-top.
#define IGR_OPEN_BITMAP_ORIENTATION_RIGHTTOP  6

/// @brief Image oriented right-bottom.
#define IGR_OPEN_BITMAP_ORIENTATION_RIGHTBOT  7

/// @brief Image oriented left-bottom.
#define IGR_OPEN_BITMAP_ORIENTATION_LEFTBOT   8


// ****************************************************************************
// * IGR_OPEN_BITMAP_PIXEL type and constants

/// @brief Type definition for bitmap pixel format specifications.
#define IGR_OPEN_BITMAP_PIXEL_TYPE IGR_LONG

/// @brief Automatically detect pixel format.
#define IGR_OPEN_BITMAP_PIXEL_AUTO            0

/// @brief 1 bit per pixel indexed color format.
#define IGR_OPEN_BITMAP_PIXEL_1BPP_INDEXED    1

/// @brief 4 bits per pixel indexed color format.
#define IGR_OPEN_BITMAP_PIXEL_4BPP_INDEXED    2

/// @brief 8 bits per pixel indexed color format.
#define IGR_OPEN_BITMAP_PIXEL_8BPP_INDEXED    3

/// @brief 16 bits per pixel RGB color format (5-6-5).
#define IGR_OPEN_BITMAP_PIXEL_16BPP_565_RGB   4

/// @brief 16 bits per pixel BGR color format (5-6-5).
#define IGR_OPEN_BITMAP_PIXEL_16BPP_565_BGR   5

/// @brief 16 bits per pixel ARGB color format (4-4-4-4).
#define IGR_OPEN_BITMAP_PIXEL_16BPP_4444_ARGB 6

/// @brief 16 bits per pixel BGRA color format (4-4-4-4).
#define IGR_OPEN_BITMAP_PIXEL_16BPP_4444_BGRA 7

/// @brief 24 bits per pixel RGB color format (8-8-8).
#define IGR_OPEN_BITMAP_PIXEL_24BPP_888_RGB   8

/// @brief 24 bits per pixel BGR color format (8-8-8).
#define IGR_OPEN_BITMAP_PIXEL_24BPP_888_BGR   9

/// @brief 32 bits per pixel ARGB color format (8-8-8-8).
#define IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ARGB 10

/// @brief 32 bits per pixel BGRA color format (8-8-8-8).
#define IGR_OPEN_BITMAP_PIXEL_32BPP_8888_BGRA 11

/// @brief 32 bits per pixel RGBA color format (8-8-8-8).
#define IGR_OPEN_BITMAP_PIXEL_32BPP_8888_RGBA 12

/// @brief 32 bits per pixel ABGR color format (8-8-8-8).
#define IGR_OPEN_BITMAP_PIXEL_32BPP_8888_ABGR 13


// ****************************************************************************
// * IGR_OPEN_CALLBACK_ACTION type and constants

/// @brief Type definition for callback actions used in document processing.
#define IGR_OPEN_CALLBACK_ACTION IGR_LONG

/// @brief Callback action for heartbeat events.
#define IGR_OPEN_CALLBACK_ACTION_HEARTBEAT 0

/// @brief Callback action for password requests.
#define IGR_OPEN_CALLBACK_ACTION_PASSWORD 1

/// @brief Callback action for text localization.
#define IGR_OPEN_CALLBACK_ACTION_LOCALIZE 2

/// @brief Callback action for getting log level.
#define IGR_OPEN_CALLBACK_ACTION_LOG_LEVEL 3

/// @brief Callback action for logging messages.
#define IGR_OPEN_CALLBACK_ACTION_LOG_MESSAGE 4

/// @brief Callback action for approving external resources.
#define IGR_OPEN_CALLBACK_ACTION_APPROVE_EXTERNAL_RESOURCE 5

/// @brief Callback action for getting resource streams.
#define IGR_OPEN_CALLBACK_ACTION_GET_RESOURCE_STREAM 6

/// @brief Callback action for OCR image processing.
#define IGR_OPEN_CALLBACK_ACTION_OCR_IMAGE 7


// ****************************************************************************
// * IGR_OPEN_FLAGS type and constants

/// @brief Type definition for open flags.
#define IGR_OPEN_FLAGS_TYPE IGR_LONG

/// @brief Extract only the body content.
#define IGR_BODY_ONLY                   0

/// @brief Extract only the metadata.
#define IGR_META_ONLY                   1

/// @brief Extract both body content and metadata.
#define IGR_BODY_AND_META               2

/// @brief Output format: plain text.
#define IGR_FORMAT_TEXT                 0x00010000L

/// @brief Output format: HTML.
#define IGR_FORMAT_HTML                 0x00020000L

/// @brief Output format: XML.
#define IGR_FORMAT_XML                  0x00030000L

/// @brief Output format: HD HTML if possible, fallback to regular HTML if not.
#define IGR_FORMAT_HDHTML_IFYOUCAN      0x00040000L

/// @brief Output format: image.
#define IGR_FORMAT_IMAGE                0x00050000L

/// @brief Output format: HTML with no fallback.
#define IGR_FORMAT_HTML_NO_FALLBACK     0x00060000L

/// @brief Output format: JSON.
#define IGR_FORMAT_JSON                 0x00070000L

/// @brief Output format: Markdown.
#define IGR_FORMAT_MARKDOWN             0x00080000L


// ****************************************************************************
// * IGR_OPEN_FROM type and constants

/// @brief Type definition for document source type used with IGR_Open_Ex.
#define IGR_OPEN_FROM IGR_LONG

/// @brief Open from a UTF-16 filename.
#define IGR_OPEN_FROM_FILENAME_UTF16 0

/// @brief Open from a stream object.
#define IGR_OPEN_FROM_STREAM 1


// ****************************************************************************
// * IGR_PAGE_ELEMENT type and constants

/// @brief Type definition for page element types.
#define IGR_PAGE_ELEMENT_TYPE IGR_ULONG

/// @brief Page element type for a full page.
#define IGR_PAGE_ELEMENT_TYPE_PAGE          1

/// @brief Page element type for a section.
#define IGR_PAGE_ELEMENT_TYPE_SECTION       2

/// @brief Page element type for a column.
#define IGR_PAGE_ELEMENT_TYPE_COLUMN        3

/// @brief Page element type for a paragraph.
#define IGR_PAGE_ELEMENT_TYPE_PARAGRAPH     4

/// @brief Page element type for a line of text.
#define IGR_PAGE_ELEMENT_TYPE_LINE          5

/// @brief Page element type for a text run.
#define IGR_PAGE_ELEMENT_TYPE_RUN           6

/// @brief Page element type for a word.
#define IGR_PAGE_ELEMENT_TYPE_WORD          7

/// @brief Page element type for a header.
#define IGR_PAGE_ELEMENT_TYPE_HEADER        8

/// @brief Page element type for a footer.
#define IGR_PAGE_ELEMENT_TYPE_FOOTER        9

/// @brief Page element type for an image.
#define IGR_PAGE_ELEMENT_TYPE_IMAGE         10

/// @brief Page element type for a table.
#define IGR_PAGE_ELEMENT_TYPE_TABLE         11

/// @brief Page element type for a table row.
#define IGR_PAGE_ELEMENT_TYPE_TABLE_ROW     12

/// @brief Page element type for a table cell.
#define IGR_PAGE_ELEMENT_TYPE_TABLE_CELL    13

/// @brief Page element type for a form.
#define IGR_PAGE_ELEMENT_TYPE_FORM          14

/// @brief Page element type for a form element.
#define IGR_PAGE_ELEMENT_TYPE_FORM_ELEMENT  15

/// @brief Page element type for floating content.
#define IGR_PAGE_ELEMENT_TYPE_FLOAT         16

/// @brief Page element type for graphics.
#define IGR_PAGE_ELEMENT_TYPE_GRAPHIC       17

/// @brief Page element type for text boxes.
#define IGR_PAGE_ELEMENT_TYPE_TEXT_BOX      18

/// @brief Page element type for notes.
#define IGR_PAGE_ELEMENT_TYPE_NOTES         19


// ****************************************************************************
// * IGR_PAGE_ELEMENT_FLAG type and constants

/// @brief Type definition for page element flags.
#define IGR_PAGE_ELEMENT_FLAG_TYPE IGR_ULONG

/// @brief Flag indicating element contains dynamic content.
#define IGR_PAGE_ELEMENT_FLAG_DYNAMIC_CONTENT  0x01

/// @brief Flag indicating element contains injected content.
#define IGR_PAGE_ELEMENT_FLAG_INJECTED_CONTENT 0x02


// ****************************************************************************
// * IGR_PAGE_FORM_ELEMENT type and constants

/// @brief Type definition for form element types.
#define IGR_PAGE_FORM_ELEMENT_TYPE IGR_LONG

/// @brief Button form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_BUTTON             0

/// @brief Radio button form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_RADIOBUTTON        1

/// @brief Checkbox form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_CHECKBOX           2

/// @brief File select form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_FILESELECT         3

/// @brief Multiline text form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_MULTILINE_TEXT     4

/// @brief Single line text form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_SINGLELINE_TEXT    5

/// @brief Combobox form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_COMBOBOX           6

/// @brief Listbox form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_LISTBOX            7

/// @brief Signature form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_SIGNATURE          8

/// @brief Password form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_PASSWORD           9


// ****************************************************************************
// * IGR_PAGE_FORM_ELEMENT_FLAG type and constants

/// @brief Type definition for form element flags.
#define IGR_PAGE_FORM_ELEMENT_FLAG_TYPE IGR_LONG

/// @brief Form element is read-only.
#define IGR_PAGE_FORM_ELEMENT_FLAG_READONLY          (1 << 0)

/// @brief Form element is required.
#define IGR_PAGE_FORM_ELEMENT_FLAG_REQUIRED          (1 << 1)

/// @brief Form element has comb formatting.
#define IGR_PAGE_FORM_ELEMENT_FLAG_COMB              (1 << 2)

/// @brief Form element is checked.
#define IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED           (1 << 3)


// ****************************************************************************
// * IGR_PAGE_FORM_ELEMENT_GET type and constants

/// @brief Type definition for form element property accessors.
#define IGR_PAGE_FORM_ELEMENT_GET_TYPE IGR_LONG

/// @brief Get form element name.
#define IGR_PAGE_FORM_ELEMENT_GET_NAME               0

/// @brief Get form element value.
#define IGR_PAGE_FORM_ELEMENT_GET_VALUE              1

/// @brief Get form element caption.
#define IGR_PAGE_FORM_ELEMENT_GET_CAPTION            2

/// @brief Get form element action.
#define IGR_PAGE_FORM_ELEMENT_GET_ACTION             3

/// @brief Get form element action destination.
#define IGR_PAGE_FORM_ELEMENT_GET_ACTION_DEST        4

/// @brief Get form element selected state.
#define IGR_PAGE_FORM_ELEMENT_GET_SELECTED           5

/// @brief Get form element font name.
#define IGR_PAGE_FORM_ELEMENT_GET_FONT_NAME          6


// ****************************************************************************
// * IGR_PEN type and constants

/// @brief Canvas pen type definition.
#define IGR_PEN_TYPE IGR_LONG

/// @brief No pen style.
#define IGR_PEN_NONE              0x00

/// @brief Solid pen style.
#define IGR_PEN_SOLID             0x01


// ****************************************************************************
// * IGR_REDACT_FLAGS type and constants

/// @brief Type definition for redaction flags.
#define IGR_REDACT_FLAGS_TYPE IGR_ULONG

/// @brief Do not redact text during redaction.
#define IGR_REDACT_FLAGS_DO_NOT_REDACT_TEXT 0x1

/// @brief Do not redact images during redaction.
#define IGR_REDACT_FLAGS_DO_NOT_REDACT_IMAGES 0x2

/// @brief Page should always be rasterized and then redacted pixels replaced 
/// with the specified color. If this flag is set and redaction cannot be 
/// performed an error will occur.
#define IGR_REDACT_FLAGS_ALWAYS_RASTER 0x4

/// @brief Page should never be rasterized. If this flag is set and redaction 
/// cannot be performed an error will occur.
#define IGR_REDACT_FLAGS_NEVER_RASTER 0x8

/// @brief Do not draw redaction regions on the canvas, redacted images will 
/// still use the redaction regions and colors
#define IGR_REDACT_FLAGS_DO_NOT_DRAW_REGION 0x10


// ****************************************************************************
// * IGR_SUBFILE_INFO_FLAG type and constants

/// @brief Type definition for subfile info flags.
#define IGR_SUBFILE_INFO_FLAG_TYPE IGR_ULONG

/// @brief Subfile is password protected.
#define IGR_SUBFILE_INFO_FLAG_PASSWORD_PROTECTED 0x0002

/// @brief Subfile has comment data.
#define IGR_SUBFILE_INFO_FLAG_HAS_COMMENT 0x0004


// ****************************************************************************
// * IGR_TEXT_ALIGN type and constants

/// @brief Canvas text alignment type definition.
#define IGR_TEXT_ALIGN_TYPE IGR_LONG

/// @brief Left alignment.
#define IGR_TEXT_ALIGN_LEFT       0x00

/// @brief Right alignment.
#define IGR_TEXT_ALIGN_RIGHT      0x01

/// @brief Center alignment.
#define IGR_TEXT_ALIGN_CENTER     0x02

/// @brief Top alignment.
#define IGR_TEXT_ALIGN_TOP        0x10

/// @brief Middle alignment.
#define IGR_TEXT_ALIGN_MIDDLE     0x11

/// @brief Bottom alignment.
#define IGR_TEXT_ALIGN_BOTTOM     0x12


// ****************************************************************************
// * IGR_TEXT_STYLE type and constants

/// @brief Type definition for text style options used with IGR_Canvas_SetFont.
#define IGR_TEXT_STYLE_TYPE IGR_LONG

/// @brief Normal text style with no formatting.
#define IGR_TEXT_STYLE_NORMAL     0x00

/// @brief Bold text formatting.
#define IGR_TEXT_STYLE_BOLD       0x01

/// @brief Italic text formatting.
#define IGR_TEXT_STYLE_ITALIC     0x02

/// @brief Underlined text formatting.
#define IGR_TEXT_STYLE_UNDERLINE  0x04


// ****************************************************************************
// * IGR_TEXTRECT type and constants

/// @brief Type definition for text rectangle rendering flags.
#define IGR_TEXTRECT_TYPE IGR_LONG

/// @brief Flag to clip text within the rectangle bounds.
#define IGR_TEXTRECT_FLAG_CLIP    0x01

/// @brief Flag to stretch text to fit within the rectangle bounds.
#define IGR_TEXTRECT_FLAG_STRETCHTOFIT 0x02


// ****************************************************************************
// * Structures
// ****************************************************************************

/// @struct tagError_Control_Block
/// @brief Structure representing an error control block containing error 
/// message details.
typedef struct tagError_Control_Block
{
	/// @brief Error message text.
	char Msg[238];
	/// @brief Error message identifier.
	IGR_SHORT MsgId;
} Error_Control_Block;

/// @brief Type definition for pointer to \ref Error_Control_Block.
typedef Error_Control_Block* lpECB_Type;
/// @brief Type definition for \ref Error_Control_Block.
typedef Error_Control_Block ECB_Type;

/// @struct tagInstance_Status_Block
/// @brief Structure containing instance status information including version 
/// and license details.
typedef struct tagInstance_Status_Block
{
	/// @brief DLL version information string.
	char DLL_Version[16];

	/// @brief Reserved field for internal use.
	IGR_SHORT RReserved;
	
	/// @brief Type of license.
	IGR_SHORT License_Type;
	
	/// @brief First part of the licensee identification.
	char Licensee_ID1[40];
	
	/// @brief Second part of the licensee identification.
	char Licensee_ID2[40];
} Instance_Status_Block, * LPInstance_Status_Block;

/// @brief Type definition for pointer to \ref IGR_Stream.
typedef struct IGR_Stream* PIGR_Stream;

#ifndef _IGR_Stream
/// @struct IGR_Stream
/// @brief Structure representing a stream interface with basic I/O operations.
struct IGR_Stream
{
	/// @brief Opens the stream
	/// @param stream [in] Pointer to the stream to open
	/// @return Status code indicating success or failure
	ULONG(CALLBACK* Open) (struct IGR_Stream* stream);

	/// @brief Reads data from the stream
	/// @param stream     [in] Pointer to the stream to read from
	/// @param buffer     [out] Buffer to store read data
	/// @param bufferSize [in] Size of the buffer in bytes
	/// @return Number of bytes read
	ULONG(CALLBACK* Read) (struct IGR_Stream* stream, void* buffer, ULONG bufferSize);

	/// @brief Seeks to a position in the stream
	/// @param stream [in] Pointer to the stream
	/// @param offset [in] Position to seek to
	/// @param whence [in] Origin for the seek operation
	/// @return New position in the stream
	IGR_LONGLONG(CALLBACK* Seek) (struct IGR_Stream* stream, IGR_LONGLONG offset, ULONG whence);

	/// @brief Closes the stream and frees resources
	/// @param stream [in] Pointer to the stream to close
	void (CALLBACK* Close) (struct IGR_Stream* stream);
};
#endif

/// @brief Type definition for pointer to \ref IGR_Writable_Stream.
typedef struct IGR_Writable_Stream* PIGR_Writable_Stream;

/// @struct IGR_Writable_Stream
/// @brief Structure representing a writable stream interface extending 
/// IGR_Stream.
struct IGR_Writable_Stream
{
	/// @brief Base stream functionality.
	struct IGR_Stream base;

	/// @brief Writes data to the stream
	/// @param stream     [in] Pointer to the stream to write to
	/// @param buffer     [in] Buffer containing data to write
	/// @param bufferSize [in] Number of bytes to write
	/// @return Number of bytes written
	ULONG(CALLBACK* Write) (struct IGR_Writable_Stream* stream, void* buffer, ULONG bufferSize);
};

/// @struct IGR_Rect
/// @brief Structure representing a rectangle with unsigned integer coordinates.
/// The coordinates are defined by the left, top, right, and bottom edges.
struct IGR_Rect
{
	/// @brief Left edge coordinate.
	IGR_ULONG left;

	/// @brief Top edge coordinate.
	IGR_ULONG top;

	/// @brief Right edge coordinate.
	IGR_ULONG right;

	/// @brief Bottom edge coordinate.
	IGR_ULONG bottom;
};

/// @struct IGR_SRect
/// @brief Structure representing a rectangle with signed integer coordinates.
/// The coordinates are defined by the left, top, right, and bottom edges.
struct IGR_SRect
{
	/// @brief Left edge coordinate.
	IGR_LONG left;

	/// @brief Top edge coordinate.
	IGR_LONG top;

	/// @brief Right edge coordinate.
	IGR_LONG right;

	/// @brief Bottom edge coordinate.
	IGR_LONG bottom;
};

/// @struct IGR_FRect
/// @brief Structure representing a rectangle with floating-point coordinates.
/// The coordinates are defined by the left, top, right, and bottom edges.
struct IGR_FRect
{
	/// @brief Left edge coordinate.
	IGR_FLOAT left;

	/// @brief Top edge coordinate.
	IGR_FLOAT top;

	/// @brief Right edge coordinate.
	IGR_FLOAT right;

	/// @brief Bottom edge coordinate.
	IGR_FLOAT bottom;
};

/// @struct IGR_Size
/// @brief Structure representing a size with unsigned integer width and height.
struct IGR_Size
{
	/// @brief Width dimension.
	IGR_ULONG width;

	/// @brief Height dimension.
	IGR_ULONG height;
};

/// @struct IGR_FPoint
/// @brief Structure representing a point with signed floating-point 
/// coordinates.
struct IGR_FPoint
{
	/// @brief X position.
	IGR_FLOAT x;

	/// @brief Y position.
	IGR_FLOAT y;
};

/// @struct IGR_QuadPoint
/// @brief Structure representing four points defining a quadrilateral in 
/// clockwise order.
struct IGR_QuadPoint
{
	union {
		struct {
			/// @brief Upper left corner point.
			struct IGR_FPoint upperLeft;

			/// @brief Upper right corner point.
			struct IGR_FPoint upperRight;

			/// @brief Lower right corner point.
			struct IGR_FPoint lowerRight;

			/// @brief Lower left corner point.
			struct IGR_FPoint lowerLeft;
		};
		/// @brief Array access to all four points.
		struct IGR_FPoint points[4];
	};
};

/// @struct IGR_Page_Word
/// @brief Structure containing positional information about each word on a 
/// page. Units are pixels.
struct IGR_Page_Word
{
	/// @brief X position of the word.
	IGR_LONG x;

	/// @brief Y position of the word.
	IGR_LONG y;
	
	/// @brief Width of the word.
	IGR_LONG width;

	/// @brief Height of the word.
	IGR_LONG height;

	/// @brief Reserved for future use. Style information.
	IGR_LONG style;

	/// @brief Pointer to a UCS2 string representing the word.
	const IGR_UCS2* word;

	/// @brief Length of the word.
	IGR_LONG wordLength;

	/// @brief Character offset of the word.
	IGR_LONG charoffset;

	/// @brief Reserved data for future use.
	IGR_LONG reserved[8];
};

/// @struct IGR_Page_Form_Element
/// @brief Structure representing a form element on a page.
struct IGR_Page_Form_Element
{
	/// @brief Reserved data for future use.
	const void* reserved;

	/// @brief Element form element name.
	IGR_UCS2 name[255];

	/// @brief X pixel position of the form element.
	IGR_LONG x;

	/// @brief Y pixel position of the form element.
	IGR_LONG y;

	/// @brief Width in pixels of the form element.
	IGR_LONG width;

	/// @brief Height in pixels of the form element.
	IGR_LONG height;

	/// @brief Element type, one of IGR_PAGE_FORM_ELEMENT_TYPE_X.
	IGR_PAGE_FORM_ELEMENT_TYPE type;

	/// @brief Element flags, bit set of IGR_PAGE_FORM_ELEMENT_FLAG_X.
	IGR_LONG flags;

	/// @brief Font size of field text in points.
	IGR_LONG fontSize;

	/// @brief Bit set of IGR_TEXT_STYLE_XYZ.
	IGR_TEXT_STYLE_TYPE fontStyle;

	/// @brief Field rotation in degrees (0, 90, 180, 270).
	IGR_LONG rotation;

	/// @brief Text alignment inside field, one of IGR_TEXT_ALIGN_X.
	IGR_TEXT_ALIGN_TYPE textAlignment;

	/// @brief Selected item for combo boxes or single select list boxes.
	IGR_LONG selectedItem;

	/// @brief Number of items available in a combobox or list box.
	IGR_LONG option_count;

	/// @brief Reserved data for future use.
	IGR_LONG reserved2[8];
};

/// @struct IGR_Bookmark
/// @brief Structure representing a bookmark item.
struct IGR_Bookmark
{
	/// @brief Reserved data for future use.
	const void* reserved;

	/// @brief Flags indicating specific characteristics.
	IGR_LONG flags;

	/// @brief Display name for the bookmark.
	IGR_UCS2 title[255];

	/// @brief Action to occur when the bookmark is selected.
	IGR_BOOKMARK_ACTION_TYPE action;

	/// @brief Destination for the bookmark.
	IGR_UCS2 dest[1024];

	/// @brief Depth/level of the bookmark in the hierarchy.
	IGR_LONG level;

	/// @brief Page fit method to use when navigating.
	IGR_BOOKMARK_FIT_TYPE fit;

	/// @brief Zoom factor to apply when navigating.
	IGR_LONG zoom;

	/// @brief X position in pixels of the bookmark destination.
	IGR_LONG x;

	/// @brief Y position in pixels of the bookmark destination.
	IGR_LONG y;

	/// @brief Width in pixels of the bookmark destination.
	IGR_LONG width;

	/// @brief Height in pixels of the bookmark destination.
	IGR_LONG height;

	/// @brief Color used when displaying the bookmark item.
	IGR_LONG color;

	/// @brief Text styles used when displaying the bookmark item.
	IGR_LONG text_style;

	/// @brief Reserved data for future use.
	IGR_LONG reserved2[8];
};

/// @brief Callback function for bookmark processing.
///
/// @param element [in] Pointer to the bookmark element being processed.
/// @param context [in] User-provided context data.
///
/// @return IGR_LONG status code.
typedef IGR_LONG(IGR_EXPORT* IGR_CALLBACK_BOOKMARK)(struct IGR_Bookmark* element, void* context);

/// @brief General callback function for various actions.
///
/// @param actionID   [in] ID of the action being performed.
/// @param actionData [in] Data specific to the action.
/// @param context    [in] User-provided context data.
///
/// @return IGR_LONG status code.
typedef IGR_LONG(*IGR_CALLBACK)(int actionID, void* actionData, void* context);

/// @struct IGR_T_ACTION_GET_STREAM_PART
/// @brief Structure passed to IGR_CALLBACK's actionData when actionID is 
/// IGR_ACTION_GET_STREAM_PART.
struct IGR_T_ACTION_GET_STREAM_PART
{
	/// @brief Size of this structure.
	IGR_ULONG size;

	/// @brief Input: Part name (e.g., foo.ra01).
	const wchar_t* partName;

	/// @brief Input: Full part name (e.g., c:\foo.ra01).
	const wchar_t* partFullName;

	/// @brief Input: Part index.
	IGR_LONG partIndex;

	/// @brief Output: Pointer to the output stream.
	struct IGR_Stream* istr;
};

/// @struct IGR_Render_Page_Form_Values
/// @brief Structure representing form values for rendering a page.
struct IGR_Render_Page_Form_Values
{
	/// @brief Pointer to the name of the form value.
	const IGR_UCS2* name;

	/// @brief Pointer to the value of the form value.
	const IGR_UCS2* value;

	/// @brief Selection state of the form value.
	IGR_LONG selected;
};

/// @struct IGR_Render_Page_Redactions
/// @brief Structure representing a redaction area for rendering on a page.
struct IGR_Render_Page_Redactions
{
	/// @brief X-coordinate of the redaction area.
	IGR_LONG x;

	/// @brief Y-coordinate of the redaction area.
	IGR_LONG y;

	/// @brief Width of the redaction area.
	IGR_LONG width;

	/// @brief Height of the redaction area.
	IGR_LONG height;

	/// @brief Color type used for rendering the redaction.
	IGR_COLOR_TYPE color;

	/// @brief Reserved data for future use.
	IGR_ULONG reserved;
};

/// @struct IGR_Render_Page_Properties
/// @brief Structure passed to IGR_Render_Page_Ex function containing page 
/// rendering properties.
struct IGR_Render_Page_Properties
{
	/// @brief Size of this structure.
	IGR_ULONG struct_size;

	/// @brief Number of items in the form_values array.
	IGR_ULONG form_value_count;

	/// @brief Pointer to the first form value item.
	const struct IGR_Render_Page_Form_Values* form_values;

	/// @brief Redaction flags.
	IGR_REDACT_FLAGS_TYPE redaction_flags;

	/// @brief Number of items in the redaction array.
	IGR_ULONG redaction_count;

	/// @brief Pointer to the first redaction item.
	const struct IGR_Render_Page_Redactions* redactions;

	/// @brief Source rectangle in pixels.
	struct IGR_Rect source_rect;

	/// @brief Destination rectangle in pixels.
	struct IGR_Rect dest_rect;
};

/// @struct IGR_Hyperlink
/// @brief Structure representing a hyperlink element that contains details like
/// action type, position, size, destination page number, zoom factor, 
/// destination position, flags, and internal data
struct IGR_Hyperlink
{
	/// @brief Reserved data for internal use.
	const void* reserved;

	/// @brief Type of action to execute when the link is clicked.
	IGR_HYPERLINK_ACTION_TYPE type;

	/// @brief X pixel position of the hyperlink element.
	IGR_LONG x;

	/// @brief Y pixel position of the hyperlink element.
	IGR_LONG y;

	/// @brief Width in pixels of the hyperlink element.
	IGR_LONG width;

	/// @brief Height in pixels of the hyperlink element.
	IGR_LONG height;

	/// @brief Page number to show if type is GOTO.
	IGR_LONG page_number;

	/// @brief Page fit to apply when navigating to the destination.
	IGR_HYPERLINK_FIT_TYPE dest_fit;

	/// @brief Zoom factor to apply when navigating.
	IGR_LONG dest_zoom;

	/// @brief Left position in pixels for the destination, fit value specific.
	IGR_LONG dest_left;

	/// @brief Top position in pixels for the destination, fit value specific.
	IGR_LONG dest_top;

	/// @brief Right position in pixels for the destination, fit value specific.
	IGR_LONG dest_right;

	/// @brief Bottom position in pixels for the destination, fit value specific.
	IGR_LONG dest_bottom;

	/// @brief Flags associated with the hyperlink.
	IGR_HYPERLINK_FLAGS_TYPE flags;

	/// @brief Reserved data for internal use.
	IGR_LONG reserved2[8];
};

/// @struct IGR_Open_DIB_Info
/// @brief Structure representing information about an open Device Independent 
/// Bitmap (DIB) containing details like size, flags, dimensions, stride, pixel 
/// format, data and palette information.
struct IGR_Open_DIB_Info
{
	/// @brief Size of this structure, must be populated.
	IGR_ULONG struct_size;

	/// @brief Flags indicating specific characteristics.
	IGR_ULONG flags;

	/// @brief Width of the bitmap, in pixels.
	IGR_ULONG width;

	/// @brief Height of the bitmap, in pixels.
	IGR_ULONG height;

	/// @brief Number of bytes per scanline.
	IGR_ULONG stride;

	/// @brief Format of the pixel data.
	IGR_OPEN_BITMAP_PIXEL_TYPE pixel_format;

	/// @brief Pointer to the raw pixel data.
	const void* pixel_data;

	/// @brief Size of the buffer in pixel_data.
	IGR_ULONG pixel_data_size;

	/// @brief Pointer to the RGB palette information.
	const IGR_LONG* palette;

	/// @brief Number of colors in the palette.
	IGR_ULONG palette_count;

	/// @brief Horizontal resolution of the bitmap.
	IGR_ULONG dpi_x;

	/// @brief Vertical resolution of the bitmap.
	IGR_ULONG dpi_y;

	/// @brief Orientation of the bitmap.
	IGR_OPEN_BITMAP_ORIENTATION_TYPE orientation;
};

/// @struct IGR_Page_Pixels
/// @brief Structure representing extracted pixels from a page including width,
/// height, stride, pixel format, flags, palette information and reserved data.
struct IGR_Page_Pixels
{
	/// @brief Number of pixels in one scan line of the extracted pixels.
	IGR_ULONG width;

	/// @brief Number of scanlines extracted.
	IGR_ULONG height;

	/// @brief Offset in bytes between consecutive scan lines. Positive for 
	/// top-down, negative for bottom-up.
	IGR_LONG stride;

	/// @brief Pixel format of the extracted pixels.
	IGR_OPEN_BITMAP_PIXEL_TYPE pixel_format;

	/// @brief Pixel format flags.
	IGR_OPEN_BITMAP_FLAGS_TYPE flags;

	/// @brief Pointer to the first scan line extracted.
	void* scanline0;

	/// @brief Number of palette colors extracted, if any.
	IGR_ULONG palette_count;

	/// @brief Palette colors extracted.
	IGR_LONG palette[256];

	/// @brief Reserved for internal use.
	void* reserved;
};

/// @struct IGR_Subfile_Info
/// @brief Structure representing information about a subfile or sub-document 
/// including size, flags, identifiers, names, comments, size in bytes, and 
/// date/time.
struct IGR_Subfile_Info
{
	/// @brief Size of this structure, must be populated.
	IGR_ULONG struct_size;

	/// @brief Flags indicating specific characteristics.
	IGR_SUBFILE_INFO_FLAG_TYPE flags;

	/// @brief Buffer to be populated with the id.
	IGR_UCS2* id;

	/// @brief Number of characters pointed to by id.
	IGR_ULONG id_size;

	/// @brief Buffer to be populated with the name.
	IGR_UCS2* name;

	/// @brief Number of characters pointed to by name.
	IGR_ULONG name_size;

	/// @brief Buffer to be populated with the comment.
	IGR_UCS2* comment;

	/// @brief Number of characters pointed to by comment.
	IGR_ULONG comment_size;

	/// @brief Size in bytes of the sub-document.
	IGR_ULONGLONG size;

	/// @brief Date and time of the sub-document in FILETIME format.
	IGR_ULONGLONG date;
};

/// @struct IGR_Subfile_Enumerator
/// @brief Forward declaration of subfile enumerator structure.
struct IGR_Subfile_Enumerator;

/// @brief Handle type for subfile enumerators.
typedef struct IGR_Subfile_Enumerator* HSUBFILES;

/// @struct IGR_Annotation
/// @brief Structure representing an annotation including type, flags, position,
/// size and reserved data.
struct IGR_Annotation
{
	/// @brief Reserved data for internal use.
	const void* reserved;

	/// @brief Type of the annotation.
	IGR_ANNOTATION_TYPE type;

	/// @brief Flags on this annotation.
	IGR_ANNOTATION_FLAGS_TYPE flags;

	/// @brief X pixel position of the annotation's bounding box.
	IGR_LONG x;

	/// @brief Y pixel position of the annotation's bounding box.
	IGR_LONG y;

	/// @brief Width in pixels of the annotation's bounding box.
	IGR_LONG width;

	/// @brief Height in pixels of the annotation's bounding box.
	IGR_LONG height;

	/// @brief Reserved data for internal use.
	IGR_LONG reserved2[8];
};

/// @struct IGR_Open_Callback_Action_Heartbeat
/// @brief Structure passed as the payload for IGR_OPEN_CALLBACK when the 
/// action argument is IGR_OPEN_CALLBACK_ACTION_HEARTBEAT to handle heartbeat 
/// events.
struct IGR_Open_Callback_Action_Heartbeat
{
	/// @brief Size of this structure in bytes, must be populated by caller.
	IGR_ULONG struct_size;
};


/// @struct IGR_Open_Callback_Action_Password
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when requesting 
/// password input.
struct IGR_Open_Callback_Action_Password
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief ID of subfile requiring password. Empty for main document 
	/// password.
	IGR_UCS2 id[4096];

	/// @brief Password to use for decoding the document/subfile. Must be null
	/// terminated.
	IGR_UCS2 password[4096];
};

/// @struct IGR_Open_Callback_Action_Localize
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when localizing 
/// strings.
struct IGR_Open_Callback_Action_Localize
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Internal string resource identifier.
	IGR_ULONG string_id;

	/// @brief Original string before localization.
	IGR_UCS2 original[1024];

	/// @brief Replacement string after localization.
	IGR_UCS2 replacement[1024];
};

/// @struct IGR_Open_Callback_Action_Log_Level
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when setting log 
/// levels.
struct IGR_Open_Callback_Action_Log_Level
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Name of module requesting log level setting.
	char module[16];

	/// @brief Desired log level for module, defaults to IGR_LOG_LEVEL_NONE.
	IGR_LOG_LEVEL_TYPE result;
};

/// @struct IGR_Open_Callback_Action_Log_Message
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when logging 
/// messages.
struct IGR_Open_Callback_Action_Log_Message
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Name of module generating the log message.
	char module[16];

	/// @brief Log level for this message.
	IGR_LOG_LEVEL_TYPE log_level;

	/// @brief The actual log message text.
	char message[512];
};

/// @struct IGR_Open_Callback_Action_Approve_External_Resource
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when approving 
/// external resources.
struct IGR_Open_Callback_Action_Approve_External_Resource
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Original URL of the external resource (e.g., image URL).
	IGR_UCS2 url[1024];
};

/// @struct IGR_Open_Callback_Action_Get_Resource_Stream
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when retrieving 
/// resource streams.
struct IGR_Open_Callback_Action_Get_Resource_Stream
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Original URL of the external resource (e.g., image URL).
	IGR_UCS2 url[1024];

	/// @brief User-created stream to read resource bytes from.
	struct IGR_Stream* result;
};

/// @struct IGR_Open_Callback_Action_OCR_Image_Style_Info
/// @brief Structure containing OCR font style information.
struct IGR_Open_Callback_Action_OCR_Image_Style_Info
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Font size detected by OCR engine.
	IGR_FLOAT font_size;

	/// @brief Font family detected by OCR engine.
	IGR_UCS2 font_family[256];

	/// @brief Font style flags using IGR_TEXT_STYLE_ bitfield.
	IGR_ULONG text_style;
};

/// @struct IGR_Open_Callback_Action_OCR_Image
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when processing 
/// OCR images.
struct IGR_Open_Callback_Action_OCR_Image
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Reserved for internal use.
	void* reserved;

	/// @brief Image data to be processed.
	const struct IGR_Open_DIB_Info* source_image_pixels;

	/// @brief Page index containing the image.
	IGR_ULONG source_page_index;

	/// @brief Rectangle location of image in source document.
	struct IGR_Rect source_rect;

	/// @brief Name of source image if known.
	IGR_UCS2 source_name[128];

	/// @brief Function to retrieve EXIF metadata from document.
	IGR_RETURN_CODE(IGR_EXPORT* GetMetadata)(const struct IGR_Open_Callback_Action_OCR_Image* action, const IGR_UCS2* name, IGR_UCS2* dest, size_t dest_size);

	/// @brief Function to save image to file with specified MIME type.
	IGR_RETURN_CODE(IGR_EXPORT* SaveImage)(const struct IGR_Open_Callback_Action_OCR_Image* action, const IGR_UCS2* filename, const IGR_UCS2* mimetype);

	/// @brief Function to start a new block element in output.
	IGR_RETURN_CODE(IGR_EXPORT* StartBlock)(const struct IGR_Open_Callback_Action_OCR_Image* action, IGR_ULONG block_type, const struct IGR_QuadPoint* rect);

	/// @brief Function to close current block element in output.
	IGR_RETURN_CODE(IGR_EXPORT* EndBlock)(const struct IGR_Open_Callback_Action_OCR_Image* action, IGR_ULONG block_type);

	/// @brief Function to add text to output.
	IGR_RETURN_CODE(IGR_EXPORT* AddText)(const struct IGR_Open_Callback_Action_OCR_Image* action, const IGR_UCS2* text, const struct IGR_QuadPoint* rect, IGR_ULONG flags, const struct IGR_Open_Callback_Action_OCR_Image_Style_Info* font_style);

	/// @brief Function to add text with character offsets to output.
	IGR_RETURN_CODE(IGR_EXPORT* AddTextWithOffsets)(const struct IGR_Open_Callback_Action_OCR_Image* action, const IGR_UCS2* text, const IGR_FLOAT* offsets, const struct IGR_QuadPoint* rect, IGR_ULONG flags, const struct IGR_Open_Callback_Action_OCR_Image_Style_Info* font_style);

	/// @brief Function to reorient processed image.
	IGR_RETURN_CODE(IGR_EXPORT* Reorient)(const struct IGR_Open_Callback_Action_OCR_Image* action, IGR_FLOAT degrees);
};

/// @struct IGR_Page_Element
/// @brief Structure representing a page element including position, type and 
/// flags.
struct IGR_Page_Element
{
	/// @brief Size of this structure, must be populated.
	IGR_ULONG struct_size;

	/// @brief Reserved data for internal use.
	const void* reserved;

	/// @brief Type of page element.
	IGR_PAGE_ELEMENT_TYPE type;

	/// @brief Depth of element in document hierarchy.
	IGR_ULONG depth;

	/// @brief Flags indicating element properties.
	IGR_PAGE_ELEMENT_FLAG_TYPE flags;

	/// @brief Position and dimensions of element.
	struct IGR_FRect pos;

	/// @brief Rotation angle in degrees.
	IGR_ULONG rotation;
};

/// @brief Callback function for processing page element styles.
///
/// @param name    [in] Name of the style.
/// @param value   [in] Value of the style.
/// @param context [in] User provided context.
///
/// @return IGR_LONG Status code
typedef IGR_LONG(IGR_EXPORT* IGR_PAGE_ELEMENT_STYLES_CALLBACK)(const IGR_UCS2* name, const IGR_UCS2* value, void* context);

/// @brief Callback function for processing page elements.
///
/// @param page    [in] Handle to the page.
/// @param item    [in] Pointer to the page element.
/// @param context [in] User provided context.
///
/// @return IGR_LONG Status code
typedef IGR_LONG(IGR_EXPORT* IGR_PAGE_ELEMENT_CALLBACK)(IGR_HPAGE page, const struct IGR_Page_Element* item, void* context);

/// @struct IGR_Compare_Documents_Callback_Context
/// @brief Context structure for document comparison callback functions.
struct IGR_Compare_Documents_Callback_Context
{
	/// @brief Size of this structure.
	IGR_ULONG struct_size;

	/// @brief Index of document being processed (0 or 1).
	IGR_ULONG doc_index;

	/// @brief Handle to document being processed.
	IGR_LONG doc_handle;

	/// @brief Index of current page being processed.
	IGR_ULONG page_index;

	/// @brief Handle to current page being processed.
	IGR_HPAGE page_handle;
};

/// @struct IGR_Compare_Documents_Difference_Item
/// @brief Structure representing a difference item between compared documents.
struct IGR_Compare_Documents_Difference_Item
{
	/// @brief Size of this structure.
	IGR_ULONG struct_size;

	/// @brief Index of page containing the difference.
	IGR_ULONG page_index;

	/// @brief Bounding box coordinates in pixels.
	struct IGR_FRect bounds;

	/// @brief Null terminated UCS2 text content.
	const IGR_UCS2* text;
};

/// @struct IGR_Compare_Documents_Difference
/// @brief Structure representing a difference between compared documents.
struct IGR_Compare_Documents_Difference
{
	/// @brief Size of this structure.
	IGR_ULONG struct_size;

	/// @brief Type of difference found.
	IGR_COMPARE_DOCUMENTS_DIFFERENCE_TYPE type;

	/// @brief Document source where difference occurred.
	IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_TYPE doc_source;

	/// @brief Page index in original/left document.
	IGR_ULONG original_page_index;

	/// @brief Page index in revised/right document.
	IGR_ULONG revised_page_index;

	/// @brief Number of difference items.
	IGR_ULONG item_count;

	/// @brief Pointer to array of difference items.
	const struct IGR_Compare_Documents_Difference_Item* items;
};

/// @brief Forward declaration of text comparison enumerator.
struct IGR_Text_Compare_Enumerator;

/// @struct IGR_Text_Compare_Settings
/// @brief Settings structure for text comparison operations.
struct IGR_Text_Compare_Settings
{
	/// @brief Size of this structure.
	IGR_ULONG struct_size;

	/// @brief Type of comparison to perform.
	IGR_COMPARE_DOCUMENTS_COMPARE_TYPE compare_type;

	/// @brief Flags controlling comparison behavior.
	IGR_COMPARE_DOCUMENTS_FLAGS_TYPE flags;

	/// @brief User provided context passed to callbacks.
	void* user_context;

	/// @brief Reserved comparison window size.
	IGR_ULONG comparison_window;

	/// @brief Reserved comparison overlap amount.
	IGR_ULONG comparison_overlap;

	/// @brief Reserved for future use.
	void* reserved1;

	/// @brief Reserved for future use.
	void* reserved2;

	/// @brief Reserved for future use.
	void* reserved3;

	/// @brief Reserved for future use.
	void* reserved4;

	/// @brief Callback to open a page resource.
	IGR_RETURN_CODE(CALLBACK* OpenPage) (void* context, const struct IGR_Compare_Documents_Callback_Context* info, IGR_ULONG page_index, IGR_HPAGE* page_handle);

	/// @brief Callback to close a page resource.
	IGR_RETURN_CODE(CALLBACK* ClosePage) (void* context, const struct IGR_Compare_Documents_Callback_Context* info, IGR_HPAGE page_handle);

	/// @brief Callback to get page margins in pixels.
	IGR_RETURN_CODE(CALLBACK* GetPageMargins)(void* context, const struct IGR_Compare_Documents_Callback_Context* info, struct IGR_FRect* margins);

	/// @brief Callback to get page area for text comparison.
	IGR_RETURN_CODE(CALLBACK* GetPageArea)(void* context, const struct IGR_Compare_Documents_Callback_Context* info, struct IGR_FRect* page_area);

	/// @brief Callback to get next page index to process.
	IGR_RETURN_CODE(CALLBACK* GetNextPageIndex) (void* context, const struct IGR_Compare_Documents_Callback_Context* info, IGR_ULONG* page_index);

	/// @brief Callback for error handling.
	IGR_RETURN_CODE(CALLBACK* HandleError) (void* context, const struct IGR_Compare_Documents_Callback_Context* info, const Error_Control_Block* ecb);
};

/// @struct IGR_Text_Compare_Document_Source
/// @brief Structure representing a source document for text comparison.
struct IGR_Text_Compare_Document_Source
{
	/// @brief Size of this structure.
	IGR_ULONG struct_size;

	/// @brief Handle to an open document.
	IGR_LONG doc_handle;

	/// @brief First page index to compare.
	IGR_ULONG doc_first_page;

	/// @brief Number of pages to compare.
	IGR_ULONG doc_page_count;

	/// @brief Document margins in points to exclude.
	struct IGR_FRect doc_margins;
};

/// @brief Type definition for pointer to text comparison enumerator.
typedef struct IGR_Text_Compare_Enumerator* IGR_HTEXTCOMPARE;

// ****************************************************************************
// * Functions
// ****************************************************************************

/// @brief Initializes the Document Filters engine and authenticates your 
/// license.
///
/// @param reserved  [in]     Unused, must be zero.
/// @param path      [in]     Path to Perceptive executables.
/// @param init      [in,out] On input, contains application License Code; 
///                           on output licensee information.
/// @param handle   [out]     Returns the instance handle.
/// @param error    [out]     Contains any error text.
void IGR_EXPORT Init_Instance(IGR_LONG reserved, const char* path, Instance_Status_Block* init, IGR_SHORT* handle, Error_Control_Block* error);

/// @brief Advises the Document Filters engine that the program is finished.
///
/// @param error [out] Contains any error text
void IGR_EXPORT Close_Instance(Error_Control_Block* error);

/// @brief Legacy method, new code does not need to call this method.
///
/// @param handle [in] Instance handle.
/// @param error  [out] Contains any error text
void IGR_EXPORT Switch_Instance(IGR_SHORT handle, Error_Control_Block* error);

/// @brief Converts a UCS2 string into UTF-8.
///
/// @param source     [in] Contains a NULL terminated UCS2 string.
/// @param buffer     [out] Pointer to a buffer that will hold the converted text.
/// @param bufferSize [in] Indicates the size of the buffer in bytes, including NULL
void IGR_EXPORT Widechar_to_UTF8(const IGR_UCS2* source, char* buffer, IGR_LONG bufferSize);

/// @brief Converts a UCS2 string into UTF-8 with explicit source length.
///
/// @param source     [in] Contains a UCS2 string.
/// @param sourceLen  [in] Length of source string in characters.
/// @param buffer     [out] Pointer to a buffer that will hold the converted text.
/// @param bufferSize [in] Indicates the size of the buffer in bytes, including NULL
void IGR_EXPORT Widechar_to_UTF8_Ex(const IGR_UCS2* source, IGR_LONG sourceLen, char* buffer, IGR_LONG bufferSize);

/// @brief Converts a UTF-8 string into a UCS2 string.
///
/// @param source     [in]  Contains a NULL terminated UTF-8 string.
/// @param buffer     [out] Pointer to a buffer that will hold the converted text.
/// @param bufferSize [in]  Indicates the size of the buffer in characters, including NULL
void IGR_EXPORT UTF8_to_Widechar(const char* source, IGR_UCS2* buffer, IGR_LONG bufferSize);

/// @brief Converts a UTF-8 string into a UCS2 string with explicit source 
/// length.
///
/// @param source     [in]  Contains a UTF-8 string.
/// @param sourceLen  [in]  Length of the source string.
/// @param buffer     [out] Pointer to a buffer that will hold the converted text.
/// @param bufferSize [in]  Indicates the size of the buffer in characters, including NULL
void IGR_EXPORT UTF8_to_Widechar_Ex(const char* source, IGR_LONG sourceLen, IGR_UCS2* buffer, IGR_LONG bufferSize);

/// @brief Legacy function for engine call tracing, not needed for new code.
///
/// @param param1     [in]  Unused parameter.
/// @param param2     [in]  Unused parameter.
/// @param param3     [in]  Unused parameter
void IGR_EXPORT Set_Engine_Call_Tracing(IGR_LONG, IGR_LONG, const char*);

/// @brief Legacy function for setting API character set, not needed for new 
/// code.
/// 
/// @param param1     [in]  Unused parameter.
/// @param error      [out] Contains any error text
void IGR_EXPORT Set_API_Charset(IGR_LONG, Error_Control_Block* error);

/// @brief Legacy function for setting DLL options, not needed for new code.
/// 
/// @param param1     [in]  Unused parameter.
/// @param error      [out] Contains any error text
void IGR_EXPORT Set_DLL_Option(const char*, Error_Control_Block* error);

/// @brief Legacy multiplex function, not needed for new code.
/// 
/// @param param1     [in]     Unused parameter.
/// @param param2     [in,out] Unused parameter.
/// @param param3     [in,out] Unused parameter.
/// @param error      [out]    Contains any error text
void IGR_EXPORT ISYS_Multiplex(IGR_LONG, size_t*, size_t*, Error_Control_Block* error);

/// @brief Gets the type and capabilities of a given document.
/// 
/// @param filename     [in]  Path to the document to be opened.
/// @param capabilities [out] Returns the Document Capabilities as a bitfield.
/// @param filetype     [out] Returns the Document Format Code of the document.
/// @param error        [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid..
IGR_RETURN_CODE IGR_EXPORT IGR_Get_File_Type(const IGR_UCS2* filename, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype, Error_Control_Block* error);

/// @brief Gets the type and capabilities of a given stream object.
/// 
/// @param stream       [in]  The stream can be either user implemented or 
///                           created using  IGR_Make_Stream_From_File and 
///                           IGR_Make_Stream_From_Memory
/// @param capabilities [out] Returns the Document Capabilities as a bitfield.
/// @param filetype     [out] Returns the Document Format Code of the document.
/// @param error        [out] Contains any error text
/// 
/// @return IGR_OK                on success
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Stream_Type(struct IGR_Stream* stream, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype, Error_Control_Block* error);

/// @brief Opens a document for text extraction or enumeration of 
/// sub-documents.
/// 
/// @param filename     [in]  Path to the document to be opened.
/// @param flags        [in]  Specifies processing options that control the 
///                           output. See Open Document Flags.
/// @param capabilities [out] Returns the Document Capabilities as a bitfield.
/// @param filetype     [out] Returns the Document Format Code of the document.
/// @param handle       [out] Returns a handle to be used in subsequent calls.
/// @param error        [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid..
IGR_RETURN_CODE IGR_EXPORT IGR_Open_File(const IGR_UCS2* filename, IGR_OPEN_FLAGS_TYPE flags, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Opens a document for text extraction or enumeration of sub-documents
/// from a stream.
/// 
/// @param stream       [in]  The stream can be either user implemented, or 
///                           created using IGR_Make_Stream_From_File and 
///                           IGR_Make_Stream_From_Memory.
/// @param flags        [in]  Specifies processing options that control the 
///                           output. See Open Document Flags.
/// @param capabilities [out] Returns the Document Capabilities as a bitfield.
/// @param filetype     [out] Returns the Document Format Code of the document.
/// @param handle       [out] Returns a handle to be used in subsequent calls.
/// @param error        [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid..
IGR_RETURN_CODE IGR_EXPORT IGR_Open_Stream(struct IGR_Stream* stream, IGR_OPEN_FLAGS_TYPE flags, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Opens a document with extended options for text extraction or 
/// enumeration of sub-documents.
/// 
/// @param filename     [in]  Path to the document to be opened.
/// @param flags        [in]  Specifies processing options that control the 
///                           output. See Open Document Flags.
/// @param options      [in]  Extended processing options for HTML conversion. 
///                           Format is Name=Value;Name=Value.
/// @param capabilities [out] Returns the Document Capabilities as a bitfield.
/// @param filetype     [out] Returns the Document Format Code of the document.
/// @param handle       [out] Returns a handle to be used in subsequent calls.
/// @param error        [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_File_Ex(const IGR_UCS2* filename, IGR_OPEN_FLAGS_TYPE flags, const IGR_UCS2* options, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Opens a document from a stream with extended options for text 
/// extraction or enumeration of sub-documents.
/// 
/// @param stream       [in]  The stream can be either user implemented, or 
///                           created using IGR_Make_Stream_From_File and 
///                           IGR_Make_Stream_From_Memory.
/// @param flags        [in]  Specifies processing options that control the 
///                           output. See Open Document Flags
/// @param options      [in]  Extended processing options for HTML conversion. 
///                           Format is Name=Value;Name=Value.
/// @param capabilities [out] Returns the Document Capabilities as a bitfield.
/// @param filetype     [out] Returns the Document Format Code of the document.
/// @param handle       [out] Returns a handle to be used in subsequent calls.
/// @param error        [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_Stream_Ex(struct IGR_Stream* stream, IGR_OPEN_FLAGS_TYPE flags, const IGR_UCS2* options, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* file_type, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Extracts text from a previously opened document.
/// 
/// @param handle      [in]     Handle to document opened by IGR_Open_File/Stream.
/// @param buffer      [in,out] Application allocated memory block to store 
///                             extracted text.
/// @param bufferSize  [in,out] On input: size of buffer in Unicode chars. On 
///                             output: number of chars extracted.
/// @param error       [out]    Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if no more text to extract.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document does not support text 
///                                        extraction.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Text(IGR_LONG handle, IGR_UCS2* buffer, IGR_LONG* bufferSize, Error_Control_Block* error);

/// @brief Enumerates sub-documents contained in a previously opened compound 
/// document.
/// 
/// @param handle [in]  Handle to document opened by IGR_Open_File/Stream.
/// @param id     [out] Buffer of 8192 bytes for 4096 Unicode chars containing 
///                     sub-doc ID.
/// @param name   [out] Buffer of 2048 bytes for 1024 Unicode chars containing 
///                     sub-doc name.
/// @param date   [out] Returns sub-document date/time in FILETIME format.
/// @param size   [out] Returns size of sub-document in bytes.
/// @param error  [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if no more text to extract.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document does not support 
///                                        sub-file enumeration.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfile_Entry(IGR_LONG handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size, Error_Control_Block* error);

/// @brief Enumerates sub-documents contained in a compound document using 
/// extended info.
/// 
/// @param handle [in]     Handle to document opened by IGR_Open_File/Stream.
/// @param result [in,out] Struct to populate with sub-document details.
/// @param error  [out]    Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if no more text to extract.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document does not support 
///                                        sub-file enumeration.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfile_Entry_Ex(IGR_LONG handle, struct IGR_Subfile_Info* result, Error_Control_Block* error);

/// @brief Creates a new enumerator for iterating sub-documents in a compound 
/// document.
/// 
/// @param docHandle  [in]  Handle to document opened by IGR_Open_File/Stream.
/// @param enumerator [out] Handle to new enumerator for sub-document 
///                         iteration.
/// @param error      [out] Contains any error text.

/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document does not support 
///                                        sub-file enumeration.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfiles_Enumerator(IGR_LONG docHandle, HSUBFILES* enumerator, Error_Control_Block* error);

/// @brief Extracts a sub-document to disk from a compound document.
/// 
/// @param handle      [in]  Handle to document opened by IGR_Open_File/Stream.
/// @param id          [in]  ID of sub-document to extract, from 
///                          IGR_Get_Subfile_Entry.
/// @param destination [in]  Path where sub-document will be written.
/// @param error       [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if sub-document not found.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document does not support 
///                                        sub-file enumeration.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Subfile(IGR_LONG handle, const IGR_UCS2* id, const IGR_UCS2* destination, Error_Control_Block* error);

/// @brief Extracts a sub-document to a stream from a compound document.
/// 
/// @param handle [in]  Handle to document opened by IGR_Open_File/Stream.
/// @param id     [in]  ID of sub-document to extract, from 
///                     IGR_Get_Subfile_Entry.
/// @param stream [out] Stream to write extracted sub-document content to.
/// @param error  [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if sub-document not found.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document does not support sub-file enumeration.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Subfile_Stream(IGR_LONG handle, const IGR_UCS2* id, struct IGR_Stream** stream, Error_Control_Block* error);

/// @brief Releases resources associated with an open document handle.
/// 
/// @param handle [in]  Handle to document opened by IGR_Open_File/Stream.
/// @param error  [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Close_File(IGR_LONG handle, Error_Control_Block* error);

/// @brief Provides extensible functionality to the Document Filters API.
/// 
/// @param function [in]     Function to perform, see Document Filters Manual.
/// @param param1   [in,out] Used by some functions.
/// @param param2   [in,out] Used by some functions.
/// @param error    [out]    Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Multiplex(IGR_LONG function, size_t* param1, size_t* param2, Error_Control_Block* error);

/// @brief Enumerates the next sub-document in an enumeration.
/// 
/// @param handle [in]  Handle from IGR_Get_Subfiles_Enumerator.
/// @param id     [out] Buffer of 8192 bytes for 4096 Unicode chars containing sub-doc ID.
/// @param name   [out] Buffer of 2048 bytes for 1024 Unicode chars containing sub-doc name.
/// @param date   [out] Returns sub-document date/time in FILETIME format.
/// @param size   [out] Returns size of sub-document in bytes.
/// @param error  [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if no more text to extract.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if enumeration invalid
IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Next(HSUBFILES handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size, Error_Control_Block* error);

/// @brief Enumerates the next sub-document using extended info.
/// 
/// @param handle [in]     Handle from IGR_Get_Subfiles_Enumerator.
/// @param result [in,out] Struct to populate with sub-document details.
/// @param error  [out]    Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if no more text to extract.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if enumeration invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Next_Ex(HSUBFILES handle, struct IGR_Subfile_Info* result, Error_Control_Block* error);

/// @brief Resets enumeration to beginning for reprocessing.
/// 
/// @param handle [in]  Handle from IGR_Get_Subfiles_Enumerator.
/// @param error  [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Reset(HSUBFILES handle, Error_Control_Block* error);

/// @brief Frees resources for a sub-document enumerator.
///
/// @param handle [in]  Handle from IGR_Get_Subfiles_Enumerator.
/// @param error  [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Close(HSUBFILES handle, Error_Control_Block* error);

/// @brief Converts a document to a plain text or HTML file without needing to 
/// call IGR_Open_File and IGR_Get_Text.
/// 
/// @param filename [in]  Path to the document to be converted.
/// @param flags    [in]  Specifies processing options controlling the output.
/// @param options  [in]  Extended processing options for HTML conversion expressed as.
///                      Name=Value pairs separated by semicolons.
/// @param output   [in]  Filename where output document should be written.
/// @param error    [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if filename, options, output or error are invalid
IGR_RETURN_CODE IGR_EXPORT IGR_Convert_File(const IGR_UCS2* filename, IGR_OPEN_FLAGS_TYPE flags, const IGR_UCS2* options, const IGR_UCS2* output, Error_Control_Block* error);

/// @brief Creates a Perceptive stream from a file for use with document stream functions.
///
/// @param filename [in]  Path to the document to be opened.
/// @param flags    [in]  Bitfield of options that affect stream behavior.
/// @param stream   [out] System allocated stream structure that caller must 
///                       free by calling stream->Close().
/// @param error    [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if filename, stream or error are invalid
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_File(const IGR_UCS2* filename, IGR_LONG flags, struct IGR_Stream** stream, Error_Control_Block* error);

/// @brief Creates a Perceptive stream from a memory buffer.
/// 
/// @param buffer     [in]  Pointer to user allocated memory buffer containing document.
/// @param bufferSize [in]  Size of the buffer in bytes.
/// @param destruct   [in]  Optional function to call when stream is closed to 
///                         free memory Must have signature: 
///                         void __cdecl Destruct(void *data)
/// @param stream     [out] System allocated stream structure that caller must free.
/// @param error      [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if buffer, stream or error are invalid
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_Memory(void* buffer, size_t bufferSize, void* destruct, struct IGR_Stream** stream, Error_Control_Block* error);

/// @brief Creates an IGR stream from callback functions, useful for languages 
/// without precise memory control.
/// 
/// @param context  [in]  User context passed to callback functions.
/// @param flags    [in]  Bitfield of options affecting stream behavior.
/// @param seek     [in]  Required Seek function pointer.
/// @param read     [in]  Required Read function pointer.
/// @param write    [in]  Optional Write function pointer.
/// @param action   [in]  Optional action handler function pointer.
/// @param destruct [in]  Required destructor to free context memory.
/// @param stream   [out] System allocated stream that caller must free.
/// @param error    [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if required parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_Functions(void* context,
	IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_TYPE flags,
	IGR_LONGLONG(CALLBACK* seek)(void* context, IGR_LONGLONG offset, IGR_ULONG whence),
	IGR_ULONG(CALLBACK* read)(void* context, void* buffer, IGR_ULONG buffer_size),
	IGR_ULONG(CALLBACK* write)(void* context, const void* buffer, IGR_ULONG buffer_size),
	IGR_LONG(CALLBACK* action)(void* context, int actionId, void* action_data),
	void (CALLBACK* destruct)(void* context),
	struct IGR_Stream** stream,
	Error_Control_Block* error
);

/// @brief Creates a custom stream that accepts callbacks.
/// 
/// @param pStream     [in]  Valid \ref IGR_Stream instance.
/// @param cb          [in]  Callback function to handle stream events.
/// @param context     [in]  User-supplied context passed to callback.
/// @param ppExtStream [out] Extended stream to use instead of original.
/// @param error       [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_ERROR if input parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Extend_Stream(struct IGR_Stream* pStream, IGR_CALLBACK cb, void* context, struct IGR_Stream** ppExtStream, Error_Control_Block* error);

/// @brief Windows-only memory diagnostic function.
/// 
/// @param buffer [out] Buffer to receive diagnostic info.
/// @param len    [in]  Length of buffer.
/// 
/// @return Size of diagnostic data written.
size_t IGR_EXPORT IGR_Get_Memory_Status(char* buffer, size_t len);

/// @brief Enumerates images from a document opened with HTML conversion enabled.
/// 
/// @param handle [in]  Handle to opened document.
/// @param id     [out] Buffer (8192 bytes) to receive image ID (4096 Unicode chars).
/// @param name   [out] Buffer (2048 bytes) to receive image name (1024 Unicode chars).
/// @param date   [out] Receives image date/time in FILETIME format.
/// @param size   [out] Receives image size in bytes.
/// @param error  [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if no more images.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document doesn't support enumeration
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Image_Entry(IGR_LONG handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size, Error_Control_Block* error);

/// @brief Creates enumerator for images in document opened with HTML conversion.
/// 
/// @param docHandle  [in]  Handle to opened document.
/// @param enumerator [out] Handle to new image enumerator.
/// @param error      [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document doesn't support enumeration.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Images_Enumerator(IGR_LONG docHandle, HSUBFILES* enumerator, Error_Control_Block* error);

/// @brief Extracts an image to disk from document opened with HTML conversion.
/// 
/// @param handle      [in]  Handle to opened document.
/// @param id          [in]  Image ID from IGR_Get_Image_Entry.
/// @param destination [in]  Path where image will be written.
/// @param error       [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if image not found.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document doesn't support extraction.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Image(IGR_LONG handle, const IGR_UCS2* id, const IGR_UCS2* destination, Error_Control_Block* error);

/// @brief Extracts an image to a stream from document opened with HTML conversion.
/// 
/// @param handle [in]  Handle to opened document.
/// @param id     [in]  Image ID from IGR_Get_Image_Entry.
/// @param stream [out] System allocated stream that caller must free.
/// @param error  [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if image not found.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document doesn't support extraction.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Image_Stream(IGR_LONG handle, const IGR_UCS2* id, struct IGR_Stream** stream, Error_Control_Block* error);

/// @brief Calculates the MD5 hash of an input stream for unique document identification.
/// 
/// @param stream       [in]  An open \ref IGR_Stream.
/// @param dest         [in]  Buffer to receive the null-terminated MD5 hash 
///                           (as a Unicode string). Must be able to hold at 
///                           least 33 UCS2 characters.
/// @param error       [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if stream, dest or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Calculate_MD5(struct IGR_Stream* stream, IGR_UCS2* dest, Error_Control_Block* error);

/// @brief Calculates the SHA1 hash of an input stream for unique document identification.
/// 
/// @param stream      [in]  An open \ref IGR_Stream.
/// @param dest        [in]  Buffer to receive the null-terminated SHA1 hash 
///                          (as a Unicode string). Must be able to hold at 
///                          least 41 UCS2 characters.
/// @param error       [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if stream, dest or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Calculate_SHA1(struct IGR_Stream* stream, IGR_UCS2* dest, Error_Control_Block* error);

/// @brief Returns the number of pages for a document opened in HD mode.
/// 
/// @param docHandle   [in]  Handle to the document.
/// @param pageCount   [out] Number of pages in the document.
/// @param error       [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document not opened in HD mode.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Count(IGR_LONG docHandle, IGR_LONG* pageCount, Error_Control_Block* error);

/// @brief Returns a handle to the specified page.
/// 
/// @param docHandle   [in]  Handle to a HD document.
/// @param pageIndex   [in]  0-based index of the page being requested.
/// @param pageHandle  [out] Handle to the requested page.
/// @param error       [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid.
/// @return IGR_E_INVALID_POINTER if pageHandle or error are invalid.
/// @return IGR_E_NOT_VALID_FOR_THIS_CLASS if document not opened in HD mode.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_Page(IGR_LONG docHandle, IGR_LONG pageIndex, HPAGE* pageHandle, Error_Control_Block* error);

/// @brief Closes the specified page handle and frees associated resources.
/// 
/// @param pageHandle [in] Handle to the page to close.
/// @param error     [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid.
/// @return IGR_E_INVALID_POINTER if pageHandle or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Close_Page(HPAGE pageHandle, Error_Control_Block* error);

/// @brief Gets the total number of words on a page.
///
/// @param pageHandle [in]  Handle to the page.
/// @param wordCount  [out] Receives the number of words on the page.
/// @param error      [out] Contains any error text.
/// 
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if wordCount or error are invalid
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Word_Count(HPAGE pageHandle, IGR_LONG* wordCount, Error_Control_Block* error);

/// @brief Gets positional information for a range of words on a page.
///
/// @param pageHandle [in]     Handle to the page.
/// @param index      [in]     Zero-based index of first word to retrieve.
/// @param count      [in,out] On input: size of words array; 
///                            On output: number of words copied
/// @param words      [in,out] Array of \ref IGR_Page_Word structs to receive 
///                            word data.
/// @param error      [out]    Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Words(HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Page_Word* words, Error_Control_Block* error);

/// @brief Gets the dimensions of a page in pixels.
///
/// @param pageHandle [in]  Handle to the page.
/// @param width      [out] Receives the width of the page in pixels.
/// @param height     [out] Receives the height of the page in pixels.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if width, height or error are invalid
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Dimensions(HPAGE pageHandle, IGR_LONG* width, IGR_LONG* height, Error_Control_Block* error);

/// @brief Returns the next piece of page text.
///
/// @param pageHandle [in]  Handle to the page.
/// @param buffer     [out] Buffer to copy text into, up to size characters.
/// @param size       [in,out] On input: number of characters buffer can hold.
///                            On output: number of characters written to buffer.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if size, buffer or error are invalid
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Text(HPAGE pageHandle, IGR_UCS2* buffer, IGR_LONG* size, Error_Control_Block* error);

/// @brief Gets information about the next image in a page.
///
/// @param pageHandle [in]  Handle to the page.
/// @param id         [out] Buffer to receive image ID (min 4096 UCS2 chars).
/// @param name       [out] Buffer to receive image name (min 1024 UCS2 chars).
/// @param date       [out] Receives image date/time in FILETIME format.
/// @param size       [out] Receives image size in bytes.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if no more images
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Image_Entry(HPAGE pageHandle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size, Error_Control_Block* error);

/// @brief Creates an enumerator for iterating through page images.
///
/// @param docHandle  [in]  Handle to the document.
/// @param enumerator [out] Handle to new image enumerator.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Images_Enumerator(IGR_LONG docHandle, HSUBFILES* enumerator, Error_Control_Block* error);

/// @brief Extracts a page image to disk.
///
/// @param pageHandle   [in]  Handle to the page.
/// @param id           [in]  ID of image to extract from IGR_Get_Page_Image_Entry.
/// @param destination  [in]  Path where image will be written.
/// @param error        [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if image not found
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Page_Image(HPAGE pageHandle, const IGR_UCS2* id, const IGR_UCS2* destination, Error_Control_Block* error);

/// @brief Extracts a page image to a stream.
///
/// @param pageHandle [in]  Handle to the page.
/// @param id         [in]  ID of image to extract from IGR_Get_Page_Image_Entry.
/// @param stream     [out] Stream to write extracted image content to.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
/// @return IGR_NO_MORE           if image not found
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Page_Image_Stream(HPAGE pageHandle, const IGR_UCS2* id, struct IGR_Stream** stream, Error_Control_Block* error);

/// @brief Gets the count of form elements on a page.
///
/// @param pageHandle [in]  Handle to the page.
/// @param count      [out] Receives number of form elements.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Count(HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);

/// @brief Gets form elements from a page.
///
/// @param pageHandle [in]     Handle to the page.
/// @param index      [in]     Zero-based index of first element to retrieve.
/// @param count      [in,out] On input: size of items array.
///                            On output: number of elements copied.
/// @param items      [out]    Array to receive form element data.
/// @param error      [out]    Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Elements(HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Page_Form_Element* items, Error_Control_Block* error);

/// @brief Gets a string value from a form element.
///
/// @param item       [in]  Form element from IGR_Get_Page_Form_Elements.
/// @param value_type [in]  Type of string value to retrieve.
/// @param char_count [in]  Size of buffer in characters.
/// @param buffer     [out] Buffer to receive string value.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Str(const struct IGR_Page_Form_Element* item, IGR_PAGE_FORM_ELEMENT_GET_TYPE value_type, IGR_LONG char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Gets a string value for a form element option.
///
/// @param item         [in]  Form element from IGR_Get_Page_Form_Elements.
/// @param value_type   [in]  Type of string value to retrieve.
/// @param option_index [in]  Zero-based index of option.
/// @param char_count   [in]  Size of buffer in characters.
/// @param buffer       [out] Buffer to receive string value.
/// @param error        [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Option_Str(const struct IGR_Page_Form_Element* item, IGR_PAGE_FORM_ELEMENT_GET_TYPE value_type, IGR_LONG option_index, IGR_LONG char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Enumerates all bookmarks up to a specified depth using a callback function.
///
/// @param docHandle     [in] Handle to a HD document.
/// @param root          [in] Optional bookmark value to enumerate from, or 
///                           NULL to enumerate from root.
/// @param recurse_depth [in] Maximum depth to scan.
/// @param callback      [in] Pointer to application-defined callback function.
/// @param context       [in] Application-defined value to be passed to callback function.
/// @param error         [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid.
/// @return IGR_E_INVALID_POINTER if stream, canvasHandle or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Enum_Bookmarks(IGR_LONG docHandle, const struct IGR_Bookmark* root, IGR_LONG recurse_depth, IGR_CALLBACK_BOOKMARK callback, void* context, Error_Control_Block* error);

/// @brief Retrieves the root bookmark item for the document.
///
/// @param docHandle [in] Handle to a HD document.
/// @param dest      [in] Pointer to application provided \ref IGR_Bookmark 
///                       that will be populated with bookmark data.
/// @param error     [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid.
/// @return IGR_E_INVALID_POINTER if stream, canvasHandle or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_Root(IGR_LONG docHandle, struct IGR_Bookmark* dest, Error_Control_Block* error);

/// @brief Retrieves the first child of the provided bookmark root.
///
/// @param docHandle [in] Handle to a HD document.
/// @param root      [in] Item to extract first child from, or NULL to get 
///                       first bookmark of document.
/// @param dest      [in] Pointer to application provided \ref IGR_Bookmark 
///                       that will be populated with bookmark data.
/// @param error     [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid.
/// @return IGR_E_INVALID_POINTER if stream, canvasHandle or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_First_Child(IGR_LONG docHandle, const struct IGR_Bookmark* root, struct IGR_Bookmark* dest, Error_Control_Block* error);

/// @brief Retrieves the next sibling of the provided bookmark root.
///
/// @param docHandle [in] Handle to a HD document.
/// @param root      [in] Item to extract next sibling from.
/// @param dest      [in] Pointer to application provided \ref IGR_Bookmark 
///                       that will be populated with bookmark data.
/// @param error     [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid.
/// @return IGR_E_INVALID_POINTER if stream, canvasHandle or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_Next_Sibling(IGR_LONG docHandle, const struct IGR_Bookmark* root, struct IGR_Bookmark* dest, Error_Control_Block* error);

/// @brief Returns a new canvas for rendering paginated output.
///
/// @param type         [in] Device type (IGR_DEVICE_IMAGE_PNG/JPG/PDF/TIF).
/// @param filename     [in] Filename to save output to.
/// @param options      [in] Additional rendering options.
/// @param canvasHandle [out] Handle to newly created output canvas, must be closed with
///                           IGR_Close_Canvas.
/// @param error        [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid.
/// @return IGR_E_INVALID_POINTER if stream, canvasHandle or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Output_Canvas(IGR_DEVICE_TYPE type, const IGR_UCS2* filename, const IGR_UCS2* options, HCANVAS* canvasHandle, Error_Control_Block* error);

/// @brief Returns a new canvas for rendering paginated output to a stream.
///
/// @param type         [in] Device type (IGR_DEVICE_IMAGE_PNG/JPG/PDF/TIF).
/// @param stream       [in] Stream to write output to.
/// @param options      [in] Additional rendering options.
/// @param canvasHandle [out] Handle to newly created output canvas, must be closed with
///                           IGR_Close_Canvas.
/// @param error        [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid.
/// @return IGR_E_INVALID_POINTER if stream, canvasHandle or error are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Output_Canvas_On(IGR_DEVICE_TYPE type, struct IGR_Writable_Stream* stream, const IGR_UCS2* options, HCANVAS* canvasHandle, Error_Control_Block* error);

/// @brief Closes the given canvas handle and frees resources.
///
/// @param canvasHandle [in]  Handle to canvas to close.
/// @param error        [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvasHandle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Close_Canvas(HCANVAS canvasHandle, Error_Control_Block* error);

/// @brief Renders a page onto a canvas, creating a new page for multi-page canvases.
///
/// @param page    [in]  Handle to a page obtained from IGR_Open_Page.
/// @param canvas  [in]  Handle to a canvas obtained from IGR_Make_Output_Canvas.
/// @param error   [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page or canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Render_Page(HPAGE page, HCANVAS canvas, Error_Control_Block* error);

/// @brief Renders a page onto a canvas with extended options and properties.
///
/// @param page       [in]  Handle to a page obtained from IGR_Open_Page.
/// @param canvas     [in]  Handle to a canvas obtained from IGR_Make_Output_Canvas.
/// @param options    [in]  Extended processing options as Name=Value pairs with semicolons.
/// @param properties [in]  Properties controlling per page rendering.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page or canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Render_Page_Ex(HPAGE page, HCANVAS canvas, const IGR_UCS2* options, const struct IGR_Render_Page_Properties* properties, Error_Control_Block* error);

/// @brief Redacts/removes words in the specified range from the document.
///
/// @param page  [in]  Handle to a page obtained from IGR_Open_Page.
/// @param From  [in]  Index of first word to redact.
/// @param To    [in]  Index of last word to redact.
/// @param Type  [in]  Reserved, must be zero.
/// @param error [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Redact_Page_Text(HPAGE page, IGR_LONG From, IGR_LONG To, IGR_LONG Type, Error_Control_Block* error);

/// @brief Gets page style/attribute information.
///
/// @param page   [in]      Handle to a page obtained from IGR_Open_Page.
/// @param name   [in]      Name of the style/attribute to retrieve.
/// @param buffer [out]     Buffer to receive the attribute/style value.
/// @param size   [in,out] On input: size of buffer in characters;
///                        On output: size of the returned string.
/// @param error  [out]    Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Attribute(HPAGE page, const IGR_UCS2* name, IGR_UCS2* buffer, IGR_LONG* size, Error_Control_Block* error);

/// @brief Gets information about a supported document format.
///
/// @param formatid [in]  Format ID returned by IGR_Get_File_Type.
/// @param what     [in]  Type of information to return:
///                      0 - Long format name
///                      1 - Short format name
///                      2 - Search index config format name
///                      3 - Format class
///                      4 - Legacy format indicator
/// @param buffer   [out] Buffer to receive text (min 255 bytes).
/// @param error    [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if formatid is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Format_Attribute(IGR_LONG formatid, IGR_LONG what, char* buffer, Error_Control_Block* error);

/// @brief Gets option attribute information for a format.
///
/// @param formatid   [in]  Format ID returned by IGR_Get_File_Type.
/// @param what       [in]  Type of information to return.
/// @param buffer     [out] Buffer to receive the attribute information.
/// @param bufferSize [in]  Size of buffer in bytes.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if formatid is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Option_Attribute(IGR_LONG formatid, IGR_LONG what, char* buffer, size_t bufferSize, Error_Control_Block* error);

/// @brief Draws an arc on the canvas using two sets of x,y coordinates.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate of starting point.
/// @param y      [in]  Y coordinate of starting point.
/// @param x2     [in]  X coordinate of ending point.
/// @param y2     [in]  Y coordinate of ending point.
/// @param x3     [in]  X coordinate of first arc point.
/// @param y3     [in]  Y coordinate of first arc point.
/// @param x4     [in]  X coordinate of second arc point.
/// @param y4     [in]  Y coordinate of second arc point.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Arc(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3, IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);

/// @brief Draws an arc on the canvas using center point, radius and angles.
///
/// @param canvas     [in]  Canvas handle to draw on.
/// @param x          [in]  X coordinate of center point.
/// @param y          [in]  Y coordinate of center point.
/// @param radius     [in]  Radius of the arc in pixels.
/// @param startAngle [in]  Starting angle in degrees.
/// @param sweepAngle [in]  Sweep angle in degrees.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_AngleArc(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG radius, IGR_LONG startAngle, IGR_LONG sweepAngle, Error_Control_Block* error);

/// @brief Draws a chord on the canvas using two sets of x,y coordinates.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate of starting point.
/// @param y      [in]  Y coordinate of starting point.
/// @param x2     [in]  X coordinate of ending point.
/// @param y2     [in]  Y coordinate of ending point.
/// @param x3     [in]  X coordinate of first chord point.
/// @param y3     [in]  Y coordinate of first chord point.
/// @param x4     [in]  X coordinate of second chord point.
/// @param y4     [in]  Y coordinate of second chord point.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Chord(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3, IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);

/// @brief Draws an ellipse on the canvas using bounding box coordinates.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate of top-left corner.
/// @param y      [in]  Y coordinate of top-left corner.
/// @param x2     [in]  X coordinate of bottom-right corner.
/// @param y2     [in]  Y coordinate of bottom-right corner.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Ellipse(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, Error_Control_Block* error);

/// @brief Draws a rectangle on the canvas using corner coordinates.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate of top-left corner.
/// @param y      [in]  Y coordinate of top-left corner.
/// @param x2     [in]  X coordinate of bottom-right corner.
/// @param y2     [in]  Y coordinate of bottom-right corner.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Rect(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, Error_Control_Block* error);

/// @brief Draws a line from current position to specified coordinates.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate of line endpoint.
/// @param y      [in]  Y coordinate of line endpoint.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_LineTo(HCANVAS canvas, IGR_LONG x, IGR_LONG y, Error_Control_Block* error);

/// @brief Moves current drawing position without drawing.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate to move to.
/// @param y      [in]  Y coordinate to move to.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_MoveTo(HCANVAS canvas, IGR_LONG x, IGR_LONG y, Error_Control_Block* error);

/// @brief Draws a pie shape on the canvas.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate of bounding rectangle top-left.
/// @param y      [in]  Y coordinate of bounding rectangle top-left.
/// @param x2     [in]  X coordinate of bounding rectangle bottom-right.
/// @param y2     [in]  Y coordinate of bounding rectangle bottom-right.
/// @param x3     [in]  X coordinate of start radial line.
/// @param y3     [in]  Y coordinate of start radial line.
/// @param x4     [in]  X coordinate of end radial line.
/// @param y4     [in]  Y coordinate of end radial line.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Pie(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3, IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);

/// @brief Draws a rounded rectangle on the canvas.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate of top-left corner.
/// @param y      [in]  Y coordinate of top-left corner.
/// @param x2     [in]  X coordinate of bottom-right corner.
/// @param y2     [in]  Y coordinate of bottom-right corner.
/// @param radius [in]  Radius of corner rounding in pixels.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_RoundRect(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG radius, Error_Control_Block* error);

/// @brief Outputs text at specified coordinates.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate for text position.
/// @param y      [in]  Y coordinate for text position.
/// @param text   [in]  UCS2 text string to output.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_TextOut(HCANVAS canvas, IGR_LONG x, IGR_LONG y, const IGR_UCS2* text, Error_Control_Block* error);

/// @brief Outputs text within a bounding rectangle.
///
/// @param canvas [in]  Canvas handle to draw on.
/// @param x      [in]  X coordinate of bounding rectangle top-left.
/// @param y      [in]  Y coordinate of bounding rectangle top-left.
/// @param x2     [in]  X coordinate of bounding rectangle bottom-right.
/// @param y2     [in]  Y coordinate of bounding rectangle bottom-right.
/// @param text   [in]  UCS2 text string to output.
/// @param flags  [in]  Text rectangle formatting flags.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_TextRect(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, const IGR_UCS2* text, IGR_TEXTRECT_TYPE flags, Error_Control_Block* error);

/// @brief Measures dimensions needed to draw text string.
///
/// @param canvas [in]     Canvas handle to measure on.
/// @param text   [in]     UCS2 text string to measure.
/// @param width  [in,out] Returns width needed in pixels.
/// @param height [in,out] Returns height needed in pixels.
/// @param error  [out]    Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_MeasureText(HCANVAS canvas, const IGR_UCS2* text, IGR_LONG* width, IGR_LONG* height, Error_Control_Block* error);

/// @brief Sets the pen for drawing outlines.
///
/// @param canvas [in]  Canvas handle to set pen for.
/// @param color  [in]  RGB color value for pen.
/// @param width  [in]  Width of pen in pixels.
/// @param style  [in]  Style of pen (solid, etc).
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetPen(HCANVAS canvas, IGR_LONG color, IGR_LONG width, IGR_PEN_TYPE style, Error_Control_Block* error);

/// @brief Sets the brush for filling shapes.
///
/// @param canvas [in]  Canvas handle to set brush for.
/// @param color  [in]  RGB color value for brush.
/// @param style  [in]  Style of brush (solid, etc).
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetBrush(HCANVAS canvas, IGR_LONG color, IGR_BRUSH_TYPE style, Error_Control_Block* error);

/// @brief Sets the font for text operations.
///
/// @param canvas     [in]  Canvas handle to set font for.
/// @param fontFamily [in]  Name of font family.
/// @param size      [in]  Size of font in points.
/// @param style     [in]  Font style flags (bold, italic, etc).
/// @param error     [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetFont(HCANVAS canvas, const IGR_UCS2* fontFamily, IGR_LONG size, IGR_TEXT_STYLE_TYPE style, Error_Control_Block* error);

/// @brief Sets global opacity level for drawing operations.
///
/// @param canvas  [in]  Canvas handle to set opacity for.
/// @param opacity [in]  Opacity level (0-255).
/// @param error   [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetOpacity(HCANVAS canvas, IGR_BYTE opacity, Error_Control_Block* error);

/// @brief Draws an image at specified coordinates.
///
/// @param canvas    [in]  Canvas handle to draw on.
/// @param x         [in]  X coordinate to draw at.
/// @param y         [in]  Y coordinate to draw at.
/// @param imagedata [in]  Raw image data bytes.
/// @param imagesize [in]  Size of image data in bytes.
/// @param mimetype  [in]  MIME type of image data.
/// @param error     [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_DrawImage(HCANVAS canvas, IGR_LONG x, IGR_LONG y, void* imagedata, size_t imagesize, const IGR_UCS2* mimetype, Error_Control_Block* error);

/// @brief Draws a scaled image within specified coordinates.
///
/// @param canvas    [in]  Canvas handle to draw on.
/// @param x         [in]  X coordinate of top-left corner.
/// @param y         [in]  Y coordinate of top-left corner.
/// @param x2        [in]  X coordinate of bottom-right corner.
/// @param y2        [in]  Y coordinate of bottom-right corner.
/// @param imagedata [in]  Raw image data bytes.
/// @param imagesize [in]  Size of image data in bytes.
/// @param mimetype  [in]  MIME type of image data.
/// @param error     [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_DrawScaleImage(HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, void* imagedata, size_t imagesize, const IGR_UCS2* mimetype, Error_Control_Block* error);

/// @brief Sets rotation angle for subsequent drawing operations.
///
/// @param canvas  [in]  Canvas handle to set rotation for.
/// @param degrees [in]  Rotation angle in degrees.
/// @param error   [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Rotation(HCANVAS canvas, IGR_LONG degrees, Error_Control_Block* error);

/// @brief Resets all canvas drawing settings to defaults.
///
/// @param canvas [in]  Canvas handle to reset.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Reset(HCANVAS canvas, Error_Control_Block* error);

/// @brief Adds annotations to canvas from JSON data.
///
/// @param canvas [in]  Canvas handle to add annotations to.
/// @param json   [in]  JSON string containing annotation data.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Annotate_JSON(HCANVAS canvas, const IGR_UCS2* json, Error_Control_Block* error);

/// @brief Gets count of hyperlinks on a page.
///
/// @param pageHandle [in]  Handle to the page.
/// @param count      [out] Returns number of hyperlinks.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlink_Count(HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);

/// @brief Gets hyperlink objects from a page.
///
/// @param pageHandle [in]     Handle to the page.
/// @param index      [in]     Index of first hyperlink to get.
/// @param count      [in,out] On input: size of items array
///                            On output: number of hyperlinks copied
/// @param items      [out]    Array to receive hyperlink data.
/// @param error      [out]    Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlinks(HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Hyperlink* items, Error_Control_Block* error);

/// @brief Gets a string value from a hyperlink.
///
/// @param item        [in]  Hyperlink object from IGR_Get_Page_Hyperlinks.
/// @param value_type  [in]  Type of string value to retrieve.
/// @param char_count  [in]  Size of buffer in characters.
/// @param buffer      [out] Buffer to receive string value.
/// @param error       [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if item is invalid
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlink_Str(const struct IGR_Hyperlink* item, IGR_HYPERLINK_GET_VALUE_TYPE value_type, IGR_LONG char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Inserts a blank page into the canvas.
///
/// @param canvas   [in]  Handle to canvas created by IGR_Make_Output_Canvas.
/// @param options  [in]  String providing extended processing options.
/// @param width    [in]  Width of the new page in pixels.
/// @param height   [in]  Height of the new page in pixels.
/// @param reserved [in]  Reserved for future use.
/// @param error    [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if docHandle is invalid
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Blank_Page(HCANVAS canvas, const IGR_UCS2* options, IGR_LONG width, IGR_LONG height, const void* reserved, Error_Control_Block* error);

/// @brief Opens a document from an OS based, device-dependent bitmap (Windows-Only).
///
/// @param bitmap       [in]  Handle to the OS bitmap (HBITMAP).
/// @param palette      [in]  Handle to optional OS palette (HPALETTE).
/// @param flags        [in]  Specifies processing options for IGR_Get_Text output.
/// @param options      [in]  Extended processing options for HTML conversion as Name=Value
///                          pairs with semicolon delimiter
/// @param capabilities [out] Returns Document Capabilities bitfield.
/// @param file_type   [out] Returns Document Format Code.
/// @param handle      [out] Returns handle for subsequent calls.
/// @param error       [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_ERROR           on invalid parameters.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_DDB(IGR_HBITMAP bitmap, IGR_HPALETTE palette, IGR_LONG flags, const IGR_UCS2* options, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* file_type, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Opens a document from raw pixel information.
///
/// @param bitmap       [in]  Provides details on bitmap information processing.
/// @param flags        [in]  Specifies processing options for IGR_Get_Text output.
/// @param options      [in]  Extended processing options for HTML conversion as Name=Value
///                           pairs with semicolon delimiter
/// @param capabilities [out] Returns Document Capabilities bitfield.
/// @param file_type    [out] Returns Document Format Code.
/// @param handle       [out] Returns handle for subsequent calls.
/// @param error        [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_ERROR           on invalid parameters.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_DIB(const struct IGR_Open_DIB_Info* bitmap, IGR_LONG flags, const IGR_UCS2* options, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* file_type, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Returns the count of annotations for a given page.
///
/// @param pageHandle [in]  Handle to page returned from IGR_Open_Page.
/// @param count     [out] Returns count of annotations on the page.
/// @param error     [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Count(HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);

/// @brief Returns annotation objects for a given page.
///
/// @param pageHandle [in]     Handle to page returned from IGR_Open_Page.
/// @param index      [in]     Offset to begin returning annotations from.
/// @param count      [in,out] Length of items array on input, number of items filled on
///                            output
/// @param items      [out]    Buffer to fill with up to count number of annotations.
/// @param error      [out]    Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotations(HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Annotation* items, Error_Control_Block* error);

/// @brief Returns a string value for a given annotation.
///
/// @param item       [in]     \ref IGR_Annotation pointer from IGR_Get_Page_Annotations.
/// @param path       [in]     Path of the annotation property to return.
/// @param char_count [in,out] Size of buffer in chars on input, length of string on output.
/// @param buffer     [out]    Buffer to fill with string value.
/// @param error      [out]    Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Str(const struct IGR_Annotation* item, const IGR_UCS2* path, IGR_LONG* char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Returns an integer value for a given annotation.
///
/// @param item   [in]  \ref IGR_Annotation pointer from IGR_Get_Page_Annotations.
/// @param path   [in]  Path of the annotation property to return.
/// @param buffer [out] Buffer to fill with integer value.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Long(const struct IGR_Annotation* item, const IGR_UCS2* path, IGR_LONG* buffer, Error_Control_Block* error);

/// @brief Returns the pixel data for the given page.
///
/// @param pageHandle   [in]     Handle to the page to get pixels from.
/// @param sourceRect   [in]     Rectangle to extract, or NULL for entire page.
/// @param destSize     [in]     Size of output, or NULL for default size.
/// @param flags        [in]     Processing flags.
/// @param options      [in]     Additional processing options.
/// @param pixel_format [in]     Format of pixel data to extract.
/// @param pixel_data   [in,out] Structure to populate with pixel data.
/// @param error        [out]    Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if pageHandle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Pixels(
	HPAGE pageHandle,
	const struct IGR_Rect* sourceRect,
	const struct IGR_Size* destSize,
	IGR_ULONG flags,
	const IGR_UCS2* options,
	IGR_OPEN_BITMAP_PIXEL_TYPE pixel_format,
	struct IGR_Page_Pixels* pixel_data,
	Error_Control_Block* error);

/// @brief Releases pixel data previously allocated by IGR_Get_Page_Pixels.
///
/// @param pixel_data [in,out] Structure populated with pixel data to be freed.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Free_Page_Pixels(
	struct IGR_Page_Pixels* pixel_data,
	Error_Control_Block* error);

/// @brief Removes all existing bookmarks from a canvas.
///
/// @param canvas [in] Handle to the canvas to clear bookmarks from.
/// @param error  [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if error is invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Bookmarks_Clear(HCANVAS canvas, Error_Control_Block* error);

/// @brief Appends a single bookmark to a canvas.
///
/// @param canvas   [in] Handle to the canvas to append bookmark to.
/// @param bookmark [in] Pointer to \ref IGR_Bookmark data to add.
/// @param error    [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if canvas handle is invalid.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Bookmarks_Append(HCANVAS canvas, const struct IGR_Bookmark* bookmark, Error_Control_Block* error);

/// @brief Callback function for document processing events.
///
/// @param action  [in] Type of action that prompted the callback.
/// @param payload [in] Action-specific structure containing event data.
/// @param context [in] User-provided context data passed to IGR_Open_Ex.
///
/// @return IGR_LONG Status code
typedef IGR_LONG(IGR_EXPORT* IGR_OPEN_CALLBACK)(IGR_OPEN_CALLBACK_ACTION action, void* payload, void* context);

/// @brief Opens a document for text extraction or sub-document enumeration with extended
///        options
/// @param source_type      [in] Type of source (filename or stream).
/// @param source           [in] Pointer to source based on source_type.
/// @param flags            [in] Processing options controlling output format.
/// @param options          [in] Extended options as Name=Value pairs.
/// @param capabilities     [out] Returns document capabilities bitfield.
/// @param file_type        [out] Returns document format code.
/// @param reserved         [in] Must be NULL, reserved for future use.
/// @param callback         [in] Event callback function.
/// @param callback_context [in] User context passed to callback.
/// @param handle           [out] Handle for subsequent API calls.
/// @param error            [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if required parameters are invalid
IGR_RETURN_CODE IGR_EXPORT IGR_Open_Ex(
	IGR_OPEN_FROM source_type,
	const void* source,
	IGR_LONG flags,
	const IGR_UCS2* options,
	IGR_FILE_SUPPORTS_TYPE* capabilities,
	IGR_LONG* file_type,
	void* reserved,
	IGR_OPEN_CALLBACK callback,
	void* callback_context,
	IGR_LONG* handle,
	Error_Control_Block* error);

/// @brief Gets page elements from a parent element.
///
/// @param page       [in] Handle to page containing elements.
/// @param parent     [in] Optional parent element to get children from.
/// @param first      [in] Index of first element to extract.
/// @param dest_count [in,out] Size of dest array on input, number filled on output.
/// @param dest       [in,out] Array of \ref IGR_Page_Element to fill with data.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Elements(IGR_HPAGE page, const struct IGR_Page_Element* parent, IGR_ULONG first, IGR_ULONG* dest_count, struct IGR_Page_Element* dest, Error_Control_Block* error);

/// @brief Gets the root element of a page.
///
/// @param page  [in] Handle to page to get root element from.
/// @param dest  [in,out] Pointer to \ref IGR_Page_Element to fill.
/// @param error [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Root(IGR_HPAGE page, struct IGR_Page_Element* dest, Error_Control_Block* error);

/// @brief Fills dest with the first child of the parent element.
///
/// @param page    [in] Page element containing the page elements to extract.
/// @param parent  [in] Optional parent element from which children are extracted.
/// @param dest    [in,out] Pointer to \ref IGR_Page_Element struct. On input, struct_size
///                         must be set. On output, filled with element data
/// @param error   [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_First_Child(IGR_HPAGE page, const struct IGR_Page_Element* parent, struct IGR_Page_Element* dest, Error_Control_Block* error);

/// @brief Fills dest with the next sibling node of the element.
///
/// @param page    [in] Page element containing the page elements to extract.
/// @param element [in] Element whose sibling should be extracted.
/// @param dest    [in,out] Pointer to \ref IGR_Page_Element struct. On input, struct_size
///                         must be set. On output, filled with element data
/// @param error   [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Next_Sibling(IGR_HPAGE page, const struct IGR_Page_Element* element, struct IGR_Page_Element* dest, Error_Control_Block* error);

/// @brief Gets text associated with a page element.
///
/// @param page       [in] Page element containing elements to extract from.
/// @param source     [in] Element to extract text from.
/// @param char_count [in,out] On input, size of buffer in chars. On output, chars copied.
/// @param buffer     [in,out] UCS2 array to receive text.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Text(IGR_HPAGE page, const struct IGR_Page_Element* source, IGR_ULONG* char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Calls callback with each style for the given source element.
///
/// @param page     [in] Page element containing elements to extract from.
/// @param source   [in] Element to extract styles from.
/// @param callback [in] Function to receive style information.
/// @param context  [in] User data passed to callback.
/// @param error    [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Styles(IGR_HPAGE page, const struct IGR_Page_Element* source, IGR_PAGE_ELEMENT_STYLES_CALLBACK callback, void* context, Error_Control_Block* error);

/// @brief Gets named style value from a page element.
///
/// @param page       [in] Page element containing elements to extract from.
/// @param source     [in] Element to get style from.
/// @param name       [in] Name of style to retrieve.
/// @param char_count [in,out] On input, size of buffer. On output, chars copied.
/// @param buffer     [in,out] UCS2 array to receive style value.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Style(IGR_HPAGE page, const struct IGR_Page_Element* source, const IGR_UCS2* name, IGR_ULONG* char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Recursively walks page elements calling callback for each element.
///
/// @param page      [in] Page containing elements to enumerate.
/// @param source    [in] Root element to start enumeration from.
/// @param flags     [in] Reserved for future use.
/// @param max_depth [in] Maximum depth of page elements to traverse.
/// @param callback  [in] Function called for each element.
/// @param context   [in] User data passed to callback.
/// @param error     [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_HANDLE  if page handle is invalid
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Enum_Page_Elements(IGR_HPAGE page, const struct IGR_Page_Element* source, IGR_ULONG flags, IGR_ULONG max_depth, IGR_PAGE_ELEMENT_CALLBACK callback, void* context, Error_Control_Block* error);

/// @brief Gets the type and capabilities of an open document.
///
/// @param handle       [in] Handle to an open document.
/// @param capabilities [out] Returns Document Capabilities bitfield.
/// @param filetype     [out] Returns Document Format Code.
/// @param error        [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Handle_File_Type(IGR_HDOC handle, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype, Error_Control_Block* error);

/// @brief Gets the type and capabilities of an open page.
///
/// @param handle       [in] Handle to an open page.
/// @param capabilities [out] Returns Document Capabilities bitfield.
/// @param filetype     [out] Returns Document Format Code.
/// @param error        [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Handle_File_Type(IGR_HPAGE handle, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype, Error_Control_Block* error);

/// @brief Compares two documents and returns enumerator for differences.
///
/// @param doc1       [in] First document to compare.
/// @param doc2       [in] Second document to compare.
/// @param settings   [in] Settings controlling text comparison.
/// @param enumerator [out] Handle to enumerator for iterating differences.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Documents(
	const struct IGR_Text_Compare_Document_Source* doc1,
	const struct IGR_Text_Compare_Document_Source* doc2,
	const struct IGR_Text_Compare_Settings* settings,
	IGR_HTEXTCOMPARE* enumerator,
	Error_Control_Block* error);

/// @brief Compares two pages and returns enumerator for differences.
///
/// @param page1         [in] First page to compare.
/// @param page1_margins [in] Margins for first page.
/// @param page2         [in] Second page to compare.
/// @param page2_margins [in] Margins for second page.
/// @param settings      [in] Settings controlling text comparison.
/// @param enumerator    [out] Handle to enumerator for iterating differences.
/// @param error         [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Pages(
	IGR_HPAGE page1,
	const struct IGR_FRect* page1_margins,
	IGR_HPAGE page2,
	const struct IGR_FRect* page2_margins,
	const struct IGR_Text_Compare_Settings* settings,
	IGR_HTEXTCOMPARE* enumerator,
	Error_Control_Block* error);

/// @brief Compares elements within pages and returns enumerator for differences.
///
/// @param page1          [in] First page containing elements.
/// @param page1_ele_root [in] Root element from first page.
/// @param page2          [in] Second page containing elements.
/// @param page2_ele_root [in] Root element from second page.
/// @param settings       [in] Settings controlling text comparison.
/// @param enumerator     [out] Handle to enumerator for iterating differences.
/// @param error          [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Elements(
	IGR_HPAGE page1,
	const struct IGR_Page_Element* page1_ele_root,
	IGR_HPAGE page2,
	const struct IGR_Page_Element* page2_ele_root,
	const struct IGR_Text_Compare_Settings* settings,
	IGR_HTEXTCOMPARE* enumerator,
	Error_Control_Block* error);

/// @brief Closes text comparison enumerator and frees resources.
///
/// @param enumerator [in] Handle to enumerator to close.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Close(IGR_HTEXTCOMPARE enumerator, Error_Control_Block* error);

/// @brief Resets text comparison enumerator to initial state.
///
/// @param enumerator [in] Handle to enumerator to reset.
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Reset(IGR_HTEXTCOMPARE enumerator, Error_Control_Block* error);

/// @brief Retrieves the next difference from the text comparison enumerator.
///
/// @param enumerator [in]  Handle to the text comparison enumerator.
/// @param result     [out] Pointer to \ref IGR_Compare_Documents_Difference structure to
///                         store the next difference
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Next(IGR_HTEXTCOMPARE enumerator, struct IGR_Compare_Documents_Difference* result, Error_Control_Block* error);

/// @brief Disposes of a difference result returned by the text comparison enumerator.
///
/// @param result     [in]  Pointer to \ref IGR_Compare_Documents_Difference structure to
///                         dispose of
/// @param error      [out] Contains any error text.
///
/// @return IGR_OK                on success.
/// @return IGR_E_INVALID_POINTER if parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Difference_Dispose(struct IGR_Compare_Documents_Difference* result, Error_Control_Block* error);

#ifdef __cplusplus
};
#endif

#ifndef PERCEPTIVE_DOCFILTERS_H_DEF
#define PERCEPTIVE_DOCFILTERS_H_DEF
#endif

#endif // HYLAND_DOCFILTERS_H_DEF
