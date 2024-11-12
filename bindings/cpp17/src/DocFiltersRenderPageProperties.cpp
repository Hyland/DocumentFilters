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
#include <list>

namespace Hyland
{
	namespace DocFilters
	{
		class RenderPageProperties::impl_t
		{
		public:
			IGR_Render_Page_Properties m_properties = { 0 };
			std::vector<IGR_Render_Page_Redactions> m_redactions;
			std::vector<IGR_Render_Page_Form_Values> m_form_value_recs;
			RenderPageProperties::form_values_t m_form_values;
			std::list<std::u16string> m_strings;

			IGR_Render_Page_Properties* data()
			{
				m_strings.clear();
				m_form_value_recs.resize(m_form_values.size());
				for (size_t i = 0; i < m_form_values.size(); ++i)
				{
					m_strings.push_back(w_to_u16(std::get<0>(m_form_values[i])));
					auto&& name = m_strings.back();
					m_strings.push_back(w_to_u16(std::get<1>(m_form_values[i])));
					auto&& value = m_strings.back();

					m_form_value_recs[i].name = reinterpret_cast<const IGR_UCS2*>(name.c_str());
					m_form_value_recs[i].value = reinterpret_cast<const IGR_UCS2*>(value.c_str());
					m_form_value_recs[i].selected = std::get<2>(m_form_values[i]) ? -1 : 0;
				}

				m_properties.struct_size = sizeof(IGR_Render_Page_Properties);
				m_properties.redaction_count = static_cast<IGR_ULONG>(m_redactions.size());
				m_properties.redactions = m_redactions.data();
				m_properties.form_value_count = static_cast<IGR_ULONG>(m_form_value_recs.size());
				m_properties.form_values = m_form_value_recs.data();

				return &m_properties;
			}
		};

		RenderPageProperties::RenderPageProperties()
			: m_impl(new impl_t())
		{
		}

		bool RenderPageProperties::empty() const
		{
			return m_impl->m_form_values.empty()
				&& m_impl->m_redactions.empty()
				&& m_impl->m_properties.dest_rect.left == m_impl->m_properties.dest_rect.right
				&& m_impl->m_properties.dest_rect.top == m_impl->m_properties.dest_rect.bottom
				&& m_impl->m_properties.source_rect.left == m_impl->m_properties.source_rect.right
				&& m_impl->m_properties.source_rect.top == m_impl->m_properties.source_rect.bottom;
		}

		IGR_Render_Page_Properties* RenderPageProperties::data() const
		{
			return empty()
				? nullptr
				: m_impl->data();
		}

		RenderPageProperties& RenderPageProperties::setSourceRect(const IGR_Rect& value)
		{
			m_impl->m_properties.source_rect = value;
			return *this;
		}
		RenderPageProperties& RenderPageProperties::setSourceRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
		{
			return setSourceRect(IGR_Rect{ left, top, right, bottom });
		}

		RenderPageProperties& RenderPageProperties::setDestRect(const IGR_Rect& value)
		{
			m_impl->m_properties.dest_rect = value;
			return *this;
		}

		RenderPageProperties& RenderPageProperties::setDestRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
		{
			return setDestRect(IGR_Rect{ left, top, right, bottom });
		}

		uint32_t RenderPageProperties::getFlags() const
		{
			return m_impl->m_properties.redaction_flags;
		}

		RenderPageProperties& RenderPageProperties::setFlags(uint32_t value)
		{
			m_impl->m_properties.redaction_flags = value;
			return *this;
		}

		RenderPageProperties& RenderPageProperties::setFlag(RenderPagePropertiesFlags flag, bool val)
		{
			if (val)
				m_impl->m_properties.redaction_flags |= static_cast<uint32_t>(flag);
			else
				m_impl->m_properties.redaction_flags &= ~static_cast<uint32_t>(flag);
			return *this;
		}

		RenderPageProperties& RenderPageProperties::AddRedaction(const RectI32& rect, const Color& c)
		{
			IGR_Render_Page_Redactions redaction = { 0 };
			redaction.x = rect.left;
			redaction.y = rect.top;
			redaction.width = rect.right - rect.left;
			redaction.height = rect.bottom - rect.top;
			redaction.color = c.to_igr_color();
			m_impl->m_redactions.push_back(redaction);
			return *this;
		}

		RenderPageProperties& RenderPageProperties::AddRedaction(int32_t left, int32_t top, int32_t right, int32_t bottom, const Color& c)
		{
			return AddRedaction(IGR_SRect{ left, top, right, bottom }, c);
		}

		RenderPageProperties& RenderPageProperties::AddFormValue(const std::wstring& name, const std::wstring& value, bool selected)
		{
			return AddFormValue(form_value_t{ name, value, selected });
		}

		RenderPageProperties& RenderPageProperties::AddFormValue(const form_value_t& value)
		{
			m_impl->m_form_values.push_back(value);
			return *this;
		}

		const RenderPageProperties::form_values_t& RenderPageProperties::form_values() const
		{
			return m_impl->m_form_values;
		}

		size_t RenderPageProperties::getFormValueCount() const
		{
			return form_values().size();
		}

		const RenderPageProperties::form_value_t& RenderPageProperties::getFormValue(size_t index) const
		{
			return form_values().at(index);
		}
	} // namespace DocFilters
} // namespace Hyland
