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
		class PageElement::impl_t : public std::enable_shared_from_this<PageElement::impl_t>
		{
		public:
			IGR_HPAGE m_page = 0;
			IGR_Page_Element m_element = { sizeof(IGR_Page_Element) };
			std::optional<std::wstring> m_text;
			std::optional<PageElement::style_map_t> m_styles;
			std::shared_ptr<enumerable_t<PageElement>> m_children;
			std::shared_ptr<enumerable_t<PageElement>> m_children_all;

			explicit impl_t(IGR_HPAGE page_handle = 0, const IGR_Page_Element& element = { 0 })
				: m_page(page_handle), m_element(element)
			{
			}

			PageElement getFirstChild()
			{
				Error_Control_Block ecb = { 0 };
				IGR_Page_Element child = { sizeof(IGR_Page_Element) };
				throw_on_error(IGR_Get_Page_Element_First_Child(m_page, &m_element, &child, &ecb), ecb, "IGR_Get_Page_Element_First_Child");
				return PageElement(m_page, child);
			}

			PageElement getNextSibling()
			{
				Error_Control_Block ecb = { 0 };
				IGR_Page_Element next = { sizeof(IGR_Page_Element) };
				throw_on_error(IGR_Get_Page_Element_Next_Sibling(m_page, &m_element, &next, &ecb), ecb, "IGR_Get_Page_Element_Next_Sibling");
				return PageElement(m_page, next);
			}
		};

		PageElement::PageElement()
			: m_impl(new impl_t(0, { 0 }))
		{
		}

		PageElement::PageElement(IGR_HPAGE page_handle, const IGR_Page_Element& element)
			: m_impl(new impl_t(page_handle, element))
		{
		}

		PageElement::PageElement(const std::shared_ptr<impl_t>& impl)
			: m_impl(impl)
		{
		}

		bool PageElement::ok() const
		{
			return m_impl->m_page != 0 && m_impl->m_element.reserved != nullptr;
		}

		bool PageElement::operator==(const PageElement& other) const
		{
			return m_impl->m_element.reserved == other.m_impl->m_element.reserved;
		}

		const IGR_Page_Element* PageElement::data() const
		{
			return ok() ? &m_impl->m_element : nullptr;
		}

		PageElementType PageElement::getType() const
		{
			return static_cast<PageElementType>(m_impl->m_element.type);
		}

		IGR_FRect PageElement::getBounds() const
		{
			return m_impl->m_element.pos;
		}

		uint32_t PageElement::getDepth() const
		{
			return m_impl->m_element.depth;
		}

		uint32_t PageElement::getFlags() const
		{
			return m_impl->m_element.flags;
		}

		PageElement PageElement::getFirstChild() const
		{
			return m_impl->getFirstChild();
		}

		PageElement PageElement::getNextSibling() const
		{
			return m_impl->getNextSibling();
		}

		std::wstring PageElement::getStyle(const std::wstring& name) const
		{
			Error_Control_Block ecb = { 0 };
			std::vector<IGR_UCS2> buffer(4096); // NOLINT
			auto buffer_size = static_cast<IGR_ULONG>(buffer.size());
			throw_on_error(IGR_Get_Page_Element_Style(m_impl->m_page, &m_impl->m_element, reinterpret_cast<const IGR_UCS2*>(w_to_u16(name).c_str()), &buffer_size, &buffer[0], &ecb), ecb, "IGR_Get_Page_Element_Style");
			return u16_to_w(&buffer[0], buffer_size);
		}

		const PageElement::style_map_t& PageElement::getStyles() const
		{
			if (!m_impl->m_styles.has_value())
			{
				m_impl->m_styles.emplace();

				Error_Control_Block ecb = { 0 };
				IGR_PAGE_ELEMENT_STYLES_CALLBACK cb = [](const IGR_UCS2* name, const IGR_UCS2* value, void* context)->IGR_LONG {
					auto& styles = *reinterpret_cast<PageElement::style_map_t*>(context);
					styles[u16_to_w(name)] = u16_to_w(value);
					return IGR_OK;
					};

				throw_on_error(IGR_Get_Page_Element_Styles(m_impl->m_page
					, &m_impl->m_element
					, cb
					, &m_impl->m_styles
					, &ecb), ecb, "IGR_Get_Page_Element_Styles");
			}
			return *m_impl->m_styles;
		}

		std::wstring PageElement::getText() const
		{
			if (!m_impl->m_text.has_value())
			{
				Error_Control_Block ecb = { 0 };
				IGR_ULONG buffer_size = 0;
				// Get the size of the buffer
				throw_on_error(IGR_Get_Page_Element_Text(m_impl->m_page, &m_impl->m_element, &buffer_size, nullptr, &ecb), ecb, "IGR_Get_Page_Element_Text");
				if (buffer_size > 0)
				{
					buffer_size++;
					std::vector<IGR_UCS2> buffer(buffer_size);
					throw_on_error(IGR_Get_Page_Element_Text(m_impl->m_page, &m_impl->m_element, &buffer_size, &buffer[0], &ecb), ecb, "IGR_Get_Page_Element_Text");
					m_impl->m_text = u16_to_w(&buffer[0], buffer_size);
				}
				else
					m_impl->m_text = std::wstring();
			}
			return *m_impl->m_text;
		}

		const PageElement::page_elements_t& PageElement::children() const
		{
			if (!m_impl->m_children)
				m_impl->m_children = std::make_shared<x_enumerable<PageElement, PageElement::impl_t>>(m_impl);
			return *m_impl->m_children;
		}

		const PageElement::page_elements_t& PageElement::allChildren() const
		{
			if (!m_impl->m_children_all)
				m_impl->m_children_all = std::make_shared<x_deep_enumerable<PageElement, PageElement::impl_t>>(m_impl);
			return *m_impl->m_children_all;
		}

		PageElement::const_iterator PageElement::begin() const
		{
			return children().begin();
		}

		PageElement::const_iterator PageElement::end() const
		{
			return children().end();
		}
	} // namespace DocFilters
} // namespace Hyland
