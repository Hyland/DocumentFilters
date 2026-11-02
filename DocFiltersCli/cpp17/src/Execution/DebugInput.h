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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_DEBUGINPUT_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_EXECUTION_DEBUGINPUT_H

#include "Configuration/SharedOptions.h"
#include "Runtime/IConsole.h"
#include "Runtime/IFileSystem.h"

#include <istream>
#include <memory>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>
				/// The three debug switches that change how a document's bytes reach the
				/// engine.
				/// </summary>
				/// <remarks>
				/// <c>--debug-use-memory-stream</c>, <c>--debug-truncate</c> and
				/// <c>--debug-measure-io</c> are each the input stream behaving
				/// differently, so none of them can be an option on the ordinary
				/// open-by-path --- the engine has to be handed a stream this tool
				/// controls. doc2text does the same, swapping in its own stream before
				/// opening.
				///
				/// They exist to answer questions about the engine rather than about a
				/// document: whether a failure depends on file-backed I/O, how much of a
				/// file is really read before it fails, and how many reads a format
				/// costs.
				/// </remarks>
				namespace DebugInput
				{
					/// <summary>
					/// The stream to open <paramref name="path"/> through, or nullptr to
					/// let the engine open the path itself.
					/// </summary>
					/// <remarks>
					/// Null in the ordinary case, so a normal run never buffers a document
					/// or wraps anything --- the debug switches cost nothing when nobody
					/// asks for them.
					/// </remarks>
					std::unique_ptr<std::istream> for_(const std::string& path, const Configuration::DiagnosticOptions& options,
						Runtime::IFileSystem& fileSystem, Runtime::IConsole& console, const std::string& displayName);
				}

				namespace Detail
				{
					/// <summary>
					/// Forwards to another streambuf, counting bulk reads and seeks.
					/// </summary>
					/// <remarks>
					/// Only <c>xsgetn</c> and <c>seekoff</c>/<c>seekpos</c> are overridden
					/// --- that is the whole shape of how the engine reads a document:
					/// binary, positional, never one character at a time through
					/// formatted extraction. <c>underflow</c> is deliberately left
					/// unimplemented; nothing above the engine seam reads this stream any
					/// other way.
					/// </remarks>
					class CountingStreambuf : public std::streambuf
					{
					public:
						explicit CountingStreambuf(std::streambuf& inner) : _inner(inner) {}

						long long reads() const { return _reads; }
						long long seeks() const { return _seeks; }
						long long bytesRead() const { return _bytesRead; }

					protected:
						std::streamsize xsgetn(char* s, std::streamsize n) override;
						pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override;
						pos_type seekpos(pos_type pos, std::ios_base::openmode which) override;

					private:
						std::streambuf& _inner;
						long long _reads = 0;
						long long _seeks = 0;
						long long _bytesRead = 0;
					};

					/// <summary>
					/// Holds the wrapped stream and its counting streambuf, constructed
					/// ahead of <c>MeasuredStream</c>'s <c>std::istream</c> base --- a
					/// streambuf must exist before the istream that reads through it does,
					/// which a same-class member declared after that base cannot arrange.
					/// </summary>
					struct MeasuredStreamBuffers
					{
						std::unique_ptr<std::istream> inner;
						CountingStreambuf buf;

						explicit MeasuredStreamBuffers(std::unique_ptr<std::istream> source) : inner(std::move(source)), buf(*inner->rdbuf())
						{
						}
					};
				}

				/// <summary>
				/// A read-through stream that counts what the engine asks for ---
				/// <c>--debug-measure-io</c>.
				/// </summary>
				/// <remarks>
				/// Reports on destruction, so the figures cover the whole document rather
				/// than whatever had happened by the time anyone thought to look. Counts
				/// reads and seeks separately because they answer different questions: a
				/// format that seeks constantly is a different problem from one that reads
				/// the file twice.
				/// </remarks>
				class MeasuredStream : private Detail::MeasuredStreamBuffers, public std::istream
				{
				public:
					MeasuredStream(std::unique_ptr<std::istream> source, Runtime::IConsole& console, std::string displayName);
					~MeasuredStream() override;

					MeasuredStream(const MeasuredStream&) = delete;
					MeasuredStream& operator=(const MeasuredStream&) = delete;

					/// <summary>
					/// The wrapped document's length, read directly from the source ---
					/// not counted as a seek, the same way <c>Stream::Length</c> is a
					/// property rather than a call to <c>Seek</c> in the C# reference.
					/// </summary>
					std::streamsize length() const { return _length; }

				private:
					Runtime::IConsole& _console;
					std::string _displayName;
					std::streamsize _length;
					bool _reported = false;
				};
			}
		}
	}
}

#endif
