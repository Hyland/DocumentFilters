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

#include "Runtime/SystemConsole.h"

#include "Runtime/Strings.h"

#include <cstdio>
#include <iostream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>

#include <conio.h>
#include <cwchar> // WEOF, which _getwch returns once the console is gone
#include <fcntl.h>
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <termios.h>
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Runtime
			{
				namespace
				{
					// Built from the code point rather than written as an escape
					// sequence, so no source file carries a literal control byte ---
					// those survive a copy-paste or a diff viewer badly.
					const std::string escape(1, static_cast<char>(27));
					const std::string red = escape + "[31m";
					const std::string dim = escape + "[2m";
					const std::string reset = escape + "[0m";

					bool streamRedirected(FILE* stream) { return ISATTY(FILENO(stream)) == 0; }

					/// <summary>What ends a line, written by this class rather than by the stream.</summary>
					/// <remarks>
					/// <c>Console.WriteLine</c> ends a line with <c>Environment.NewLine</c> and
					/// leaves the text it was given alone. A Windows text-mode stream instead
					/// translates <em>every</em> <c>\n</c> that passes through it, including
					/// one already inside the text --- and an engine string can hold one: an
					/// option description in the options catalogue is several lines of
					/// Markdown table, and the reference prints those breaks exactly as the
					/// engine gave them.
					///
					/// That difference is invisible to every other suite. The conformance
					/// fixtures collapse whitespace before comparing, each language's unit
					/// tests drive a recording console rather than this one, and the
					/// end-to-end checks assert properties rather than bytes. It took
					/// comparing the implementations against each other to see it: 41 bytes
					/// of drift across <c>docfilters options</c> alone, and every line of
					/// every JSON and <c>--dry-run</c> plan, which the reference emits as
					/// bare LF on every platform.
					///
					/// So translation is turned off (see useVerbatimNewlines) and this is
					/// written explicitly instead. The Python port does exactly the same
					/// thing for exactly the same reason.
					/// </remarks>
#if defined(_WIN32)
					const char* const lineTerminator = "\r\n";
#else
					const char* const lineTerminator = "\n";
#endif

					/// <summary>Stops the output streams rewriting line breaks inside the text they are given.</summary>
					/// <remarks>
					/// Another thing the reference gets for free, because .NET's console is a
					/// byte stream and never translated anything.
					///
					/// Both streams, because <c>Console.Error.WriteLine</c> follows the same
					/// rule as <c>Console.WriteLine</c>. stdin is deliberately left alone:
					/// <c>readAllInputLines</c> splits with <c>Strings::splitLines</c>, which
					/// already ends a line on CR, LF or CRLF the way
					/// <c>Console.In.ReadLine</c> does.
					///
					/// Called once, from the constructor, rather than around each write:
					/// <c>_setmode</c> is a property of the file descriptor, so setting it
					/// repeatedly would be the same call with the same answer.
					/// </remarks>
					void useVerbatimNewlines()
					{
#if defined(_WIN32)
						_setmode(FILENO(stdout), _O_BINARY);
						_setmode(FILENO(stderr), _O_BINARY);
#endif
					}

#if defined(_WIN32)
					/// <summary>The typed characters as UTF-8.</summary>
					/// <remarks>
					/// <c>WideCharToMultiByte</c> rather than anything written here: it is the
					/// platform's own conversion, it is what the rest of the world calls for
					/// this, and it gets surrogate pairs right without the caller tracking
					/// them. The standard C++ alternative, <c>wstring_convert</c>, was
					/// deprecated in C++17 and removed in C++26, and the binding's
					/// <c>w_to_u8</c> is not reachable from here --- <c>Engine/</c> is the only
					/// layer that names binding types.
					///
					/// Called once, when the password is complete, so nothing has to be
					/// converted or buffered per keystroke.
					/// </remarks>
					std::string toUtf8(const std::wstring& text)
					{
						if (text.empty())
							return std::string();

						int size = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
														 nullptr, 0, nullptr, nullptr);
						if (size <= 0)
							return std::string();

						std::string utf8(static_cast<size_t>(size), 0);
						::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
											  &utf8[0], size, nullptr, nullptr);
						return utf8;
					}
#endif

#if !defined(_WIN32)
					/// <summary>Reads one key without echoing it, POSIX raw mode.</summary>
					int readKeyNoEcho()
					{
						termios original {};
						tcgetattr(STDIN_FILENO, &original);
						termios raw = original;
						raw.c_lflag &= ~(ICANON | ECHO);
						tcsetattr(STDIN_FILENO, TCSANOW, &raw);

						int key = std::getchar();

						tcsetattr(STDIN_FILENO, TCSANOW, &original);
						return key;
					}
