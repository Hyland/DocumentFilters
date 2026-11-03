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

#include "DocumentFiltersFormats.h"

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ****************************************************************************
// * Platform and architecture specific definitions
// ****************************************************************************
/// @cond
#if defined(_MSC_VER)
#	define DOCUMENT_FILTERS_PLATFORM_WINDOWS
#	define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__linux__) && defined(__powerpc) && !defined(_LITTLE_ENDIAN)
#	define DOCUMENT_FILTERS_PLATFORM_LINUX
#	define DOCUMENT_FILTERS_PLATFORM_BIG_ENDIAN
#elif defined(__linux__)
#	define DOCUMENT_FILTERS_PLATFORM_LINUX
#	define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__FreeBSD__)
#	define DOCUMENT_FILTERS_PLATFORM_FREEBSD
#	define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__APPLE__)
#	define DOCUMENT_FILTERS_PLATFORM_MACOSX
#	define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__sun) && defined(__sparc)
#	define DOCUMENT_FILTERS_PLATFORM_SOLARIS
#	define DOCUMENT_FILTERS_PLATFORM_SOLARIS_SPARC
#	define DOCUMENT_FILTERS_PLATFORM_BIG_ENDIAN
#elif defined(__sun)
#	define DOCUMENT_FILTERS_PLATFORM_SOLARIS
#	define DOCUMENT_FILTERS_PLATFORM_SOLARIS_INTEL
#	define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#elif defined(__hpux)
#	define DOCUMENT_FILTERS_PLATFORM_HPUX
#	define DOCUMENT_FILTERS_PLATFORM_BIG_ENDIAN
#elif defined(_AIX)
#	define DOCUMENT_FILTERS_PLATFORM_AIX
#	define DOCUMENT_FILTERS_PLATFORM_BIG_ENDIAN
#elif defined(__wasm32__)
#	define DOCUMENT_FILTERS_PLATFORM_WASM
#	define DOCUMENT_FILTERS_PLATFORM_LITTLE_ENDIAN
#else
#	error "Document Filters: Unsupported platform"
#endif
/// @endcond

// ****************************************************************************
// * Simple types definitions
// ****************************************************************************
/**
 * @defgroup simple_types Basic Portable Types
 * @brief Fixed-width and portable type aliases used throughout the Document
 *        Filters API.
 *
 * These typedefs provide a consistent, platform-independent set of basic
 * types for use in the public C API, decoupling callers from
 * platform-specific type sizes.
 * @{
 */

#ifdef _WIN32
/// @brief UCS-2 (16-bit Unicode) character type.
typedef wchar_t IGR_UCS2;
/// @brief Signed 32-bit integer type.
typedef long IGR_LONG;
/// @brief Unsigned 32-bit integer type.
typedef unsigned long IGR_ULONG;
#else
/// @brief UCS-2 (16-bit Unicode) character type.
typedef uint16_t IGR_UCS2;
/// @brief Signed 32-bit integer type.
typedef int32_t IGR_LONG;
/// @brief Unsigned 32-bit integer type.
typedef uint32_t IGR_ULONG;
#endif

/// @brief Unsigned 8-bit integer (byte) type.
typedef unsigned char IGR_BYTE;

/// @brief Signed 16-bit integer type.
typedef short int IGR_SHORT;

/// @brief Unsigned 16-bit integer type.
typedef unsigned short int IGR_USHORT;

/// @brief Signed 64-bit integer type.
typedef int64_t IGR_LONGLONG;

/// @brief Unsigned 64-bit integer type.
typedef uint64_t IGR_ULONGLONG;

/// @brief Generic opaque handle type.
typedef void* IGR_HANDLE;

/// @brief Single-precision floating point type.
typedef float IGR_FLOAT;

/// @brief Double-precision floating point type.
typedef double IGR_DOUBLE;

/** @} */ // simple_types

/// @cond
#if defined(DOCUMENT_FILTERS_PLATFORM_WINDOWS)
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	define IGR_EXPORT   WINAPI
#	define IGR_HBITMAP  HBITMAP
#	define IGR_HPALETTE HPALETTE
#else
#	define IGR_EXPORT
#	define CALLBACK
#	define IGR_HBITMAP  void*
#	define IGR_HPALETTE void*
#	ifndef WCHAR
#		define WCHAR IGR_UCS2
#	endif
#	ifndef BOOL
#		define BOOL int
#	endif
#	ifndef SHORT
#		define SHORT IGR_SHORT
#	endif
#	ifndef USHORT
#		define USHORT IGR_USHORT
#	endif
#	ifndef LPCWSTR
#		define LPCWSTR const WCHAR*
#	endif
#	ifndef LONG
#		define LONG IGR_LONG
#	endif
#	ifndef ULONG
#		define ULONG IGR_ULONG
#	endif
#	ifndef BYTE
#		define BYTE IGR_BYTE
#	endif
#	ifndef LONGLONG
#		define LONGLONG IGR_LONGLONG
#	endif
#	ifndef ULONGLONG
#		define ULONGLONG IGR_ULONGLONG
#	endif
#	ifndef PLONGLONG
#		define PLONGLONG *LONGLONG
#	endif
#	ifndef HANDLE
#		define HANDLE IGR_HANDLE
#	endif
#	ifndef HINSTANCE
#		define HINSTANCE IGR_HANDLE
#	endif
#	ifndef HMODULE
#		define HMODULE IGR_HANDLE
#	endif
#	ifndef HWND
#		define HWND IGR_HANDLE
#	endif
#	ifndef LPWSTR
#		define LPWSTR WCHAR*
#	endif
#	ifndef LPSTR
#		define LPSTR char*
#	endif
#	ifndef LPCSTR
#		define LPCSTR const LPSTR
#	endif
#endif

#ifndef IGR_EXPORT
#	define IGR_EXPORT
#endif
/// @endcond

/// @brief Page handle type.
typedef IGR_LONG IGR_HPAGE;

/// @brief Canvas handle type for rendering page output.
typedef IGR_LONG IGR_HCANVAS;

/// @brief Document handle type.
typedef IGR_LONG IGR_HDOC;

/// @cond
/// @brief Alias for \ref IGR_HPAGE page handle type.
typedef IGR_HPAGE HPAGE;

/// @brief Alias for \ref IGR_HCANVAS canvas handle type.
typedef IGR_HCANVAS HCANVAS;
/// @endcond

// ****************************************************************************
// * Constants
// ****************************************************************************

// ****************************************************************************
// * IGR_RETURN_CODE type and constants

/**
 * @defgroup error_codes Document Filters status codes
 * @brief Status and error codes returned by Document Filters operations.
 *
 * Functions returning an `IGR_RETURN_CODE` return `IGR_OK` on success or one
 * of the error codes in this group on failure.
 *
 * @{
 */

/// @brief Return type for API functions.
typedef IGR_LONG IGR_RETURN_CODE;

/// @brief Operation completed successfully.
#define IGR_OK                         0

/// @brief Error opening the file/resource.
#define IGR_E_OPEN_ERROR               1

/// @brief Wrong type was used for operation.
#define IGR_E_WRONG_TYPE               2

/// @brief Resource is already in use.
#define IGR_E_IN_USE                   3

/// @brief Resource is not readable.
#define IGR_E_NOT_READABLE             4

/// @brief Password required or incorrect password provided.
#define IGR_E_PASSWORD                 5

/// @brief Resource was not found.
#define IGR_E_NOT_FOUND                10

/// @brief Error writing to file/resource.
#define IGR_E_WRITE_ERROR              11

/// @brief Operation not valid for this class/type.
#define IGR_E_NOT_VALID_FOR_THIS_CLASS 12

/// @brief General error occurred.
#define IGR_E_ERROR                    13

/// @brief Invalid handle was provided.
#define IGR_E_INVALID_HANDLE           14

/// @brief Invalid pointer was provided.
#define IGR_E_INVALID_POINTER          15

/// @brief Invalid parameter was provided.
#define IGR_E_INVALID_PARAMETER        16

/// @brief File is corrupt or truncated.
#define IGR_E_FILE_CORRUPT             17

/// @brief No more data available.
#define IGR_NO_MORE                    19

/// @brief Out of memory error.
#define IGR_E_OUT_OF_MEMORY            20

/// @brief Severe error occurred.
#define IGR_E_BAD_ERROR                21

/// @brief Operation called from wrong thread.
#define IGR_E_WRONG_THREAD             22

/// @brief Too many handles are open.
#define IGR_E_TOO_MANY_HANDLES         23

/// @brief Required fonts were not found.
#define IGR_E_FONTS_NOT_FOUND          24

/// @brief Version mismatch between components.
#define IGR_E_VERSION_MISMATCH         25

/// @brief Requested action is not supported.
#define IGR_E_ACTION_NOT_SUPPORTED     26

/// @brief Operation was cancelled.
#define IGR_CANCELLED                  27

/// @brief Document is DRM protected, metadata may be available.
#define IGR_E_DRM_PROTECTED            28

/** @} */

/**
 * @defgroup digest_constants Message Digest Buffer Size Constants
 * @brief Size constants for message digest (hash) result buffers.
 *
 * These constants define the required buffer sizes for storing hexadecimal
 * representations of cryptographic hash values, including space for the null
 * terminator.
 * @{
 */

/// @brief Length of an MD5 hex string including null terminator.
#define PERCEPTIVE_SZ_MD5_HEX_LEN      33

/// @brief Length of a SHA1 hex string including null terminator.
#define PERCEPTIVE_SZ_SHA1_HEX_LEN     41

/** @} */ // digest_constants

/**
 * @defgroup legacy_multi_codes Legacy Multi-Function Codes
 * @brief Deprecated codes for ISYS_Multiplex() function.
 *
 * These constants are part of the legacy multi-conversion API and are
 * maintained for backward compatibility. New code should use the modern
 * conversion APIs.
 * @{
 */

/// @brief Legacy multi-function code to set code page.
#define IGR_Multi_Set_Code_Page        1

/// @brief Legacy multi-function code to set temporary path.
#define IGR_Multi_Set_Temp_Path        186

/** @} */ // legacy_multi_codes

/**
 * @defgroup stream_action_constants Stream Action Constants
 * @brief Action identifiers for stream operation callbacks.
 *
 * These constants are used with IGR_Make_Stream_Action_Function callbacks to
 * identify specific operations requested on a stream.
 * @{
 */

/// @brief IGR_ACTION_GET_STREAM_PART is used to indicate an action to
///        retrieve part of a stream. Used with @ref
///        IGR_T_ACTION_GET_STREAM_PART in callbacks.
#define IGR_ACTION_GET_STREAM_PART     10

/** @} */ // stream_action_constants

// ****************************************************************************
// * IGR_ANNOTATION type and constants

/**
 * @defgroup annotation_types Annotation Types
 * @brief Enumeration of document annotation types (e.g., highlights,
 *        comments, redactions).
 *
 * These constants define the various types of annotations that can appear in
 * documents.
 * @{
 */

/// @brief Type definition for annotation types.
typedef IGR_LONG IGR_ANNOTATION_TYPE;

/// @brief Unknown annotation type.
#define IGR_ANNOTATION_UNKNOWN           0

/// @brief Text annotation.
#define IGR_ANNOTATION_TEXT              1

/// @brief Link annotation.
#define IGR_ANNOTATION_LINK              2

/// @brief Free text annotation.
#define IGR_ANNOTATION_FREETEXT          3

/// @brief Line annotation.
#define IGR_ANNOTATION_LINE              4

/// @brief Square/rectangle annotation.
#define IGR_ANNOTATION_SQUARE            5

/// @brief Circle/ellipse annotation.
#define IGR_ANNOTATION_CIRCLE            6

/// @brief Polygon annotation.
#define IGR_ANNOTATION_POLYGON           7

/// @brief Polyline annotation.
#define IGR_ANNOTATION_POLYLINE          8

/// @brief Highlight annotation.
#define IGR_ANNOTATION_HIGHLIGHT         9

/// @brief Underline annotation.
#define IGR_ANNOTATION_UNDERLINE         10

/// @brief Squiggly underline annotation.
#define IGR_ANNOTATION_SQUIGGLE          11

/// @brief Strikeout annotation.
#define IGR_ANNOTATION_STRIKEOUT         12

/// @brief Stamp annotation.
#define IGR_ANNOTATION_STAMP             13

/// @brief Caret annotation.
#define IGR_ANNOTATION_CARET             14

/// @brief Ink annotation.
#define IGR_ANNOTATION_INK               15

/// @brief Popup annotation.
#define IGR_ANNOTATION_POPUP             16

/// @brief File attachment annotation.
#define IGR_ANNOTATION_FILEATTACHMENT    17

/// @brief Sound annotation.
#define IGR_ANNOTATION_SOUND             18

/// @brief Movie/video annotation.
#define IGR_ANNOTATION_MOVIE             19

/// @brief Form widget annotation.
#define IGR_ANNOTATION_WIDGET            20

/// @brief Screen annotation.
#define IGR_ANNOTATION_SCREEN            21

/// @brief Printer mark annotation.
#define IGR_ANNOTATION_PRINTERMARK       22

/// @brief Trap network annotation.
#define IGR_ANNOTATION_TRAPNET           23

/// @brief Watermark annotation.
#define IGR_ANNOTATION_WATERMARK         24

/// @brief 3D annotation.
#define IGR_ANNOTATION_3D                25

/// @brief Barcode annotation.
#define IGR_ANNOTATION_BARCODE           26

/// @brief Named destination annotation.
#define IGR_ANNOTATION_NAMED_DESTINATION 27

/** @} */ // annotation_types

// ****************************************************************************
// * IGR_ANNOTATION_FLAGS type and constants

/**
 * @defgroup annotation_flags Annotation Flags
 * @brief Bit flags controlling annotation display and behavior properties.
 *
 * These constants define bitfield flags that control how annotations are
 * displayed and what operations are allowed on them.
 * @{
 */

/// @brief Type definition for annotation flags.
typedef IGR_LONG IGR_ANNOTATION_FLAGS_TYPE;

/// @brief Annotation is invisible.
#define IGR_ANNOTATION_FLAGS_INVISIBLE      (1 << 0)

/// @brief Annotation is hidden.
#define IGR_ANNOTATION_FLAGS_HIDDEN         (1 << 1)

/// @brief Annotation should be printed.
#define IGR_ANNOTATION_FLAGS_PRINT          (1 << 2)

/// @brief Annotation should not be zoomed.
#define IGR_ANNOTATION_FLAGS_NOZOOM         (1 << 3)

/// @brief Annotation should not be rotated.
#define IGR_ANNOTATION_FLAGS_NOROTATE       (1 << 4)

/// @brief Annotation should not be viewed.
#define IGR_ANNOTATION_FLAGS_NOVIEW         (1 << 5)

/// @brief Annotation is read-only.
#define IGR_ANNOTATION_FLAGS_READONLY       (1 << 6)

/// @brief Annotation is locked.
#define IGR_ANNOTATION_FLAGS_LOCKED         (1 << 7)

/// @brief Toggle no view state.
#define IGR_ANNOTATION_FLAGS_TOGGLENOVIEW   (1 << 8)

/// @brief Annotation contents are locked.
#define IGR_ANNOTATION_FLAGS_LOCKEDCONTENTS (1 << 9)

/** @} */ // annotation_flags

// ****************************************************************************
// * IGR_BOOKMARK_ACTION type and constants

/**
 * @defgroup bookmark_action_types Bookmark Action Types
 * @brief Enumeration of bookmark action types.
 *
 * These constants define the types of actions that bookmarks can trigger.
 * @{
 */

/// @brief Type definition for bookmark action types.
typedef IGR_LONG IGR_BOOKMARK_ACTION_TYPE;

/// @brief Goto action for bookmark.
#define IGR_BOOKMARK_ACTION_GOTO 0

/// @brief Link action for bookmark.
#define IGR_BOOKMARK_ACTION_LINK 1

/** @} */ // bookmark_action_types

// ****************************************************************************
// * IGR_BOOKMARK_FIT type and constants

/**
 * @defgroup bookmark_fit_types Bookmark Fit Types
 * @brief Enumeration of bookmark view fit modes.
 *
 * These constants define how to fit content when navigating to a bookmark.
 * @{
 */

/// @brief Type definition for bookmark fit modes.
typedef IGR_LONG IGR_BOOKMARK_FIT_TYPE;

/// @brief No fit mode specified.
#define IGR_BOOKMARK_FIT_NONE   0

/// @brief Fit both width and height.
#define IGR_BOOKMARK_FIT_BOTH   1

/// @brief Fit width only.
#define IGR_BOOKMARK_FIT_WIDTH  2

/// @brief Fit height only.
#define IGR_BOOKMARK_FIT_HEIGHT 3

/// @brief Fit to rectangle.
#define IGR_BOOKMARK_FIT_RECT   4

/** @} */ // bookmark_fit_types

// ****************************************************************************
// * IGR_BRUSH type and constants

/**
 * @defgroup brush_types Canvas Brush Types
 * @brief Enumeration of brush styles for canvas drawing.
 *
 * These constants define the styles available for canvas brush fills.
 * @{
 */

/// @brief Canvas brush type definition.
typedef IGR_LONG IGR_BRUSH_TYPE;

/// @brief No brush style.
#define IGR_BRUSH_NONE  0x00

/// @brief Solid brush style.
#define IGR_BRUSH_SOLID 0x01

/** @} */ // brush_types

// ****************************************************************************
// * IGR_CHAR_TYPE and constants

/**
 * @defgroup char_codes Character Code Constants
 * @brief Special character codes and text style indicators.
 *
 * These constants represent special character codes used in text processing,
 * including formatting markers, whitespace indicators, and style codes.
 * @{
 */

/// @brief Type definition for character type, using UCS2 encoding.
typedef IGR_UCS2 IGR_CHAR_TYPE;

/// @brief Special character code indicating a soft space character.
#define IGR_CHAR_SOFT_SPACE       1

/// @brief Special character code for normal text style.
#define IGR_CHAR_STYLE_NORMAL     2

/// @brief Special character code for bold text style.
#define IGR_CHAR_STYLE_BOLD       3

/// @brief Special character code for italics text style.
#define IGR_CHAR_STYLE_ITALICS    4

/// @brief Special character code for underlined text style.
#define IGR_CHAR_STYLE_UNDERLINED 5

/// @brief Special character code for tab character.
#define IGR_CHAR_TAB              9

/// @brief Special character code for page break character.
#define IGR_CHAR_PAGE_BREAK       12

/// @brief Special character code for line break character.
#define IGR_CHAR_LINE_BREAK       13

/// @brief Special character code for paragraph break character.
#define IGR_CHAR_PARA_BREAK       14

/// @brief Special character code marking start of metadata.
#define IGR_CHAR_START_META       15

/// @brief Special character code marking end of metadata.
#define IGR_CHAR_END_META         16

/** @} */ // char_codes

// ****************************************************************************
// * IGR_COLOR type, constants and macros

/**
 * @defgroup color_types Color Type and Macros
 * @brief Color value definitions and utility macros.
 *
 * These definitions support ARGB color specification for canvas rendering.
 * @{
 */

/// @brief Type definition for color values.
typedef IGR_ULONG IGR_COLOR_TYPE;

/// @brief Creates an RGB color value.
///
/// @param R Red color component (0-255).
/// @param G Green color component (0-255).
/// @param B Blue color component (0-255).
///
/// @return Combined RGB color value
#define MAKE_COLOR(R, G, B)                   ((int)R | ((int)G << 8) | ((int)B << 16))

/// @brief Creates an ARGB color value with alpha.
///
/// @param A Alpha transparency component (0-255).
/// @param R Red color component (0-255).
/// @param G Green color component (0-255).
/// @param B Blue color component (0-255).
///
/// @return Combined ARGB color value
#define IGR_MAKE_COLOR_WITH_ALPHA(A, R, G, B) (((int)A << 24) | ((int)R << 16) | ((int)G << 8) | ((int)B << 0))

/// @brief Creates an RGB color value with full opacity.
///
/// @param R Red color component (0-255).
/// @param G Green color component (0-255).
/// @param B Blue color component (0-255).
///
/// @return Combined RGB color value with alpha=255
#define IGR_MAKE_COLOR(R, G, B)               IGR_MAKE_COLOR_WITH_ALPHA(0xff, R, G, B)

/** @} */ // color_types

// ****************************************************************************
// * IGR_COMPARE_DOCUMENTS_DIFFERENCE type and constants

/**
 * @defgroup document_difference_types Document Difference Types
 * @brief Enumeration of difference types found during document comparison.
 *
 * These constants identify what type of difference was found between
 * documents.
 * @{
 */

/// @brief Type definition for difference types between compared documents.
typedef IGR_ULONG IGR_COMPARE_DOCUMENTS_DIFFERENCE_TYPE;

/// @brief No difference between compared documents.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL      0

/// @brief Content was inserted in the revised document.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT     1

/// @brief Content was deleted from the original document.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE     2

/// @brief Formatting changes between documents.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING 3

/// @brief More differences are available in next batch.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH 0xFF

/** @} */ // document_difference_types

// ****************************************************************************
// * IGR_COMPARE_DOCUMENTS_FLAGS type and constants

/**
 * @defgroup document_comparison_flags Document Comparison Flags
 * @brief Bit flags controlling document comparison behavior and what to
 *        detect.
 *
 * These flags control which elements are compared and what types of
 * differences to detect.
 * @{
 */

/// @brief Type definition for document comparison flags.
typedef IGR_ULONG IGR_COMPARE_DOCUMENTS_FLAGS_TYPE;

/// @brief Include equal/matching content in results.
#define IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS             0x1

/// @brief Detect moved content between documents.
#define IGR_COMPARE_DOCUMENTS_FLAGS_MOVES              0x10

/// @brief Detect formatting changes between documents.
#define IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING         0x20

/// @brief Ignore comments when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS        0x40

/// @brief Ignore case when comparing text.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE            0x80

/// @brief Ignore whitespace when comparing text.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE      0x100

/// @brief Ignore punctuation when comparing text.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION     0x200

/// @brief Ignore table content when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES          0x400

/// @brief Ignore headers when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS         0x800

/// @brief Ignore footers when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS         0x1000

/// @brief Ignore footnotes when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES       0x2000

/// @brief Ignore textbox content when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES       0x4000

/// @brief Ignore field content when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS          0x8000

/// @brief Ignore both headers and footers when comparing.
#define IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS_FOOTERS (IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS | IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS)

/** @} */ // document_comparison_flags

// ****************************************************************************
// * IGR_COMPARE_DOCUMENTS_COMPARE type and constants

/**
 * @defgroup document_compare_methods Document Comparison Methods
 * @brief Enumeration of comparison granularity methods.
 * @{
 */

/// @brief Type definition for document comparison methods.
typedef IGR_ULONG IGR_COMPARE_DOCUMENTS_COMPARE_TYPE;

/// @brief Compare documents by words.
#define IGR_COMPARE_DOCUMENTS_COMPARE_WORDS 0x0

/** @} */ // document_compare_methods

// ****************************************************************************
// * IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE type and constants

/**
 * @defgroup document_difference_sources Document Difference Sources
 * @brief Enumeration of document sources for detected differences.
 *
 * These constants indicate which document(s) contain the detected difference.
 * @{
 */

/// @brief Type definition for difference source types.
typedef IGR_ULONG IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_TYPE;

/// @brief Difference found in original document.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_ORIGINAL 0x0

