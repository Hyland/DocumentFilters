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
        class Hyperlink::impl_t
        {
        public:
            IGR_Hyperlink m_hyperlink;

            explicit impl_t(const IGR_Hyperlink& Hyperlink = {})
                : m_hyperlink(Hyperlink)
            {
            }
            std::wstring get_str(int what, size_t max_length = 4096) // NOLINT
            {
                std::vector<IGR_UCS2> buffer(max_length);
                Error_Control_Block ecb = { 0 };
                throw_on_error(IGR_Get_Page_Hyperlink_Str(&m_hyperlink, what, static_cast<IGR_LONG>(max_length), &buffer[0], &ecb), ecb, "IGR_Get_Page_Hyperlink_Str");
                return u16_to_w(&buffer[0]);
            }
        };

        Hyperlink::Hyperlink(const IGR_Hyperlink& Hyperlink)
            : m_impl(new impl_t(Hyperlink))
        {
        }

        Hyperlink::Hyperlink()
            : m_impl(new impl_t())
        {
        }

        Hyperlink::Type Hyperlink::get_type() const
        {
            return static_cast<Hyperlink::Type>(m_impl->m_hyperlink.type);
        }

        int Hyperlink::getX() const
        {
            return m_impl->m_hyperlink.x;
        }

        int Hyperlink::getY() const
        {
            return m_impl->m_hyperlink.y;
        }

        int Hyperlink::getWidth() const
        {
            return m_impl->m_hyperlink.width;
        }

        int Hyperlink::getHeight() const
        {
            return m_impl->m_hyperlink.height;
        }

        RectI32 Hyperlink::getBounds() const
        {
            return RectI32::xywh(m_impl->m_hyperlink.x, m_impl->m_hyperlink.y, m_impl->m_hyperlink.width, m_impl->m_hyperlink.height);
        }

        Hyperlink::Fit Hyperlink::getDestFit() const
        {
            return static_cast<Hyperlink::Fit>(m_impl->m_hyperlink.dest_fit);
        }

        int Hyperlink::getPageNumber() const
        {
            return m_impl->m_hyperlink.page_number;
        }

        int Hyperlink::getDestLeft() const
        {
            return m_impl->m_hyperlink.dest_left;
        }

        int Hyperlink::getDestTop() const
        {
            return m_impl->m_hyperlink.dest_top;
        }

        int Hyperlink::getDestRight() const
        {
            return m_impl->m_hyperlink.dest_right;
        }

        int Hyperlink::getDestBottom() const
        {
            return m_impl->m_hyperlink.dest_bottom;
        }

        RectI32 Hyperlink::getDestRect() const
        {
            return RectI32::ltrb(m_impl->m_hyperlink.dest_left, m_impl->m_hyperlink.dest_top, m_impl->m_hyperlink.dest_right, m_impl->m_hyperlink.dest_bottom);
        }

        Hyperlink::Flags Hyperlink::getFlags() const
        {
            return static_cast<Flags>(m_impl->m_hyperlink.flags);
        }

        bool Hyperlink::getFlag(Flags flag) const
        {
            return static_cast<size_t>(getFlags() & flag) != 0;
        }

        std::wstring Hyperlink::getRef() const
        {
            return m_impl->get_str(IGR_HYPERLINK_GET_VALUE_REF);
        }

        std::wstring Hyperlink::getUri() const
        {
            return m_impl->get_str(IGR_HYPERLINK_GET_VALUE_URI);
        }
	} // namespace DocFilters
} // namespace Hyland
