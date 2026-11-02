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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_CONVERTCOMMAND_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_CONVERTCOMMAND_H

#include "Configuration/CommandOptions.h"
#include "ICommand.h"
#include "Planning/RunPlan.h"

#include <optional>
#include <string>
#include <utility>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				/// <summary>
				/// <c>convert</c> --- render documents to a target format, and optionally
				/// report document and page structure while doing so.
				/// </summary>
				class ConvertCommand : public ICommand
				{
				public:
					explicit ConvertCommand(Configuration::ConvertOptions options) : _options(std::move(options)) {}

					/// <summary>The bound per-command options. Readable so the binding itself can be tested.</summary>
					const Configuration::ConvertOptions& options() const { return _options; }

					std::string name() const override { return "convert"; }

					bool requiresEngine() const override { return true; }

					int execute(CommandContext& context) const override;

					std::string describePlan(CommandContext& context) const override;

					/// <summary>
					/// The one file a run produced, for <c>--view</c> to open --- or
					/// absent when there is more than one, or none at all.
					/// </summary>
					/// <remarks>
					/// "Single output" is the spec's own wording: several inputs, or one
					/// input to a per-page format, leave nothing unambiguous to open. Pure,
					/// so <c>--view</c>'s target can be checked without a filesystem or an
					/// engine.
					/// </remarks>
					static std::optional<std::string> singleOutputOf(const Planning::RunPlan& plan);

				private:
					Configuration::ConvertOptions _options;
				};
			}
		}
	}
}

#endif