/// @brief Difference found in original document - Legacy spelling.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_ORGINAL  0x0

/// @brief Difference found in revised document.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_REVISED  0x1

/// @brief Difference found in both documents.
#define IGR_COMPARE_DOCUMENTS_DIFFERENCE_SOURCE_BOTH     0x2

/** @} */ // document_difference_sources

// ****************************************************************************
// * IGR_DEVICE_TYPE type and constants

/**
 * @defgroup canvas_device_types Canvas Output Device Types
 * @brief Enumeration of output formats for canvas rendering.
 *
 * These constants specify the format for canvas output (image, document,
 * markup).
 * @{
 */

/// @brief Canvas output device type definition.
typedef IGR_LONG IGR_DEVICE_TYPE;

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

/** @} */ // canvas_device_types

// ****************************************************************************
// * IGR_FILE_SUPPORTS type and constants

/**
 * @defgroup file_support_flags File Support Capability Flags
 * @brief Bit flags indicating supported capabilities for file types.
 *
 * These flags indicate which operations and features are supported for a
 * document format.
 * @{
 */

/// @brief Type definition for indicating file support type compatibility.
typedef IGR_LONG IGR_FILE_SUPPORTS_TYPE;

/// @brief Document supports text extraction capability.
#define IGR_FILE_SUPPORTS_TEXT        1

/// @brief Document supports sub-file enumeration capability.
#define IGR_FILE_SUPPORTS_SUBFILES    2

/// @brief Document supports HD HTML conversion capability.
#define IGR_FILE_SUPPORTS_HDHTML      4

/// @brief Document is corrupt or truncated.
#define IGR_FILE_CORRUPT_OR_TRUNCATED 0x2000

/** @} */ // file_support_flags

// ****************************************************************************
// * IGR_FILETYPE type and constants

/**
 * @defgroup file_types File Type Classifications
 * @brief Enumeration of document file types and categories.
 *
 * These constants classify documents into categories based on their format
 * and purpose.
 * @{
 */

/// @brief Type definition for document file types.
typedef IGR_LONG IGR_FILETYPE_TYPE;

/// @brief Unknown file type.
#define IGR_FILETYPE_UNKNOWN          0

/// @brief Plain text document.
#define IGR_FILETYPE_TEXT             100

/// @brief Text document with markup (e.g. HTML, XML).
#define IGR_FILETYPE_TEXT_MARKUP      101

/// @brief Text representation of data (e.g. JSON, CSV)
#define IGR_FILETYPE_TEXT_DATA        102

/// @brief Word processor document.
#define IGR_FILETYPE_WORD_PROCESSOR   200

/// @brief Spreadsheet document.
#define IGR_FILETYPE_SPREADSHEET      300

/// @brief Presentation document.
#define IGR_FILETYPE_PRESENTATION     400

/// @brief Other office document type.
#define IGR_FILETYPE_OFFICE_OTHER     500

/// @brief Computer aided design (CAD) document.
#define IGR_FILETYPE_CAD              600

/// @brief Archive file (compressed container).
#define IGR_FILETYPE_ARCHIVE          700

/// @brief System/configuration file.
#define IGR_FILETYPE_SYSTEM           800

/// @brief Database file.
#define IGR_FILETYPE_DATABASE         900

/// @brief Generic graphic file.
#define IGR_FILETYPE_GRAPHIC          1000

/// @brief Raster graphic file (bitmap).
#define IGR_FILETYPE_GRAPHIC_RASTER   1001

/// @brief Vector graphic file.
#define IGR_FILETYPE_GRAPHIC_VECTOR   1002

/// @brief Generic email file.
#define IGR_FILETYPE_EMAIL            1100

/// @brief Individual email message.
#define IGR_FILETYPE_EMAIL_MESSAGE    1101

/// @brief Email container/folder.
#define IGR_FILETYPE_EMAIL_CONTAINER  1102

/// @brief Email contact.
#define IGR_FILETYPE_EMAIL_CONTACT    1103

/// @brief Email calendar item.
#define IGR_FILETYPE_EMAIL_CALENDAR   1104

/// @brief Other email-related file.
#define IGR_FILETYPE_EMAIL_OTHER      1105

/// @brief Generic multimedia file.
#define IGR_FILETYPE_MULTIMEDIA       1200

/// @brief Video file.
#define IGR_FILETYPE_MULTIMEDIA_VIDEO 1201

/// @brief Audio file.
#define IGR_FILETYPE_MULTIMEDIA_AUDIO 1202

/// @brief Desktop publishing file.
#define IGR_FILETYPE_PUBLISHING       1300

/// @brief Application data file.
#define IGR_FILETYPE_APPDATA          2000

/** @} */ // file_types

// ****************************************************************************
// * IGR_GET_FORMAT_ATTRIBUTE type and constants

/**
 * @defgroup get_format_attribute_types Format Attribute Query Types
 * @brief Attribute selectors for IGR_Get_Format_Attribute().
 *
 * These constants specify which piece of information to retrieve about a
 * supported document format when calling IGR_Get_Format_Attribute().
 * @{
 */

/// @brief Type definition for format attribute query selectors.
typedef IGR_LONG IGR_GET_FORMAT_ATTRIBUTE_TYPE;

/// @brief Retrieve the long (full) format name.
#define IGR_GET_FORMAT_ATTRIBUTE_LONG_NAME     0

/// @brief Retrieve the short format name.
#define IGR_GET_FORMAT_ATTRIBUTE_SHORT_NAME    1

/// @brief Retrieve the search index config format name.
#define IGR_GET_FORMAT_ATTRIBUTE_CONFIG_NAME   2

/// @brief Retrieve the format class code.
#define IGR_GET_FORMAT_ATTRIBUTE_CLASS_CODE    3

/// @brief Retrieve whether the format is considered legacy.
#define IGR_GET_FORMAT_ATTRIBUTE_IS_LEGACY     4

/// @brief Retrieve the format's MIME type.
#define IGR_GET_FORMAT_ATTRIBUTE_MIME_TYPE     5

/// @brief Retrieve the format's file category. See @ref file_types.
#define IGR_GET_FORMAT_ATTRIBUTE_FILE_CATEGORY 6

/** @} */ // get_format_attribute_types

// ****************************************************************************
// * IGR_GET_OPTION_ATTRIBUTE type and constants

/**
 * @defgroup get_option_attribute_types Option Attribute Query Types
 * @brief Attribute selectors for IGR_Get_Option_Attribute().
 *
 * These constants specify which piece of information to retrieve about a
 * configuration option when calling IGR_Get_Option_Attribute().
 * @{
 */

/// @brief Type definition for option attribute query selectors.
typedef IGR_LONG IGR_GET_OPTION_ATTRIBUTE_TYPE;

/// @brief Retrieve the option's display name.
#define IGR_GET_OPTION_ATTRIBUTE_DISPLAY_NAME     0

/// @brief Retrieve the option's description.
#define IGR_GET_OPTION_ATTRIBUTE_DESCRIPTION      1

/// @brief Retrieve the option's default value.
#define IGR_GET_OPTION_ATTRIBUTE_DEFAULT_VALUE    2

/// @brief Retrieve the option's flags.
#define IGR_GET_OPTION_ATTRIBUTE_FLAGS            3

/// @brief Retrieve the option's value type (e.g., string, integer, boolean).
#define IGR_GET_OPTION_ATTRIBUTE_VALUE_TYPE       4

/// @brief Retrieve the option's available (possible) values.
#define IGR_GET_OPTION_ATTRIBUTE_AVAILABLE_VALUES 5

/** @} */ // get_option_attribute_types

// ****************************************************************************
// * IGR_HYPERLINK_ACTION type and constants

/**
 * @defgroup hyperlink_action_types Hyperlink Action Types
 * @brief Enumeration of hyperlink target action types.
 * @{
 */

/// @brief Type definition for hyperlink action types.
typedef IGR_LONG IGR_HYPERLINK_ACTION_TYPE;

/// @brief Action type for navigation within document.
#define IGR_HYPERLINK_ACTION_GOTO 0x1

/// @brief Action type for external/URI links. Destination is either a
///        qualified or relative link.
#define IGR_HYPERLINK_ACTION_URI  0x2

/** @} */ // hyperlink_action_types

// ****************************************************************************
// * IGR_HYPERLINK_FLAGS type and constants

/**
 * @defgroup hyperlink_flags Hyperlink Navigation Flags
 * @brief Bit flags controlling hyperlink navigation behavior.
 *
 * These flags specify which coordinates and zoom level should be adjusted
 * when following a hyperlink.
 * @{
 */

/// @brief Type definition for hyperlink flags.
typedef IGR_LONG IGR_HYPERLINK_FLAGS_TYPE;

/// @brief Change left coordinate when navigating.
#define IGR_HYPERLINK_FLAGS_CHANGE_LEFT 0x1

/// @brief Change top coordinate when navigating.
#define IGR_HYPERLINK_FLAGS_CHANGE_TOP  0x2

/// @brief Change zoom level when navigating.
#define IGR_HYPERLINK_FLAGS_CHANGE_ZOOM 0x4

/** @} */ // hyperlink_flags

// ****************************************************************************
// * IGR_HYPERLINK_FIT type and constants

/**
 * @defgroup hyperlink_fit_types Hyperlink View Fit Types
 * @brief Enumeration of hyperlink fit/zoom modes.
 *
 * These constants define how to position and zoom content when following a
 * hyperlink.
 * @{
 */

/// @brief Type definition for hyperlink fit modes.
typedef IGR_LONG IGR_HYPERLINK_FIT_TYPE;

/// @brief Fit at specific coordinates with zoom.
#define IGR_HYPERLINK_FIT_XYZ   0

/// @brief Fit page to window.
#define IGR_HYPERLINK_FIT_FIT   1

/// @brief Fit page horizontally.
#define IGR_HYPERLINK_FIT_FITH  2

/// @brief Fit page vertically.
#define IGR_HYPERLINK_FIT_FITV  3

/// @brief Fit page to rectangle.
#define IGR_HYPERLINK_FIT_FITR  4

/// @brief Fit page bounds.
#define IGR_HYPERLINK_FIT_FITB  5

/// @brief Fit page bounds horizontally.
#define IGR_HYPERLINK_FIT_FITBH 6

/// @brief Fit page bounds vertically.
#define IGR_HYPERLINK_FIT_FITBV 7

/** @} */ // hyperlink_fit_types

// ****************************************************************************
// * IGR_HYPERLINK_GET_VALUE type and constants

/**
 * @defgroup hyperlink_value_types Hyperlink Value Types
 * @brief Enumeration of hyperlink value/destination types.
 * @{
 */

/// @brief Type definition for hyperlink value types.
typedef IGR_LONG IGR_HYPERLINK_GET_VALUE_TYPE;

/// @brief Get reference value.
#define IGR_HYPERLINK_GET_VALUE_REF 0

/// @brief Get URI value.
#define IGR_HYPERLINK_GET_VALUE_URI 1

/** @} */ // hyperlink_value_types

// ****************************************************************************
// * IGR_GET_PAGE_PIXELS_FLAGS_TYPE type and constants

/**
 * @defgroup page_pixels_flags Page Pixel Buffer Flags
 * @brief Bit flags for page pixel retrieval operations.
 * @{
 */

/// @brief Type definition for page pixel flags.
typedef IGR_ULONG IGR_GET_PAGE_PIXELS_FLAGS_TYPE;

/// @brief Buffer has been allocated.
#define IGR_GET_PAGE_PIXELS_FLAGS_BUFFER_ALLOCATED 0x00010000

/** @} */ // page_pixels_flags

// ****************************************************************************
// * IGR_LOG_LEVEL type and constants

/**
 * @defgroup log_levels Logging Level Constants
 * @brief Enumeration of logging severity levels.
 *
 * These constants define the severity levels for diagnostic logging output.
 * @{
 */

/// @brief Type definition for log level values.
typedef IGR_ULONG IGR_LOG_LEVEL_TYPE;

/// @brief No logging.
#define IGR_LOG_LEVEL_NONE        0

/// @brief Critical error logging level.
#define IGR_LOG_LEVEL_CRITICAL    1

/// @brief Error logging level.
#define IGR_LOG_LEVEL_ERROR       2

/// @brief Warning logging level.
#define IGR_LOG_LEVEL_WARNING     3

/// @brief Information logging level.
#define IGR_LOG_LEVEL_INFORMATION 4

/// @brief Debug logging level.
#define IGR_LOG_LEVEL_DEBUG       5

/// @brief Trace logging level.
#define IGR_LOG_LEVEL_TRACE       6

/** @} */ // log_levels

// ****************************************************************************
// * IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS type and constants

/**
 * @defgroup stream_function_flags Stream Creation Function Flags
 * @brief Bit flags for custom stream function handling.
 * @{
 */

/// @brief Type definition for defining flags used in
///        IGR_Make_Stream_From_Functions.
typedef IGR_LONG IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_TYPE;

/// @brief No flags set for stream functions.
#define IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_NONE     0x0

/// @brief Use cdecl calling convention for stream functions.
#define IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_USECDECL 0x1

/** @} */ // stream_function_flags

// ****************************************************************************
// * IGR_OPEN_BITMAP_FLAGS type and constants

/**
 * @defgroup bitmap_open_flags Bitmap Loading Flags
 * @brief Bit flags controlling bitmap data interpretation.
 * @{
 */

/// @brief Flags type definition for bitmap processing options.
typedef IGR_LONG IGR_OPEN_BITMAP_FLAGS_TYPE;

/// @brief Flag indicating minimum value represents black.
#define IGR_OPEN_BITMAP_FLAGS_MIN_IS_BLACK 0x01

/// @brief Flag indicating bitmap data is stored bottom-up.
#define IGR_OPEN_BITMAP_FLAGS_BOTTOM_UP    0x02

/** @} */ // bitmap_open_flags

// ****************************************************************************
// * IGR_OPEN_BITMAP_ORIENTATION type and constants

/**
 * @defgroup bitmap_orientations Bitmap Image Orientations
 * @brief Enumeration of bitmap orientation specifications.
 *
 * These constants define the orientation (rotation/reflection) of bitmap
 * images.
 * @{
 */

/// @brief Type definition for bitmap orientation specifications.
typedef IGR_LONG IGR_OPEN_BITMAP_ORIENTATION_TYPE;

/// @brief Image oriented top-left.
#define IGR_OPEN_BITMAP_ORIENTATION_TOPLEFT  1

/// @brief Image oriented top-right.
#define IGR_OPEN_BITMAP_ORIENTATION_TOPRIGHT 2

/// @brief Image oriented bottom-right.
#define IGR_OPEN_BITMAP_ORIENTATION_BOTRIGHT 3

/// @brief Image oriented bottom-left.
#define IGR_OPEN_BITMAP_ORIENTATION_BOTLEFT  4

/// @brief Image oriented left-top.
#define IGR_OPEN_BITMAP_ORIENTATION_LEFTTOP  5

/// @brief Image oriented right-top.
#define IGR_OPEN_BITMAP_ORIENTATION_RIGHTTOP 6

/// @brief Image oriented right-bottom.
#define IGR_OPEN_BITMAP_ORIENTATION_RIGHTBOT 7

/// @brief Image oriented left-bottom.
#define IGR_OPEN_BITMAP_ORIENTATION_LEFTBOT  8

/** @} */ // bitmap_orientations

// ****************************************************************************
// * IGR_OPEN_BITMAP_PIXEL type and constants

/**
 * @defgroup bitmap_pixel_formats Bitmap Pixel Format Specifications
 * @brief Enumeration of bitmap pixel format configurations.
 *
 * These constants specify the color depth and encoding for bitmap pixel data.
 * @{
 */

/// @brief Type definition for bitmap pixel format specifications.
typedef IGR_LONG IGR_OPEN_BITMAP_PIXEL_TYPE;

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

/** @} */ // bitmap_pixel_formats

// ****************************************************************************
// * IGR_OPEN_CALLBACK_ACTION type and constants

/**
 * @defgroup callback_action_types Callback Action Type Constants
 * @brief Enumeration of callback action types for document processing events.
 *
 * These constants identify the type of event/action triggering a callback
 * invocation.
 * @{
 */

/// @brief Type definition for callback actions used in document processing.
typedef IGR_LONG IGR_OPEN_CALLBACK_ACTION;

/// @brief Callback action for heartbeat events.
#define IGR_OPEN_CALLBACK_ACTION_HEARTBEAT                 0

/// @brief Callback action for password requests.
#define IGR_OPEN_CALLBACK_ACTION_PASSWORD                  1

/// @brief Callback action for text localization.
#define IGR_OPEN_CALLBACK_ACTION_LOCALIZE                  2

/// @brief Callback action for getting log level.
#define IGR_OPEN_CALLBACK_ACTION_LOG_LEVEL                 3

/// @brief Callback action for logging messages.
#define IGR_OPEN_CALLBACK_ACTION_LOG_MESSAGE               4

/// @brief Callback action for approving external resources.
#define IGR_OPEN_CALLBACK_ACTION_APPROVE_EXTERNAL_RESOURCE 5

/// @brief Callback action for getting resource streams.
#define IGR_OPEN_CALLBACK_ACTION_GET_RESOURCE_STREAM       6

/// @brief Callback action for OCR image processing.
#define IGR_OPEN_CALLBACK_ACTION_OCR_IMAGE                 7

/// @brief Callback action for image description processing.
#define IGR_OPEN_CALLBACK_ACTION_DESCRIBE_IMAGE            8

/** @} */ // callback_action_types

// ****************************************************************************
// * IGR_OPEN_FLAGS type and constants

/**
 * @defgroup open_flags Document Opening Flags
 * @brief Bit flags controlling document processing and output options.
 *
 * These flags control what content is extracted, output format, and
 * processing options.
 * @{
 */

/// @brief Type definition for open flags.
typedef IGR_LONG IGR_OPEN_FLAGS_TYPE;

/// @brief Extract only the body content.
#define IGR_BODY_ONLY               0

/// @brief Extract only the metadata.
#define IGR_META_ONLY               1

/// @brief Extract both body content and metadata.
#define IGR_BODY_AND_META           2

/// @brief Output format: plain text.
#define IGR_FORMAT_TEXT             0x00010000L

/// @brief Output format: HTML.
#define IGR_FORMAT_HTML             0x00020000L

/// @brief Output format: XML.
#define IGR_FORMAT_XML              0x00030000L

/// @brief Output format: HD HTML if possible, fallback to regular HTML if
///        not.
#define IGR_FORMAT_HDHTML_IFYOUCAN  0x00040000L

/// @brief Output format: image.
#define IGR_FORMAT_IMAGE            0x00050000L

/// @brief Output format: HTML with no fallback.
#define IGR_FORMAT_HTML_NO_FALLBACK 0x00060000L

/// @brief Output format: JSON.
#define IGR_FORMAT_JSON             0x00070000L

/// @brief Output format: Markdown.
#define IGR_FORMAT_MARKDOWN         0x00080000L

/** @} */ // open_flags

// ****************************************************************************
// * IGR_OPEN_FROM type and constants

/**
 * @defgroup open_from_types Document Source Type Constants
 * @brief Enumeration of document source types for IGR_Open_Ex().
 *
 * These constants specify where the document data originates when opening a
 * document with IGR_Open_Ex(). The source type determines how the file_path
 * parameter is interpreted and which callback functions are used.
 * @{
 */

/// @brief Type definition for document source type used with IGR_Open_Ex.
typedef IGR_LONG IGR_OPEN_FROM;

/// @brief Open from a UTF-16 filename (traditional file-based access).
#define IGR_OPEN_FROM_FILENAME_UTF16 0

/// @brief Open from a stream object with custom I/O callbacks.
#define IGR_OPEN_FROM_STREAM         1

/** @} */ // open_from_types

// ****************************************************************************
// * IGR_PAGE_ELEMENT type and constants

/**
 * @defgroup page_element_types Page Element Types
 * @brief Enumeration of structural element types found on document pages.
 *
 * These constants identify the structural type of elements within document
 * content (paragraphs, tables, images, form fields, etc.).
 * @{
 */

/// @brief Type definition for page element types.
typedef IGR_ULONG IGR_PAGE_ELEMENT_TYPE;

/// @brief Page element type for a full page.
#define IGR_PAGE_ELEMENT_TYPE_PAGE         1

/// @brief Page element type for a section.
#define IGR_PAGE_ELEMENT_TYPE_SECTION      2

/// @brief Page element type for a column.
#define IGR_PAGE_ELEMENT_TYPE_COLUMN       3

/// @brief Page element type for a paragraph.
#define IGR_PAGE_ELEMENT_TYPE_PARAGRAPH    4

/// @brief Page element type for a line of text.
#define IGR_PAGE_ELEMENT_TYPE_LINE         5

/// @brief Page element type for a text run.
#define IGR_PAGE_ELEMENT_TYPE_RUN          6

/// @brief Page element type for a word.
#define IGR_PAGE_ELEMENT_TYPE_WORD         7

/// @brief Page element type for a header.
#define IGR_PAGE_ELEMENT_TYPE_HEADER       8

/// @brief Page element type for a footer.
#define IGR_PAGE_ELEMENT_TYPE_FOOTER       9

/// @brief Page element type for an image.
#define IGR_PAGE_ELEMENT_TYPE_IMAGE        10

/// @brief Page element type for a table.
#define IGR_PAGE_ELEMENT_TYPE_TABLE        11

/// @brief Page element type for a table row.
#define IGR_PAGE_ELEMENT_TYPE_TABLE_ROW    12

/// @brief Page element type for a table cell.
#define IGR_PAGE_ELEMENT_TYPE_TABLE_CELL   13

/// @brief Page element type for a form.
#define IGR_PAGE_ELEMENT_TYPE_FORM         14

/// @brief Page element type for a form element.
#define IGR_PAGE_ELEMENT_TYPE_FORM_ELEMENT 15

/// @brief Page element type for floating content.
#define IGR_PAGE_ELEMENT_TYPE_FLOAT        16

/// @brief Page element type for graphics.
#define IGR_PAGE_ELEMENT_TYPE_GRAPHIC      17

/// @brief Page element type for text boxes.
#define IGR_PAGE_ELEMENT_TYPE_TEXT_BOX     18

/// @brief Page element type for notes.
#define IGR_PAGE_ELEMENT_TYPE_NOTES        19

/** @} */ // page_element_types

/**
 * @defgroup page_element_roles Page Element Types
 * @brief Enumeration of structural element types found on document pages.
 *
 * These constants identify the structural type of elements within document
 * content (paragraphs, tables, images, form fields, etc.).
 * @{
 */

/// @brief Type definition for page element roles.
typedef IGR_ULONG IGR_PAGE_ELEMENT_ROLE_TYPE;

/// @brief Page element role indicating an unknown or unspecified role.
#define IGR_PAGE_ELEMENT_ROLE_UNKNOWN    0

/// @brief Page element role indicating the document title.
#define IGR_PAGE_ELEMENT_ROLE_TITLE      1

/// @brief Page element role indicating a heading.
#define IGR_PAGE_ELEMENT_ROLE_HEADING_1  2

/// @brief Page element role indicating a heading.
#define IGR_PAGE_ELEMENT_ROLE_HEADING_2  3

/// @brief Page element role indicating a heading.
#define IGR_PAGE_ELEMENT_ROLE_HEADING_3  4

/// @brief Page element role indicating a heading.
#define IGR_PAGE_ELEMENT_ROLE_HEADING_4  5

