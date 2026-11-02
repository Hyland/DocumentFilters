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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_INTERRUPTION_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_RUNTIME_INTERRUPTION_H

#include "CliError.h"

#include <atomic>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				/// <summary>
				/// Whether the user has asked the run to stop --- Ctrl+C.
				/// </summary>
				/// <remarks>
				/// A plain object rather than a static flag so a test can raise one without
				/// touching the process, and so two runs in the same process cannot see
				/// each other's.
				///
				/// Read in two places, which between them are every point the run can be
				/// stopped at all: <c>BatchRunner</c> checks it between documents, and the
				/// engine's open callback polls it so a document that hangs while parsing
				/// does not have to finish first. Nothing can interrupt a page loop ---
				/// <c>IGR_Render_Page</c> takes no callback --- so a long render still runs
				/// to completion, the same limitation <c>--timeout</c> has.
				/// </remarks>
				class Interruption
				{
				public:
					/// <summary>True once the user has interrupted the run.</summary>
					/// <remarks>
					/// Atomic because a signal handler runs on a thread the caller does not
					/// control.
					/// </remarks>
					bool requested() const { return _requested.load(); }

					/// <summary>Records that the user asked to stop.</summary>
					void request() { _requested.store(true); }

					/// <summary>
					/// Installs a Ctrl+C handler that records the interrupt instead of
					/// terminating the process, and returns the object the run should read.
					/// </summary>
					/// <remarks>
					/// A POSIX signal handler carries no user data, unlike
					/// <c>Console.CancelKeyPress</c>, so only one installed
					/// <see cref="Interruption"/> can be live in a process at a time ---
					/// exactly the real use this method exists for, one run per process.
					/// Nothing in the test suite calls this: every test constructs an
					/// <see cref="Interruption"/> directly and calls <see cref="request"/>
					/// on it, which needs no signal at all.
					/// </remarks>
					static Interruption& install();

				private:
					std::atomic<bool> _requested{ false };
				};

				/// <summary>
				/// The run was interrupted by the user. Not a failure of any document, so
				/// it names none --- the same shape as a run-level limit.
				/// </summary>
				class InterruptedError : public CliError
				{
				public:
					InterruptedError() : CliError(ErrorCodes::interrupted, "interrupted") {}

					int exitCode() const override;
				};
			}
		}
	}
}

#endif
