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

#include "Execution/ExtractorOpenSettings.h"

#include "Execution/CanvasConversion.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				namespace
				{
					// The C++ binding has no OpenType wrapper; these are the raw
					// IGR_* flag words the managed binding's enum carries.
					constexpr int igrMetaOnly = 1;
					constexpr int igrBodyAndMeta = 2;
					constexpr int igrFormatText = 0x00010000;
					constexpr int igrFormatHtml = 0x00020000;
					constexpr int igrFormatImage = 0x00050000;
					constexpr int igrFormatMarkdown = 0x00080000;
				}

				ExtractorOpenSettings ExtractorOpenSettings::forFormat(
					const Planning::FormatSpec& format, const Configuration::ConvertOptions& options, const Configuration::SharedOptions& shared)
				{
					std::string engineOptions = buildOptionsString(shared);
					if (options.noImages && format.kind == Planning::FormatKind::Text)
					{
						std::string suppressed = CanvasConversion::imageSuppression(format);
						if (!suppressed.empty())
							engineOptions = append(engineOptions, suppressed);
					}

					ExtractorOpenSettings result;
					result.flags = igrBodyAndMeta | formatBit(format, options);
					result.options = engineOptions;
					return result;
				}

				std::string ExtractorOpenSettings::append(const std::string& existing, const std::string& addition)
				{
					std::string result = existing;
					if (!result.empty() && result.back() != ';')
						result += ";";
					return result + addition + ";";
				}

				int ExtractorOpenSettings::formatBit(const Planning::FormatSpec& format, const Configuration::ConvertOptions& options)
				{
					if (format.kind == Planning::FormatKind::Canvas)
						return igrFormatImage;

					if (format.kind == Planning::FormatKind::DocPack)
						return 0;

					if (format.token == "text")
						return options.includeControlCodes ? 0 : igrFormatText;
					if (format.token == "md-text")
						return igrFormatMarkdown;
					if (format.token == "html")
						return igrFormatHtml;

					return igrFormatText;
				}

				ExtractorOpenSettings ExtractorOpenSettings::forInspection(const Configuration::SharedOptions& shared, bool enumerateImages)
				{
					std::string options = buildOptionsString(shared);
					if (enumerateImages)
					{
						if (!options.empty() && options.back() != ';')
							options += ";";
						options += "ENUMERATE_IMAGES=ON;";
					}

					ExtractorOpenSettings result;
					result.flags = igrBodyAndMeta | igrFormatText;
					result.options = options;
					return result;
				}

				ExtractorOpenSettings ExtractorOpenSettings::forMetadataOnly(const Configuration::SharedOptions& shared)
				{
					ExtractorOpenSettings result;
					result.flags = igrMetaOnly | igrFormatText;
					result.options = buildOptionsString(shared);
					return result;
				}

				std::string ExtractorOpenSettings::buildOptionsString(const Configuration::SharedOptions& shared)
				{
					std::string options = shared.processing.engineOptions.value_or(std::string());
					if (shared.processing.codepage.has_value() && !shared.processing.codepage->empty())
					{
						if (!options.empty() && options.back() != ';')
							options += ";";
						options += "CODEPAGE=" + *shared.processing.codepage + ";";
					}
					return options;
				}
			}
		}
	}
}