/// @brief Page element role indicating a heading.
#define IGR_PAGE_ELEMENT_ROLE_HEADING_5  6

/// @brief Page element role indicating a heading.
#define IGR_PAGE_ELEMENT_ROLE_HEADING_6  7

/// @brief Page element role indicating a caption.
#define IGR_PAGE_ELEMENT_ROLE_CAPTION    8

/// @brief Page element role indicating a footnote.
#define IGR_PAGE_ELEMENT_ROLE_FOOTNOTE   9

/// @brief Page element role indicating a list item.
#define IGR_PAGE_ELEMENT_ROLE_LIST_ITEM  10

/// @brief Page element role indicating a table of contents item.
#define IGR_PAGE_ELEMENT_ROLE_TOC_ITEM   11

/// @brief Page element role indicating a code block.
#define IGR_PAGE_ELEMENT_ROLE_CODE       12

/// @brief Page element role indicating a formula.
#define IGR_PAGE_ELEMENT_ROLE_FORMULA    13

/// @brief Page element role indicating a quote.
#define IGR_PAGE_ELEMENT_ROLE_QUOTE      14

/// @brief Page element role indicating a reference.
#define IGR_PAGE_ELEMENT_ROLE_REFERENCE  15

/// @brief Page element role indicating a chart.
#define IGR_PAGE_ELEMENT_ROLE_CHART      16

/// @brief Page element role indicating a decoration.
#define IGR_PAGE_ELEMENT_ROLE_DECORATION 17

/// @brief Page element role indicating a seal.
#define IGR_PAGE_ELEMENT_ROLE_SEAL       18

/// @brief Page element role indicating a checked state.
#define IGR_PAGE_ELEMENT_ROLE_CHECKED    19

/// @brief Page element role indicating an unchecked state.
#define IGR_PAGE_ELEMENT_ROLE_UNCHECKED  20

/** @} */ // page_element_roles

// ****************************************************************************
// * IGR_PAGE_ELEMENT_FLAG type and constants

/**
 * @defgroup page_element_flags Page Element Property Flags
 * @brief Bit flags indicating properties and characteristics of page
 *        elements.
 *
 * These flags provide information about special properties and states of page
 * elements.
 * @{
 */

/// @brief Type definition for page element flags.
typedef IGR_ULONG IGR_PAGE_ELEMENT_FLAG_TYPE;

/// @brief Flag indicating element contains dynamic content.
#define IGR_PAGE_ELEMENT_FLAG_DYNAMIC_CONTENT  0x01

/// @brief Flag indicating element contains injected content.
#define IGR_PAGE_ELEMENT_FLAG_INJECTED_CONTENT 0x02

/** @} */ // page_element_flags

// ****************************************************************************
// * IGR_PAGE_FORM_ELEMENT type and constants

/**
 * @defgroup form_element_types HTML Form Element Types
 * @brief Enumeration of HTML form field element types.
 *
 * These constants identify the type of interactive form elements found in
 * documents.
 * @{
 */

/// @brief Type definition for form element types.
typedef IGR_LONG IGR_PAGE_FORM_ELEMENT_TYPE;

/// @brief Button form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_BUTTON          0

/// @brief Radio button form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_RADIOBUTTON     1

/// @brief Checkbox form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_CHECKBOX        2

/// @brief File select form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_FILESELECT      3

/// @brief Multiline text form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_MULTILINE_TEXT  4

/// @brief Single line text form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_SINGLELINE_TEXT 5

/// @brief Combobox form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_COMBOBOX        6

/// @brief Listbox form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_LISTBOX         7

/// @brief Signature form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_SIGNATURE       8

/// @brief Password form element.
#define IGR_PAGE_FORM_ELEMENT_TYPE_PASSWORD        9

/** @} */ // form_element_types

// ****************************************************************************
// * IGR_PAGE_FORM_ELEMENT_FLAG type and constants

/**
 * @defgroup form_element_flags Form Element Property Flags
 * @brief Bit flags describing form element state and properties.
 *
 * These flags indicate attributes and states of form input elements.
 * @{
 */

/// @brief Type definition for form element flags.
typedef IGR_LONG IGR_PAGE_FORM_ELEMENT_FLAG_TYPE;

/// @brief Form element is read-only.
#define IGR_PAGE_FORM_ELEMENT_FLAG_READONLY (1 << 0)

/// @brief Form element is required.
#define IGR_PAGE_FORM_ELEMENT_FLAG_REQUIRED (1 << 1)

/// @brief Form element has comb formatting.
#define IGR_PAGE_FORM_ELEMENT_FLAG_COMB     (1 << 2)

/// @brief Form element is checked.
#define IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED  (1 << 3)

/** @} */ // form_element_flags

// ****************************************************************************
// * IGR_PAGE_FORM_ELEMENT_GET type and constants

/**
 * @defgroup form_element_property_accessors Form Element Property Access
 *                                           Types
 * @brief Enumeration of form element properties that can be queried.
 * @{
 */

/// @brief Type definition for form element property accessors.
typedef IGR_LONG IGR_PAGE_FORM_ELEMENT_GET_TYPE;

/// @brief Get form element name.
#define IGR_PAGE_FORM_ELEMENT_GET_NAME        0

/// @brief Get form element value.
#define IGR_PAGE_FORM_ELEMENT_GET_VALUE       1

/// @brief Get form element caption.
#define IGR_PAGE_FORM_ELEMENT_GET_CAPTION     2

/// @brief Get form element action.
#define IGR_PAGE_FORM_ELEMENT_GET_ACTION      3

/// @brief Get form element action destination.
#define IGR_PAGE_FORM_ELEMENT_GET_ACTION_DEST 4

/// @brief Get form element selected state.
#define IGR_PAGE_FORM_ELEMENT_GET_SELECTED    5

/// @brief Get form element font name.
#define IGR_PAGE_FORM_ELEMENT_GET_FONT_NAME   6

/** @} */ // form_element_property_accessors

// ****************************************************************************
// * OCR_BLOCK_TYPE type and constants

/**
 * @defgroup ocr_block_types OCR Structural Block Types
 * @brief Enumeration of structural block element types for OCR processing.
 *
 * These constants identify different types of logical blocks that can be
 * created during OCR text extraction and layout analysis.
 * @{
 */

/// @brief Type definition for OCR block types.
typedef IGR_ULONG OCR_BLOCK_TYPE;

/// @brief Block representing a general area or container.
#define OCR_BLOCK_TYPE_AREA     0

/// @brief Block representing a paragraph.
#define OCR_BLOCK_TYPE_PARA     1

/// @brief Block representing a line of text.
#define OCR_BLOCK_TYPE_LINE     2

/// @brief Block representing a word or word cluster.
#define OCR_BLOCK_TYPE_WORD     4

/// @brief Block for image reorientation/rotation information.
#define OCR_BLOCK_TYPE_REORIENT 5

/// @brief Block representing a section or chapter.
#define OCR_BLOCK_TYPE_SECTION  6

/// @brief Block representing a column within a page layout.
#define OCR_BLOCK_TYPE_COLUMN   7

/** @} */ // ocr_block_types

// ****************************************************************************
// * IGR_PEN type and constants

/**
 * @defgroup pen_types Canvas Pen Types
 * @brief Pen style constants for canvas drawing operations.
 *
 * These constants define pen styles used when drawing lines or outlines on
 * canvas output during document rendering.
 * @{
 */

/// @brief Canvas pen type definition.
typedef IGR_LONG IGR_PEN_TYPE;

/// @brief No pen style.
#define IGR_PEN_NONE  0x00

/// @brief Solid pen style.
#define IGR_PEN_SOLID 0x01

/** @} */ // pen_types

// ****************************************************************************
// * IGR_REDACT_FLAGS type and constants

/**
 * @defgroup redaction_flags Redaction Control Flags
 * @brief Flags controlling document redaction behavior.
 *
 * These flags control how sensitive content is redacted from documents,
 * including options for text vs. image redaction, rasterization strategy, and
 * visual presentation of redaction regions.
 * @{
 */

/// @brief Type definition for redaction flags.
typedef IGR_ULONG IGR_REDACT_FLAGS_TYPE;

/// @brief Do not redact text during redaction.
#define IGR_REDACT_FLAGS_DO_NOT_REDACT_TEXT   0x1

/// @brief Do not redact images during redaction.
#define IGR_REDACT_FLAGS_DO_NOT_REDACT_IMAGES 0x2

/// @brief Page should always be rasterized and then redacted pixels replaced
///        with the specified color. If this flag is set and redaction cannot
///        be performed an error will occur.
#define IGR_REDACT_FLAGS_ALWAYS_RASTER        0x4

/// @brief Page should never be rasterized. If this flag is set and redaction
///        cannot be performed an error will occur.
#define IGR_REDACT_FLAGS_NEVER_RASTER         0x8

/// @brief Do not draw redaction regions on the canvas, redacted images will
///        still use the redaction regions and colors
#define IGR_REDACT_FLAGS_DO_NOT_DRAW_REGION   0x10

/** @} */ // redaction_flags

// ****************************************************************************
// * IGR_SUBFILE_INFO_FLAG type and constants

/**
 * @defgroup subfile_info_flags Subfile Information Flags
 * @brief Flags describing properties of embedded subfiles.
 *
 * These flags indicate metadata about embedded subfiles (e.g., within
 * container formats), such as password protection and comment status.
 * @{
 */

/// @brief Type definition for subfile info flags.
typedef IGR_ULONG IGR_SUBFILE_INFO_FLAG_TYPE;

/// @brief Subfile is password protected.
#define IGR_SUBFILE_INFO_FLAG_PASSWORD_PROTECTED 0x0002

/// @brief Subfile has comment data.
#define IGR_SUBFILE_INFO_FLAG_HAS_COMMENT        0x0004

/** @} */ // subfile_info_flags

// ****************************************************************************
// * IGR_TEXT_ALIGN type and constants

/**
 * @defgroup text_alignment_types Text Alignment Types
 * @brief Text alignment constants for canvas text rendering.
 *
 * These constants specify horizontal and vertical text alignment options when
 * rendering text on canvas output. Horizontal alignment (LEFT, CENTER, RIGHT)
 * can be combined with vertical alignment (TOP, MIDDLE, BOTTOM) using bitwise
 * OR.
 * @{
 */

/// @brief Canvas text alignment type definition.
typedef IGR_LONG IGR_TEXT_ALIGN_TYPE;

/// @brief Left alignment.
#define IGR_TEXT_ALIGN_LEFT   0x00

/// @brief Right alignment.
#define IGR_TEXT_ALIGN_RIGHT  0x01

/// @brief Center alignment.
#define IGR_TEXT_ALIGN_CENTER 0x02

/// @brief Top alignment.
#define IGR_TEXT_ALIGN_TOP    0x10

/// @brief Middle alignment.
#define IGR_TEXT_ALIGN_MIDDLE 0x11

/// @brief Bottom alignment.
#define IGR_TEXT_ALIGN_BOTTOM 0x12

/** @} */ // text_alignment_types

// ****************************************************************************
// * IGR_TEXT_STYLE type and constants

/**
 * @defgroup text_style_types Text Style Types
 * @brief Text formatting style constants for canvas text rendering.
 *
 * These flags specify text formatting styles (bold, italic, underline) used
 * with IGR_Canvas_SetFont() and related canvas text operations. Multiple
 * styles can be combined using bitwise OR.
 * @{
 */

/// @brief Type definition for text style options used with
///        IGR_Canvas_SetFont.
typedef IGR_LONG IGR_TEXT_STYLE_TYPE;

/// @brief Normal text style with no formatting.
#define IGR_TEXT_STYLE_NORMAL    0x00

/// @brief Bold text formatting.
#define IGR_TEXT_STYLE_BOLD      0x01

/// @brief Italic text formatting.
#define IGR_TEXT_STYLE_ITALIC    0x02

/// @brief Underlined text formatting.
#define IGR_TEXT_STYLE_UNDERLINE 0x04

/** @} */ // text_style_types

// ****************************************************************************
// * IGR_TEXTRECT type and constants

/**
 * @defgroup text_rectangle_flags Text Rectangle Rendering Flags
 * @brief Flags controlling text rendering within rectangular bounds.
 *
 * These flags control how text is rendered within a defined rectangle,
 * including clipping and scaling behavior.
 * @{
 */

/// @brief Type definition for text rectangle rendering flags.
typedef IGR_LONG IGR_TEXTRECT_TYPE;

/// @brief Flag to clip text within the rectangle bounds.
#define IGR_TEXTRECT_FLAG_CLIP         0x01

/// @brief Flag to stretch text to fit within the rectangle bounds.
#define IGR_TEXTRECT_FLAG_STRETCHTOFIT 0x02

/** @} */ // text_rectangle_flags

// ****************************************************************************
// * IGR_DESCRIBE_IMAGE_ADDTEXT_FLAGS type and constants

/**
 * @defgroup describe_image_addtext_flags Describe Image AddText Flags
 * @brief Flags controlling how text is added during image description.
 *
 * These flags control whether AddText operations append to or replace
 * existing image descriptions/alternative text.
 * @{
 */

/// @brief Type definition for describe image AddText flags.
typedef IGR_ULONG IGR_DESCRIBE_IMAGE_ADDTEXT_FLAGS_TYPE;

/// @brief Append the provided text to existing alt text.
#define IGR_DESCRIBE_IMAGE_ADDTEXT_FLAGS_APPEND  0x00

/// @brief Replace existing alt text with the provided text.
#define IGR_DESCRIBE_IMAGE_ADDTEXT_FLAGS_REPLACE 0x01

/** @} */ // describe_image_addtext_flags

// ****************************************************************************
// * IGR_DESCRIBE_IMAGE image type

/**
 * @defgroup describe_image_types Describe Image Type Classifications
 * @brief Image type constants for image description and analysis.
 *
 * These constants classify images by type for the image description feature,
 * enabling type-specific processing (e.g., OCR for pictures, metadata
 * extraction for charts).
 * @{
 */

/// @brief Type definition of images for describe image feature.
typedef IGR_LONG IGR_DESCRIBE_IMAGE_TYPE;

/// @brief Unknown image type.
#define IGR_DESCRIBE_IMAGE_TYPE_UNKNOWN 0

/// @brief Picture image type (e.g., photograph or raster image).
#define IGR_DESCRIBE_IMAGE_TYPE_PICTURE 1

/// @brief Drawing image type (e.g., shape, SmartArt, or vector graphic).
#define IGR_DESCRIBE_IMAGE_TYPE_DRAWING 2

/// @brief Chart image type (e.g., embedded Excel chart).
#define IGR_DESCRIBE_IMAGE_TYPE_CHART   3

/** @} */ // describe_image_types

// ****************************************************************************
// * Machine learning models

/**
 * @defgroup ml_model_types Machine Learning Model Types
 * @brief What a machine learning model does.
 *
 * A model's capability is a property of its artifact rather than something
 * chosen per call: a classifier cannot caption and a captioner cannot
 * classify. IGR_Enum_ML_Models reports it so that a host can tell what each
 * model is for without loading any of them.
 * @{
 */

/// @brief Type definition for the capability a machine learning model offers.
typedef IGR_ULONG IGR_ML_MODEL_TYPE;

/// @brief The model's capability was not stated when it was registered.
#define IGR_ML_MODEL_TYPE_UNKNOWN         0

/// @brief Puts a class to an image: a form, a receipt, a chart, a photograph.
#define IGR_ML_MODEL_TYPE_CLASSIFIER      1

/// @brief Describes an image in words, for alt text or for indexing.
#define IGR_ML_MODEL_TYPE_CAPTIONER       2

/// @brief Recovers a page's structure: its paragraphs, tables and cells.
#define IGR_ML_MODEL_TYPE_LAYOUT          3

/// @brief Reads the words on a page, with their positions.
#define IGR_ML_MODEL_TYPE_OCR             4

/// @brief Recovers the rows, columns and cells of one table.
///
/// Distinct from a layout model, which finds where the tables are: this reads one
/// once found, and the two are ordinarily used together.
#define IGR_ML_MODEL_TYPE_TABLE_STRUCTURE 5

/** @} */ // ml_model_types

/**
 * @defgroup ml_model_flags Machine Learning Model Flags
 * @brief Flags reported for each enumerated machine learning model.
 * @{
 */

/// @brief A machine learning backend is loaded, so requests may succeed.
///
/// Without this flag, nothing naming one of these models can produce a
/// result. With it, a request may still fail: the flag says a backend was
/// found, not that the runtime it needs is installed. That runtime ships
/// separately and is loaded the first time a model is actually run, so its
/// absence surfaces as a failed run rather than here.
///
/// The flag describes the installation rather than the individual model, so
/// it carries the same value for every model in one enumeration. Whether a
/// specific model loads and runs cannot be known without loading it, which
/// costs hundreds of megabytes and seconds.
#define IGR_ML_MODEL_FLAG_BACKEND_READY   0x0001

/** @} */ // ml_model_flags

/**
 * @defgroup ml_property_types Machine Learning Property Value Types
 * @brief How to interpret an IGR_ML_Property value.
 *
 * Values always arrive as text; the type says what the text spells. A
 * property whose value is neither a string nor a scalar is reported as
 * IGR_ML_PROPERTY_TYPE_STRING holding its JSON.
 * @{
 */

/// @brief Type definition for the value type of a model property.
typedef IGR_ULONG IGR_ML_PROPERTY_TYPE;

/// @brief The value is text, and is used as it stands.
#define IGR_ML_PROPERTY_TYPE_STRING  0

/// @brief The value spells a number, such as "256".
#define IGR_ML_PROPERTY_TYPE_NUMBER  1

/// @brief The value is "true" or "false".
#define IGR_ML_PROPERTY_TYPE_BOOLEAN 2

/// @brief The property is present but has no value.
#define IGR_ML_PROPERTY_TYPE_NULL    3

/** @} */ // ml_property_types

// ****************************************************************************
// * Structures
// ****************************************************************************

/// @struct tagError_Control_Block
/// @brief Structure representing an error control block containing error
///        message details.
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
///        and license details.
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
} Instance_Status_Block, *LPInstance_Status_Block;

/// @brief Type definition for pointer to \ref IGR_Stream.
typedef struct IGR_Stream* PIGR_Stream;

#ifndef _IGR_Stream
struct IGR_Stream;

/// @brief Callback for the IGR_Stream method
/// @param[in] stream Pointer to the stream to open
/// @return Status code indicating success or failure
typedef ULONG(CALLBACK* IGR_Stream_Open_Function)(struct IGR_Stream* stream);

/// @brief Callback for reading data from the stream
/// @param[in]  stream     Pointer to the stream to read from
/// @param[out] buffer     Buffer to store read data
/// @param[in]  bufferSize Size of the buffer in bytes
/// @return Number of bytes read
typedef ULONG(CALLBACK* IGR_Stream_Read_Function)(struct IGR_Stream* stream, void* buffer, ULONG bufferSize);

/// @brief Callback for seeking to a position in the stream
/// @param[in] stream Pointer to the stream
/// @param[in] offset Position to seek to
/// @param[in] whence Origin for the seek operation
/// @return New position in the stream
typedef IGR_LONGLONG(CALLBACK* IGR_Stream_Seek_Function)(struct IGR_Stream* stream, IGR_LONGLONG offset, ULONG whence);

/// @brief Callback for closing the stream and freeing resources
/// @param[in] stream Pointer to the stream to close
typedef void(CALLBACK* IGR_Stream_Close_Function)(struct IGR_Stream* stream);

/// @struct IGR_Stream
/// @brief Structure representing a stream interface with basic I/O
///        operations.
struct IGR_Stream
{
	/// @brief Callback to invoke when opening the stream.
	IGR_Stream_Open_Function Open;

	/// @brief Callback to invoke when reading from the stream.
	IGR_Stream_Read_Function Read;

	/// @brief Callback to invoke when seeking the stream.
	IGR_Stream_Seek_Function Seek;

	/// @brief Callback to invoke when closing the stream.
	IGR_Stream_Close_Function Close;
};
#endif
struct IGR_Writable_Stream;

/**
 * @defgroup callback_functions Callback Function Types
 * @brief Callback function pointer typedefs for Document Filters operations.
 *
 * These typedefs define the signatures for callback functions used throughout
 * the Document Filters API. Callbacks allow applications to customize
 * processing behavior, handle resources, and respond to events during
 * document operations.
 *
 * @{
 */

/**
 * @defgroup stream_callbacks Stream I/O Callbacks
 * @brief Callbacks for stream read/write operations.
 *
 * These callbacks are invoked when performing I/O operations on custom
 * streams, allowing applications to provide their own stream implementations.
 *
 * @ingroup callback_functions
 * @{
 */

/// @brief Callback function to write data to a writable stream.
///
/// Called to write a buffer of data to the output stream. The implementation
/// should handle the actual I/O operation and report the number of bytes
/// successfully written.
///
/// @param[in] stream     Pointer to the IGR_Writable_Stream instance to write
///                       to.
/// @param[in] buffer     Pointer to the data buffer containing the bytes to
///                       write. The buffer must remain valid for the duration
///                       of the function call.
/// @param[in] bufferSize Number of bytes to write from the buffer.
///
/// @return The number of bytes successfully written to the stream. If an
///         error occurs, return 0 or a value less than bufferSize to indicate
///         a partial or failed write. Return 0 if the write cannot be
///         completed.
///
/// @ingroup stream_callbacks
typedef ULONG(CALLBACK* IGR_Writable_Stream_Write_Function)(struct IGR_Writable_Stream* stream, void* buffer, ULONG bufferSize);

/** @} */ // stream_callbacks
/** @} */ // callback_functions

/// @struct IGR_Writable_Stream
/// @brief Structure representing a writable stream interface extending
///        IGR_Stream.
struct IGR_Writable_Stream
{
	/// @brief Base stream functionality.
	struct IGR_Stream base;

	/// @brief Callback to invoke when writing to the stream.
	IGR_Writable_Stream_Write_Function Write;
};

/// @brief Type definition for pointer to \ref IGR_Writable_Stream.
typedef struct IGR_Writable_Stream* PIGR_Writable_Stream;

/// @struct IGR_Rect
/// @brief Structure representing a rectangle with unsigned integer
///        coordinates. The coordinates are defined by the left, top, right,
///        and bottom edges.
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
///        The coordinates are defined by the left, top, right, and bottom
///        edges.
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
///        The coordinates are defined by the left, top, right, and bottom
///        edges.
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
/// @brief Structure representing a size with unsigned integer width and
///        height.
struct IGR_Size
{
	/// @brief Width dimension.
	IGR_ULONG width;

	/// @brief Height dimension.
	IGR_ULONG height;
};

/// @struct IGR_FPoint
/// @brief Structure representing a point with signed floating-point
///        coordinates.
struct IGR_FPoint
{
	/// @brief X position.
	IGR_FLOAT x;

	/// @brief Y position.
	IGR_FLOAT y;
};

