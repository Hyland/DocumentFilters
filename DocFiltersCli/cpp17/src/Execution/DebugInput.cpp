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

#include "Execution/DebugInput.h"

#include <algorithm>
#include <sstream>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				namespace
				{
					/// <summary>
					/// The document's bytes in memory, optionally only the first
					/// <paramref name="limit"/> of them.
					/// </summary>
					/// <remarks>
					/// <c>--debug-truncate</c> implies the memory stream: reading a prefix
					/// means deciding where the bytes stop, which a file handed to the
					/// engine by name cannot express.
					/// </remarks>
					std::unique_ptr<std::istream> inMemory(const std::string& path, const std::optional<long long>& limit, Runtime::IFileSystem& fileSystem)
					{
						std::unique_ptr<std::istream> source = fileSystem.openRead(path);
						std::string buffer;

						if (limit.has_value())
						{
							char chunk[64 * 1024];
							long long remaining = *limit;
							while (remaining > 0)
							{
								std::streamsize wanted = static_cast<std::streamsize>(std::min<long long>(sizeof(chunk), remaining));
								source->read(chunk, wanted);
								std::streamsize got = source->gcount();
								if (got <= 0)
									break;

								buffer.append(chunk, static_cast<std::size_t>(got));
								remaining -= got;
							}
						}
						else
						{
							std::ostringstream all;
							all << source->rdbuf();
							buffer = all.str();
						}

						return std::make_unique<std::istringstream>(buffer);
					}
				}

				namespace DebugInput
				{
					std::unique_ptr<std::istream> for_(const std::string& path, const Configuration::DiagnosticOptions& options,
						Runtime::IFileSystem& fileSystem, Runtime::IConsole& console, const std::string& displayName)
					{
						// Two independent questions --- where the bytes come from, and
						// whether the asking is counted --- so they compose rather than
						// exclude each other. Handling them with an if/else here would
						// silently drop --debug-measure-io whenever a truncated or
						// buffered source was also asked for, which is the same "accepted
						// and ignored" failure these switches exist to expose.
						std::unique_ptr<std::istream> source;

						if (options.truncateInputBytes.has_value() || options.useMemoryStream)
							source = inMemory(path, options.truncateInputBytes, fileSystem);

						if (options.measureIo)
						{
							// Counting a buffer rather than a file still answers the
							// question the switch asks: the figures are what the *engine
							// requested*, not what the disk served. Combined with
							// --debug-truncate that is the more useful pairing of the two
							// --- how much of a prefix the engine consumed before it gave
							// up.
							std::unique_ptr<std::istream> underlying = source ? std::move(source) : fileSystem.openRead(path);
							source = std::make_unique<MeasuredStream>(std::move(underlying), console, displayName);
						}

						return source;
					}
				}

				std::streamsize Detail::CountingStreambuf::xsgetn(char* s, std::streamsize n)
				{
					std::streamsize got = _inner.sgetn(s, n);
					_reads++;
					_bytesRead += got;
					return got;
				}

				Detail::CountingStreambuf::pos_type Detail::CountingStreambuf::seekoff(
					off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which)
				{
					_seeks++;
					return _inner.pubseekoff(off, dir, which);
				}

				Detail::CountingStreambuf::pos_type Detail::CountingStreambuf::seekpos(pos_type pos, std::ios_base::openmode which)
				{
					_seeks++;
					return _inner.pubseekpos(pos, which);
				}

				MeasuredStream::MeasuredStream(std::unique_ptr<std::istream> source, Runtime::IConsole& console, std::string displayName)
					: Detail::MeasuredStreamBuffers(std::move(source)), std::istream(&buf), _console(console), _displayName(std::move(displayName))
				{
					inner->seekg(0, std::ios::end);
					_length = static_cast<std::streamsize>(inner->tellg());
					inner->seekg(0, std::ios::beg);
				}

				MeasuredStream::~MeasuredStream()
				{
					if (!_reported)
					{
						_reported = true;
						_console.writeLine(_displayName + ": reads=" + std::to_string(buf.reads()) + " seeks=" + std::to_string(buf.seeks())
							+ " bytes=" + std::to_string(buf.bytesRead()));
					}
				}
			}
		}
	}
}
