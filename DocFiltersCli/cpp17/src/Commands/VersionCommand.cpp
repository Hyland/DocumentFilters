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

#include "VersionCommand.h"

#include "CommandExecution.h"
#include "Runtime/ExitCode.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				int VersionCommand::execute(CommandContext& context) const
				{
					return CommandExecution::withSession(context, [&context]()
						{
							// DOCFILTERSCLI_VERSION is defined by CMakeLists.txt from
							// DocFiltersCli/VERSION.txt, the file the C# and Python ports
							// read too. Only `--help` text is outside the cross-language
							// contract --- this line is compared, because there is nothing
							// language-specific about it to excuse a difference.
							context.console->writeLine(std::string("docfilters ") + DOCFILTERSCLI_VERSION);
							context.console->writeLine("Document Filters " + context.session->describeVersion());
							return Runtime::ExitCode::success;
						});
				}

				std::string VersionCommand::describePlan(CommandContext&) const { return "command version"; }
			}
		}
	}
}
