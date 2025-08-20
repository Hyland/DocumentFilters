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
		CompareDocumentSettings::CompareDocumentSettings()
			: m_impl(new IGR_Text_Compare_Document_Source{ sizeof(IGR_Text_Compare_Document_Source) })
		{
		}

		uint32_t CompareDocumentSettings::getFirstPage() const
		{
			return m_impl->doc_first_page;
		}
		CompareDocumentSettings& CompareDocumentSettings::setFirstPage(uint32_t value)
		{
			m_impl->doc_first_page = value;
			return *this;
		}

		uint32_t CompareDocumentSettings::getPageCount() const
		{
			return m_impl->doc_page_count;
		}

		CompareDocumentSettings& CompareDocumentSettings::setPageCount(uint32_t value)
		{
			m_impl->doc_page_count = value;
			return *this;
		}

		RectF CompareDocumentSettings::getMargins() const
		{
			return RectF(m_impl->doc_margins);
		}

		CompareDocumentSettings& CompareDocumentSettings::setMargins(const RectF& margins)
		{
			m_impl->doc_margins = margins;
			return *this;
		}
		
		CompareDocumentSettings& CompareDocumentSettings::setMargins(float left, float top, float right, float bottom)
		{
			return setMargins(RectF(left, top, right, bottom));
		}
	} // namespace DocFilters
} // namespace Hyland
