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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_CLI_CLIHARNESS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_CLI_CLIHARNESS_H

#include "Engine/EngineException.h"
#include "Fakes/FakeDocumentFiltersEngine.h"
#include "Fakes/InMemoryFileSystem.h"
#include "Fakes/RecordingConsole.h"
#include "Runtime/Interruption.h"

#include <chrono>
#include <string>
#include <vector>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Tests
			{
				namespace Cli
				{
					/// <summary>
					/// Runs a whole command line the way the shell does, in process.
					/// </summary>
					/// <remarks>
					/// <c>argv</c> in; exit code, stdout and stderr out. Everything
					/// between is the real thing --- the real parser, the real
					/// validator, the real walk, the real planner, the real commands,
					/// the real error-to-exit-code mapping --- with only the three edges
					/// swapped: the console, the filesystem and the engine.
					///
					/// This covers the one seam the conformance corpus cannot reach. The
					/// corpus stops at <c>bind</c> and <c>describePlan</c>; everything
					/// above that --- <c>Program::run</c>, <c>CommandLineApp::run</c>'s
					/// dispatch, <c>--print-contract</c>, the dry-run branch, the
					/// session's lifetime, and <c>reportError</c>'s stderr format --- had
					/// no test at all before this.
					///
					/// No license and no native library: the engine is a fake, so these
					/// run wherever the unit tests run. What they deliberately do not
					/// prove is that the real engine behaves as the fake claims --- that
					/// is tier 3's job.
					/// </remarks>
					class CliHarness
					{
					public:
						/// <summary>The default: nothing is watching for an interrupt.</summary>
						CliHarness() : _interruption(nullptr) {}

						/// <summary>Builds a harness whose runs watch <paramref name="interruption"/>.</summary>
						explicit CliHarness(Runtime::Interruption& interruption) : _interruption(&interruption) {}

						/// <summary>The fake disk, so a test can assert what the run left on it.</summary>
						Fakes::InMemoryFileSystem fileSystem;

						/// <summary>The engine the run sees. Script documents and failures on it before running.</summary>
						Fakes::FakeDocumentFiltersEngine engine;

						/// <summary>A monotonic reading the test advances itself.</summary>
						/// <remarks>
						/// Starts at the epoch and only moves when a test says so, so a run takes
						/// no real time however long it claims to have taken. Every harness run
						/// uses one, which also makes timings printed by <c>--time</c>
						/// reproducible rather than whatever the machine happened to do.
						/// </remarks>
						class TestClock
						{
						public:
							std::chrono::steady_clock::time_point now() const { return _now; }

							void advance(std::chrono::steady_clock::duration by) { _now += by; }

						private:
							std::chrono::steady_clock::time_point _now {};
						};

						TestClock clock;

						/// <summary>
						/// What <c>--view</c> was handed, and what was in it at that moment.
						/// </summary>
						/// <remarks>
						/// The content is read when the viewer is called rather than afterwards,
						/// because the question <c>--view</c> raises is one of timing: a combined
						/// output is not a finished file until its canvas or its stream is closed,
						/// and reading it after the run would look the same either way.
						/// </remarks>
						std::vector<std::string> viewed;

						std::vector<std::string> viewedContent;

						/// <summary>Everything the run wrote, kept after <see cref="run"/> returns.</summary>
						Fakes::RecordingConsole console;

						int exitCode = 0;

						const std::vector<std::string>& out() const { return console.outLines; }

						/// <summary>
						/// Everything written to stdout, verbatim.
						/// </summary>
						/// <remarks>
						/// <see cref="out"/> only records whole lines, so it misses output
						/// written without one --- the JSON reports build a document and
						/// write it in a single call, which is exactly the output whose
						/// exact shape is worth asserting.
						/// </remarks>
						std::string output() const { return console.output(); }

						const std::vector<std::string>& errors() const { return console.errorLines; }

						/// <summary>Adds an input that exists on disk and opens in the engine.</summary>
						/// <remarks>
						/// Both halves, always. A path in only one of the two is the setup
						/// mistake that otherwise shows up as a walk selecting nothing, or
						/// an open failing for a file the walk did find --- neither of
						/// which is what the test meant to say.
						/// </remarks>
						CliHarness& withDocument(const std::string& path, const std::string& text = "")
						{
							fileSystem.addFile(path);
							engine.addDocument(path, text);
							return *this;
						}

						/// <summary>Adds a file on disk that the engine will refuse to open.</summary>
						CliHarness& withUnreadableDocument(const std::string& path, Engine::EngineError error)
						{
							fileSystem.addFile(path);
							engine.failToOpen(path, error);
							return *this;
						}

						/// <summary>Adds a file the engine opens only with the right password.</summary>
						CliHarness& withProtectedDocument(const std::string& path, const std::string& password, const std::string& text = "")
						{
							fileSystem.addFile(path);
							engine.addProtectedDocument(path, password, text);
							return *this;
						}

						/// <summary>Adds a file on disk that the engine has never heard of.</summary>
						/// <remarks>
						/// The walk finds it and the open fails --- which is what a file of
						/// a format this build cannot read looks like from the outside.
						/// </remarks>
						CliHarness& withUnopenableFile(const std::string& path)
						{
							fileSystem.addFile(path);
							return *this;
						}

						/// <summary>Puts a file where an output would go, for the overwrite rules.</summary>
						CliHarness& withExistingOutput(const std::string& path)
						{
							fileSystem.addFile(path, "already here");
							return *this;
						}

						/// <summary>Runs the command line and returns the exit code.</summary>
						int run(const std::vector<std::string>& args);

						/// <summary>
						/// Convenience overload for a literal argument list. Fixed to a
						/// <c>const char*</c> first parameter, rather than a fully generic
						/// pack, so it never competes with <see cref="run"/>'s vector
						/// overload for a call that already has one built --- a vector
						/// does not convert to <c>const char*</c>, so this overload is
						/// simply not viable there, rather than being selected and then
						/// failing to compile its body.
						/// </summary>
						template <typename... Rest>
						int run(const char* first, Rest&&... rest)
						{
							return run(std::vector<std::string>{ std::string(first), std::string(std::forward<Rest>(rest))... });
						}

						/// <summary>The files the run created, for asserting on what it wrote.</summary>
						const std::vector<std::string>& created() const { return fileSystem.created; }

					private:
						Runtime::Interruption* _interruption;
					};
				}
			}
		}
	}
}

#endif
