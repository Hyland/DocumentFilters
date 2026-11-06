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

#include "ValueParsers.h"

#include "Runtime/CliError.h"
#include "Runtime/Strings.h"

#include <charconv>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Configuration
			{
				namespace ValueParsers
				{
					namespace
					{
						namespace ErrorCodes = Runtime::ErrorCodes;
						using Runtime::UsageError;

						[[noreturn]] void throwBadValue(const std::string& argument, const std::string& text)
						{
							throw UsageError(ErrorCodes::badValue, argument + " does not accept the value '" + text + "'");
						}

						// Digits only: no leading sign, no leading or trailing whitespace,
						// no decimal point --- the same shape
						// int.TryParse(text, NumberStyles.None, ...) accepts, and stricter
						// than PageRange's own parser, which allows a sign. from_chars<T>
						// would accept a leading '-' for a signed T, so every character is
						// checked by hand first; from_chars is left to do the actual
						// digit-to-number conversion and the overflow check (its ec reports
						// std::errc::result_out_of_range).
						template <typename T>
						bool tryParseDigitsOnly(const std::string& text, T& value)
						{
							if (text.empty())
								return false;

							for (char c : text)
							{
								if (c < '0' || c > '9')
									return false;
							}

							const char* end = text.data() + text.size();
							std::from_chars_result result = std::from_chars(text.data(), end, value);
							return result.ec == std::errc() && result.ptr == end;
						}

						std::vector<std::string> splitOn(const std::string& text, char separator)
						{
							std::vector<std::string> parts;
							size_t start = 0;
							while (true)
							{
								size_t at = text.find(separator, start);
								parts.push_back(text.substr(start, at == std::string::npos ? std::string::npos : at - start));
								if (at == std::string::npos)
									break;
								start = at + 1;
							}
							return parts;
						}
					}

					int parseCount(const std::string& argument, const std::string& text)
					{
						int value = 0;
						if (!tryParseDigitsOnly(text, value) || value < 0)
							throwBadValue(argument, text);
						return value;
					}

					int parseSeconds(const std::string& argument, const std::string& text)
					{
						int value = 0;
						if (!tryParseDigitsOnly(text, value) || value <= 0)
							throwBadValue(argument, text);
						return value;
					}

					long long parseMegabytes(const std::string& argument, const std::string& text)
					{
						long long value = 0;
						if (!tryParseDigitsOnly(text, value) || value <= 0)
							throwBadValue(argument, text);
						return value;
					}

					std::vector<std::string> parseIdList(const std::string& text)
					{
						std::vector<std::string> result;
						for (const std::string& part : splitOn(text, ';'))
						{
							std::string trimmed = Runtime::Strings::trim(part);
							if (!trimmed.empty())
								result.push_back(trimmed);
						}
						return result;
					}

					RedactSpec parseRedaction(const std::string& text)
					{
						std::vector<std::string> parts = splitOn(text, ',');
						if (parts.size() != 5)
							throwBadValue("--redact", text);

						int values[5] = { 0, 0, 0, 0, 0 };
						for (int i = 0; i < 5; ++i)
						{
							if (!tryParseDigitsOnly(parts[static_cast<size_t>(i)], values[i]))
								throwBadValue("--redact", text);
						}

						return RedactSpec{ values[0], values[1], values[2], values[3], values[4] };
					}

					FormValue parseFormValue(const std::string& text)
					{
						size_t comma = text.find(',');
						if (comma == std::string::npos)
							throwBadValue("--form", text);

						std::string pageText = text.substr(0, comma);
						std::string rest = text.substr(comma + 1);

						size_t equals = rest.find('=');
						if (equals == std::string::npos)
							throwBadValue("--form", text);

						int page = 0;
						if (!tryParseDigitsOnly(pageText, page))
							throwBadValue("--form", text);

						return FormValue{ page, rest.substr(0, equals), rest.substr(equals + 1) };
					}

					OnErrorBehaviour parseOnError(const std::string& text)
					{
						if (text == "skip")
							return OnErrorBehaviour::Skip;
						if (text == "stop")
							return OnErrorBehaviour::Stop;

						throwBadValue("--on-error", text);
						return OnErrorBehaviour::Skip; // unreachable; throwBadValue always throws
					}

					std::string foldToken(const std::string& text) { return Runtime::Strings::foldAscii(text); }
				}
			}
		}
	}
}
