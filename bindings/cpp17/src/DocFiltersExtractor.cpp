/*
# (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.

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
*/
#include "DocumentFiltersObjects.h"
#include "DocFiltersCommon.h"
#include <algorithm>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#	define TARGET_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#	define TARGET_LITTLE_ENDIAN 0
#elif defined(_WIN32) || defined(_WIN64) || defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) || defined(__arm__) || defined(__aarch64__)
#	define TARGET_LITTLE_ENDIAN 1
#else
#	include <endian.h>
#	if __BYTE_ORDER == __LITTLE_ENDIAN
#		define TARGET_LITTLE_ENDIAN 1
#	elif __BYTE_ORDER == __BIG_ENDIAN
#		define TARGET_LITTLE_ENDIAN 0
#	else
#		error "Unknown endianess"
#	endif
#endif

namespace Hyland
{
	namespace DocFilters
	{
		namespace
		{
			std::u16string& swap_endianness(std::u16string& buffer)
			{
				for (auto&& ch : buffer)
					ch = (ch >> 8) | (ch << 8); // NOLINT
				return buffer;
			}

			std::u32string& swap_endianness(std::u32string& buffer)
			{
				for (auto&& ch : buffer)
					ch = (ch >> 24) | ((ch >> 8) & 0xFF00) | ((ch << 8) & 0xFF0000) | (ch << 24); // NOLINT
				return buffer;
			}

			template <typename str>
			str& prepare_endianness(str& buffer, bool need_little)
			{
#if TARGET_LITTLE_ENDIAN == 1
				// system is little
				if (need_little)
					return buffer;
				
				return swap_endianness(buffer);
#else
				// system is big
				if (need_little)
					return swap_endianness(buffer);
				return buffer;
#endif
			}
		} // namespace

		class Extractor::impl_t
		{
		public:
			IGR_Stream* m_stream = nullptr;
			handle_holder_t<IGR_LONG> m_handle;
			IGR_LONG m_type = 0;
			IGR_LONG m_caps = 0;
			bool m_eof = false;
			DocumentFilters::open_callback_t m_callback;
			Extractor::password_callback_t m_password_callback;
			Extractor::localize_callback_t m_localize_callback;
			Extractor::heartbeat_callback_t m_heartbeat_callback;
			std::optional<Extractor::pages_t> m_pages_loader;
			std::shared_ptr<subfile_enumerable_t> m_subfiles;
			std::shared_ptr<subfile_enumerable_t> m_images;

			explicit impl_t(IGR_Stream* Stream)
				: m_stream(Stream)
				, m_handle(0, &IGR_Close_File)
			{
			}
			impl_t(const impl_t&) = delete; 
			impl_t& operator=(const impl_t&) = delete;
			impl_t(impl_t&&) = delete; 
			impl_t& operator=(impl_t&&) = delete;

			~impl_t()
			{
				try
				{
					Close(true);
				}
				catch (...)
				{
				}
			}

			void Close(bool close_stream)
			{
				m_handle.reset();
				m_eof = false;
				m_subfiles.reset();
				m_images.reset();
				m_pages_loader.reset();

				if (close_stream && m_stream != nullptr)
				{
					m_stream->Close(m_stream);
					m_stream = nullptr;
				}
			}

			[[nodiscard]] 
			IGR_Stream* need_stream() const
			{
				if (m_stream == nullptr)
					throw DocumentFilters::Error("Stream has been closed");
				return m_stream;
			}

			[[nodiscard]] 
			IGR_LONG need_handle() const
			{
				if (m_handle.getHandle() == 0)
					throw DocumentFilters::Error("Extractor has been closed");
				return m_handle.getHandle();
			}

			[[nodiscard]] 
			bool has_handle() const
			{
				return m_handle.getHandle() != 0;
			}

			void need_type()
			{
				if (!has_handle() && m_type == 0 && m_caps == 0)
				{
					Error_Control_Block ecb = { 0 };
					IGR_Get_Stream_Type(need_stream(), &m_caps, &m_type, &ecb); // ignore errors
				}
			}
		};


		Extractor::Extractor()
			: m_impl(new impl_t(nullptr))
		{
		}

		Extractor::Extractor(IGR_Stream* Stream)
			: m_impl(new impl_t(Stream))
		{
		}

		IGR_HDOC Extractor::getHandle() const
		{
			return m_impl->need_handle();
		}

		IGR_Stream* Extractor::need_stream() const
		{
			if (m_impl->m_stream == nullptr)
				m_impl->m_stream = resolve_stream();
			return m_impl->need_stream();
		}

