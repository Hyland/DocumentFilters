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

using System.Text;
using Hyland.DocumentFilters.Cli.Runtime;

using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// The <c>meta</c> command's output.
	/// </summary>
	/// <remarks>
	/// Console form is one <c>key: value</c> per line, no padding or boxes, so that
	/// the four language implementations can agree without anyone matching column
	/// widths. JSON form is a flat object with keys in the order the engine reports
	/// them.
	/// </remarks>
	public sealed class MetadataReport
	{
		/// <summary>Writes the plain-text form.</summary>
		public void WriteText(IDocument document, string displayName, IConsole console)
		{
			foreach (var field in ReadFields(document))
				console.WriteLine($"{displayName}: field={field.Key} value={field.Value}");
		}

		/// <summary>Writes one document's JSON object into an open writer.</summary>
		public void WriteJson(IDocument document, string displayName, JsonWriter json)
		{
			json.BeginObject().Property("path", displayName).Name("metadata").BeginObject();
			foreach (var field in ReadFields(document))
				json.Property(field.Key, field.Value);
			json.EndObject().EndObject();
		}

		/// <summary>
		/// Reads the document's metadata fields, in the order the engine reports them.
		/// </summary>
		/// <remarks>
		/// There is no structured metadata API: the document must be opened with
		/// <see cref="ExtractorOpenSettings.ForMetadataOnly"/>, and <c>GetText</c> then
		/// returns the fields as plain text, one <c>Label: value</c> line per field —
		/// confirmed against a real <c>isys_doc2text.exe --meta</c> run, e.g.
		/// <c>Title: _.pdf</c>, <c>Author: slop001</c>, <c>xmp:dc:creator: slop001</c>.
		/// Splitting on the first <c>": "</c> rather than the first <c>':'</c> is what
		/// keeps a label such as <c>xmp:dc:creator</c>, which contains colons of its
		/// own, from being cut apart — none of its internal colons are followed by a
		/// space, only the one separating the label from the value is.
		/// </remarks>
		private static List<KeyValuePair<string, string>> ReadFields(IDocument document)
		{
			var text = new StringBuilder();
			while (!document.GetEOF())
				text.Append(document.GetText(4096));

			var fields = new List<KeyValuePair<string, string>>();

			// The engine does not separate metadata records with newlines. It emits
			// 0x0F before the block and 0x0E between records, so splitting the raw text
			// on '\n' found no boundary at all and collapsed every field of a document
			// into one, with the 0x0F still stuck to the first label. Running it
			// through the same table the text path uses turns those into newlines.
			foreach (string record in TextConversion.StripControlCodes(text.ToString()).Split('\n'))
			{
				// CR and TAB inside a value each become a single space, so a record
				// stays one line however the document wrote it. A bare CR is the one
				// that matters: it survives the split above and returns a terminal's
				// cursor to column 0, so the record overwrites itself.
				string line = record.Replace('\r', ' ').Replace('\t', ' ').Trim();
				if (line.Length == 0)
					continue;

				int separator = line.IndexOf(": ", StringComparison.Ordinal);
				if (separator >= 0)
				{
					fields.Add(new KeyValuePair<string, string>(line.Substring(0, separator), line.Substring(separator + 2)));
				}
				else if (fields.Count > 0)
				{
					// No label, so this is the rest of a value that carried a newline of
					// its own — a Word Title or Comments field routinely does. Dropping
					// it silently truncated the value; it rejoins with the single space
					// the newline became.
					KeyValuePair<string, string> previous = fields[fields.Count - 1];
					fields[fields.Count - 1] = new KeyValuePair<string, string>(previous.Key, previous.Value + " " + line);
				}
			}

			return fields;
		}
	}
}
