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

#include "EngineException.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Engine
			{
				EngineError EngineException::classify(int code)
				{
					switch (code)
					{
						case 0:
							return EngineError::none;
						case 1:
							return EngineError::openError;
						case 2:
							return EngineError::wrongType;
						case 3:
							return EngineError::inUse;
						case 4:
							return EngineError::notReadable;
						case 5:
							return EngineError::password;
						case 10:
							return EngineError::notFound;
						case 11:
							return EngineError::writeError;
						case 12:
							return EngineError::notValidForThisClass;
						case 13:
							return EngineError::error;
						case 14:
							return EngineError::invalidHandle;
						case 15:
							return EngineError::invalidPointer;
						case 16:
							return EngineError::invalidParameter;
						case 17:
							return EngineError::fileCorrupt;
						case 20:
							return EngineError::outOfMemory;
						case 21:
							return EngineError::badError;
						case 22:
							return EngineError::wrongThread;
						case 23:
							return EngineError::tooManyHandles;
						case 24:
							return EngineError::fontsNotFound;
						case 25:
							return EngineError::versionMismatch;
						case 26:
							return EngineError::actionNotSupported;
						case 27:
							return EngineError::cancelled;
						default:
							return EngineError::error;
					}
				}
			}
		}
	}
}
