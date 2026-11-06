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

package com.hyland.documentfilters.cli.execution;

import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.JsonWriter;
import java.util.AbstractMap.SimpleEntry;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * The {@code meta} command's output.
 *
 * <p>Console form is one {@code key: value} per line, no padding or boxes, so that
 * the four language implementations can agree without anyone matching column
 * widths. JSON form is a flat object with keys in the order the engine reports
 * them.
 */
public final class MetadataReport
{
	/** Writes the plain-text form. */
	public void writeText(IDocument document, String displayName, IConsole console)
	{
		for (Map.Entry<String, String> field : readFields(document))
			console.writeLine(displayName + ": field=" + field.getKey() + " value=" + field.getValue());
	}

	/** Writes one document's JSON object into an open writer. */
	public void writeJson(IDocument document, String displayName, JsonWriter json)
	{
		json.beginObject().property("path", displayName).name("metadata").beginObject();
		for (Map.Entry<String, String> field : readFields(document))
			json.property(field.getKey(), field.getValue());
		json.endObject().endObject();
	}

	/**
	 * Reads the document's metadata fields, in the order the engine reports them.
	 *
	 * <p>There is no structured metadata API: the document must be opened with
	 * {@link ExtractorOpenSettings#forMetadataOnly}, and {@code getText} then
	 * returns the fields as plain text, one {@code Label: value} line per field.
	 * Splitting on the first {@code ": "} rather than the first {@code ':'} is what
	 * keeps a label such as {@code xmp:dc:creator}, which contains colons of its
	 * own, from being cut apart.
	 */
	private static List<Map.Entry<String, String>> readFields(IDocument document)
	{
		StringBuilder text = new StringBuilder();
		while (!document.getEOF())
			text.append(document.getText(4096));

		List<Map.Entry<String, String>> fields = new ArrayList<>();

		// The engine does not separate metadata records with newlines. It emits
		// 0x0F before the block and 0x0E between records; running the text through
		// the same table the text path uses turns those into newlines.
		for (String record : TextConversion.stripControlCodes(text.toString()).split("\n", -1))
		{
			// CR and TAB inside a value each become a single space, so a record
			// stays one line however the document wrote it. A bare CR is the one
			// that matters: it survives the split above and returns a terminal's
			// cursor to column 0, so the record overwrites itself.
			String line = ValueParsers.trim(record.replace('\r', ' ').replace('\t', ' '));
			if (line.isEmpty())
				continue;

			int separator = line.indexOf(": ");
			if (separator >= 0)
			{
				fields.add(new SimpleEntry<>(line.substring(0, separator), line.substring(separator + 2)));
			}
			else if (!fields.isEmpty())
			{
				// No label, so this is the rest of a value that carried a newline of
				// its own --- a Word Title or Comments field routinely does. It
				// rejoins with the single space the newline became.
				Map.Entry<String, String> previous = fields.get(fields.size() - 1);
				fields.set(fields.size() - 1, new SimpleEntry<>(previous.getKey(), previous.getValue() + " " + line));
			}
		}

		return fields;
	}
}