		void Extractor::Open(uint32_t open_flags, const std::wstring& option, const DocumentFilters::open_callback_t& callback)
		{
			m_impl->Close(false);
			m_impl->m_callback = callback;

			int flags = open_flags;

			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Open_Ex(IGR_OPEN_FROM_STREAM
				, need_stream()
				, flags
				, reinterpret_cast<const IGR_UCS2*>(w_to_u16(option).c_str())
				, &m_impl->m_caps
				, &m_impl->m_type
				, nullptr
				, [](IGR_OPEN_CALLBACK_ACTION action, void* payload, void* context) -> IGR_LONG
				{
					auto* impl = reinterpret_cast<impl_t*>(context);
					switch (action)
					{
					case IGR_OPEN_CALLBACK_ACTION_HEARTBEAT:
						if (impl->m_heartbeat_callback)
							return impl->m_heartbeat_callback();
						else if (impl->m_callback)
							return impl->m_callback(action, payload);
						break;
					case IGR_OPEN_CALLBACK_ACTION_PASSWORD:
						if (impl->m_password_callback)
						{
							auto *p = reinterpret_cast<IGR_Open_Callback_Action_Password*>(payload);
							std::wstring res = impl->m_password_callback(u16_to_w(p->id));
							if (res.empty())
								return IGR_E_ERROR;
							copy_string(w_to_u16(res), p->password);
							return IGR_OK;
						}
						else if (impl->m_callback)
							return impl->m_callback(action, payload);
						break;
					case IGR_OPEN_CALLBACK_ACTION_LOCALIZE:
						if (impl->m_localize_callback)
						{
							auto *p = reinterpret_cast<IGR_Open_Callback_Action_Localize*>(payload);
							std::wstring res = impl->m_localize_callback(p->string_id, u16_to_w(p->original));
							if (res.empty())
								return IGR_E_ERROR;
							copy_string(w_to_u16(res), p->replacement);
							return IGR_OK;
						}
						else if (impl->m_callback)
							return impl->m_callback(action, payload);
						break;
					default:
						if (impl->m_callback)
							return impl->m_callback(action, payload);
						break;
					}
					return 0; }, m_impl.get(), m_impl->m_handle.attach(), &ecb),
				ecb, "IGR_Open_Ex");
			;
		}

		void Extractor::Open(OpenMode mode, uint32_t open_flags, const std::wstring& option, const DocumentFilters::open_callback_t& callback)
		{
			uint32_t flags = open_flags;
			if (mode == OpenMode::Text)
				flags = (flags & ~0xffff0000) | IGR_FORMAT_TEXT; // NOLINT
			else if (mode == OpenMode::Paginated)
				flags = (flags & ~0xffff0000) | IGR_FORMAT_IMAGE; // NOLINT
			else if (mode == OpenMode::ClassicHtml)
				flags = (flags & ~0xffff0000) | IGR_FORMAT_HTML; // NOLINT

			Open(flags, option, callback);
		}

		IGR_Stream* Extractor::resolve_stream() const
		{
			return m_impl->need_stream();
		}

		void Extractor::Close()
		{
			m_impl->Close(false);
		}

		void Extractor::setPasswordCallback(const password_callback_t& callback)
		{
			m_impl->m_password_callback = callback;
		}

		void Extractor::setLocalizeCallback(const localize_callback_t& callback)
		{
			m_impl->m_localize_callback = callback;
		}

		void Extractor::setHeartbeatCallback(const heartbeat_callback_t& callback)
		{
			m_impl->m_heartbeat_callback = callback;
		}

		uint32_t Extractor::getFileType() const
		{
			need_stream();
			m_impl->need_type();
			return static_cast<uint32_t>(m_impl->m_type);
		}

		std::wstring Extractor::getFileType(Format::What what) const
		{
			std::wstring res;
			Format::Fetch(static_cast<uint32_t>(getFileType()), what, res);
			return res;
		}

		uint32_t Extractor::getFileCapabilities() const
		{
			m_impl->need_type();
			return static_cast<uint32_t>(m_impl->m_caps);
		}

        std::wstring Extractor::getText(size_t max_length, bool strip_control_chars)
        {
            if (max_length == 0)
                throw std::invalid_argument("max_length");

            Error_Control_Block ecb = { 0 };
            IGR_LONG length = static_cast<IGR_ULONG>(max_length);
            std::vector<IGR_UCS2> buffer(max_length + 1);
            throw_on_error(IGR_Get_Text(m_impl->need_handle(), &buffer[0], &length, &ecb), ecb, "IGR_Get_Text");
            m_impl->m_eof = length == 0;

            auto&& res = u16_to_w(&buffer[0], length);

            if (strip_control_chars)
            {
                // Replace \xE with new line
                std::replace(res.begin(), res.end(), '\xE', '\n');

                // Strip \x01-\x08,\x0b,\x0c-\x10
                res.erase(std::remove_if(res.begin(), res.end(), [](wchar_t ch) {
                    return (ch >= '\x01' && ch <= '\x08') || ch == '\x0B' || (ch >= '\x0C' && ch <= '\x10');
                }), res.end());
            }

            return res;
        }

