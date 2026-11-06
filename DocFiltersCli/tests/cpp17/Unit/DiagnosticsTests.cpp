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

// The per-document timing window, and --timeout on top of it. Pinned in the
// order the commands actually call these: beginDocument, work, endDocument,
// then checkDocumentTimeout. That order is the whole point --- endDocument
// used to discard the timer, so the check that ran immediately after always
// saw nothing and --timeout never fired for any document, however long it
// took.

#include "Execution/Diagnostics.h"
#include "Fakes/RecordingConsole.h"
#include "Runtime/CliError.h"
#include "Runtime/ExitCode.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <optional>
#include <thread>

using namespace Hyland::DocumentFilters::Cli;
using Hyland::DocumentFilters::Cli::Tests::Fakes::RecordingConsole;

namespace
{
	Configuration::DiagnosticOptions timeoutOptions(std::optional<int> seconds)
	{
		Configuration::DiagnosticOptions options;
		options.timeout = seconds;
		return options;
	}
}

namespace
{
	/// <summary>A clock a test moves by hand, so a deadline can pass instantly.</summary>
	/// <remarks>
	/// Every case below drives this rather than sleeping. Asserting that a deadline
	/// passed used to mean waiting for it, which made the assertions both slow and
	/// approximate --- a loaded machine could stretch a five-millisecond sleep past
	/// any limit, and a fast one could leave it short.
	/// </remarks>
	class TestClock
	{
	public:
		std::chrono::steady_clock::time_point now() const { return _now; }

		void advance(std::chrono::steady_clock::duration by) { _now += by; }

	private:
		std::chrono::steady_clock::time_point _now {};
	};
}

TEST_CASE("a document that runs past the timeout is reported after it ends", "[Diagnostics]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options = timeoutOptions(30);
	TestClock clock;
	Execution::Diagnostics diagnostics(options, console, nullptr, [&clock]() { return clock.now(); });

	diagnostics.beginDocument("slow.pdf", 1, 1);
	clock.advance(std::chrono::seconds(31));
	diagnostics.endDocument("slow.pdf");

	try
	{
		diagnostics.checkDocumentTimeout("slow.pdf");
		FAIL("expected a LimitExceededError");
	}
	catch (const Runtime::LimitExceededError& error)
	{
		CHECK(error.code() == Runtime::ErrorCodes::limitExceeded);
		CHECK(error.exitCode() == Runtime::ExitCode::limitExceeded);
		CHECK(error.path() == std::optional<std::string>("slow.pdf"));
		CHECK(std::string(error.what()) == "--timeout of 30s exceeded");
	}
}

TEST_CASE("a document inside the timeout passes", "[Diagnostics]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options = timeoutOptions(600);
	Execution::Diagnostics diagnostics(options, console);

	diagnostics.beginDocument("quick.pdf", 1, 1);
	diagnostics.endDocument("quick.pdf");

	CHECK_NOTHROW(diagnostics.checkDocumentTimeout("quick.pdf"));
}

TEST_CASE("without the option no amount of time is a failure", "[Diagnostics]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options = timeoutOptions(std::nullopt);
	TestClock clock;
	Execution::Diagnostics diagnostics(options, console, nullptr, [&clock]() { return clock.now(); });

	diagnostics.beginDocument("slow.pdf", 1, 1);
	clock.advance(std::chrono::hours(1));
	diagnostics.endDocument("slow.pdf");

	CHECK_NOTHROW(diagnostics.checkDocumentTimeout("slow.pdf"));
}

TEST_CASE("the check is harmless before any document has begun", "[Diagnostics]")
{
	RecordingConsole console;
	Configuration::DiagnosticOptions options = timeoutOptions(0);
	Execution::Diagnostics diagnostics(options, console);

	CHECK_NOTHROW(diagnostics.checkDocumentTimeout("nothing.pdf"));
}

TEST_CASE("each document is timed from its own start", "[Diagnostics]")
{
	// The timer outlives endDocument now, so the guard against it carrying
	// over is beginDocument replacing it --- not endDocument clearing it.
	RecordingConsole console;
	Configuration::DiagnosticOptions options = timeoutOptions(600);
	TestClock clock;
	Execution::Diagnostics diagnostics(options, console, nullptr, [&clock]() { return clock.now(); });

	diagnostics.beginDocument("first.pdf", 1, 2);
	clock.advance(std::chrono::seconds(1));
	diagnostics.endDocument("first.pdf");
	CHECK_NOTHROW(diagnostics.checkDocumentTimeout("first.pdf"));

	diagnostics.beginDocument("second.pdf", 2, 2);
	diagnostics.endDocument("second.pdf");
	CHECK_NOTHROW(diagnostics.checkDocumentTimeout("second.pdf"));
}

TEST_CASE("log time still reports the documents own elapsed time", "[Diagnostics]")
{
	// endDocument does two things; keeping the timer must not cost the other.
	RecordingConsole console;
	Configuration::DiagnosticOptions options;
	options.logTime = true;
	Execution::Diagnostics diagnostics(options, console);

	diagnostics.beginDocument("a.pdf", 1, 1);
	diagnostics.endDocument("a.pdf");

	bool found = false;
	for (const std::string& line : console.outLines)
	{
		if (line.rfind("a.pdf: ", 0) == 0 && !line.empty() && line.back() == 's')
			found = true;
	}
	CHECK(found);
}
