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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_PLANFORMAT_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_PLANFORMAT_H

#include "Discovery/InputSource.h"

#include <optional>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				/// <summary>
				/// The handful of rendering rules every command's <c>describePlan</c> shares,
				/// kept in one place so the conformance corpus's vocabulary --- "yes"/"no",
				/// "none" for an absent value --- is spelled the same way everywhere.
				/// </summary>
				namespace PlanFormat
				{
					std::string yesNo(bool value);

					std::string noneOr(const std::optional<std::string>& value);

					std::string noneOr(const std::vector<std::string>& values);

					std::string inputs(const std::vector<Discovery::InputSource>& inputs);

					/// <summary>Joins a command's plan lines with "\n", matching C#'s <c>string.Join</c>.</summary>
					std::string joinLines(const std::vector<std::string>& lines);
				}
			}
		}
	}
}

#endif
