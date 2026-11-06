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

#include "PlanFormat.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				namespace PlanFormat
				{
					std::string yesNo(bool value) { return value ? "yes" : "no"; }

					std::string noneOr(const std::optional<std::string>& value)
					{
						return (!value.has_value() || value->empty()) ? "none" : *value;
					}

					std::string noneOr(const std::vector<std::string>& values)
					{
						if (values.empty())
							return "none";

						std::string result;
						for (size_t i = 0; i < values.size(); ++i)
						{
							if (i > 0)
								result += ", ";
							result += values[i];
						}
						return result;
					}

					std::string inputs(const std::vector<Discovery::InputSource>& inputs)
					{
						std::string result = "inputs ";
						for (size_t i = 0; i < inputs.size(); ++i)
						{
							if (i > 0)
								result += ", ";
							result += inputs[i].path;
						}
						return result;
					}

					std::string joinLines(const std::vector<std::string>& lines)
					{
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
}
