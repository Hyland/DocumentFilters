/*
   (c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_PAGERANGE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_PAGERANGE_H

#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Planning
			{
				/// <summary>
				/// A <c>--pages</c> selection such as <c>1,3-5,8</c>. Pages are one-based,
				/// as the user writes them.
				/// </summary>
				class PageRange
				{
				public:
					/// <summary>An inclusive from-to span of one-based page numbers.</summary>
					using Span = std::pair<int, int>;

					/// <summary>
					/// The pages of a selection, ascending, produced one at a time as they
					/// are walked --- what C#'s <c>IEnumerable&lt;int&gt;</c> is to the
					/// reference. A span can be two billion pages wide, so nothing here is
					/// ever expanded into a list.
					/// </summary>
					class Pages
					{
					public:
						class iterator
						{
						public:
							using iterator_category = std::input_iterator_tag;
							using value_type = int;
							using difference_type = std::ptrdiff_t;
							using pointer = const int*;
							using reference = int;

							int operator*() const { return _page; }

							iterator& operator++();

							iterator operator++(int)
							{
								iterator before = *this;
								++*this;
								return before;
							}

							bool operator==(const iterator& other) const
							{
								return _span == other._span && _page == other._page;
							}

							bool operator!=(const iterator& other) const { return !(*this == other); }

						private:
							friend class Pages;

							iterator(const std::vector<Span>* spans, size_t span);

							const std::vector<Span>* _spans;
							size_t _span;
							int _page;
						};

						iterator begin() const { return iterator(&_spans, 0); }
						iterator end() const { return iterator(&_spans, _spans.size()); }

					private:
						friend class PageRange;

						explicit Pages(std::vector<Span> spans) : _spans(std::move(spans)) {}

						std::vector<Span> _spans;
					};

					/// <summary>Selects every page.</summary>
					static const PageRange& all();

					/// <summary>True when no restriction was given.</summary>
					bool isAll() const { return !_spans.has_value(); }

					/// <summary>The lowest page selected: 1 for <see cref="all"/>.</summary>
					int firstPage() const { return isAll() ? 1 : _spans->front().first; }

					/// <summary>
					/// Parses a range expression. Accepts comma-separated single pages and
					/// <c>from-to</c> spans. Duplicates collapse and the result is
					/// ascending, so <c>3,1,1-2</c> and <c>1,2,3</c> are the same selection.
					/// </summary>
					/// <exception cref="Runtime::UsageError">
					/// Malformed syntax, a page below 1, or a descending span such as
					/// <c>5-3</c>.
					/// </exception>
					static PageRange parse(const std::string& text);

					/// <summary>True when this selection includes the given one-based page number.</summary>
					bool contains(int pageNumber) const;

					/// <summary>
					/// The selected page numbers, ascending: every page of a document with
					/// <paramref name="pageCount"/> pages for <see cref="all"/>, or the
					/// explicit selection exactly as given otherwise.
					/// </summary>
					/// <remarks>
					/// Lazy, so a caller that stops early pays only for the pages it took:
					/// a span can be two billion pages wide, and a document is rarely that
					/// long.
					///
					/// An explicit selection is never filtered against
					/// <paramref name="pageCount"/> here --- a count is all this class is
					/// ever given, not a verified list of which page numbers are real, and
					/// not every caller even has a genuine one yet: a plan built before the
					/// document is open has nothing better than a guess. Deciding whether a
					/// requested page actually exists belongs to whoever opens it, once it
					/// has a count it can trust; this only ever answers "what did the user
					/// ask for".
					/// </remarks>
					Pages enumerate(int pageCount) const;

					/// <summary>The canonical text form, used by the conformance corpus.</summary>
					std::string toCanonicalString() const { return _canonicalText; }

				private:
					PageRange(std::optional<std::vector<Span>> spans, std::string canonicalText)
						: _spans(std::move(spans)), _canonicalText(std::move(canonicalText))
					{
					}

					// Absent means "every page" (PageRange::all()); otherwise the selected
					// one-based pages as inclusive spans, ascending, and merged so that no
					// two overlap or touch.
					//
					// Spans and not the pages themselves, because a span costs the same
					// however wide it is. Expanding one into a set made --pages
					// 1-2147483647 --- a valid value --- allocate two billion entries while
					// the arguments were still being read.
					std::optional<std::vector<Span>> _spans;
					std::string _canonicalText;
				};
			}
		}
	}
}

#endif
