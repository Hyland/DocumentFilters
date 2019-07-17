//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

#if NETSTANDARD
[assembly:DefaultDllImportSearchPaths(DllImportSearchPath.AssemblyDirectory | DllImportSearchPath.ApplicationDirectory | DllImportSearchPath.SafeDirectories | DllImportSearchPath.UseDllDirectoryForDependencies)]
#endif


namespace Hyland.DocumentFilters
{
    public partial class ISYS11dfConstants
    {
        /// IGR_Multi_Set_Code_Page -> 1
        public const int IGR_Multi_Set_Code_Page = 1;

        /// IGR_Multi_Set_Temp_Path -> 186
        public const int IGR_Multi_Set_Temp_Path = 186;

        /// IGR_BODY_ONLY -> 0
        public const int IGR_BODY_ONLY = 0;

        /// IGR_META_ONLY -> 1
        public const int IGR_META_ONLY = 1;

        /// IGR_BODY_AND_META -> 2
        public const int IGR_BODY_AND_META = 2;

        /// IGR_FORMAT_TEXT -> 0x00010000L
        public const int IGR_FORMAT_TEXT = 65536;

        /// IGR_FORMAT_HTML -> 0x00020000L
        public const int IGR_FORMAT_HTML = 131072;

        /// IGR_FORMAT_XML -> 0x00030000L
        public const int IGR_FORMAT_XML = 196608;

        /// IGR_FORMAT_HDHTML_IFYOUCAN -> 0x00040000L
        public const int IGR_FORMAT_HDHTML_IFYOUCAN = 262144;

        /// IGR_FORMAT_IMAGE -> 0x00050000L
        public const int IGR_FORMAT_IMAGE = 327680;

        /// IGR_FORMAT_HTML_NO_FALLBACK -> 0x00060000L
        public const int IGR_FORMAT_HTML_NO_FALLBACK = 393216;

        /// IGR_FILE_SUPPORTS_TEXT -> 1
        public const int IGR_FILE_SUPPORTS_TEXT = 1;

        /// IGR_FILE_SUPPORTS_SUBFILES -> 2
        public const int IGR_FILE_SUPPORTS_SUBFILES = 2;

        /// IGR_FILE_SUPPORTS_HDHTML -> 4
        public const int IGR_FILE_SUPPORTS_HDHTML = 4;

        /// IGR_OK -> 0
        public const int IGR_OK = 0;

        /// IGR_E_OPEN_ERROR -> 1
        public const int IGR_E_OPEN_ERROR = 1;

        /// IGR_E_WRONG_TYPE -> 2
        public const int IGR_E_WRONG_TYPE = 2;

        /// IGR_E_IN_USE -> 3
        public const int IGR_E_IN_USE = 3;

        /// IGR_E_NOT_READABLE -> 4
        public const int IGR_E_NOT_READABLE = 4;

        /// IGR_E_PASSWORD -> 5
        public const int IGR_E_PASSWORD = 5;

        /// IGR_E_NOT_FOUND -> 10
        public const int IGR_E_NOT_FOUND = 10;

        /// IGR_E_WRITE_ERROR -> 11
        public const int IGR_E_WRITE_ERROR = 11;

        /// IGR_E_NOT_VALID_FOR_THIS_CLASS -> 12
        public const int IGR_E_NOT_VALID_FOR_THIS_CLASS = 12;

        /// IGR_E_ERROR -> 13
        public const int IGR_E_ERROR = 13;

        /// IGR_E_INVALID_HANDLE -> 14
        public const int IGR_E_INVALID_HANDLE = 14;

        /// IGR_E_INVALID_POINTER -> 15
        public const int IGR_E_INVALID_POINTER = 15;

        /// IGR_E_INVALID_PARAMETER -> 16
        public const int IGR_E_INVALID_PARAMETER = 16;

        /// IGR_E_FILE_CORRUPT -> 17
        public const int IGR_E_FILE_CORRUPT = 17;

        /// IGR_NO_MORE -> 19
        public const int IGR_NO_MORE = 19;

        /// IGR_E_OUT_OF_MEMORY -> 20
        public const int IGR_E_OUT_OF_MEMORY = 20;