/// @struct IGR_QuadPoint
/// @brief Structure representing four points defining a quadrilateral in
///        clockwise order.
struct IGR_QuadPoint
{
	union
	{
		struct
		{
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
///        page. Units are pixels.
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
	IGR_PAGE_FORM_ELEMENT_FLAG_TYPE flags;

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
/// @param[in] element Pointer to the bookmark element being processed.
/// @param[in] context User-provided context data.
///
/// @return IGR_LONG status code.
typedef IGR_LONG(IGR_EXPORT* IGR_CALLBACK_BOOKMARK)(struct IGR_Bookmark* element, void* context);

/// @brief General callback function for various actions.
///
/// @param[in] actionID   ID of the action being performed.
/// @param[in] actionData Data specific to the action.
/// @param[in] context    User-provided context data.
///
/// @return IGR_LONG status code.
typedef IGR_LONG (*IGR_CALLBACK)(int actionID, void* actionData, void* context);

/// @struct IGR_T_ACTION_GET_STREAM_PART
/// @brief Structure passed to IGR_CALLBACK's actionData when actionID is
///        IGR_ACTION_GET_STREAM_PART.
struct IGR_T_ACTION_GET_STREAM_PART
{
	/// @brief Size of this structure.
	IGR_ULONG size;

	/// @brief Input: Part name (e.g., foo.ra01).
	const wchar_t* partName;

	/// @brief Input: Full part name (e.g., c:\\foo.ra01).
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
///        rendering properties.
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
/// @brief Structure representing a hyperlink element that contains details
///        like action type, position, size, destination page number, zoom
///        factor, destination position, flags, and internal data
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

	/// @brief Left position in pixels for the destination, fit value
	///        specific.
	IGR_LONG dest_left;

	/// @brief Top position in pixels for the destination, fit value specific.
	IGR_LONG dest_top;

	/// @brief Right position in pixels for the destination, fit value
	///        specific.
	IGR_LONG dest_right;

	/// @brief Bottom position in pixels for the destination, fit value
	///        specific.
	IGR_LONG dest_bottom;

	/// @brief Flags associated with the hyperlink.
	IGR_HYPERLINK_FLAGS_TYPE flags;

	/// @brief Reserved data for internal use.
	IGR_LONG reserved2[8];
};

/// @struct IGR_Open_DIB_Info
/// @brief Structure representing information about an open Device Independent
///        Bitmap (DIB) containing details like size, flags, dimensions,
///        stride, pixel format, data and palette information.
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
/// @brief Structure representing extracted pixels from a page including
///        width, height, stride, pixel format, flags, palette information and
///        reserved data.
struct IGR_Page_Pixels
{
	/// @brief Number of pixels in one scan line of the extracted pixels.
	IGR_ULONG width;

	/// @brief Number of scanlines extracted.
	IGR_ULONG height;

	/// @brief Offset in bytes between consecutive scan lines. Positive for
	///        top-down, negative for bottom-up.
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
///        including size, flags, identifiers, names, comments, size in bytes,
///        and date/time.
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
/// @brief Structure representing an annotation including type, flags,
///        position, size and reserved data.
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
///        action argument is IGR_OPEN_CALLBACK_ACTION_HEARTBEAT to handle
///        heartbeat events.
struct IGR_Open_Callback_Action_Heartbeat
{
	/// @brief Size of this structure in bytes, must be populated by caller.
	IGR_ULONG struct_size;
};

/// @struct IGR_Open_Callback_Action_Password
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when requesting
///        password input.
struct IGR_Open_Callback_Action_Password
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief ID of subfile requiring password. Empty for main document
	///        password.
	IGR_UCS2 id[4096];

	/// @brief Password to use for decoding the document/subfile. Must be null
	///        terminated.
	IGR_UCS2 password[4096];
};

/// @struct IGR_Open_Callback_Action_Localize
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when localizing
///        strings.
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
///        levels.
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
///        messages.
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
///        external resources.
struct IGR_Open_Callback_Action_Approve_External_Resource
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Original URL of the external resource (e.g., image URL).
	IGR_UCS2 url[1024];
};

/// @struct IGR_Open_Callback_Action_Get_Resource_Stream
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when retrieving
///        resource streams.
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
/// @brief Forward declaration of the OCR image action context.
///
/// Declared here, ahead of the callbacks below that take a pointer to it,
/// because in C a struct tag first named inside a parameter list has prototype
/// scope: it would be a distinct incomplete type that ends at the closing
/// parenthesis, unrelated to the definition further down. A C caller assigning
/// its own handler to one of those typedefs then gets an incompatible-pointer
/// diagnostic naming two types that print identically. C++ has no such rule and
/// never saw the problem.
struct IGR_Open_Callback_Action_OCR_Image;

/**
 * @defgroup ocr_image_callbacks OCR Image Callbacks
 * @brief Callbacks for processing image data during OCR operations.
 *
 * These callbacks are invoked when processing images with optical character
 * recognition (OCR) to extract text, metadata, and structural information.
 *
 * @ingroup callback_functions
 * @{
 */

/// @brief Function to retrieve EXIF metadata from the source image.
///
/// Called to query EXIF metadata tags from the original image being
/// processed. Implementations should populate the destination buffer with the
/// requested metadata value if available.
///
/// @param[in]  action    Pointer to the OCR image action context containing
///                       state information.
/// @param[in]  name      The EXIF metadata tag name to retrieve
///                       (null-terminated UCS2 string).
/// @param[out] dest      Buffer to receive the retrieved metadata value
///                       (null-terminated UCS2 string).
/// @param[in]  dest_size Size of the destination buffer in characters.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_NOT_FOUND if the requested metadata tag does not exist.
///
/// @ingroup ocr_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_Open_Callback_Action_OCR_Image_GetMetadata_Function)(
    const struct IGR_Open_Callback_Action_OCR_Image* action, const IGR_UCS2* name, IGR_UCS2* dest, size_t dest_size);

/// @brief Function to save the processed image to disk with a specified
///        format.
///
/// Called to persist the OCR-processed image to a file. The caller specifies
/// the desired output format via the MIME type parameter.
///
/// @param[in] action   Pointer to the OCR image action context containing
///                     state information.
/// @param[in] filename The target file path where the image should be saved
///                     (UCS2 string).
/// @param[in] mimetype The MIME type specifying the output format (e.g.,
///                     L"image/png", L"image/jpeg").
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if the specified MIME type is not supported or file
///         I/O fails.
///
/// @ingroup ocr_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT*
                            IGR_Open_Callback_Action_OCR_Image_SaveImage_Function)(const struct IGR_Open_Callback_Action_OCR_Image* action,
                                                                                   const IGR_UCS2* filename, const IGR_UCS2* mimetype);

/// @brief Function to mark the start of a new structured block element in
///        output.
///
/// Called to begin a new logical block (paragraph, heading, table, etc.) in
/// the OCR output. Blocks can be nested and must be properly balanced with
/// EndBlock calls. The spatial boundary of the block is defined by the
/// provided rectangle.
///
/// @param[in] action     Pointer to the OCR image action context containing
///                       state information.
/// @param[in] block_type The type of block being started. See @ref
///                       ocr_block_types for available block type constants.
/// @param[in] rect       Pointer to the bounding rectangle defining the
///                       spatial extent of this block in the image coordinate
///                       system.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if the block_type is invalid or nesting depth limits
///         are exceeded.
///
/// @ingroup ocr_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_Open_Callback_Action_OCR_Image_StartBlock_Function)(const struct IGR_Open_Callback_Action_OCR_Image*
                                                                                                action,
                                                                                            OCR_BLOCK_TYPE block_type,
                                                                                            const struct IGR_QuadPoint* rect);

/// @brief Function to mark the end of the current block element in output.
///
/// Called to close the most recently opened block at this nesting level.
/// EndBlock calls must match corresponding StartBlock calls in a properly
/// balanced manner. Any text or nested blocks added since the matching
/// StartBlock are now finalized.
///
/// @param[in] action     Pointer to the OCR image action context containing
///                       state information.
/// @param[in] block_type The type of the block being closed (must match the
///                       corresponding StartBlock block_type). See @ref
///                       ocr_block_types for available block type constants.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if block_type does not match the current open block or
///         no block is currently open.
///
/// @ingroup ocr_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_Open_Callback_Action_OCR_Image_EndBlock_Function)(const struct IGR_Open_Callback_Action_OCR_Image*
                                                                                              action,
                                                                                          OCR_BLOCK_TYPE block_type);

/// @brief Function to add recognized text to the output with styling
///        information.
///
/// Called to insert text extracted/recognized by the OCR engine into the
/// current output block. The text includes its spatial location, visual
/// styling (font, size, color), and rendering flags. Character-level
/// positioning is available via AddTextWithOffsets for more precise layouts.
///
/// @param[in] action     Pointer to the OCR image action context containing
///                       state information.
/// @param[in] text       The recognized text to add (null-terminated UCS2
///                       string).
/// @param[in] rect       Pointer to the bounding rectangle defining the
///                       spatial extent of this text in the image coordinate
///                       system.
/// @param[in] flags      Rendering flags controlling text appearance. See
///                       callback documentation for flag options.
/// @param[in] font_style Pointer to style information (font family, size,
///                       color, etc.) detected by the OCR engine. May be NULL
///                       if no style information is available.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if a required parameter is invalid or output buffer is
///         full.
///
/// @ingroup ocr_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_Open_Callback_Action_OCR_Image_AddText_Function)(
    const struct IGR_Open_Callback_Action_OCR_Image* action, const IGR_UCS2* text, const struct IGR_QuadPoint* rect,
    IGR_DESCRIBE_IMAGE_ADDTEXT_FLAGS_TYPE flags, const struct IGR_Open_Callback_Action_OCR_Image_Style_Info* font_style);

/// @brief Function to add text with precise per-character positional offsets
///        to output.
///
/// Called to insert recognized text with character-level position information
/// for high-precision layout preservation. Each character's horizontal offset
/// from the block start is provided, enabling accurate reconstruction of the
/// original text positioning. Useful for maintaining column alignment,
/// tabulation, and other precise spatial relationships.
///
/// @param[in] action     Pointer to the OCR image action context containing
///                       state information.
/// @param[in] text       The recognized text to add (null-terminated UCS2
///                       string).
/// @param[in] offsets    Array of character offsets (in pixels or document
///                       units) for each character in the text string. Array
///                       length must match the number of characters in @p
///                       text. May be NULL if no character-level positioning
///                       is available.
/// @param[in] rect       Pointer to the bounding rectangle defining the
///                       overall spatial extent of this text in the image
///                       coordinate system.
/// @param[in] flags      Rendering flags controlling text appearance. See
///                       callback documentation for flag options.
/// @param[in] font_style Pointer to style information (font family, size,
///                       color, etc.) detected by the OCR engine. May be NULL
///                       if no style information is available.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if a required parameter is invalid or output buffer is
///         full.
///
/// @ingroup ocr_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_Open_Callback_Action_OCR_Image_AddTextWithOffsets_Function)(
    const struct IGR_Open_Callback_Action_OCR_Image* action, const IGR_UCS2* text, const IGR_FLOAT* offsets,
    const struct IGR_QuadPoint* rect, IGR_ULONG flags, const struct IGR_Open_Callback_Action_OCR_Image_Style_Info* font_style);

/// @brief Function to apply rotation/reorientation to the processed image.
///
/// Called to rotate the OCR-processed image by the specified angle. This is
/// typically used to correct skewed or upside-down images detected during
/// processing. The rotation is applied to the image after text extraction but
/// before final output, adjusting all spatial coordinates and bounding boxes
/// accordingly.
///
/// @param[in] action  Pointer to the OCR image action context containing
///                    state information.
/// @param[in] degrees The rotation angle in degrees. Positive values rotate
///                    counter-clockwise; negative values rotate clockwise.
///                    Common values: 0 (no rotation), 90, 180, 270, or
///                    fractional values for skew correction (e.g., 1.5 for
///                    slight rotation).
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if the rotation angle is invalid or image
///         transformation fails.
///
/// @ingroup ocr_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_Open_Callback_Action_OCR_Image_Reorient_Function)(const struct IGR_Open_Callback_Action_OCR_Image*
                                                                                              action,
                                                                                          IGR_FLOAT degrees);

/** @} */ // ocr_image_callbacks

/**
 * @defgroup describe_image_callbacks Describe Image Callbacks
 * @brief Callbacks for processing image descriptions and metadata extraction.
 *
 * These callbacks are invoked when describing images, extracting metadata,
 * and generating accessibility information about images in documents.
 *
 * @ingroup callback_functions
 * @{
 */

/// @struct IGR_Open_Callback_Action_OCR_Image
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when processing
///        OCR images.
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
	IGR_Open_Callback_Action_OCR_Image_GetMetadata_Function GetMetadata;

	/// @brief Function to save image to file with specified MIME type.
	IGR_Open_Callback_Action_OCR_Image_SaveImage_Function SaveImage;

	/// @brief Function to start a new block element in output.
	IGR_Open_Callback_Action_OCR_Image_StartBlock_Function StartBlock;

	/// @brief Function to close current block element in output.
	IGR_Open_Callback_Action_OCR_Image_EndBlock_Function EndBlock;

	/// @brief Function to add text to output.
	IGR_Open_Callback_Action_OCR_Image_AddText_Function AddText;

	/// @brief Function to add text with character offsets to output.
	IGR_Open_Callback_Action_OCR_Image_AddTextWithOffsets_Function AddTextWithOffsets;

	/// @brief Function to reorient processed image.
	IGR_Open_Callback_Action_OCR_Image_Reorient_Function Reorient;
};

/// @struct IGR_Open_Callback_Action_Describe_Image
/// @brief Forward declaration of the describe-image action context.
///
/// Declared ahead of the callbacks that take a pointer to it, for the same
/// reason as \ref IGR_Open_Callback_Action_OCR_Image above.
struct IGR_Open_Callback_Action_Describe_Image;

/// @brief Function to retrieve the pixel data of the source image being
///        processed.
///
/// Called to obtain the raw image pixel data (DIB - Device Independent
/// Bitmap) for further analysis or processing. The returned DIB contains the
/// image pixels in a standard format.
///
/// @param[in] action Pointer to the Describe_Image action context containing
///                   state information.
///
/// @return Pointer to IGR_Open_DIB_Info structure containing the pixel data,
///         or NULL if the image data cannot be retrieved or is not available.
///         The returned structure, along with its pixel_data and palette
///         buffers, is owned by Document Filters and remains valid only for
///         the duration of the callback invocation; copy any data that is
///         needed beyond that point.
///
/// @ingroup describe_image_callbacks
typedef const struct IGR_Open_DIB_Info*(IGR_EXPORT* IGR_Open_Callback_Action_Describe_Image_GetSourceImagePixels_Function)(
    const struct IGR_Open_Callback_Action_Describe_Image* action);

/// @brief Function to save the processed image to disk with a specified
///        format.
///
/// Called to persist the image (after description/OCR processing) to a file.
/// The caller specifies the desired output format via the MIME type
/// parameter. This is used when generating image output artifacts during
/// document processing.
///
/// @param[in] action   Pointer to the Describe_Image action context
///                     containing state information.
/// @param[in] filename The target file path where the image should be saved
///                     (UCS2 string).
/// @param[in] mimetype The MIME type specifying the output format (e.g.,
///                     L"image/png", L"image/jpeg").
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if the specified MIME type is not supported or file
///         I/O fails.
///
/// @ingroup describe_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_Open_Callback_Action_Describe_Image_SaveImage_Function)(
    const struct IGR_Open_Callback_Action_Describe_Image* action, const IGR_UCS2* filename, const IGR_UCS2* mimetype);

/// @brief Function to add recognized or extracted text from the image to the
///        output.
///
/// Called to insert text recognized by the image description/OCR engine into
/// the document output. Unlike AddText in OCR_Image, this variant takes
/// simplified parameters without spatial positioning or font styling
/// information, used when outputting descriptive text or accessibility
/// information about the image.
///
/// @param[in] action Pointer to the Describe_Image action context containing
///                   state information.
/// @param[in] text   The text to add (null-terminated UCS2 string). Can be
///                   recognized text, image description, alternative text, or
///                   metadata.
/// @param[in] flags  Processing flags controlling text handling. Refer to
///                   callback flag documentation for available options.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if a required parameter is invalid or output buffer is
///         full.
///
/// @ingroup describe_image_callbacks
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_Open_Callback_Action_Describe_Image_AddText_Function)(
    const struct IGR_Open_Callback_Action_Describe_Image* action, const IGR_UCS2* text, const IGR_ULONG flags);

/** @} */ // describe_image_callbacks

/// @struct IGR_Open_Callback_Action_Describe_Image
/// @brief Structure passed as payload for IGR_OPEN_CALLBACK when processing
///        images.
struct IGR_Open_Callback_Action_Describe_Image
{
	/// @brief Size of this structure, must be populated by caller.
	IGR_ULONG struct_size;

	/// @brief Reserved for internal use.
	void* reserved;

	/// @brief Getter for the image data to be processed.
	IGR_Open_Callback_Action_Describe_Image_GetSourceImagePixels_Function GetSourceImagePixels;

	/// @brief Page index containing the image.
	IGR_ULONG source_page_index;

	/// @brief Rectangle location of image in source document.
	struct IGR_Rect source_rect;

	/// @brief Type of image
	IGR_DESCRIBE_IMAGE_TYPE source_type;

	/// @brief Name of source image if known.
	IGR_UCS2 source_name[128];

	/// @brief Existing alt text if any.
	IGR_UCS2 existing_alt_text[4096];

	/// @brief Function to save image to file with specified MIME type.
	IGR_Open_Callback_Action_Describe_Image_SaveImage_Function SaveImage;

	/// @brief Function to add text to output.
	IGR_Open_Callback_Action_Describe_Image_AddText_Function AddText;
};

/// @struct IGR_ML_Property
/// @brief One setting a machine learning model is configured with.
///
/// These are the options the model was registered with, not the options it
/// would accept: they say how this installation runs the model, such as which
/// backbone its artifact holds or how much text a caption model is asked for.
/// Nothing here has to be understood to use the model.
struct IGR_ML_Property
{
	/// @brief Size of this structure, populated by Document Filters.
	IGR_ULONG struct_size;

	/// @brief Name of the setting, such as "backbone". Never null.
	const IGR_UCS2* name;

	/// @brief Value of the setting as text, such as "clip". Never null.
	///
	/// A value that is not text is spelled as text, with @c type saying what
	/// the spelling means.
	const IGR_UCS2* value;

	/// @brief How to read @c value. A value from @ref ml_property_types.
	IGR_ML_PROPERTY_TYPE type;
};

/// @struct IGR_ML_Model
/// @brief A machine learning model this installation can name.
///
/// Owned by Document Filters, and valid only until the enumeration callback
/// that received it returns. A caller keeping any part of it past that point
/// must copy what it needs, strings included.
struct IGR_ML_Model
{
	/// @brief Size of this structure, populated by Document Filters.
	///
	/// A caller reads a field only after checking that @c struct_size covers
	/// it, so that a binary built against this header keeps working against a
	/// later one that has grown the structure.
	IGR_ULONG struct_size;

	/// @brief Flags describing the model. Values from @ref ml_model_flags.
	IGR_ULONG flags;

	/// @brief The name that selects this model, such as
	///        "clip-image-classifier". Never null.
	const IGR_UCS2* name;

	/// @brief What the model does. A value from @ref ml_model_types.
	IGR_ML_MODEL_TYPE type;

	/// @brief Number of entries in @c properties. Zero when the model carries
	///        no settings.
	IGR_ULONG property_count;

	/// @brief The settings the model is configured with, or null when
	///        @c property_count is zero.
	const struct IGR_ML_Property* properties;
};

/// @brief Callback function for enumerating machine learning models.
///
/// @param[in] model   The model being reported. Owned by Document Filters and
///                    valid only until this call returns.
/// @param[in] context User provided context.
///
/// @return IGR_OK to continue the enumeration, or any other @ref error_codes
///         value to stop it. The value returned to stop becomes the return
///         value of IGR_ML_Enum_Models.
/// @ingroup callback_functions
typedef IGR_RETURN_CODE(IGR_EXPORT* IGR_ML_MODEL_CALLBACK)(const struct IGR_ML_Model* model, void* context);

/// @brief The highest resolution a model is given pixels at.
///
/// Above this, the extra pixels buy nothing a model can use and cost a render
/// that grows with the square of the resolution. IGR_ML_Render::dpi is clamped
/// to this rather than refused.
#define IGR_ML_MAX_DPI 300

/// @struct IGR_ML_Render
/// @brief Which part of a page a model is given, and at what resolution.
///
/// Separates the two axes that matter. @c source_rect says which part of the
/// page, in the page pixel coordinates every other page call uses. @c dpi says
/// how much detail that part is rendered with, which is deliberately not tied
/// to the document's own resolution: raising GRAPHIC_DPI to feed a model would
/// move the coordinate space of every rect and word bound the caller already
/// holds.
///
/// Pass a null IGR_ML_Render for the whole page at the model's preferred
/// resolution, which is what most callers want.
struct IGR_ML_Render
{
	/// @brief Size of this structure, must be populated.
	IGR_ULONG struct_size;

	/// @brief Reserved for future use, must be zero.
	IGR_ULONG flags;

	/// @brief The part of the page to analyse, in page pixels.
	///
	/// The same coordinates IGR_Get_Page_Dimensions reports and
	/// IGR_Get_Page_Pixels takes. All four members zero asks for the whole
	/// page, a zero-area rectangle being meaningless.
	struct IGR_Rect source_rect;

	/// @brief Resolution to render at, in dots per inch, or zero to let the
	///        model choose.
	///
	/// Zero falls back to the model's registered @c dpi setting, and then to
	/// the document's own resolution. Whatever the source, the value is
	/// clamped to @ref IGR_ML_MAX_DPI.
	///
	/// Worth setting for a small region: a one inch figure rendered at the
	/// document's usual 96 dpi is 96 pixels square, which a model wanting 224
	/// can only upscale. Raising it re-renders the page rather than scaling
	/// up the pixels a lower resolution produced.
	IGR_ULONG dpi;
};

/// @struct IGR_ML_Result
/// @brief The prefix every machine learning result structure begins with.
///
/// A result says what it is, so one entry point serves every kind of model and
/// each result structure can grow without disturbing the others. A caller
/// fills in both members, passes the concrete structure to IGR_ML_Run_Page,
/// and reads the members that structure defines.
///
/// This structure is never used on its own. @c type selects which concrete
/// structure is in play, and so which action runs:
///
/// - IGR_ML_MODEL_TYPE_CLASSIFIER, \ref IGR_ML_Classification
/// - IGR_ML_MODEL_TYPE_CAPTIONER, \ref IGR_ML_Description
struct IGR_ML_Result
{
	/// @brief Size of the concrete structure, must be populated.
	IGR_ULONG struct_size;

	/// @brief Which concrete structure this is, and so which action to run.
	///        A value from @ref ml_model_types.
	IGR_ML_MODEL_TYPE type;
};

/// @struct IGR_ML_Classification
/// @brief What a classifier concluded about a page or a region of one.
///
/// Set @c struct_size and @c type before the call; everything else is output.
struct IGR_ML_Classification
{
	/// @brief Size of this structure, must be populated.
	IGR_ULONG struct_size;

	/// @brief Must be IGR_ML_MODEL_TYPE_CLASSIFIER.
	IGR_ML_MODEL_TYPE type;

	/// @brief The class the model settled on, as the backend numbers them.
	///
	/// Dense and stable within one backend build, so it can index a table the
	/// caller built from its own enumeration of the classes.
	IGR_ULONG class_id;

	/// @brief The class as the routing rules name it, such as
	///        "chart_or_infographic". Always terminated.
	///
	/// Taken from the backend's taxonomy rather than a table compiled into
	/// Document Filters, so a backend that adds a class is understood without
	/// rebuilding the caller.
	IGR_UCS2 label[64];

