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
		class Bookmark::impl_t
		{
		public:
			IGR_LONG m_doc = 0;
			IGR_Bookmark m_item = { nullptr };
			std::shared_ptr<enumerable_t<Bookmark> > m_children;
			std::shared_ptr<enumerable_t<Bookmark> > m_all_children;
			std::list<Bookmark> m_alt_children;

			explicit impl_t(IGR_LONG doc_handle, const IGR_Bookmark item)
				: m_doc(doc_handle)
				, m_item(item)
			{
			}
			impl_t(const impl_t&) = delete;
			impl_t& operator=(const impl_t&) = delete;
			impl_t(impl_t&&) = delete;
			impl_t& operator=(impl_t&&) = delete;
			~impl_t() = default;

			void throw_if_not_ok() const
			{
				if (m_doc == 0 || m_item.reserved == nullptr)
					throw std::runtime_error("Bookmark is not valid");
			}

			[[nodiscard]]
			Bookmark getFirstChild() const
			{
				if (m_doc == 0 || m_item.reserved == nullptr)
					return Bookmark();

				Error_Control_Block ecb = { 0 };
				IGR_Bookmark child = { nullptr };
				throw_on_error(IGR_Get_Bookmark_First_Child(m_doc, &m_item, &child, &ecb), ecb, "IGR_Get_Bookmark_Child");
				return Bookmark(m_doc, child);
			}

			[[nodiscard]]
			Bookmark getNextSibling() const
			{
				if (m_doc == 0 || m_item.reserved == nullptr)
					return Bookmark();

				Error_Control_Block ecb = { 0 };
				IGR_Bookmark next = { nullptr };
				throw_on_error(IGR_Get_Bookmark_Next_Sibling(m_doc, &m_item, &next, &ecb), ecb, "IGR_Get_Bookmark_Next_Sibling");
				return Bookmark(m_doc, next);
			}
		};

		Bookmark::Bookmark()
			: m_impl(new impl_t(0, IGR_Bookmark{}))
		{
		}

		Bookmark::Bookmark(const std::shared_ptr<impl_t>& impl) // NOLINT
			: m_impl(impl)
		{
		}

		Bookmark::Bookmark(IGR_LONG doc_handle)
			: m_impl(new impl_t(doc_handle, IGR_Bookmark{}))
		{
			Error_Control_Block ecb = { 0 };

			throw_on_error(IGR_Get_Bookmark_Root(doc_handle, &m_impl->m_item, &ecb), ecb, "IGR_Get_Bookmark_Root");
		}

		Bookmark::Bookmark(IGR_LONG doc_handle, const IGR_Bookmark& Bookmark)
			: m_impl(new impl_t(doc_handle, Bookmark))
		{
			m_impl->m_item = Bookmark;
		}

		std::wstring Bookmark::getTitle() const
		{
			return u16_to_w(m_impl->m_item.title);
		}
		std::wstring Bookmark::getDestination() const
		{
			return u16_to_w(m_impl->m_item.dest);
		}
		Color Bookmark::getColor() const
		{
			return Color::from_igr_color(m_impl->m_item.color);
		}
		Bookmark::ActionType Bookmark::getAction() const
		{
			return static_cast<ActionType>(m_impl->m_item.action);
		}
		Bookmark::FitType Bookmark::getFit() const
		{
			return static_cast<FitType>(m_impl->m_item.fit);
		}
		double Bookmark::getZoom() const
		{
			return m_impl->m_item.zoom;
		}
		uint32_t Bookmark::getLevel() const
		{
			return m_impl->m_item.level;
		}
		RectI32 Bookmark::getRect() const
		{
			return RectI32::xywh(
				m_impl->m_item.x,
				m_impl->m_item.y,
				m_impl->m_item.width,
				m_impl->m_item.height
			);
		}
		uint32_t Bookmark::getPageIndex() const
		{
			return stoul_or(getDestination(), -1);
		}

		Bookmark::TextStyle Bookmark::getTextStyle() const
		{
			return static_cast<TextStyle>(m_impl->m_item.text_style);
		}

		const Bookmark::bookmarks_t& Bookmark::children() const
		{
			if (!m_impl->m_children)
			{
				if (m_impl->m_alt_children.empty())
					m_impl->m_children = std::make_shared<x_enumerable<Bookmark, Bookmark::impl_t> >(m_impl);
				else
					m_impl->m_children = std::make_shared<x_std_enumerable<Bookmark, std::list<Bookmark> > >(m_impl->m_alt_children);
			}

			return *m_impl->m_children;
		}

		const Bookmark::bookmarks_t& Bookmark::allChildren() const
		{
			if (!m_impl->m_all_children)
				m_impl->m_all_children = std::make_shared<x_deep_enumerable<Bookmark, Bookmark::impl_t> >(m_impl);
			return *m_impl->m_all_children;
		}

		Bookmark Bookmark::getFirstChild() const
		{
			if (!ok())
				return Bookmark();

			return m_impl->getFirstChild();
		}

		Bookmark Bookmark::getNextSibling() const
		{
			if (!ok())
				return Bookmark();

			return m_impl->getNextSibling();
		}

		bool Bookmark::ok() const
		{
			return m_impl->m_doc != 0 && m_impl->m_item.reserved != nullptr;
		}

		const IGR_Bookmark* Bookmark::data() const
		{
			return &m_impl->m_item;
		}

		Bookmark& Bookmark::setTitle(const std::wstring& value)
		{
			copy_string(value, m_impl->m_item.title);
			return *this;
		}

		Bookmark& Bookmark::setDestination(const std::wstring& value)
		{
			copy_string(value, m_impl->m_item.dest);
			return *this;
		}

		Bookmark& Bookmark::setColor(const Color value)
		{
			m_impl->m_item.color = value.to_igr_color();
			return *this;
		}

		Bookmark& Bookmark::setAction(ActionType value)
		{
			m_impl->m_item.action = static_cast<IGR_LONG>(value);
			return *this;
		}

		Bookmark& Bookmark::setFit(FitType value)
		{
			m_impl->m_item.fit = static_cast<IGR_LONG>(value);
			return *this;
		}

		Bookmark& Bookmark::setZoom(double value)
		{
			m_impl->m_item.zoom = static_cast<IGR_LONG>(value);
			return *this;
		}

		Bookmark& Bookmark::setLevel(uint32_t value)
		{
			m_impl->m_item.level = value;
			return *this;
		}

		Bookmark& Bookmark::setRect(const RectI32& value)
		{
			m_impl->m_item.x = value.left;
			m_impl->m_item.y = value.top;
			m_impl->m_item.width = value.width();
			m_impl->m_item.height = value.height();
			return *this;
		}

		Bookmark& Bookmark::setPageIndex(uint32_t value)
		{
			copy_string(std::to_wstring(value), m_impl->m_item.dest);
			return *this;
		}

		Bookmark& Bookmark::setTextStyle(TextStyle flags)
		{
			m_impl->m_item.text_style = static_cast<IGR_LONG>(flags);
			return *this;
		}

		Bookmark& Bookmark::AppendChild(const Bookmark& child)
		{
			if (child.getLevel() != getLevel() + 1)
				throw std::invalid_argument("child.level is not correctly set");

			// Copy the existing bookmarks into the alt_children list
			if (m_impl->m_alt_children.empty() && ok())
				m_impl->m_alt_children.insert(m_impl->m_alt_children.end(), children().begin(), children().end());

			// Add the new child to the alt_children list
			m_impl->m_alt_children.push_back(child);

			// Update the iterator
			m_impl->m_children.reset();

			return *this;
		}

		bool Bookmark::operator==(const Bookmark& other) const
		{
			return m_impl.get() == other.m_impl.get();
		}

		bool Bookmark::operator!=(const Bookmark& other) const
		{
			return !(*this == other);
		}
	} // namespace DocFilters
} // namespace Hyland
