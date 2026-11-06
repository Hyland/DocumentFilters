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

#include "NameMatcher.h"

#include "Runtime/CliError.h"

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

					/// <summary>
					/// Escapes one character for ECMAScript regex syntax, the dialect
					/// std::regex uses by default. A superset of what .NET's Regex.Escape
					/// treats as a metacharacter --- also covering ']' and '}', which are
					/// safe to escape even where they are not strictly required to be.
					/// </summary>
					std::string escapeRegexChar(char c)
					{
						static const std::string metacharacters = "\\^$.|?*+()[]{}";
						if (metacharacters.find(c) != std::string::npos)
							return std::string("\\") + c;

						return std::string(1, c);
					}
				}

				NameMatcher NameMatcher::fromGlobs(const std::vector<std::string>& patterns)
				{
					NameMatcher matcher;
					matcher._globPatterns = patterns;

					for (const std::string& raw : patterns)
					{
						bool isExclude = !raw.empty() && raw.front() == '!';
						std::string body = isExclude ? raw.substr(1) : raw;
						std::regex regex = compileGlob(body, raw);
						(isExclude ? matcher._excludes : matcher._includes).push_back(regex);
					}
					return matcher;
				}

				NameMatcher NameMatcher::fromRegexes(const std::vector<std::string>& patterns)
				{
					NameMatcher matcher;
					matcher._regexPatterns = patterns;

					for (const std::string& raw : patterns)
					{
						try
						{
							matcher._includes.push_back(std::regex(raw, std::regex::ECMAScript));
						}
						catch (const std::regex_error&)
						{
							throw UsageError(ErrorCodes::badValue, "invalid regular expression '" + raw + "'");
						}
					}
					return matcher;
				}

				bool NameMatcher::matches(const std::string& path) const
				{
					std::string normalized = path;
					for (char& c : normalized)
					{
						if (c == '\\')
							c = '/';
					}

					for (const std::regex& regex : _excludes)
					{
						if (std::regex_match(normalized, regex))
							return false;
					}

					if (_includes.empty())
						return true;

					for (const std::regex& regex : _includes)
					{
						if (std::regex_match(normalized, regex))
							return true;
					}
					return false;
				}

				/// <summary>Translates one <c>-g/--glob</c> pattern (minus any leading <c>!</c>) into a regex.</summary>
				std::regex NameMatcher::compileGlob(const std::string& pattern, const std::string& original)
				{
					std::vector<std::string> rawSegments = splitOn(pattern, '/');
					std::vector<std::string> segments;
					for (const std::string& segment : rawSegments)
					{
						if (segment == "**" && !segments.empty() && segments.back() == "**")
							continue;
						segments.push_back(segment);
					}

					std::string text;
					for (size_t i = 0; i < segments.size(); ++i)
					{
						const std::string& segment = segments[i];
						bool isFirst = (i == 0);
						bool isLast = (i == segments.size() - 1);
						bool previousIsDoubleStar = !isFirst && segments[i - 1] == "**";

						if (segment == "**")
						{
							if (!isFirst)
								text += '/';
							text += isLast ? ".*" : "(?:.*/)?";
						}
						else
						{
							if (!isFirst && !previousIsDoubleStar)
								text += '/';
							text += translateGlobSegment(segment, original);
						}
					}

					try
					{
						return std::regex(text, std::regex::ECMAScript);
					}
					catch (const std::regex_error&)
					{
						throw UsageError(ErrorCodes::badValue, "invalid glob pattern '" + original + "'");
					}
				}

				/// <summary>
				/// Translates one path segment: '*' and '?' stay within the segment,
				/// everything else is literal --- including a bracket pair, which this
				/// tool's glob dialect gives no matching meaning to, so "[abc]" matches
				/// only that literal five-character text. An unterminated '[' is still
				/// rejected as malformed rather than silently taken as a literal
				/// character: it is almost certainly a typo, not an intentional
				/// five-character filename fragment.
				/// </summary>
				std::string NameMatcher::translateGlobSegment(const std::string& segment, const std::string& original)
				{
					std::string result;
					size_t i = 0;
					while (i < segment.size())
					{
						char c = segment[i];

						if (c == '*')
						{
							result += "[^/]*";
							++i;
						}
						else if (c == '?')
						{
							result += "[^/]";
							++i;
						}
						else if (c == '[')
						{
							size_t close = segment.find(']', i + 1);
							if (close == std::string::npos)
								throw UsageError(ErrorCodes::badValue, "invalid glob pattern '" + original + "'");

							for (size_t k = i; k <= close; ++k)
								result += escapeRegexChar(segment[k]);
							i = close + 1;
						}
						else
						{
							result += escapeRegexChar(c);
							++i;
						}
					}
					return result;
				}

				std::string NameMatcher::toCanonicalString() const
				{
					if (_globPatterns.has_value())
					{
						std::string text = "glob ";
						for (size_t i = 0; i < _globPatterns->size(); ++i)
						{
							if (i > 0)
								text += ", ";
							text += (*_globPatterns)[i];
						}
						return text;
					}

					if (_regexPatterns.has_value())
					{
						std::string text = "regex ";
						for (size_t i = 0; i < _regexPatterns->size(); ++i)
						{
							if (i > 0)
								text += ", ";
							text += (*_regexPatterns)[i];
						}
						return text;
					}

					return "";
				}
			}
		}
	}
}
