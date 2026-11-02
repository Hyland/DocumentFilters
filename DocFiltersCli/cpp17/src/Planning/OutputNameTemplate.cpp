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

#include "OutputNameTemplate.h"

#include "Runtime/IFileSystem.h"
#include "Runtime/CliError.h"

#include <charconv>
#include <filesystem>

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

					[[noreturn]] void throwBadTemplate(const std::string& input)
					{
						throw UsageError(ErrorCodes::badTemplate, "--out-name does not accept the value '" + input + "'");
					}

					// std::from_chars<int> does the digit parsing and the overflow check.
					// It would accept a leading '-' (int is signed), unlike the
					// character-by-character loop this replaced, but the trailing
					// value > 0 guard rejects that case exactly as before --- a
					// negative width fails the same way, one step later.
					bool tryParsePositiveInt(const std::string& text, int& value)
					{
						if (text.empty())
							return false;

						const char* end = text.data() + text.size();
						std::from_chars_result result = std::from_chars(text.data(), end, value);
						if (result.ec != std::errc() || result.ptr != end)
							return false;

						return value > 0;
					}

					/// <summary>
					/// <paramref name="path"/> with every backslash turned into a forward
					/// slash.
					/// </summary>
					/// <remarks>
					/// std::filesystem::path only recognises the separators the running
					/// platform does --- backslash is not one of them on Linux --- but a
					/// source path is not guaranteed to match the platform running this
					/// tool: a walk finds real paths in the platform's own convention, but
					/// an explicit --files-from list or a subfile name from a container
					/// built elsewhere can carry either. Without this, {stem} on
					/// ..\..\windows\system32\config returned the whole string unchanged on
					/// Linux, backslashes included, which is exactly the path separator
					/// this template exists to keep out of the rendered name.
					/// </remarks>
					std::string normalizeSeparators(const std::string& path)
					{
						std::string result = path;
						for (char& c : result)
						{
							if (c == '\\')
								c = '/';
						}
						return result;
					}
				}

				OutputNameTemplate OutputNameTemplate::defaultTemplate(bool isSingleFile)
				{
					return isSingleFile ? parse("{stem}.{ext}") : parse("{stem}-{page:04}.{ext}");
				}

				OutputNameTemplate OutputNameTemplate::parse(const std::string& text)
				{
					OutputNameTemplate result;
					result._originalText = text;

					static const std::string specialChars = "{}/\\";
					size_t startIndex = 0;

					while (startIndex < text.size())
					{
						size_t endIndex = text.find_first_of(specialChars, startIndex);

						if (endIndex == std::string::npos)
						{
							result._segments.push_back(Segment{ SegmentType::Literal, text.substr(startIndex) });
							break;
						}

						if (text[endIndex] != '{')
							throwBadTemplate(text); // invalid character

						if (endIndex > startIndex)
							result._segments.push_back(Segment{ SegmentType::Literal, text.substr(startIndex, endIndex - startIndex) });

						startIndex = endIndex;
						endIndex = text.find('}', startIndex + 1);
						if (endIndex == std::string::npos)
							throwBadTemplate(text); // no closing brace

						// The name between the braces, e.g. "stem" or "page:04".
						std::string placeholder = text.substr(startIndex + 1, endIndex - startIndex - 1);

						if (placeholder == "stem")
						{
							result._segments.push_back(Segment{ SegmentType::Stem, "" });
						}
						else if (placeholder == "page")
						{
							result._segments.push_back(Segment{ SegmentType::Page, "" });
						}
						else if (placeholder == "srcext")
						{
							result._segments.push_back(Segment{ SegmentType::SrcExt, "" });
						}
						else if (placeholder == "ext")
						{
							result._segments.push_back(Segment{ SegmentType::Ext, "" });
						}
						else if (placeholder.rfind("page:", 0) == 0)
						{
							std::string widthText = placeholder.substr(std::string("page:").size());
							int width = 0;
							if (!tryParsePositiveInt(widthText, width))
								throwBadTemplate(text); // malformed width

							result._segments.push_back(Segment{ SegmentType::Page, "", width });
						}
						else
						{
							throwBadTemplate(text); // unknown placeholder
						}

						startIndex = endIndex + 1;
					}

					return result;
				}

				bool OutputNameTemplate::usesPage() const
				{
					for (const Segment& segment : _segments)
					{
						if (segment.type == SegmentType::Page)
							return true;
					}
					return false;
				}

				std::string OutputNameTemplate::render(const std::string& sourcePath, const std::string& outputExtension, int pageNumber) const
				{
					std::string result;

					for (const Segment& segment : _segments)
					{
						switch (segment.type)
						{
							case SegmentType::Literal:
								result += segment.literalText;
								break;

							case SegmentType::SrcExt:
							{
								std::string extension = Runtime::FilePath::toUtf8(Runtime::FilePath::of(normalizeSeparators(sourcePath)).extension());
								if (!extension.empty() && extension.front() == '.')
									extension.erase(0, 1);
								result += extension;
								break;
							}

							case SegmentType::Ext:
								result += outputExtension;
								break;

							case SegmentType::Page:
							{
								std::string pageText = std::to_string(pageNumber);
								if (segment.width > 0 && pageText.size() < static_cast<size_t>(segment.width))
									pageText.insert(0, segment.width - pageText.size(), '0');
								result += pageText;
								break;
							}

							case SegmentType::Stem:
								result += Runtime::FilePath::toUtf8(Runtime::FilePath::of(normalizeSeparators(sourcePath)).stem());
								break;
						}
					}

					return result;
				}
			}
		}
	}
}
