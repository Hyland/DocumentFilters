/*
# (c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.

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
#include "DocFiltersCommon.h"
#include "DocumentFiltersObjects.h"

namespace //anonymous
{
#define HAS_METHOD(payload, name) \
	((payload) && payload->struct_size >= offsetof(IGR_Open_Callback_Action_Describe_Image, name) + sizeof(void*) && payload->name)

	void saveImage(IGR_Open_Callback_Action_Describe_Image* dest, const std::u16string& filename, const std::u16string& mimeType)
	{
		if (!HAS_METHOD(dest, SaveImage))
			throw std::runtime_error("SaveImage method not available");

		auto rc = dest->SaveImage(dest, reinterpret_cast<const IGR_UCS2*>(filename.c_str()),
		                          reinterpret_cast<const IGR_UCS2*>(mimeType.c_str()));
		if (rc != IGR_OK)
			throw std::runtime_error(std::string("SaveImage failed (rc=") + std::to_string(static_cast<int>(rc)) + ")");
	}
	void addText(IGR_Open_Callback_Action_Describe_Image* dest, const std::u16string& text, const IGR_ULONG flags)
	{
		if (!HAS_METHOD(dest, AddText))
			throw std::runtime_error("AddText method not available");

		auto rc = dest->AddText(dest, reinterpret_cast<const IGR_UCS2*>(text.c_str()), flags);
		if (rc != IGR_OK)
			throw std::runtime_error(std::string("AddText failed (rc=") + std::to_string(static_cast<int>(rc)) + ")");
	}
} // namespace

namespace Hyland
{
	namespace DocFilters
	{
		DescribeImage::DescribeImage(IGR_Open_Callback_Action_Describe_Image* dest)
		    : m_dest(dest)
		{
			if (m_dest == nullptr)
				throw std::runtime_error("DescribeImage constructor called with null pointer");
		}

		const IGR_Open_Callback_Action_Describe_Image* DescribeImage::raw() const
		{
			return m_dest;
		}

		const IGR_Open_DIB_Info* DescribeImage::getPixelData() const
		{
			if (!HAS_METHOD(m_dest, GetSourceImagePixels))
				throw std::runtime_error("GetSourceImagePixels method not available");

			return m_dest->GetSourceImagePixels(m_dest);
		}

		size_t DescribeImage::getSourcePageIndex() const
		{
			return m_dest->source_page_index;
		}

		Rect DescribeImage::getSourceRect() const
		{
			return Rect::ltrb(m_dest->source_rect.left, m_dest->source_rect.top, m_dest->source_rect.right, m_dest->source_rect.bottom);
		}

		std::wstring DescribeImage::getSourceName() const
		{
			return u16_to_w(m_dest->source_name, sizeof(m_dest->source_name) / sizeof(m_dest->source_name[0]));
		}

		std::wstring DescribeImage::getExistingAltText() const
		{
			return u16_to_w(m_dest->existing_alt_text, sizeof(m_dest->existing_alt_text) / sizeof(m_dest->existing_alt_text[0]));
		}
		IGR_LONG DescribeImage::getSourceType() const
		{
			return m_dest->source_type;
		}
		void DescribeImage::SaveImage(const std::wstring& filename, const std::wstring& mimeType) const
		{
			saveImage(m_dest, w_to_u16(filename), w_to_u16(mimeType));
		}
		void DescribeImage::SaveImage(const std::string& filename, const std::string& mimeType) const
		{
			saveImage(m_dest, u8_to_u16(filename), u8_to_u16(mimeType));
		}

		void DescribeImage::AddText(const std::wstring& text, const IGR_ULONG flags) const
		{
			addText(m_dest, w_to_u16(text), flags);
		}
		void DescribeImage::AddText(const std::string& text, const IGR_ULONG flags) const
		{
			addText(m_dest, u8_to_u16(text), flags);
		}
	} // namespace DocFilters
} // namespace Hyland
