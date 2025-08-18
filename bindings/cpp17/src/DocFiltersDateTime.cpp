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

#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

namespace Hyland
{
	namespace DocFilters
	{
		DateTime::DateTime(IGR_ULONGLONG value)
			: m_time(from_filetime(value))
		{
		}

		DateTime::DateTime(const std::chrono::system_clock::time_point& time)
			: m_time(time)
		{
		}

		DateTime::DateTime(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second, uint16_t millisecond)
			: DateTime(DateTime::date_type_t::utc, year, month, day, hour, minute, second, millisecond)
		{
		}
		
		DateTime::DateTime(date_type_t type, uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second, uint16_t /*millisecond*/)
		{
			auto x = std::tm{ second, minute, hour, day, month - 1, year - 1900 }; // NOLINT
			if (type == date_type_t::local)
			{
				m_time = std::chrono::system_clock::from_time_t(std::mktime(&x));
			}
			else if (type == date_type_t::utc)
			{
				m_time = std::chrono::system_clock::from_time_t(timegm(&x));
			}
		}

		DateTime::DateTime(const std::string& value)
		{
			if (!value.empty())
			{
				std::tm tm = {};
				std::string tz;
				std::istringstream ss(value);
				ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S") >> tz;
				if (!ss.bad())
				{
					time_t tt = tz == "GMT" || tz == "UTC"
						? timegm(&tm) 
						: mktime(&tm);

					m_time = std::chrono::system_clock::from_time_t(tt);
				}
			}
		}

		DateTime::DateTime(const std::wstring& value)
			: DateTime(w_to_u8(value))
		{
		}

		bool DateTime::operator==(const DateTime& other) const 
		{ 
			return m_time == other.m_time; 
		}
		
		bool DateTime::operator!=(const DateTime& other) const 
		{ 
			return m_time != other.m_time; 
		}

		std::chrono::system_clock::time_point DateTime::ToTimePoint() const
		{
			return m_time;
		}

		bool DateTime::empty() const
		{
			return m_time == std::chrono::system_clock::time_point();
		}

		std::optional<std::tm> DateTime::ToTm(bool as_utc) const
		{
			// Convert to time_t to get the calendar time
			std::time_t time_t_value = std::chrono::system_clock::to_time_t(m_time);

			// Convert time_t to tm struct
			std::tm tm_value = { 0 };
#ifdef _WIN32
			bool ok = as_utc 
				? gmtime_s(&tm_value, &time_t_value) == 0
				: localtime_s(&tm_value, &time_t_value) == 0;
#else
			bool ok = as_utc
				? gmtime_r(&time_t_value, &tm_value) != nullptr
				: localtime_r(&time_t_value, &tm_value) != nullptr;
#endif
			if (ok)
				return tm_value;
			else
				return std::nullopt;
		}

		std::string DateTime::ToIsoString() const
		{
			// Convert to time_t to get the calendar time
			std::time_t time_t_value = std::chrono::system_clock::to_time_t(m_time);

			// Convert time_t to tm struct
			std::tm tm_value = { 0 };
#ifdef _WIN32
			bool ok = gmtime_s(&tm_value, &time_t_value) == 0;
#else
			bool ok = gmtime_r(&time_t_value, &tm_value) != nullptr;
#endif
			
			// Create a string stream to hold the formatted date and time
			if (ok)
			{
				std::ostringstream oss;
				oss << std::put_time(&tm_value, "%Y-%m-%dT%H:%M:%SZ");
				return oss.str();
			}
			return std::string();
		}

		std::chrono::system_clock::time_point DateTime::from_filetime(IGR_ULONGLONG value)
		{
			if (value != 0)
			{
				// Define the Unix epoch as a time_point
				std::chrono::system_clock::time_point unix_epoch = std::chrono::system_clock::from_time_t(0);
				// Calculate the difference in seconds between 1601-01-01 and 1970-01-01
				constexpr std::chrono::seconds seconds_between_1601_and_1970(11644473600LL);
				// Calculate the time_point for 1601-01-01 by subtracting the difference
				auto time_1601 = unix_epoch - seconds_between_1601_and_1970;
				auto date_ms = std::chrono::microseconds(value / 10); // NOLINT
				return time_1601 + date_ms;
			}
			return std::chrono::system_clock::time_point();
		}

		DateTime DateTime::utc(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second, uint16_t millisecond)
		{
			return DateTime(DateTime::date_type_t::utc, year, month, day, hour, minute, second, millisecond);
		}
		DateTime DateTime::local(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second, uint16_t millisecond)
		{
			return DateTime(DateTime::date_type_t::local, year, month, day, hour, minute, second, millisecond);
		}

	} // namespace DocFilters
} // namespace Hyland
