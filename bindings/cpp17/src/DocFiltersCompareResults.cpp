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
		class CompareResults::impl_t
		{
		public:
			IGR_HTEXTCOMPARE m_handle = nullptr;
			CompareResultDifference m_current;
			bool m_eof = false;

			explicit impl_t(IGR_HTEXTCOMPARE handle)
				: m_handle(handle)
			{
			}
			impl_t(const impl_t&) = delete; 
			impl_t& operator=(const impl_t&) = delete;
			impl_t(impl_t&&) = delete; 
			impl_t& operator=(impl_t&&) = delete;			

			~impl_t()
			{
				try {
					Close();
				}
				catch (...) {
				}
			}

			void Close()
			{
				if (m_handle != nullptr)
				{
					Error_Control_Block ecb = { 0 };
					throw_on_error(IGR_Text_Compare_Close(m_handle, &ecb), ecb, "IGR_Text_Compare_Close", "Failed to close compare results");
					m_handle = nullptr;
				}
			}

			[[nodiscard]] 
			IGR_HTEXTCOMPARE need_handle() const
			{
				if (m_handle == nullptr)
					throw DocumentFilters::Error("Invalid compare results handle");
				return  m_handle;
			}
		};

		CompareResults::CompareResults(IGR_HTEXTCOMPARE handle)
			: m_impl(new impl_t(handle))
		{
		}

		void CompareResults::Close()
		{
			m_impl->Close();
		}

		void CompareResults::Reset()
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Text_Compare_Reset(m_impl->need_handle(), &ecb), ecb, "IGR_Text_Compare_Reset", "Failed to reset compare results");
		}

		bool CompareResults::MoveNext()
		{
			return getNext().ok();
		}

		CompareResultDifference CompareResults::getCurrent() const
		{
			return m_impl->m_current;
		}

		CompareResultDifference CompareResults::getNext()
		{
			if (!m_impl->m_eof)
			{
				IGR_Compare_Documents_Difference diff = { sizeof(IGR_Compare_Documents_Difference) };
				Error_Control_Block ecb = { 0 };

				if (IGR_Text_Compare_Next(m_impl->need_handle(), &diff, &ecb) == IGR_OK)
					m_impl->m_current = CompareResultDifference(diff);
				else
				{
					m_impl->m_current = CompareResultDifference();
					m_impl->m_eof = true;
				}
			}
			return m_impl->m_current;
		}
	} // namespace DocFilters
} // namespace Hyland
