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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_COMMANDEXECUTION_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_COMMANDS_COMMANDEXECUTION_H

#include "Execution/Diagnostics.h"
#include "Execution/DocumentFiltersSession.h"
#include "ICommand.h"
#include "Execution/StopSignal.h"
#include "Runtime/Interruption.h"

#include <functional>
#include <optional>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Commands
			{
				/// <summary>
				/// The one piece every <c>execute()</c> needs and none of them owns:
				/// <see cref="Execution::DocumentFiltersSession"/>'s lifetime.
				/// </summary>
				/// <remarks>
				/// A CLI process runs exactly one command, so creating the session at
				/// the start of that one <c>execute()</c> call and disposing it at the
				/// end already matches its documented lifetime --- "once for the
				/// lifetime of the process" --- without <c>CommandLineApp</c> needing to
				/// know about it. A context wired with a session already --- as tests
				/// are free to do --- is left alone and not disposed here, since this
				/// did not create it.
				/// </remarks>
				namespace CommandExecution
				{
					/// <summary>
					/// A fresh <c>StopSignal</c> for one document.
					/// </summary>
					/// <remarks>
					/// Per document, not per run: the signal latches the first reason it sees,
					/// and a deadline belongs to the document it was measured against.
					/// <paramref name="interruption"/> is null for a caller that installed no
					/// handler, which is every test that does not ask for one.
					/// </remarks>
					inline Execution::StopSignal watchForStop(Execution::Diagnostics& diagnostics, Runtime::Interruption* interruption)
					{
						return Execution::StopSignal(diagnostics, interruption);
					}

					/// <summary>
					/// Runs <paramref name="action"/> with an initialised session in
					/// <paramref name="context"/>, creating and disposing one if the
					/// caller did not already supply one.
					/// </summary>
					inline int withSession(CommandContext& context, const std::function<int()>& action)
					{
						bool ownsSession = context.session == nullptr;
						std::optional<Execution::DocumentFiltersSession> owned;
						if (ownsSession)
						{
							owned.emplace(Execution::DocumentFiltersSession::create(context.sharedOptions.library, context.sharedOptions.processing.tempDir));
							context.session = &*owned;
						}

						// Mirrors the C# reference's try/finally: on the way out --- return
						// or exception alike --- the pointer is cleared before the session
						// it names is disposed, so context.session never dangles once this
						// returns. Declared after `owned`, so it destructs first (guards
						// destruct in reverse declaration order): the pointer is nulled,
						// then the object it pointed to is destroyed.
						struct ClearSessionOnExit
						{
							CommandContext& context;
							bool active;
							~ClearSessionOnExit()
							{
								if (active)
									context.session = nullptr;
							}
						} clearSession { context, ownsSession };

						return action();
					}
				}
			}
		}
	}
}

#endif
