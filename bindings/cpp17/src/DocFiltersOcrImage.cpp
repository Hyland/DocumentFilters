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
#define HAS_METHOD(payload, name) \
		((payload) && payload->struct_size >= offsetof(IGR_Open_Callback_Action_OCR_Image, name) + sizeof(void*) && payload->name)


		OcrImage::OcrImage(IGR_Open_Callback_Action_OCR_Image* dest)
			: m_dest(dest)
		{
			if (m_dest == nullptr)
				throw std::runtime_error("OcrImage constructor called with null pointer");
		}

		const IGR_Open_Callback_Action_OCR_Image* OcrImage::raw() const
		{
			return m_dest;
		}

		const IGR_Open_DIB_Info* OcrImage::getPixelData() const
		{
			return m_dest->source_image_pixels;
		}

		size_t OcrImage::getSourcePageIndex() const
		{
			return m_dest->source_page_index;
		}

		Rect OcrImage::getSourceRect() const
		{
			return Rect::ltrb(m_dest->source_rect.left, m_dest->source_rect.top, m_dest->source_rect.right, m_dest->source_rect.bottom);
		}
		
		void OcrImage::SaveImage(const std::wstring& filename, const std::wstring& mimeType) const
		{
			if (HAS_METHOD(m_dest, SaveImage))
			{
				const auto&& u16_filename = w_to_u16(filename);
				const auto&& u16_mimeType = w_to_u16(mimeType);
				m_dest->SaveImage(m_dest, reinterpret_cast<const IGR_UCS2*>(u16_filename.c_str()), reinterpret_cast<const IGR_UCS2*>(u16_mimeType.c_str()));
			}
			else
				throw std::runtime_error("SaveImage method not available");
		}
		
		void OcrImage::StartBlock(int blockType, const Rect& rect) const
		{
			StartBlock(blockType, rect.to_quadpoint());
		}
		
		void OcrImage::StartBlock(int blockType, const IGR_QuadPoint& quad) const
		{
			if (HAS_METHOD(m_dest, StartBlock))
				m_dest->StartBlock(m_dest, blockType, &quad);
			else
				throw std::runtime_error("StartBlock method not available");
		}

		void OcrImage::EndBlock(int blockType) const
		{
			if (HAS_METHOD(m_dest, EndBlock))
				m_dest->EndBlock(m_dest, blockType);
			else
				throw std::runtime_error("EndBlock method not available");
		}

		void OcrImage::AddText(const std::wstring& text, const Rect& rect, uint32_t flags, const OcrStyleInfo& style) const
		{
			AddText(text, rect.to_quadpoint(), flags, style);
		}

		void OcrImage::AddText(const std::wstring& text, const IGR_QuadPoint& quad, uint32_t flags, const OcrStyleInfo& style) const
		{
			if (HAS_METHOD(m_dest, AddText))
			{
				const auto&& u16_text = w_to_u16(text);

				m_dest->AddText(m_dest, reinterpret_cast<const IGR_UCS2*>(u16_text.c_str()), &quad, flags, &style.raw());
			}
			else
				throw std::runtime_error("AddText method not available");
		}

		void OcrImage::Reorient(float angle) const
		{
			if (HAS_METHOD(m_dest, Reorient))
				m_dest->Reorient(m_dest, angle);
			else
				throw std::runtime_error("Reorient method not available");
		}

	} // namespace DocFilters
} // namespace Hyland
