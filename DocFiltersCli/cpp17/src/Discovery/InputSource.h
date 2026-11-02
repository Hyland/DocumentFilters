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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_DISCOVERY_INPUTSOURCE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_DISCOVERY_INPUTSOURCE_H

#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Discovery
			{
				/// <summary>One file the walk selected for processing.</summary>
				struct InputSource
				{
					/// <summary>The path to open.</summary>
					std::string path;

					/// <summary>
					/// The path relative to the root the user named, used to replicate
					/// the source tree under <c>--out-dir</c>. For a file given directly
					/// on the command line this is just the filename.
					/// </summary>
					std::string relativePath;

					/// <summary>The directory root this input was found under, or nullopt for an explicit file argument.</summary>
					std::optional<std::string> root;
				};
			}
		}
	}
}

#endif
