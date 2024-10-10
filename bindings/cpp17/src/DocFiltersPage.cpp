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
		class Page::impl_t
		{
		public:
			handle_holder_t<IGR_HPAGE> m_handle;
			size_t m_page_index = 0;
			IGR_LONG m_width = 0;
			IGR_LONG m_height = 0;
			IGR_LONG m_word_count = 0;
			std::optional<std::wstring> m_text;
			std::optional<std::vector<Word>> m_words;
			Page::words_t m_words_loader;
			std::shared_ptr<subfile_enumerable_t> m_images;
			std::optional<PageElement> m_root_page_element;
			std::optional<std::vector<FormElement>> m_form_elements;
			std::optional<std::vector<Hyperlink>> m_hyperlinks;
			std::optional<std::vector<std::shared_ptr<Annotation>>> m_annotations;
			Page::annotations_t m_annotations_loader;

			impl_t(IGR_HPAGE page_handle, size_t page_index)
				: m_handle(page_handle, &IGR_Close_Page), m_page_index(page_index)
			{
				if (page_handle != 0)
				{
					Error_Control_Block ecb = { 0 };
					throw_on_error(IGR_Get_Page_Dimensions(page_handle, &m_width, &m_height, &ecb), ecb, "IGR_Get_Page_Dimensions");
					throw_on_error(IGR_Get_Page_Word_Count(page_handle, &m_word_count, &ecb), ecb, "IGR_Get_Page_Word_Count");
				}
			}

			[[nodiscard]]
			IGR_HPAGE need_handle() const
			{
				if (m_handle.getHandle() == 0)
					throw std::runtime_error("Page is not initialized");

				return m_handle.getHandle();
			}

			std::wstring getText()
			{
				if (!m_text.has_value())
				{
					Error_Control_Block ecb = { 0 };
					std::vector<IGR_UCS2> buffer(1024); // NOLINT
					auto len = static_cast<IGR_LONG>(buffer.size());

					std::wstring res;
					while (IGR_Get_Page_Text(m_handle.getHandle(), &buffer[0], &len, &ecb) == IGR_OK)
					{
						res += u16_to_w(&buffer[0], len);
						len = static_cast<IGR_LONG>(buffer.size());
					}

					m_text = res;
				}
				return *m_text;
			}
			const std::vector<Word>& need_words()
			{
				if (!m_words.has_value())
				{
					auto&& dest = m_words.emplace();
					dest.reserve(m_word_count);

					Error_Control_Block ecb = { 0 };
					IGR_Page_Word w = { 0 };

					for (size_t i = 0, c = static_cast<size_t>(m_word_count); i < c; ++i)
					{
						IGR_LONG req = 1;
						if (IGR_Get_Page_Words(m_handle.getHandle(), static_cast<IGR_LONG>(i), &req, &w, &ecb) == IGR_OK)
							dest.emplace_back(Word(w, i));
						else
							break;
					}

					m_words_loader = words_t(dest.size(), [&dest](size_t index) -> Word { return dest[index]; });
				}
				return *m_words;
			}
			const std::vector<FormElement>& need_form_elements()
			{
				if (!m_form_elements.has_value())
				{
					m_form_elements.emplace();

					Error_Control_Block ecb = { 0 };

					IGR_LONG count = 0;
					throw_on_error(IGR_Get_Page_Form_Element_Count(m_handle.getHandle(), &count, &ecb), ecb, "IGR_Get_Page_Form_Element_Count");
					m_form_elements->reserve(count);

					for (IGR_LONG i = 0; i < count; ++i)
					{
						IGR_Page_Form_Element fe = { nullptr };
						IGR_LONG req = 1;
						throw_on_error(IGR_Get_Page_Form_Elements(m_handle.getHandle(), i, &req, &fe, &ecb), ecb, "IGR_Get_Page_Form_Element");
						if (req == 1)
							m_form_elements->emplace_back(FormElement(fe));
					}
				}
				return *m_form_elements;
			}
			const std::vector<Hyperlink>& need_hyperlinks()
			{
				if (!m_hyperlinks.has_value())
				{
					m_hyperlinks.emplace();

					Error_Control_Block ecb = { 0 };

					IGR_LONG count = 0;
					throw_on_error(IGR_Get_Page_Hyperlink_Count(m_handle.getHandle(), &count, &ecb), ecb, "IGR_Get_Page_Form_Element_Count");
					m_hyperlinks->reserve(count);

					for (IGR_LONG i = 0; i < count; ++i)
					{
						IGR_Hyperlink link = { nullptr };
						IGR_LONG req = 1;
						throw_on_error(IGR_Get_Page_Hyperlinks(m_handle.getHandle(), i, &req, &link, &ecb), ecb, "IGR_Get_Page_Form_Element");
						if (req == 1)
							m_hyperlinks->emplace_back(Hyperlink(link));
					}
				}
				return *m_hyperlinks;
			}

			void need_annotations()
			{
				if (!m_annotations.has_value())
				{
					m_annotations.emplace();
					auto&& dest = *m_annotations;

					Error_Control_Block ecb = { 0 };

					IGR_LONG count = 0;
					throw_on_error(IGR_Get_Page_Annotation_Count(m_handle.getHandle(), &count, &ecb), ecb, "IGR_Get_Page_Annotation_Count");
					m_annotations->reserve(count);

					for (IGR_LONG i = 0; i < count; ++i)
					{
						IGR_Annotation anno = { nullptr };
						IGR_LONG req = 1;

						throw_on_error(IGR_Get_Page_Annotations(m_handle.getHandle(), i, &req, &anno, &ecb), ecb, "IGR_Get_Page_Form_Element");
						if (req != 1)
							continue;

						auto a = Annotation::make(anno);
						if (!a)
							continue;

						m_annotations->push_back(std::shared_ptr<Annotation>(a.release()));
					}

					m_annotations_loader = annotations_t(dest.size(), [&dest](size_t index) -> std::shared_ptr<Annotation> { return dest[index]; });
				}
			}
		};

		Page::Page(IGR_HPAGE page_handle, size_t index)
			: m_impl(new impl_t(page_handle, index))
		{
		}

		Page::Page()
			: m_impl(new impl_t(0, 0))
		{
		}

		size_t Page::getIndex() const
		{
			return m_impl->m_page_index;
		}

		IGR_HPAGE Page::getHandle() const
		{
			return m_impl->m_handle.getHandle();
		}

		uint32_t Page::getWidth() const
		{
			return m_impl->m_width;
		}

		uint32_t Page::getHeight() const
		{
			return m_impl->m_height;
		}

		size_t Page::getWordCount() const
		{
			return m_impl->m_word_count;
		}

		std::wstring Page::getText()
		{
			return m_impl->getText();
		}

		Word Page::getWord(size_t index) const
		{
			auto&& words = m_impl->need_words();
			if (index >= words.size())
				throw std::out_of_range("index");
			return words[index];
		}

		const Page::words_t& Page::words() const
		{
			m_impl->need_words();
			return m_impl->m_words_loader;
		}

		PageElement Page::getRootPageElement() const
		{
			if (!m_impl->m_root_page_element.has_value())
			{
				Error_Control_Block ecb = { 0 };
				IGR_Page_Element pe = { 0 };
				pe.struct_size = sizeof(IGR_Page_Element);
				throw_on_error(IGR_Get_Page_Element_Root(getHandle(), &pe, &ecb), ecb, "IGR_Get_Page_Element_Root");

				m_impl->m_root_page_element.emplace(getHandle(), pe);
			}
			return *m_impl->m_root_page_element;
		}

		const Page::images_t& Page::images() const
		{
			if (!m_impl->m_images)
			{
				m_impl->m_images = std::make_shared<subfile_enumerable_t>(getHandle()
					, &IGR_Get_Page_Images_Enumerator
					, &IGR_Extract_Page_Image_Stream);
			}
			return *m_impl->m_images;
		}

		Subfile Page::getImage(const std::wstring& id) const
		{
			Error_Control_Block ecb = { 0 };
			IGR_Stream* Stream = nullptr;
			throw_on_error(IGR_Extract_Page_Image_Stream(getHandle(), reinterpret_cast<const IGR_UCS2*>(w_to_u16(id).c_str()), &Stream, &ecb), ecb, "IGR_Extract_Page_Image_Stream");
			return Subfile(0, id, Stream);
		}

		const std::vector<FormElement>& Page::formElements() const
		{
			return m_impl->need_form_elements();
		}

		const std::vector<Hyperlink>& Page::hyperlinks() const
		{
			return m_impl->need_hyperlinks();
		}

		const Page::annotations_t& Page::annotations() const
		{
			m_impl->need_annotations();
			return m_impl->m_annotations_loader;
		}

		void Page::Render(Canvas& Canvas) const
		{
			Canvas.RenderPage(*this);
		}

		void Page::Render(Canvas& Canvas, std::wstring& options, const RenderPageProperties& properties) const
		{
			Canvas.RenderPage(*this, options, properties);
		}

		void Page::Render(Canvas& Canvas, const RenderPageProperties& properties) const
		{
			Canvas.RenderPage(*this, properties);
		}

		std::wstring Page::getAttribute(const std::wstring& name) const
		{
			IGR_LONG buffer_size = 1024; // NOLINT
			std::vector<IGR_UCS2> buffer(buffer_size + 1);
			Error_Control_Block ecb = { 0 };

			throw_on_error(IGR_Get_Page_Attribute(getHandle()
				, reinterpret_cast<const IGR_UCS2*>(w_to_u16(name).c_str())
				, &buffer[0], &buffer_size, &ecb), ecb, "IGR_Get_Page_Attribute");
			return u16_to_w(&buffer[0], buffer_size);
		}

		PagePixels Page::getPixels() const
		{
			return getPixels(PixelType::PixelDefault);
		}

		PagePixels Page::getPixels(PixelType type) const
		{
			return getPixels(type, IGR_Rect{ 0, 0, getWidth(), getHeight() }, IGR_Size{ getWidth(), getHeight() });
		}

		PagePixels Page::getPixels(PixelType type, const IGR_Rect& src_rect, const std::wstring& options) const
		{
			return getPixels(type, src_rect, IGR_Size{ src_rect.right - src_rect.left, src_rect.bottom - src_rect.top }, options);
		}

		PagePixels Page::getPixels(PixelType type, const IGR_Rect& src_rect, const IGR_Size& dest_size, const std::wstring& options) const
		{
			Error_Control_Block ecb = { 0 };
			IGR_Page_Pixels pixels = { 0 };

			throw_on_error(IGR_Get_Page_Pixels(getHandle()
				, &src_rect
				, &dest_size
				, 0
				, reinterpret_cast<const IGR_UCS2*>(w_to_u16(options).c_str())
				, static_cast<IGR_LONG>(type)
				, &pixels
				, &ecb), ecb, "IGR_Get_Page_Pixels");

			return PagePixels(getHandle(), pixels);
		}

		CompareResults Page::Compare(const Page& other, const CompareSettings& settings) const 
		{
			RectF margins = { 0, 0, 0, 0 };
			return Compare(other, margins, margins, settings);
		}
		
		CompareResults Page::Compare(const Page& other, const RectF& margins, const CompareSettings& settings) const 
		{
			return Compare(other, margins, margins, settings);
		}

		CompareResults Page::Compare(const Page& other, const RectF& leftMargins, const RectF& rightMargins, const CompareSettings& settings) const 
		{
			auto&& compare_settings = settings.data();
			Error_Control_Block ecb = { 0 };
			IGR_HTEXTCOMPARE res = nullptr;

			const IGR_FRect* l = leftMargins.left != 0 || leftMargins.right != 0 || leftMargins.top != 0 || leftMargins.bottom != 0 ? &leftMargins : nullptr;
			const IGR_FRect* r = rightMargins.left != 0 || rightMargins.right != 0 || rightMargins.top != 0 || rightMargins.bottom != 0 ? &rightMargins : nullptr;

			throw_on_error(IGR_Text_Compare_Pages(m_impl->need_handle()
				, l
				, other.getHandle()
				, r
				, &compare_settings
				, &res
				, &ecb), ecb, "IGR_Text_Compare_Pages", "Failed to compare pages");

			return CompareResults(res);
		}

	} // namespace DocFilters
} // namespace Hyland
