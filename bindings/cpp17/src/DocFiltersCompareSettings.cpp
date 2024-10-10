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
		CompareSettings::CompareSettings()
			: m_impl(new IGR_Text_Compare_Settings{ sizeof(IGR_Text_Compare_Settings) })
		{
		}

		CompareType CompareSettings::getType() const
		{
			return static_cast<CompareType>(m_impl->compare_type);
		}

		CompareSettings& CompareSettings::setType(CompareType value)
		{
			m_impl->compare_type = static_cast<IGR_COMPARE_DOCUMENTS_COMPARE_TYPE>(value);
			return *this;
		}

		CompareFlags CompareSettings::getFlags() const
		{
			return static_cast<CompareFlags>(m_impl->flags);
		}
		
		CompareSettings& CompareSettings::setFlags(CompareFlags value)
		{
			m_impl->flags = static_cast<IGR_COMPARE_DOCUMENTS_FLAGS_TYPE>(value);
			return *this;
		}

		CompareSettings& CompareSettings::setFlag(CompareFlags value, bool enable)
		{
			if (enable)
				m_impl->flags |= static_cast<IGR_COMPARE_DOCUMENTS_FLAGS_TYPE>(value);
			else
				m_impl->flags &= ~static_cast<IGR_COMPARE_DOCUMENTS_FLAGS_TYPE>(value);
			return *this;
		}

		IGR_Text_Compare_Settings& CompareSettings::data() const
		{
			return *m_impl;
		}

	} // namespace DocFilters
} // namespace Hyland
