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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_CONFIGURATION_COMMANDOPTIONS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_CONFIGURATION_COMMANDOPTIONS_H

#include "Planning/FormatSpec.h"
#include "Planning/NameMatcher.h"
#include "Planning/PageRange.h"

#include <optional>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Configuration
			{
				/// <summary>One <c>--redact page,x,y,w,h</c> region.</summary>
				struct RedactSpec
				{
					int page = 0;
					int x = 0;
					int y = 0;
					int width = 0;
					int height = 0;
				};

				/// <summary>One <c>--form page,name=value</c> assignment.</summary>
				struct FormValue
				{
					int page = 0;
					std::string name;
					std::string value;
				};

				/// <summary>
				/// The document and page structure aspects <c>convert</c> can report
				/// while it works, emitted during a conversion run rather than by a
				/// separate command.
				/// </summary>
				struct StructureReportOptions
				{
					bool fonts = false;
					bool fontMappings = false;
					bool pageFonts = false;
					bool pageFontMappings = false;
					bool dimensions = false;
					bool pageElements = false;
					bool formElements = false;
					bool hyperlinks = false;
					bool bookmarks = false;
					bool annotations = false;

					/// <summary>True when any report was requested.</summary>
					bool any() const
					{
						return fonts || fontMappings || pageFonts || pageFontMappings || dimensions || pageElements || formElements
							|| hyperlinks || bookmarks || annotations;
					}

					/// <summary>
					/// The report flags asked for, spelled as the user typed them, in a
					/// fixed order.
					/// </summary>
					/// <remarks>
					/// So that a message naming one report names the same one in all four
					/// implementations: <c>--fonts --bookmarks</c> and
					/// <c>--bookmarks --fonts</c> are the same request and must produce the
					/// same error text. Hand-written beside the fields, in the same idiom
					/// as <c>ErrorCodes::all</c> and for the same reason --- C++17 has no
					/// reflection.
					/// </remarks>
					std::vector<std::string> requested() const
					{
						std::vector<std::string> result;
						if (fonts)
							result.push_back("--fonts");
						if (fontMappings)
							result.push_back("--font-mappings");
						if (pageFonts)
							result.push_back("--page-fonts");
						if (pageFontMappings)
							result.push_back("--page-font-mappings");
						if (dimensions)
							result.push_back("--dimensions");
						if (pageElements)
							result.push_back("--page-elements");
						if (formElements)
							result.push_back("--form-elements");
						if (hyperlinks)
							result.push_back("--hyperlinks");
						if (bookmarks)
							result.push_back("--bookmarks");
						if (annotations)
							result.push_back("--annotations");
						return result;
					}
				};

				/// <summary>Options that only <c>convert</c> accepts.</summary>
				struct ConvertOptions
				{
					/// <summary><c>--to</c>: the resolved target format. Required.</summary>
					const Planning::FormatSpec* format = nullptr;

					/// <summary><c>--combine</c>: merge every input into one output document.</summary>
					bool combine = false;

					/// <summary><c>--pages</c>. Absent means every page.</summary>
					std::optional<Planning::PageRange> pages;

					/// <summary><c>--canvas-options</c>, passed to the engine untouched.</summary>
					std::optional<std::string> canvasOptions;

					/// <summary><c>--render-options</c>, passed to the engine untouched.</summary>
					std::optional<std::string> renderOptions;

					/// <summary><c>--no-images</c>.</summary>
					bool noImages = false;

					/// <summary><c>--include-control-codes</c>: keep ISYS control codes in text output.</summary>
					bool includeControlCodes = false;

					/// <summary><c>--fingerprint</c>.</summary>
					bool fingerprint = false;

					/// <summary><c>--watermark</c>: a file path or a literal string.</summary>
					std::optional<std::string> watermark;

					/// <summary><c>--redact</c>, repeatable.</summary>
					std::vector<RedactSpec> redactions;

					/// <summary><c>--form</c>, repeatable.</summary>
					std::vector<FormValue> formValues;

					/// <summary><c>--view</c>: open the result when done. Best effort, platform specific.</summary>
					bool view = false;

					/// <summary>The structure reports emitted during conversion.</summary>
					StructureReportOptions reports;
				};

				/// <summary>Options that only <c>extract</c> accepts.</summary>
				struct ExtractOptions
				{
					/// <summary><c>-i, --images</c>: pull embedded images too, not just subfiles.</summary>
					bool images = false;

					/// <summary><c>--include</c>, repeatable: keep subfiles whose name matches.</summary>
					/// <remarks>
					/// Globs, the same pattern language as <c>-g/--glob</c> --- and
					/// repeatable for the same reason, so that one glob per idea reads
					/// better than one glob doing several jobs.
					///
					/// <b>A different axis from <c>-g/--glob</c>.</b> That one chooses
					/// which files on disk to process; this one chooses which subfiles
					/// inside a container to extract --- the same distinction
					/// <c>--max-depth</c> and <c>--subfile-depth</c> draw between outside
					/// the document and inside it.
					/// </remarks>
					std::vector<std::string> includePatterns;

					/// <summary><c>--exclude</c>, repeatable: drop subfiles whose name matches.</summary>
					std::vector<std::string> excludePatterns;

					/// <summary>
					/// <c>includePatterns</c> compiled, or absent when none were given.
					/// </summary>
					/// <remarks>
					/// Built during binding, so a malformed pattern is reported before any
					/// archive is opened rather than with one half-extracted.
					/// </remarks>
					std::optional<Planning::NameMatcher> subfileInclude;

					/// <summary><c>excludePatterns</c> compiled, or absent when none were given.</summary>
					std::optional<Planning::NameMatcher> subfileExclude;

					/// <summary><c>--make-dirs</c>: replicate the source tree under the output directory.</summary>
					/// <remarks>
					/// Recorded separately from <c>flatWasGiven</c> for the same reason
					/// <c>--max-depth</c> and <c>--no-recurse</c> are kept apart: collapsing
					/// two opposing switches into one boolean would make
					/// <c>--make-dirs --flat</c> indistinguishable from <c>--flat</c>
					/// alone, so the contradiction could not be reported.
					/// </remarks>
					bool makeDirsWasGiven = false;

					/// <summary><c>--flat</c>: put every extracted file straight into the output directory.</summary>
					bool flatWasGiven = false;

					/// <summary>
					/// Whether the source tree is replicated. Defaults to true, so
					/// <c>extract</c> lays out its output the way <c>convert</c> does.
					/// </summary>
					bool replicateTree() const { return !flatWasGiven; }

					/// <summary><c>--sub-id</c>: append the subfile id to extracted filenames.</summary>
					bool appendSubfileId = false;
				};

				/// <summary>Options that only <c>list</c> accepts.</summary>
				struct ListOptions
				{
					bool json = false;
				};

				/// <summary>Options that only <c>identify</c> accepts.</summary>
				struct IdentifyOptions
				{
					bool json = false;

					/// <summary><c>--capabilities</c>: also print the format's capability flags.</summary>
					bool capabilities = false;
				};

				/// <summary>Options that only <c>meta</c> accepts.</summary>
				struct MetaOptions
				{
					bool json = false;
				};

				/// <summary>Options that only <c>formats</c> accepts.</summary>
				struct FormatsOptions
				{
					bool json = false;

					/// <summary><c>--filter</c>: only formats whose id or name matches.</summary>
					std::optional<std::string> filter;
				};

				/// <summary>Options that only the <c>options</c> command accepts.</summary>
				struct OptionCatalogOptions
				{
					bool json = false;

					/// <summary><c>--filter</c>: only options whose name or description matches.</summary>
					std::optional<std::string> filter;

					/// <summary><c>--applies-to</c>: document, page, canvas, render, and so on.</summary>
					std::optional<std::string> appliesTo;
				};
			}
		}
	}
}

#endif