		bool Extractor::getEOF() const
		{
			return m_impl->m_eof;
		}

		void Extractor::SaveTo(const std::wstring& filename, int code_page)
		{
			std::ofstream Stream(w_to_u8(filename), std::ios::binary);
			if (!Stream)
				throw std::runtime_error("Failed to open file for writing");
			SaveTo(Stream, code_page);
		}

		void Extractor::SaveTo(std::ostream& Stream, int code_page)
		{
			static const int utf8 = 65001;
			static const int utf16le = 1200;
			static const int utf16be = 1201;
			static const int utf32le = 12000;
			static const int utf32be = 12001;

			while (!getEOF())
			{
				std::wstring text = getText(4096); // NOLINT
				if (text.empty())
					break;

				switch (code_page)
				{
				case utf16le: // UTF-16 LE 
				case utf16be: // UTF-16 BE
				{
					std::u16string utf16 = w_to_u16(text);
					prepare_endianness(utf16, code_page == utf16le);
					Stream.write(reinterpret_cast<const char*>(utf16.c_str()), utf16.size() * sizeof(char16_t));
				}
				break;
				case utf32le: // UTF-32 LE
				case utf32be: // UTF-32 BE
				{
					std::u32string utf32 = w_to_u32(text);
					prepare_endianness(utf32, code_page == utf32le);
					Stream.write(reinterpret_cast<const char*>(utf32.c_str()), utf32.size() * sizeof(char32_t));
				}
				break;
				case utf8:
				default:
				{
					std::string utf8 = w_to_u8(text);
					Stream.write(utf8.c_str(), utf8.size());
				}
				break;
				}
			}
		}

		void Extractor::CopyTo(const std::wstring& filename)
		{
			std::ofstream Stream(w_to_u8(filename), std::ios::binary);
			if (!Stream)
				throw std::runtime_error("Failed to open file for writing");
			CopyTo(Stream);
		}

		void Extractor::CopyTo(std::ostream& Stream)
		{
			IGR_Stream* strm = need_stream();

			static const IGR_ULONG buffer_size = 1024;
			strm->Seek(strm, 0, SEEK_SET);
			std::vector<uint8_t> buffer(buffer_size);
			IGR_ULONG bytes_read = 0;

			while ((bytes_read = strm->Read(strm, &buffer[0], buffer_size)) > 0)
			{
				Stream.write(reinterpret_cast<const char*>(&buffer[0]), bytes_read);
			}
		}

		void Extractor::CopyTo(Stream& Stream)
		{
			IGR_Stream* strm = need_stream();

			static const IGR_ULONG buffer_size = 1024;
			strm->Seek(strm, 0, SEEK_SET);
			std::vector<uint8_t> buffer(buffer_size);
			IGR_ULONG bytes_read = 0;

			while ((bytes_read = strm->Read(strm, &buffer[0], buffer_size)) > 0)
			{
				Stream.write(reinterpret_cast<const char*>(&buffer[0]), bytes_read);
			}
		}

		std::wstring Extractor::getHashMD5()
		{
			Error_Control_Block ecb = { 0 };
			IGR_UCS2 buffer[50] = { 0 }; // NOLINT
			throw_on_error(IGR_Calculate_MD5(need_stream(), &buffer[0], &ecb), ecb, "IGR_Calculate_MD5");
			return u16_to_w(buffer);
		}

		std::wstring Extractor::getHashSHA1()
		{
			Error_Control_Block ecb = { 0 };
			IGR_UCS2 buffer[50] = { 0 }; // NOLINT
			throw_on_error(IGR_Calculate_SHA1(need_stream(), &buffer[0], &ecb), ecb, "IGR_Calculate_MD5");
			return u16_to_w(buffer);
		}

		size_t Extractor::getPageCount() const
		{
			Error_Control_Block ecb = { 0 };
			IGR_LONG res = 0;
			throw_on_error(IGR_Get_Page_Count(m_impl->need_handle(), &res, &ecb), ecb, "IGR_Get_Page_Count");
			return static_cast<size_t>(res);
		}

		Page Extractor::getPage(size_t index) const
		{
			if (index > getPageCount())
				throw std::out_of_range("index");

			Error_Control_Block ecb = { 0 };
			IGR_HPAGE p = 0;
			throw_on_error(IGR_Open_Page(m_impl->need_handle(), static_cast<IGR_LONG>(index), &p, &ecb), ecb, "IGR_Get_Page");
			return Page(p, index);
		}

