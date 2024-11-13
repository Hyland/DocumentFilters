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
		class FormElement::impl_t
		{
		public:
			IGR_Page_Form_Element m_element;
			std::optional<std::vector<FormElement::Option> > m_options;

			explicit impl_t(const IGR_Page_Form_Element& element = IGR_Page_Form_Element())
				: m_element(element)
			{
			}
			impl_t(const impl_t&) = delete; 
			impl_t& operator=(const impl_t&) = delete;
			impl_t(impl_t&&) = delete; 
			impl_t& operator=(impl_t&&) = delete;
			~impl_t() = default;

			[[nodiscard]]
			std::wstring get_opt_str(size_t opt, int type) const
			{
				Error_Control_Block ecb = { 0 };
				std::vector<IGR_UCS2> buffer(4096); // NOLINT
				auto buffer_size = static_cast<IGR_ULONG>(buffer.size());
				throw_on_error(IGR_Get_Page_Form_Element_Option_Str(&m_element, type, static_cast<int>(opt), buffer_size, &buffer[0], &ecb), ecb, "IGR_Get_Page_Form_Element_Option_Str");
				return u16_to_w(&buffer[0], buffer_size);
			}

			std::vector<FormElement::Option>& need_options()
			{
				if (!m_options.has_value())
				{
					m_options.emplace();
					if (m_element.reserved != nullptr)
					{
						for (size_t i = 0; i < m_element.option_count; ++i)
						{
							m_options->emplace_back(get_opt_str(i, IGR_PAGE_FORM_ELEMENT_GET_NAME)
								, get_opt_str(i, IGR_PAGE_FORM_ELEMENT_GET_VALUE)
								, get_opt_str(i, IGR_PAGE_FORM_ELEMENT_GET_SELECTED) == L"1");
						}
					}
				}
				return *m_options;
			}

		};

		FormElement::Option::Option(const std::wstring& name, const std::wstring& value, bool selected)
			: m_name(name)
			, m_value(value)
			, m_selected(selected)
		{
		}

		FormElement::FormElement()
			: m_impl(new impl_t())
		{

		}

		FormElement::FormElement(const IGR_Page_Form_Element& element)
			: m_impl(new impl_t(element))
		{
		}

		FormElementType FormElement::get_type() const
		{
			return static_cast<FormElementType>(m_impl->m_element.type);
		}

		uint32_t FormElement::getFlags() const
		{
			return m_impl->m_element.flags;
		}

		int32_t FormElement::getX() const
		{
			return m_impl->m_element.x;
		}

		int32_t FormElement::getY() const
		{
			return m_impl->m_element.y;
		}

		int32_t FormElement::getWidth() const
		{
			return m_impl->m_element.width;
		}

		int32_t FormElement::getHeight() const
		{
			return m_impl->m_element.height;
		}

		RectI32 FormElement::getBounds() const
		{
			return RectI32::xywh(m_impl->m_element.x, m_impl->m_element.y, m_impl->m_element.width, m_impl->m_element.height);
		}

		std::wstring FormElement::getName() const
		{
			return get_str(IGR_PAGE_FORM_ELEMENT_GET_NAME);
		}

		std::wstring FormElement::getValue() const
		{
			return get_str(IGR_PAGE_FORM_ELEMENT_GET_VALUE);
		}

		std::wstring FormElement::getCaption() const
		{
			return get_str(IGR_PAGE_FORM_ELEMENT_GET_CAPTION);
		}

		std::wstring FormElement::getFontName() const
		{
			return get_str(IGR_PAGE_FORM_ELEMENT_GET_FONT_NAME);
		}

		std::wstring FormElement::getAction() const
		{
			return get_str(IGR_PAGE_FORM_ELEMENT_GET_ACTION);
		}

		std::wstring FormElement::getActionDest() const
		{
			return get_str(IGR_PAGE_FORM_ELEMENT_GET_ACTION_DEST);
		}

		int FormElement::getFontSize() const
		{
			return m_impl->m_element.fontSize;
		}

		int FormElement::getSelected() const
		{
			return m_impl->m_element.selectedItem;
		}

		const FormElement::options_t& FormElement::getOptions() const
		{
			return m_impl->need_options();
		}

		std::wstring FormElement::get_str(int type) const
		{
			Error_Control_Block ecb = { 0 };
			std::vector<IGR_UCS2> buffer(4096); // NOLINT
			auto buffer_size = static_cast<IGR_ULONG>(buffer.size());
			throw_on_error(IGR_Get_Page_Form_Element_Str(&m_impl->m_element, type, buffer_size, &buffer[0], &ecb), ecb, "IGR_Page_Form_Element_Get_String");
			return u16_to_w(&buffer[0], buffer_size);
		}
	} // namespace DocFilters
} // namespace Hyland
