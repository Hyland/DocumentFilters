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
#include <cstring>

namespace Hyland
{
	namespace DocFilters
	{

		template <typename InnerStream, typename Traits>
		IGR_Stream* bridge_stream_t(InnerStream* stream, bool own_stream, IGR_Stream** igr_stream)
		{
			struct funcs
			{
				static IGR_ULONG IGR_EXPORT read(void* handle, void* buffer, IGR_ULONG size)
				{
					return Traits::read(reinterpret_cast<InnerStream*>(handle), buffer, size);
				}
				static IGR_ULONG IGR_EXPORT write(void* handle, const void* buffer, IGR_ULONG size)
				{
					return Traits::write(reinterpret_cast<InnerStream*>(handle), buffer, size);
				}
				static IGR_LONGLONG IGR_EXPORT seek(void* handle, IGR_LONGLONG offset, IGR_ULONG origin)
				{
					return Traits::seek(reinterpret_cast<InnerStream*>(handle), offset, origin);
				}
				static void IGR_EXPORT destroy(void* handle)
				{
					Traits::destroy(reinterpret_cast<InnerStream*>(handle));
				}
				static void IGR_EXPORT destroy_noop(void* handle)
				{
				}
			};
			if (!stream)
				throw std::invalid_argument("stream cannot be null");
			if (!igr_stream)
				throw std::invalid_argument("igr_stream cannot be null");

			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Make_Stream_From_Functions(stream
				, 0
				, funcs::seek
				, funcs::read
				, funcs::write
				, nullptr // no actions
				, own_stream ? funcs::destroy : funcs::destroy_noop, igr_stream, &ecb),
				ecb, "IGR_Make_Stream_From_Functions", "Failed to create stream from file");

			return *igr_stream;
		}

		struct iostream_traits
		{
			static IGR_ULONG read(std::istream* stream, void* buffer, IGR_ULONG size)
			{
				stream->clear(); // clear any bad-bit state
				stream->read(reinterpret_cast<char*>(buffer), size);
				return static_cast<IGR_ULONG>(stream->gcount());
			}
			static IGR_ULONG write(std::istream* /*stream*/, const void* /*buffer*/, IGR_ULONG /*size*/)
			{
				return 0;
			}
			static IGR_ULONG write(std::iostream* stream, const void* buffer, IGR_ULONG size)
			{
				stream->clear(); // clear any bad-bit state
				stream->write(reinterpret_cast<const char*>(buffer), size);
				return stream->good() ? size : 0;
			}
			static IGR_LONGLONG seek(std::iostream* stream, IGR_LONGLONG offset, IGR_ULONG origin)
			{
				stream->clear(); // clear any bad-bit state
				stream->seekg(offset, static_cast<std::ios_base::seekdir>(origin));
				stream->seekp(offset, static_cast<std::ios_base::seekdir>(origin));
				std::streamoff res = stream->tellg();
				return static_cast<IGR_LONGLONG>(res);
			}
			static IGR_LONGLONG seek(std::istream* stream, IGR_LONGLONG offset, IGR_ULONG origin)
			{
				stream->clear(); // clear any bad-bit state
				stream->seekg(offset, static_cast<std::ios_base::seekdir>(origin));
				std::streamoff res = stream->tellg();
				return static_cast<IGR_LONGLONG>(res);
			}
			static void destroy(std::iostream* stream)
			{
				delete stream;
			}
			static void destroy(std::istream* stream)
			{
				delete stream;
			}
		};

		IGR_Stream* Stream::bridge_iostream(std::iostream* stream, bool own_stream, IGR_Stream** igr_stream)
		{
			return bridge_stream_t<std::iostream, iostream_traits>(stream, own_stream, igr_stream);
		}
		IGR_Stream* Stream::bridge_istream(std::istream* stream, bool own_stream, IGR_Stream** igr_stream)
		{
			return bridge_stream_t<std::istream, iostream_traits>(stream, own_stream, igr_stream);
		}

		IGR_Stream* Stream::bridge_file(FILE* file, bool own_file, IGR_Stream** igr_stream)
		{
			struct traits
			{
				static IGR_ULONG read(FILE* file, void* buffer, IGR_ULONG size)
				{
					return static_cast<IGR_ULONG>(fread(buffer, 1, size, file));
				}
				static IGR_ULONG write(FILE* file, const void* buffer, IGR_ULONG size)
				{
					return static_cast<IGR_ULONG>(fwrite(buffer, 1, size, file));
				}
				static IGR_LONGLONG seek(FILE* file, IGR_LONGLONG offset, IGR_ULONG origin)
				{
#ifdef _WIN32
					_fseeki64(file, offset, origin);
					return static_cast<IGR_ULONG>(_ftelli64(file));
#else
					fseeko64(file, offset, static_cast<int>(origin));
					return ftello64(file);
#endif
				}
				static void destroy(FILE* file)
				{
					fclose(file);
				}
			};

			return bridge_stream_t<FILE, traits>(file, own_file, igr_stream);
		}

