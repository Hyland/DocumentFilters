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

namespace Hyland
{
	namespace DocFilters
	{
		Format::Format(uint32_t id)
			: m_id(id)
		{
			if (Fetch(id, What::LongName, m_display_name))
			{
				Fetch(id, What::ShortName, m_short_name);
				Fetch(id, What::ConfigName, m_config_name);
				Fetch(id, What::MimeType, m_mime_type);
				Fetch(id, What::FileCategory, m_file_type_category);
			}
			else
				m_id = static_cast<uint32_t>(-1);
		}

		bool Format::Fetch(uint32_t id, What what, std::wstring& result)
		{
			Error_Control_Block ecb = { 0 };
			std::vector<char> buffer(4096); // NOLINT
			if (IGR_Get_Format_Attribute(static_cast<int>(id), static_cast<int>(what), &buffer[0], &ecb) == 0)
			{
				result = u8_to_w(&buffer[0]);
				return true;
			}
			return false;
		}

		bool Format::Fetch(uint32_t id, What what, uint32_t& result)
		{
			std::wstring temp;
			if (Fetch(id, what, temp))
			{
				result = std::stoul(w_to_u8(temp));
				return true;
			}
			return false;
		}
	} // namespace DocFilters
} // namespace Hyland