	/// @brief How sure the model is, from 0 to 100.
	///
	/// A whole percentage, which is the precision the backend reports and all
	/// a routing threshold needs.
	IGR_ULONG confidence;

	/// @brief The model that produced the classification. Always terminated.
	IGR_UCS2 model[128];
};

/// @struct IGR_ML_Description
/// @brief What a caption model made of a page or a region of one.
///
/// Set @c struct_size, @c type, @c description and @c description_size before
/// the call; the rest is output.
struct IGR_ML_Description
{
	/// @brief Size of this structure, must be populated.
	IGR_ULONG struct_size;

	/// @brief Must be IGR_ML_MODEL_TYPE_CAPTIONER.
	IGR_ML_MODEL_TYPE type;

	/// @brief Buffer the description is written to, or null to ask only for
	///        the length.
	///
	/// Always terminated when non-null, and one character of the capacity is
	/// spent on that terminator. A description too long for the buffer is
	/// truncated rather than refused, and the call still returns IGR_OK.
	IGR_UCS2* description;

	/// @brief On input the capacity of @c description in characters; on output
	///        the length the description needs, excluding the terminator.
	///
	/// Reporting the length needed rather than the length written is what lets
	/// a caller notice truncation. Do not use that to size a second call
	/// unless nothing else will do: generating a caption costs seconds, and
	/// asking again pays that cost twice. A generous buffer usually pays it
	/// once.
	IGR_ULONG description_size;

	/// @brief How sure the model is, from 0 to 100, or zero when it does not
	///        say. Caption models generally do not.
	IGR_ULONG confidence;

	/// @brief The model that produced the description. Always terminated.
	IGR_UCS2 model[128];
};

/// @struct IGR_Page_Element
/// @brief Structure representing a page element including position, type and
///        flags.
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
/// @param[in] name    Name of the style.
/// @param[in] value   Value of the style.
/// @param[in] context User provided context.
///
/// @return @ref error_codes value indicating success or failure.
/// @ingroup callback_functions
typedef IGR_LONG(IGR_EXPORT* IGR_PAGE_ELEMENT_STYLES_CALLBACK)(const IGR_UCS2* name, const IGR_UCS2* value, void* context);

/// @brief Callback function for processing page elements.
///
/// @param[in] page    Handle to the page.
/// @param[in] item    Pointer to the page element. The element's @c type
///                    field contains a value from @ref page_element_types,
///                    and the @c flags field contains bitfield values from
///                    @ref page_element_flags.
/// @param[in] context User provided context.
///
/// @return @ref error_codes value indicating how to proceed with processing.
/// @ingroup callback_functions
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
/// @brief Structure representing a difference item between compared
///        documents.
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

/**
 * @defgroup text_compare_callbacks Text Comparison Callbacks
 * @brief Callbacks for document text comparison operations.
 *
 * These callbacks are invoked during document comparison to manage page
 * resources, define comparison boundaries, and handle errors.
 *
 * @ingroup callback_functions
 * @{
 */

/// @brief Callback function to open and obtain a handle to a page resource.
///
/// Called during comparison to load and prepare a specific page for analysis.
/// The page handle returned must remain valid until the corresponding
/// ClosePage callback is invoked.
///
/// @param[in]  context     User-provided context pointer (from
///                         IGR_Text_Compare_Settings.user_context).
/// @param[in]  info        Comparison context information including document
///                         and source identifiers.
/// @param[in]  page_index  Zero-based index of the page to open. The page
///                         numbering corresponds to the pages retrieved via
///                         GetNextPageIndex().
/// @param[out] page_handle Pointer to receive the opened page handle. Must be
///                         non-NULL on success.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if the page cannot be opened or does not exist.
/// @see IGR_Text_Compare_Settings_GetNextPageIndex_Function
///
/// @ingroup text_compare_callbacks
typedef IGR_RETURN_CODE(CALLBACK* IGR_Text_Compare_Settings_OpenPage_Function)(void* context,
                                                                               const struct IGR_Compare_Documents_Callback_Context* info,
                                                                               IGR_ULONG page_index, IGR_HPAGE* page_handle);

/// @brief Callback function to release a previously opened page resource.
///
/// Called to close and release resources associated with a page handle that
/// was obtained via OpenPage. After this call, the page handle must not be
/// used further.
///
/// @param[in] context     User-provided context pointer (from
///                        IGR_Text_Compare_Settings.user_context).
/// @param[in] info        Comparison context information including document
///                        and source identifiers.
/// @param[in] page_handle The page handle to close, obtained from a preceding
///                        OpenPage call.
///
/// @return @ref error_codes code indicating success or failure.
/// @see IGR_Text_Compare_Settings_OpenPage_Function
///
/// @ingroup text_compare_callbacks
typedef IGR_RETURN_CODE(CALLBACK* IGR_Text_Compare_Settings_ClosePage_Function)(void* context,
                                                                                const struct IGR_Compare_Documents_Callback_Context* info,
                                                                                IGR_HPAGE page_handle);

/// @brief Callback function to retrieve the margin boundaries of a page in
///        pixels.
///
/// Called to determine the margin dimensions around the page content. These
/// margins define the boundaries where comparison should occur, excluding
/// header/footer areas or other non-content regions. Used in conjunction with
/// GetPageArea to define the content region.
///
/// @param[in]  context User-provided context pointer (from
///                     IGR_Text_Compare_Settings.user_context).
/// @param[in]  info    Comparison context information including document and
///                     source identifiers.
/// @param[out] margins Pointer to receive the margin boundaries (left, top,
///                     right, bottom in pixels).
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if not implemented to use default margins.
/// @see IGR_Text_Compare_Settings_GetPageArea_Function
///
/// @ingroup text_compare_callbacks
typedef IGR_RETURN_CODE(CALLBACK* IGR_Text_Compare_Settings_GetPageMargins_Function)(void* context,
                                                                                     const struct IGR_Compare_Documents_Callback_Context*
                                                                                         info,
                                                                                     struct IGR_FRect* margins);

/// @brief Callback function to retrieve the active page area for text
///        comparison.
///
/// Called to define the region of the page where text comparison should
/// occur. This allows excluding specific areas (such as page numbers,
/// headers, or footers) from comparison. Works in conjunction with
/// GetPageMargins to define the content region.
///
/// @param[in]  context   User-provided context pointer (from
///                       IGR_Text_Compare_Settings.user_context).
/// @param[in]  info      Comparison context information including document
///                       and source identifiers.
/// @param[out] page_area Pointer to receive the page area rectangle
///                       coordinates.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_ERROR if not implemented to use the entire page.
/// @see IGR_Text_Compare_Settings_GetPageMargins_Function
///
/// @ingroup text_compare_callbacks
typedef IGR_RETURN_CODE(CALLBACK* IGR_Text_Compare_Settings_GetPageArea_Function)(void* context,
                                                                                  const struct IGR_Compare_Documents_Callback_Context* info,
                                                                                  struct IGR_FRect* page_area);

/// @brief Callback function to retrieve the next page index to process in the
///        comparison.
///
/// Called to determine which page should be compared next. The comparison
/// engine uses this to allow custom page ordering or filtering, enabling
/// comparisons of non-sequential pages or subset selection. The returned page
/// indices are passed to OpenPage for loading.
///
/// @param[in]  context    User-provided context pointer (from
///                        IGR_Text_Compare_Settings.user_context).
/// @param[in]  info       Comparison context information including document
///                        and source identifiers.
/// @param[out] page_index Pointer to receive the next page index to process.
///                        Set to a value larger than the total page count to
///                        signal end of comparison.
///
/// @return @ref error_codes code indicating success or failure. Return
///         IGR_E_EOF when no more pages should be processed.
/// @see IGR_Text_Compare_Settings_OpenPage_Function
///
/// @ingroup text_compare_callbacks
typedef IGR_RETURN_CODE(CALLBACK* IGR_Text_Compare_Settings_GetNextPageIndex_Function)(void* context,
                                                                                       const struct IGR_Compare_Documents_Callback_Context*
                                                                                           info,
                                                                                       IGR_ULONG* page_index);

/// @brief Callback function for handling errors during comparison operations.
///
/// Called when an error or exception occurs during document comparison. The
/// callback receives detailed error information and can decide whether to
/// continue or abort the comparison.
///
/// @param[in] context User-provided context pointer (from
///                    IGR_Text_Compare_Settings.user_context).
/// @param[in] info    Comparison context information including document and
///                    source identifiers.
/// @param[in] ecb     Error control block containing error details, location,
///                    and severity information.
///
/// @return @ref error_codes value indicating how to proceed. Return IGR_OK to
///         continue comparison, or IGR_CANCELLED to stop the comparison
///         immediately.
///
/// @ingroup text_compare_callbacks
typedef IGR_RETURN_CODE(CALLBACK* IGR_Text_Compare_Settings_HandleError_Function)(void* context,
                                                                                  const struct IGR_Compare_Documents_Callback_Context* info,
                                                                                  const Error_Control_Block* ecb);

/** @} */ // text_compare_callbacks

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
	IGR_Text_Compare_Settings_OpenPage_Function OpenPage;

	/// @brief Callback to close a page resource.
	IGR_Text_Compare_Settings_ClosePage_Function ClosePage;

	/// @brief Callback to get page margins in pixels.
	IGR_Text_Compare_Settings_GetPageMargins_Function GetPageMargins;

	/// @brief Callback to get page area for text comparison.
	IGR_Text_Compare_Settings_GetPageArea_Function GetPageArea;

	/// @brief Callback to get next page index to process.
	IGR_Text_Compare_Settings_GetNextPageIndex_Function GetNextPageIndex;

	/// @brief Callback for error handling.
	IGR_Text_Compare_Settings_HandleError_Function HandleError;
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
///        license.
///
/// @param[in]     reserved Unused, must be zero.
/// @param[in]     path     Path to Perceptive executables.
/// @param[in,out] init     On input, contains application License Code; on
///                         output licensee information.
/// @param[out]    handle   Returns the instance handle.
/// @param[out]    error    Contains any error text.
void IGR_EXPORT Init_Instance(IGR_LONG reserved, const char* path, Instance_Status_Block* init, IGR_SHORT* handle,
                              Error_Control_Block* error);

/// @brief Advises the Document Filters engine that the program is finished.
///
/// @param[out] error Contains any error text.
void IGR_EXPORT Close_Instance(Error_Control_Block* error);

/// @brief Legacy method, new code does not need to call this method.
///
/// @param[in]  handle Instance handle.
/// @param[out] error  Contains any error text.
void IGR_EXPORT Switch_Instance(IGR_SHORT handle, Error_Control_Block* error);

/// @brief Converts a UCS2 string into UTF-8.
///
/// @param[in]  source     Contains a NULL terminated UCS2 string.
/// @param[out] buffer     Pointer to a buffer that will hold the converted
///                        text.
/// @param[in]  bufferSize Indicates the size of the buffer in bytes,
///                        including NULL.
void IGR_EXPORT Widechar_to_UTF8(const IGR_UCS2* source, char* buffer, IGR_LONG bufferSize);

/// @brief Converts a UCS2 string into UTF-8 with explicit source length.
///
/// @param[in]  source     Contains a UCS2 string.
/// @param[in]  sourceLen  Length of source string in characters.
/// @param[out] buffer     Pointer to a buffer that will hold the converted
///                        text.
/// @param[in]  bufferSize Indicates the size of the buffer in bytes,
///                        including NULL.
void IGR_EXPORT Widechar_to_UTF8_Ex(const IGR_UCS2* source, IGR_LONG sourceLen, char* buffer, IGR_LONG bufferSize);

/// @brief Converts a UTF-8 string into a UCS2 string.
///
/// @param[in]  source     Contains a NULL terminated UTF-8 string.
/// @param[out] buffer     Pointer to a buffer that will hold the converted
///                        text.
/// @param[in]  bufferSize Indicates the size of the buffer in characters,
///                        including NULL.
void IGR_EXPORT UTF8_to_Widechar(const char* source, IGR_UCS2* buffer, IGR_LONG bufferSize);

/// @brief Converts a UTF-8 string into a UCS2 string with explicit source
///        length.
///
/// @param[in]  source     Contains a UTF-8 string.
/// @param[in]  sourceLen  Length of the source string.
/// @param[out] buffer     Pointer to a buffer that will hold the converted
///                        text.
/// @param[in]  bufferSize Indicates the size of the buffer in characters,
///                        including NULL.
void IGR_EXPORT UTF8_to_Widechar_Ex(const char* source, IGR_LONG sourceLen, IGR_UCS2* buffer, IGR_LONG bufferSize);

/// @brief Legacy function for engine call tracing, not needed for new code.
///
/// @param[in] param1 Unused parameter.
/// @param[in] param2 Unused parameter.
/// @param[in] param3 Unused parameter.
void IGR_EXPORT Set_Engine_Call_Tracing(IGR_LONG param1, IGR_LONG param2, const char* param3);

/// @brief Legacy function for setting API character set, not needed for new
///        code.
///
/// @param[in]  param1 Unused parameter.
/// @param[out] error  Contains any error text.
void IGR_EXPORT Set_API_Charset(IGR_LONG param1, Error_Control_Block* error);

/// @brief Legacy function for setting DLL options, not needed for new code.
///
/// @param[in]  param1 Unused parameter.
/// @param[out] error  Contains any error text.
void IGR_EXPORT Set_DLL_Option(const char* param1, Error_Control_Block* error);

/// @brief Legacy multiplex function, not needed for new code.
///
/// @param[in]     param1 Unused parameter.
/// @param[in,out] param2 Unused parameter.
/// @param[in,out] param3 Unused parameter.
/// @param[out]    error  Contains any error text.
void IGR_EXPORT ISYS_Multiplex(IGR_LONG param1, size_t* param2, size_t* param3, Error_Control_Block* error);

/// @brief Gets the type and capabilities of a given document.
///
/// @param[in]  filename     Path to the document to be opened.
/// @param[out] capabilities Returns the Document Capabilities as a bitfield.
/// @param[out] filetype     Returns the Document Format Code of the document.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p capabilities is a null pointer, or @p
///                               filetype is a null pointer.
/// @retval IGR_E_NOT_READABLE    The file could not be opened, or its format
///                               could not be recognized.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_File_Type(const IGR_UCS2* filename, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype,
                                             Error_Control_Block* error);

/// @brief Gets the type and capabilities of a given stream object.
///
/// @param[in]  stream       The stream can be either user implemented or
///                          created using IGR_Make_Stream_From_File and
///                          IGR_Make_Stream_From_Memory.
/// @param[out] capabilities Returns the Document Capabilities as a bitfield.
/// @param[out] filetype     Returns the Document Format Code of the document.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p stream is a null pointer, @p capabilities
///                               is a null pointer, or @p filetype is a null
///                               pointer.
/// @retval IGR_E_NOT_READABLE    The stream's format could not be recognized.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Stream_Type(struct IGR_Stream* stream, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype,
                                               Error_Control_Block* error);

/// @brief Opens a document for text extraction or enumeration of
///        sub-documents.
///
/// @param[in]  filename     Path to the document to be opened.
/// @param[in]  flags        Specifies processing options that control the
///                          output. See Open Document Flags.
/// @param[out] capabilities Returns the Document Capabilities as a bitfield.
/// @param[out] filetype     Returns the Document Format Code of the document.
/// @param[out] handle       Returns a handle to be used in subsequent calls.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_POINTER          @p capabilities is a null pointer,
///                                        or @p filetype is a null pointer.
/// @retval IGR_E_TOO_MANY_HANDLES         Too many documents are already
///                                        open.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS The format does not support a
///                                        required capability, or a needed
///                                        feature is disabled.
/// @retval IGR_E_PASSWORD                 The document is encrypted and
///                                        requires a password.
/// @retval IGR_E_DRM_PROTECTED            The document is protected by DRM.
/// @retval IGR_E_FILE_CORRUPT             The document is corrupt.
/// @retval IGR_CANCELLED                  The operation was cancelled by a
///                                        user-supplied callback.
/// @retval IGR_E_NOT_READABLE             The document could not be opened,
///                                        or its format could not be
///                                        recognized.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_File(const IGR_UCS2* filename, IGR_OPEN_FLAGS_TYPE flags, IGR_FILE_SUPPORTS_TYPE* capabilities,
                                         IGR_LONG* filetype, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Opens a document for text extraction or enumeration of
///        sub-documents from a stream.
///
/// @param[in]  stream       The stream can be either user implemented, or
///                          created using IGR_Make_Stream_From_File and
///                          IGR_Make_Stream_From_Memory.
/// @param[in]  flags        Specifies processing options that control the
///                          output. See Open Document Flags.
/// @param[out] capabilities Returns the Document Capabilities as a bitfield.
/// @param[out] filetype     Returns the Document Format Code of the document.
/// @param[out] handle       Returns a handle to be used in subsequent calls.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_POINTER          @p stream is a null pointer, @p
///                                        capabilities is a null pointer, or
///                                        @p filetype is a null pointer.
/// @retval IGR_E_TOO_MANY_HANDLES         Too many documents are already
///                                        open.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS The format does not support a
///                                        required capability, or a needed
///                                        feature is disabled.
/// @retval IGR_E_PASSWORD                 The document is encrypted and
///                                        requires a password.
/// @retval IGR_E_DRM_PROTECTED            The document is protected by DRM.
/// @retval IGR_E_FILE_CORRUPT             The document is corrupt.
/// @retval IGR_CANCELLED                  The operation was cancelled by a
///                                        user-supplied callback.
/// @retval IGR_E_NOT_READABLE             The document could not be opened,
///                                        or its format could not be
///                                        recognized.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_Stream(struct IGR_Stream* stream, IGR_OPEN_FLAGS_TYPE flags, IGR_FILE_SUPPORTS_TYPE* capabilities,
                                           IGR_LONG* filetype, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Opens a document with extended options for text extraction or
///        enumeration of sub-documents.
///
/// @param[in]  filename     Path to the document to be opened.
/// @param[in]  flags        Specifies processing options that control the
///                          output. See Open Document Flags.
/// @param[in]  options      Extended processing options as Name=Value pairs
///                          joined by semicolons, or NULL for the defaults.
/// @param[out] capabilities Returns the Document Capabilities as a bitfield.
/// @param[out] filetype     Returns the Document Format Code of the document.
/// @param[out] handle       Returns a handle to be used in subsequent calls.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_POINTER          @p capabilities is a null pointer,
///                                        or @p filetype is a null pointer.
/// @retval IGR_E_TOO_MANY_HANDLES         Too many documents are already
///                                        open.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS The format does not support a
///                                        required capability, or a needed
///                                        feature is disabled.
/// @retval IGR_E_PASSWORD                 The document is encrypted and
///                                        requires a password.
/// @retval IGR_E_DRM_PROTECTED            The document is protected by DRM.
/// @retval IGR_E_FILE_CORRUPT             The document is corrupt.
/// @retval IGR_CANCELLED                  The operation was cancelled by a
///                                        user-supplied callback.
/// @retval IGR_E_NOT_READABLE             The document could not be opened,
///                                        or its format could not be
///                                        recognized.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_File_Ex(const IGR_UCS2* filename, IGR_OPEN_FLAGS_TYPE flags, const IGR_UCS2* options,
                                            IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype, IGR_LONG* handle,
                                            Error_Control_Block* error);

/// @brief Opens a document from a stream with extended options for text
///        extraction or enumeration of sub-documents.
///
/// @param[in]  stream       The stream can be either user implemented, or
///                          created using IGR_Make_Stream_From_File and
///                          IGR_Make_Stream_From_Memory.
/// @param[in]  flags        Specifies processing options that control the
///                          output. See Open Document Flags.
/// @param[in]  options      Extended processing options as Name=Value pairs
///                          joined by semicolons, or NULL for the defaults.
/// @param[out] capabilities Returns the Document Capabilities as a bitfield.
/// @param[out] file_type    Returns the Document Format Code of the document.
/// @param[out] handle       Returns a handle to be used in subsequent calls.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_POINTER          @p stream is a null pointer, @p
///                                        capabilities is a null pointer, or
///                                        @p file_type is a null pointer.
/// @retval IGR_E_TOO_MANY_HANDLES         Too many documents are already
///                                        open.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS The format does not support a
///                                        required capability, or a needed
///                                        feature is disabled.
/// @retval IGR_E_PASSWORD                 The document is encrypted and
///                                        requires a password.
/// @retval IGR_E_DRM_PROTECTED            The document is protected by DRM.
/// @retval IGR_E_FILE_CORRUPT             The document is corrupt.
/// @retval IGR_CANCELLED                  The operation was cancelled by a
///                                        user-supplied callback.
/// @retval IGR_E_NOT_READABLE             The document could not be opened,
///                                        or its format could not be
///                                        recognized.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_Stream_Ex(struct IGR_Stream* stream, IGR_OPEN_FLAGS_TYPE flags, const IGR_UCS2* options,
                                              IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* file_type, IGR_LONG* handle,
                                              Error_Control_Block* error);

/// @brief Extracts text from a previously opened document.
///
/// @param[in]     handle     Handle to document opened by
///                           IGR_Open_File/Stream.
/// @param[in,out] buffer     Application allocated memory block to store
///                           extracted text.
/// @param[in,out] bufferSize On input: size of buffer in Unicode chars. On
///                           output: number of chars extracted.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE           @p handle is not a valid document
///                                        handle.
/// @retval IGR_E_INVALID_POINTER          @p buffer is a null pointer, or @p
///                                        bufferSize is a null pointer.
/// @retval IGR_NO_MORE                    There is no more text to extract.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS The document does not support text
///                                        extraction.
/// @retval IGR_E_ERROR                    An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Text(IGR_LONG handle, IGR_UCS2* buffer, IGR_LONG* bufferSize, Error_Control_Block* error);

/// @brief Enumerates sub-documents contained in a previously opened compound
///        document.
///
/// @param[in]  handle Handle to document opened by IGR_Open_File/Stream.
/// @param[out] id     Buffer of 8192 bytes for 4096 Unicode chars containing
///                    sub-doc ID.
/// @param[out] name   Buffer of 2048 bytes for 1024 Unicode chars containing
///                    sub-doc name.
/// @param[out] date   Returns sub-document date/time in FILETIME format.
/// @param[out] size   Returns size of sub-document in bytes.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p handle is not a valid document handle.
/// @retval IGR_E_INVALID_POINTER @p id is a null pointer, or @p name is a
///                               null pointer.
/// @retval IGR_NO_MORE           There are no more sub-documents to
///                               enumerate.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfile_Entry(IGR_LONG handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size,
                                                 Error_Control_Block* error);

/// @brief Enumerates sub-documents contained in a compound document using
///        extended info.
///
/// @param[in]     handle Handle to document opened by IGR_Open_File/Stream.
/// @param[in,out] result Struct to populate with sub-document details.
/// @param[out]    error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p handle is not a valid document handle.
/// @retval IGR_E_INVALID_POINTER @p result is a null pointer.
/// @retval IGR_NO_MORE           There are no more sub-documents to
///                               enumerate.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfile_Entry_Ex(IGR_LONG handle, struct IGR_Subfile_Info* result, Error_Control_Block* error);

/// @brief Creates a new enumerator for iterating sub-documents in a compound
///        document.
///
/// @param[in]  docHandle  Handle to document opened by IGR_Open_File/Stream.
/// @param[out] enumerator Handle to new enumerator for sub-document
///                        iteration.
/// @param[out] error      Contains any error text.
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p docHandle is not a valid document handle.
/// @retval IGR_E_INVALID_POINTER @p enumerator is a null pointer.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfiles_Enumerator(IGR_LONG docHandle, HSUBFILES* enumerator, Error_Control_Block* error);