		IGR_Stream* Stream::bridge_stream(Hyland::DocFilters::Stream* stream, bool own_stream, IGR_Stream** igr_stream)
		{
			struct traits
			{
				static IGR_ULONG read(Hyland::DocFilters::Stream* stream, void* buffer, IGR_ULONG size)
				{
					return static_cast<IGR_ULONG>(stream->read(reinterpret_cast<char*>(buffer), size));
				}
				static IGR_ULONG write(Hyland::DocFilters::Stream* stream, const void* buffer, IGR_ULONG size)
				{
					return static_cast<IGR_ULONG>(stream->write(reinterpret_cast<const char*>(buffer), size));
				}
				static IGR_LONGLONG seek(Hyland::DocFilters::Stream* stream, IGR_LONGLONG offset, IGR_ULONG origin)
				{
					return stream->seek(offset, static_cast<std::ios_base::seekdir>(origin));
				}
				static void destroy(Hyland::DocFilters::Stream* Stream)
				{
					delete Stream;
				}
			};

			return bridge_stream_t<Hyland::DocFilters::Stream, traits>(stream, own_stream, igr_stream);
		}

		// --------------------------------------------------------------------------------
		std::streamoff Stream::getSize()
		{
			auto curr = get_position();
			auto size = seek(0, std::ios::end);
			set_position(curr);
			return size;
		}
		std::streamoff Stream::get_position()
		{
			return seek(0, std::ios::cur);
		}
		std::streamoff Stream::set_position(std::streamoff pos)
		{
			return seek(pos, std::ios::beg);
		}

		// --------------------------------------------------------------------------------

		MemStream::MemStream(void* buffer, size_t size)
			: m_buffer(buffer), m_size(size), m_capacity(size)
		{
		}
		MemStream::MemStream(const void* buffer, size_t size)
			: m_buffer(const_cast<void*>(buffer)) // NOLINT
			, m_size(size)
			, m_capacity(size)
		{
		}

		std::streamoff MemStream::seek(std::streampos offset, std::ios_base::seekdir way)
		{
			switch (way)
			{
			case std::ios::beg:
				m_offset = offset;
				break;
			case std::ios::end:
				m_offset = m_size + offset;
				break;
			case std::ios::cur:
				m_offset += offset;
				break;
			default:
				throw std::runtime_error("Not implemented");
			}
			return m_offset;
		}

		size_t MemStream::read(void* buffer, size_t size)
		{
			if (m_offset >= m_size || m_offset >= m_capacity)
				return 0;
			size_t bytes = std::min(size, m_capacity - m_offset);
			std::memcpy(buffer, static_cast<uint8_t*>(m_buffer) + m_offset, bytes);
			m_offset += bytes;
			return bytes;
		}

		size_t MemStream::write(const void* buffer, size_t size)
		{
			size_t end = m_offset + size;
			if (end > m_size)
				resize(end);

			std::memcpy(static_cast<uint8_t*>(m_buffer) + m_offset, buffer, size);
			m_offset = end;
			if (m_offset > m_size)
				m_size = m_offset;
			return size;
		}

		void MemStream::set_size(size_t size)
		{
			m_size = size;
			if (m_size > m_capacity)
				resize(m_size);
		}

		void MemStream::resize(size_t new_size)
		{
			if (new_size > m_capacity)
			{
				void* new_buffer = resize_buffer(m_buffer, new_size);
				if (new_buffer == nullptr)
					throw std::runtime_error("Failed to resize buffer");
				m_buffer = new_buffer;
				m_capacity = new_size;
				m_size = new_size;
			}
		}

		// --------------------------------------------------------------------------------

		VectorStream::VectorStream()
			: MemStream(static_cast<void*>(nullptr), 0)
		{

		}

		void* VectorStream::resize_buffer(void* /*buffer*/, size_t new_size)
		{
			// Calculate the nearest chunk size divisible by 1024
			size_t chunkSize = (new_size + 1023) / 1024 * 1024; // NOLINT

			if (chunkSize > m_data.size())
				m_data.resize(chunkSize);

			return &m_data[0];
		}
	} // namespace DocFilters
} // namespace Hyland
