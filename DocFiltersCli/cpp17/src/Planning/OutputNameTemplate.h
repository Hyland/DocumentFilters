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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_OUTPUTNAMETEMPLATE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_PLANNING_OUTPUTNAMETEMPLATE_H

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
				/// An <c>--out-name</c> filename template, such as
				/// <c>{stem}-{page:04}.{ext}</c>.
				/// </summary>
				/// <remarks>
				/// Placeholders:
				/// <list type="table">
				/// <item><term>{stem}</term><description>input filename without its extension</description></item>
				/// <item><term>{page}</term><description>one-based page number; {page:04} zero-pads to four digits</description></item>
				/// <item><term>{srcext}</term><description>the input's original extension, without a dot</description></item>
				/// <item><term>{ext}</term><description>the output format's extension, without a dot</description></item>
				/// </list>
				/// The template is compiled when the argument is parsed, not on first
				/// use, so a typo is reported before any work starts.
				/// </remarks>
				class OutputNameTemplate
				{
				public:
					/// <summary>
					/// The template used when <c>--out-name</c> is not given: <c>{stem}.{ext}</c>
					/// for single-file formats, <c>{stem}-{page:04}.{ext}</c> for
					/// one-per-page formats.
					/// </summary>
					static OutputNameTemplate defaultTemplate(bool isSingleFile);

					/// <summary>Compiles a template.</summary>
					/// <remarks>
					/// The template names a file <em>inside</em> <c>--out-dir</c>, so path
					/// separators and <c>..</c> are rejected here rather than sanitised at
					/// render time. Checking only the rendered output would miss them,
					/// because they can come from the template's own literal text as
					/// easily as from the input path.
					/// </remarks>
					/// <exception cref="Runtime::UsageError">
					/// An unknown placeholder, an unclosed brace, a malformed width such as
					/// <c>{page:xx}</c>, or a path separator or <c>..</c> anywhere in the
					/// template.
					/// </exception>
					static OutputNameTemplate parse(const std::string& text);

					/// <summary>True when the template refers to <c>{page}</c>, which a single-file format cannot satisfy.</summary>
					bool usesPage() const;

					/// <summary>Produces a filename. <paramref name="pageNumber"/> is ignored when the template has no <c>{page}</c>.</summary>
					std::string render(const std::string& sourcePath, const std::string& outputExtension, int pageNumber) const;

					/// <summary>The original template text.</summary>
					std::string toCanonicalString() const { return _originalText; }

				private:
					enum class SegmentType
					{
						Literal,
						Stem,
						Page,
						SrcExt,
						Ext
					};

					struct Segment
					{
						SegmentType type;
						std::string literalText;
						int width = 0;
					};

					std::string _originalText;
					std::vector<Segment> _segments;
				};
			}
		}
	}
}

#endif