/// @brief Extracts a sub-document to disk from a compound document.
///
/// @param[in]  handle      Handle to document opened by IGR_Open_File/Stream.
/// @param[in]  id          ID of sub-document to extract, from
///                         IGR_Get_Subfile_Entry.
/// @param[in]  destination Path where sub-document will be written.
/// @param[out] error       Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p handle is not a valid document handle, or
///                               has already been closed.
/// @retval IGR_E_INVALID_POINTER @p id is a null pointer, or @p destination
///                               is a null pointer.
/// @retval IGR_E_NOT_FOUND       @p id does not match a sub-document in this
///                               compound document.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Subfile(IGR_LONG handle, const IGR_UCS2* id, const IGR_UCS2* destination,
                                               Error_Control_Block* error);

/// @brief Extracts a sub-document to a stream from a compound document.
///
/// @param[in]  handle Handle to document opened by IGR_Open_File/Stream.
/// @param[in]  id     ID of sub-document to extract, from
///                    IGR_Get_Subfile_Entry.
/// @param[out] stream Stream to write extracted sub-document content to.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK               Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE @p handle is not a valid document handle, or
///                              has already been closed.
/// @retval IGR_E_NOT_FOUND      @p id does not match a sub-document in this
///                              compound document.
/// @retval IGR_E_ERROR          An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Subfile_Stream(IGR_LONG handle, const IGR_UCS2* id, struct IGR_Stream** stream,
                                                      Error_Control_Block* error);

/// @brief Releases resources associated with an open document handle.
///
/// @param[in]  handle Handle to document opened by IGR_Open_File/Stream.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK               Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE @p handle is not a valid document handle, or
///                              has already been closed.
/// @retval IGR_E_ERROR          An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Close_File(IGR_LONG handle, Error_Control_Block* error);

/// @brief Provides extensible functionality to the Document Filters API.
///
/// @param[in]     function Function to perform, see Document Filters Manual.
/// @param[in,out] param1   Used by some functions.
/// @param[in,out] param2   Used by some functions.
/// @param[out]    error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK      Operation succeeded.
/// @retval IGR_E_ERROR An internal error occurred while performing the
///                     requested operation.
IGR_RETURN_CODE IGR_EXPORT IGR_Multiplex(IGR_LONG function, size_t* param1, size_t* param2, Error_Control_Block* error);

/// @brief Enumerates the next sub-document in an enumeration.
///
/// @param[in]  handle Handle from IGR_Get_Subfiles_Enumerator.
/// @param[out] id     Buffer of 8192 bytes for 4096 Unicode chars containing
///                    sub-doc ID.
/// @param[out] name   Buffer of 2048 bytes for 1024 Unicode chars containing
///                    sub-doc name.
/// @param[out] date   Returns sub-document date/time in FILETIME format.
/// @param[out] size   Returns size of sub-document in bytes.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p handle is not a valid enumerator
///                                 handle.
/// @retval IGR_NO_MORE             There are no more sub-documents to
///                                 enumerate.
/// @retval IGR_E_NOT_READABLE      The sub-document data could not be read.
IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Next(HSUBFILES handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size,
                                             Error_Control_Block* error);

/// @brief Enumerates the next sub-document using extended info.
///
/// @param[in]     handle Handle from IGR_Get_Subfiles_Enumerator.
/// @param[in,out] result Struct to populate with sub-document details.
/// @param[out]    error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p handle is not a valid enumerator
///                                 handle, or @p result is a null pointer.
/// @retval IGR_NO_MORE             There are no more sub-documents to
///                                 enumerate.
/// @retval IGR_E_NOT_READABLE      The sub-document data could not be read.
IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Next_Ex(HSUBFILES handle, struct IGR_Subfile_Info* result, Error_Control_Block* error);

/// @brief Resets enumeration to beginning for reprocessing.
///
/// @param[in]  handle Handle from IGR_Get_Subfiles_Enumerator.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p handle is not a valid enumerator
///                                 handle.
/// @retval IGR_E_NOT_READABLE      The enumerator could not be reset.
IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Reset(HSUBFILES handle, Error_Control_Block* error);

/// @brief Frees resources for a sub-document enumerator.
///
/// @param[in]  handle Handle from IGR_Get_Subfiles_Enumerator.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p handle is not a valid enumerator
///                                 handle.
/// @retval IGR_E_NOT_READABLE      The enumerator could not be closed.
IGR_RETURN_CODE IGR_EXPORT IGR_Subfiles_Close(HSUBFILES handle, Error_Control_Block* error);

/// @brief Converts a document to a plain text or HTML file without needing to
///        call IGR_Open_File and IGR_Get_Text.
///
/// @param[in]  filename Path to the document to be converted.
/// @param[in]  flags    Specifies processing options controlling the output.
/// @param[in]  options  Extended processing options as Name=Value pairs
///                      joined by semicolons, or NULL for the defaults.
/// @param[in]  output   Filename where output document should be written.
/// @param[out] error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p filename is a null pointer.
/// @retval IGR_E_ERROR           The source document could not be opened, or
///                               an unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Convert_File(const IGR_UCS2* filename, IGR_OPEN_FLAGS_TYPE flags, const IGR_UCS2* options,
                                            const IGR_UCS2* output, Error_Control_Block* error);

/// @brief Creates a Perceptive stream from a file for use with document
///        stream functions.
///
/// @param[in]  filename Path to the document to be opened.
/// @param[in]  flags    Bitfield of options that affect stream behavior.
/// @param[out] stream   System allocated stream structure that caller must
///                      free by calling stream->Close().
/// @param[out] error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_OUT_OF_MEMORY          There is insufficient memory to
///                                      complete the operation.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR The file could not be opened, or an
///                                      unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_File(const IGR_UCS2* filename, IGR_LONG flags, struct IGR_Stream** stream,
                                                     Error_Control_Block* error);

/// @brief Creates a Perceptive stream from a memory buffer.
///
/// @param[in]  buffer     Pointer to user allocated memory buffer containing
///                        document.
/// @param[in]  bufferSize Size of the buffer in bytes.
/// @param[in]  destruct   Optional function to call when stream is closed to
///                        free memory. Must have signature: void __cdecl
///                        Destruct(void *data).
/// @param[out] stream     System allocated stream structure that caller must
///                        free.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_OUT_OF_MEMORY          There is insufficient memory to
///                                      complete the operation.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_Memory(void* buffer, size_t bufferSize, void* destruct, struct IGR_Stream** stream,
                                                       Error_Control_Block* error);

/// @brief Callback function invoked when reading from a stream.
/// @param[in] context User context passed to callback functions.
/// @param[in] offset  Offset to seek to.
/// @param[in] whence  Seek origin (0 = beginning, 1 = current, 2 = end).
/// @return IGR_LONGLONG The new position in the stream after seeking, or -1
///         on error.
/// @ingroup callback_functions
typedef IGR_LONGLONG(CALLBACK* IGR_Make_Stream_Seek_Function)(void* context, IGR_LONGLONG offset, IGR_ULONG whence);

/// @brief Callback function invoked when reading from a stream.
/// @param[in] context     User context passed to callback functions.
/// @param[in] buffer      Buffer to read data into.
/// @param[in] buffer_size Size of the buffer in bytes.
/// @return IGR_ULONG The number of bytes read, or 0 on end of stream, or -1
///         on error.
/// @ingroup callback_functions
typedef IGR_ULONG(CALLBACK* IGR_Make_Stream_Read_Function)(void* context, void* buffer, IGR_ULONG buffer_size);

/// @brief Callback function invoked when writing to a stream.
/// @param[in] context     User context passed to callback functions.
/// @param[in] buffer      Buffer containing data to write.
/// @param[in] buffer_size Size of the buffer in bytes.
/// @return IGR_ULONG The number of bytes written, or -1 on error
/// @ingroup callback_functions
typedef IGR_ULONG(CALLBACK* IGR_Make_Stream_Write_Function)(void* context, const void* buffer, IGR_ULONG buffer_size);

/// @brief Callback function invoked for stream actions.
/// @param[in] context     User context passed to callback functions.
/// @param[in] actionId    Action identifier.
/// @param[in] action_data Pointer to action-specific data.
/// @return IGR_LONG 0 on success, or an error code on failure.
/// @ingroup callback_functions
typedef IGR_LONG(CALLBACK* IGR_Make_Stream_Action_Function)(void* context, int actionId, void* action_data);

/// @brief Callback function invoked to destruct the stream context.
/// @param[in] context User context passed to callback functions.
/// @ingroup callback_functions
typedef void(CALLBACK* IGR_Make_Stream_Destruct_Function)(void* context);

/// @brief Creates an IGR stream from callback functions, useful for languages
///        without precise memory control.
///
/// @param[in]  context  User context passed to callback functions.
/// @param[in]  flags    Bitfield of options affecting stream behavior.
/// @param[in]  seek     Required Seek function pointer.
/// @param[in]  read     Required Read function pointer.
/// @param[in]  write    Optional Write function pointer.
/// @param[in]  action   Optional action handler function pointer.
/// @param[in]  destruct Required destructor to free context memory.
/// @param[out] stream   System allocated stream that caller must free.
/// @param[out] error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p context is a null pointer, @p seek is a
///                               null pointer, @p read is a null pointer, or
///                               @p stream is a null pointer.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_Functions(void* context, IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_TYPE flags,
                                                          IGR_Make_Stream_Seek_Function seek, IGR_Make_Stream_Read_Function read,
                                                          IGR_Make_Stream_Write_Function write, IGR_Make_Stream_Action_Function action,
                                                          IGR_Make_Stream_Destruct_Function destruct, struct IGR_Stream** stream,
                                                          Error_Control_Block* error);

/// @brief Creates a custom stream that accepts callbacks.
///
/// @param[in]  pStream     Valid \ref IGR_Stream instance.
/// @param[in]  cb          Callback function to handle stream events.
/// @param[in]  context     User-supplied context passed to callback.
/// @param[out] ppExtStream Extended stream to use instead of original.
/// @param[out] error       Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK      Operation succeeded.
/// @retval IGR_E_ERROR One or more input parameters are invalid.
IGR_RETURN_CODE IGR_EXPORT IGR_Extend_Stream(struct IGR_Stream* pStream, IGR_CALLBACK cb, void* context, struct IGR_Stream** ppExtStream,
                                             Error_Control_Block* error);

/// @brief Windows-only memory diagnostic function.
///
/// @param[out] buffer Buffer to receive diagnostic info.
/// @param[in]  len    Length of buffer.
///
/// @return Size of diagnostic data written.
size_t IGR_EXPORT IGR_Get_Memory_Status(char* buffer, size_t len);

/// @brief Enumerates images from a document opened with HTML conversion
///        enabled.
///
/// @param[in]  handle Handle to opened document.
/// @param[out] id     Buffer (8192 bytes) to receive image ID (4096 Unicode
///                    chars).
/// @param[out] name   Buffer (2048 bytes) to receive image name (1024 Unicode
///                    chars).
/// @param[out] date   Receives image date/time in FILETIME format.
/// @param[out] size   Receives image size in bytes.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE         @p handle does not resolve to an open
///                                      document.
/// @retval IGR_E_INVALID_PARAMETER      @p handle is zero.
/// @retval IGR_E_INVALID_POINTER        @p id is a null pointer, or @p name
///                                      is a null pointer.
/// @retval IGR_NO_MORE                  There are no more images to
///                                      enumerate, or the document does not
///                                      support image enumeration.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Image_Entry(IGR_LONG handle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date, IGR_LONGLONG* size,
                                               Error_Control_Block* error);

/// @brief Creates enumerator for images in document opened with HTML
///        conversion.
///
/// @param[in]  docHandle  Handle to opened document.
/// @param[out] enumerator Handle to new image enumerator.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                     Operation succeeded, including when the
///                                    document does not support image
///                                    enumeration (an empty enumerator is
///                                    returned).
/// @retval IGR_E_INVALID_HANDLE       @p docHandle is not a valid document
///                                    handle.
/// @retval IGR_E_INVALID_POINTER      @p enumerator is a null pointer.
/// @retval IGR_E_ACTION_NOT_SUPPORTED An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Images_Enumerator(IGR_LONG docHandle, HSUBFILES* enumerator, Error_Control_Block* error);

/// @brief Extracts an image to disk from document opened with HTML
///        conversion.
///
/// @param[in]  handle      Handle to opened document.
/// @param[in]  id          Image ID from IGR_Get_Image_Entry.
/// @param[in]  destination Path where image will be written.
/// @param[out] error       Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE           @p handle is not a valid document
///                                        handle.
/// @retval IGR_E_INVALID_POINTER          @p id is a null pointer, or @p
///                                        destination is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER        @p id does not correspond to an
///                                        existing image.
/// @retval IGR_E_NOT_FOUND                @p id does not match an image in
///                                        this document.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS The document does not support image
///                                        extraction.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR   An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Image(IGR_LONG handle, const IGR_UCS2* id, const IGR_UCS2* destination, Error_Control_Block* error);

/// @brief Extracts an image to a stream from document opened with HTML
///        conversion.
///
/// @param[in]  handle Handle to opened document.
/// @param[in]  id     Image ID from IGR_Get_Image_Entry.
/// @param[out] stream System allocated stream that caller must free.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE         @p handle is not a valid document
///                                      handle.
/// @retval IGR_E_INVALID_PARAMETER      @p id does not correspond to an
///                                      existing image.
/// @retval IGR_E_NOT_FOUND              @p id does not match an image in this
///                                      document.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Image_Stream(IGR_LONG handle, const IGR_UCS2* id, struct IGR_Stream** stream,
                                                    Error_Control_Block* error);

/// @brief Calculates the MD5 hash of an input stream for unique document
///        identification.
///
/// @param[in]  stream An open \ref IGR_Stream.
/// @param[in]  dest   Buffer to receive the null-terminated MD5 hash (as a
///                    Unicode string). Must be able to hold at least 33 UCS2
///                    characters.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p stream is a null pointer, or @p dest is a
///                               null pointer.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Calculate_MD5(struct IGR_Stream* stream, IGR_UCS2* dest, Error_Control_Block* error);

/// @brief Calculates the SHA1 hash of an input stream for unique document
///        identification.
///
/// @param[in]  stream An open \ref IGR_Stream.
/// @param[in]  dest   Buffer to receive the null-terminated SHA1 hash (as a
///                    Unicode string). Must be able to hold at least 41 UCS2
///                    characters.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p stream is a null pointer, or @p dest is a
///                               null pointer.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Calculate_SHA1(struct IGR_Stream* stream, IGR_UCS2* dest, Error_Control_Block* error);

/// @brief Returns the number of pages for a document opened in HD mode.
///
/// @param[in]  docHandle Handle to the document.
/// @param[out] pageCount Number of pages in the document.
/// @param[out] error     Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE         @p docHandle does not resolve to a
///                                      document opened in HD mode.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Count(IGR_LONG docHandle, IGR_LONG* pageCount, Error_Control_Block* error);

/// @brief Returns a handle to the specified page.
///
/// @param[in]  docHandle  Handle to a HD document.
/// @param[in]  pageIndex  0-based index of the page being requested.
/// @param[out] pageHandle Handle to the requested page.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE    @p docHandle does not resolve to a
///                                 document opened in HD mode.
/// @retval IGR_E_INVALID_PARAMETER @p pageHandle is a null pointer, or @p
///                                 pageIndex is out of range.
/// @retval IGR_E_TOO_MANY_HANDLES  Too many pages are already open, so a new
///                                 page handle could not be allocated.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_Page(IGR_LONG docHandle, IGR_LONG pageIndex, IGR_HPAGE* pageHandle, Error_Control_Block* error);

/// @brief Closes the specified page handle and frees associated resources.
///
/// @param[in]  pageHandle Handle to the page to close.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK               Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE @p pageHandle is not a valid page handle.
/// @retval IGR_E_BAD_ERROR      An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Close_Page(IGR_HPAGE pageHandle, Error_Control_Block* error);

/// @brief Gets the total number of words on a page.
///
/// @param[in]  pageHandle Handle to the page.
/// @param[out] wordCount  Receives the number of words on the page.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK               Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE @p pageHandle is not a valid page handle.
/// @retval IGR_E_BAD_ERROR      An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Word_Count(IGR_HPAGE pageHandle, IGR_LONG* wordCount, Error_Control_Block* error);

/// @brief Gets positional information for a range of words on a page.
///
/// @param[in]     pageHandle Handle to the page.
/// @param[in]     index      Zero-based index of first word to retrieve.
/// @param[in,out] count      On input: size of words array; On output: number
///                           of words copied.
/// @param[in,out] words      Array of \ref IGR_Page_Word structs to receive
///                           word data.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE    @p pageHandle is not a valid page handle.
/// @retval IGR_E_INVALID_PARAMETER @p count is a null pointer, or @p index is
///                                 negative.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Words(IGR_HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Page_Word* words,
                                              Error_Control_Block* error);

/// @brief Gets the dimensions of a page in pixels.
///
/// @param[in]  pageHandle Handle to the page.
/// @param[out] width      Receives the width of the page in pixels.
/// @param[out] height     Receives the height of the page in pixels.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK               Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE @p pageHandle is not a valid page handle.
/// @retval IGR_E_BAD_ERROR      An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Dimensions(IGR_HPAGE pageHandle, IGR_LONG* width, IGR_LONG* height, Error_Control_Block* error);

/// @brief Returns the next piece of page text.
///
/// @param[in]     pageHandle Handle to the page.
/// @param[out]    buffer     Buffer to copy text into, up to size characters.
/// @param[in,out] size       On input: number of characters buffer can hold.
///                           On output: number of characters written to
///                           buffer.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE    @p pageHandle is not a valid page handle.
/// @retval IGR_E_INVALID_PARAMETER @p size is a null pointer, or @p buffer is
///                                 a null pointer.
/// @retval IGR_NO_MORE             There is no more text on the page.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Text(IGR_HPAGE pageHandle, IGR_UCS2* buffer, IGR_LONG* size, Error_Control_Block* error);

/// @brief Gets information about the next image in a page.
///
/// @param[in]  pageHandle Handle to the page.
/// @param[out] id         Buffer to receive image ID (min 4096 UCS2 chars).
/// @param[out] name       Buffer to receive image name (min 1024 UCS2 chars).
/// @param[out] date       Receives image date/time in FILETIME format.
/// @param[out] size       Receives image size in bytes.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p pageHandle is not a valid page handle,
///                                 @p id is a null pointer, or @p name is a
///                                 null pointer.
/// @retval IGR_NO_MORE             There are no more images on the page.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Image_Entry(IGR_HPAGE pageHandle, IGR_UCS2* id, IGR_UCS2* name, IGR_LONGLONG* date,
                                                    IGR_LONGLONG* size, Error_Control_Block* error);

/// @brief Creates an enumerator for iterating through page images.
///
/// @param[in]  pageHandle Handle to the page.
/// @param[out] enumerator Handle to new image enumerator.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK      Operation succeeded.
/// @retval IGR_E_ERROR An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Images_Enumerator(IGR_HPAGE pageHandle, HSUBFILES* enumerator, Error_Control_Block* error);

/// @brief Extracts a page image to disk.
///
/// @param[in]  pageHandle  Handle to the page.
/// @param[in]  id          ID of image to extract from
///                         IGR_Get_Page_Image_Entry.
/// @param[in]  destination Path where image will be written.
/// @param[out] error       Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p pageHandle is not a valid page handle,
///                                 @p id is a null pointer, or @p destination
///                                 is a null pointer.
/// @retval IGR_E_ERROR             @p id does not match an image on the page.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Page_Image(IGR_HPAGE pageHandle, const IGR_UCS2* id, const IGR_UCS2* destination,
                                                  Error_Control_Block* error);

/// @brief Extracts a page image to a stream.
///
/// @param[in]  pageHandle Handle to the page.
/// @param[in]  id         ID of image to extract from
///                        IGR_Get_Page_Image_Entry.
/// @param[out] stream     Stream to write extracted image content to.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p pageHandle is not a valid page handle,
///                                 @p id is a null pointer, or @p stream is a
///                                 null pointer.
/// @retval IGR_E_ERROR             @p id does not match an image on the page.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Page_Image_Stream(IGR_HPAGE pageHandle, const IGR_UCS2* id, struct IGR_Stream** stream,
                                                         Error_Control_Block* error);

/// @brief Gets the count of form elements on a page.
///
/// @param[in]  pageHandle Handle to the page.
/// @param[out] count      Receives number of form elements.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p pageHandle is not a valid page handle,
///                                 or @p count is a null pointer.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Count(IGR_HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);

/// @brief Gets form elements from a page.
///
/// @param[in]     pageHandle Handle to the page.
/// @param[in]     index      Zero-based index of first element to retrieve.
/// @param[in,out] count      On input: size of items array. On output: number
///                           of elements copied.
/// @param[out]    items      Array to receive form element data.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p pageHandle is not a valid page handle,
///                                 @p count is a null pointer, or @p index is
///                                 out of range.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Elements(IGR_HPAGE pageHandle, IGR_LONG index, IGR_LONG* count,
                                                      struct IGR_Page_Form_Element* items, Error_Control_Block* error);

/// @brief Gets a string value from a form element.
///
/// @param[in]  item       Form element from IGR_Get_Page_Form_Elements.
/// @param[in]  value_type Type of string value to retrieve.
/// @param[in]  char_count Size of buffer in characters.
/// @param[out] buffer     Buffer to receive string value.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p item is a null pointer, @p buffer is a
///                                 null pointer, @p char_count is too small,
///                                 or @p value_type is not recognized.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Str(const struct IGR_Page_Form_Element* item,
                                                         IGR_PAGE_FORM_ELEMENT_GET_TYPE value_type, IGR_LONG char_count, IGR_UCS2* buffer,
                                                         Error_Control_Block* error);

/// @brief Gets a string value for a form element option.
///
/// @param[in]  item         Form element from IGR_Get_Page_Form_Elements.
/// @param[in]  value_type   Type of string value to retrieve.
/// @param[in]  option_index Zero-based index of option.
/// @param[in]  char_count   Size of buffer in characters.
/// @param[out] buffer       Buffer to receive string value.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p item is a null pointer, @p buffer is a
///                                 null pointer, @p char_count is too small,
///                                 @p option_index is out of range, or @p
///                                 value_type is not recognized.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Form_Element_Option_Str(const struct IGR_Page_Form_Element* item,
                                                                IGR_PAGE_FORM_ELEMENT_GET_TYPE value_type, IGR_LONG option_index,
                                                                IGR_LONG char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Enumerates all bookmarks up to a specified depth using a callback
///        function.
///
/// @param[in]  docHandle     Handle to a HD document.
/// @param[in]  root          Optional bookmark value to enumerate from, or
///                           NULL to enumerate from root.
/// @param[in]  recurse_depth Maximum depth to scan.
/// @param[in]  callback      Pointer to application-defined callback
///                           function.
/// @param[in]  context       Application-defined value to be passed to
///                           callback function.
/// @param[out] error         Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p docHandle does not resolve to an open
///                               document.
/// @retval IGR_E_INVALID_POINTER @p docHandle is a null pointer, or @p
///                               callback is a null pointer.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Enum_Bookmarks(IGR_LONG docHandle, const struct IGR_Bookmark* root, IGR_LONG recurse_depth,
                                              IGR_CALLBACK_BOOKMARK callback, void* context, Error_Control_Block* error);

