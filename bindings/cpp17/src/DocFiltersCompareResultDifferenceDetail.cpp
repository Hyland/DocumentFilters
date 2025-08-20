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
		class CompareResultDifferenceDetail::impl_t 
		{
		public:
			std::wstring m_text;
			IGR_FRect m_bounds = { 0,0,0,0 };
			uint32_t m_pageIndex = 0;
			bool m_ok = false;

		};

		CompareResultDifferenceDetail::CompareResultDifferenceDetail()
			: m_impl(new impl_t())
		{
		}
		
		CompareResultDifferenceDetail::CompareResultDifferenceDetail(const IGR_Compare_Documents_Difference_Item& ref)
			: CompareResultDifferenceDetail()
		{
			m_impl->m_text = u16_to_w(ref.text);
			m_impl->m_bounds = ref.bounds;
			m_impl->m_pageIndex = ref.page_index;
			m_impl->m_ok = true;
		}
		
		bool CompareResultDifferenceDetail::ok() const
		{
			return m_impl->m_ok;
		}

		uint32_t CompareResultDifferenceDetail::getPageIndex() const
		{
			return m_impl->m_pageIndex;
		}

		RectF CompareResultDifferenceDetail::getBounds() const
		{
			return m_impl->m_bounds;
		}
		
		std::wstring CompareResultDifferenceDetail::getText() const
		{
			return  m_impl->m_text;
		}
	} // namespace DocFilters
} // namespace Hyland
