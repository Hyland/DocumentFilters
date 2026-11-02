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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_CONFORMANCE_PLANCORPUS_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_CONFORMANCE_PLANCORPUS_H

#include "Conformance/Fixture.h"
#include "Fakes/InMemoryFileSystem.h"

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
				namespace Conformance
				{
					/// <summary>
					/// The plan corpus: <see cref="Fixture"/> plus the one thing specific to
					/// it, splitting a subject line into command line arguments.
					/// </summary>
					namespace PlanCorpus
					{
						/// <summary>Every case in the corpus.</summary>
						std::vector<FixtureCase> load();

						/// <summary>The expected exit code for a case, defaulting to success.</summary>
						int expectedExit(const FixtureCase& testCase);

						/// <summary>
						/// Builds the filesystem a case is given by its <c>&lt;</c> lines. Never
						/// the real one --- see <c>tests/conformance/README.md</c> for why.
						/// </summary>
						Fakes::InMemoryFileSystem buildFileSystem(const FixtureCase& testCase);

						/// <summary>
						/// Splits a corpus subject into arguments on whitespace, honouring
						/// double quotes so a value may contain spaces.
						/// </summary>
						std::vector<std::string> splitArguments(const std::string& commandLine);
					}
				}
			}
		}
	}
}

#endif
