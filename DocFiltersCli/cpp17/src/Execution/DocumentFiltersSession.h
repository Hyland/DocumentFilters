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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_DOCUMENTFILTERSSESSION_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_DOCUMENTFILTERSSESSION_H

#include "Configuration/SharedOptions.h"
#include "Engine/IDocumentFiltersEngine.h"
#include "Execution/ExtractorOpenSettings.h"

#include <functional>
#include <istream>
#include <memory>
#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				class PasswordProvider;
				class EngineLog;

				/// <summary>
				/// Owns the one Document Filters instance for the whole run.
				/// </summary>
				/// <remarks>
				/// The engine is expensive to create and is designed to be initialised once
				/// for the lifetime of the process, so this is created lazily --- the first
				/// time a command actually needs to open something --- and disposed at the
				/// end. <c>--print-plan</c> therefore never needs a license key.
				///
				/// Holds its engine by reference rather than by owning pointer, with
				/// ownership tracked separately: <see cref="forEngine"/> is how the tests
				/// put a fake engine (which the test itself keeps constructing and
				/// inspecting) behind the execution paths, so the session must never delete
				/// it; <see cref="create"/> builds and therefore owns the real one. C#'s
				/// reference semantics make this one field either way --- the fake survives
				/// being "disposed" regardless, since dispose there is just a flag.
				/// </remarks>
				class DocumentFiltersSession
				{
				public:
					/// <summary>
					/// Wraps an engine that is already initialised, without taking ownership
					/// of it.
					/// </summary>
					/// <remarks>
					/// How the tests put a fake engine behind the execution paths. The
					/// production route is <see cref="create"/>, which builds the real one.
					/// </remarks>
					static DocumentFiltersSession forEngine(Engine::IDocumentFiltersEngine& engine);

					/// <summary>Initialises the engine.</summary>
					/// <exception cref="Runtime::InitialisationError">
					/// The native library could not be found, or the license was rejected.
					/// </exception>
					static DocumentFiltersSession create(
						const Configuration::LibraryOptions& options, const std::optional<std::string>& tempDir = std::nullopt);

					/// <summary>
					/// <c>--library</c>, then <c>DF_PATH</c>, then the directory this tool's
					/// own executable lives in --- never the process's current directory.
					/// </summary>
					/// <remarks>
					/// This is also where <c>DocumentFilters::Initialize</c> looks for fonts,
					/// which is where the fonts package copies its bundled <c>fonts.ini</c>
					/// and TTFs at build time --- next to the executable, not next to
					/// whatever the caller's shell happens to be sitting in. doc2text and the
					/// other published samples default this to <c>"."</c> and get away with
					/// it only because they are always run from their own build output
					/// directory; this tool is invoked from wherever the user's documents
					/// are, so <c>"."</c> there resolves to a directory with no fonts at all.
					/// The engine still renders in that case --- it does not fail to
					/// initialise --- but falls back to a font it cannot rasterise glyphs
					/// from, so text comes out blank instead of missing.
					/// <paramref name="baseDirectory"/> is threaded in rather than read here
					/// so this stays a pure function of its inputs.
					/// </remarks>
					static std::string resolveLibraryPath(const Configuration::LibraryOptions& options,
						const std::optional<std::string>& environmentValue, const std::string& baseDirectory);

					/// <summary>The initialised engine, for the execution helpers.</summary>
					Engine::IDocumentFiltersEngine& api() const { return _engine; }

					/// <summary>
					/// Opens one document with the given flags and options, wiring up the
					/// password callback.
					/// </summary>
					/// <remarks>
					/// The open settings are passed in rather than worked out here, because
					/// the engine applies them during <c>Open</c>: the format-specific bits
					/// for flow HTML or text-mode markdown have no effect once the document
					/// is already open. Each conversion path resolves its
					/// <c>ExtractorOpenSettings</c> first, and the document is opened exactly
					/// once.
					/// </remarks>
					/// <param name="shouldCancel">
					/// Polled while the engine parses; true stops the open. Empty for a
					/// caller with no deadline to enforce.
					/// </param>
					std::unique_ptr<Engine::IDocument> openDocument(const std::string& path, const ExtractorOpenSettings& open,
						PasswordProvider& passwords, const std::function<bool()>& shouldCancel = nullptr, const EngineLog* log = nullptr);

					/// <summary>
					/// The same, from a stream --- for the debug switches that change how
					/// bytes reach the engine.
					/// </summary>
					std::unique_ptr<Engine::IDocument> openDocument(std::istream& source, const std::string& displayName,
						const ExtractorOpenSettings& open, PasswordProvider& passwords,
						const std::function<bool()>& shouldCancel = nullptr, const EngineLog* log = nullptr);

					/// <summary>Version and build information, for the <c>version</c> command.</summary>
					std::string describeVersion() const { return _engine.describeVersion(); }

					~DocumentFiltersSession();

					DocumentFiltersSession(DocumentFiltersSession&&) = default;
					DocumentFiltersSession(const DocumentFiltersSession&) = delete;
					DocumentFiltersSession& operator=(const DocumentFiltersSession&) = delete;
					DocumentFiltersSession& operator=(DocumentFiltersSession&&) = delete;

				private:
					DocumentFiltersSession(Engine::IDocumentFiltersEngine& engine, std::unique_ptr<Engine::IDocumentFiltersEngine> owned)
						: _engine(engine), _owned(std::move(owned))
					{
					}

					Engine::IDocumentFiltersEngine& _engine;

					/// <summary>Non-null only when this session built the engine itself, via <see cref="create"/>.</summary>
					std::unique_ptr<Engine::IDocumentFiltersEngine> _owned;
				};
			}
		}
	}
}

#endif
