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
		class CompareResultDifference::impl_t
		{
		public:
			IGR_Compare_Documents_Difference m_difference;
			std::shared_ptr<CompareResultDifference::details_t> m_details_enum;

			explicit impl_t(const IGR_Compare_Documents_Difference& difference)
				: m_difference(difference)
			{
			}
			impl_t(const impl_t&) = delete; 
			impl_t& operator=(const impl_t&) = delete;
			impl_t(impl_t&&) = delete; 
			impl_t& operator=(impl_t&&) = delete;

			~impl_t()
			{
				Error_Control_Block ecb = { 0 };
				if (m_difference.items != nullptr)
					IGR_Text_Compare_Difference_Dispose(&m_difference, &ecb);
			}
		};

		CompareResultDifference::CompareResultDifference()
			: m_impl(new impl_t(IGR_Compare_Documents_Difference{}))
		{
		}

		CompareResultDifference::CompareResultDifference(const IGR_Compare_Documents_Difference& ref)
			: m_impl(new impl_t(ref))
		{
		}

		bool CompareResultDifference::ok() const
		{
			return m_impl->m_difference.items != nullptr;
		}

		DifferenceType CompareResultDifference::getType() const
		{
			return static_cast<DifferenceType>(m_impl->m_difference.type);
		}

		DifferenceSource CompareResultDifference::getSource() const
		{
			return static_cast<DifferenceSource>(m_impl->m_difference.doc_source);
		}

		uint32_t CompareResultDifference::getOriginalPageIndex() const
		{
			return m_impl->m_difference.original_page_index;
		}

		uint32_t CompareResultDifference::getRevisedPageIndex() const
		{
			return m_impl->m_difference.revised_page_index;
		}

		std::wstring CompareResultDifference::getText() const
		{
			std::wstring result;

			IGR_FRect last = { 0 };
			for (size_t i = 0; i < m_impl->m_difference.item_count; ++i)
			{
				if (i > 0)
				{
					if (m_impl->m_difference.items[i].bounds.top > last.top)
						result += L"\n";
					else if (m_impl->m_difference.items[i].bounds.left - 2 > last.right)
						result += L" ";
				}
				result += u16_to_w(m_impl->m_difference.items[i].text);
				last = m_impl->m_difference.items[i].bounds;
			}
			return result;
		}

		size_t CompareResultDifference::size() const
		{
			return m_impl->m_difference.item_count;
		}

		CompareResultDifferenceDetail CompareResultDifference::operator[](size_t index) const
		{
			return at(index);
		}

		CompareResultDifferenceDetail CompareResultDifference::at(size_t index) const
		{
			if (index >= m_impl->m_difference.item_count)
				throw std::out_of_range("index out of range");

			return CompareResultDifferenceDetail(m_impl->m_difference.items[index]);
		}

		CompareResultDifference::details_t& CompareResultDifference::details() const
		{
			if (!m_impl->m_details_enum)
			{
				m_impl->m_details_enum = std::make_shared<CompareResultDifference::details_t>(size(), [this](size_t index) { return at(index); });
			}
			return *m_impl->m_details_enum;
		}

		CompareResultDifference::const_iterator CompareResultDifference::begin() const
		{ 
			return details().begin();
		}
		
		CompareResultDifference::const_iterator CompareResultDifference::end() const
		{
			return details().end();
		}
	} // namespace DocFilters
} // namespace Hyland
