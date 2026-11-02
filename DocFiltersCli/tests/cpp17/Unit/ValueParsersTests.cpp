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

// ---------------------------------------------------------------------------
//  The conversions that can fail. These live in shared code precisely so that
//  a bad value produces the same message and the same exit code in the C#,
//  Python, Java and C++17 builds, rather than four different parsing
//  libraries' wording.
// ---------------------------------------------------------------------------

#include "Configuration/ValueParsers.h"
#include "Runtime/CliError.h"

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <stdexcept>

using namespace Hyland::DocumentFilters::Cli::Configuration;
using namespace Hyland::DocumentFilters::Cli::Runtime;

namespace
{
	UsageError expectUsageError(const std::function<void()>& action)
	{
		try
		{
			action();
		}
		catch (const UsageError& error)
		{
			return error;
		}
		throw std::logic_error("expected a UsageError");
	}
}

TEST_CASE("counts parse", "[ValueParsers]")
{
	CHECK(ValueParsers::parseCount("--max-depth", "0") == 0);
	CHECK(ValueParsers::parseCount("--max-depth", "1") == 1);
	CHECK(ValueParsers::parseCount("--max-depth", "42") == 42);
}

TEST_CASE("a bad count is a usage error", "[ValueParsers]")
{
	for (const std::string& text : { std::string("abc"), std::string(""), std::string("-1"), std::string("1.5"), std::string("1e3"),
										std::string(" 1") })
	{
		UsageError error = expectUsageError([&] { ValueParsers::parseCount("--max-depth", text); });
		CHECK(error.code() == ErrorCodes::badValue);
	}
}

TEST_CASE("the error names the argument that was wrong", "[ValueParsers]")
{
	// Without the argument name, a batch of options makes the message useless.
	UsageError error = expectUsageError([] { ValueParsers::parseCount("--max-depth", "abc"); });
	CHECK(std::string(error.what()).find("--max-depth") != std::string::npos);
}

// -------------------------------------------------------------------- seconds

TEST_CASE("durations parse", "[ValueParsers]")
{
	CHECK(ValueParsers::parseSeconds("--timeout", "1") == 1);
	CHECK(ValueParsers::parseSeconds("--timeout", "60") == 60);
	CHECK(ValueParsers::parseSeconds("--timeout", "86400") == 86400);
}

TEST_CASE("a bad duration is a usage error", "[ValueParsers]")
{
	// Zero is rejected as well as negative: a timeout of nothing is a mistake,
	// not a request to finish instantly.
	for (const std::string& text :
		{ std::string("0"), std::string("-1"), std::string("abc"), std::string(""), std::string("1.5"), std::string("30s") })
	{
		UsageError error = expectUsageError([&] { ValueParsers::parseSeconds("--timeout", text); });
		CHECK(error.code() == ErrorCodes::badValue);
	}
}

// ----------------------------------------------------------------- megabytes

TEST_CASE("memory limits parse", "[ValueParsers]")
{
	CHECK(ValueParsers::parseMegabytes("--max-memory", "1") == 1);
	CHECK(ValueParsers::parseMegabytes("--max-memory", "512") == 512);
	CHECK(ValueParsers::parseMegabytes("--max-memory", "8192") == 8192);
}

TEST_CASE("a memory limit beyond int range still parses", "[ValueParsers]")
{
	// The value is megabytes and the field is a 64-bit integer, so a limit
	// larger than INT_MAX is legitimate rather than an overflow.
	CHECK(ValueParsers::parseMegabytes("--max-memory", "4194304") == 4194304LL);
}

TEST_CASE("a bad memory limit is a usage error", "[ValueParsers]")
{
	for (const std::string& text :
		{ std::string("0"), std::string("-1"), std::string("abc"), std::string(""), std::string("512MB"), std::string("1.5") })
	{
		UsageError error = expectUsageError([&] { ValueParsers::parseMegabytes("--max-memory", text); });
		CHECK(error.code() == ErrorCodes::badValue);
	}
}

