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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_ENGINE_DOCUMENTFILTERSENGINE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_ENGINE_DOCUMENTFILTERSENGINE_H

#include "IDocumentFiltersEngine.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Engine
			{
				/// <summary>
				/// <see cref="IDocumentFiltersEngine"/> over the real library.
				/// </summary>
				/// <remarks>
				/// The only file in the project that touches
				/// <c>Hyland::DocFilters</c> types. Every method here is one binding call
				/// plus, where the binding hands back a type a test cannot construct, a
				/// copy into the plain values from <see cref="IDocumentFiltersEngine"/>.
				///
				/// Nothing here decides anything. If a method in this file grows an
				/// <c>if</c> that is not null-handling or error translation, it belongs
				/// above the seam.
				/// </remarks>
				class DocumentFiltersEngine : public IDocumentFiltersEngine
				{
				public:
					DocumentFiltersEngine();
					~DocumentFiltersEngine() override;

					void initialize(const std::string& licenseKey, const std::string& libraryPath) override;

					std::unique_ptr<IDocument> openDocument(const std::string& path, int flags, const std::string& options,
						const std::function<std::optional<std::string>()>& password, const std::function<bool()>& shouldCancel,
						const std::function<int(const std::string&)>& logLevelFor,
						const std::function<void(int, const std::string&, const std::string&)>& onLogMessage) override;

					std::unique_ptr<IDocument> openDocument(std::istream& source, const std::string& displayName, int flags,
						const std::string& options, const std::function<std::optional<std::string>()>& password,
						const std::function<bool()>& shouldCancel, const std::function<int(const std::string&)>& logLevelFor,
						const std::function<void(int, const std::string&, const std::string&)>& onLogMessage) override;

					void setTempPath(const std::string& path) override;

					std::string getMemoryStatus() override;

					EngineFileType identifyFile(const std::string& path) override;

					EngineFileType identifyFile(std::istream& source, const std::string& displayName) override;

					std::unique_ptr<IOutputCanvas> makeCanvas(
						const std::string& target, Hyland::DocFilters::CanvasType type, const std::string& options) override;

					std::unique_ptr<IOutputCanvas> makeCanvas(
						std::ostream& target, Hyland::DocFilters::CanvasType type, const std::string& options) override;

					std::vector<EngineFormat> getSupportedFormats() override;

					std::vector<EngineOption> getAvailableOptions() override;

					/// <summary>
					/// The native library's own file version, read from disk the way
					/// doc2text's <c>showVersion</c> does --- the managed binding this
					/// project ports from has no native call for this at all, so there is no
					/// single behaviour to be faithful to; reading the DLL's version
					/// resource is the closest thing this tool has to "the engine's own
					/// version".
					/// </summary>
					std::string describeVersion() override;

				private:
					Hyland::DocFilters::DocumentFilters _api;

					/// <summary>Remembered from <see cref="initialize"/>, for <see cref="describeVersion"/>.</summary>
					std::string _libraryPath;
				};
			}
		}
	}
}

#endif
