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

#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"
#include "Runtime/Interruption.h"

#include <catch2/catch_test_macros.hpp>

#include <set>

using namespace Hyland::DocumentFilters::Cli::Runtime;

TEST_CASE("the agreed values are stable", "[ExitCode]")
{
	CHECK(ExitCode::success == 0);
	CHECK(ExitCode::partialFailure == 1);
	CHECK(ExitCode::usageError == 2);
	CHECK(ExitCode::noInputsMatched == 3);
	CHECK(ExitCode::outputConflict == 4);
	CHECK(ExitCode::initialisationFailure == 5);
	CHECK(ExitCode::limitExceeded == 6);
	CHECK(ExitCode::interrupted == 130);
}

TEST_CASE("a usage error exits two", "[ExitCode]")
{
	UsageError error(ErrorCodes::unknownArgument, "unknown option --nonsense");
	CHECK(error.exitCode() == ExitCode::usageError);
}

TEST_CASE("no inputs exits three", "[ExitCode]")
{
	CHECK(NoInputsError("nothing matched").exitCode() == ExitCode::noInputsMatched);
}

TEST_CASE("an output conflict exits four", "[ExitCode]")
{
	OutputConflictError error(ErrorCodes::outFileMultiInput, "several inputs, one --out-file");
	CHECK(error.exitCode() == ExitCode::outputConflict);
}

TEST_CASE("an initialisation failure exits five", "[ExitCode]")
{
	CHECK(InitialisationError("no license").exitCode() == ExitCode::initialisationFailure);
}

TEST_CASE("a limit being exceeded exits six", "[ExitCode]")
{
	CHECK(LimitExceededError("--max-time passed").exitCode() == ExitCode::limitExceeded);
}

TEST_CASE("an interrupted run exits 130", "[ExitCode]")
{
	CHECK(InterruptedError().exitCode() == ExitCode::interrupted);
}

TEST_CASE("a single document failure maps to partial failure", "[ExitCode]")
{
	DocumentError error("a.docx", "could not open");
	CHECK(error.exitCode() == ExitCode::partialFailure);
}

TEST_CASE("every error carries a machine readable code", "[ExitCode]")
{
	// The corpus matches on the code, not the prose, so the message can be
	// improved without rewriting every fixture.
	CHECK(UsageError(ErrorCodes::unknownArgument, "x").code() == ErrorCodes::unknownArgument);
	CHECK(NoInputsError("x").code() == ErrorCodes::noInputs);
	CHECK(DocumentError("a", "x").code() == ErrorCodes::documentFailed);
}

TEST_CASE("a document error remembers which file failed", "[ExitCode]")
{
	DocumentError error("corpus/a.docx", "could not open");
	REQUIRE(error.path().has_value());
	CHECK(*error.path() == "corpus/a.docx");
}

TEST_CASE("a run-level error names no path", "[ExitCode]")
{
	CHECK_FALSE(NoInputsError("nothing matched").path().has_value());
	CHECK_FALSE(LimitExceededError("--max-time passed").path().has_value());
}

TEST_CASE("every declared error code is distinct", "[ExitCode]")
{
	const std::vector<std::string>& codes = ErrorCodes::all();
	CHECK(codes.size() == 25);

	std::set<std::string> unique(codes.begin(), codes.end());
	CHECK(unique.size() == codes.size());
}
