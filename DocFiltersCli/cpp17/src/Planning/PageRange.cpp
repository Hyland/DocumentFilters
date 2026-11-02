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

#include "PageRange.h"

#include "Runtime/CliError.h"

#include <algorithm>
#include <charconv>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Planning
			{
				namespace
				{
					namespace ErrorCodes = Runtime::ErrorCodes;
					using Runtime::UsageError;

					[[noreturn]] void throwBadRange()
					{
						throw UsageError(ErrorCodes::badPageRange, "--pages must be a list of pages or ranges, such as 1,3-5,8");
					}

					// Accepts an optional leading '+' or '-' followed by decimal digits,
					// with nothing left over --- the same shape int.TryParse(text, out
					// value) accepts under NumberStyles.Integer, minus the whitespace
					// tolerance that shape also has (untested by anything that reaches
					// this function, since every caller hands it an already-split,
					// unpadded token).
					//
					// std::from_chars<int> does the digit parsing and the overflow check
					// (its ec reports std::errc::result_out_of_range), so this only has
					// to handle what it doesn't: a leading '+', which from_chars rejects
					// outright, and confirming the whole token was consumed rather than
					// just a numeric prefix of it.
					bool tryParseInt(const std::string& text, int& value)
					{
						if (text.empty())
							return false;

						const char* begin = text.data();
						const char* end = text.data() + text.size();

						if (*begin == '+')
							++begin;

						if (begin == end)
							return false;

						std::from_chars_result result = std::from_chars(begin, end, value);
						return result.ec == std::errc() && result.ptr == end;
					}

					int parsePageNumber(const std::string& text)
					{
						int value = 0;
						if (text.empty() || !tryParseInt(text, value) || value < 1)
							throwBadRange();

						return value;
					}

					// Sorts spans and joins every pair that overlaps or touches, so
					// 3,1,1-2 and 1-3 become the same single span.
					std::vector<PageRange::Span> merge(std::vector<PageRange::Span> spans)
					{
						std::sort(spans.begin(), spans.end());

						std::vector<PageRange::Span> merged;
						for (const PageRange::Span& span : spans)
						{
							// long long, because the page after INT_MAX is not an int.
							if (!merged.empty() && span.first <= static_cast<long long>(merged.back().second) + 1)
								merged.back().second = std::max(merged.back().second, span.second);
							else
								merged.push_back(span);
						}
						return merged;
					}

					// Writes merged spans in their canonical form, e.g. 1,3,4,5,8
					// becomes 1,3-5,8: a run of three or more pages as from-to, and a
					// run of two as the two pages.
					std::string buildCanonicalText(const std::vector<PageRange::Span>& spans)
					{
						std::vector<std::string> parts;
						for (const PageRange::Span& span : spans)
						{
							int start = span.first;
							int end = span.second;

							if (end - start <= 1)
							{
								parts.push_back(std::to_string(start));
								if (end != start)
									parts.push_back(std::to_string(end));
							}
							else
							{
								parts.push_back(std::to_string(start) + "-" + std::to_string(end));
							}
						}

						std::string text;
						for (size_t k = 0; k < parts.size(); ++k)
						{
							if (k > 0)
								text += ',';
							text += parts[k];
						}
						return text;
					}
				}

				const PageRange& PageRange::all()
				{
					static const PageRange singleton(std::nullopt, "all");
					return singleton;
				}

				PageRange PageRange::parse(const std::string& text)
				{
					if (text.empty())
						throwBadRange();

					std::vector<Span> given;
					size_t start = 0;
					while (start <= text.size())
					{
						size_t comma = text.find(',', start);
						std::string token = text.substr(start, comma == std::string::npos ? std::string::npos : comma - start);

						if (token.empty())
							throwBadRange(); // a leading, trailing or doubled comma

						size_t dash = token.find('-');
						if (dash == std::string::npos)
						{
							int page = parsePageNumber(token);
							given.emplace_back(page, page);
						}
						else
						{
							int from = parsePageNumber(token.substr(0, dash));
							int to = parsePageNumber(token.substr(dash + 1));

							if (to < from)
								throwBadRange(); // "5-3": far more likely a typo than a request to reverse

							given.emplace_back(from, to);
						}

						if (comma == std::string::npos)
							break;
						start = comma + 1;
					}

					std::vector<Span> spans = merge(std::move(given));
					std::string canonicalText = buildCanonicalText(spans);
					return PageRange(std::move(spans), std::move(canonicalText));
				}

				bool PageRange::contains(int pageNumber) const
				{
					if (isAll())
						return pageNumber >= 1;

					return std::any_of(_spans->begin(), _spans->end(),
						[pageNumber](const Span& span) { return span.first <= pageNumber && pageNumber <= span.second; });
				}

				PageRange::Pages PageRange::enumerate(int pageCount) const
				{
					if (isAll())
						return Pages(pageCount >= 1 ? std::vector<Span>{ { 1, pageCount } } : std::vector<Span>{});

					return Pages(*_spans);
				}

				PageRange::Pages::iterator::iterator(const std::vector<Span>* spans, size_t span)
					: _spans(spans), _span(span), _page(span < spans->size() ? (*spans)[span].first : 0)
				{
				}

				PageRange::Pages::iterator& PageRange::Pages::iterator::operator++()
				{
					// Compared with the span's end before stepping, not `page <= to` after:
					// with the end at INT_MAX that is always true, and the step past it
					// overflows.
					if (_page == (*_spans)[_span].second)
					{
						++_span;
						_page = _span < _spans->size() ? (*_spans)[_span].first : 0;
					}
					else
					{
						++_page;
					}
					return *this;
				}
			}
		}
	}
}
