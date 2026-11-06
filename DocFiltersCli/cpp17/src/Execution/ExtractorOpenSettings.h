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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_EXTRACTOROPENSETTINGS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_EXTRACTOROPENSETTINGS_H

#include "Configuration/CommandOptions.h"
#include "Configuration/SharedOptions.h"
#include "Planning/FormatSpec.h"

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
				/// How a document should be opened: the engine flags and the options string
				/// passed to <c>IDocument::open</c>.
				/// </summary>
				/// <remarks>
				/// These have to be decided <em>before</em> the document is opened, because
				/// the engine applies them during <c>Open</c> --- a flag worked out
				/// afterwards has no effect. That is why
				/// <c>DocumentFiltersSession::openDocument</c> takes this rather than each
				/// conversion path computing flags for a document that is already open.
				///
				/// No <c>OpenType</c> enum: the C++ binding, unlike the managed one, has no
				/// wrapper around the raw <c>IGR_*</c> flag words, so <c>flags</c> is built
				/// directly from those macros. Simpler than the C# port here, not harder.
				/// </remarks>
				struct ExtractorOpenSettings
				{
					/// <summary>
					/// The complete <c>IGR_</c> open flags: the content selector, plus
					/// exactly one <c>IGR_FORMAT_*</c> bit --- or none, where the engine's
					/// default is what is wanted.
					/// </summary>
					/// <remarks>
					/// Complete because it has to be: these reach the engine exactly as
					/// built here, and nothing downstream supplies a format bit.
					///
					/// The <c>IGR_FORMAT_*</c> values are an enumeration packed into bits 16
					/// to 19 (text 0x1, HTML 0x2, image 0x5, markdown 0x8, each shifted by
					/// 16), not independent bits, so at most one may ever be set.
					/// </remarks>
					int flags = 0;

					/// <summary>The engine options string, from <c>--options</c> and <c>--codepage</c>.</summary>
					std::string options;

					/// <summary>The settings for converting to a given target format.</summary>
					/// <remarks>
					/// <c>--no-images</c> reaches a text-mode format here rather than
					/// through the canvas options, because <c>md-text</c> never opens a
					/// canvas. <c>md</c>, which does, is handled by
					/// <c>CanvasConversion::composeCanvasOptions</c>; both read the same
					/// rule from <c>CanvasConversion::imageSuppression</c> so it is stated
					/// once.
					/// </remarks>
					static ExtractorOpenSettings forFormat(
						const Planning::FormatSpec& format, const Configuration::ConvertOptions& options, const Configuration::SharedOptions& shared);

					/// <summary>
					/// The one <c>IGR_FORMAT_*</c> bit a conversion opens with, or zero for
					/// none.
					/// </summary>
					/// <remarks>
					/// A canvas format needs <c>IGR_FORMAT_IMAGE</c>: that is what makes
					/// pages exist. A text format names the reader it wants, and
					/// <c>text</c> asks for none at all under <c>--include-control-codes</c>
					/// --- omitting <c>IGR_FORMAT_TEXT</c> is what leaves the ISYS control
					/// codes in the stream instead of having the engine clean them out.
					/// <c>docpack</c> takes none either, matching doc2text, which opens it
					/// with a bare <c>IGR_BODY_AND_META</c>.
					///
					/// Structure reports need no case of their own: they require a
					/// paginated handle, and <c>OptionsValidator::checkReportsNeedCanvasFormat</c>
					/// refuses them against anything but a canvas format, which already
					/// opens paginated --- so by the time this runs, a run with reports has
					/// a canvas format.
					///
					/// Pure, and separate from <see cref="forFormat"/>, so the whole table
					/// can be checked without an engine.
					/// </remarks>
					static int formatBit(const Planning::FormatSpec& format, const Configuration::ConvertOptions& options);

					/// <summary>
					/// The settings for a command that reads a document without converting
					/// it --- <c>identify</c>, <c>meta</c>, <c>list</c> and <c>extract</c>.
					/// </summary>
					/// <param name="enumerateImages">
					/// <c>extract --images</c>: ask the engine to surface a document's
					/// embedded images as well as its subfiles.
					/// </param>
					/// <remarks>
					/// <c>IGR_FORMAT_TEXT</c>: these commands enumerate subfiles and read
					/// metadata rather than rendering, so they want the text reader and no
					/// pages.
					///
					/// <c>ENUMERATE_IMAGES</c> has to be set here, at open, because that is
					/// the only place the engine reads it --- <c>IDocument::images()</c> is
					/// empty without it, and so is the subfile list for a document whose
					/// only subfiles are images. Appended after the user's <c>--options</c>
					/// so an explicit <c>ENUMERATE_IMAGES=OFF</c> still wins, the engine
					/// resolving a repeated key to its first occurrence.
					/// </remarks>
					static ExtractorOpenSettings forInspection(const Configuration::SharedOptions& shared, bool enumerateImages = false);

					/// <summary>
					/// The settings for <c>meta</c> specifically: <c>IGR_META_ONLY</c> rather
					/// than the <c>IGR_BODY_AND_META</c> every other inspection command
					/// uses.
					/// </summary>
					/// <remarks>
					/// Confirmed against doc2text's own <c>--meta</c> flag (<c>main.cpp</c>:
					/// <c>flags = IGR_META_ONLY;</c> --- an assignment that replaces the
					/// default entirely, not an addition to it). <c>MetadataReport</c>
					/// parses <c>getText</c>'s output as one <c>Label: value</c> line per
					/// field; opening with body text included as well would mix document
					/// content into that same stream.
					/// </remarks>
					static ExtractorOpenSettings forMetadataOnly(const Configuration::SharedOptions& shared);

				private:
					/// <summary>
					/// Builds the semicolon-separated engine options string from
					/// <c>--options</c> and <c>--codepage</c>, in the form doc2text itself
					/// uses: <c>...;CODEPAGE=1252;</c>.
					/// </summary>
					static std::string buildOptionsString(const Configuration::SharedOptions& shared);

					/// <summary>Joins two engine option strings, inserting a separator where needed.</summary>
					static std::string append(const std::string& existing, const std::string& addition);
				};
			}
		}
	}
}

#endif
