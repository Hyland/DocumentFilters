/*
# (c) 2025 Hyland Software, Inc. and its affiliates. All rights reserved.

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
		OcrStyleInfo::OcrStyleInfo()
			: m_style{}
		{
			m_style.struct_size = sizeof(IGR_Open_Callback_Action_OCR_Image_Style_Info);
			m_style.font_size = 12;
		}

		std::wstring OcrStyleInfo::getFontFamily() const
		{
			return u16_to_w(m_style.font_family);
		}

		OcrStyleInfo& OcrStyleInfo::setFontFamily(const std::wstring& fontFamily)
		{
			const auto&& u16 = w_to_u16(fontFamily);
			m_style.font_family[u16.copy(reinterpret_cast<char16_t*>(m_style.font_family), (sizeof(m_style.font_family) / sizeof(m_style.font_family[0])) - 1)] = 0;
			return *this;
		}

		float OcrStyleInfo::getFontSize() const
		{
			return m_style.font_size;
		}

		OcrStyleInfo& OcrStyleInfo::setFontSize(float fontSize)
		{
			m_style.font_size = fontSize;
			return *this;
		}

		OcrStyleInfo::TextStyle OcrStyleInfo::getTextStyle() const
		{
			return static_cast<TextStyle>(m_style.text_style);
		}

		OcrStyleInfo& OcrStyleInfo::setTextStyle(TextStyle textStyle)
		{
			m_style.text_style = static_cast<uint32_t>(textStyle);
			return *this;
		}

		const IGR_Open_Callback_Action_OCR_Image_Style_Info& OcrStyleInfo::raw() const
		{
			return m_style;
		}

	} // namespace DocFilters
} // namespace Hyland