        /// IGR_E_BAD_ERROR -> 21
        public const int IGR_E_BAD_ERROR = 21;

        /// IGR_E_WRONG_THREAD -> 22
        public const int IGR_E_WRONG_THREAD = 22;

        /// IGR_E_TOO_MANY_HANDLES -> 23
        public const int IGR_E_TOO_MANY_HANDLES = 23;

        /// IGR_E_FONTS_NOT_FOUND -> 24
        public const int IGR_E_FONTS_NOT_FOUND = 24;

        /// IGR_CHAR_SOFT_SPACE -> 1
        public const int IGR_CHAR_SOFT_SPACE = 1;

        /// IGR_CHAR_STYLE_NORMAL -> 2
        public const int IGR_CHAR_STYLE_NORMAL = 2;

        /// IGR_CHAR_STYLE_BOLD -> 3
        public const int IGR_CHAR_STYLE_BOLD = 3;

        /// IGR_CHAR_STYLE_ITALICS -> 4
        public const int IGR_CHAR_STYLE_ITALICS = 4;

        /// IGR_CHAR_STYLE_UNDERLINED -> 5
        public const int IGR_CHAR_STYLE_UNDERLINED = 5;

        /// IGR_CHAR_TAB -> 9
        public const int IGR_CHAR_TAB = 9;

        /// IGR_CHAR_PAGE_BREAK -> 12
        public const int IGR_CHAR_PAGE_BREAK = 12;

        /// IGR_CHAR_LINE_BREAK -> 13
        public const int IGR_CHAR_LINE_BREAK = 13;

        /// IGR_CHAR_PARA_BREAK -> 14
        public const int IGR_CHAR_PARA_BREAK = 14;

        /// IGR_CHAR_START_META -> 15
        public const int IGR_CHAR_START_META = 15;

        /// IGR_CHAR_END_META -> 16
        public const int IGR_CHAR_END_META = 16;

        /// IGR_DEVICE_IMAGE_PNG -> 0
        public const int IGR_DEVICE_IMAGE_PNG = 0;

        /// IGR_DEVICE_IMAGE_JPG -> 1
        public const int IGR_DEVICE_IMAGE_JPG = 1;

        /// IGR_DEVICE_IMAGE_PDF -> 2
        public const int IGR_DEVICE_IMAGE_PDF = 2;

        /// IGR_DEVICE_IMAGE_TIF -> 3
        public const int IGR_DEVICE_IMAGE_TIF = 3;

        /// IGR_DEVICE_IMAGE_BMP -> 4
        public const int IGR_DEVICE_IMAGE_BMP = 4;

        /// IGR_DEVICE_IMAGE_PBM -> 7
        public const int IGR_DEVICE_IMAGE_PBM = 7;

        /// IGR_DEVICE_IMAGE_PGM -> 8
        public const int IGR_DEVICE_IMAGE_PGM = 8;

        /// IGR_DEVICE_IMAGE_PPM -> 9
        public const int IGR_DEVICE_IMAGE_PPM = 9;

        /// IGR_DEVICE_IMAGE_WEBP -> 10
        public const int IGR_DEVICE_IMAGE_WEBP = 10;

        /// IGR_DEVICE_IMAGE_XPS -> 11
        public const int IGR_DEVICE_IMAGE_XPS = 11;

        /// IGR_DEVICE_IMAGE_SVG -> 12
        public const int IGR_DEVICE_IMAGE_SVG = 12;

        /// IGR_DEVICE_IMAGE_EPS -> 13
        public const int IGR_DEVICE_IMAGE_EPS = 13;

        /// IGR_DEVICE_IMAGE_PS -> 14
        public const int IGR_DEVICE_IMAGE_PS = 14;

        /// IGR_DEVICE_XML -> 5
        public const int IGR_DEVICE_XML = 5;

        /// IGR_DEVICE_HTML -> 6
        public const int IGR_DEVICE_HTML = 6;

        /// IGR_TEXT_ALIGN_LEFT -> 0x00
        public const int IGR_TEXT_ALIGN_LEFT = 0;

        /// IGR_TEXT_ALIGN_RIGHT -> 0x01
        public const int IGR_TEXT_ALIGN_RIGHT = 1;

