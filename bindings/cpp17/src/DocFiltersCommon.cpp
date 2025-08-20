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
#include <sstream>

namespace Hyland
{
	namespace DocFilters
	{
		IGR_RETURN_CODE throw_on_error(IGR_RETURN_CODE code, const Error_Control_Block& ecb, const std::string& function_name, const std::string& error_message)
		{
			if (code == IGR_OK || code == IGR_NO_MORE)
				return code;
			std::stringstream s;
			if (!error_message.empty())
				s << error_message;
			else if (ecb.Msg[0] != 0)
				s << &ecb.Msg[0];
			else
				s << "Error " << code;

			if (!function_name.empty())
				s << " in " << function_name;

			throw DocumentFilters::Error(code, s.str());

			return code;
		}

		uint32_t stoul_or(const std::wstring& s, uint32_t default_value)
		{
			if (s.empty())
				return default_value;
			try {
				size_t e = std::wstring::npos;
				uint32_t res = std::stoul(s, &e, 10); // NOLINT
				return e == s.size() ? res : default_value;
			}
			catch (std::invalid_argument&) {
				return default_value;
			}
			catch (std::out_of_range&) {
				return default_value;
			}
		}

		double stod_or(const std::wstring& s, double default_value)
		{
			if (s.empty())
				return default_value;
			try {
				size_t e = std::wstring::npos;
				double res = std::stod(s, &e);
				return e == s.size() ? res : default_value;
			}
			catch (std::invalid_argument&) {
				return default_value;
			}
			catch (std::out_of_range&) {
				return default_value;
			}
		}

		std::vector<std::wstring> split(const std::wstring& str, const std::wstring& delimiters)
		{
			std::vector<std::wstring> result;
			size_t start = 0;
			size_t end = 0;

			while ((end = str.find_first_of(delimiters, start)) != std::wstring::npos)
			{
				if (end != start)
					result.push_back(str.substr(start, end - start));
				start = end + 1;
			}

			// Add the last substring if there is any
			if (start < str.size())
				result.push_back(str.substr(start));

			return result;
		}

		std::string base64_encode(const void* data, std::size_t length)
		{
			static const std::string base64_chars =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz"
				"0123456789+/";

			std::string encoded;
			int val = 0;
			int valb = -6; // NOLINT

			for (std::size_t i = 0; i < length; ++i) {
				val = (val << 8) + reinterpret_cast<const uint8_t*>(data)[i]; // NOLINT
				valb += 8; // NOLINT
				while (valb >= 0)
				{
					encoded.push_back(base64_chars[(val >> valb) & 0x3F]); // NOLINT
					valb -= 6; // NOLINT
				}
			}

			if (valb > -6) // NOLINT
				encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]); // NOLINT

			while (encoded.size() % 4) // NOLINT
				encoded.push_back('=');

			return encoded;
		}


		class subfile_enumerator_t : public enumerable_t<Subfile>::enumerator_t
		{
		private:
			IGR_LONG m_owning_doc = 0;
			handle_holder_t<HSUBFILES> m_handle;
			subfile_enumerable_t::stream_opener_t m_opener;
			IGR_Subfile_Info m_current = {};
			std::u16string m_name;
			std::u16string m_id;
			std::u16string m_comment;

		public:
			explicit subfile_enumerator_t(IGR_LONG owning_doc, HSUBFILES handle, const subfile_enumerable_t::stream_opener_t& opener) // NOLINT
				: m_owning_doc(owning_doc)
				, m_handle(handle, &IGR_Subfiles_Close)
				, m_opener(opener)
				, m_name(1024, ' ') // NOLINT
				, m_id(2048, ' ') // NOLINT
				, m_comment(1024, ' ') // NOLINT
			{
				m_current.struct_size = sizeof(m_current);
				m_current.name = reinterpret_cast<IGR_UCS2*>(&m_name[0]);
				m_current.name_size = static_cast<IGR_ULONG>(m_name.size());
				m_current.id = reinterpret_cast<IGR_UCS2*>(&m_id[0]);
				m_current.id_size = static_cast<IGR_ULONG>(m_id.size());
				m_current.comment = reinterpret_cast<IGR_UCS2*>(&m_comment[0]);
				m_current.comment_size = static_cast<IGR_ULONG>(m_comment.size());
			}
			subfile_enumerator_t(const subfile_enumerator_t&) = delete;
			subfile_enumerator_t& operator=(const subfile_enumerator_t&) = delete;
			subfile_enumerator_t(subfile_enumerator_t&&) = delete;
			subfile_enumerator_t& operator=(subfile_enumerator_t&&) = delete;
			~subfile_enumerator_t() override = default;

			bool move_next() override
			{
				Error_Control_Block ecb = { 0 };
				return IGR_Subfiles_Next_Ex(m_handle.getHandle(), &m_current, &ecb) == IGR_OK;
			}
			[[nodiscard]] 
			Subfile current() const override
			{
				return Subfile(m_owning_doc, m_current, m_opener);
			}
		};

		subfile_enumerable_t::subfile_enumerable_t(IGR_LONG doc, const enumerator_creator_t& creator, const stream_opener_t& opener) // NOLINT
			: m_creator(creator)
			, m_opener(opener)
			, m_doc(doc)
		{
		}

		std::shared_ptr<enumerable_t<Subfile>::enumerator_t> subfile_enumerable_t::get_enumerator() const
		{
			Error_Control_Block ecb = { 0 };
			HSUBFILES subs = nullptr;
			throw_on_error(m_creator(m_doc, &subs, &ecb), ecb, "IGR_Subfiles_Open");
			return std::make_shared<subfile_enumerator_t>(m_doc, subs, m_opener);
		}
	} // namespace DocFilters
} // namespace Hyland
