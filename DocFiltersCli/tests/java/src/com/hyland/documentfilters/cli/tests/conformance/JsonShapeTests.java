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

package com.hyland.documentfilters.cli.tests.conformance;

import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;

import com.hyland.documentfilters.cli.Program;
import com.hyland.documentfilters.cli.engine.EngineFormat;
import com.hyland.documentfilters.cli.engine.EngineOption;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocument;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocumentFiltersEngine;
import com.hyland.documentfilters.cli.tests.fakes.FakeSubFile;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.time.LocalDateTime;
import java.util.Collections;

/**
 * The {@code --json} shape of every command that has one.
 *
 * <p>These shapes are the tool's machine-readable interface, so a renamed key or a
 * restructured array breaks whatever is parsing it.
 *
 * <p>Driven through {@link Program#run} rather than the report classes, so the
 * assertion covers the path a user actually takes (argv, dispatch, the command, the
 * JSON writer) and the engine answers from a script, which is what lets the expected
 * document be written out in full rather than described.
 */
public final class JsonShapeTests
{
	private final InMemoryFileSystem fileSystem = new InMemoryFileSystem();
	private final FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();
	private final RecordingConsole console = new RecordingConsole();

	private JsonShapeTests withDocument(String path)
	{
		fileSystem.addFile(path);
		engine.addDocument(path);
		return this;
	}

	private JsonShapeTests withDocument(String path, String text)
	{
		fileSystem.addFile(path);
		engine.addDocument(path, text);
		return this;
	}

	private String json(String... args)
	{
		assertEquals(ExitCode.SUCCESS, Program.run(args, console, fileSystem, engine));
		assertEquals(Collections.emptyList(), console.errorLines);
		// Raw, not the line list: a JSON report is built as one document and written
		// in a single call, so line capture misses it entirely.
		String output = console.output().replace("\r\n", "\n");
		while (output.endsWith("\n"))
			output = output.substring(0, output.length() - 1);
		return output;
	}

	private static String lines(String... lines)
	{
		return String.join("\n", lines);
	}

	@Test
	public void identifyReportsAnArrayOfPathFormatMimeAndName()
	{
		withDocument("sample.docx");
		FakeDocument document = engine.addDocument("sample.docx");
		document.fileType = 254;
		document.mimeType = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
		document.formatName = "Microsoft Word 2007";

		assertEquals(lines(
			"[",
			"  {",
			"    \"path\": \"sample.docx\",",
			"    \"format\": 254,",
			"    \"mime\": \"application/vnd.openxmlformats-officedocument.wordprocessingml.document\",",
			"    \"name\": \"Microsoft Word 2007\"",
			"  }",
			"]"),
			json("identify", "--json", "sample.docx"));
	}

	@Test
	public void metaReportsMetadataAsANestedObjectKeyedByFieldName()
	{
		// Nested rather than flat, and every value a string: the engine reports
		// metadata as text, so a port that guessed at numbers would disagree about
		// "Pages".
		withDocument("sample.docx", "\u000FAuthor: Jane Smith\u000ETitle: Quarterly Report\u000EPages: 12\u0010");

		assertEquals(lines(
			"[",
			"  {",
			"    \"path\": \"sample.docx\",",
			"    \"metadata\": {",
			"      \"Author\": \"Jane Smith\",",
			"      \"Title\": \"Quarterly Report\",",
			"      \"Pages\": \"12\"",
			"    }",
			"  }",
			"]"),
			json("meta", "--json", "sample.docx"));
	}

	@Test
	public void listReportsSubfilesAsAnArrayInsideEachContainer()
	{
		withDocument("bundle.zip");
		FakeDocument container = engine.addDocument("bundle.zip");
		FakeSubFile readme = new FakeSubFile("readme.txt", "0001");
		readme.size = 4096;
		readme.modified = LocalDateTime.of(2026, 1, 15, 9, 30, 0);
		container.subFileEntries.add(readme);

		assertEquals(lines(
			"[",
			"  {",
			"    \"path\": \"bundle.zip\",",
			"    \"subfiles\": [",
			"      {",
			"        \"id\": \"0001\",",
			"        \"name\": \"readme.txt\",",
			"        \"size\": 4096,",
			"        \"modified\": \"2026-01-15T09:30:00Z\",",
			"        \"depth\": 1",
			"      }",
			"    ]",
			"  }",
			"]"),
			json("list", "--json", "bundle.zip"));
	}

	@Test
	public void formatsReportsAFlatArrayOfIdExtAndName()
	{
		engine.formats.add(format(254, "docx", "Microsoft Word 2007"));
		engine.formats.add(format(112, "doc", "Microsoft Word for Windows"));

		assertEquals(lines(
			"[",
			"  {",
			"    \"id\": 254,",
			"    \"ext\": \"docx\",",
			"    \"name\": \"Microsoft Word 2007\"",
			"  },",
			"  {",
			"    \"id\": 112,",
			"    \"ext\": \"doc\",",
			"    \"name\": \"Microsoft Word for Windows\"",
			"  }",
			"]"),
			json("formats", "--json", "--filter", "word"));
	}

	@Test
	public void optionsReportsNameTypeDefaultAppliesToAndDescription()
	{
		// appliesTo is the one camelCase key in the whole surface; everything else is
		// a single lowercase word, so it is easy to port as "applies-to" or
		// "appliesto" by accident.
		EngineOption option = new EngineOption();
		option.displayName = "BW_MODE";
		option.type = "int";
		option.defaultValue = "0";
		option.flags = 0x2; // canvas; 0x1 is document, and both join as "document,canvas"
		option.description = "Black and white rendering mode";
		engine.options.add(option);

		assertEquals(lines(
			"[",
			"  {",
			"    \"name\": \"BW_MODE\",",
			"    \"type\": \"int\",",
			"    \"default\": \"0\",",
			"    \"appliesTo\": \"canvas\",",
			"    \"description\": \"Black and white rendering mode\"",
			"  }",
			"]"),
			json("options", "--json", "--filter", "BW_MODE"));
	}

	@Test
	public void everyJsonCommandEmitsOneDocumentEvenWithNothingToReport()
	{
		// An empty array, not empty output: whatever is parsing this should not have
		// to special-case "no results" as a parse failure.
		assertEquals("[]", json("formats", "--json", "--filter", "nothing-matches-this"));
		assertEquals("[]", new JsonShapeTests().json("options", "--json", "--filter", "nothing-matches-this"));
	}

	private static EngineFormat format(int id, String shortName, String displayName)
	{
		EngineFormat format = new EngineFormat();
		format.id = id;
		format.shortName = shortName;
		format.displayName = displayName;
		return format;
	}
}
