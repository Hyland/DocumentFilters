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

#ifndef INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_CONFORMANCE_FIXTURE_H
#define INC_HYLAND_DOCUMENTFILTERSCLI_TESTS_CONFORMANCE_FIXTURE_H

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
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
					/// The fixture text does not match the format
					/// tests/conformance/README.md describes: an unrecognised line prefix, a
					/// '&lt;' line out of order, an expectation with no preceding '$' case, or
					/// a yes/no value that is neither. Matches C#'s <c>InvalidDataException</c>
					/// from the same call sites.
					/// </summary>
					class FixtureFormatError : public std::runtime_error
					{
					public:
						explicit FixtureFormatError(const std::string& message) : std::runtime_error(message) {}
					};

					/// <summary>
					/// No such fixture file under tests/conformance/. Matches C#'s
					/// <c>FileNotFoundException</c> from <see cref="Fixture::load"/>.
					/// </summary>
					class FixtureNotFoundError : public std::runtime_error
					{
					public:
						explicit FixtureNotFoundError(const std::string& message) : std::runtime_error(message) {}
					};

					/// <summary>
					/// One case from a shared fixture: a subject, the world it runs in, and
					/// its expectations. Ported from tests/csharp/Conformance/Fixture.cs,
					/// which every accessor here matches one for one.
					/// </summary>
					class FixtureCase
					{
					public:
						/// <summary>The text after '$' --- a command line, a format token, a template.</summary>
						std::string subject;

						/// <summary>
						/// The '&lt;' lines: what the case is given, in fixture order. Keys may
						/// repeat.
						/// </summary>
						/// <remarks>
						/// Held apart from <see cref="expectations"/> rather than distinguished
						/// by key name, because they are the opposite kind of thing: '&lt;'
						/// describes the world before the run and '=' describes what the run
						/// must produce. A reader that mixed them would make every consumer
						/// filter by a list of key names it had to keep in step.
						/// </remarks>
						std::vector<std::pair<std::string, std::string>> given;

						/// <summary>The '=' lines, in fixture order. Keys may repeat.</summary>
						std::vector<std::pair<std::string, std::string>> expectations;

						/// <summary>Fixture filename, for failure messages.</summary>
						std::string file;

						/// <summary>1-based line of the '$' line.</summary>
						int line = 0;

						/// <summary>The expected error code, when the case expects a failure.</summary>
						std::optional<std::string> error() const { return get("error"); }

						/// <summary>
						/// The single value for a key, trimmed, or nullopt when the key is
						/// absent.
						/// </summary>
						/// <remarks>
						/// Values are stored verbatim and trimmed here, because fixtures pad
						/// them into a readable column and almost no case cares about that
						/// padding. The exceptions use <see cref="getRaw"/>, for a value whose
						/// own leading whitespace is part of what it asserts.
						/// </remarks>
						std::optional<std::string> get(const std::string& key) const;

						/// <summary>The single value for a key exactly as written, padding included.</summary>
						std::optional<std::string> getRaw(const std::string& key) const;

						/// <summary>Every value for a repeated key, trimmed, in fixture order.</summary>
						std::vector<std::string> getAll(const std::string& key) const;

						/// <summary>
						/// Every value for a repeated key exactly as written, with one
						/// separating space removed and everything beyond it kept.
						/// </summary>
						/// <remarks>
						/// This is what a golden-output case would use. <c>= json    "a": 1</c>
						/// yields <c>   "a": 1</c> --- the first space after the key separates,
						/// the rest is the value's own indentation.
						/// </remarks>
						std::vector<std::string> getAllRaw(const std::string& key) const;

						/// <summary>True when the fixture states an expectation for this key.</summary>
						bool has(const std::string& key) const;

						/// <summary>Every value for a repeated '&lt;' key, trimmed, in fixture order.</summary>
						std::vector<std::string> givenAll(const std::string& key) const;

						/// <summary>True when the case is given anything under this key.</summary>
						bool hasGiven(const std::string& key) const;

						/// <summary>A yes/no expectation. Accepts only "yes" and "no", so the fixtures stay uniform.</summary>
						/// <exception cref="FixtureFormatError">The key is absent, or its value is neither "yes" nor "no".</exception>
						bool getBool(const std::string& key) const;

						/// <summary>
						/// A failure message that names the fixture and line, states the
						/// problem, and reproduces the whole case, so a failing run does not
						/// send the reader off to open another file.
						/// </summary>
						std::string describe(const std::string& problem) const;
					};

					/// <summary>
					/// Reader for the shared conformance fixtures under tests/conformance/.
					/// </summary>
					/// <remarks>
					/// Every fixture uses one format, so each language needs exactly one
					/// reader rather than one per file. A case is a '$' subject line, then any
					/// '&lt;' lines saying what the case is given, then '=' lines saying what
					/// it must produce. Blank lines separate cases and '#' starts a comment.
					///
					/// Deliberately free of JSON and YAML so this suite and the Java one can
					/// parse it with nothing but their standard library.
					/// </remarks>
					namespace Fixture
					{
						/// <summary>Loads a fixture by filename, e.g. "formats.txt".</summary>
						/// <exception cref="FixtureNotFoundError">No such fixture file.</exception>
						/// <exception cref="FixtureFormatError">The fixture text is malformed.</exception>
						std::vector<FixtureCase> load(const std::string& name);

						/// <summary>Parses fixture text. Exposed so the reader itself can be tested.</summary>
						/// <exception cref="FixtureFormatError">The fixture text is malformed.</exception>
						std::vector<FixtureCase> parse(const std::string& file, const std::vector<std::string>& lines);
					}
				}
			}
		}
	}
}

#endif