TEST_CASE("both limit parsers name the argument that was wrong", "[ValueParsers]")
{
	CHECK(std::string(expectUsageError([] { ValueParsers::parseSeconds("--timeout", "x"); }).what()).find("--timeout")
		  != std::string::npos);

	CHECK(std::string(expectUsageError([] { ValueParsers::parseMegabytes("--max-memory", "x"); }).what()).find("--max-memory")
		  != std::string::npos);
}

// ------------------------------------------------------------------- id lists

TEST_CASE("id lists split on semicolons", "[ValueParsers]")
{
	CHECK(ValueParsers::parseIdList("PDF;DOCX") == std::vector<std::string>{ "PDF", "DOCX" });
}

TEST_CASE("id lists trim whitespace and drop empty entries", "[ValueParsers]")
{
	CHECK(ValueParsers::parseIdList(" PDF ; ; DOCX ;") == std::vector<std::string>{ "PDF", "DOCX" });
}

TEST_CASE("a single id needs no separator", "[ValueParsers]")
{
	CHECK(ValueParsers::parseIdList("PDF") == std::vector<std::string>{ "PDF" });
}

// ------------------------------------------------------------------ redactions

TEST_CASE("a redaction parses as page then rectangle", "[ValueParsers]")
{
	RedactSpec spec = ValueParsers::parseRedaction("2,10,20,100,50");

	CHECK(spec.page == 2);
	CHECK(spec.x == 10);
	CHECK(spec.y == 20);
	CHECK(spec.width == 100);
	CHECK(spec.height == 50);
}

TEST_CASE("a malformed redaction is a usage error", "[ValueParsers]")
{
	for (const std::string& text :
		{ std::string("2,10,20,100"), std::string("2,10,20,100,50,60"), std::string("2,10,20,100,abc"), std::string("") })
	{
		CHECK_THROWS_AS(ValueParsers::parseRedaction(text), UsageError);
	}
}

// ----------------------------------------------------------------- form values

TEST_CASE("a form value parses as page then name equals value", "[ValueParsers]")
{
	FormValue value = ValueParsers::parseFormValue("1,surname=Smith");

	CHECK(value.page == 1);
	CHECK(value.name == "surname");
	CHECK(value.value == "Smith");
}

TEST_CASE("only the first equals separates name from value", "[ValueParsers]")
{
	// Form values legitimately contain '=', for instance a base64 payload.
	FormValue value = ValueParsers::parseFormValue("1,token=abc=def=");

	CHECK(value.name == "token");
	CHECK(value.value == "abc=def=");
}

TEST_CASE("an empty form value is allowed", "[ValueParsers]")
{
	// Clearing a field is a legitimate operation.
	FormValue value = ValueParsers::parseFormValue("1,surname=");

	CHECK(value.value == "");
}

TEST_CASE("a malformed form value is a usage error", "[ValueParsers]")
{
	for (const std::string& text : { std::string("1,surname"), std::string("surname=Smith"), std::string("") })
		CHECK_THROWS_AS(ValueParsers::parseFormValue(text), UsageError);
}

// --------------------------------------------------------------------- on-error

TEST_CASE("on-error accepts exactly the two documented values", "[ValueParsers]")
{
	CHECK(ValueParsers::parseOnError("skip") == OnErrorBehaviour::Skip);
	CHECK(ValueParsers::parseOnError("stop") == OnErrorBehaviour::Stop);
}

TEST_CASE("anything else for on-error is a usage error", "[ValueParsers]")
{
	// Case sensitive on purpose: if one implementation accepted "SKIP" and
	// another did not, the four would diverge on what is a valid command
	// line.
	for (const std::string& text : { std::string("SKIP"), std::string("Skip"), std::string("continue"), std::string("") })
		CHECK_THROWS_AS(ValueParsers::parseOnError(text), UsageError);
}
