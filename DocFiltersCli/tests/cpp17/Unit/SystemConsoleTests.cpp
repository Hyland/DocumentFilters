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

// --color, asserted against the real console rather than a fake.
// RecordingConsole deliberately knows nothing about colour: its errorLines
// are the cross-language contract and are asserted character for character,
// so letting escape sequences into them would mean every one of those
// assertions had to strip ANSI first --- and a wrong sequence would then
// pass. Colour is presentation, the fake captures content. That leaves
// SystemConsole as the only thing that can be asserted on here, which is
// the point: a flag tested only through "the option got set" is what let
// nine others sit unimplemented.
//
// These tests redirect std::cout/std::cerr's streambuf, process-wide state.
// Catch2 runs test cases one at a time within this binary, and nothing else
// in this suite writes to the real console, so the redirection cannot be
// observed by another test.

#include "Configuration/SharedOptions.h"
#include "Runtime/SystemConsole.h"

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <iostream>
#include <sstream>

using namespace Hyland::DocumentFilters::Cli;

namespace
{
	const std::string escape(1, static_cast<char>(27));

	Configuration::ConsoleOptions options(bool verbose = false)
	{
		Configuration::ConsoleOptions result;
		result.color = true;
		result.verbose = verbose;
		return result;
	}

	std::string trimEnd(std::string text)
	{
		while (!text.empty() && (text.back() == '\r' || text.back() == '\n'))
			text.pop_back();
		return text;
	}

	/// <summary>Runs <paramref name="write"/> and returns what landed on stderr.</summary>
	std::string captureError(Runtime::SystemConsole& console, const std::function<void(Runtime::SystemConsole&)>& write)
	{
		std::ostringstream captured;
		std::streambuf* original = std::cerr.rdbuf(captured.rdbuf());
		try
		{
			write(console);
		}
		catch (...)
		{
			std::cerr.rdbuf(original);
			throw;
		}
		std::cerr.rdbuf(original);
		return captured.str();
	}

	/// <summary>Runs <paramref name="write"/> and returns what landed on stdout.</summary>
	std::string captureOut(Runtime::SystemConsole& console, const std::function<void(Runtime::SystemConsole&)>& write)
	{
		std::ostringstream captured;
		std::streambuf* original = std::cout.rdbuf(captured.rdbuf());
		try
		{
			write(console);
		}
		catch (...)
		{
			std::cout.rdbuf(original);
			throw;
		}
		std::cout.rdbuf(original);
		return captured.str();
	}
}

// ------------------------------------------------------------ the rule

TEST_CASE("colour is used only when asked for and the stream can show it", "[SystemConsole]")
{
	struct Case
	{
		bool requested;
		bool redirected;
		bool expected;
	};

	for (Case c : { Case{ true, false, true }, // asked for, terminal
			 Case{ true, true, false },        // asked for, but redirected
			 Case{ false, false, false },      // not asked for
			 Case{ false, true, false } })
	{
		CHECK(Runtime::SystemConsole::shouldColor(c.requested, c.redirected) == c.expected);
	}
}

// ------------------------------------------------------- what is written

TEST_CASE("an error is wrapped in red", "[SystemConsole]")
{
	Configuration::ConsoleOptions opts = options();
	Runtime::SystemConsole console(opts, true, true);

	std::string written = captureError(console, [](Runtime::SystemConsole& c) { c.writeError("E_DOCUMENT_FAILED: boom"); });

	CHECK(trimEnd(written) == escape + "[31m" + "E_DOCUMENT_FAILED: boom" + escape + "[0m");
}

TEST_CASE("an error carries no escapes when colour is off", "[SystemConsole]")
{
	Configuration::ConsoleOptions opts = options();
	Runtime::SystemConsole console(opts, false, false);

	std::string written = captureError(console, [](Runtime::SystemConsole& c) { c.writeError("E_DOCUMENT_FAILED: boom"); });

	// Asserted as an exact match rather than "contains no escape": a
	// substring search would find a match at position 0 for a control
	// character too, so it would pass here for the wrong reason.
	CHECK(trimEnd(written) == "E_DOCUMENT_FAILED: boom");
}

TEST_CASE("verbose chatter is dimmed", "[SystemConsole]")
{
	Configuration::ConsoleOptions opts = options(true);
	Runtime::SystemConsole console(opts, true, true);

	std::string written = captureOut(console, [](Runtime::SystemConsole& c) { c.writeVerbose("opening a.pdf"); });

	CHECK(trimEnd(written) == escape + "[2m" + "opening a.pdf" + escape + "[0m");
}

/// <summary>
/// The deliberate half of the decision: --color must not reach the answer
/// the user asked for, which is routinely piped into something that would
/// choke on escapes. A change that colours reports uniformly would pass
/// every test above and fail this one.
/// </summary>
TEST_CASE("report output is never coloured", "[SystemConsole]")
{
	Configuration::ConsoleOptions opts = options();
	Runtime::SystemConsole console(opts, true, true);

	std::string line = captureOut(console, [](Runtime::SystemConsole& c) { c.writeLine("a.pdf: format=51"); });
	std::string chunk = captureOut(console, [](Runtime::SystemConsole& c) { c.write("a.pdf: format=51"); });

	CHECK(trimEnd(line) == "a.pdf: format=51");
	CHECK(chunk == "a.pdf: format=51");
}

/// <summary>
/// --silent still wins over --color: an error is the one thing that
/// survives it, and it survives coloured.
/// </summary>
TEST_CASE("silent suppresses coloured report output but not coloured errors", "[SystemConsole]")
{
	Configuration::ConsoleOptions silent;
	silent.color = true;
	silent.silent = true;
	silent.verbose = true;
	Runtime::SystemConsole console(silent, true, true);

	CHECK(captureOut(console, [](Runtime::SystemConsole& c) { c.writeLine("a.pdf: format=51"); }).empty());
	CHECK(captureOut(console, [](Runtime::SystemConsole& c) { c.writeVerbose("opening a.pdf"); }).empty());

	std::string error = captureError(console, [](Runtime::SystemConsole& c) { c.writeError("E_DOCUMENT_FAILED: boom"); });
	CHECK(trimEnd(error) == escape + "[31m" + "E_DOCUMENT_FAILED: boom" + escape + "[0m");
}
