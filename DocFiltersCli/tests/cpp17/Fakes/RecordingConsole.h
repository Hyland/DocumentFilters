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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_FAKES_RECORDINGCONSOLE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_FAKES_RECORDINGCONSOLE_H

#include "Runtime/IConsole.h"

#include <deque>
#include <sstream>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Tests
			{
				namespace Fakes
				{
					/// <summary>
					/// Captures everything the tool would have printed, so that output can
					/// be asserted on without a terminal.
					/// </summary>
					/// <remarks>
					/// Standard output and standard error are kept apart deliberately: error
					/// text is part of the cross-language contract and is asserted
					/// precisely, while progress chatter is not.
					/// </remarks>
					class RecordingConsole : public Runtime::IConsole
					{
					public:
						/// <summary>Every line written to standard output, in order.</summary>
						std::vector<std::string> outLines;

						/// <summary>Every line written to standard error, in order. Part of the contract.</summary>
						std::vector<std::string> errorLines;

						/// <summary>Every verbose line. Not part of the contract.</summary>
						std::vector<std::string> verboseLines;

						/// <summary>Set this to exercise the <c>--password-prompt</c> paths.</summary>
						bool isInputInteractiveFlag = false;

						/// <summary>Standard input, for <c>--files-from -</c>.</summary>
						std::vector<std::string> inputLines;

						/// <summary>Answers handed to successive <see cref="readPassword"/> calls, in order.</summary>
						std::deque<std::optional<std::string>> passwordResponses;

						/// <summary>The prompts <see cref="readPassword"/> was called with.</summary>
						std::vector<std::string> passwordPrompts;

						/// <summary>How many times <see cref="waitForKey"/> was called, for <c>--pause</c>.</summary>
						int keyWaits = 0;

						std::vector<std::string> readAllInputLines() override { return inputLines; }

						std::optional<std::string> readPassword(const std::string& prompt) override
						{
							passwordPrompts.push_back(prompt);
							if (passwordResponses.empty())
								return std::nullopt;

							std::optional<std::string> response = passwordResponses.front();
							passwordResponses.pop_front();
							return response;
						}

						void waitForKey() override { ++keyWaits; }

						/// <summary>Everything written to the binary stdout stream.</summary>
						std::string binaryOutput() const { return _binary.str(); }

						/// <summary>Standard output as one string.</summary>
						std::string output() const { return _out.str(); }

						void write(const std::string& text) override { _out << text; }

						void writeLine(const std::string& text) override
						{
							_out << text << '\n';
							outLines.push_back(text);
						}

						void writeError(const std::string& text) override { errorLines.push_back(text); }

						void writeVerbose(const std::string& text) override { verboseLines.push_back(text); }

						bool isInputInteractive() const override { return isInputInteractiveFlag; }

						std::ostream& openStandardOutput() override { return _binary; }

					private:
						std::ostringstream _out;
						std::ostringstream _binary;
					};
				}
			}
		}
	}
}

#endif
