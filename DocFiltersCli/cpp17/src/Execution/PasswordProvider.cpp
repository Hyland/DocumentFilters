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

#include "Execution/PasswordProvider.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				std::optional<std::string> PasswordProvider::getPassword(const std::string& documentId)
				{
					if (!_fileCandidates.has_value())
						loadPasswordFile();

					const std::vector<std::string>& candidates = _options.passwords;
					if (_nextIndex < candidates.size())
						return candidates[_nextIndex++];

					std::size_t fileIndex = _nextIndex - candidates.size();
					if (fileIndex < _fileCandidates->size())
					{
						_nextIndex++;
						return (*_fileCandidates)[fileIndex];
					}

					if (_options.prompt && !_promptExhausted && _console.isInputInteractive())
					{
						_promptExhausted = true;
						return _console.readPassword("Password for " + documentId + ": ");
					}

					return std::nullopt;
				}

				void PasswordProvider::reset()
				{
					_nextIndex = 0;
					_promptExhausted = false;
				}

				void PasswordProvider::loadPasswordFile()
				{
					if (!_options.passwordFile.has_value() || _options.passwordFile->empty())
					{
						_fileCandidates = std::vector<std::string>();
						return;
					}

					std::vector<std::string> lines = _fileSystem.readAllLines(*_options.passwordFile);
					std::vector<std::string> candidates;
					for (const std::string& line : lines)
					{
						if (!line.empty())
							candidates.push_back(line);
					}
					_fileCandidates = std::move(candidates);
				}
			}
		}
	}
}
