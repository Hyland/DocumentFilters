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

package com.hyland.documentfilters.cli.tests.unit;

import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertFalse;
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;

import com.hyland.documentfilters.cli.configuration.ExtractOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.engine.EngineException;
import com.hyland.documentfilters.cli.execution.SubfileEntry;
import com.hyland.documentfilters.cli.execution.SubfileExtraction;
import com.hyland.documentfilters.cli.planning.NameMatcher;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocument;
import com.hyland.documentfilters.cli.tests.fakes.FakeSubFile;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import java.time.LocalDateTime;
import java.util.Collections;
import java.util.List;

/**
 * What the subfile walk does with each way a container can misbehave.
 *
 * <p>The engine hands subfile names straight out of the container, which is
 * attacker-controlled data in a way a file on disk is not. These check both halves:
 * that a failure inside a container is handled, and that a hostile name cannot
 * write outside the output directory.
 */
public final class EngineSubfileOutcomeTests
{
	private static SharedOptions shared()
	{
		return shared(false, null);
	}

	private static SharedOptions shared(boolean subfilesEnabled, Integer depth)
	{
		SharedOptions shared = new SharedOptions();
		shared.output.outDir = "out";
		shared.subfiles.enabled = subfilesEnabled;
		shared.subfiles.maxDepth = depth;
		return shared;
	}

	private static FakeSubFile subfile(String name, String id, FakeDocument content)
	{
		FakeSubFile subfile = new FakeSubFile(name, id);
		subfile.content = content;
		return subfile;
	}

	// ============================================ listing

	@Test
	public void everyEntryTheContainerReportsIsListed()
	{
		FakeDocument container = new FakeDocument("bundle.zip");
		FakeSubFile readme = new FakeSubFile("readme.txt", "0001");
		readme.size = 4096;
		readme.modified = LocalDateTime.of(2026, 1, 15, 9, 30, 0);
		container.subFileEntries.add(readme);
		FakeSubFile report = new FakeSubFile("reports/q1.docx", "0002");
		report.size = 20480;
		container.subFileEntries.add(report);

		List<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem()).listAll(container, "bundle.zip", shared());

