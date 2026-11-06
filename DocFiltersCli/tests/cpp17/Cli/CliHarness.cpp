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

#include "Cli/CliHarness.h"

#include "Program.h"

#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Tests
			{
				namespace Cli
				{
					int CliHarness::run(const std::vector<std::string>& args)
					{
						exitCode = Hyland::DocumentFilters::Cli::Program::run(args, console, fileSystem, &engine, _interruption,
							[this]() { return clock.now(); },
							[this](const std::optional<std::string>& path, Hyland::DocumentFilters::Cli::Runtime::IConsole&) {
								viewed.push_back(path.value_or(std::string()));
								viewedContent.push_back(path && fileSystem.fileExists(*path) ? fileSystem.readText(*path) : std::string());
							});
						return exitCode;
					}
				}
			}
		}
	}
}
