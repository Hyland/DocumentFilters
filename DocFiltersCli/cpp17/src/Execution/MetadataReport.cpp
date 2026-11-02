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

#include "Execution/MetadataReport.h"
#include "Execution/TextConversion.h"
#include "Runtime/Strings.h"

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Execution
			{
				namespace
				{
					/// <summary>
					/// Reads the document's metadata fields, in the order the engine
					/// reports them.
					/// </summary>
					/// <remarks>
					/// There is no structured metadata API: the document must be opened
					/// with <c>ExtractorOpenSettings::forMetadataOnly</c>, and
					/// <c>getText</c> then returns the fields as plain text, one
					/// <c>Label: value</c> line per field --- confirmed against a real
					/// <c>isys_doc2text.exe --meta</c> run, e.g. <c>Title: _.pdf</c>,
					/// <c>Author: slop001</c>, <c>xmp:dc:creator: slop001</c>. Splitting
					/// on the first <c>": "</c> rather than the first <c>':'</c> is what
					/// keeps a label such as <c>xmp:dc:creator</c>, which contains colons
					/// of its own, from being cut apart --- none of its internal colons
					/// are followed by a space, only the one separating the label from the
					/// value is.
					/// </remarks>
					std::vector<std::pair<std::string, std::string>> readFields(Engine::IDocument& document)
					{
						std::string text;
						while (!document.getEOF())
							text += document.getText(4096);

						std::vector<std::pair<std::string, std::string>> fields;

						// The engine does not separate metadata records with newlines. It
						// emits 0x0F before the block and 0x0E between records, so
						// splitting the raw text on '\n' found no boundary at all and
						// collapsed every field of a document into one, with the 0x0F
						// still stuck to the first label. Running it through the same
						// table the text path uses turns those into newlines.
						for (const std::string& record : Runtime::Strings::splitLines(TextConversion::stripControlCodes(text)))
						{
							// CR and TAB inside a value each become a single space, so a
							// record stays one line however the document wrote it. A bare
							// CR is the one that matters: it survives the split above and
							// returns a terminal's cursor to column 0, so the record
							// overwrites itself.
							std::string line = record;
							for (char& ch : line)
							{
								if (ch == '\r' || ch == '\t')
									ch = ' ';
							}
							line = Runtime::Strings::trim(line);
							if (line.empty())
								continue;

							std::size_t separator = line.find(": ");
							if (separator != std::string::npos)
							{
								fields.emplace_back(line.substr(0, separator), line.substr(separator + 2));
							}
							else if (!fields.empty())
							{
								// No label, so this is the rest of a value that carried a
								// newline of its own --- a Word Title or Comments field
								// routinely does. Dropping it silently truncated the
								// value; it rejoins with the single space the newline
								// became.
								fields.back().second += " " + line;
							}
						}

						return fields;
					}
				}

				namespace MetadataReport
				{
					void writeText(Engine::IDocument& document, const std::string& displayName, Runtime::IConsole& console)
					{
						for (const auto& field : readFields(document))
							console.writeLine(displayName + ": field=" + field.first + " value=" + field.second);
					}

					void writeJson(Engine::IDocument& document, const std::string& displayName, Runtime::JsonWriter& json)
					{
						json.beginObject().property("path", displayName).name("metadata").beginObject();
						for (const auto& field : readFields(document))
							json.property(field.first, field.second);
						json.endObject().endObject();
					}
				}
			}
		}
	}
}
