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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_ENGINE_ENGINEEXCEPTION_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_ENGINE_ENGINEEXCEPTION_H

#include <exception>
#include <stdexcept>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Engine
			{
				/// <summary>
				/// Every way a Document Filters call can fail, as the engine reports them.
				/// </summary>
				/// <remarks>
				/// One member per <c>IGR_E_*</c> constant in <c>ISYS11dfConstants</c>, with
				/// the constant's own value, so a code coming back from the binding maps
				/// across without a lookup table and an unrecognised one is still carried
				/// faithfully.
				///
				/// This is the axis the engine tests enumerate: for each outcome, what the
				/// CLI is supposed to do with it. Most become a <c>DocumentError</c> against
				/// one input and let the batch continue; a few are worth telling apart, and
				/// those are the interesting cases.
				/// </remarks>
				enum class EngineError
				{
					/// <summary>No error. Present so the enum can round-trip any code the engine returns.</summary>
					none = 0,

					/// <summary><c>IGR_E_OPEN_ERROR</c>: the file could not be opened at all.</summary>
					openError = 1,

					/// <summary><c>IGR_E_WRONG_TYPE</c>: the call does not apply to this kind of document.</summary>
					wrongType = 2,

					/// <summary><c>IGR_E_IN_USE</c>.</summary>
					inUse = 3,

					/// <summary><c>IGR_E_NOT_READABLE</c>: recognised, but nothing can be read out of it.</summary>
					notReadable = 4,

					/// <summary><c>IGR_E_PASSWORD</c>: a password is needed, or the one supplied was wrong.</summary>
					password = 5,

					/// <summary><c>IGR_E_NOT_FOUND</c>: no such file, page, subfile or option.</summary>
					notFound = 10,

					/// <summary><c>IGR_E_WRITE_ERROR</c>: the output could not be written.</summary>
					writeError = 11,

					/// <summary><c>IGR_E_NOT_VALID_FOR_THIS_CLASS</c>.</summary>
					notValidForThisClass = 12,

					/// <summary><c>IGR_E_ERROR</c>: the general failure the engine falls back on.</summary>
					error = 13,

					/// <summary><c>IGR_E_INVALID_HANDLE</c>: a handle used after it was closed.</summary>
					invalidHandle = 14,

					/// <summary><c>IGR_E_INVALID_POINTER</c>.</summary>
					invalidPointer = 15,

					/// <summary><c>IGR_E_INVALID_PARAMETER</c>: an option string or flag the engine rejected.</summary>
					invalidParameter = 16,

					/// <summary><c>IGR_E_FILE_CORRUPT</c>: the document is damaged.</summary>
					fileCorrupt = 17,

					/// <summary><c>IGR_E_OUT_OF_MEMORY</c>.</summary>
					outOfMemory = 20,

					/// <summary><c>IGR_E_BAD_ERROR</c>.</summary>
					badError = 21,

					/// <summary><c>IGR_E_WRONG_THREAD</c>.</summary>
					wrongThread = 22,

					/// <summary><c>IGR_E_TOO_MANY_HANDLES</c>: handles leaked, or a batch too large to hold open.</summary>
					tooManyHandles = 23,

					/// <summary><c>IGR_E_FONTS_NOT_FOUND</c>: rendering needs fonts the install does not have.</summary>
					fontsNotFound = 24,

					/// <summary><c>IGR_E_VERSION_MISMATCH</c>: the binding and the native library disagree.</summary>
					versionMismatch = 25,

					/// <summary><c>IGR_E_ACTION_NOT_SUPPORTED</c>: this format cannot do what was asked.</summary>
					actionNotSupported = 26,

					/// <summary>
					/// The open callback asked the engine to stop --- <c>IGR_CANCELLED</c>.
					/// </summary>
					/// <remarks>
					/// Not a failure of the document. This is the only way to interrupt the
					/// engine at all: the callback is raised during <c>Open</c> as
					/// <c>IGR_OPEN_CALLBACK_ACTION_HEARTBEAT</c>, and nothing in the API
					/// offers the same for rendering --- <c>IGR_Render_Page</c> takes no
					/// callback. So a caller that asks for this already knows why, and
					/// translates it into its own error rather than reporting the document
					/// as broken.
					/// </remarks>
					cancelled = 27
				};

				/// <summary>
				/// A Document Filters call failed. The one exception type above the seam.
				/// </summary>
				/// <remarks>
				/// Everything the binding can throw is translated into this, so nothing
				/// above the seam catches the binding's own exception type --- which is what
				/// lets a fake engine reproduce any failure the real one can, and what stops
				/// a binding type from leaking into the conversion paths.
				/// </remarks>
				class EngineException : public std::runtime_error
				{
				public:
					explicit EngineException(EngineError error, int code, const std::string& message, std::exception_ptr inner = nullptr)
						: std::runtime_error(message), _error(error), _code(code), _inner(inner)
					{
					}

					/// <summary>The failure, named.</summary>
					EngineError error() const { return _error; }

					/// <summary>
					/// The raw <c>IGR_E_*</c> value, kept even when <see cref="error"/> could
					/// not name it, so a code this enum does not know about is still
					/// reportable.
					/// </summary>
					int code() const { return _code; }

					/// <summary>The underlying failure, when this wraps one the binding threw.</summary>
					std::exception_ptr innerError() const { return _inner; }

					/// <summary>Maps a raw engine code onto <see cref="EngineError"/>.</summary>
					/// <remarks>
					/// An unknown code becomes <see cref="EngineError::error"/> --- the
					/// engine's own catch-all --- rather than throwing, because a newer
					/// native library returning a code this build has never heard of is not
					/// a reason to crash. <see cref="code"/> still carries the real value.
					/// </remarks>
					static EngineError classify(int code);

				private:
					EngineError _error;
					int _code;
					std::exception_ptr _inner;
				};
			}
		}
	}
}

#endif
