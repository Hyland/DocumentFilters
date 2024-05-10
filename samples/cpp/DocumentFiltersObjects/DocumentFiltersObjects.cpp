/*
(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved.

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
* Document Filters 11 C++ objects & helper functions
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"

#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <cstdio>
#ifdef PERCEPTIVE_PLATFORM_WINDOWS
#include <io.h>     // setmode()
#include <fcntl.h>  // O_BINARY
#include <direct.h> // _mkdir()
#include <stdio.h>  // _wfopen_s
#pragma warning(disable: 4996)
#else
#include <unistd.h> // usleep()
#endif

extern "C" {
	typedef LONG (IGR_EXPORT * IGR_Get_Text_Ptr)(LONG handle, LPWSTR buffer, LONG* bufferSize, Error_Control_Block* error);
}

namespace Perceptive
{
#ifdef PERCEPTIVE_PLATFORM_WINDOWS
	const char PathDelim = '\\';
	const char* NewLine = "\r\n";
	const wchar_t* NewLineW = L"\r\n";
#else
	const char PathDelim = '/';
	const char* NewLine = "\n";
	const wchar_t* NewLineW = L"\n";
#endif

#if defined(__APPLE__) || defined(__hpux) || defined(__pseries) || defined(__FreeBSD__)
#define ftello64 ftello
#define fseeko64 fseeko
#define fopen64 fopen
#endif

#define ENC_JAPANESE            "Shift-JIS"
#define ENC_CHINESE_SIMPLIFIED  "GBK"
#define ENC_KOREAN              "ISO-2022-KR"
#define ENC_CHINESE_TRADITIONAL "Big5"
#define ENC_CYRILLIC            "KOI8-R"
#define ENC_LATIN_1             "Latin1"
#define ENC_GREEK               "ISO-8859-7"
#define ENC_TURKISH             "ISO-8859-9"
#define ENC_HEBREW              "Windows-1255"
#define ENC_ARABIC              "ISO-8859-6"
#define ENC_UTF_8               "UTF-8"

	namespace
	{
		int igr_check(int return_code, const char* method, const Error_Control_Block& error)
		{
			if (return_code != IGR_OK && return_code != IGR_NO_MORE)
			{
				std::string text = std::string(method) + ": " + GetErrorStr(return_code);
				if (error.Msg[0])
					text += ", " + std::string(error.Msg);
				
				throw Exception(text);
			}
			return return_code;
		}
		template <typename _string_type> struct strip_traits
		{
			_string_type space;
			_string_type new_line;
			_string_type tab;
		};
		template <typename _string_type, typename _trails_string_type> _string_type strip_control_chars(const _string_type& val, const strip_traits<_trails_string_type>& traits)
		{
			struct Helper
			{
			public:
				static void append(_string_type& dest, const _trails_string_type& val)
				{
					for (typename _trails_string_type::const_iterator i = val.begin(); i != val.end(); ++i)
						dest.push_back(*i);
				}
			};

			_string_type retval;
			retval.reserve(val.size() * 2);
			for (typename _string_type::const_iterator i = val.begin(); i != val.end(); ++i)
			{
				switch (*i)
				{
				case IGR_CHAR_SOFT_SPACE:
					Helper::append(retval, traits.space);
					break;
				case IGR_CHAR_STYLE_NORMAL:
				case IGR_CHAR_STYLE_BOLD:
				case IGR_CHAR_STYLE_ITALICS:
				case IGR_CHAR_STYLE_UNDERLINED:
				case IGR_CHAR_START_META:
				case IGR_CHAR_END_META:
					break;
				case IGR_CHAR_TAB:
					Helper::append(retval, traits.tab);
					break;
				case IGR_CHAR_LINE_BREAK:
					Helper::append(retval, traits.new_line);
					break;
				case IGR_CHAR_PAGE_BREAK:
				case IGR_CHAR_PARA_BREAK:
					Helper::append(retval, traits.new_line + traits.new_line);
					break;
				default:
					retval.push_back(*i);
					break;
				}
			}
			return retval;
		}
		template <typename _string_type, typename _retval_type> _retval_type parse_metadata(const _string_type& val)
		{
			// ISYS Metadata format:
			// Name1: Value1<#14>Name2: Value2<#14>

			_retval_type retval;
			

			bool in_name = true;
			_string_type name;
			_string_type value;
			unsigned int i = 0;
			for (typename _string_type::size_type i = 0; i < val.size(); )
			{
				typename _string_type::value_type Ch = val[i];
				switch (Ch)
				{
				case ':': // Name & Value separator
				{
					if (in_name && i < val.size() - 1 && val[i + 1] == ' ')
					{
						in_name = false;
						i += 2;
					}
					else if (in_name)
					{
						name += Ch;
						i++;
					}
					else
					{
						value += Ch;
						i++;
					}
					break;
				}
				case IGR_CHAR_PARA_BREAK:
				{
					if (!name.empty() && !value.empty())
						retval[name] = value;
					in_name = true;
					name.clear();
					value.clear();
					i++;
					break;
				}
				default:
				{
					if (in_name)
						name += Ch;
					else
						value += Ch;
					i++;
					break;
				}
				}
			}
			if (!name.empty() && !value.empty())
				retval[name] = value;

			return retval;
		}
		FILE *fopen_utf8(const char *filename, const char *mode)
		{
			FILE* F;
#if defined(PERCEPTIVE_PLATFORM_WINDOWS)
			// Convert UTF8 filename to wide chars on Windows.
			errno_t fileErr = _wfopen_s(&F, _UCS2(filename), _UCS2(mode));
			if (fileErr) F = NULL;
#else
			F = fopen(filename, mode);
#endif
			return F;
		}

		template <typename _string_type> _string_type extract_text(
			LONG handle, 
			size_t max_length, 
			IGR_Get_Text_Ptr extract,
			const char* method_name,
			_string_type(*string_convert)(const WCHAR*, const int),
			bool& eof,
			LONG& char_count)
		{
			_string_type retval;
			Error_Control_Block ecb;

			std::vector<WCHAR> buffer(max_length);

			LONG buffer_size = (LONG)(buffer.size() - 1);
			if (!buffer.empty())
			{
				int rc = igr_check(extract(handle, &buffer[0], &buffer_size, &ecb), method_name, ecb);
				eof = (rc == IGR_NO_MORE);
				if (buffer_size > 0)
				{
					retval = string_convert(&buffer[0], buffer_size);
					char_count += buffer_size;
				}
			}
			return retval;
		}
	}

	std::wstring ToWideStr(const WCHAR *S, const int NumChars)
	{
		std::wstring Result = L"";
		Result.reserve(NumChars);
		for (int i = 0; i < NumChars; i++)
		{
			wchar_t Ch = S[i];
			Result.append(1, Ch);
		}
		return Result;
	}

	std::vector<WCHAR> ToUTF16(const WCHAR *S, const int NumChars)
	{
		return std::vector<WCHAR>(S, S+NumChars);
	}


	std::string ToUTF8Str(const WCHAR *S, const int num_chars)
	{
		std::string retval = "";
		
		int buffer_len = 1;
		if (num_chars < 0)
		{
			for (int i = 0; S[i]; i++)
				buffer_len++;
		}
		else
			buffer_len = num_chars + 1;


		std::vector<char> buffer(buffer_len * 4);
		if (!buffer.empty())
		{
			if (num_chars < 0)
				Widechar_to_UTF8(S, &buffer[0], buffer_len * 4);
			else
				Widechar_to_UTF8_Ex(S, num_chars, &buffer[0], buffer_len * 4);
			retval.assign((const char *)&buffer[0]);
		}
		return retval;
	}

	std::string ToWCHARStr(const std::string &text)
	{
		std::string retval = "";
		int buffer_len = int(text.length()) + 1;
		std::vector<WCHAR> buffer(buffer_len);
		if (!buffer.empty())
		{
			UTF8_to_Widechar_Ex(text.c_str(), (LONG)text.size(), &buffer[0], buffer_len);
			retval.assign((const char *)&buffer[0], buffer_len * sizeof(WCHAR));
		}
		return retval;
	}

	std::string ToWCHARStr(const std::wstring &S)
	{
		std::string retval = "";
		int buffer_len = int(S.length()) + 1;
		std::vector<WCHAR> buffer(buffer_len);
		if (!buffer.empty())
		{
			int i = 0;
			while (i < buffer_len)
			{
				buffer[i] = S[i];
				i++;
			}
			buffer[i] = 0;
			retval.assign((const char *)&buffer[0], buffer_len * sizeof(WCHAR));
		}
		return retval;
	}

	/****************************************************************************
	* Perceptive::Stream
	****************************************************************************/
	struct StreamBridge 
	{
		struct IGR_Writable_Stream m_methods;
		Stream* m_inner;

		StreamBridge(Stream* s)
			: m_inner(s)
		{
			m_methods.base.Open = _Open;
			m_methods.base.Close = _Close;
			m_methods.base.Read = _Read;
			m_methods.base.Seek = _Seek;
			m_methods.Write = _Write;
		}
		static ULONG CALLBACK _Write(struct IGR_Writable_Stream* stream, void* buffer, ULONG bufferSize)
		{
			return ((StreamBridge*)stream)->m_inner->Write(buffer, bufferSize);
		}
		static ULONG CALLBACK _Open(struct IGR_Stream* stream)
		{
			return ((StreamBridge*)stream)->m_inner->Open();
		}
		static ULONG CALLBACK _Read(struct IGR_Stream* stream, void* buffer, ULONG bufferSize)
		{
			return ((StreamBridge*)stream)->m_inner->Read(buffer, bufferSize);
		}
		static LONGLONG CALLBACK _Seek(struct IGR_Stream* stream, LONGLONG offset, ULONG whence)
		{
			return ((StreamBridge*)stream)->m_inner->Seek(offset, whence);
		}
		static void CALLBACK _Close(struct IGR_Stream* stream)
		{
			((StreamBridge*)stream)->m_inner->Close();
			delete ((StreamBridge*)stream);
		}
	};

	Stream::Stream()
		: m_proxy(NULL)
	{
		m_proxy = new StreamBridge(this);
	}
	Stream::~Stream()
	{
		Close();
		delete m_proxy;
	}
	ULONG Stream::Write(void* buffer, ULONG bufferSize)
	{
		return 0;
	}
	void Stream::Close()
	{
	}

	IGR_Stream* Stream::getIGRStream() const
	{
		return (IGR_Stream*) m_proxy;
	}

	FileStream::FileStream(const std::string &filename, const std::string& Mode)
		: m_file(NULL)
	{
#ifdef _WIN32
		m_file = fopen(filename.c_str(), Mode.c_str());
#else
		m_file = fopen64(filename.c_str(), Mode.c_str());
#endif
		if (m_file == NULL)
			throw Exception("FileStream Error - could not open file");
	}
	FileStream::~FileStream()
	{
		Close();
	}
	ULONG FileStream::Open()
	{
		return 0;
	}
	ULONG FileStream::Read(void* buffer, ULONG bufferSize)
	{
		if (m_file)
			return (ULONG) fread(buffer, 1, bufferSize, m_file);
		else
			return 0;
	}
	ULONG FileStream::Write(void* buffer, ULONG bufferSize)
	{
		if (m_file)
			return (ULONG) fwrite(buffer, 1, bufferSize, m_file);
		else
			return 0;
	}
	LONGLONG FileStream::Seek(LONGLONG offset, ULONG whence)
	{
		if (m_file)
		{
#ifdef _WIN32
			_fseeki64(m_file, offset, whence);
			return _ftelli64(m_file);
#else
			fseeko64(m_file, offset, whence);
			return ftello64(m_file);
#endif

		}
		else
			return 0;
	}
	void FileStream::Close()
	{
		if (m_file)
			fclose(m_file);
		m_file = NULL;
	}

	IGRStream::IGRStream()
		: m_stream(NULL)
	{
	}
	IGRStream::IGRStream(IGR_Stream* s)
		: m_stream(s)
	{
	}
	ULONG IGRStream::Open()
	{
		return m_stream->Open(m_stream);
	}
	ULONG IGRStream::Read(void* buffer, ULONG bufferSize)
	{
		return m_stream->Read(m_stream, buffer, bufferSize);
	}
	ULONG IGRStream::Write(void* buffer, ULONG bufferSize)
	{
		return ((IGR_Writable_Stream*)m_stream)->Write(((IGR_Writable_Stream*)m_stream), buffer, bufferSize);
	}
	LONGLONG IGRStream::Seek(LONGLONG offset, ULONG whence)
	{
		return m_stream->Seek(m_stream, offset, whence);
	}
	void IGRStream::Close()
	{
		if (m_stream != NULL)
		{
			m_stream->Close(m_stream);
			m_stream = NULL;
		}
	}
	IGR_Stream* IGRStream::getIGRStream() const
	{
		return m_stream;
	}

	MemStream::MemStream(void *Data, LONG DataSize)
	{
		IGR_Stream* innerStream;
		Error_Control_Block ecb;
		
		igr_check(IGR_Make_Stream_From_Memory(Data, DataSize, NULL, &innerStream, &ecb), "IGR_Make_Stream_From_Memory", ecb);

		m_stream = innerStream;
	}

	/****************************************************************************
	* Perceptive::MemOutputStream
	****************************************************************************/

	MemOutputStream::MemOutputStream()
	{
		// Nothing to do here...
	}

	MemOutputStream::~MemOutputStream()
	{
		// Nothing to do here...
	}

	ULONG MemOutputStream::Open()
	{
		Reset();
		return IGR_OK;
	}

	ULONG MemOutputStream::Read(void* buffer, ULONG bufferSize)
	{
		ULONG bytes_read = 0;
		size_t i = 0;
		BYTE *ByteBuffer = (BYTE *)buffer;
		while ((bytes_read < bufferSize) && (m_data_pos < m_data.size()))
		{
			ByteBuffer[i++] = m_data[m_data_pos++];
			bytes_read++;
		}
		return bytes_read;
	}

	ULONG MemOutputStream::Write(void* buffer, ULONG bufferSize)
	{
		ULONG bytes_written = 0;
		size_t i = 0;
		BYTE *ByteBuffer = (BYTE *)buffer;
		if (m_data_pos + bufferSize > m_data.size())
			m_data.resize(m_data_pos + bufferSize);
		while (bytes_written < bufferSize)
		{
			m_data[m_data_pos++] = ByteBuffer[i++];
			bytes_written++;
		}
		return bytes_written;
	}

	LONGLONG MemOutputStream::Seek(LONGLONG offset, ULONG whence)
	{
		switch (whence)
		{
		case SEEK_SET:
			m_data_pos = (size_t)offset;
			break;
		case SEEK_CUR:
			m_data_pos += (size_t)offset;
			break;
		case SEEK_END:
			m_data_pos = m_data.size() + (size_t)offset;
			break;
		}
		return m_data_pos;
	}

	void MemOutputStream::Close()
	{
		Reset();
	}

	void MemOutputStream::Reset()
	{
		m_data.clear();
		m_data_pos = 0;
	}

	ULONG MemOutputStream::WriteToFile(FILE *f)
	{
		ULONG bytes_written = 0;
		for (size_t i = 0; i < m_data.size(); i++)
			bytes_written += (ULONG) fwrite(&m_data[i], 1, 1, f);
		return bytes_written;
	}

	ULONG MemOutputStream::SaveToFile(const std::string &filename)
	{
		ULONG bytes_written = 0;
		FILE *f = fopen(filename.c_str(), "wb");
		if (f)
		{
			bytes_written = WriteToFile(f);
			fclose(f);
		}
		return bytes_written;
	}

	/****************************************************************************
	* Perceptive::Extractor
	****************************************************************************/

	class Extractor::Impl
	{
	public:
		Stream *m_stream;
		LONG m_capabilities;
		LONG m_doc_type;
		LONG m_doc_handle;
		bool m_text_eof;
		LONG m_num_chars;
		LONG m_page_index;
		bool m_own_stream;
		Extractor::COnGetSubStream OnGetSubStream;

		Impl()
			: m_stream(NULL)
			, m_capabilities(0)
			, m_doc_type(0)
			, m_doc_handle(0)
			, m_text_eof(false)
			, m_num_chars(0)
			, m_page_index(0)
			, OnGetSubStream(NULL)
		{
		}
		~Impl()
		{
			if (m_stream)
			{
				m_stream->Close();
				if (m_own_stream)
					delete m_stream;
			}
		}
		template <typename _string_type> _string_type get_text(size_t max_length, _string_type (*conv)(const WCHAR*, const int))
		{
			return extract_text<_string_type>(m_doc_handle, max_length, IGR_Get_Text, "IGR_Get_Text", conv, m_text_eof, m_num_chars);
		}
	};

	Extractor::Extractor(Stream *stream, bool own_stream)
		: m_impl(new Impl())
	{
		setStream(stream, own_stream);
	}

	Extractor::~Extractor()
	{
		Close();
		delete m_impl;
	}

	LONG Extractor::needHandle()
	{
		Open();
		return getHandle();
	}

	IGR_Stream* Extractor::needStream()
	{
		if (m_impl->m_stream)
			return m_impl->m_stream->getIGRStream();
		return NULL;
	}

	void Extractor::setStream(Stream* stream, bool own_stream)
	{
		if (m_impl->m_stream && m_impl->m_own_stream)
			delete m_impl->m_stream;
		m_impl->m_stream = stream;
		m_impl->m_own_stream = own_stream;
	}

	LONG Extractor::getHandle() const
	{
		return m_impl->m_doc_handle;
	}

	void Extractor::Open(const int flags, const std::string &options)
	{
		if (m_impl->m_doc_handle == 0)
		{
			Error_Control_Block ecb;

			igr_check(IGR_Open_Stream_Ex(needStream(), flags, _UCS2(options), &m_impl->m_capabilities, &m_impl->m_doc_type, &m_impl->m_doc_handle, &ecb), "IGR_Open_Stream_Ex", ecb);
		}
	}

	int Extractor::getFileType()
	{
		if (m_impl->m_doc_type == 0)
		{
			Error_Control_Block ecb;
			if (IGR_Get_Stream_Type(needStream(), &m_impl->m_capabilities, &m_impl->m_doc_type, &ecb) != IGR_OK)
			{
				m_impl->m_capabilities = m_impl->m_doc_type = 0;
			}
		}
		return m_impl->m_doc_type;
	}

	std::string Extractor::getFileType(FileTypeWhat what)
	{
		Error_Control_Block ecb;

		std::vector<char> buffer(255);

		if (IGR_Get_Format_Attribute(getFileType(), what, &buffer[0], &ecb) == IGR_OK)
			return &buffer[0];
		
		return "Unknown";
	}

	LONG Extractor::getCapabilities()
	{
		(void)getFileType();
		return m_impl->m_capabilities;
	}

	bool Extractor::getSupportsText()
	{
		return (getCapabilities() & IGR_FILE_SUPPORTS_TEXT) != 0;
	}

	bool Extractor::getSupportsSubFiles()
	{
		return (getCapabilities() & IGR_FILE_SUPPORTS_SUBFILES) != 0;
	}

	bool Extractor::getSupportsHD()
	{
		return (getCapabilities() & IGR_FILE_SUPPORTS_HDHTML) != 0;
	}

	bool Extractor::getEOF()
	{
		return m_impl->m_text_eof;
	}

	std::string Extractor::GetText(const int max_length)
	{
		Open();

		return m_impl->get_text<std::string>(max_length, ToUTF8Str);
	}

	std::wstring Extractor::GetTextW(const int max_length)
	{
		Open();

		return m_impl->get_text<std::wstring>(max_length, ToWideStr);
	}

	std::vector<WCHAR> Extractor::GetTextUTF16(const int max_length)
	{
		Open();

		return m_impl->get_text< std::vector<WCHAR> >(max_length, ToUTF16);
	}

	std::string Extractor::PrepareText(const std::string &text)
	{
		strip_traits<std::string> traits = { " ", NewLine, "  " };
		
		return strip_control_chars(text, traits);
	}

	std::wstring Extractor::PrepareTextW(const std::wstring &text)
	{
		strip_traits<std::wstring> traits = { L" ", NewLineW, L"  " };

		return strip_control_chars(text, traits);
	}

	std::vector<WCHAR> Extractor::PrepareText(const std::vector<WCHAR> &text)
	{
		strip_traits<std::wstring> traits = { L" ", NewLineW, L"  " };
		return strip_control_chars(text, traits);
	}

	Metadata Extractor::ParseMetadata(const std::string &text)
	{
		return parse_metadata<std::string, Metadata>(text);
	}

	MetadataW Extractor::ParseMetadataW(const std::wstring &text)
	{
		return parse_metadata<std::wstring, MetadataW>(text);
	}

	SubFile *Extractor::GetFirstSubFile()
	{
		return GetNextSubFile();
	}

	SubFile *Extractor::GetNextSubFile()
	{
		Open();

		std::vector<WCHAR> id(4096);
		std::vector<WCHAR> name(1024);
		LONGLONG file_time;
		LONGLONG file_size;
		Error_Control_Block ecb;

		int rc = igr_check(IGR_Get_Subfile_Entry(m_impl->m_doc_handle, &id[0], &name[0], &file_time, &file_size, &ecb), "IGR_Get_Subfile_Entry", ecb);
		if (rc != IGR_OK)
			return NULL;

		return new SubFile(getHandle(), ToUTF8Str(&id[0], -1), ToUTF8Str(&name[0], -1), file_time, file_size, IGR_Extract_Subfile_Stream);
	}

	SubFile *Extractor::GetSubFile(const std::string &id)
	{
		Open();

		return new SubFile(getHandle(), id, "", 0, 0, IGR_Extract_Subfile_Stream);
	}

	SubFile *Extractor::GetFirstImage()
	{
		return GetNextImage();
	}

	SubFile *Extractor::GetNextImage()
	{
		Open();

		std::vector<WCHAR> id(4096);
		std::vector<WCHAR> name(1024);
		LONGLONG file_time;
		LONGLONG file_size;
		Error_Control_Block ecb;

		int rc = igr_check(IGR_Get_Image_Entry(m_impl->m_doc_handle, &id[0], &name[0], &file_time, &file_size, &ecb), "IGR_Get_Image_Entry", ecb);
		if (rc != IGR_OK)
			return NULL;

		return new SubFile(getHandle(), ToUTF8Str(&id[0], -1), ToUTF8Str(&name[0], -1), file_time, file_size, IGR_Extract_Image_Stream);
	}

	void Extractor::CopyTo(const std::string &filename)
	{
		const int FileBufferSize = 4096;
		std::vector<uint8_t> buffer(FileBufferSize);

		bool CopyOK = false;
		FILE *F = fopen_utf8(filename.c_str(), "wb");
		if (F)
		{
			IGR_Stream *stream = needStream();
			if (stream == NULL)
				throw Exception("Could not get stream");

			int64_t stream_pos = stream->Seek(stream, 0, SEEK_CUR);
			int64_t stream_size = stream->Seek(stream, 0, SEEK_END);
			int64_t bytes_written = 0, total_bytes_written = 0, bytes_read;
			
			stream->Seek(stream, 0, SEEK_SET);

			while ((bytes_read = stream->Read(stream, &buffer[0], FileBufferSize)) > 0)
			{
				bytes_written = fwrite(&buffer[0], 1, (size_t) bytes_read, F);
				total_bytes_written += bytes_written;

				if (bytes_written < bytes_read)
					break;
			}

			stream->Seek(stream, stream_pos, SEEK_SET);
			fclose(F);
			CopyOK = (total_bytes_written == stream_size);
		}
		if (!CopyOK)
			throw Exception("Cannot copy to: " + filename);
	}

	void Extractor::SaveTo(const std::string &filename)
	{
		Open();
		bool SaveOK = false;
		FILE *F = fopen_utf8(filename.c_str(), "wb");
		if (F)
		{
			int64_t bytes_extracted = 0;
			int64_t bytes_written = 0;
			while (!getEOF())
			{
				std::string text = GetText();
				bytes_extracted += text.length();
				bytes_written += fwrite(text.c_str(), 1, text.length(), F);
			}
			fclose(F);
			SaveOK = (bytes_written == bytes_extracted);
		}
		if (!SaveOK)
			throw Exception("Cannot save to: " + filename);
	}

	std::string Extractor::getHashMD5()
	{
		WCHAR strHexOut[PERCEPTIVE_SZ_MD5_HEX_LEN];
		Error_Control_Block ecb;
		igr_check(IGR_Calculate_MD5(m_impl->m_stream->getIGRStream(), strHexOut, &ecb), "IGR_Calculate_MD5", ecb);
		return ToUTF8Str(strHexOut, PERCEPTIVE_SZ_MD5_HEX_LEN - 1);
	}

	std::string Extractor::getHashSHA1()
	{
		WCHAR strHexOut[PERCEPTIVE_SZ_SHA1_HEX_LEN];
		Error_Control_Block ecb;
		igr_check(IGR_Calculate_SHA1(m_impl->m_stream->getIGRStream(), strHexOut, &ecb), "IGR_Calculate_SHA1", ecb);
		return ToUTF8Str(strHexOut, PERCEPTIVE_SZ_SHA1_HEX_LEN - 1);
	}

	void Extractor::Close()
	{
		if (m_impl->m_doc_handle > 0)
		{
			Error_Control_Block ecb;
			IGR_Close_File(m_impl->m_doc_handle, &ecb);
			m_impl->m_capabilities = 0;
			m_impl->m_doc_type = 0;
			m_impl->m_doc_handle = 0;
			m_impl->m_text_eof = false;
			m_impl->m_page_index = 0;
		}
	}

	Page *Extractor::GetFirstPage()
	{
		m_impl->m_page_index = 0;
		return GetPage(m_impl->m_page_index);
	}

	Page *Extractor::GetNextPage()
	{
		m_impl->m_page_index++;
		if (m_impl->m_page_index < getPageCount())
			return GetPage(m_impl->m_page_index);
		return NULL;
	}

	Page *Extractor::GetPage(const int PageIndex)
	{
		Open();
		HPAGE PageHandle = 0;
		Error_Control_Block ecb;

		igr_check(IGR_Open_Page(m_impl->m_doc_handle, PageIndex, &PageHandle, &ecb), "IGR_Open_Page", ecb);

		return new Page(PageHandle);
	}

	int Extractor::getPageCount()
	{
		Open();
		LONG PageCount = 0;
		Error_Control_Block ecb;
		igr_check(IGR_Get_Page_Count(m_impl->m_doc_handle, &PageCount, &ecb), "IGR_Get_Page_Count", ecb);
		return PageCount;
	}

	void Extractor::DumpStatistics(std::ostream &Stream)
	{
	}

	/****************************************************************************
	* Perceptive::SubFile
	****************************************************************************/
	class SubFile::Impl
	{
	public:
		std::string m_id;
		std::string m_name;
		LONGLONG m_time;
		LONGLONG m_size;
		
		LONG m_parent_handle;
		SubFile::CMethod m_method;

		Impl(LONG parent_handle, const std::string &id, const std::string &name, LONGLONG file_time, LONGLONG file_size, SubFile::CMethod method)
			: m_parent_handle(parent_handle)
			, m_method(method)
			, m_id(id)
			, m_name(name)
			, m_time(file_time)
			, m_size(file_size)
		{

		}
	};

	SubFile::SubFile(LONG parent_handle, const std::string &id, const std::string &name, LONGLONG file_time, LONGLONG file_size, CMethod method)
		: Extractor(NULL, true)
		, m_impl(new Impl(parent_handle, id, name, file_time, file_size, method))
	{
	}
	SubFile::~SubFile()
	{
		delete m_impl;
	}

	IGR_Stream* SubFile::needStream()
	{
		IGR_Stream* retval = Extractor::needStream();
		if (retval == NULL)
		{
			Error_Control_Block ecb;
			igr_check(m_impl->m_method(m_impl->m_parent_handle, _UCS2(m_impl->m_id), &retval, &ecb), "SubFile", ecb);
			Extractor::setStream(new IGRStream(retval), true);
		}
		return retval;
	}

	const std::string& SubFile::getID() const
	{
		return m_impl->m_id;
	}

	const std::string& SubFile::getName() const
	{
		return m_impl->m_name;
	}

	LONGLONG SubFile::getFileDate() const
	{
		return m_impl->m_time;
	}

	LONGLONG SubFile::getFileSize() const
	{
		return m_impl->m_size;
	}

	/****************************************************************************
	* Perceptive::Page
	****************************************************************************/

	class Page::Impl
	{
	public:
		HPAGE m_page_handle;
		bool m_text_eof;
		LONG m_num_chars;
		std::vector<IGR_Page_Word> m_words;
		LONG m_word_count;
		LONG m_word_index;
		
		Impl(HPAGE handle)
			: m_page_handle(handle)
			, m_text_eof(false)
			, m_num_chars(0)
			, m_words(0)
			, m_word_index(0)
		{
		}
		template <typename _string_type> _string_type get_text(size_t max_length, _string_type(*conv)(const WCHAR*, const int))
		{
			return extract_text<_string_type>(m_page_handle, max_length, IGR_Get_Page_Text, "IGR_Get_Page_Text", conv, m_text_eof, m_num_chars);
		}

	};

	Page::Page(HPAGE handle)
		: m_impl(new Impl(handle))
	{
	}

	Page::~Page()
	{
		Close();
		delete m_impl;
	}

	HPAGE Page::getHandle() const
	{
		return m_impl->m_page_handle;
	}

	void Page::Close()
	{
		Error_Control_Block ecb;
		IGR_Close_Page(m_impl->m_page_handle, &ecb);
		m_impl->m_page_handle = 0;
		m_impl->m_words.resize(0);
	}

	int Page::getWordCount()
	{
		LONG WordCount = 0;
		Error_Control_Block ecb;

		igr_check(IGR_Get_Page_Word_Count(m_impl->m_page_handle, &WordCount, &ecb), "IGR_Get_Page_Word_Count", ecb);

		return WordCount;
	}

	int Page::getWidth()
	{
		LONG Width;
		LONG Height;
		getDimensions(Width, Height);
		return Width;
	}

	int Page::getHeight()
	{
		LONG Width;
		LONG Height;
		getDimensions(Width, Height);
		return Height;
	}

	std::string Page::GetText(const int max_length)
	{
		return m_impl->get_text<std::string>(max_length, ToUTF8Str);
	}

	std::wstring Page::GetTextW(const int max_length)
	{
		return m_impl->get_text<std::wstring>(max_length, ToWideStr);
	}

	std::string Page::GetAttribute(const std::string &name)
	{
		Error_Control_Block ecb;
		std::vector<WCHAR> buffer(4096);
		LONG buffer_size = (LONG) buffer.size() - 1;

		igr_check(IGR_Get_Page_Attribute(m_impl->m_page_handle, _UCS2(name), &buffer[0], &buffer_size, &ecb), "IGR_Get_Page_Attribute", ecb);

		return ToUTF8Str(&buffer[0], buffer_size);
	}

	Word *Page::GetFirstWord()
	{
		if (m_impl->m_words.empty())
		{
			LONG WordCount = getWordCount();
			m_impl->m_words.resize(WordCount);
			if (!m_impl->m_words.empty())
			{
				Error_Control_Block ecb;
				
				LONG RC = IGR_Get_Page_Words(m_impl->m_page_handle, 0, &WordCount, &m_impl->m_words[0], &ecb);
				if (RC != IGR_OK)
					m_impl->m_words.resize(0);
				else
					m_impl->m_word_count = WordCount;
			}
		}
		m_impl->m_word_index = 0;
		return GetWord(m_impl->m_word_index);
	}

	Word *Page::GetNextWord()
	{
		return GetWord(++m_impl->m_word_index);
	}

	Word *Page::GetWord(const int WordIndex)
	{
		if (!m_impl->m_words.empty() && m_impl->m_word_index >= 0 && m_impl->m_word_index < m_impl->m_word_count)
			return new Word(&m_impl->m_words[m_impl->m_word_index], m_impl->m_word_index);
		return NULL;
	}

	SubFile *Page::GetFirstImage()
	{
		return GetNextImage();
	}

	SubFile *Page::GetNextImage()
	{
		std::vector<WCHAR> id(4096);
		std::vector<WCHAR> name(1024);
		LONGLONG file_time;
		LONGLONG file_size;
		Error_Control_Block ecb;
		
		int rc = igr_check(IGR_Get_Page_Image_Entry(m_impl->m_page_handle, &id[0], &name[0], &file_time, &file_size, &ecb), "IGR_Get_Page_Image_Entry", ecb);
		if (rc != IGR_OK)
			return NULL;

		return new SubFile(getHandle(), ToUTF8Str(&id[0], -1), ToUTF8Str(&name[0], -1), file_time, file_size, IGR_Extract_Page_Image_Stream);
	}

	void Page::Redact(int FirstWord, int LastWord)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Redact_Page_Text(m_impl->m_page_handle, FirstWord, LastWord, 0, &ecb), "IGR_Redact_Page_Text", ecb);
	}

	void Page::Redact(Word *FirstWord, Word *LastWord)
	{
		Redact(FirstWord->getWordIndex(), LastWord->getWordIndex());
	}

	void Page::getDimensions(LONG &Width, LONG &Height)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Get_Page_Dimensions(m_impl->m_page_handle, &Width, &Height, &ecb), "IGR_Get_Page_Dimensions", ecb);
	}

	/****************************************************************************
	* Perceptive::Word
	****************************************************************************/

	Word::Word(IGR_Page_Word *W, int WordIndex)
	{
		m_page_word = W;
		m_word_index = WordIndex;
	}

	Word::~Word()
	{
	}

	std::string Word::GetText() const
	{
		return ToUTF8Str(m_page_word->word, m_page_word->wordLength);
	}

	std::wstring Word::GetTextW() const
	{
		return ToWideStr(m_page_word->word, m_page_word->wordLength);
	}

	int Word::getX() const
	{
		return m_page_word->x;
	}

	int Word::getY() const
	{
		return m_page_word->y;
	}

	int Word::getWidth() const
	{
		return m_page_word->width;
	}

	int Word::getHeight() const
	{
		return m_page_word->height;
	}

	int Word::getStyle() const
	{
		return m_page_word->style;
	}

	int Word::getCharacterOffset() const
	{
		return m_page_word->charoffset;
	}

	int Word::getWordIndex() const
	{
		return m_word_index;
	}

	/****************************************************************************
	* Perceptive::Canvas
	****************************************************************************/

	Canvas::Canvas(HCANVAS C)
	{
		m_canvas_handle = C;
	}

	Canvas::~Canvas()
	{
		Close();
	}

	HCANVAS Canvas::getHandle() const
	{
		return m_canvas_handle;
	}

	void Canvas::Close()
	{
		Error_Control_Block ecb;
		IGR_Close_Canvas(m_canvas_handle, &ecb);
	}

	void Canvas::RenderPage(Page* P)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Render_Page(P->getHandle(), m_canvas_handle, &ecb), "IGR_Render_Page", ecb);
	}

	void Canvas::Arc(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_Arc(m_canvas_handle, x, y, x2, y2, x3, y3, x4, y4, &ecb), "IGR_Canvas_Arc", ecb);
	}

	void Canvas::AngleArc(int x, int y, int Radius, int StartAngle, int SweepAngle)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_AngleArc(m_canvas_handle, x, y, Radius, StartAngle, SweepAngle, &ecb), "IGR_Canvas_AngleArc", ecb);
	}

	void Canvas::Chord(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_Chord(m_canvas_handle, x, y, x2, y2, x3, y3, x4, y4, &ecb), "IGR_Canvas_Chord", ecb);
	}

	void Canvas::Ellipse(int x, int y, int x2, int y2)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_Ellipse(m_canvas_handle, x, y, x2, y2, &ecb), "IGR_Canvas_Ellipse", ecb);
	}

	void Canvas::Rect(int x, int y, int x2, int y2)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_Rect(m_canvas_handle, x, y, x2, y2, &ecb), "IGR_Canvas_Rect", ecb);
	}

	void Canvas::LineTo(int x, int y)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_LineTo(m_canvas_handle, x, y, &ecb), "IGR_Canvas_LineTo", ecb);
	}

	void Canvas::MoveTo(int x, int y)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_MoveTo(m_canvas_handle, x, y, &ecb), "IGR_Canvas_MoveTo", ecb);
	}

	void Canvas::Pie(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_Pie(m_canvas_handle, x, y, x2, y2, x3, y3, x4, y4, &ecb), "IGR_Canvas_Pie", ecb);
	}

	void Canvas::RoundRect(int x, int y, int x2, int y2, int Radius)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_RoundRect(m_canvas_handle, x, y, x2, y2, Radius, &ecb), "IGR_Canvas_RoundRect", ecb);
	}

	void Canvas::TextOut(int x, int y, const std::string& text)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_TextOut(m_canvas_handle, x, y, _UCS2(text), &ecb), "IGR_Canvas_TextOut", ecb);
	}

	void Canvas::TextOut(int x, int y, const std::wstring& text)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_TextOut(m_canvas_handle, x, y, _UCS2(text), &ecb), "IGR_Canvas_TextOut", ecb);
	}

	void Canvas::TextRect(int x, int y, int x2, int y2, const std::string& text, int Flags)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_TextRect(m_canvas_handle, x, y, x2, y2, _UCS2(text), Flags, &ecb), "IGR_Canvas_TextRect", ecb);
	}

	void Canvas::TextRect(int x, int y, int x2, int y2, const std::wstring& text, int Flags)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_TextRect(m_canvas_handle, x, y, x2, y2, _UCS2(text), Flags, &ecb), "IGR_Canvas_TextRect", ecb);
	}

	int Canvas::TextWidth(const std::string& text)
	{
		LONG Width;
		LONG Height;
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_MeasureText(m_canvas_handle, _UCS2(text), &Width, &Height, &ecb), "IGR_Canvas_MeasureText", ecb);
		return Width;
	}

	int Canvas::TextHeight(const std::string& text)
	{
		LONG Width;
		LONG Height;
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_MeasureText(m_canvas_handle, _UCS2(text), &Width, &Height, &ecb), "IGR_Canvas_MeasureText", ecb);
		return Height;
	}

	int Canvas::TextWidth(const std::wstring& text)
	{
		LONG Width;
		LONG Height;
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_MeasureText(m_canvas_handle, _UCS2(text), &Width, &Height, &ecb), "IGR_Canvas_MeasureText", ecb);
		return Width;
	}

	int Canvas::TextHeight(const std::wstring& text)
	{
		LONG Width;
		LONG Height;
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_MeasureText(m_canvas_handle, _UCS2(text), &Width, &Height, &ecb), "IGR_Canvas_MeasureText", ecb);
		return Height;
	}

	void Canvas::SetPen(int Color, int Width, int Style)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_SetPen(m_canvas_handle, Color, Width, Style, &ecb), "IGR_Canvas_SetPen", ecb);
	}

	void Canvas::SetBrush(int Color, int Style)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_SetBrush(m_canvas_handle, Color, Style, &ecb), "IGR_Canvas_SetBrush", ecb);
	}

	void Canvas::SetFont(const std::string& Name, int Size, int Style)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_SetFont(m_canvas_handle, _UCS2(Name), Size, Style, &ecb), "IGR_Canvas_SetFont", ecb);
	}

	void Canvas::SetOpacity(BYTE Opacity)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_SetOpacity(m_canvas_handle, Opacity, &ecb), "IGR_Canvas_SetOpacity", ecb);
	}

	void Canvas::DrawImage(int x, int y, void *ImageData, size_t ImageSize, const std::string &MimeType)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_DrawImage(m_canvas_handle, x, y, ImageData, ImageSize, _UCS2(MimeType), &ecb), "IGR_Canvas_DrawImage", ecb);
	}

	void Canvas::DrawScaleImage(int x, int y, int x2, int y2, void *ImageData, size_t ImageSize, const std::string &MimeType)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_DrawScaleImage(m_canvas_handle, x, y, x2, y2, ImageData, ImageSize, _UCS2(MimeType), &ecb), "DrawScaleImage", ecb);
	}

	void Canvas::Rotation(int Degrees)
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_Rotation(m_canvas_handle, Degrees, &ecb), "IGR_Canvas_Rotation", ecb);
	}

	void Canvas::Reset()
	{
		Error_Control_Block ecb;
		igr_check(IGR_Canvas_Reset(m_canvas_handle, &ecb), "IGR_Canvas_Reset", ecb);
	}

	/****************************************************************************
	* Perceptive::DocumentFilters
	****************************************************************************/

	DocumentFilters::DocumentFilters()
		: m_instance(0)
	{
	}

	DocumentFilters::~DocumentFilters()
	{
		Uninitialize();
	}

	void DocumentFilters::Initialize(const std::string &License, const std::string &Path)
	{
		if (m_instance == 0)
		{
			Error_Control_Block ecb;
			Instance_Status_Block ISB;
			strncpy(ISB.Licensee_ID1, License.c_str(), sizeof(ISB.Licensee_ID1)-1);
			Init_Instance(0, Path.c_str(), &ISB, &m_instance, &ecb);
			if (strlen(ecb.Msg) > 0)
				throw Exception("Init_Instance: Unable to initialize Document Filters: " + std::string(ecb.Msg));
		}
	}

	void DocumentFilters::Uninitialize()
	{
		if (m_instance > 0)
		{
			Error_Control_Block ecb;
			Close_Instance(&ecb);
			m_instance = 0;
		}
	}

	std::string DocumentFilters::GetMemoryStatus()
	{
		std::string retval = "";
		long OutBufm_size = 0xFFFF;
		std::vector<char> buffer(OutBufm_size);
		if (buffer.empty())
		{
			if (IGR_Get_Memory_Status(&buffer[0], OutBufm_size) > 0)
				retval = std::string(&buffer[0]);
		}
		return retval;
	}

	Extractor *DocumentFilters::GetExtractor(const std::string &filename)
	{
		if (m_instance == 0)
			throw Exception("Document Filters not initialized");
		return GetExtractor(new FileStream(filename), true);
	}

	Extractor *DocumentFilters::GetExtractor(Stream *S, bool own_stream)
	{
		if (m_instance == 0)
			throw Exception("Document Filters not initialized");
		return new Extractor(S, own_stream);
	}
	Extractor *DocumentFilters::GetExtractor(IGR_Stream *stream)
	{
		if (m_instance == 0)
			throw Exception("Document Filters not initialized");
		return new Extractor(new IGRStream(stream), true);
	}

	void DocumentFilters::SetDefaultInputEncoding(const std::string &Encoding)
	{
		int CP = 1252;
		if (SameText(Encoding, ENC_JAPANESE))
			CP = 932;
		else if (SameText(Encoding, ENC_CHINESE_SIMPLIFIED))
			CP = 936;
		else if (SameText(Encoding, ENC_KOREAN))
			CP = 949;
		else if (SameText(Encoding, ENC_CHINESE_TRADITIONAL))
			CP = 950;
		else if (SameText(Encoding, ENC_CYRILLIC))
			CP = 1251;
		else if (SameText(Encoding, ENC_LATIN_1))
			CP = 1252;
		else if (SameText(Encoding, ENC_GREEK))
			CP = 1253;
		else if (SameText(Encoding, ENC_TURKISH))
			CP = 1254;
		else if (SameText(Encoding, ENC_HEBREW))
			CP = 1255;
		else if (SameText(Encoding, ENC_ARABIC))
			CP = 1256;
		else if (SameText(Encoding, ENC_UTF_8))
			CP = 65001;
		SetDefaultInputCodePage(CP);
	}

	void DocumentFilters::SetDefaultInputCodePage(const int code_page)
	{
		size_t L1 = code_page;
		size_t L2 = 0;
		Error_Control_Block ecb;
		IGR_Multiplex(IGR_Multi_Set_Code_Page, &L1, &L2, &ecb);
	}

	void DocumentFilters::SetTempPath(const std::string &path)
	{
		size_t L1 = (size_t)path.c_str();
		size_t L2 = 0;
		Error_Control_Block ecb;
		IGR_Multiplex(IGR_Multi_Set_Temp_Path, &L1, &L2, &ecb);
	}

	void DocumentFilters::DumpStatistics(std::ostream &S)
	{
		// This functionality has been moved to Extractor::DumpStatistics()
	}

	Canvas* DocumentFilters::MakeOutputCanvas(const std::string& filename, int Type, const std::string& Options)
	{
		HCANVAS CanvasHandle;
		Error_Control_Block ecb;
		igr_check(IGR_Make_Output_Canvas(Type, _UCS2(filename), _UCS2(Options), &CanvasHandle, &ecb), "IGR_Make_Output_Canvas", ecb);
		return new Canvas(CanvasHandle);
	}

	Canvas* DocumentFilters::MakeOutputCanvas(Stream *stream, int type, const std::string& options)
	{
		HCANVAS CanvasHandle;
		Error_Control_Block ecb;
		igr_check(IGR_Make_Output_Canvas_On(type, (IGR_Writable_Stream*)stream->getIGRStream(), _UCS2(options), &CanvasHandle, &ecb), "IGR_Make_Output_Canvas_On", ecb);
		return new Canvas(CanvasHandle);
	}

	/****************************************************************************
	* Utility functions
	****************************************************************************/

	std::string GetErrorStr(const int ReturnCode)
	{
		std::string Result = "Unknown";
		switch (ReturnCode)
		{
		case IGR_OK:
			Result = "OK";
			break;
		case IGR_E_OPEN_ERROR:
			Result = "Document open error";
			break;
		case IGR_E_WRONG_TYPE:
			Result = "Document is wrong type";
			break;
		case IGR_E_IN_USE:
			Result = "Document is in use";
			break;
		case IGR_E_NOT_READABLE:
			Result = "Document is not readable";
			break;
		case IGR_E_PASSWORD:
			Result = "Document is password protected";
			break;
		case IGR_E_NOT_FOUND:
			Result = "Document not found";
			break;
		case IGR_E_WRITE_ERROR:
			Result = "Document write error";
			break;
		case IGR_E_NOT_VALID_FOR_THIS_CLASS:
			Result = "Document operation not valid";
			break;
		case IGR_E_ERROR:
			Result = "Document error";
			break;
		case IGR_E_INVALID_HANDLE:
			Result = "Invalid document handle";
			break;
		case IGR_E_INVALID_POINTER:
			Result = "Invalid pointer";
			break;
		case IGR_E_INVALID_PARAMETER:
			Result = "Invalid parameter";
			break;
		case IGR_E_FILE_CORRUPT:
			Result = "Document is unreadable or corrupt";
			break;
		case IGR_E_OUT_OF_MEMORY:
			Result = "Out of memory";
			break;
		case IGR_E_BAD_ERROR:
			Result = "Unknown exception";
			break;
		case IGR_E_WRONG_THREAD:
			Result = "Document belongs to different thread";
			break;
		case IGR_E_TOO_MANY_HANDLES:
			Result = "Too many documents open";
			break;
		case IGR_E_FONTS_NOT_FOUND:
			Result = "Could not find fonts";
			break;
		}
		return Result;
	}

	std::string GetFileTypeName(const int FileType)
	{
		const int buffer_len = 256;
		std::string retval = "Unknown";

		Error_Control_Block ecb;
		std::vector<char> buffer(buffer_len);
		if (!buffer.empty())
		{
			if (IGR_Get_Format_Attribute(FileType, 0, &buffer[0], &ecb) == IGR_OK)
				retval.assign((const char *)&buffer[0]);
		}
		return retval;
	}

	std::string HTMLEncode(const std::string &S)
	{
		std::string Result;
		Result.reserve(S.length() * 2);
		for (size_t i = 0; i < S.length(); i++)
		{
			char Ch = S[i];
			switch (Ch)
			{
			case '<':
				Result.append("&lt;");
				break;
			case '>':
				Result.append("&gt;");
				break;
			case '&':
				Result.append("&amp;");
				break;
			case '"':
				Result.append("&quot;");
				break;
			default:
				//if (Ch >= 0x80)
				//  Result.append("&#" + itoa(Ch));
				//else
				Result.append(1, Ch);
				break;
			}
		}
		return Result;
	}

	std::string ReplaceAll(const std::string &S, const std::string& Search, const std::string& Replace)
	{
		std::string Result = S;
		size_t i = 0;
		while (true)
		{
			i = Result.find(Search, i);
			if (i == std::string::npos) break;
			Result.replace(i, Search.length(), Replace);
			i += Replace.length();
		}
		return Result;
	}

	void PrepareFileForBinaryOutput(FILE *F)
	{
#ifdef PERCEPTIVE_PLATFORM_WINDOWS
		_setmode(_fileno(F), O_BINARY);
#endif
	}

	void PrepareFileForWideOutput(FILE *F, const bool IncludeBOM)
	{
		PrepareFileForBinaryOutput(F);
		if (IncludeBOM)
		{
#if defined(PERCEPTIVE_PLATFORM_LITTLE_ENDIAN)
			fwrite("\xFF\xFE", 1, strlen("\xFF\xFE"), F);
#elif defined(PERCEPTIVE_PLATFORM_BIG_ENDIAN)
			fwrite("\xFF\xFE", 1, strlen("\xFE\xFF"), F);
#endif
		}
	}

	void PrepareFileForUTF8Output(FILE *F, const bool IncludeBOM)
	{
		PrepareFileForBinaryOutput(F);
		if (IncludeBOM)
			fwrite("\xEF\xBB\xBF", 1, strlen("\xEF\xBB\xBF"), F);
	}

	std::string ReadTextFile(const std::string &filename)
	{
		std::string Result = "";
#if defined(PERCEPTIVE_PLATFORM_WINDOWS)
		// Convert UTF8 filename to wide chars on Windows.
		FILE* F = 0;
		std::string wFilename = ToWCHARStr(filename);
		errno_t fileErr = _wfopen_s(&F, (wchar_t*)wFilename.c_str(), L"r");
		if (!fileErr)
#else
		FILE *F = fopen(filename.c_str(), "r");
		if (F)
#endif
		{
			while (!feof(F))
			{
				char LineStr[256];
				(void)fgets(LineStr, sizeof(LineStr), F);
				Result += LineStr;
			}
			fclose(F);
		}
		return Result;
	}

	//////////

	bool SameText(const std::string &S1, const std::string &S2)
	{
#ifdef PERCEPTIVE_PLATFORM_WINDOWS
		return _stricmp(S1.c_str(), S2.c_str()) == 0;
#else
		return strcasecmp(S1.c_str(), S2.c_str()) == 0;
#endif
	}

	std::string IncludeTrailingPathDelimiter(const std::string &Path)
	{
		std::string Result = Path;
		if ((Result.length() > 0) && (Result[Result.length() - 1] != PathDelim))
			Result += PathDelim;
		return Result;
	}

	std::string RemoveTrailingPathDelimiter(const std::string &Path)
	{
		std::string Result = Path;
		if ((Result.length() > 0) && (Result[Result.length() - 1] == PathDelim))
			Result.erase(Result.length() - 1, 1);
		return Result;
	}

	std::string JoinFilename(const std::string &Path, const std::string &filename)
	{
		if (Path.empty())
			return filename;
		else
			return IncludeTrailingPathDelimiter(Path) + filename;
	}

	std::string MakeSafeFilename(const std::string &filename)
	{
		std::string Result = "";
		for (size_t i = 0; i < filename.length(); i++)
			if ((filename[i] < ' ') || (filename[i] == '<') || (filename[i] == '>') || (filename[i] == ':') ||
				(filename[i] == '"') || (filename[i] == '|') || (filename[i] == '?') || (filename[i] == '*'))
				Result += '_';
			else if ((filename[i] == '\\') || (filename[i] == '/'))
				Result += PathDelim;
			else
				Result += filename[i];
			return Result;
	}

	bool FileExists(const std::string &filename)
	{
#ifdef PERCEPTIVE_PLATFORM_WINDOWS
		struct _stat buf;
		return(_stat(filename.c_str(), &buf) == 0);
#else
		struct stat buf;
		return(stat(filename.c_str(), &buf) == 0);
#endif
	}

	bool DirExists(const std::string &Path)
	{
		std::string PathEx = RemoveTrailingPathDelimiter(Path);
#ifdef PERCEPTIVE_PLATFORM_WINDOWS
		struct _stat buf;
		return((_stat(PathEx.c_str(), &buf) == 0) && (buf.st_mode & _S_IFDIR));
#else
		struct stat buf;
		return((stat(PathEx.c_str(), &buf) == 0) && (buf.st_mode & S_IFDIR));
#endif
	}

	bool CreateFolder(const std::string &Path)
	{
		bool Result = DirExists(Path);
		if (!Result)
		{
			size_t Found = Path.find_last_of(PathDelim);
			if (Found != std::string::npos)
			{
				std::string ParentPath = Path.substr(0, Found);
				if (!DirExists(ParentPath))
					Result = CreateFolder(ParentPath);
			}
			if (!DirExists(Path))
#ifdef PERCEPTIVE_PLATFORM_WINDOWS
				Result = (_mkdir(Path.c_str()) == 0);
#else
				Result = (mkdir(Path.c_str(), 0755) == 0);
#endif
		}
		return(Result);
	}
	bool CopyFile(const std::string &Source, const std::string &Dest, bool overwrite)
	{
		if (!overwrite && FileExists(Dest))
			return false;

		FILE *s = NULL, *d = NULL;
		size_t size = 0;

		s = fopen(Source.c_str(), "rb");
		if (s == NULL)
			return false;

		d = fopen(Dest.c_str(), "wb");
		if (d == NULL)
		{
			fclose(s);
			return false;
		}

		char buffer[255];
		while ( (size = fread(buffer, 1, sizeof(buffer), s)) > 0 )
		{
			fwrite(buffer, 1, size, d);
		}

		fclose(s);
		fclose(d);
		return true;
	}
	std::string ExtractFilePath(const std::string &Path)
	{
		std::string::size_type p = Path.find_last_of(PathDelim);
		if (p != std::string::npos)
			return Path.substr(0, p+1);
		return "";
	}
	std::string ExtractFileName(const std::string &Path)
	{
		std::string::size_type p = Path.find_last_of(PathDelim);
		if (p != std::string::npos)
			return Path.substr(p+1);
		return Path;
	}
	std::string ChangeFileExt(const std::string &filename, const std::string &Ext)
	{
		std::string::size_type p = filename.find_last_of(".");
		if (p != std::string::npos)
			return filename.substr(0, p) + Ext;
		return filename + Ext;
	}

}; // namespace Perceptive
