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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_METACOMMAND_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_METACOMMAND_H

#include "Configuration/CommandOptions.h"
#include "ICommand.h"

#include <utility>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				/// <summary><c>meta</c> --- output document metadata only.</summary>
				class MetaCommand : public ICommand
				{
				public:
					explicit MetaCommand(Configuration::MetaOptions options) : _options(std::move(options)) {}

					/// <summary>The bound per-command options. Readable so the binding itself can be tested.</summary>
					const Configuration::MetaOptions& options() const { return _options; }

					std::string name() const override { return "meta"; }

					bool requiresEngine() const override { return true; }

					int execute(CommandContext& context) const override;

					std::string describePlan(CommandContext& context) const override;

				private:
					Configuration::MetaOptions _options;
				};
			}
		}
	}
}

#endif