		const Extractor::pages_t& Extractor::pages() const
		{
			if (!m_impl->m_pages_loader.has_value())
				m_impl->m_pages_loader.emplace(getPageCount(), [this](size_t index) -> Page
					{ return getPage(index); });
			return *m_impl->m_pages_loader;
		}

		const Extractor::subfiles_t& Extractor::subfiles() const
		{
			if (!m_impl->m_subfiles)
				m_impl->m_subfiles = std::make_shared<subfile_enumerable_t>(m_impl->need_handle(), &IGR_Get_Subfiles_Enumerator, &IGR_Extract_Subfile_Stream);

			return *m_impl->m_subfiles;
		}

		Subfile Extractor::getSubFile(const std::wstring& id) const
		{
			if (id.empty())
				throw std::invalid_argument("id");

			Error_Control_Block ecb = { 0 };
			IGR_Stream* Stream = nullptr;
			throw_on_error(IGR_Extract_Subfile_Stream(m_impl->need_handle(), reinterpret_cast<const IGR_UCS2*>(w_to_u16(id).c_str()), &Stream, &ecb), ecb, "IGR_Extract_Subfile_Stream");
			return Subfile(m_impl->need_handle(), id, Stream);
		}

		const Extractor::subfiles_t& Extractor::images() const
		{
			if (!m_impl->m_images)
				m_impl->m_images = std::make_shared<subfile_enumerable_t>(m_impl->need_handle(), &IGR_Get_Images_Enumerator, &IGR_Extract_Image_Stream);

			return *m_impl->m_images;
		}

		Subfile Extractor::getImage(const std::wstring& id) const
		{
			if (id.empty())
				throw std::invalid_argument("id");

			Error_Control_Block ecb = { 0 };
			IGR_Stream* Stream = nullptr;
			throw_on_error(IGR_Extract_Image_Stream(m_impl->need_handle(), reinterpret_cast<const IGR_UCS2*>(w_to_u16(id).c_str()), &Stream, &ecb), ecb, "IGR_Extract_Image_Stream");
			return Subfile(m_impl->need_handle(), id, Stream);
		}

		Bookmark Extractor::getRootBookmark() const
		{
			Error_Control_Block ecb = { 0 };
			IGR_Bookmark b = { nullptr };
			throw_on_error(IGR_Get_Bookmark_Root(m_impl->need_handle(), &b, &ecb), ecb, "IGR_Get_Root_Bookmark");
			return Bookmark(m_impl->need_handle(), b);
		}

		IGR_Stream* Extractor::StealStream()
		{
			IGR_Stream* res = m_impl->m_stream;
			m_impl->m_stream = nullptr;

			return res;
		}

		CompareResults Extractor::Compare(const Extractor& other, const CompareSettings& settings) const
		{
			if (other.getHandle() == 0)
				throw std::invalid_argument("other cannot be null");

			CompareDocumentSource left(getHandle());
			CompareDocumentSource right(other.getHandle());

			return Compare(left, right, settings);
		}

		CompareResults Extractor::Compare(const CompareDocumentSettings& thisDocSettings, const Extractor& other, const CompareDocumentSettings& otherDocSettings, const CompareSettings& settings) const
		{
			if (other.getHandle() == 0)
				throw std::invalid_argument("other cannot be null");

			CompareDocumentSource left(getHandle(), thisDocSettings);
			CompareDocumentSource right(other.getHandle(), otherDocSettings);
			
			return Compare(left, right, settings);
		}

		CompareResults Extractor::Compare(const CompareDocumentSettings& thisDocSettings, const CompareDocumentSource& other, const CompareSettings& settings) const
		{
			if (other.getHandle() == 0)
				throw std::invalid_argument("other cannot be null");

			CompareDocumentSource left(getHandle(), thisDocSettings);
			return Compare(left, other, settings);
		}

		CompareResults Extractor::Compare(const CompareDocumentSource& thisDoc, const CompareDocumentSource& otherDoc, const CompareSettings& settings)
		{
			if (thisDoc.getHandle() == 0)
				throw std::invalid_argument("thisDoc cannot be null");
			if (otherDoc.getHandle() == 0)
				throw std::invalid_argument("otherDoc cannot be null");

			auto&& compare_settings = settings.data();
			auto&& left = thisDoc.data();
			auto&& right = otherDoc.data();

			Error_Control_Block ecb = { 0 };
			IGR_HTEXTCOMPARE res = nullptr;
			throw_on_error(IGR_Text_Compare_Documents(&left, &right, &compare_settings, &res, &ecb), ecb, "IGR_Text_Compare_Documents", "Failed to compare documents");

			return CompareResults(res);
		}

	} // namespace DocFilters
} // namespace Hyland