        /// IGR_TEXT_ALIGN_CENTER -> 0x02
        public const int IGR_TEXT_ALIGN_CENTER = 2;

        /// IGR_TEXT_ALIGN_TOP -> 0x10
        public const int IGR_TEXT_ALIGN_TOP = 16;

        /// IGR_TEXT_ALIGN_MIDDLE -> 0x11
        public const int IGR_TEXT_ALIGN_MIDDLE = 17;

        /// IGR_TEXT_ALIGN_BOTTOM -> 0x12
        public const int IGR_TEXT_ALIGN_BOTTOM = 18;

        /// IGR_TEXT_STYLE_BOLD -> 0x01
        public const int IGR_TEXT_STYLE_BOLD = 1;

        /// IGR_TEXT_STYLE_ITALIC -> 0x02
        public const int IGR_TEXT_STYLE_ITALIC = 2;

        /// IGR_TEXT_STYLE_UNDERLINE -> 0x04
        public const int IGR_TEXT_STYLE_UNDERLINE = 4;

        /// IGR_BRUSH_NONE -> 0x00
        public const int IGR_BRUSH_NONE = 0;

        /// IGR_BRUSH_SOLID -> 0x01
        public const int IGR_BRUSH_SOLID = 1;

        /// IGR_PEN_NONE -> 0x00
        public const int IGR_PEN_NONE = 0;

        /// IGR_PEN_SOLID -> 0x01
        public const int IGR_PEN_SOLID = 1;

        /// IGR_ACTION_GET_STREAM_PART -> 10
        public const int IGR_ACTION_GET_STREAM_PART = 10;

