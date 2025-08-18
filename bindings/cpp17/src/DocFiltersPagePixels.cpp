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
        class PagePixels::impl_t
        {
        public:
            IGR_HPAGE m_page = 0;
            IGR_Page_Pixels m_pixels = { 0 };

            impl_t(IGR_HPAGE page_handle, const IGR_Page_Pixels& pixels)
                : m_page(page_handle), m_pixels(pixels)
            {
            }
			impl_t(const impl_t&) = delete; 
			impl_t& operator=(const impl_t&) = delete;
			impl_t(impl_t&&) = delete; 
			impl_t& operator=(impl_t&&) = delete;            
            ~impl_t()
            {
                Error_Control_Block ecb = { 0 };
                IGR_Free_Page_Pixels(&m_pixels, &ecb);
            }
        };

        PagePixels::PagePixels(IGR_HPAGE page_handle, const IGR_Page_Pixels& pixels)
            : m_impl(new impl_t(page_handle, pixels))
        {
        }

        const IGR_Page_Pixels* PagePixels::data() const
        {
            return &m_impl->m_pixels;
        }

        size_t PagePixels::getWidth() const
        {
            return m_impl->m_pixels.width;
        }

        size_t PagePixels::getHeight() const
        {
            return m_impl->m_pixels.height;
        }

        size_t PagePixels::getStride() const
        {
            return static_cast<size_t>(m_impl->m_pixels.stride);
        }

        PixelType PagePixels::getType() const
        {
            return static_cast<PixelType>(m_impl->m_pixels.pixel_format);
        }

        uint32_t PagePixels::getFlags() const
        {
            return m_impl->m_pixels.flags;
        }

        const void* PagePixels::getData() const
        {
            return m_impl->m_pixels.scanline0;
        }

        const void* PagePixels::getRow(size_t row) const
        {
            return static_cast<const uint8_t*>(m_impl->m_pixels.scanline0) + row * getStride();
        }

        size_t PagePixels::getColorCount() const
        {
            return m_impl->m_pixels.palette_count;
        }

        Color PagePixels::getColor(size_t index) const
        {
            if (index >= getColorCount())
                throw std::out_of_range("index");
            return Color::from_igr_color(m_impl->m_pixels.palette[index]); // NOLINT

        }
	} // namespace DocFilters
} // namespace Hyland
