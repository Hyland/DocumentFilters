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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_CATALOGREPORTS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_CATALOGREPORTS_H

#include "Configuration/CommandOptions.h"
#include "Execution/DocumentFiltersSession.h"
#include "Runtime/IConsole.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>
				/// The two catalogue commands: <c>formats</c> and <c>options</c>. Both
				/// ask the engine what this build supports and print it; neither opens a
				/// document.
				/// </summary>
				class CatalogReports
				{
				public:
					CatalogReports(DocumentFiltersSession& session, Runtime::IConsole& console) : _session(session), _console(console) {}

					/// <summary>
					/// <c>formats</c> --- every readable file type, from
					/// <c>getSupportedFormats</c>.
					/// </summary>
					/// <remarks>
					/// There is no capability column here, unlike <c>identify</c>:
					/// capability flags (text/subfiles/hd) exist only on an opened
					/// document (<c>getFileCapabilities</c>), and neither
					/// <c>Format</c> nor doc2text's own catalogue dump exposes them for a
					/// format nothing has been opened for.
					/// </remarks>
					void writeFormats(const Configuration::FormatsOptions& options);

					/// <summary>
					/// <c>options</c> --- every engine tunable, from
					/// <c>getAvailableOptions</c>. These are the keys accepted by
					/// <c>--options</c>.
					/// </summary>
					/// <remarks>
					/// <c>applies-to</c> comes from the option's flags: bit <c>0x1</c>
					/// means document, <c>0x2</c> means canvas, confirmed against
					/// doc2text's own <c>--options --json</c> dump.
					/// </remarks>
					void writeOptions(const Configuration::OptionCatalogOptions& options);

				private:
					DocumentFiltersSession& _session;
					Runtime::IConsole& _console;
				};
			}
		}
	}
}

#endif
