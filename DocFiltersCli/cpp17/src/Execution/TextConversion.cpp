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

#include "Execution/TextConversion.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				namespace TextConversion
				{
					void convert(Engine::IDocument& document, const Planning::FormatSpec* format, const Configuration::ConvertOptions& options,
						std::ostream& destination)
					{
						bool isFlowHtml = format != nullptr && format->token == "html";
						while (!document.getEOF())
						{
							std::string chunk = document.getText(4096);
							destination << (options.includeControlCodes ? chunk : stripControlCodes(chunk, isFlowHtml));
						}
					}

					std::string stripControlCodes(const std::string& chunk, bool isFlowHtml)
					{
						std::string stripped;
						stripped.reserve(chunk.size());

						for (char character : chunk)
						{
							switch (character)
							{
							case '\x01':
								stripped += ' ';
								break;

							case '\x02':
							case '\x03':
							case '\x04':
							case '\x05':
							case '\x06':
							case '\x0C':
							case '\x10':
								break;

							case '\x0E':
								if (!isFlowHtml)
									stripped += '\n';
								break;

							case '\x0F':
								stripped += '\n';
								break;

							default:
								stripped += character;
								break;
							}
						}

						return stripped;
					}
				}
			}
		}
	}
}