/// @brief Retrieves the root bookmark item for the document.
///
/// @param[in]  docHandle Handle to a HD document.
/// @param[in]  dest      Pointer to application provided \ref IGR_Bookmark
///                       that will be populated with bookmark data.
/// @param[out] error     Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p docHandle does not resolve to an open
///                               document.
/// @retval IGR_E_INVALID_POINTER @p dest is a null pointer, or @p error is a
///                               null pointer.
/// @retval IGR_NO_MORE           The document has no root bookmark.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_Root(IGR_LONG docHandle, struct IGR_Bookmark* dest, Error_Control_Block* error);

/// @brief Retrieves the first child of the provided bookmark root.
///
/// @param[in]  docHandle Handle to a HD document.
/// @param[in]  root      Item to extract first child from, or NULL to get
///                       first bookmark of document.
/// @param[in]  dest      Pointer to application provided \ref IGR_Bookmark
///                       that will be populated with bookmark data.
/// @param[out] error     Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p docHandle does not resolve to an open
///                               document.
/// @retval IGR_E_INVALID_POINTER @p dest is a null pointer, or @p error is a
///                               null pointer.
/// @retval IGR_NO_MORE           @p root has no children.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_First_Child(IGR_LONG docHandle, const struct IGR_Bookmark* root, struct IGR_Bookmark* dest,
                                                        Error_Control_Block* error);

/// @brief Retrieves the next sibling of the provided bookmark root.
///
/// @param[in]  docHandle Handle to a HD document.
/// @param[in]  root      Item to extract next sibling from.
/// @param[in]  dest      Pointer to application provided \ref IGR_Bookmark
///                       that will be populated with bookmark data.
/// @param[out] error     Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p docHandle does not resolve to an open
///                               document.
/// @retval IGR_E_INVALID_POINTER @p dest is a null pointer, or @p error is a
///                               null pointer.
/// @retval IGR_NO_MORE           @p root has no next sibling.
/// @retval IGR_E_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Bookmark_Next_Sibling(IGR_LONG docHandle, const struct IGR_Bookmark* root, struct IGR_Bookmark* dest,
                                                         Error_Control_Block* error);

/// @brief Returns a new canvas for rendering paginated output.
///
/// @param[in]  type         Device type (IGR_DEVICE_IMAGE_PNG/JPG/PDF/TIF).
/// @param[in]  filename     Filename to save output to.
/// @param[in]  options      Extended rendering options as Name=Value pairs
///                          joined by semicolons, or NULL for the defaults.
/// @param[out] canvasHandle Handle to newly created output canvas, must be
///                          closed with IGR_Close_Canvas.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_POINTER   @p filename is a null pointer, or @p
///                                 canvasHandle is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER @p type is not a recognized device type.
/// @retval IGR_E_TOO_MANY_HANDLES  Too many canvases are already open, so a
///                                 new canvas handle could not be allocated.
/// @retval IGR_E_BAD_ERROR         The output renderer could not be created,
///                                 or an unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Output_Canvas(IGR_DEVICE_TYPE type, const IGR_UCS2* filename, const IGR_UCS2* options,
                                                  IGR_HCANVAS* canvasHandle, Error_Control_Block* error);

/// @brief Returns a new canvas for rendering paginated output to a stream.
///
/// @param[in]  type         Device type (IGR_DEVICE_IMAGE_PNG/JPG/PDF/TIF).
/// @param[in]  stream       Stream to write output to.
/// @param[in]  options      Extended rendering options as Name=Value pairs
///                          joined by semicolons, or NULL for the defaults.
/// @param[out] canvasHandle Handle to newly created output canvas, must be
///                          closed with IGR_Close_Canvas.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_POINTER   @p stream is a null pointer, or @p
///                                 canvasHandle is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER @p type is not a recognized device type,
///                                 or @p stream is not a valid stream object.
/// @retval IGR_E_TOO_MANY_HANDLES  Too many canvases are already open, so a
///                                 new canvas handle could not be allocated.
/// @retval IGR_E_BAD_ERROR         The output renderer could not be created,
///                                 or an unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Make_Output_Canvas_On(IGR_DEVICE_TYPE type, struct IGR_Writable_Stream* stream, const IGR_UCS2* options,
                                                     IGR_HCANVAS* canvasHandle, Error_Control_Block* error);

/// @brief Closes the given canvas handle and frees resources.
///
/// @param[in]  canvasHandle Handle to canvas to close.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE         @p canvasHandle is not a valid canvas
///                                      handle.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Close_Canvas(IGR_HCANVAS canvasHandle, Error_Control_Block* error);

/// @brief Renders a page onto a canvas, creating a new page for multi-page
///        canvases.
///
/// @param[in]  page   Handle to a page obtained from IGR_Open_Page.
/// @param[in]  canvas Handle to a canvas obtained from
///                    IGR_Make_Output_Canvas.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p page or canvas does not resolve to
///                                      a valid page or canvas.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Render_Page(IGR_HPAGE page, IGR_HCANVAS canvas, Error_Control_Block* error);

/// @brief Renders a page onto a canvas with extended options and properties.
///
/// @param[in]  page       Handle to a page obtained from IGR_Open_Page.
/// @param[in]  canvas     Handle to a canvas obtained from
///                        IGR_Make_Output_Canvas.
/// @param[in]  options    Extended rendering options as Name=Value pairs
///                        joined by semicolons, or NULL for the defaults.
/// @param[in]  properties Properties controlling per page rendering.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p page does not resolve to a valid page,
///                                 or @p canvas does not resolve to a valid
///                                 canvas.
/// @retval IGR_E_ERROR             The page could not be prepared for
///                                 rendering with the "always rasterize"
///                                 redaction option.
/// @retval IGR_E_BAD_ERROR         The output renderer could not be created,
///                                 or an unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Render_Page_Ex(IGR_HPAGE page, IGR_HCANVAS canvas, const IGR_UCS2* options,
                                              const struct IGR_Render_Page_Properties* properties, Error_Control_Block* error);

/// @brief Redacts/removes words in the specified range from the document.
///
/// @param[in]  page  Handle to a page obtained from IGR_Open_Page.
/// @param[in]  From  Index of first word to redact.
/// @param[in]  To    Index of last word to redact.
/// @param[in]  Type  Reserved, must be zero.
/// @param[out] error Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p page does not resolve to a valid
///                                      page, @p From or @p To is out of
///                                      range, or @p Type is not zero.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Redact_Page_Text(IGR_HPAGE page, IGR_LONG From, IGR_LONG To, IGR_LONG Type, Error_Control_Block* error);

/// @brief Gets page style/attribute information.
///
/// @param[in]     page   Handle to a page obtained from IGR_Open_Page.
/// @param[in]     name   Name of the style/attribute to retrieve.
/// @param[out]    buffer Buffer to receive the attribute/style value.
/// @param[in,out] size   On input: size of buffer in characters; On output:
///                       size of the returned string.
/// @param[out]    error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE         @p page is not a valid page handle.
/// @retval IGR_E_INVALID_POINTER        @p name is a null pointer, or @p size
///                                      is a null pointer.
/// @retval IGR_NO_MORE                  The page has no sections.
/// @retval IGR_E_WRITE_ERROR            The attribute value is too large to
///                                      return.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Attribute(IGR_HPAGE page, const IGR_UCS2* name, IGR_UCS2* buffer, IGR_LONG* size,
                                                  Error_Control_Block* error);

/// @brief Gets information about a supported document format.
///
/// @param[in]  formatid Format ID returned by IGR_Get_File_Type.
/// @param[in]  what     Attribute to retrieve. See @ref
///                      get_format_attribute_types for available values.
/// @param[out] buffer   Buffer to receive text (min 255 bytes).
/// @param[out] error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p buffer is a null pointer.
/// @retval IGR_E_ERROR           @p formatid does not match a known format,
///                               or @p what is not recognized.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Format_Attribute(IGR_LONG formatid, IGR_GET_FORMAT_ATTRIBUTE_TYPE what, char* buffer,
                                                    Error_Control_Block* error);

/// @brief Gets option attribute information for a format.
///
/// @param[in]  formatid   Format ID returned by IGR_Get_File_Type.
/// @param[in]  what       Attribute to retrieve. See @ref
///                        get_option_attribute_types for available values.
/// @param[out] buffer     Buffer to receive the attribute information.
/// @param[in]  bufferSize Size of buffer in bytes.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p buffer is a null pointer.
/// @retval IGR_E_ERROR           @p formatid is out of range, or @p what is
///                               not recognized.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Option_Attribute(IGR_LONG formatid, IGR_GET_OPTION_ATTRIBUTE_TYPE what, char* buffer, size_t bufferSize,
                                                    Error_Control_Block* error);

/// @brief Draws an arc on the canvas using two sets of x,y coordinates.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate of starting point.
/// @param[in]  y      Y coordinate of starting point.
/// @param[in]  x2     X coordinate of ending point.
/// @param[in]  y2     Y coordinate of ending point.
/// @param[in]  x3     X coordinate of first arc point.
/// @param[in]  y3     Y coordinate of first arc point.
/// @param[in]  x4     X coordinate of second arc point.
/// @param[in]  y4     Y coordinate of second arc point.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Arc(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3,
                                          IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);

/// @brief Draws an arc on the canvas using center point, radius and angles.
///
/// @param[in]  canvas     Canvas handle to draw on.
/// @param[in]  x          X coordinate of center point.
/// @param[in]  y          Y coordinate of center point.
/// @param[in]  radius     Radius of the arc in pixels.
/// @param[in]  startAngle Starting angle in degrees.
/// @param[in]  sweepAngle Sweep angle in degrees.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_AngleArc(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG radius, IGR_LONG startAngle,
                                               IGR_LONG sweepAngle, Error_Control_Block* error);

/// @brief Draws a chord on the canvas using two sets of x,y coordinates.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate of starting point.
/// @param[in]  y      Y coordinate of starting point.
/// @param[in]  x2     X coordinate of ending point.
/// @param[in]  y2     Y coordinate of ending point.
/// @param[in]  x3     X coordinate of first chord point.
/// @param[in]  y3     Y coordinate of first chord point.
/// @param[in]  x4     X coordinate of second chord point.
/// @param[in]  y4     Y coordinate of second chord point.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Chord(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3,
                                            IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);

/// @brief Draws an ellipse on the canvas using bounding box coordinates.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate of top-left corner.
/// @param[in]  y      Y coordinate of top-left corner.
/// @param[in]  x2     X coordinate of bottom-right corner.
/// @param[in]  y2     Y coordinate of bottom-right corner.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Ellipse(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2,
                                              Error_Control_Block* error);

/// @brief Draws a rectangle on the canvas using corner coordinates.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate of top-left corner.
/// @param[in]  y      Y coordinate of top-left corner.
/// @param[in]  x2     X coordinate of bottom-right corner.
/// @param[in]  y2     Y coordinate of bottom-right corner.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Rect(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2,
                                           Error_Control_Block* error);

/// @brief Draws a line from current position to specified coordinates.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate of line endpoint.
/// @param[in]  y      Y coordinate of line endpoint.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_LineTo(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, Error_Control_Block* error);

/// @brief Moves current drawing position without drawing.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate to move to.
/// @param[in]  y      Y coordinate to move to.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_MoveTo(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, Error_Control_Block* error);

/// @brief Draws a pie shape on the canvas.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate of bounding rectangle top-left.
/// @param[in]  y      Y coordinate of bounding rectangle top-left.
/// @param[in]  x2     X coordinate of bounding rectangle bottom-right.
/// @param[in]  y2     Y coordinate of bounding rectangle bottom-right.
/// @param[in]  x3     X coordinate of start radial line.
/// @param[in]  y3     Y coordinate of start radial line.
/// @param[in]  x4     X coordinate of end radial line.
/// @param[in]  y4     Y coordinate of end radial line.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Pie(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG x3, IGR_LONG y3,
                                          IGR_LONG x4, IGR_LONG y4, Error_Control_Block* error);

/// @brief Draws a rounded rectangle on the canvas.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate of top-left corner.
/// @param[in]  y      Y coordinate of top-left corner.
/// @param[in]  x2     X coordinate of bottom-right corner.
/// @param[in]  y2     Y coordinate of bottom-right corner.
/// @param[in]  radius Radius of corner rounding in pixels.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_RoundRect(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, IGR_LONG radius,
                                                Error_Control_Block* error);

/// @brief Outputs text at specified coordinates.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate for text position.
/// @param[in]  y      Y coordinate for text position.
/// @param[in]  text   UCS2 text string to output.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_TextOut(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, const IGR_UCS2* text, Error_Control_Block* error);

/// @brief Outputs text within a bounding rectangle.
///
/// @param[in]  canvas Canvas handle to draw on.
/// @param[in]  x      X coordinate of bounding rectangle top-left.
/// @param[in]  y      Y coordinate of bounding rectangle top-left.
/// @param[in]  x2     X coordinate of bounding rectangle bottom-right.
/// @param[in]  y2     Y coordinate of bounding rectangle bottom-right.
/// @param[in]  text   UCS2 text string to output.
/// @param[in]  flags  Text rectangle formatting flags.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_TextRect(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, const IGR_UCS2* text,
                                               IGR_TEXTRECT_TYPE flags, Error_Control_Block* error);

/// @brief Measures dimensions needed to draw text string.
///
/// @param[in]     canvas Canvas handle to measure on.
/// @param[in]     text   UCS2 text string to measure.
/// @param[in,out] width  Returns width needed in pixels.
/// @param[in,out] height Returns height needed in pixels.
/// @param[out]    error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_MeasureText(IGR_HCANVAS canvas, const IGR_UCS2* text, IGR_LONG* width, IGR_LONG* height,
                                                  Error_Control_Block* error);

/// @brief Sets the pen for drawing outlines.
///
/// @param[in]  canvas Canvas handle to set pen for.
/// @param[in]  color  RGB color value for pen.
/// @param[in]  width  Width of pen in pixels.
/// @param[in]  style  Style of pen (solid, etc).
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetPen(IGR_HCANVAS canvas, IGR_LONG color, IGR_LONG width, IGR_PEN_TYPE style,
                                             Error_Control_Block* error);

/// @brief Sets the brush for filling shapes.
///
/// @param[in]  canvas Canvas handle to set brush for.
/// @param[in]  color  RGB color value for brush.
/// @param[in]  style  Style of brush (solid, etc).
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_ERROR                    The drawing engine has not been
///                                        initialized.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetBrush(IGR_HCANVAS canvas, IGR_LONG color, IGR_BRUSH_TYPE style, Error_Control_Block* error);

/// @brief Sets the font for text operations.
///
/// @param[in]  canvas     Canvas handle to set font for.
/// @param[in]  fontFamily Name of font family.
/// @param[in]  size       Size of font in points.
/// @param[in]  style      Font style flags (bold, italic, etc).
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_ERROR                    The drawing engine has not been
///                                        initialized.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetFont(IGR_HCANVAS canvas, const IGR_UCS2* fontFamily, IGR_LONG size, IGR_TEXT_STYLE_TYPE style,
                                              Error_Control_Block* error);

/// @brief Sets global opacity level for drawing operations.
///
/// @param[in]  canvas  Canvas handle to set opacity for.
/// @param[in]  opacity Opacity level (0-255).
/// @param[out] error   Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_ERROR                    The drawing engine has not been
///                                        initialized.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetOpacity(IGR_HCANVAS canvas, IGR_BYTE opacity, Error_Control_Block* error);

/// @brief Draws an image at specified coordinates.
///
/// @param[in]  canvas    Canvas handle to draw on.
/// @param[in]  x         X coordinate to draw at.
/// @param[in]  y         Y coordinate to draw at.
/// @param[in]  imagedata Raw image data bytes.
/// @param[in]  imagesize Size of image data in bytes.
/// @param[in]  mimetype  MIME type of image data.
/// @param[out] error     Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle, or @p imagedata is a null
///                                        pointer.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_NOT_READABLE             @p imagedata contains JSON
///                                        annotation data of an unsupported
///                                        or unrecognized type.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_DrawImage(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, void* imagedata, size_t imagesize,
                                                const IGR_UCS2* mimetype, Error_Control_Block* error);

/// @brief Draws a scaled image within specified coordinates.
///
/// @param[in]  canvas    Canvas handle to draw on.
/// @param[in]  x         X coordinate of top-left corner.
/// @param[in]  y         Y coordinate of top-left corner.
/// @param[in]  x2        X coordinate of bottom-right corner.
/// @param[in]  y2        Y coordinate of bottom-right corner.
/// @param[in]  imagedata Raw image data bytes.
/// @param[in]  imagesize Size of image data in bytes.
/// @param[in]  mimetype  MIME type of image data.
/// @param[out] error     Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle, or @p imagedata is a null
///                                        pointer.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_NOT_READABLE             @p imagedata contains JSON
///                                        annotation data of an unsupported
///                                        or unrecognized type.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_DrawScaleImage(IGR_HCANVAS canvas, IGR_LONG x, IGR_LONG y, IGR_LONG x2, IGR_LONG y2, void* imagedata,
                                                     size_t imagesize, const IGR_UCS2* mimetype, Error_Control_Block* error);

/// @brief Sets rotation angle for subsequent drawing operations.
///
/// @param[in]  canvas  Canvas handle to set rotation for.
/// @param[in]  degrees Rotation angle in degrees.
/// @param[out] error   Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_ERROR                    The drawing engine has not been
///                                        initialized.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Rotation(IGR_HCANVAS canvas, IGR_LONG degrees, Error_Control_Block* error);

/// @brief Resets all canvas drawing settings to defaults.
///
/// @param[in]  canvas Canvas handle to reset.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p canvas is not a valid canvas handle.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Reset(IGR_HCANVAS canvas, Error_Control_Block* error);

/// @brief Adds annotations to canvas from JSON data.
///
/// @param[in]  canvas Canvas handle to add annotations to.
/// @param[in]  json   JSON string containing annotation data.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER        @p canvas is not a valid canvas
///                                        handle.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p canvas does not support drawing
///                                        operations.
/// @retval IGR_E_NOT_READABLE             @p json contains annotation data of
///                                        an unsupported or unrecognized
///                                        type.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Annotate_JSON(IGR_HCANVAS canvas, const IGR_UCS2* json, Error_Control_Block* error);

/// @brief Gets count of hyperlinks on a page.
///
/// @param[in]  pageHandle Handle to the page.
/// @param[out] count      Returns number of hyperlinks.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p pageHandle is not a valid page handle,
///                                 @p count is a null pointer, or @p error is
///                                 a null pointer.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlink_Count(IGR_HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);

/// @brief Gets hyperlink objects from a page.
///
/// @param[in]     pageHandle Handle to the page.
/// @param[in]     index      Index of first hyperlink to get.
/// @param[in,out] count      On input: size of items array; On output: number
///                           of hyperlinks copied.
/// @param[out]    items      Array to receive hyperlink data.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p pageHandle is not a valid page handle,
///                                 @p count is a null pointer, or @p index is
///                                 out of range.
/// @retval IGR_E_INVALID_HANDLE    @p pageHandle resolves to a page with no
///                                 associated document.
/// @retval IGR_NO_MORE             No hyperlink data could be resolved for
///                                 the page.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlinks(IGR_HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Hyperlink* items,
                                                   Error_Control_Block* error);

/// @brief Gets a string value from a hyperlink.
///
/// @param[in]  item       Hyperlink object from IGR_Get_Page_Hyperlinks.
/// @param[in]  value_type Type of string value to retrieve.
/// @param[in]  char_count Size of buffer in characters.
/// @param[out] buffer     Buffer to receive string value.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p item is not a valid hyperlink object,
///                                 @p buffer is a null pointer, or @p
///                                 char_count is too small.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Hyperlink_Str(const struct IGR_Hyperlink* item, IGR_HYPERLINK_GET_VALUE_TYPE value_type,
                                                      IGR_LONG char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Inserts a blank page into the canvas.
///
/// @param[in]  canvas   Handle to canvas created by IGR_Make_Output_Canvas.
/// @param[in]  options  Extended processing options as Name=Value pairs
///                      joined by semicolons, or NULL for the defaults.
/// @param[in]  width    Width of the new page in pixels.
/// @param[in]  height   Height of the new page in pixels.
/// @param[in]  reserved Reserved for future use.
/// @param[out] error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p canvas is not a valid canvas handle, or
///                                 @p width or @p height is less than or
///                                 equal to zero.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Blank_Page(IGR_HCANVAS canvas, const IGR_UCS2* options, IGR_LONG width, IGR_LONG height,
                                                 const void* reserved, Error_Control_Block* error);

/// @brief Opens a document from an OS based, device-dependent bitmap
///        (Windows-Only).
///
/// @param[in]  bitmap       Handle to the OS bitmap (HBITMAP).
/// @param[in]  palette      Handle to optional OS palette (HPALETTE).
/// @param[in]  flags        Specifies processing options for IGR_Get_Text
///                          output.
/// @param[in]  options      Extended processing options as Name=Value pairs
///                          joined by semicolons, or NULL for the defaults.
/// @param[out] capabilities Returns Document Capabilities bitfield.
/// @param[out] file_type    Returns Document Format Code.
/// @param[out] handle       Returns handle for subsequent calls.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                 Operation succeeded.
/// @retval IGR_E_TOO_MANY_HANDLES Too many documents are already open.
/// @retval IGR_E_OPEN_ERROR       This function does not currently support
///                                opening a DDB.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_DDB(IGR_HBITMAP bitmap, IGR_HPALETTE palette, IGR_LONG flags, const IGR_UCS2* options,
                                        IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* file_type, IGR_LONG* handle,
                                        Error_Control_Block* error);

/// @brief Opens a document from raw pixel information.
///
/// @param[in]  bitmap       Provides details on bitmap information
///                          processing.
/// @param[in]  flags        Specifies processing options for IGR_Get_Text
///                          output.
/// @param[in]  options      Extended processing options as Name=Value pairs
///                          joined by semicolons, or NULL for the defaults.
/// @param[out] capabilities Returns Document Capabilities bitfield.
/// @param[out] file_type    Returns Document Format Code.
/// @param[out] handle       Returns handle for subsequent calls.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_POINTER          @p bitmap is a null pointer, @p
///                                        file_type is a null pointer, or @p
///                                        capabilities is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER        @p flags, @p capabilities, @p
///                                        file_type, or @p handle is not
///                                        valid for high-definition mode.
/// @retval IGR_E_TOO_MANY_HANDLES         Too many documents are already
///                                        open.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS @p flags do not specify a supported
///                                        image or text processing mode.
/// @retval IGR_E_OPEN_ERROR               @p bitmap could not be decoded.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_DIB(const struct IGR_Open_DIB_Info* bitmap, IGR_LONG flags, const IGR_UCS2* options,
                                        IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* file_type, IGR_LONG* handle,
                                        Error_Control_Block* error);

/// @brief Returns the count of annotations for a given page.
///
/// @param[in]  pageHandle Handle to page returned from IGR_Open_Page.
/// @param[out] count      Returns count of annotations on the page.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p pageHandle is not a valid page
///                                      handle, @p count is a null pointer,
///                                      or @p error is a null pointer.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Count(IGR_HPAGE pageHandle, IGR_LONG* count, Error_Control_Block* error);

