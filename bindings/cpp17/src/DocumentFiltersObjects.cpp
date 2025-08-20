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
#include <mutex>
#include <optional>
#include <vector>
#include <cstdint>

namespace Hyland
{
    namespace DocFilters
    {

        class DocumentFilters::impl_t
        {
        public:
            IGR_SHORT m_instance = 0;
            std::mutex m_lock;
            std::optional<std::vector<Format>> m_formats;
            std::optional<std::vector<Option>> m_options;

            void ensure_initialized() const
            {
                if (m_instance == 0)
                    throw DocumentFilters::Error("Document filters not initialized");
            }

            const std::vector<Format> &getFormats()
            {
                std::lock_guard<std::mutex> lock(m_lock);
                if (!m_formats.has_value())
                    m_formats = load_formats();

                return *m_formats;
            }

            const std::vector<Option> &getOptions()
            {
                std::lock_guard<std::mutex> lock(m_lock);
                if (!m_options.has_value())
                    m_options = load_options();

                return *m_options;
            }

            [[nodiscard]]
            std::vector<Format> load_formats() const
            {
                static const size_t max_formats = 0xffff;
                std::vector<Format> result;

                ensure_initialized();

                for (uint32_t i = 1; i < max_formats; ++i)
                {
                    Format format(static_cast<uint32_t>(i));
                    if (format.id() != static_cast<uint32_t>(-1))
                        result.push_back(format);
                }
                return result;
            }

            [[nodiscard]]
            std::vector<Option> load_options() const
            {
                static const size_t max_options = 0xffff;
                std::vector<Option> result;

                ensure_initialized();

                for (uint32_t i = 0; i < max_options; ++i)
                {
                    Option option(static_cast<uint32_t>(i));
                    if (!option.getDisplayName().empty())
                        result.push_back(option);
                    else
                        break;
                }
                return result;
            }
        };

        DocumentFilters::Error::Error(IGR_RETURN_CODE code, const std::string &message)
            : std::runtime_error(message), m_code(code)
        {
        }

        DocumentFilters::DocumentFilters()
            : m_impl(std::make_shared<impl_t>())
        {
        }

        DocumentFilters::DocumentFilters(const std::wstring& license, const std::wstring& path)
            : DocumentFilters()
        {
			Initialize(license, path);
        }

        DocumentFilters::DocumentFilters(const std::string& license, const std::string& path)
            : DocumentFilters()
        {
            Initialize(license, path);
        }

        void DocumentFilters::Initialize(const std::wstring &license, const std::wstring &path)
        {
            Initialize(w_to_u8(license), w_to_u8(path));
        }

        void DocumentFilters::Initialize(const std::string &license, const std::string &path)
        {
            Error_Control_Block ecb = {0};
            Instance_Status_Block isb = {0};

            copy_string(license, isb.Licensee_ID1);
            Init_Instance(0, path.c_str(), &isb, &m_impl->m_instance, &ecb);
            throw_on_error(IGR_OK, ecb, "Init_Instance", "Failed to initialize document filters");
        }

        const std::vector<Format> &DocumentFilters::getFormats() const
        {
            return m_impl->getFormats();
        }

        const std::vector<Option> &DocumentFilters::getOptions() const
        {
            return m_impl->getOptions();
        }

        Extractor DocumentFilters::GetExtractor(const std::string &filename)
        {
            return GetExtractor(u8_to_w(filename));
        }

        Extractor DocumentFilters::GetExtractor(const std::wstring &filename)
        {
            if (filename.empty())
                throw std::invalid_argument("filename");

            Error_Control_Block ecb = {0};
            IGR_Stream *Stream = nullptr;

            throw_on_error(IGR_Make_Stream_From_File(reinterpret_cast<const IGR_UCS2 *>(w_to_u16(filename).c_str()), 0, &Stream, &ecb), ecb, "IGR_Make_Stream_From_File", "Failed to create stream from file");
            return Extractor(Stream);
        }

