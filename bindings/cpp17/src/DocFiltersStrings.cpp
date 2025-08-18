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
        namespace
        {
            template <typename S>
            size_t find_len(const S* text, size_t max_length)
            {
                size_t len = 0;
                while (len < max_length && text[len])
                    ++len;
                return len;
            }

            template <typename T>
            T u8_to_uX(const char* str, size_t str_len)
            {
                if (str == nullptr || str_len == 0)
                    return T();
                if (str_len == std::wstring::npos)
                    str_len = std::char_traits<char>::length(str);


                T result;
                for (size_t i = 0; i < str_len; ++i)
                {
                    unsigned char ch = str[i];
                    if (ch == 0)
                        break;
                    if (ch <= 0x7F) // NOLINT
                        result.push_back(static_cast<typename T::value_type>(ch));
                    else if (ch <= 0xDF && i + 1 < str_len)  // NOLINT
                    {
                        result.push_back(static_cast<typename T::value_type>(((ch & 0x1F) << 6) | (str[i + 1] & 0x3F)));  // NOLINT
                        ++i;
                    }
                    else if (ch <= 0xEF && i + 2 < str_len)  // NOLINT
                    {
                        result.push_back(static_cast<typename T::value_type>(((ch & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F)));  // NOLINT
                        i += 2;
                    }
                    else if (ch <= 0xF7 && i + 3 < str_len)  // NOLINT
                    {
                        uint32_t codepoint = ((ch & 0x07) << 18) | ((str[i + 1] & 0x3F) << 12) | ((str[i + 2] & 0x3F) << 6) | (str[i + 3] & 0x3F);  // NOLINT
                        if (sizeof(typename T::value_type) == 4 || codepoint <= 0xFFFF)  // NOLINT
                            result.push_back(static_cast<typename T::value_type>(codepoint));
                        else
                        {
                            result.push_back(static_cast<typename T::value_type>(0xD800 + ((codepoint - 0x10000) >> 10))); // NOLINT
                            result.push_back(static_cast<typename T::value_type>(0xDC00 + ((codepoint - 0x10000) & 0x3FF)));  // NOLINT
                        }
                        i += 3;
                    }
                }
                return result;
            }

        } // namespace


        std::wstring u16_to_w(const char16_t* str, size_t str_len)
        {
            if (str == nullptr || str_len == 0)
                return std::wstring();

            if (str_len == std::wstring::npos)
                str_len = std::char_traits<char16_t>::length(str);

            if (sizeof(wchar_t) == 2)
                return std::wstring(reinterpret_cast<const wchar_t*>(str), find_len(str, str_len));

            std::wstring result;
            for (size_t i = 0; i < str_len; ++i)
            {
                char16_t ch = str[i];
                if (ch == 0)
                    break;
                if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < str_len) // NOLINT
                {
                    char16_t lowSurrogate = str[i + 1];
                    if (lowSurrogate >= 0xDC00 && lowSurrogate <= 0xDFFF) // NOLINT
                    {
                        uint32_t codepoint = ((ch - 0xD800) << 10) + (lowSurrogate - 0xDC00) + 0x10000; // NOLINT
                        result.push_back(static_cast<wchar_t>(codepoint));
                        ++i;
                        continue;
                    }
                }
                result.push_back(static_cast<wchar_t>(ch));
            }
            return result;
        }

        std::string u16_to_u8(const char16_t* str, size_t str_len)
        {
            if (str == nullptr || str_len == 0)
                return std::string();
            if (str_len == std::wstring::npos)
                str_len = std::char_traits<char16_t>::length(str);
            std::string result;
            for (size_t i = 0; i < str_len; ++i)
            {
                char16_t ch = str[i];
                if (ch == 0)
                    break;
                if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < str_len) // NOLINT
                {
                    char16_t lowSurrogate = str[i + 1];
                    if (lowSurrogate >= 0xDC00 && lowSurrogate <= 0xDFFF) // NOLINT
                    {
                        uint32_t codepoint = ((ch - 0xD800) << 10) + (lowSurrogate - 0xDC00) + 0x10000; // NOLINT
                        if (codepoint <= 0x7F) // NOLINT
                            result.push_back(static_cast<char>(codepoint));
                        else if (codepoint <= 0x7FF) // NOLINT
                        {
                            result.push_back(static_cast<char>(0xC0 | (codepoint >> 6))); // NOLINT
                            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F))); // NOLINT
                        }
                        else if (codepoint <= 0xFFFF) // NOLINT
                        {
                            result.push_back(static_cast<char>(0xE0 | (codepoint >> 12))); // NOLINT
                            result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F))); // NOLINT
                            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F))); // NOLINT
                        }
                        else
                        {
                            result.push_back(static_cast<char>(0xF0 | (codepoint >> 18))); // NOLINT
                            result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F))); // NOLINT
                            result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F))); // NOLINT
                            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F))); // NOLINT
                        }
                        i++;
                        continue;
                    }
                }
                if (ch <= 0x7F) // NOLINT
                    result.push_back(static_cast<char>(ch));
                else if (ch <= 0x7FF) // NOLINT
                {
                    result.push_back(static_cast<char>(0xC0 | (ch >> 6))); // NOLINT
                    result.push_back(static_cast<char>(0x80 | (ch & 0x3F))); // NOLINT
                }
                else
                {
                    result.push_back(static_cast<char>(0xE0 | (ch >> 12))); // NOLINT
                    result.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F))); // NOLINT
                    result.push_back(static_cast<char>(0x80 | (ch & 0x3F))); // NOLINT
                }
            }
            return result;
        }

        std::u32string u8_to_u32(const char* str, size_t str_len)
        {
            return u8_to_uX<std::u32string>(str, str_len);
        }

        std::u16string u8_to_u16(const char* str, size_t str_len)
        {
            return u8_to_uX<std::u16string>(str, str_len);
        }

        std::wstring u8_to_w(const char* str, size_t str_len)
        {
            return u8_to_uX<std::wstring>(str, str_len);
        }

        std::u16string w_to_u16(const wchar_t* str, size_t str_len)
        {
            if (str == nullptr || str_len == 0)
                return std::u16string();

            if (str_len == std::wstring::npos)
                str_len = std::char_traits<wchar_t>::length(str);

            if (sizeof(wchar_t) == 2)
                return std::u16string(reinterpret_cast<const char16_t*>(str), str_len);

            std::u16string result;

            for (size_t i = 0; i < str_len; ++i)
            {
                wchar_t ch = str[i];
                if (ch == 0)
                    break;
                if (ch <= 0xFFFF) // NOLINT
                    result.push_back(static_cast<char16_t>(ch));
                else
                {
                    uint32_t codepoint = ch - 0x10000; // NOLINT
                    char16_t highSurrogate = static_cast<char16_t>((codepoint >> 10) + 0xD800); // NOLINT
                    char16_t lowSurrogate = static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00); // NOLINT
                    result.push_back(highSurrogate);
                    result.push_back(lowSurrogate);
                }
            }

            return result;
        }
        std::string w_to_u8(const wchar_t* str, size_t str_len)
        {
            if (str == nullptr || str_len == 0)
                return std::string();
            if (str_len == std::wstring::npos)
                str_len = std::char_traits<wchar_t>::length(str);
            std::string result;
            for (size_t i = 0; i < str_len; ++i)
            {
                wchar_t ch = str[i];
                if (ch == 0)
                    break;
                if (ch <= 0x7F) // NOLINT
                    result.push_back(static_cast<char>(ch));
                else if (ch <= 0x7FF) // NOLINT
                {
                    result.push_back(static_cast<char>(0xC0 | (ch >> 6))); // NOLINT
                    result.push_back(static_cast<char>(0x80 | (ch & 0x3F))); // NOLINT
                }
                else if (ch <= 0xFFFF) // NOLINT
                {
                    result.push_back(static_cast<char>(0xE0 | (ch >> 12))); // NOLINT
                    result.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F))); // NOLINT
                    result.push_back(static_cast<char>(0x80 | (ch & 0x3F))); // NOLINT
                }
                else
                {
                    uint32_t codepoint = ch - 0x10000; // NOLINT
                    char16_t highSurrogate = static_cast<char16_t>((codepoint >> 10) + 0xD800); // NOLINT
                    char16_t lowSurrogate = static_cast<char16_t>((codepoint & 0x3FF) + 0xDC00); // NOLINT
                    result.push_back(static_cast<char>(highSurrogate)); 
                    result.push_back(static_cast<char>(lowSurrogate));
                }
            }
            return result;
        }

        std::u32string w_to_u32(const std::wstring& wstr)
        {
            std::u32string result;
            if (sizeof(wchar_t) == sizeof(char32_t))
                return std::u32string(reinterpret_cast<const char32_t*>(wstr.c_str()), wstr.size());

            for (const auto& ch : wstr)
            {
                if (ch <= 0xFFFF) // NOLINT
                    result.push_back(static_cast<char32_t>(ch));
                else
                {
                    uint32_t codepoint = ch - 0x10000; // NOLINT
                    char32_t highSurrogate = static_cast<char32_t>((codepoint >> 10) + 0xD800); // NOLINT
                    char32_t lowSurrogate = static_cast<char32_t>((codepoint & 0x3FF) + 0xDC00); // NOLINT
                    result.push_back(highSurrogate);
                    result.push_back(lowSurrogate);
                }
            }
            return result;
        }

	} // namespace DocFilters
} // namespace Hyland
