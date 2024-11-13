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
		Subfile::Subfile()
			: Extractor(nullptr)
		{
		}

		Subfile::Subfile(IGR_LONG doc_handle, const std::wstring& id, IGR_Stream* Stream)
			: Extractor(Stream), m_owning_doc(doc_handle), m_id(id)
		{
		}

		Subfile::Subfile(IGR_LONG doc_handle, const IGR_Subfile_Info& info, const stream_opener_t& stream_opener)
			: Extractor(nullptr), m_owning_doc(doc_handle), m_stream_opener(stream_opener)
		{
			m_id = u16_to_w(info.id, info.id_size);
			m_name = u16_to_w(info.name, info.name_size);
			m_size = info.size;
			m_flags = info.flags;
			m_comment = u16_to_w(info.comment, info.comment_size);
			m_file_date = info.date;
		}

		IGR_Stream* Subfile::resolve_stream() const
		{
			Error_Control_Block ecb = { 0 };
			IGR_Stream* res = nullptr;
			throw_on_error(m_stream_opener(m_owning_doc, reinterpret_cast<const IGR_UCS2*>(w_to_u16(m_id).c_str()), &res, &ecb), ecb, "IGR_Extract_Subfile_Stream");
			return res;
		}
		std::wstring Subfile::getId() const
		{
			return m_id;
		}

		std::wstring Subfile::getName() const
		{
			return m_name;
		}

		uint64_t Subfile::getSize() const
		{
			return m_size;
		}

		uint32_t Subfile::getFlags() const
		{
			return m_flags;
		}

		bool Subfile::getIsEncrypted() const
		{
			return (getFlags() & IGR_SUBFILE_INFO_FLAG_PASSWORD_PROTECTED) != 0; // NOLINT
		}

		std::wstring Subfile::getComment() const
		{
			return m_comment;
		}

		DateTime Subfile::getFileDate() const
		{
			return m_file_date;
		}

	} // namespace DocFilters
} // namespace Hyland
