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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_TEXTCONVERSION_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_TEXTCONVERSION_H

#include "Configuration/CommandOptions.h"
#include "Engine/IDocumentFiltersEngine.h"
#include "Planning/FormatSpec.h"

#include <ostream>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>
				/// The text path: formats produced straight from the extractor, without a
				/// canvas --- <c>text</c>, <c>md-text</c> and flow <c>html</c>.
				/// </summary>
				/// <remarks>
				/// This is the shortest and most-copied piece of Document Filters code
				/// there is: open the extractor with the right arguments, then pull text
				/// until EOF.
				/// </remarks>
				namespace TextConversion
				{
					/// <summary>
					/// Writes one document's text to <paramref name="destination"/>.
					/// </summary>
					/// <remarks>
					/// <b>Precondition:</b> <paramref name="document"/> must already have
					/// been opened with <c>ExtractorOpenSettings::forFormat</c> for this
					/// same <paramref name="format"/>. The engine applies those flags
					/// during <c>Open</c> and they cannot be changed afterwards; the
					/// binding exposes no way to read them back, so this is enforced by
					/// <c>ConvertCommand</c> being the only caller and opening the document
					/// in one step.
					///
					/// Control codes are stripped unless <c>--include-control-codes</c>
					/// asks for them; nothing else is rewritten --- no line-ending
					/// translation, no re-encoding --- because the payload is the document.
					/// </remarks>
					void convert(Engine::IDocument& document, const Planning::FormatSpec* format, const Configuration::ConvertOptions& options,
						std::ostream& destination);

					/// <summary>
					/// Replaces the ISYS control codes the extractor emits with what they
					/// mean.
					/// </summary>
					/// <remarks>
					/// This has to happen here. It cannot be asked of the engine: a
					/// text-mode open resolves to the same reader whether
					/// <c>IGR_FORMAT_TEXT</c> is named or left out, so there is no flag
					/// that turns this on or off. doc2text strips them in the tool for the
					/// same reason:
					/// <list type="bullet">
					/// <item><c>0x01</c> start-of-heading becomes a space.</item>
					/// <item><c>0x02</c>-<c>0x06</c> (normal, bold, italic, underline,
					/// start-of-hit), <c>0x0C</c> soft break and <c>0x10</c> end-of-meta
					/// are dropped: they mark styling this tool does not carry.</item>
					/// <item><c>0x0E</c> paragraph and <c>0x0F</c> start-of-meta become
					/// newlines.</item>
					/// </list>
					/// Safe on a chunk boundary because every code is a single character
					/// and none depends on its neighbours, so splitting the stream cannot
					/// split one.
					///
					/// Flow <c>html</c> keeps its paragraph marks: doc2text suppresses the
					/// <c>0x0E</c> newline when the open flags say HTML, where the markup
					/// already carries the structure and a newline would only add
					/// whitespace.
					/// </remarks>
					/// <param name="isFlowHtml">
					/// True for flow <c>html</c> only, which wants no newline from a
					/// paragraph mark. Everything else --- including <c>meta</c>, whose
					/// records the engine separates with these same codes --- passes
					/// false.
					/// </param>
					std::string stripControlCodes(const std::string& chunk, bool isFlowHtml = false);
				}
			}
		}
	}
}

#endif