		assertEquals(2, entries.size());
		assertEquals("bundle.zip>readme.txt", entries.get(0).displayName);
		assertEquals("2026-01-15T09:30:00Z", entries.get(0).modified);
		assertEquals(1, entries.get(0).depth);
	}

	@Test
	public void aContainerWithNoEntriesListsNothingRatherThanFailing()
	{
		List<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem())
			.listAll(new FakeDocument("flat.txt"), "flat.txt", shared());

		assertEquals(Collections.emptyList(), entries);
	}

	@Test
	public void aContainerThatFailsToEnumerateSurfacesTheEngineError()
	{
		// Nothing above the seam catches IGRException, so this has to arrive as an
		// EngineException for the command layer to turn into a DocumentError.
		for (EngineError error : EngineOutcomeTests.everyEngineError())
		{
			FakeDocument container = new FakeDocument("bundle.zip");
			container.readFailure = error;
			container.readFailureOn = "SubFiles";

			EngineException thrown = assertThrows(EngineException.class,
				() -> new SubfileExtraction(new InMemoryFileSystem()).listAll(container, "bundle.zip", shared()));

			assertEquals(error, thrown.getError());
			assertEquals(error.getValue(), thrown.getCode());
		}
	}

	// ============================================ nesting

	@Test
	public void nestedContainersAreWalkedWhenSubfilesAreEnabled()
	{
		FakeDocument inner = new FakeDocument("inner.zip");
		inner.subFileEntries.add(new FakeSubFile("note.txt", "0001"));

		FakeDocument outer = new FakeDocument("nested.zip");
		outer.subFileEntries.add(subfile("inner.zip", "0001", inner));

		List<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem())
			.listAll(outer, "nested.zip", shared(true, null));

		assertEquals(2, entries.size());
		assertEquals("nested.zip>inner.zip", entries.get(0).displayName);
		assertEquals("nested.zip>inner.zip>note.txt", entries.get(1).displayName);
		assertEquals(2, entries.get(1).depth);
	}

	@Test
	public void subfileDepthStopsTheWalkWhereItSays()
	{
		FakeDocument deepest = new FakeDocument("deepest.zip");
		deepest.subFileEntries.add(new FakeSubFile("buried.txt", "0001"));

		FakeDocument middle = new FakeDocument("middle.zip");
		middle.subFileEntries.add(subfile("deepest.zip", "0001", deepest));

		FakeDocument outer = new FakeDocument("outer.zip");
		outer.subFileEntries.add(subfile("middle.zip", "0001", middle));

		List<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem())
			.listAll(outer, "outer.zip", shared(true, 2));

		// Depth 2 reaches middle.zip's own children and stops before their contents.
		assertEquals(2, entries.size());
		for (SubfileEntry entry : entries)
			assertFalse("buried.txt".equals(entry.name), "the walk went past its depth to " + entry.displayName);
	}

	@Test
	public void withoutSubfilesEnabledOnlyTheTopLevelIsReported()
	{
		FakeDocument inner = new FakeDocument("inner.zip");
		inner.subFileEntries.add(new FakeSubFile("note.txt", "0001"));

		FakeDocument outer = new FakeDocument("nested.zip");
		outer.subFileEntries.add(subfile("inner.zip", "0001", inner));

		List<SubfileEntry> entries = new SubfileExtraction(new InMemoryFileSystem()).listAll(outer, "nested.zip", shared());

		assertEquals(1, entries.size());
	}

	@Test
	public void anEntryThatWillNotOpenSurfacesTheEngineError()
	{
		for (EngineError error : EngineOutcomeTests.everyEngineError())
		{
			FakeDocument outer = new FakeDocument("nested.zip");
			FakeSubFile entry = new FakeSubFile("inner.zip", "0001");
			entry.openFailure = error;
			outer.subFileEntries.add(entry);

			EngineException thrown = assertThrows(EngineException.class,
				() -> new SubfileExtraction(new InMemoryFileSystem()).listAll(outer, "nested.zip", shared(true, null)));

			assertEquals(error, thrown.getError());
		}
	}

	// ============================================ extraction

	@Test
	public void extractionWritesEveryEntryThatPassesTheFilters()
	{
		FakeDocument container = new FakeDocument("archive.zip");
		FakeSubFile keep = new FakeSubFile("keep.docx", "0001");
		FakeSubFile drop = new FakeSubFile("draft-notes.txt", "0002");
		container.subFileEntries.add(keep);
		container.subFileEntries.add(drop);

		// Compiled the way the command line compiles them when the argument is read,
		// so this tests the walk's use of a matcher rather than the binding of one.
		ExtractOptions options = new ExtractOptions();
		options.subfileInclude = NameMatcher.fromGlobs(Collections.singletonList("*.docx"));

		List<String> written = new SubfileExtraction(new InMemoryFileSystem())
			.extractAll(container, "archive.zip", shared(), options);

		assertEquals(1, written.size());
		assertEquals(1, keep.copiedTo.size());
		assertEquals(0, drop.copiedTo.size());
	}

	@Test
	public void anEntryThatWillNotCopySurfacesTheEngineError()
	{
		for (EngineError error : EngineOutcomeTests.everyEngineError())
		{
			FakeDocument container = new FakeDocument("archive.zip");
			FakeSubFile entry = new FakeSubFile("broken.docx", "0001");
			entry.copyFailure = error;
			container.subFileEntries.add(entry);

			EngineException thrown = assertThrows(EngineException.class,
				() -> new SubfileExtraction(new InMemoryFileSystem()).extractAll(container, "archive.zip", shared(), new ExtractOptions()));

			assertEquals(error, thrown.getError());
		}
	}

	/**
	 * A container-supplied name can never write outside the output directory.
	 *
	 * <p>The name comes from the archive, not from the person running the tool, so
	 * it is attacker-controlled: traversal, an absolute path, a drive letter and a
	 * UNC share all have to be neutralised. Checked through the real extraction path
	 * rather than only against {@code resolveSafeOutputPath}, so that a future caller
	 * bypassing it is caught here.
	 */
	@Test
	public void aHostileEntryNameStaysInsideTheOutputDirectory()
	{
		String[] hostileNames =
		{
			"../../outside.txt",
			"..\\..\\outside.txt",
			"/etc/passwd",
			"C:\\Windows\\System32\\evil.dll",
			"\\\\server\\share\\evil.dll",
			"../",
			"..",
		};

		for (String hostileName : hostileNames)
		{
			FakeDocument container = new FakeDocument("archive.zip");
			FakeSubFile entry = new FakeSubFile(hostileName, "0001");
			container.subFileEntries.add(entry);

			List<String> written = new SubfileExtraction(new InMemoryFileSystem())
				.extractAll(container, "archive.zip", shared(), new ExtractOptions());

			assertEquals(1, written.size());
			String target = written.get(0);
			assertTrue(target.startsWith("out/"), hostileName + " resolved to " + target);
			assertFalse(target.contains(".."), hostileName + " resolved to " + target);
			assertEquals(1, entry.copiedTo.size());
			assertEquals(target, entry.copiedTo.get(0));
		}
	}

	@Test
	public void imagesAreExtractedOnlyWhenAskedFor()
	{
		FakeDocument container = new FakeDocument("doc.docx");
		FakeSubFile image = new FakeSubFile("image1.png", "0001");
		container.imageEntries.add(image);

		assertEquals(Collections.emptyList(),
			new SubfileExtraction(new InMemoryFileSystem()).extractAll(container, "doc.docx", shared(), new ExtractOptions()));
		assertEquals(0, image.copiedTo.size());

		ExtractOptions options = new ExtractOptions();
		options.images = true;
		List<String> withImages = new SubfileExtraction(new InMemoryFileSystem()).extractAll(container, "doc.docx", shared(), options);

		assertEquals(1, withImages.size());
		assertEquals(1, image.copiedTo.size());
	}

	// ============================================ timestamps

	@Test
	public void aContainerThatRecordsNoTimestampReportsNone()
	{
		// The binding gives a placeholder date for an entry with none; reporting that
		// as a date in year one, or at the epoch, would be a lie.
		assertNull(SubfileExtraction.formatTimestamp(null));
		assertNull(SubfileExtraction.formatTimestamp(LocalDateTime.of(1, 1, 1, 0, 0)));
		assertNull(SubfileExtraction.formatTimestamp(LocalDateTime.of(1970, 1, 1, 0, 0)));
		assertEquals("2026-01-15T09:30:00Z", SubfileExtraction.formatTimestamp(LocalDateTime.of(2026, 1, 15, 9, 30, 0)));
	}
}
