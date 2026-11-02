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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_VIEWER_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_VIEWER_H

#include "Runtime/IConsole.h"

#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// <c>--view</c>: hand the finished output to whatever the desktop opens
				/// it with.
				/// </summary>
				/// <remarks>
				/// Best effort, and deliberately so: what opens is the machine's
				/// business, there is no output of our own to compare, and a headless
				/// box has nothing to open it with at all. A failure to launch is
				/// reported and ignored rather than failing the run, because the
				/// conversion itself already succeeded.
				///
				/// Three platforms, three commands, the same three doc2text uses:
				/// <c>ShellExecute</c> on Windows, <c>open</c> on macOS, <c>xdg-open</c>
				/// elsewhere. Windows goes through the shell, which is what makes a file
				/// association apply rather than trying to execute the document.
				/// </remarks>
				namespace Viewer
				{
					/// <summary>Opens <paramref name="path"/>, reporting but not failing on trouble.</summary>
					void open(const std::optional<std::string>& path, IConsole& console);
				}
			}
		}
	}
}

#endif