        Extractor DocumentFilters::GetExtractor(const void *data, size_t size, memory_destruct_t deleter)
        {
            if (data == nullptr)
                throw std::invalid_argument("data cannot be null");
            if (size == 0)
                throw std::invalid_argument("size cannot be 0");

            Error_Control_Block ecb = {0};
            IGR_Stream *Stream = nullptr;

            throw_on_error(IGR_Make_Stream_From_Memory(const_cast<void *>(data), size, reinterpret_cast<void*>(deleter), &Stream, &ecb), ecb, "IGR_Make_Stream_From_Memory", "Failed to create stream from memory"); // NOLINT
            return Extractor(Stream);
        }

        Extractor DocumentFilters::GetExtractor(IGR_Stream *stream)
        {
            if (stream == nullptr)
                throw std::invalid_argument("stream cannot be null");

            return Extractor(stream);
        }

        Extractor DocumentFilters::GetExtractor(std::istream &stream)
        {
            IGR_Stream *igr_stream = nullptr;

            return Extractor(Stream::bridge_istream(&stream, false, &igr_stream));
        }

        Extractor DocumentFilters::GetExtractor(std::istream *stream, bool own_stream)
        {
            if (stream == nullptr)
                throw std::invalid_argument("stream cannot be nul");

            IGR_Stream *igr_stream = nullptr;

            return Extractor(Stream::bridge_istream(stream, own_stream, &igr_stream));
        }

        Extractor DocumentFilters::GetExtractor(FILE* file, bool own_file)
        {
			if (file == nullptr)
				throw std::invalid_argument("file cannot be null");

            IGR_Stream* igr_stream = nullptr;
            return Extractor(Stream::bridge_file(file, own_file, &igr_stream));
        }

        Extractor DocumentFilters::GetExtractor(Stream& stream)
        {
            IGR_Stream* igr_stream = nullptr;
            return Extractor(Stream::bridge_stream(&stream, false, &igr_stream));
        }

        Extractor DocumentFilters::GetExtractor(Stream* stream, bool own_stream)
        {
			if (stream == nullptr)
				throw std::invalid_argument("stream");

            IGR_Stream* igr_stream = nullptr;
            return Extractor(Stream::bridge_stream(stream, own_stream, &igr_stream));
        }

        Extractor DocumentFilters::OpenExtractor(const std::string &filename, OpenMode mode, int open_flags, const std::wstring &options, const open_callback_t& callback)
        {
            return OpenExtractor(u8_to_w(filename), mode, open_flags, options, callback);
        }

        Extractor DocumentFilters::OpenExtractor(const std::wstring &filename, OpenMode mode, int open_flags, const std::wstring &options, const open_callback_t& callback)
        {
            Extractor res = GetExtractor(filename);
            res.Open(mode, open_flags, options, callback);
            return res;
        }

        Extractor DocumentFilters::OpenExtractor(const void *data, size_t size, memory_destruct_t deleter, OpenMode mode, int open_flags, const std::wstring &option, const open_callback_t& callback)
        {
            Extractor res = GetExtractor(data, size, deleter);
            res.Open(mode, open_flags, option, callback);
            return res;
        }

        Extractor DocumentFilters::OpenExtractor(IGR_Stream *stream, OpenMode mode, int open_flags, const std::wstring &option, const open_callback_t& callback)
        {
            Extractor res = GetExtractor(stream);
            res.Open(mode, open_flags, option, callback);
            return res;
        }

        Extractor DocumentFilters::OpenExtractor(std::istream &stream, OpenMode mode, int open_flags, const std::wstring &option, const open_callback_t& callback)
        {
            Extractor res = GetExtractor(stream);
            res.Open(mode, open_flags, option, callback);
            return res;
        }

        Extractor DocumentFilters::OpenExtractor(std::istream *stream, bool own_stream, OpenMode mode, int open_flags, const std::wstring &option, const open_callback_t& callback)
        {
            Extractor res = GetExtractor(stream, own_stream);
            res.Open(mode, open_flags, option, callback);
            return res;
        }

        Extractor DocumentFilters::OpenExtractor(FILE* file, bool own_file, OpenMode mode, int open_flags, const std::wstring& option, const open_callback_t& callback)
        {
            Extractor res = GetExtractor(file, own_file);
            res.Open(mode, open_flags, option, callback);
            return res;
        }