#endif
				}

				SystemConsole::SystemConsole(const Configuration::ConsoleOptions& options)
					: SystemConsole(options, shouldColor(options.color, streamRedirected(stderr)), shouldColor(options.color, streamRedirected(stdout)))
				{
				}

				SystemConsole::SystemConsole(const Configuration::ConsoleOptions& options, bool colorErrors, bool colorVerbose)
					: _options(options), _colorErrors(colorErrors), _colorVerbose(colorVerbose)
				{
					useVerbatimNewlines();
				}

				void SystemConsole::write(const std::string& text)
				{
					if (_options.silent)
						return;

					std::cout << text;
					std::cout.flush();
				}

				void SystemConsole::writeLine(const std::string& text)
				{
					if (_options.silent)
						return;

					std::cout << text << lineTerminator;
					std::cout.flush();
				}

				void SystemConsole::writeError(const std::string& text)
				{
					// std::cerr is unit-buffered by the standard, so this flushes on its
					// own; std::cout is not, which is why write()/writeLine()/writeVerbose()
					// flush explicitly below --- .NET's Console auto-flushes both, and a
					// process whose output is piped rather than run interactively must not
					// depend on an implicit flush happening before it exits.
					std::cerr << (_colorErrors ? red + text + reset : text) << lineTerminator;
				}

				void SystemConsole::writeVerbose(const std::string& text)
				{
					if (!_options.verbose || _options.silent)
						return;

					std::cout << (_colorVerbose ? dim + text + reset : text) << lineTerminator;
					std::cout.flush();
				}

				std::ostream& SystemConsole::openStandardOutput()
				{
					// Already binary from the constructor, which needs it for text output
					// too --- see useVerbatimNewlines. Kept because this is the call site
					// where document bytes depend on it, and a reader asking "what stops a
					// PDF being mangled on the way to stdout?" should find the answer
					// here rather than having to know about the constructor.
					useVerbatimNewlines();
					return std::cout;
				}

				bool SystemConsole::isInputInteractive() const { return ISATTY(FILENO(stdin)) != 0; }

				/// <remarks>
				/// Read whole and split by the same rule as a file, rather than with
				/// <c>std::getline</c>. getline ends a line only on LF, so a CRLF list left a
				/// carriage return on the end of every path -- invisible in the error message
				/// and fatal to the lookup -- and a CR-only list arrived as one path. Windows
				/// hid half of that, because its text mode translates CRLF before this sees
				/// it; on Linux <c>--files-from -</c> simply did not work with a list written
				/// on Windows. C#'s <c>Console.In.ReadLine</c> ends a line on CR, LF or CRLF,
				/// and <c>Strings::splitLines</c> is where that rule already lives.
				/// </remarks>
				std::vector<std::string> SystemConsole::readAllInputLines()
				{
					std::ostringstream all;
					all << std::cin.rdbuf();
					return Strings::splitLines(all.str());
				}
				/// <remarks>
				/// A password is no longer restricted to printable ASCII. Every byte above 126
				/// used to be discarded, so a password containing any accented or non-Latin
				/// character could be supplied with -p or --password-file but never typed here.
				///
				/// The two platforms accumulate different things because they hand over
				/// different things: a POSIX terminal delivers UTF-8 bytes, which is already what
				/// this tool stores, while Windows' _getwch delivers UTF-16 code units, converted
				/// in one call once the password is complete. One asterisk per character rather
				/// than per byte or code unit, and backspace removes a whole character, so what
				/// is shown matches what was typed.
				///
				/// End of input counts as cancelling, the same as Escape. The prompt only runs
				/// on a terminal, but a terminal can still end: close the window or drop the
				/// connection and the read returns EOF and goes on returning it. Appending that
				/// as a byte --- 0xff on POSIX, U+FFFF on Windows --- never ends the loop, so
				/// the process would spin at full speed growing a password nobody is typing.
				/// </remarks>
				std::optional<std::string> SystemConsole::readPassword(const std::string& prompt)
				{
					std::cout << prompt;
					std::cout.flush();

				#if defined(_WIN32)
					std::wstring typed;
				#else
					std::string typed;
				#endif

					while (true)
					{
				#if defined(_WIN32)
						wint_t key = _getwch();
						bool endOfInput = key == WEOF;
				#else
						int key = readKeyNoEcho();
						bool endOfInput = key == EOF;
				#endif
						if (endOfInput)
						{
							std::cout << lineTerminator;
							return std::nullopt;
						}

						if (key == '\r' || key == '\n')
						{
							std::cout << lineTerminator;
				#if defined(_WIN32)
							return toUtf8(typed);
				#else
							return typed;
				#endif
						}

						if (key == 27) // Escape
						{
							std::cout << lineTerminator;
							return std::nullopt;
						}

						if (key == '\b' || key == 127) // Backspace / DEL
						{
							if (typed.empty())
								continue;

				#if defined(_WIN32)
							// A non-BMP character is two code units; removing one would leave half of it.
							bool lowSurrogate = typed.back() >= 0xDC00 && typed.back() <= 0xDFFF;
							typed.pop_back();
							if (lowSurrogate && !typed.empty())
								typed.pop_back();
				#else
							// Drop the trailing UTF-8 continuation bytes, then the character's lead byte.
							while (!typed.empty() && (static_cast<unsigned char>(typed.back()) & 0xC0) == 0x80)
								typed.pop_back();
							if (!typed.empty())
								typed.pop_back();
				#endif
							std::cout << "\b \b";
							continue;
						}

				#if defined(_WIN32)
						if (key < 32) // any other control key: ignored, as before
							continue;

						typed += static_cast<wchar_t>(key);

						// A high surrogate is half a character; its low half follows and completes it.
						if (key < 0xD800 || key > 0xDBFF)
							std::cout << '*';
				#else
						unsigned char byte = static_cast<unsigned char>(key);
						if (byte < 32) // any other control key: ignored, as before
							continue;

						typed += static_cast<char>(byte);

						// A continuation byte (10xxxxxx) is the rest of a character already shown.
						if ((byte & 0xC0) != 0x80)
							std::cout << '*';
				#endif
					}
				}

				void SystemConsole::waitForKey()
				{
#if defined(_WIN32)
					_getch();
#else
					readKeyNoEcho();
#endif
				}
			}
		}
	}
}
