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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_METADATAREPORT_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_METADATAREPORT_H

#include "Engine/IDocumentFiltersEngine.h"
#include "Runtime/IConsole.h"
#include "Runtime/JsonWriter.h"

#include <string>
#include <utility>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>
				/// The <c>meta</c> command's output.
				/// </summary>
				/// <remarks>
				/// Console form is one <c>key: value</c> per line, no padding or boxes,
				/// so that the four language implementations can agree without anyone
				/// matching column widths. JSON form is a flat object with keys in the
				/// order the engine reports them.
				/// </remarks>
				namespace MetadataReport
				{
					/// <summary>Writes the plain-text form.</summary>
					void writeText(Engine::IDocument& document, const std::string& displayName, Runtime::IConsole& console);

					/// <summary>Writes one document's JSON object into an open writer.</summary>
					void writeJson(Engine::IDocument& document, const std::string& displayName, Runtime::JsonWriter& json);
				}
			}
		}
	}
}

#endif
