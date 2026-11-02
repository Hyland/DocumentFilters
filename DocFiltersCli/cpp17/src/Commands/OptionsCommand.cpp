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

#include "OptionsCommand.h"

#include "CommandExecution.h"
#include "Execution/CatalogReports.h"
#include "PlanFormat.h"
#include "Runtime/ExitCode.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				int OptionsCommand::execute(CommandContext& context) const
				{
					return CommandExecution::withSession(context, [&context, this]()
						{
							Execution::CatalogReports(*context.session, *context.console).writeOptions(_options);
							return Runtime::ExitCode::success;
						});
				}

				std::string OptionsCommand::describePlan(CommandContext&) const
				{
					std::vector<std::string> lines = {
						"command options",
						"json " + PlanFormat::yesNo(_options.json),
						"filter " + PlanFormat::noneOr(_options.filter),
						"applies-to " + PlanFormat::noneOr(_options.appliesTo)
					};
					return PlanFormat::joinLines(lines);
				}
			}
		}
	}
}
