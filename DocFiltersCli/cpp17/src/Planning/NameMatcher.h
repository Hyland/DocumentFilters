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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_NAMEMATCHER_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_NAMEMATCHER_H

#include <optional>
#include <regex>
#include <string>
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
				/// Path selection from <c>-g/--glob</c> or <c>--regex</c>.
				/// </summary>
				/// <remarks>
				/// Glob is the default idiom. Patterns are matched against the path as
				/// written, using forward slashes on every platform so that a pattern is
				/// portable. <c>*</c> matches within one path segment, <c>**</c> crosses
				/// segments, and a leading <c>!</c> turns a pattern into an exclusion.
				///
				/// Multiple patterns combine as: an entry is kept when it matches at
				/// least one include pattern (or there are none) and matches no exclude
				/// pattern.
				///
				/// Every match is a whole-path match via <c>std::regex_match</c>, which
				/// requires the entire target to match --- the C++ equivalent of .NET's
				/// <c>^(?:pattern)\z</c> wrapping with no ECMAScript counterpart to
				/// <c>\z</c> needed, since <c>std::regex_match</c> anchors both ends by
				/// definition.
				/// </remarks>
				class NameMatcher
				{
				public:
					/// <summary>Builds a matcher from one or more <c>-g/--glob</c> patterns.</summary>
					/// <exception cref="Runtime::UsageError">A pattern is malformed.</exception>
					static NameMatcher fromGlobs(const std::vector<std::string>& patterns);

					/// <summary>Builds a matcher from one or more <c>--regex</c> patterns.</summary>
					/// <exception cref="Runtime::UsageError">A pattern is not a valid regular expression.</exception>
					static NameMatcher fromRegexes(const std::vector<std::string>& patterns);

					/// <summary>True when the path should be processed.</summary>
					bool matches(const std::string& path) const;

					/// <summary>The canonical text form, used by the conformance corpus.</summary>
					std::string toCanonicalString() const;

				private:
					static std::regex compileGlob(const std::string& pattern, const std::string& original);
					static std::string translateGlobSegment(const std::string& segment, const std::string& original);

					std::optional<std::vector<std::string>> _globPatterns;
					std::optional<std::vector<std::string>> _regexPatterns;

					std::vector<std::regex> _includes;
					std::vector<std::regex> _excludes;
				};
			}
		}
	}
}

#endif
