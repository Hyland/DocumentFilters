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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_HELPCOMMAND_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_HELPCOMMAND_H

#include "ICommand.h"

#include <optional>
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
				/// <c>help [command]</c> --- usage for the tool, or for one command.
				/// </summary>
				/// <remarks>
				/// The plain-text rendering comes from the CLI parsing library and therefore
				/// differs slightly between the four language implementations. That is
				/// accepted: help text is deliberately outside the cross-language contract.
				///
				/// <c>help --json</c> is different. It emits the command and argument
				/// reference as structured data, so it must be identical everywhere, and it
				/// renders through <c>ContractDump::renderJson</c> rather than the
				/// line-oriented <c>ContractDump::render</c>. The two share the argument
				/// table, not the renderer.
				/// </remarks>
				class HelpCommand : public ICommand
				{
				public:
					HelpCommand(std::optional<std::string> topic, bool json) : _topic(std::move(topic)), _json(json) {}

					std::string name() const override { return "help"; }

					bool requiresEngine() const override { return false; }

					int execute(CommandContext& context) const override;

					/// <summary>
					/// <c>help</c> processes no documents, so <c>--dry-run</c> has nothing to
					/// add to what running it outright already does: it never opens the
					/// engine.
					/// </summary>
					std::string describePlan(CommandContext& context) const override;

				private:
					std::optional<std::string> _topic;
					bool _json;
				};
			}
		}
	}
}

#endif
