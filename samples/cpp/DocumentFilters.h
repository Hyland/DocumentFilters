/*
Copyright (c) 1988-2014, Perceptive Software. All rights reserved.

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
* Perceptive Document Filters 11 C API
****************************************************************************/

#ifndef PERCEPTIVE_DOCFILTERS_H_DEF
#define PERCEPTIVE_DOCFILTERS_H_DEF

const int PERCEPTIVE_SZ_MD5_HEX_LEN = 33;
const int PERCEPTIVE_SZ_SHA1_HEX_LEN = 41;

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
# define PERCEPTIVE_PLATFORM_WINDOWS
# define PERCEPTIVE_PLATFORM_LITTLE_ENDIAN
#elif defined(__linux__) && defined (__powerpc) && !defined(_LITTLE_ENDIAN)
# define PERCEPTIVE_PLATFORM_LINUX
# define PERCEPTIVE_PLATFORM_BIG_ENDIAN
#elif defined(__linux__)
# define PERCEPTIVE_PLATFORM_LINUX
# define PERCEPTIVE_PLATFORM_LITTLE_ENDIAN
#elif defined(__FreeBSD__)
# define PERCEPTIVE_PLATFORM_FREEBSD
# define PERCEPTIVE_PLATFORM_LITTLE_ENDIAN
#elif defined(__APPLE__)
# define PERCEPTIVE_PLATFORM_MACOSX
# define PERCEPTIVE_PLATFORM_LITTLE_ENDIAN
#elif defined(__sun) && defined (__sparc)
# define PERCEPTIVE_PLATFORM_SOLARIS
# define PERCEPTIVE_PLATFORM_SOLARIS_SPARC
# define PERCEPTIVE_PLATFORM_BIG_ENDIAN
#elif defined(__sun)
# define PERCEPTIVE_PLATFORM_SOLARIS
# define PERCEPTIVE_PLATFORM_SOLARIS_INTEL
# define PERCEPTIVE_PLATFORM_LITTLE_ENDIAN
#elif defined(__hpux)
# define PERCEPTIVE_PLATFORM_HPUX
# define PERCEPTIVE_PLATFORM_BIG_ENDIAN
#elif defined(_AIX)
# define PERCEPTIVE_PLATFORM_AIX
# define PERCEPTIVE_PLATFORM_BIG_ENDIAN
#else
# error "Perceptive Document Filters: Unsupported platform"
#endif

#include <stdlib.h>
#ifdef PERCEPTIVE_PLATFORM_WINDOWS
#  include <windows.h>
#  define IGR_EXPORT  WINAPI
#else
#  ifndef WCHAR
#    define WCHAR unsigned short
#  endif
#  ifndef BOOL
#    define BOOL int
#  endif
#  ifndef SHORT
#    define SHORT short
#  endif
#  ifndef LPCWSTR
#    define LPCWSTR const WCHAR*
#  endif
#  ifndef LONG
#    define LONG int
#  endif
#  ifndef ULONG
#    define ULONG unsigned LONG
#  endif
#  ifndef BYTE
#    define BYTE unsigned char
#  endif
#  define CALLBACK
#  define IGR_EXPORT
#  define FARPROC void*
#  define HANDLE void*
#  define HINSTANCE HANDLE
#  define HMODULE HANDLE
#  define HWND HANDLE
#  define LPWSTR WCHAR*
#  define LPSTR char*
#  define LPCSTR const char*
#  define LONGLONG long long
#  define ULONGLONG unsigned LONGLONG
#  define PLONGLONG LONGLONG*
#endif

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

typedef LONG IGR_RETURN_CODE;
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
#define IGR_DEVICE_IMAGE_PNG  0
#define IGR_DEVICE_IMAGE_JPG  1
#define IGR_DEVICE_IMAGE_PDF  2
#define IGR_DEVICE_IMAGE_TIF  3
#define IGR_DEVICE_IMAGE_BMP  4
#define IGR_DEVICE_IMAGE_PBM  7
#define IGR_DEVICE_IMAGE_PGM  8
#define IGR_DEVICE_IMAGE_PPM  9
#define IGR_DEVICE_IMAGE_WEBP 10
#define IGR_DEVICE_IMAGE_XPS  11
#define IGR_DEVICE_IMAGE_SVG  12
#define IGR_DEVICE_IMAGE_EPS  13
#define IGR_DEVICE_IMAGE_PS   14
#define IGR_DEVICE_XML        5
#define IGR_DEVICE_HTML       6

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

	/**
	*  IGR_CALLBACK Actions
	*/
