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

#include "CaseProbe.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				namespace
				{
					bool isSeparator(char c) { return c == '/' || c == '\\'; }

					std::string trimSeparators(const std::string& path)
					{
						std::string trimmed = path;
						while (!trimmed.empty() && isSeparator(trimmed.back()))
							trimmed.pop_back();

						return trimmed;
					}

					std::string nameOf(const std::string& path)
					{
						std::string trimmed = trimSeparators(path);
						for (std::size_t i = trimmed.size(); i > 0; i--)
						{
							if (isSeparator(trimmed[i - 1]))
								return trimmed.substr(i);
						}

						return trimmed;
					}

					/// <summary>
					/// ASCII-only, and every letter rather than a lowercasing, for the same
					/// reason <c>Strings::foldAscii</c> is ASCII-only: a locale-aware fold
					/// turns 'I' into a dotless 'i' under a Turkish locale and would probe
					/// for a name that exists under neither rule.
					/// </summary>
					std::string flipCase(const std::string& text)
					{
						std::string flipped = text;
						for (char& c : flipped)
						{
							if (c >= 'a' && c <= 'z')
								c = static_cast<char>(c - 'a' + 'A');
							else if (c >= 'A' && c <= 'Z')
								c = static_cast<char>(c - 'A' + 'a');
						}

						return flipped;
					}
				}

				std::optional<bool> CaseProbe::measure(const std::string& directory,
					const std::function<std::optional<std::string>()>& nextEntry,
					const std::function<bool(const std::string&)>& exists)
				{
					for (std::optional<std::string> entry = nextEntry(); entry; entry = nextEntry())
					{
						std::optional<std::string> sibling = flippedSibling(directory, *entry);
						if (!sibling)
							continue;

						return !exists(*sibling);
					}

					return std::nullopt;
				}

				std::optional<std::string> CaseProbe::flippedSibling(const std::string& directory, const std::string& entry)
				{
					std::string name = nameOf(entry);
					std::string flipped = flipCase(name);
					if (flipped == name)
						return std::nullopt;

					std::string parent = trimSeparators(directory);
					return parent.empty() ? flipped : parent + "/" + flipped;
				}
			}
		}
	}
}
