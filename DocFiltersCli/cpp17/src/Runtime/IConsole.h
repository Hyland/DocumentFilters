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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_ICONSOLE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_ICONSOLE_H

#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// Everything the tool writes to the terminal, behind one seam so tests can
				/// capture output without a console.
				/// </summary>
				/// <remarks>
				/// <c>--silent</c>, <c>--verbose</c> and <c>--color</c> are honoured by the
				/// implementation, not by callers: call sites always say what they mean and
				/// let this decide whether it is shown.
				/// </remarks>
				class IConsole
				{
				public:
					virtual ~IConsole() = default;

					/// <summary>Report output --- the answer the user asked for. Goes to stdout.</summary>
					virtual void write(const std::string& text) = 0;

					/// <summary>Report output followed by a newline.</summary>
					virtual void writeLine(const std::string& text) = 0;

					/// <summary>An error message. Goes to stderr, and is part of the cross-language contract.</summary>
					virtual void writeError(const std::string& text) = 0;

					/// <summary>Progress or diagnostic chatter. Suppressed unless <c>--verbose</c>.</summary>
					virtual void writeVerbose(const std::string& text) = 0;

					/// <summary>
					/// The raw stdout stream, for <c>--stdout</c> with a binary format. An
					/// implementation backed by the real console must put it in binary mode so
					/// that PDF and image bytes survive on Windows.
					/// </summary>
					virtual std::ostream& openStandardOutput() = 0;

					/// <summary>True when stdin is an interactive terminal, which <c>--password-prompt</c> requires.</summary>
					virtual bool isInputInteractive() const = 0;

					/// <summary>
					/// Every line of standard input, for <c>--files-from -</c>.
					/// </summary>
					/// <remarks>
					/// Reading stdin belongs here rather than on <see cref="IFileSystem"/>:
					/// "-" is not a path, and routing it through the filesystem would leave
					/// tests unable to supply it.
					/// </remarks>
					virtual std::vector<std::string> readAllInputLines() = 0;

					/// <summary>
					/// Prompts for a password and reads it without echoing, for
					/// <c>--password-prompt</c>. Returns nullopt when the user cancels.
					/// </summary>
					virtual std::optional<std::string> readPassword(const std::string& prompt) = 0;

					/// <summary>Waits for a keypress between documents, for <c>--pause</c>.</summary>
					virtual void waitForKey() = 0;
				};
			}
		}
	}
}

#endif
