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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_SYSTEMCONSOLE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_SYSTEMCONSOLE_H

#include "Configuration/SharedOptions.h"
#include "IConsole.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// The real console. Honours <c>--silent</c>, <c>--verbose</c> and
				/// <c>--color</c>.
				/// </summary>
				/// <remarks>
				/// <c>--color</c> is applied here, by the sink, and only where the sink
				/// already knows what a line <em>is</em>: an error is red, verbose
				/// chatter is dim. Report output --- what the user actually asked for
				/// --- is never coloured, because it is routinely piped into something
				/// else and because this class cannot tell one part of a report from
				/// another.
				/// </remarks>
				class SystemConsole : public IConsole
				{
				public:
					explicit SystemConsole(const Configuration::ConsoleOptions& options);

					/// <summary>
					/// The same, with the colour decision supplied rather than detected.
					/// </summary>
					/// <remarks>
					/// Only tests use this. Capturing what this class writes means
					/// redirecting the real streams, and the ordinary constructor would
					/// then --- correctly --- decline to colour a redirected stream, so a
					/// test built on it could only ever observe the uncoloured case.
					/// </remarks>
					SystemConsole(const Configuration::ConsoleOptions& options, bool colorErrors, bool colorVerbose);

					/// <summary>
					/// Whether to colour a stream: asked for, and going somewhere that
					/// can show it.
					/// </summary>
					/// <remarks>
					/// Named and pure so the rule can be checked without a terminal.
					/// Detecting redirection is the ambient half and stays at the call
					/// site.
					/// </remarks>
					static bool shouldColor(bool requested, bool streamRedirected) { return requested && !streamRedirected; }

					/// <summary><c>--silent</c> means "print nothing but errors", so this is a no-op under it.</summary>
					void write(const std::string& text) override;

					void writeLine(const std::string& text) override;

					/// <summary>Always written, even under <c>--silent</c>: stderr is the contract.</summary>
					void writeError(const std::string& text) override;

					void writeVerbose(const std::string& text) override;

					std::ostream& openStandardOutput() override;

					bool isInputInteractive() const override;

					std::vector<std::string> readAllInputLines() override;

					/// <summary>Escape cancels and returns nullopt; Enter accepts what was typed so far.</summary>
					std::optional<std::string> readPassword(const std::string& prompt) override;

					void waitForKey() override;

				private:
					// Held by value, not by reference. main() builds these from
					// preScanConsoleOptions(args), which returns by value, so a reference
					// member would bind to a temporary that dies at the end of that
					// declaration
					Configuration::ConsoleOptions _options;
					bool _colorErrors;
					bool _colorVerbose;
				};
			}
		}
	}
}

#endif
