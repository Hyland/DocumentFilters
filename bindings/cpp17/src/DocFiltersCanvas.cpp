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
		class Canvas::impl_t
		{
		public:
			IGR_HCANVAS m_canvas = 0;
			bool m_has_page = false;

			explicit impl_t(IGR_HCANVAS handle)
				: m_canvas(handle)
			{
			}
			impl_t(const impl_t&) = delete; 
			impl_t& operator=(const impl_t&) = delete;
			impl_t(impl_t&&) = delete; 
			impl_t& operator=(impl_t&&) = delete;

			~impl_t()
			{
				try {
					Close();
				}
				catch (...) {}
			}

			[[nodiscard]] IGR_HCANVAS getHandle() const
			{
				return m_canvas;
			}

			[[nodiscard]] IGR_HCANVAS needHandle() const
			{
				if (m_canvas == 0)
					throw DocumentFilters::Error("Canvas is not open.");
				return m_canvas;
			}
			
			[[nodiscard]] IGR_HCANVAS drawable_handle() const
			{
				if (!m_has_page)
					throw DocumentFilters::Error("RenderPage or BlankPage must be called first.");
				return needHandle();
			}
			void Close()
			{
				if (m_canvas != 0)
				{
					Error_Control_Block ecb = { 0 };
					IGR_Close_Canvas(m_canvas, &ecb);
					m_canvas = 0;
				}
			}
		};

		Canvas::Canvas()
			: m_impl(new impl_t(0))
		{
		}

		Canvas::Canvas(IGR_HCANVAS handle)
			: m_impl(new impl_t(handle))
		{
		}

		IGR_HCANVAS Canvas::getHandle() const
		{
			return m_impl->needHandle();
		}

		bool Canvas::hasHandle() const
		{
			return m_impl->getHandle() != 0;
		}

		void Canvas::Close()
		{
			m_impl->Close();
		}

		void Canvas::RenderPage(const Page& Page, const std::wstring& options, const RenderPageProperties& properties)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Render_Page_Ex(Page.getHandle()
				, m_impl->needHandle()
				, reinterpret_cast<const IGR_UCS2*>(w_to_u16(options).c_str())
				, properties.data()
				, &ecb), ecb, "IGR_Render_Page_Ex");
			m_impl->m_has_page = true;
		}

		void Canvas::RenderPage(const Page& Page, const RenderPageProperties& properties)
		{
			RenderPage(Page, std::wstring(), properties);
		}

		void Canvas::RenderPages(const Extractor& Extractor)
		{
			for (auto&& Page : Extractor.pages())
			{
				RenderPage(Page, std::wstring(), RenderPageProperties());
			}
		}

		void Canvas::BlankPage(int width, int height, const std::wstring& options)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Blank_Page(m_impl->needHandle(), reinterpret_cast<const IGR_UCS2*>(w_to_u16(options).c_str()), width, height, nullptr, &ecb), ecb, "IGR_Canvas_Blank_Page");

			m_impl->m_has_page = true;
		}

		void Canvas::Arc(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Arc(m_impl->drawable_handle(), x, y, x2, y2, x3, y3, x4, y4, &ecb), ecb, "IGR_Canvas_Arc");
		}

		void Canvas::AngleArc(int x, int y, int radius, double start_angle, double sweep_angle)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_AngleArc(m_impl->drawable_handle(), x, y, radius, static_cast<IGR_LONG>(start_angle), static_cast<IGR_LONG>(sweep_angle), &ecb), ecb, "IGR_Canvas_Angle_Arc");
		}

		void Canvas::Chord(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Chord(m_impl->drawable_handle(), x, y, x2, y2, x3, y3, x4, y4, &ecb), ecb, "IGR_Canvas_Chord");
		}

		void Canvas::Ellipse(int x, int y, int x2, int y2)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Ellipse(m_impl->drawable_handle(), x, y, x2, y2, &ecb), ecb, "IGR_Canvas_Ellipse");
		}

		void Canvas::Line(int x, int y, int x2, int y2)
		{
			MoveTo(x, y);
			LineTo(x2, y2);
		}

		void Canvas::MoveTo(int x, int y)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_MoveTo(m_impl->drawable_handle(), x, y, &ecb), ecb, "IGR_Canvas_Ellipse");
		}

		void Canvas::LineTo(int x, int y)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_LineTo(m_impl->drawable_handle(), x, y, &ecb), ecb, "IGR_Canvas_LineTo");
		}

		void Canvas::Rect(int x, int y, int x2, int y2)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Rect(m_impl->drawable_handle(), x, y, x2, y2, &ecb), ecb, "IGR_Canvas_Rectangle");
		}

		void Canvas::Rect(const RectF& r)
		{
			Rect(static_cast<int>(r.left), static_cast<int>(r.top), static_cast<int>(r.right + 0.5), static_cast<int>(r.bottom + 0.5));
		}

		void Canvas::Pie(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Pie(m_impl->drawable_handle(), x, y, x2, y2, x3, y3, x4, y4, &ecb), ecb, "IGR_Canvas_Pie");
		}

		void Canvas::RoundRect(int x, int y, int x2, int y2, int radius)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_RoundRect(m_impl->drawable_handle(), x, y, x2, y2, radius, &ecb), ecb, "IGR_Canvas_RoundRect");
		}

		void Canvas::TextOut(int x, int y, const std::wstring& text)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_TextOut(m_impl->drawable_handle(), x, y, reinterpret_cast<const IGR_UCS2*>(w_to_u16(text).c_str()), &ecb), ecb, "IGR_Canvas_TextOut");
		}

		void Canvas::TextRect(int x, int y, int x2, int y2, const std::wstring& text, int flags)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_TextRect(m_impl->drawable_handle(), x, y, x2, y2, reinterpret_cast<const IGR_UCS2*>(w_to_u16(text).c_str()), flags, &ecb), ecb, "IGR_Canvas_TextRect");
		}

		uint32_t Canvas::TextWidth(const std::wstring& text)
		{
			return MeasureText(text).width;
		}

		uint32_t Canvas::TextHeight(const std::wstring& text)
		{
			return MeasureText(text).height;
		}

		IGR_Size Canvas::MeasureText(const std::wstring& text)
		{
			Error_Control_Block ecb = { 0 };
			IGR_LONG width = 0;
			IGR_LONG height = 0;
			throw_on_error(IGR_Canvas_MeasureText(m_impl->drawable_handle(), reinterpret_cast<const IGR_UCS2*>(w_to_u16(text).c_str()), &width, &height, &ecb), ecb, "IGR_Canvas_MeasureText");
			return IGR_Size{ static_cast<IGR_ULONG>(width), static_cast<IGR_ULONG>(height) };
		}

		void Canvas::SetPen(const Color& textColor, int width, PenStyle style)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_SetPen(m_impl->drawable_handle(), textColor.to_igr_color(), width, static_cast<int>(style), &ecb), ecb, "IGR_Canvas_SetPen");
		}

		void Canvas::SetBrush(const Color& textColor, BrushStyle style)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_SetBrush(m_impl->drawable_handle(), textColor.to_igr_color(), static_cast<int>(style), &ecb), ecb, "IGR_Canvas_SetBrush");
		}

		void Canvas::SetFont(const std::wstring& name, int size, FontStyle style)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_SetFont(m_impl->drawable_handle(), reinterpret_cast<const IGR_UCS2*>(w_to_u16(name).c_str()), size, static_cast<int>(style), &ecb), ecb, "IGR_Canvas_SetFont");
		}

		void Canvas::SetOpacity(int value)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_SetOpacity(m_impl->drawable_handle(), value, &ecb), ecb, "IGR_Canvas_SetOpacity");
		}

		void Canvas::DrawImage(int x, int y, const void* buffer, size_t buffer_size, const std::wstring& mime_type)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_DrawImage(m_impl->drawable_handle(), x, y, const_cast<void*>(buffer), buffer_size, reinterpret_cast<const IGR_UCS2*>(w_to_u16(mime_type).c_str()), &ecb), ecb, "IGR_Canvas_DrawImage"); // NOLINT
		}

		void Canvas::DrawScaleImage(int x, int y, int width, int height, const void* buffer, size_t buffer_size, const std::wstring& mime_type)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_DrawScaleImage(m_impl->drawable_handle()
				, x
				, y
				, width
				, height
				, const_cast<void*>(buffer) // NOLINT
				, buffer_size
				, reinterpret_cast<const IGR_UCS2*>(w_to_u16(mime_type).c_str()), &ecb), ecb, "IGR_Canvas_DrawScaleImage");
		}

		void Canvas::DrawScaleImage(const RectI32& rc, const void* buffer, size_t buffer_size, const std::wstring& mime_type)
		{
			DrawScaleImage(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, buffer, buffer_size, mime_type);
		}

		void Canvas::Rotate(int degrees)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Rotation(m_impl->drawable_handle(), degrees, &ecb), ecb, "IGR_Canvas_Rotate");
		}

		void Canvas::Reset()
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Reset(m_impl->drawable_handle(), &ecb), ecb, "IGR_Canvas_Reset");
		}

		void Canvas::Annotate(const std::string& json)
		{
			Error_Control_Block ecb = { 0 };
			std::u16string payload = u8_to_u16(json);
			throw_on_error(IGR_Canvas_Annotate_JSON(m_impl->needHandle(), reinterpret_cast<const IGR_UCS2*>(payload.c_str()), &ecb), ecb, "IGR_Canvas_Annotate");
		}

		void Canvas::Annotate(const AnnotationSerializable& Annotation)
		{
			Annotate(Annotation.serialize());
		}

		void Canvas::ClearBookmarks()
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Bookmarks_Clear(m_impl->needHandle(), &ecb), ecb, "IGR_Canvas_Clear_Bookmarks");
		}
		void Canvas::AppendBookmark(const Bookmark& item, bool recurse)
		{
			Error_Control_Block ecb = { 0 };
			throw_on_error(IGR_Canvas_Bookmarks_Append(m_impl->needHandle(), item.data(), &ecb), ecb, "IGR_Canvas_Append_Bookmark");

			if (recurse)
			{
				for (auto&& child : item.allChildren())
				{
					AppendBookmark(child, false);
				}
			}
		}

	} // namespace DocFilters
} // namespace Hyland
