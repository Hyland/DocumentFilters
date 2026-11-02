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

#include "SharedOptions.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Configuration
			{
				namespace
				{
					std::string formatIdList(const std::vector<std::string>& ids)
					{
						if (ids.empty())
							return "none";

						std::string text;
						for (size_t i = 0; i < ids.size(); ++i)
						{
							if (i > 0)
								text += ',';
							text += ids[i];
						}
						return text;
					}
				}

				std::string SharedOptions::toCanonicalString(bool includeSubfiles) const
				{
					std::string text;

					std::optional<int> depthValue = input.effectiveMaxDepth();
					std::string depth = depthValue.has_value() ? std::to_string(*depthValue) : "unlimited";

					text += "walk depth=" + depth;
					text += std::string(" follow=") + (input.followSymlinks ? "yes" : "no");
					text += std::string(" sort=") + (input.sort ? "yes" : "no");
					text += std::string(" hidden=") + (input.includeHidden ? "yes" : "no");
					text += '\n';

					text += "match ";
					text += input.matcher.has_value() ? input.matcher->toCanonicalString() : std::string("none");
					text += '\n';

					text += "types include=" + formatIdList(input.includeTypes);
					text += " exclude=" + formatIdList(input.excludeTypes);
					text += '\n';

					if (includeSubfiles)
					{
						text += "subfiles ";
						if (subfiles.enabled)
						{
							text += "enabled depth=";
							text += subfiles.maxDepth.has_value() ? std::to_string(*subfiles.maxDepth) : std::string("unlimited");
						}
						else
						{
							text += "disabled";
						}
						text += '\n';
					}

					text += "on-error ";
					text += (batch.onError == OnErrorBehaviour::Stop ? "stop" : "skip");

					return text;
				}
			}
		}
	}
}
