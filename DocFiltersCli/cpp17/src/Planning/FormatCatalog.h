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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_FORMATCATALOG_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_FORMATCATALOG_H

#include "FormatSpec.h"

#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Planning
			{
				/// <summary>
				/// The <c>--to</c> token table: what each format name means, and the two
				/// static properties the output planner needs from it.
				/// </summary>
				/// <remarks>
				/// This table is the single source of truth for the format surface in all
				/// four language implementations, and several entries are not a straight
				/// mapping onto a single engine enum value:
				/// <list type="bullet">
				/// <item><c>md</c> is high-definition markdown via a canvas; <c>md-text</c>
				/// is text-mode markdown straight from the extractor.</item>
				/// <item><c>html</c> is flow HTML from the extractor; <c>html5</c> is
				/// paginated high-definition HTML via a canvas.</item>
				/// <item><c>docpack</c> is neither --- it goes through
				/// <c>Extractor::MakeDocPack</c>.</item>
				/// </list>
				/// </remarks>
				namespace FormatCatalog
				{
					/// <summary>Every format token this build accepts, in help-display order.</summary>
					const std::vector<FormatSpec>& all();

					/// <summary>Resolves a <c>--to</c> token, after an ASCII-only case fold.</summary>
					/// <exception cref="Runtime::UsageError">
					/// The token is not in the catalogue. The message lists where to find the
					/// valid ones, so that a typo is self-correcting.
					/// </exception>
					const FormatSpec& resolve(const std::string& token);

					/// <summary>Resolves a token, returning nullptr instead of throwing when it is unknown.</summary>
					const FormatSpec* tryResolve(const std::string& token);
				}
			}
		}
	}
}

#endif