        /// Warning: Generation of Method Macros is not supported at this time
        /// MAKE_COLOR -> "(R,G,B) ((int) R | ((int) G << 8) | ((int) B << 16))"
        public const string MAKE_COLOR = "(R,G,B) ((int) R | ((int) G << 8) | ((int) B << 16))";
    }


    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct Error_Control_Block
    {

        /// char[238]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 238)]
        public string Msg;

        /// SHORT->short
        public short MsgId;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct Instance_Status_Block
    {

        /// char[16]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string DLL_Version;

        /// SHORT->short
        public short RReserved;

        /// SHORT->short
        public short License_Type;

        /// char[40]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 40)]
        public string Licensee_ID1;

        /// char[40]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 40)]
        public string Licensee_ID2;
    }

    public delegate int IGR_Get_Entry(int handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);
    public delegate int IGR_Extract_File(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, [In()] [MarshalAs(UnmanagedType.LPWStr)] string destination, ref Error_Control_Block error);
    public delegate int IGR_Extract_Stream(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, ref System.IntPtr stream, ref Error_Control_Block error);

    /// Return Type: ULONG->unsigned int
    ///stream: IGR_Stream*
    ///
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate UInt32 IGR_Stream_Open(IntPtr stream);

    /// Return Type: ULONG->unsigned int
    ///stream: IGR_Stream*
    ///buffer: void*
    ///bufferSize: ULONG->unsigned int
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate UInt32 IGR_Stream_Read(IntPtr stream, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex=2), Out] byte[] buffer, UInt32 bufferSize);

    /// Return Type: int
    ///stream: IGR_Stream*
    ///offset: int
    ///whence: ULONG->unsigned int
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate Int64 IGR_Stream_Seek(IntPtr stream, Int64 offset, uint whence);

    /// Return Type: void
    ///stream: IGR_Stream*
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void IGR_Stream_Close(IntPtr stream);

    /// Return Type: ULONG->unsigned int
    ///stream: IGR_Writable_Stream*
    ///buffer: void*
    ///bufferSize: ULONG->unsigned int
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate UInt32 IGR_Writable_Stream_Write(IntPtr stream, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2), In] byte[] buffer, UInt32 bufferSize);


    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Stream
    {

        /// IGR_Stream_Open
        public IGR_Stream_Open Open;

        /// IGR_Stream_Read
        public IGR_Stream_Read Read;

        /// IGR_Stream_Seek
        public IGR_Stream_Seek Seek;

        /// IGR_Stream_Close
        public IGR_Stream_Close Close;
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Writable_Stream
    {

        /// IGR_Stream
        public IGR_Stream @base;

        /// IGR_Writable_Stream_Write
        public IGR_Writable_Stream_Write Write;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_Page_Word
    {

        /// int
        public int x;

        /// int
        public int y;

        /// int
        public int width;

        /// int
        public int height;

        /// int
        public int style;

        /// WCHAR*
        //[MarshalAs(UnmanagedType.LPWStr)]
        public IntPtr _word;

        /// int
        public int wordLength;

        /// int
        public int charoffset;

        /// int[8]
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

    /// Return Type: LONG->int
    ///actionID: int
    ///actionData: void*
    ///context: void*
    public delegate int IGR_CALLBACK(int actionID, System.IntPtr actionData, System.IntPtr context);

    [StructLayout(LayoutKind.Sequential)]
    public struct IGR_T_ACTION_GET_STREAM_PART
    {

        /// unsigned int
        public uint size;

        /// wchar_t*
        [MarshalAs(UnmanagedType.LPWStr)]
        public string partName;

        /// wchar_t*
        [MarshalAs(UnmanagedType.LPWStr)]
        public string partFullName;

        /// int
        public int partIndex;

        /// IGR_Stream*
        public System.IntPtr istr;
    }

    public partial class ISYS11df
    {

        /// Return Type: void
        ///reserved: LONG->int
        ///path: char*
        ///init: Instance_Status_Block*
        ///handle: SHORT*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Init_Instance")]
        public static extern void Init_Instance(int reserved, [In()] [MarshalAs(UnmanagedType.LPStr)] string path, ref Instance_Status_Block init, ref short handle, ref Error_Control_Block error);


        /// Return Type: void
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Close_Instance")]
        public static extern void Close_Instance(ref Error_Control_Block error);


        /// Return Type: void
        ///handle: SHORT->short
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Switch_Instance")]
        public static extern void Switch_Instance(short handle, ref Error_Control_Block error);


        /// Return Type: void
        ///source: LPCWSTR->WCHAR*
        ///buffer: LPSTR->CHAR*
        ///bufferSize: LONG->int
        [DllImport("ISYS11df", EntryPoint = "Widechar_to_UTF8")]
        public static extern void Widechar_to_UTF8([In()] [MarshalAs(UnmanagedType.LPWStr)] string source, [MarshalAs(UnmanagedType.LPStr)] System.Text.StringBuilder buffer, int bufferSize);


        /// Return Type: void
        ///source: LPCWSTR->WCHAR*
        ///sourceLen: LONG->int
        ///buffer: LPSTR->CHAR*
        ///bufferSize: LONG->int
        [DllImport("ISYS11df", EntryPoint = "Widechar_to_UTF8_Ex")]
        public static extern void Widechar_to_UTF8_Ex([In()] [MarshalAs(UnmanagedType.LPWStr)] string source, int sourceLen, [MarshalAs(UnmanagedType.LPStr)] System.Text.StringBuilder buffer, int bufferSize);


        /// Return Type: void
        ///source: LPCSTR->CHAR*
        ///buffer: LPWSTR->WCHAR*
        ///bufferSize: LONG->int
        [DllImport("ISYS11df", EntryPoint = "UTF8_to_Widechar")]
        public static extern void UTF8_to_Widechar([In()] [MarshalAs(UnmanagedType.LPStr)] string source, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, int bufferSize);


        /// Return Type: void
        ///source: LPCSTR->CHAR*
        ///sourceLen: LONG->int
        ///buffer: LPWSTR->WCHAR*
        ///bufferSize: LONG->int
        [DllImport("ISYS11df", EntryPoint = "UTF8_to_Widechar_Ex")]
        public static extern void UTF8_to_Widechar_Ex([In()] [MarshalAs(UnmanagedType.LPStr)] string source, int sourceLen, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, int bufferSize);


        /// Return Type: void
        ///param0: LONG->int
        ///param1: LONG->int
        ///param2: LPCSTR->CHAR*
        [DllImport("ISYS11df", EntryPoint = "Set_Engine_Call_Tracing")]
        public static extern void Set_Engine_Call_Tracing(int param0, int param1, [In()] [MarshalAs(UnmanagedType.LPStr)] string param2);


        /// Return Type: void
        ///param0: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Set_API_Charset")]
        public static extern void Set_API_Charset(int param0, ref Error_Control_Block error);


        /// Return Type: void
        ///param0: LPCSTR->CHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "Set_DLL_Option")]
        public static extern void Set_DLL_Option([In()] [MarshalAs(UnmanagedType.LPStr)] string param0, ref Error_Control_Block error);


        /// Return Type: void
        ///param0: LONG->int
        ///param1: size_t*
        ///param2: size_t*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "ISYS_Multiplex")]
        public static extern void ISYS_Multiplex(int param0, ref uint param1, ref uint param2, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_File_Type")]
        public static extern int IGR_Get_File_Type([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, ref int capabilities, ref int filetype, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Stream_Type")]
        public static extern int IGR_Get_Stream_Type(IntPtr stream, ref int capabilities, ref int filetype, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///flags: LONG->int
        ///capabilities: LONG*
        ///filetype: LONG*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_File")]
        public static extern int IGR_Open_File([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, int flags, ref int capabilities, ref int filetype, ref int handle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///flags: LONG->int
        ///capabilities: LONG*
        ///filetype: LONG*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_Stream")]
        public static extern int IGR_Open_Stream(IntPtr stream, int flags, ref int capabilities, ref int filetype, ref int handle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///flags: LONG->int
        ///options: LPCWSTR->WCHAR*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_File_Ex")]
        public static extern int IGR_Open_File_Ex([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, int flags, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref int capabilities, ref int filetype, ref int handle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///flags: LONG->int
        ///options: LPCWSTR->WCHAR*
        ///capabilities: LONG*
        ///filetype: LONG*
        ///handle: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_Stream_Ex")]
        public static extern int IGR_Open_Stream_Ex(IntPtr stream, int flags, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref int capabilities, ref int filetype, ref int handle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///buffer: LPWSTR->WCHAR*
        ///bufferSize: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Text")]
        public static extern int IGR_Get_Text(int handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, ref int bufferSize, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPWSTR->WCHAR*
        ///name: LPWSTR->WCHAR*
        ///date: PLONGLONG->LONGLONG*
        ///size: PLONGLONG->LONGLONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Subfile_Entry")]
        public static extern int IGR_Get_Subfile_Entry(int handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPCWSTR->WCHAR*
        ///destination: LPCWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Subfile")]
        public static extern int IGR_Extract_Subfile(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, [In()] [MarshalAs(UnmanagedType.LPWStr)] string destination, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPCWSTR->WCHAR*
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Subfile_Stream")]
        public static extern int IGR_Extract_Subfile_Stream(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, ref System.IntPtr stream, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Close_File")]
        public static extern int IGR_Close_File(int handle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///function: LONG->int
        ///param1: size_t*
        ///param2: size_t*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Multiplex")]
        public static extern int IGR_Multiplex(int function, ref uint param1, ref uint param2, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///flags: LONG->int
        ///options: LPCWSTR->WCHAR*
        ///output: LPCWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Convert_File")]
        public static extern int IGR_Convert_File([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, int flags, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, [In()] [MarshalAs(UnmanagedType.LPWStr)] string output, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///filename: LPCWSTR->WCHAR*
        ///flags: LONG->int
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_Stream_From_File")]
        public static extern int IGR_Make_Stream_From_File([In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, int flags, ref System.IntPtr stream, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///buffer: void*
        ///bufferSize: size_t->unsigned int
        ///destruct: void*
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_Stream_From_Memory")]
        public static extern int IGR_Make_Stream_From_Memory(System.IntPtr buffer, [MarshalAs(UnmanagedType.SysUInt)] IntPtr bufferSize, System.IntPtr destruct, ref System.IntPtr stream, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pStream: IGR_Stream*
        ///cb: IGR_CALLBACK
        ///context: void*
        ///ppExtStream: IGR_Stream**
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extend_Stream")]
        public static extern int IGR_Extend_Stream(IntPtr stream, IGR_CALLBACK cb, System.IntPtr context, ref System.IntPtr ppExtStream, ref Error_Control_Block error);


        /// Return Type: size_t->unsigned int
        ///buffer: LPSTR->CHAR*
        ///len: size_t->unsigned int
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Memory_Status")]
        [return: MarshalAs(UnmanagedType.SysUInt)]
        public static extern uint IGR_Get_Memory_Status([MarshalAs(UnmanagedType.LPStr)] System.Text.StringBuilder buffer, [MarshalAs(UnmanagedType.SysUInt)] uint len);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPWSTR->WCHAR*
        ///name: LPWSTR->WCHAR*
        ///date: PLONGLONG->LONGLONG*
        ///size: PLONGLONG->LONGLONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Image_Entry")]
        public static extern int IGR_Get_Image_Entry(int handle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPCWSTR->WCHAR*
        ///destination: LPCWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Image")]
        public static extern int IGR_Extract_Image(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, [In()] [MarshalAs(UnmanagedType.LPWStr)] string destination, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///handle: LONG->int
        ///id: LPCWSTR->WCHAR*
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Image_Stream")]
        public static extern int IGR_Extract_Image_Stream(int handle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, ref System.IntPtr stream, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///dest: LPWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Calculate_MD5")]
        public static extern int IGR_Calculate_MD5(IntPtr stream, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder dest, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///stream: PIGR_Stream->IGR_Stream*
        ///dest: LPWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Calculate_SHA1")]
        public static extern int IGR_Calculate_SHA1(IntPtr stream, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder dest, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///docHandle: LONG->int
        ///pageCount: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Count")]
        public static extern int IGR_Get_Page_Count(int docHandle, ref int pageCount, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///docHandle: LONG->int
        ///pageIndex: LONG->int
        ///pageHandle: HPAGE*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Open_Page")]
        public static extern int IGR_Open_Page(int docHandle, int pageIndex, ref int pageHandle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Close_Page")]
        public static extern int IGR_Close_Page(int pageHandle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///wordCount: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Word_Count")]
        public static extern int IGR_Get_Page_Word_Count(int pageHandle, ref int wordCount, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///index: LONG->int
        ///count: LONG*
        ///words: IGR_Page_Word*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Words")]
        public static extern int IGR_Get_Page_Words(int pageHandle, int index, ref int count, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2), Out] IGR_Page_Word[] words, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///width: LONG*
        ///height: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Dimensions")]
        public static extern int IGR_Get_Page_Dimensions(int pageHandle, ref int width, ref int height, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///buffer: WCHAR*
        ///size: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Text")]
        public static extern int IGR_Get_Page_Text(int pageHandle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, ref int size, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///id: LPWSTR->WCHAR*
        ///name: LPWSTR->WCHAR*
        ///date: PLONGLONG->LONGLONG*
        ///size: PLONGLONG->LONGLONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Image_Entry")]
        public static extern int IGR_Get_Page_Image_Entry(int pageHandle, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder id, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder name, ref long date, ref long size, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///id: LPCWSTR->WCHAR*
        ///destination: LPCWSTR->WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Page_Image")]
        public static extern int IGR_Extract_Page_Image(int pageHandle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, [In()] [MarshalAs(UnmanagedType.LPWStr)] string destination, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///pageHandle: HPAGE->LONG->int
        ///id: LPCWSTR->WCHAR*
        ///stream: PIGR_Stream*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Extract_Page_Image_Stream")]
        public static extern int IGR_Extract_Page_Image_Stream(int pageHandle, [In()] [MarshalAs(UnmanagedType.LPWStr)] string id, ref System.IntPtr stream, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///type: LONG->int
        ///filename: WCHAR*
        ///options: WCHAR*
        ///canvasHandle: HCANVAS*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_Output_Canvas")]
        public static extern int IGR_Make_Output_Canvas(int type, [In()] [MarshalAs(UnmanagedType.LPWStr)] string filename, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref int canvasHandle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///type: LONG->int
        ///stream: IGR_Writable_Stream*
        ///options: WCHAR*
        ///canvasHandle: HCANVAS*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Make_Output_Canvas_On")]
        public static extern int IGR_Make_Output_Canvas_On(int type, IntPtr stream, [In()] [MarshalAs(UnmanagedType.LPWStr)] string options, ref int canvasHandle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvasHandle: HCANVAS->LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Close_Canvas")]
        public static extern int IGR_Close_Canvas(int canvasHandle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///page: HPAGE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Render_Page")]
        public static extern int IGR_Render_Page(int page, int canvas, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///page: HPAGE->LONG->int
        ///From: LONG->int
        ///To: LONG->int
        ///Type: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Redact_Page_Text")]
        public static extern int IGR_Redact_Page_Text(int page, int From, int To, int Type, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///page: HPAGE->LONG->int
        ///name: WCHAR*
        ///buffer: WCHAR*
        ///size: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Page_Attribute")]
        public static extern int IGR_Get_Page_Attribute(int page, [In()] [MarshalAs(UnmanagedType.LPWStr)] string name, [MarshalAs(UnmanagedType.LPWStr)] System.Text.StringBuilder buffer, ref int size, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///formatid: LONG->int
        ///what: LONG->int
        ///buffer: char*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Get_Format_Attribute", CharSet = CharSet.Ansi)]
        public static extern int IGR_Get_Format_Attribute(int formatid, int what, StringBuilder buffer, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
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


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///radius: LONG->int
        ///startAngle: LONG->int
        ///sweepAngle: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_AngleArc")]
        public static extern int IGR_Canvas_AngleArc(int canvas, int x, int y, int radius, int startAngle, int sweepAngle, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
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


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Ellipse")]
        public static extern int IGR_Canvas_Ellipse(int canvas, int x, int y, int x2, int y2, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Rect")]
        public static extern int IGR_Canvas_Rect(int canvas, int x, int y, int x2, int y2, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_LineTo")]
        public static extern int IGR_Canvas_LineTo(int canvas, int x, int y, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_MoveTo")]
        public static extern int IGR_Canvas_MoveTo(int canvas, int x, int y, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
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


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///x2: LONG->int
        ///y2: LONG->int
        ///radius: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_RoundRect")]
        public static extern int IGR_Canvas_RoundRect(int canvas, int x, int y, int x2, int y2, int radius, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///text: WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_TextOut")]
        public static extern int IGR_Canvas_TextOut(int canvas, int x, int y, [In()] [MarshalAs(UnmanagedType.LPWStr)] string text, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
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


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///text: WCHAR*
        ///width: LONG*
        ///height: LONG*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_MeasureText")]
        public static extern int IGR_Canvas_MeasureText(int canvas, [In()] [MarshalAs(UnmanagedType.LPWStr)] string text, ref int width, ref int height, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///color: LONG->int
        ///width: LONG->int
        ///style: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_SetPen")]
        public static extern int IGR_Canvas_SetPen(int canvas, int color, int width, int style, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///color: LONG->int
        ///style: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_SetBrush")]
        public static extern int IGR_Canvas_SetBrush(int canvas, int color, int style, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///fontFamily: WCHAR*
        ///size: LONG->int
        ///style: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_SetFont")]
        public static extern int IGR_Canvas_SetFont(int canvas, [In()] [MarshalAs(UnmanagedType.LPWStr)] string fontFamily, int size, int style, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///opacity: BYTE->unsigned char
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_SetOpacity")]
        public static extern int IGR_Canvas_SetOpacity(int canvas, byte opacity, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///x: LONG->int
        ///y: LONG->int
        ///imagedata: void*
        ///imagesize: size_t->unsigned int
        ///mimetype: WCHAR*
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_DrawImage")]
        public static extern int IGR_Canvas_DrawImage(int canvas, int x, int y, Byte[] imagedata, [MarshalAs(UnmanagedType.SysUInt)] IntPtr imagesize, [In()] [MarshalAs(UnmanagedType.LPWStr)] string mimetype, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
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


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///degrees: LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Rotation")]
        public static extern int IGR_Canvas_Rotation(int canvas, int degrees, ref Error_Control_Block error);


        /// Return Type: IGR_RETURN_CODE->LONG->int
        ///canvas: HCANVAS->LONG->int
        ///error: Error_Control_Block*
        [DllImport("ISYS11df", EntryPoint = "IGR_Canvas_Reset")]
        public static extern int IGR_Canvas_Reset(int canvas, ref Error_Control_Block error);
    }
}
