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

#include "JsonWriter.h"

#include <cstdio>
#include <stdexcept>

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
					// Recognises a UTF-8 encoded surrogate code point (U+D800..U+DFFF)
					// at index: lead byte 0xED, a byte in 0xA0..0xBF, then a
					// continuation byte. Valid UTF-8 never holds one; it arrives from
					// converting unvalidated UTF-16 (a Windows filename) unchecked.
					bool decodeSurrogate(const std::string& text, size_t index, unsigned int& codePoint)
					{
						if (index + 2 >= text.size() || static_cast<unsigned char>(text[index]) != 0xED)
							return false;

						unsigned char second = static_cast<unsigned char>(text[index + 1]);
						unsigned char third = static_cast<unsigned char>(text[index + 2]);
						if (second < 0xA0 || second > 0xBF || (third & 0xC0) != 0x80)
							return false;

						codePoint = 0xD000u | (static_cast<unsigned int>(second & 0x3F) << 6) | (third & 0x3Fu);
						return true;
					}
				}

				JsonWriter& JsonWriter::beginObject() { return beginContainer(ContainerKind::Object, '{'); }

				JsonWriter& JsonWriter::endObject() { return endContainer('}'); }

				JsonWriter& JsonWriter::beginArray() { return beginContainer(ContainerKind::Array, '['); }

				JsonWriter& JsonWriter::endArray() { return endContainer(']'); }

				JsonWriter& JsonWriter::name(const std::string& propertyName)
				{
					beginItem();
					_text += '"';
					_text += escape(propertyName);
					_text += "\": ";
					_atPropertyValue = true;
					return *this;
				}

				JsonWriter& JsonWriter::value(const std::optional<std::string>& text)
				{
					beforeValue();
					if (text.has_value())
					{
						_text += '"';
						_text += escape(*text);
						_text += '"';
					}
					else
					{
						_text += "null";
					}
					return *this;
				}

				JsonWriter& JsonWriter::value(const char* text)
				{
					return text == nullptr ? value(std::optional<std::string>(std::nullopt)) : value(std::optional<std::string>(std::string(text)));
				}

				JsonWriter& JsonWriter::value(long long number)
				{
					beforeValue();
					_text += std::to_string(number);
					return *this;
				}

				JsonWriter& JsonWriter::value(int number) { return value(static_cast<long long>(number)); }

				JsonWriter& JsonWriter::value(bool flag)
				{
					beforeValue();
					_text += flag ? "true" : "false";
					return *this;
				}

				JsonWriter& JsonWriter::property(const std::string& propertyName, const std::optional<std::string>& text)
				{
					return name(propertyName).value(text);
				}

				JsonWriter& JsonWriter::property(const std::string& propertyName, const char* text) { return name(propertyName).value(text); }

				JsonWriter& JsonWriter::property(const std::string& propertyName, long long number) { return name(propertyName).value(number); }

				JsonWriter& JsonWriter::property(const std::string& propertyName, int number) { return name(propertyName).value(number); }

				JsonWriter& JsonWriter::property(const std::string& propertyName, bool flag) { return name(propertyName).value(flag); }

				std::string JsonWriter::toString() const
				{
					if (!_open.empty())
						throw std::logic_error("a JSON object or array was left open");

					return _text + "\n";
				}

				std::string JsonWriter::escape(const std::string& text)
				{
					std::string escaped;
					escaped.reserve(text.size());

					// Indexed rather than range-for because the surrogate rule below
					// consumes three bytes at a time and needs to look ahead.
					for (size_t i = 0; i < text.size(); i++)
					{
						char c = text[i];
						switch (c)
						{
							case '"':
								escaped += "\\\"";
								break;
							case '\\':
								escaped += "\\\\";
								break;
							case '\n':
								escaped += "\\n";
								break;
							case '\r':
								escaped += "\\r";
								break;
							case '\t':
								escaped += "\\t";
								break;
							default:
							{
								// Cast to unsigned before comparing: a UTF-8 continuation byte
								// (0x80-0xFF) is negative as a signed char, which would
								// otherwise satisfy "< 0x20" and get mangled into a \u escape
								// --- exactly the non-ASCII-literal guarantee this writer
								// exists to keep.
								unsigned char byte = static_cast<unsigned char>(c);
								unsigned int surrogate = 0;
								if (byte < 0x20)
								{
									char buffer[8];
									std::snprintf(buffer, sizeof(buffer), "\\u%04x", byte);
									escaped += buffer;
								}
								else if (decodeSurrogate(text, i, surrogate))
								{
									// Deviation from the reference, which asks IsPaired because
									// its strings are UTF-16 units. Here a string is UTF-8, where
									// a valid pair is one four-byte character that never reaches
									// this branch, so a surrogate code point that does reach it
									// is unpaired by definition. Escaped as C# escapes an
									// unpaired one: six characters, lowercase hex.
									char buffer[8];
									std::snprintf(buffer, sizeof(buffer), "\\u%04x", surrogate);
									escaped += buffer;
									i += 2; // the lead byte's two continuation bytes
								}
								else
								{
									escaped += c;
								}
								break;
							}
						}
					}
					return escaped;
				}

				JsonWriter& JsonWriter::beginContainer(ContainerKind kind, char openToken)
				{
					beforeValue();
					_text += openToken;
					_open.push(Frame{ kind });
					return *this;
				}

				JsonWriter& JsonWriter::endContainer(char closeToken)
				{
					Frame frame = _open.top();
					_open.pop();

					if (frame.hasChildren)
					{
						_text += '\n';
						_text.append(_open.size() * 2, ' ');
					}
					_text += closeToken;
					return *this;
				}

				void JsonWriter::beforeValue()
				{
					if (!_atPropertyValue)
						beginItem();

					_atPropertyValue = false;
				}

				void JsonWriter::beginItem()
				{
					if (_open.empty())
						return; // the document's own root value; nothing to separate it from

					Frame& frame = _open.top();
					_text += frame.hasChildren ? ",\n" : "\n";
					_text.append(_open.size() * 2, ' ');
					frame.hasChildren = true;
				}
			}
		}
	}
}
