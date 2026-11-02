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

#pragma once

#include "Execution/Diagnostics.h"
#include "Runtime/CliError.h"
#include "Runtime/Interruption.h"

#include <optional>
#include <string>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				/// <summary>Why a document was abandoned.</summary>
				enum class StopReason
				{
					/// <summary>The user pressed Ctrl+C.</summary>
					interrupted,

					/// <summary><c>--timeout</c> elapsed for this document.</summary>
					timedOut
				};

				/// <summary>
				/// The predicate the engine polls during <c>Open</c>, and the reason it gave
				/// when it said to stop.
				/// </summary>
				/// <remarks>
				/// One object rather than a bare <c>std::function&lt;bool()&gt;</c> because
				/// the answer and the reason for it are the same fact, decided at the same
				/// instant. The engine can only be told <em>whether</em> to stop ---
				/// <c>IGR_CANCELLED</c> carries nothing else --- so a cause worked out
				/// afterwards from the same two conditions is a second guess at a question
				/// already answered: both can be true by then, either can have become true
				/// after the decision, and neither need still be true at all. That guess is
				/// what made an interrupt during <c>Open</c> report a <c>--timeout</c> that
				/// had never been set, and dereference the empty optional holding it.
				///
				/// Recorded on the first poll that returns true and never revised, so the run
				/// reports the condition that actually stopped it.
				/// </remarks>
				class StopSignal
				{
				public:
					StopSignal(Diagnostics& diagnostics, Runtime::Interruption* interruption)
						: _diagnostics(diagnostics), _interruption(interruption)
					{
					}

					/// <summary>The reason recorded by the poll that stopped the document, if one did.</summary>
					/// <remarks>
					/// Empty after a cancellation means the engine stopped for a reason of its
					/// own and nobody asked it to --- a real possibility, and one the old
					/// guesswork could not tell apart from a limit firing.
					/// </remarks>
					const std::optional<StopReason>& reason() const { return _reason; }

					/// <summary>
					/// What the engine polls. True once, then true forever.
					/// </summary>
					/// <remarks>
					/// The interrupt is tested first: if the user pressed Ctrl+C, that is what
					/// they are owed an answer about, whatever else may also have come true.
					/// Testing it here rather than later is the point --- only the poll knows
					/// which condition was the one that fired.
					/// </remarks>
					bool shouldStop()
					{
						if (_reason.has_value())
							return true;

						if (_interruption != nullptr && _interruption->requested())
							_reason = StopReason::interrupted;
						else if (_diagnostics.documentDeadlineExceeded())
							_reason = StopReason::timedOut;

						return _reason.has_value();
					}

					/// <summary>
					/// Throws the error for a cancelled open, given what this signal recorded.
					/// </summary>
					/// <remarks>
					/// Throws rather than returns: the errors are distinct types and a
					/// base-class return would slice them.
					/// </remarks>
					[[noreturn]] void raise(const std::string& displayName) const
					{
						if (_reason == StopReason::interrupted)
							throw Runtime::InterruptedError();

						if (_reason == StopReason::timedOut)
							throw _diagnostics.documentTimedOut(displayName);

						throw Runtime::DocumentError(displayName, "the engine cancelled the operation");
					}

				private:
					Diagnostics& _diagnostics;
					Runtime::Interruption* _interruption;
					std::optional<StopReason> _reason;
				};
			}
		}
	}
}