#define IGR_ACTION_GET_STREAM_PART  10


#define MAKE_COLOR(R, G, B)       ((int) R | ((int) G << 8) | ((int) B << 16))

	typedef struct tagError_Control_Block
	{
		char   Msg[238];
		SHORT  MsgId;
	} Error_Control_Block;

	typedef Error_Control_Block *lpECB_Type;
	typedef Error_Control_Block ECB_Type;

	typedef struct tagInstance_Status_Block   /* Returned from Init_Instance */
	{
		char   DLL_Version[16];
		SHORT  RReserved;
		SHORT  License_Type;
		char   Licensee_ID1[40];
		char   Licensee_ID2[40];
	} Instance_Status_Block, *LPInstance_Status_Block;

	typedef struct IGR_Stream *PIGR_Stream;
#ifndef _IGR_Stream
	struct IGR_Stream
	{
		ULONG (CALLBACK* Open) (struct IGR_Stream* stream);
		ULONG (CALLBACK* Read) (struct IGR_Stream* stream, void* buffer, ULONG bufferSize);
		LONGLONG (CALLBACK* Seek) (struct IGR_Stream* stream, LONGLONG offset, ULONG whence);
		void (CALLBACK* Close) (struct IGR_Stream* stream);
	} ;
#endif

	typedef struct IGR_Writable_Stream *PIGR_Writable_Stream;
	struct IGR_Writable_Stream
	{
		struct IGR_Stream base;
		ULONG (CALLBACK* Write) (struct IGR_Writable_Stream* stream, void* buffer, ULONG bufferSize);
	};

	typedef LONG HPAGE;
	typedef LONG HCANVAS;

	/**
	* Structure containing positional information about each word on a page. Units are pixels.
	*/
	struct IGR_Page_Word
	{
		int x;
		int y;
		int width;
		int height;

		/*
		* Style information is reserved for future use
		*/
		int style;

		/*
		* Pointer to a UCS2 string, the pointer is only valid while the HPAGE is open
		*/
		const WCHAR* word;
		int wordLength;
		int charoffset;
		int reserved[8];
	};

	typedef LONG (*IGR_CALLBACK)(int actionID, void* actionData, void* context);

	/**
	* Structure that is passed to IGR_CALLBACK's actionData when actionID is IGR_ACTION_GET_STREAM_PART
	*/
	struct IGR_T_ACTION_GET_STREAM_PART
	{
		unsigned int size;
		const wchar_t* partName;        // in (e.g. foo.ra01)
		const wchar_t* partFullName;    // in (e.g. c:\foo.ra01)
		int partIndex;                  // in
		struct IGR_Stream* istr;        // out
	};


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
	void IGR_EXPORT Init_Instance(LONG reserved, const char* path, Instance_Status_Block* init, SHORT* handle, Error_Control_Block* error);

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
	void IGR_EXPORT Switch_Instance(SHORT handle, Error_Control_Block* error);

	/**
	*  Widechar_to_UTF8 converts a UCS2 string into UTF-8
	*    source     - [in]  contains a NULL terminated UCS2 string
	*    buffer     - [out] pointer to a buffer that will hold the converted text
	*    bufferSize - [in]  indicates the size of the buffer, the system will copy upto this size including the NULL.
	*  Returns
	*    None
	*/
	void IGR_EXPORT Widechar_to_UTF8(LPCWSTR source, LPSTR buffer, LONG bufferSize);
	void IGR_EXPORT Widechar_to_UTF8_Ex(LPCWSTR source, LONG sourceLen, LPSTR buffer, LONG bufferSize);

	/**
	*  UTF8_to_Widechar converts a UTF-* string into a UCS2 string
	*    source     - [in]  contains a NULL terminated UTF-8 string
	*    buffer     - [out] pointer to a buffer that will hold the converted text
	*    bufferSize - [in]  indicates the size of the buffer, the system will copy upto this size including the NULL.
	*  Returns
	*    None
	*/
	void IGR_EXPORT UTF8_to_Widechar(LPCSTR source, LPWSTR buffer, LONG bufferSize);
	void IGR_EXPORT UTF8_to_Widechar_Ex(LPCSTR source, LONG sourceLen, LPWSTR buffer, LONG bufferSize);

	/**
	*  Set_Engine_Call_Tracing is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Set_Engine_Call_Tracing(LONG, LONG, LPCSTR);

	/**
	*  Set_API_Charset is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Set_API_Charset(LONG, Error_Control_Block* error);

	/**
	*  Set_DLL_Option is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT Set_DLL_Option(LPCSTR, Error_Control_Block* error);

	/**
	*  ISYS_Multiplex is for legacy use only, new code does not need to call this method
	*    error       - [out] contains any error text
	*  Returns
	*    None
	*/
	void IGR_EXPORT ISYS_Multiplex(LONG, size_t*, size_t*, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_File_Type(LPCWSTR filename, LONG* capabilities, LONG* filetype, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Stream_Type(PIGR_Stream stream, LONG* capabilities, LONG* filetype, Error_Control_Block* error);


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
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_File(LPCWSTR filename, LONG flags, LONG* capabilities, LONG* filetype, LONG* handle, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_Stream(PIGR_Stream stream, LONG flags, LONG* capabilities, LONG* filetype, LONG* handle, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_File_Ex(LPCWSTR filename, LONG flags, LPCWSTR options, LONG* capabilities, LONG* filetype, LONG* handle, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_Stream_Ex(PIGR_Stream stream, LONG flags, LPCWSTR options, LONG* capabilities, LONG* filetype, LONG* handle, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Text(LONG handle, LPWSTR buffer, LONG* bufferSize, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Subfile_Entry(LONG handle, LPWSTR id, LPWSTR name, PLONGLONG date, PLONGLONG size, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Subfile(LONG handle, LPCWSTR id, LPCWSTR destination, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Subfile_Stream(LONG handle, LPCWSTR id, PIGR_Stream* stream, Error_Control_Block* error);

	/**
	*  IGR_Close_File releases the resources associated with the file handle. It must be called for every document opened by IGR_Open_File,
	*  IGR_Open_File_Ex, IGR_Open_Stream or IGR_Open_Stream_Ex.
	*    handle       - [in]  Handle to a document, opened by a call to IGR_Open_File, IGR_Open_Stream, IGR_Open_File_Ex or IGR_Open_Stream_Ex.
	*    error        - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_POINTER - return if pageHandle or error are invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Close_File(LONG handle, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Multiplex(LONG function, size_t* param1, size_t* param2, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Convert_File(LPCWSTR filename, LONG flags, LPCWSTR options, LPCWSTR output, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_File(LPCWSTR filename, LONG flags, PIGR_Stream* stream, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Stream_From_Memory(void* buffer, size_t bufferSize, void* destruct, PIGR_Stream* stream, Error_Control_Block* error);

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
	size_t IGR_EXPORT IGR_Get_Memory_Status(LPSTR buffer, size_t len);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Image_Entry(LONG handle, LPWSTR id, LPWSTR name, PLONGLONG date, PLONGLONG size, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Image(LONG handle, LPCWSTR id, LPCWSTR destination, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Image_Stream(LONG handle, LPCWSTR id, PIGR_Stream* stream, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Calculate_MD5(PIGR_Stream stream, LPWSTR dest, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Calculate_SHA1(PIGR_Stream stream, LPWSTR dest, Error_Control_Block* error);

	/**
	*  Return the number of pages for a document opened in HD mode.
	*  Returns
	*   IGR_OK
	*   IGR_E_NOT_VALID_FOR_THIS_CLASS - returned when docHandle not opened in HD mode
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Count(LONG docHandle, LONG* pageCount, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Open_Page(LONG docHandle, LONG pageIndex, HPAGE* pageHandle, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Word_Count(HPAGE pageHandle, LONG* wordCount, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Words(HPAGE pageHandle, LONG index, LONG *count, struct IGR_Page_Word* words, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Dimensions(HPAGE pageHandle, LONG* width, LONG* height, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Text(HPAGE pageHandle, WCHAR* buffer, LONG* size, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Image_Entry(HPAGE pageHandle, LPWSTR id, LPWSTR name, PLONGLONG date, PLONGLONG size, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Page_Image(HPAGE pageHandle, LPCWSTR id, LPCWSTR destination, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Extract_Page_Image_Stream(HPAGE pageHandle, LPCWSTR id, PIGR_Stream* stream, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Output_Canvas(LONG type, const WCHAR* filename, const WCHAR* options, HCANVAS* canvasHandle, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Make_Output_Canvas_On(LONG type, struct IGR_Writable_Stream* stream, const WCHAR* options, HCANVAS* canvasHandle, Error_Control_Block* error);

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
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if error is invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Render_Page(HPAGE page, HCANVAS canvas, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Redact_Page_Text(HPAGE page, LONG From, LONG To, LONG Type, Error_Control_Block* error);

	/**
	*  Returns page style/attribute information
	*    page        - [in] the handle to a page returned from IGR_Open_Page
	*    name        - [in] the first word to be redacted
	*    buffer      - [out] buffer that is filled with the requested attribute/style
	*    size        - [in/out] contains the size of the buffer, in characters. On return, is set to the size of the string
	*    error       - [out] contains any error text
	*  Returns
	*   IGR_OK
	*   IGR_E_INVALID_HANDLE - returned if docHandle is invalid
	*   IGR_E_INVALID_POINTER - returned if error is invalid
	*/
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Page_Attribute(HPAGE page, const WCHAR* name, WCHAR* buffer, LONG* size, Error_Control_Block* error);

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
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Format_Attribute(LONG formatid, LONG what, char* buffer, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Get_Option_Attribute(LONG formatid, LONG what, char* buffer, size_t bufferSize, Error_Control_Block* error);

	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Arc(HCANVAS canvas, LONG x, LONG y, LONG x2, LONG y2, LONG x3, LONG y3, LONG x4, LONG y4, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_AngleArc(HCANVAS canvas, LONG x, LONG y, LONG radius, LONG startAngle, LONG sweepAngle, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Chord(HCANVAS canvas, LONG x, LONG y, LONG x2, LONG y2, LONG x3, LONG y3, LONG x4, LONG y4, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Ellipse(HCANVAS canvas, LONG x, LONG y, LONG x2, LONG y2, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Rect(HCANVAS canvas, LONG x, LONG y, LONG x2, LONG y2, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_LineTo(HCANVAS canvas, LONG x, LONG y, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_MoveTo(HCANVAS canvas, LONG x, LONG y, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Pie(HCANVAS canvas, LONG x, LONG y, LONG x2, LONG y2, LONG x3, LONG y3, LONG x4, LONG y4, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_RoundRect(HCANVAS canvas, LONG x, LONG y, LONG x2, LONG y2, LONG radius, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_TextOut(HCANVAS canvas, LONG x, LONG y, const WCHAR* text, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_TextRect(HCANVAS canvas, LONG x, LONG y, LONG x2, LONG y2, const WCHAR* text, LONG flags, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_MeasureText(HCANVAS canvas, const WCHAR* text, LONG* width, LONG* height, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetPen(HCANVAS canvas, LONG color, LONG width, LONG style, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetBrush(HCANVAS canvas, LONG color, LONG style, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetFont(HCANVAS canvas, const WCHAR* fontFamily, LONG size, LONG style, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_SetOpacity(HCANVAS canvas, BYTE opacity, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_DrawImage(HCANVAS canvas, LONG x, LONG y, void* imagedata, size_t imagesize, const WCHAR* mimetype, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_DrawScaleImage(HCANVAS canvas, LONG x, LONG y, LONG x2, LONG y2, void* imagedata, size_t imagesize, const WCHAR* mimetype, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Rotation(HCANVAS canvas, LONG degrees, Error_Control_Block* error);
	IGR_RETURN_CODE IGR_EXPORT IGR_Canvas_Reset(HCANVAS canvas, Error_Control_Block* error);

#ifdef __cplusplus
};
#endif

#endif // PERCEPTIVE_DOCFILTERS_H_DEF
