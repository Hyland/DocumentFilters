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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_VERSIONCOMMAND_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_VERSIONCOMMAND_H

#include "ICommand.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				/// <summary>
				/// <c>version</c> --- the tool version and the Document Filters build it is
				/// running against.
				/// </summary>
				/// <remarks>
				/// This is the one command that needs the engine without opening any
				/// document: the interesting half of the answer is the native library's
				/// version, which requires initialising it.
				/// </remarks>
				class VersionCommand : public ICommand
				{
				public:
					std::string name() const override { return "version"; }

					bool requiresEngine() const override { return true; }

					int execute(CommandContext& context) const override;

					/// <summary>
					/// <c>version</c> reports the tool and native library versions outright;
					/// a dry run has nothing further to plan.
					/// </summary>
					std::string describePlan(CommandContext& context) const override;
				};
			}
		}
	}
}

#endif