/// @brief Returns annotation objects for a given page.
///
/// @param[in]     pageHandle Handle to page returned from IGR_Open_Page.
/// @param[in]     index      Offset to begin returning annotations from.
/// @param[in,out] count      Length of items array on input, number of items
///                           filled on output.
/// @param[out]    items      Buffer to fill with up to count number of
///                           annotations.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p pageHandle is not a valid page
///                                      handle, @p count is a null pointer,
///                                      @p error is a null pointer, or @p
///                                      index is out of range.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotations(IGR_HPAGE pageHandle, IGR_LONG index, IGR_LONG* count, struct IGR_Annotation* items,
                                                    Error_Control_Block* error);

/// @brief Returns a string value for a given annotation.
///
/// @param[in]     item       \ref IGR_Annotation pointer from
///                           IGR_Get_Page_Annotations.
/// @param[in]     path       Path of the annotation property to return.
/// @param[in,out] char_count Size of buffer in chars on input, length of
///                           string on output.
/// @param[out]    buffer     Buffer to fill with string value.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p item is a null pointer, @p char_count
///                                 is a null pointer, @p buffer is a null
///                                 pointer, or @p error is a null pointer.
/// @retval IGR_E_NOT_FOUND         @p path does not resolve to a value in the
///                                 annotation's data.
/// @retval IGR_E_ERROR             @p path is not given, and the resulting
///                                 value is empty.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Str(const struct IGR_Annotation* item, const IGR_UCS2* path, IGR_LONG* char_count,
                                                       IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Returns an integer value for a given annotation.
///
/// @param[in]  item   \ref IGR_Annotation pointer from
///                    IGR_Get_Page_Annotations.
/// @param[in]  path   Path of the annotation property to return.
/// @param[out] buffer Buffer to fill with integer value.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER @p item is a null pointer, @p buffer is a
///                                 null pointer, or @p error is a null
///                                 pointer, or @p path is a null pointer or
///                                 resolves to an empty string.
/// @retval IGR_E_NOT_FOUND         @p path does not resolve to a value in the
///                                 annotation's data.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Annotation_Long(const struct IGR_Annotation* item, const IGR_UCS2* path, IGR_LONG* buffer,
                                                        Error_Control_Block* error);

/// @brief Returns the pixel data for the given page.
///
/// @param[in]     pageHandle   Handle to the page to get pixels from.
/// @param[in]     sourceRect   Rectangle to extract, or NULL for entire page.
/// @param[in]     destSize     Size of output, or NULL for default size.
/// @param[in]     flags        Processing flags.
/// @param[in]     options      Additional processing options.
/// @param[in]     pixel_format Format of pixel data to extract.
/// @param[in,out] pixel_data   Structure to populate with pixel data.
/// @param[out]    error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE         @p pageHandle does not resolve to an
///                                      open page.
/// @retval IGR_E_INVALID_PARAMETER      @p pageHandle is zero, @p pixel_data
///                                      is a null pointer, or @p error is a
///                                      null pointer.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Pixels(IGR_HPAGE pageHandle, const struct IGR_Rect* sourceRect, const struct IGR_Size* destSize,
                                               IGR_GET_PAGE_PIXELS_FLAGS_TYPE flags, const IGR_UCS2* options,
                                               IGR_OPEN_BITMAP_PIXEL_TYPE pixel_format, struct IGR_Page_Pixels* pixel_data,
                                               Error_Control_Block* error);

/// @brief Releases pixel data previously allocated by IGR_Get_Page_Pixels.
///
/// @param[in,out] pixel_data Structure populated with pixel data to be freed.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p pixel_data is a null pointer, or
///                                      @p error is a null pointer.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Free_Page_Pixels(struct IGR_Page_Pixels* pixel_data, Error_Control_Block* error);

/// @brief Removes all existing bookmarks from a canvas.
///
/// @param[in]  canvas Handle to the canvas to clear bookmarks from.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p canvas is not a valid canvas
///                                      handle, or @p error is a null
///                                      pointer.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Bookmarks_Clear(IGR_HCANVAS canvas, Error_Control_Block* error);

/// @brief Appends a single bookmark to a canvas.
///
/// @param[in]  canvas   Handle to the canvas to append bookmark to.
/// @param[in]  bookmark Pointer to \ref IGR_Bookmark data to add.
/// @param[out] error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p canvas is not a valid canvas
///                                      handle, @p bookmark is a null
///                                      pointer, or @p error is a null
///                                      pointer.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Bookmarks_Append(IGR_HCANVAS canvas, const struct IGR_Bookmark* bookmark, Error_Control_Block* error);

/// @brief Callback function for document processing events.
///
/// @param[in] action  Type of action that prompted the callback. See @ref
///                    callback_action_types for available action values.
/// @param[in] payload Action-specific structure containing event data.
/// @param[in] context User-provided context data passed to IGR_Open_Ex.
///
/// @return @ref error_codes value indicating how to proceed with processing.
/// @ingroup callback_functions
typedef IGR_LONG(IGR_EXPORT* IGR_OPEN_CALLBACK)(IGR_OPEN_CALLBACK_ACTION action, void* payload, void* context);

/// @brief Opens a document for text extraction or sub-document enumeration
///        with extended options
/// @param[in]  source_type      Type of source (filename or stream).
/// @param[in]  source           Pointer to source based on source_type.
/// @param[in]  flags            Processing options controlling output format.
/// @param[in]  options          Extended processing options as Name=Value
///                              pairs joined by semicolons, or NULL for the
///                              defaults.
/// @param[out] capabilities     Returns document capabilities bitfield.
/// @param[out] file_type        Returns document format code.
/// @param[in]  reserved         Must be NULL, reserved for future use.
/// @param[in]  callback         Event callback function.
/// @param[in]  callback_context User context passed to callback.
/// @param[out] handle           Handle for subsequent API calls.
/// @param[out] error            Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                         Operation succeeded.
/// @retval IGR_E_INVALID_POINTER          @p capabilities is a null pointer,
///                                        or @p file_type is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER        @p source_type is not a recognized
///                                        value.
/// @retval IGR_E_TOO_MANY_HANDLES         Too many documents are already
///                                        open.
/// @retval IGR_E_NOT_VALID_FOR_THIS_CLASS The format does not support a
///                                        required capability, or a needed
///                                        feature is disabled.
/// @retval IGR_E_PASSWORD                 The document is encrypted and
///                                        requires a password.
/// @retval IGR_E_DRM_PROTECTED            The document is protected by DRM.
/// @retval IGR_E_FILE_CORRUPT             The document is corrupt.
/// @retval IGR_CANCELLED                  The operation was cancelled by a
///                                        user-supplied callback.
/// @retval IGR_E_NOT_READABLE             The document could not be opened,
///                                        or its format could not be
///                                        recognized.
/// @retval IGR_E_OUT_OF_MEMORY            There is insufficient memory to
///                                        complete the operation.
/// @retval IGR_E_BAD_ERROR                An unexpected internal error
///                                        occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Open_Ex(IGR_OPEN_FROM source_type, const void* source, IGR_LONG flags, const IGR_UCS2* options,
                                       IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* file_type, void* reserved,
                                       IGR_OPEN_CALLBACK callback, void* callback_context, IGR_LONG* handle, Error_Control_Block* error);

/// @brief Gets page elements from a parent element.
///
/// @param[in]     page       Handle to page containing elements.
/// @param[in]     parent     Optional parent element to get children from.
/// @param[in]     first      Index of first element to extract.
/// @param[in,out] dest_count Size of dest array on input, number filled on
///                           output.
/// @param[in,out] dest       Array of \ref IGR_Page_Element to fill with
///                           data.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_POINTER   @p dest_count is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER A non-null @p dest is too small, or @p
///                                 dest is null while *dest_count is nonzero.
/// @retval IGR_E_INVALID_HANDLE    @p page is not a valid page handle, or a
///                                 non-null @p parent does not resolve to a
///                                 valid page element.
/// @retval IGR_NO_MORE             No elements were written.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Elements(IGR_HPAGE page, const struct IGR_Page_Element* parent, IGR_ULONG first,
                                                 IGR_ULONG* dest_count, struct IGR_Page_Element* dest, Error_Control_Block* error);

/// @brief Gets the root element of a page.
///
/// @param[in]     page  Handle to page to get root element from.
/// @param[in,out] dest  Pointer to \ref IGR_Page_Element to fill.
/// @param[out]    error Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p dest is a null pointer.
/// @retval IGR_E_INVALID_HANDLE  @p page is not a valid page handle.
/// @retval IGR_E_ERROR           The page has no root element.
/// @retval IGR_E_BAD_ERROR       An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Root(IGR_HPAGE page, struct IGR_Page_Element* dest, Error_Control_Block* error);

/// @brief Fills dest with the first child of the parent element.
///
/// @param[in]     page   Page element containing the page elements to
///                       extract.
/// @param[in]     parent Optional parent element from which children are
///                       extracted.
/// @param[in,out] dest   Pointer to \ref IGR_Page_Element struct. On input,
///                       struct_size must be set. On output, filled with
///                       element data.
/// @param[out]    error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_POINTER   @p dest is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER @p dest's struct_size is too small.
/// @retval IGR_E_INVALID_HANDLE    @p page is not a valid page handle, or a
///                                 non-null @p parent does not resolve to a
///                                 valid page element.
/// @retval IGR_E_ERROR             @p parent is not given, and the page has
///                                 no root element.
/// @retval IGR_NO_MORE             The given @p parent has no children.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_First_Child(IGR_HPAGE page, const struct IGR_Page_Element* parent,
                                                            struct IGR_Page_Element* dest, Error_Control_Block* error);

/// @brief Fills dest with the next sibling node of the element.
///
/// @param[in]     page    Page element containing the page elements to
///                        extract.
/// @param[in]     element Element whose sibling should be extracted.
/// @param[in,out] dest    Pointer to \ref IGR_Page_Element struct. On input,
///                        struct_size must be set. On output, filled with
///                        element data.
/// @param[out]    error   Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_POINTER   @p dest is a null pointer, or @p element
///                                 is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER @p element does not resolve to a valid
///                                 page element.
/// @retval IGR_E_INVALID_HANDLE    @p page is not a valid page handle.
/// @retval IGR_NO_MORE             @p element has no next sibling.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Next_Sibling(IGR_HPAGE page, const struct IGR_Page_Element* element,
                                                             struct IGR_Page_Element* dest, Error_Control_Block* error);

/// @brief Gets text associated with a page element.
///
/// @param[in]     page       Page element containing elements to extract
///                           from.
/// @param[in]     source     Element to extract text from.
/// @param[in,out] char_count On input, size of buffer in chars. On output,
///                           chars copied.
/// @param[in,out] buffer     UCS2 array to receive text.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_POINTER @p source is a null pointer, or @p
///                               char_count is a null pointer.
/// @retval IGR_E_INVALID_HANDLE  @p page is not a valid page handle.
/// @retval IGR_E_BAD_ERROR       An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Text(IGR_HPAGE page, const struct IGR_Page_Element* source, IGR_ULONG* char_count,
                                                     IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Calls callback with each style for the given source element.
///
/// @param[in]  page     Page element containing elements to extract from.
/// @param[in]  source   Element to extract styles from.
/// @param[in]  callback Function to receive style information.
/// @param[in]  context  User data passed to callback.
/// @param[out] error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p page is not a valid page handle.
/// @retval IGR_E_INVALID_POINTER One or more parameters are invalid.
/// @retval IGR_E_BAD_ERROR       An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Styles(IGR_HPAGE page, const struct IGR_Page_Element* source,
                                                       IGR_PAGE_ELEMENT_STYLES_CALLBACK callback, void* context,
                                                       Error_Control_Block* error);

/// @brief Gets named style value from a page element.
///
/// @param[in]     page       Page element containing elements to extract
///                           from.
/// @param[in]     source     Element to get style from.
/// @param[in]     name       Name of style to retrieve.
/// @param[in,out] char_count On input, size of buffer. On output, chars
///                           copied.
/// @param[in,out] buffer     UCS2 array to receive style value.
/// @param[out]    error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE  @p page is not a valid page handle.
/// @retval IGR_E_INVALID_POINTER One or more parameters are invalid.
/// @retval IGR_E_BAD_ERROR       An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Element_Style(IGR_HPAGE page, const struct IGR_Page_Element* source, const IGR_UCS2* name,
                                                      IGR_ULONG* char_count, IGR_UCS2* buffer, Error_Control_Block* error);

/// @brief Recursively walks page elements calling callback for each element.
///
/// @param[in]  page      Page containing elements to enumerate.
/// @param[in]  source    Root element to start enumeration from.
/// @param[in]  flags     Reserved for future use.
/// @param[in]  max_depth Maximum depth of page elements to traverse.
/// @param[in]  callback  Function called for each element.
/// @param[in]  context   User data passed to callback.
/// @param[out] error     Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                  Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE    @p page is not a valid page handle.
/// @retval IGR_E_INVALID_PARAMETER A non-null @p source does not resolve to a
///                                 valid page element.
/// @retval IGR_E_ERROR             @p callback is a null pointer, or the
///                                 processing engine is not ready.
/// @retval IGR_E_BAD_ERROR         An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Enum_Page_Elements(IGR_HPAGE page, const struct IGR_Page_Element* source, IGR_ULONG flags,
                                                  IGR_ULONG max_depth, IGR_PAGE_ELEMENT_CALLBACK callback, void* context,
                                                  Error_Control_Block* error);

/// @brief Gets the type and capabilities of an open document.
///
/// @param[in]  handle       Handle to an open document.
/// @param[out] capabilities Returns Document Capabilities bitfield.
/// @param[out] filetype     Returns Document Format Code.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK               Operation succeeded.
/// @retval IGR_E_INVALID_HANDLE @p handle is not a valid document handle.
/// @retval IGR_E_ERROR          The processing engine is not initialized, or
///                              an unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Handle_File_Type(IGR_HDOC handle, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype,
                                                    Error_Control_Block* error);

/// @brief Gets the type and capabilities of an open page.
///
/// @param[in]  handle       Handle to an open page.
/// @param[out] capabilities Returns Document Capabilities bitfield.
/// @param[out] filetype     Returns Document Format Code.
/// @param[out] error        Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p handle is zero.
/// @retval IGR_E_INVALID_HANDLE         @p handle does not resolve to a page
///                                      associated with an open document.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Handle_File_Type(IGR_HPAGE handle, IGR_FILE_SUPPORTS_TYPE* capabilities, IGR_LONG* filetype,
                                                         Error_Control_Block* error);

/// @brief Compares two documents and returns enumerator for differences.
///
/// @param[in]  doc1       First document to compare.
/// @param[in]  doc2       Second document to compare.
/// @param[in]  settings   Settings controlling text comparison.
/// @param[out] enumerator Handle to enumerator for iterating differences.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p doc1 is a null pointer or
///                                      malformed, @p doc2 is a null pointer
///                                      or malformed, or @p settings is a
///                                      null pointer or malformed.
/// @retval IGR_E_INVALID_HANDLE         @p doc1 is too small or does not
///                                      resolve to a valid document handle,
///                                      or @p doc2 is too small or does not
///                                      resolve to a valid document handle.
/// @retval IGR_E_NOT_READABLE           The document is not valid for text
///                                      comparison.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Documents(const struct IGR_Text_Compare_Document_Source* doc1,
                                                      const struct IGR_Text_Compare_Document_Source* doc2,
                                                      const struct IGR_Text_Compare_Settings* settings, IGR_HTEXTCOMPARE* enumerator,
                                                      Error_Control_Block* error);

/// @brief Compares two pages and returns enumerator for differences.
///
/// @param[in]  page1         First page to compare.
/// @param[in]  page1_margins Margins for first page.
/// @param[in]  page2         Second page to compare.
/// @param[in]  page2_margins Margins for second page.
/// @param[in]  settings      Settings controlling text comparison.
/// @param[out] enumerator    Handle to enumerator for iterating differences.
/// @param[out] error         Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p page1 is zero, @p page2 is zero,
///                                      or @p settings is a null pointer or
///                                      malformed.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Pages(IGR_HPAGE page1, const struct IGR_FRect* page1_margins, IGR_HPAGE page2,
                                                  const struct IGR_FRect* page2_margins, const struct IGR_Text_Compare_Settings* settings,
                                                  IGR_HTEXTCOMPARE* enumerator, Error_Control_Block* error);

/// @brief Compares elements within pages and returns enumerator for
///        differences.
///
/// @param[in]  page1          First page containing elements.
/// @param[in]  page1_ele_root Root element from first page.
/// @param[in]  page2          Second page containing elements.
/// @param[in]  page2_ele_root Root element from second page.
/// @param[in]  settings       Settings controlling text comparison.
/// @param[out] enumerator     Handle to enumerator for iterating differences.
/// @param[out] error          Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_POINTER        @p page1 is a null pointer, @p
///                                      page1_ele_root is a null pointer, @p
///                                      page2 is a null pointer, @p
///                                      page2_ele_root is a null pointer, or
///                                      @p enumerator is a null pointer.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Elements(IGR_HPAGE page1, const struct IGR_Page_Element* page1_ele_root, IGR_HPAGE page2,
                                                     const struct IGR_Page_Element* page2_ele_root,
                                                     const struct IGR_Text_Compare_Settings* settings, IGR_HTEXTCOMPARE* enumerator,
                                                     Error_Control_Block* error);

/// @brief Closes text comparison enumerator and frees resources.
///
/// @param[in]  enumerator Handle to enumerator to close.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p enumerator is a null pointer.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Close(IGR_HTEXTCOMPARE enumerator, Error_Control_Block* error);

/// @brief Resets text comparison enumerator to initial state.
///
/// @param[in]  enumerator Handle to enumerator to reset.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p enumerator is a null pointer.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Reset(IGR_HTEXTCOMPARE enumerator, Error_Control_Block* error);

/// @brief Retrieves the next difference from the text comparison enumerator.
///
/// @param[in]  enumerator Handle to the text comparison enumerator.
/// @param[out] result     Pointer to \ref IGR_Compare_Documents_Difference
///                        structure to store the next difference.
/// @param[out] error      Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p enumerator is a null pointer, or
///                                      @p result is a null pointer.
/// @retval IGR_NO_MORE                  The enumerator has no more
///                                      differences to return.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Next(IGR_HTEXTCOMPARE enumerator, struct IGR_Compare_Documents_Difference* result,
                                                 Error_Control_Block* error);

/// @brief Disposes of a difference result returned by the text comparison
///        enumerator.
///
/// @param[in]  result Pointer to \ref IGR_Compare_Documents_Difference
///                    structure to dispose of.
/// @param[out] error  Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                       Operation succeeded.
/// @retval IGR_E_INVALID_PARAMETER      @p result is a null pointer or
///                                      malformed.
/// @retval IGR_E_ERROR, IGR_E_BAD_ERROR An unexpected internal error
///                                      occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_Text_Compare_Difference_Dispose(struct IGR_Compare_Documents_Difference* result, Error_Control_Block* error);

/// @brief Calls callback once for each machine learning model this
///        installation can name.
///
/// Reports the models found on disk, which is not the same as the models that
/// can run. @ref IGR_ML_MODEL_FLAG_BACKEND_READY reports whether the ML backend
/// module is loaded; the ONNX Runtime that executes the models is installed
/// separately and loaded when a model first runs, so its absence surfaces as a
/// failure from that run rather than in the flags here. An installation with no model directory
/// enumerates nothing and returns IGR_OK.
///
/// Needs no open document. The first call loads the rendering engine, and the
/// set of models is resolved once per process and then reused, so later calls
/// are cheap.
///
/// @param[in]  callback Function called for each model. The structure it
///                      receives belongs to Document Filters and is valid
///                      only until the call returns; return IGR_OK to
///                      continue or anything else to stop.
/// @param[in]  context  User data passed to callback.
/// @param[out] error    Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                Every model was reported, or there were
///                               none.
/// @retval IGR_E_INVALID_POINTER @p callback is a null pointer.
/// @retval IGR_E_BAD_ERROR       The rendering engine could not be loaded, or
///                               an unexpected internal error occurred.
///
/// Any other value is one the callback returned to stop the enumeration.
IGR_RETURN_CODE IGR_EXPORT IGR_ML_Enum_Models(IGR_ML_MODEL_CALLBACK callback, void* context, Error_Control_Block* error);

/// @brief Runs a machine learning model over a page, or a region of one.
///
/// The action is not a parameter: @p result says what kind of structure it is,
/// and that selects the action. Asking a model for something it does not do,
/// such as a classification from a caption model, is IGR_E_WRONG_TYPE rather
/// than a failed run.
///
/// Pixels are produced only once the model has been resolved, so naming a
/// model nothing registered costs no render. The first run of a given model
/// loads it, which for a vision backbone means reading hundreds of megabytes
/// and building a session; the instance is then kept for the life of the
/// document, so only the first run on a document pays.
///
/// @param[in]     page           Handle to the page to analyse.
/// @param[in]     model          Registered name of the model to run, as
///                               reported by IGR_ML_Enum_Models.
/// @param[in]     render         Which part of the page to analyse and at
///                               what resolution, or NULL for the whole page
///                               at the model's preferred resolution.
/// @param[in]     property_count Number of entries in @p properties.
/// @param[in]     properties     Per-run settings for the model, or NULL when
///                               @p property_count is zero. Reserved: no
///                               backend consumes these yet, and naming one
///                               of the model's registered options here is
///                               IGR_E_INVALID_PARAMETER, since those are
///                               fixed when the model loads.
/// @param[in,out] result         An IGR_ML_Classification or
///                               IGR_ML_Description, with @c struct_size and
///                               @c type filled in.
/// @param[out]    error          Contains any error text.
///
/// @return A value from @ref error_codes indicating success or failure.
/// @retval IGR_OK                    The model ran and @p result is
///                                   populated. Also returned when a
///                                   description was truncated to fit.
/// @retval IGR_E_INVALID_HANDLE      @p page does not resolve to an open page.
/// @retval IGR_E_INVALID_POINTER     @p model or @p result is a null pointer.
/// @retval IGR_E_INVALID_PARAMETER   A structure's @c struct_size is too small
///                                   for its type, @c type is not a type that
///                                   can be run, @p render is malformed, or a
///                                   property names a registered option.
/// @retval IGR_E_NOT_FOUND           @p model is not a registered model.
/// @retval IGR_E_WRONG_TYPE          @p model is registered, but not as the
///                                   kind of model @p result asks for.
/// @retval IGR_E_ACTION_NOT_SUPPORTED No backend is loaded, or the one loaded
///                                   does not offer this action. Check
///                                   @ref IGR_ML_MODEL_FLAG_BACKEND_READY to
///                                   tell those apart.
/// @retval IGR_E_ERROR               The model ran and failed.
/// @retval IGR_E_BAD_ERROR           An unexpected internal error occurred.
IGR_RETURN_CODE IGR_EXPORT IGR_ML_Run_Page(IGR_HPAGE page, const IGR_UCS2* model, const struct IGR_ML_Render* render,
                                           IGR_ULONG property_count, const struct IGR_ML_Property* properties, void* result,
                                           Error_Control_Block* error);

#ifdef __cplusplus
};
#endif

/// @cond
#ifndef PERCEPTIVE_DOCFILTERS_H_DEF
#	define PERCEPTIVE_DOCFILTERS_H_DEF
#endif
/// @endcond

#endif // HYLAND_DOCFILTERS_H_DEF
