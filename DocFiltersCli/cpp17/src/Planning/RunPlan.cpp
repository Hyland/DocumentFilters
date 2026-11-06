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

#include "RunPlan.h"

#include "Runtime/IFileSystem.h"

#include <filesystem>
#include <stdexcept>

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
					std::string patternText(const TargetPattern& pattern)
					{
						return pattern.directory.empty() ? pattern.templateName.toCanonicalString()
														   : pattern.directory + "/" + pattern.templateName.toCanonicalString();
					}
				}

				std::string cardinalityText(OutputCardinality cardinality)
				{
					switch (cardinality)
					{
						case OutputCardinality::PerInput:
							return "per-input";
						case OutputCardinality::Combined:
							return "combined";
						case OutputCardinality::SingleTarget:
							return "single-target";
						case OutputCardinality::Stream:
							return "stream";
					}
					throw std::invalid_argument("unknown OutputCardinality");
				}

				std::string TargetPattern::resolve(const std::string& sourcePath, int pageNumber) const
				{
					std::string fileName = templateName.render(sourcePath, extension, pageNumber);
					return directory.empty() ? fileName : (directory + "/" + fileName);
				}

				std::string TargetPattern::collisionKey(const std::string& sourcePath) const
				{
					// Any fixed page number identifies the non-page part of the name: if
					// two items differ only by page, they render identically here and
					// correctly collide; if they differ elsewhere, they differ here too.
					return resolve(sourcePath, 0);
				}

				std::string TargetPattern::firstTarget(const std::string& sourcePath, const PageRange& pages) const
				{
					int firstPage = pages.firstPage();
					return resolve(sourcePath, firstPage);
				}

				std::optional<std::string> PlannedItem::outputDirectory() const
				{
					if (targetPattern.has_value())
						return targetPattern->directory;

					if (!target.has_value() || target->empty())
						return std::nullopt;

					return Runtime::FilePath::toUtf8(Runtime::FilePath::of(*target).parent_path());
				}

				std::string RunPlan::toCanonicalString() const
				{
					std::vector<std::string> lines;
					lines.push_back("cardinality " + cardinalityText(cardinality));

					if (cardinality == OutputCardinality::Combined)
					{
						lines.push_back("target " + (combinedTarget.has_value() ? *combinedTarget : std::string()));
					}
					else
					{
						for (const PlannedItem& item : items)
						{
							if (item.target.has_value())
								lines.push_back("target " + *item.target);
							else if (item.targetPattern.has_value())
								lines.push_back("pattern " + patternText(*item.targetPattern));
						}
					}

					std::string result;
					for (size_t i = 0; i < lines.size(); ++i)
					{
						if (i > 0)
							result += '\n';
						result += lines[i];
					}
					return result;
				}
			}
		}
	}
}
