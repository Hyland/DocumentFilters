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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_SYSTEMFILESYSTEM_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_SYSTEMFILESYSTEM_H

#include "IFileSystem.h"
#include <map>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>The real filesystem.</summary>
				/// <remarks>
				/// Every path this returns uses forward slashes, whatever separator the OS
				/// or the caller used going in: <c>NameMatcher</c> and the rest of the
				/// portable walk logic compare paths as plain strings, so a backslash on
				/// Windows would make every glob and regex platform-specific.
				/// </remarks>
				class SystemFileSystem : public IFileSystem
				{
				public:
					bool fileExists(const std::string& path) override;

					bool directoryExists(const std::string& path) override;

					std::vector<std::string> listDirectory(const std::string& path) override;

					/// <summary>
					/// A dot-prefixed name is hidden on every platform this runs on, in
					/// addition to whatever the OS itself flags --- POSIX has no hidden
					/// attribute at all, and checking only the OS's own flag would make
					/// <c>--include-hidden</c> a no-op there.
					/// </summary>
					bool isHidden(const std::string& path) override;

					bool isSymbolicLink(const std::string& path) override;

					std::string getDirectoryIdentity(const std::string& path) override;

					bool isCaseSensitive(const std::string& path) override;

				private:
					/// <summary>Remembered per probed directory; the volume does not change mid-run.</summary>
					std::map<std::string, bool> _caseSensitivity;

				public:

					std::unique_ptr<std::istream> openRead(const std::string& path) override;

					std::unique_ptr<std::ostream> createFile(const std::string& path) override;

					void createDirectory(const std::string& path) override;

					void moveFile(const std::string& from, const std::string& to) override;

					std::vector<std::string> readAllLines(const std::string& path) override;
				};
			}
		}
	}
}

#endif
