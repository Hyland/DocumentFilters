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

// The error vocabulary, checked against the fixtures that already state it.
// There is no separate fixture listing the codes: every failing case in
// plan-corpus.txt already states its code, its message and its exit code,
// and a second file repeating them would be a second place to keep in step.
//
// ErrorCodes::all() is a hand-written list beside the constants rather than
// reflection, because C++17 has none and these tests are translated into the
// other three languages rather than rewritten there.

#include "Conformance/PlanCorpus.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <set>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Conformance::FixtureCase;

namespace
{
	/// <summary>
	/// The codes no planning fixture can provoke.
	/// </summary>
	/// <remarks>
	/// plan-corpus.txt resolves argv into a plan and opens nothing, so none
	/// of these can appear there: three need a live engine --- the library
	/// has to fail to initialise, a document has to fail to open, or one has
	/// to outrun --timeout --- the fourth needs a person pressing Ctrl+C, and
	/// the fifth needs an open container, because what a subfile is named is
	/// not knowable until one is. They are exercised all the same, by
	/// SubfileExtractionTests and CliTests against the fake engine and by
	/// tests/e2e/end2end.py against the real one, so what is listed here is
	/// the fact that they have a home, not an exemption from having one.
	///
	/// Written out rather than derived, so that adding a code and forgetting
	/// to provoke it still fails: this list is the only way to opt out, and
	/// it is short enough to argue with.
	/// </remarks>
	std::vector<std::string> codesNoPlanCanProvoke()
	{
		return {
			Runtime::ErrorCodes::initFailed,
			Runtime::ErrorCodes::documentFailed,
			Runtime::ErrorCodes::limitExceeded,
			Runtime::ErrorCodes::interrupted,
			Runtime::ErrorCodes::unsafeOutput,
		};
	}

	/// <summary>Every code named by a failing case in the corpus, plus those no case can reach.</summary>
	std::set<std::string> codesInFixtures()
	{
		std::set<std::string> codes;
		for (const std::string& code : codesNoPlanCanProvoke())
			codes.insert(code);

		for (const FixtureCase& testCase : Tests::Conformance::PlanCorpus::load())
		{
			if (testCase.error().has_value())
				codes.insert(*testCase.error());
		}

		return codes;
	}
}

TEST_CASE("every code the tool declares is exercised by a fixture", "[ErrorCode]")
{
	// The direction that matters: a code nobody provokes has no message,
	// which contradicts stderr being contract.
	std::set<std::string> exercised = codesInFixtures();

	for (const std::string& code : Runtime::ErrorCodes::all())
	{
		INFO(code << " is declared in ErrorCodes but no fixture case provokes it, so its message is untested");
		CHECK(exercised.count(code) > 0);
	}
}

TEST_CASE("every code a fixture names is one the tool declares", "[ErrorCode]")
{
	// Catches a typo in a fixture, which would otherwise look like an
	// implementation bug in whichever language ran it next.
	const std::vector<std::string>& all = Runtime::ErrorCodes::all();
	std::set<std::string> declared(all.begin(), all.end());

	for (const std::string& code : codesInFixtures())
	{
		INFO("a fixture names " << code << ", which ErrorCodes does not declare");
		CHECK(declared.count(code) > 0);
	}
}

TEST_CASE("one code always means one exit code", "[ErrorCode]")
{
	// E_MUTUALLY_EXCLUSIVE appears in eight cases. Nothing else checks they
	// agree, so one drifting to a different exit code would reach three
	// more implementations before anyone noticed.
	std::map<std::string, int> seen;

	for (const FixtureCase& testCase : Tests::Conformance::PlanCorpus::load())
	{
		if (!testCase.error().has_value())
			continue;

		int exit = Tests::Conformance::PlanCorpus::expectedExit(testCase);

		auto found = seen.find(*testCase.error());
		if (found != seen.end())
		{
			INFO(testCase.describe(*testCase.error() + " exits " + std::to_string(exit) + " here but " + std::to_string(found->second) + " elsewhere"));
			CHECK(found->second == exit);
		}
		else
			seen[*testCase.error()] = exit;
	}
}

TEST_CASE("every exit code a fixture states is one the table defines", "[ErrorCode]")
{
	std::set<int> valid = {
		Runtime::ExitCode::success,
		Runtime::ExitCode::partialFailure,
		Runtime::ExitCode::usageError,
		Runtime::ExitCode::noInputsMatched,
		Runtime::ExitCode::outputConflict,
		Runtime::ExitCode::initialisationFailure,
		Runtime::ExitCode::limitExceeded,
		Runtime::ExitCode::interrupted,
	};

	for (const FixtureCase& testCase : Tests::Conformance::PlanCorpus::load())
	{
		INFO(testCase.describe("states an exit code that is not in the table"));
		CHECK(valid.count(Tests::Conformance::PlanCorpus::expectedExit(testCase)) > 0);
	}
}