        Extractor DocumentFilters::OpenExtractor(Stream& stream, OpenMode mode, int open_flags, const std::wstring& option, const open_callback_t& callback)
        {
            Extractor res = GetExtractor(stream);
            res.Open(mode, open_flags, option, callback);
            return res;
        }

        Extractor DocumentFilters::OpenExtractor(Stream* stream, bool own_stream, OpenMode mode, int open_flags, const std::wstring& option, const open_callback_t& callback)
        {
            Extractor res = GetExtractor(stream, own_stream);
            res.Open(mode, open_flags, option, callback);
            return res;
		}

		Canvas DocumentFilters::MakeOutputCanvas(const std::string& filename, CanvasType type, const std::wstring& options)
		{
            return MakeOutputCanvas(u8_to_w(filename), type, options);
        }

		Canvas DocumentFilters::MakeOutputCanvas(const std::wstring& filename, CanvasType type, const std::wstring& options)
		{
            Error_Control_Block ecb = { 0 };
			IGR_HCANVAS handle = 0;

            throw_on_error(IGR_Make_Output_Canvas(static_cast<int>(type)
                , reinterpret_cast<const IGR_UCS2*>(w_to_u16(filename).c_str())
                , reinterpret_cast<const IGR_UCS2*>(w_to_u16(options).c_str())
                , &handle
				, &ecb), ecb, "IGR_Make_Output_Canvas");

			return Canvas(handle);
		}

		Canvas DocumentFilters::MakeOutputCanvas(std::iostream& stream, CanvasType type, const std::wstring& options)
		{
			IGR_Writable_Stream* igr_stream = nullptr;
			Stream::bridge_iostream(&stream, false, reinterpret_cast<IGR_Stream**>(&igr_stream));
			return MakeOutputCanvas(igr_stream, type, options);
		}

		Canvas DocumentFilters::MakeOutputCanvas(std::iostream* stream, bool own_stream, CanvasType type, const std::wstring& options)
		{
            IGR_Writable_Stream* igr_stream = nullptr;
            Stream::bridge_iostream(stream, own_stream, reinterpret_cast<IGR_Stream**>(&igr_stream));
            return MakeOutputCanvas(igr_stream, type, options);
        }

		Canvas DocumentFilters::MakeOutputCanvas(FILE* file, bool own_file, CanvasType type, const std::wstring& options)
		{
            IGR_Writable_Stream* igr_stream = nullptr;
            Stream::bridge_file(file, own_file, reinterpret_cast<IGR_Stream**>(&igr_stream));
            return MakeOutputCanvas(igr_stream, type, options);
        }

		Canvas DocumentFilters::MakeOutputCanvas(Stream& stream, CanvasType type, const std::wstring& options)
		{
            IGR_Writable_Stream* igr_stream = nullptr;
            Stream::bridge_stream(&stream, false, reinterpret_cast<IGR_Stream**>(&igr_stream));
            return MakeOutputCanvas(igr_stream, type, options);
        }

		Canvas DocumentFilters::MakeOutputCanvas(Stream* stream, bool own_stream, CanvasType type, const std::wstring& options)
		{
            IGR_Writable_Stream* igr_stream = nullptr;
            Stream::bridge_stream(stream, own_stream, reinterpret_cast<IGR_Stream**>(&igr_stream));
            return MakeOutputCanvas(igr_stream, type, options);
        }

		Canvas DocumentFilters::MakeOutputCanvas(IGR_Writable_Stream* stream, CanvasType type, const std::wstring& options)
		{
			Error_Control_Block ecb = { 0 };
            IGR_HCANVAS handle = 0;
            throw_on_error(IGR_Make_Output_Canvas_On(static_cast<int>(type)
                , stream
                , reinterpret_cast<const IGR_UCS2*>(w_to_u16(options).c_str())
				, &handle
                , &ecb), ecb, "IGR_Make_Output_Canvas_On");
            return Canvas(handle);
		}		

	} // namespace DocFilters
} // namespace Hyland
