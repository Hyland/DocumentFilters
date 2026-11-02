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
import static com.hyland.documentfilters.cli.tests.Assert.assertNotEquals;
import static com.hyland.documentfilters.cli.tests.Assert.assertNull;
import static com.hyland.documentfilters.cli.tests.Assert.assertThrows;
import static com.hyland.documentfilters.cli.tests.Assert.assertTrue;
import static com.hyland.documentfilters.cli.tests.Assert.fail;

import com.hyland.documentfilters.cli.configuration.InputOptions;
import com.hyland.documentfilters.cli.discovery.InputEnumerator;
import com.hyland.documentfilters.cli.discovery.InputSource;
import com.hyland.documentfilters.cli.planning.NameMatcher;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.NoInputsError;
import com.hyland.documentfilters.cli.tests.Test;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * The filesystem walk: which files a command line actually selects.
 *
 * <p>Runs entirely against InMemoryFileSystem, so these are ordinary fast unit tests
 * despite being about the filesystem. Directory depth here is entirely separate
 * from subfile depth, which is recursion into documents nested inside other
 * documents.
 */
public final class InputEnumeratorTests
{
	private static InMemoryFileSystem tree()
	{
		return new InMemoryFileSystem()
			.addFile("corpus/a.docx")
			.addFile("corpus/b.pdf")
			.addFile("corpus/reports/q1.docx")
			.addFile("corpus/reports/q2.docx")
			.addFile("corpus/reports/archive/old.docx")
			.addFile("corpus/notes.txt");
	}

	private static List<String> walk(InMemoryFileSystem fs, InputOptions options)
	{
		InputEnumerator enumerator = new InputEnumerator(fs, new RecordingConsole());
		List<String> paths = new ArrayList<>();

		for (InputSource source : enumerator.enumerate(options))
			paths.add(source.path);

		return paths;
	}

	private static InputOptions from(String... paths)
	{
		InputOptions options = new InputOptions();
		options.paths.addAll(Arrays.asList(paths));
		return options;
	}

	// -------------------------------------------------------------------------- depth

	@Test
	public void aDirectoryIsWalkedRecursivelyByDefault()
	{
		// Matching fd and ripgrep rather than grep and cp: a directory handed to a batch
		// converter is a corpus to walk.
		List<String> found = walk(tree(), from("corpus"));

		assertTrue(found.contains("corpus/a.docx"));
		assertTrue(found.contains("corpus/reports/q1.docx"));
		assertTrue(found.contains("corpus/reports/archive/old.docx"));
	}

	@Test
	public void noRecurseStaysInTheNamedDirectory()
	{
		InputOptions options = from("corpus");
		options.noRecurseWasGiven = true;

		List<String> found = walk(tree(), options);

		assertTrue(found.contains("corpus/a.docx"));
		assertFalse(found.contains("corpus/reports/q1.docx"));
	}

	@Test
	public void maxDepthOneIsTheSameAsNoRecurse()
	{
		InputOptions options = from("corpus");
		options.maxDepth = 1;

		assertFalse(walk(tree(), options).contains("corpus/reports/q1.docx"));
	}

	@Test
	public void maxDepthTwoReachesOneLevelOfSubdirectory()
	{
		InputOptions options = from("corpus");
		options.maxDepth = 2;

		List<String> found = walk(tree(), options);

		assertTrue(found.contains("corpus/reports/q1.docx"));
		assertFalse(found.contains("corpus/reports/archive/old.docx"));
	}

	// ----------------------------------------------------------------- explicit files

	@Test
	public void anExplicitFileIsTakenAsGiven()
	{
		assertEquals(Arrays.asList("corpus/a.docx"), walk(tree(), from("corpus/a.docx")));
	}

	@Test
	public void filesAndDirectoriesCanBeMixedAndKeepTheirOrder()
	{
		List<String> found = walk(tree(), from("corpus/notes.txt", "corpus/reports"));

		assertEquals("corpus/notes.txt", found.get(0));
		assertTrue(found.contains("corpus/reports/q1.docx"));
	}

	@Test
	public void anExplicitFileIsNotSubjectToTheNameMatcher()
	{
		// Naming a file directly is an instruction, not a suggestion; a glob filters what
		// a walk discovers.
		InputOptions options = from("corpus/notes.txt");
		options.matcher = NameMatcher.fromGlobs(Arrays.asList("**/*.docx"));

		assertEquals(Arrays.asList("corpus/notes.txt"), walk(tree(), options));
	}

	// --------------------------------------------------------------------- selection

	@Test
	public void aGlobFiltersWhatTheWalkDiscovers()
	{
		InputOptions options = from("corpus");
		options.matcher = NameMatcher.fromGlobs(Arrays.asList("**/*.docx"));

		List<String> found = walk(tree(), options);

		assertTrue(found.contains("corpus/a.docx"));
		assertFalse(found.contains("corpus/b.pdf"));
		assertFalse(found.contains("corpus/notes.txt"));
	}

	@Test
	public void anExclusionRemovesASubtree()
	{
		InputOptions options = from("corpus");
		options.matcher = NameMatcher.fromGlobs(Arrays.asList("**/*.docx", "!**/archive/*"));

		List<String> found = walk(tree(), options);

		assertTrue(found.contains("corpus/reports/q1.docx"));
		assertFalse(found.contains("corpus/reports/archive/old.docx"));
	}

	// ------------------------------------------------------------------------ hidden

	@Test
	public void hiddenEntriesAreSkippedByDefault()
	{
		InMemoryFileSystem fs = tree().addFile("corpus/.secret.docx").markHidden("corpus/.secret.docx");

		assertFalse(walk(fs, from("corpus")).contains("corpus/.secret.docx"));
	}

	@Test
	public void includeHiddenBringsThemBack()
	{
		InMemoryFileSystem fs = tree().addFile("corpus/.secret.docx").markHidden("corpus/.secret.docx");
		InputOptions options = from("corpus");
		options.includeHidden = true;

		assertTrue(walk(fs, options).contains("corpus/.secret.docx"));
	}

	@Test
	public void aHiddenDirectoryHidesEverythingUnderIt()
	{
		InMemoryFileSystem fs = tree().addFile("corpus/.git/config.docx").markHidden("corpus/.git");

		assertFalse(walk(fs, from("corpus")).contains("corpus/.git/config.docx"));
	}

	// ---------------------------------------------------------------------- symlinks

	@Test
	public void aSymlinkedDirectoryIsNotEnteredByDefault()
	{
		// Following by default risks walking in a circle.
		InMemoryFileSystem fs = tree().addFile("corpus/link/x.docx").markSymlink("corpus/link");

		assertFalse(walk(fs, from("corpus")).contains("corpus/link/x.docx"));
	}

	@Test
	public void followEntersSymlinkedDirectories()
	{
		InMemoryFileSystem fs = tree().addFile("corpus/link/x.docx").markSymlink("corpus/link");
		InputOptions options = from("corpus");
		options.followSymlinks = true;

		assertTrue(walk(fs, options).contains("corpus/link/x.docx"));
	}

	@Test
	public void followTerminatesWhenALinkPointsAtAnAncestor()
	{
		// The reason the walk tracks directory identity rather than path strings: a link
		// back to its own parent produces an endless supply of distinct paths ---
		// corpus/loop/loop/loop/... --- so a visited-paths set never closes the cycle.
		InMemoryFileSystem fs = tree().markSymlink("corpus/loop", "corpus");
		InputOptions options = from("corpus");
		options.followSymlinks = true;

		for (String path : walk(fs, options))
			assertFalse(path.contains("loop/loop"), path);
	}

	@Test
	public void followVisitsASharedDirectoryOnce()
	{
		// Two links to the same place is not a cycle, but it is still one directory:
		// processing it twice would convert every document in it twice.
		InMemoryFileSystem fs = tree()
			.addFile("shared/x.docx")
			.markSymlink("corpus/one", "shared")
			.markSymlink("corpus/two", "shared");

		InputOptions options = from("corpus");
		options.followSymlinks = true;

		int copies = 0;

		for (String path : walk(fs, options))
			if (path.endsWith("x.docx"))
				copies++;

		assertEquals(1, copies);
	}

	// ---------------------------------------------------------------------------- sort

	@Test
	public void sortOrdersTheWalk()
	{
		InputOptions options = from("corpus");
		options.sort = true;

		List<String> found = walk(tree(), options);

		// String.compareTo compares UTF-16 code units, which is what
		// StringComparer.Ordinal asks for in the C#.
		assertEquals(sorted(found), found);
	}

	@Test
	public void withoutSortTheWalkFollowsTheFilesystemOrder()
	{
		// The counterpart that makes the test above meaningful: the fake returns entries
		// unordered, so if --sort were ignored the sorted assertion would still have to
		// fail somewhere.
		List<String> found = walk(tree(), from("corpus"));

		assertNotEquals(sorted(found), found);
	}

	// ---------------------------------------------------------------------- files-from

	@Test
	public void filesFromReadsAList()
	{
		InMemoryFileSystem fs = tree().addFile("list.txt", "corpus/a.docx\ncorpus/b.pdf\n");
		InputOptions options = new InputOptions();
		options.filesFrom = "list.txt";

		assertEquals(Arrays.asList("corpus/a.docx", "corpus/b.pdf"), walk(fs, options));
	}

	@Test
	public void filesFromIgnoresBlankLines()
	{
		InMemoryFileSystem fs = tree().addFile("list.txt", "corpus/a.docx\n\n\ncorpus/b.pdf\n");
		InputOptions options = new InputOptions();
		options.filesFrom = "list.txt";

		assertEquals(Arrays.asList("corpus/a.docx", "corpus/b.pdf"), walk(fs, options));
	}

	@Test
	public void filesFromDashReadsStandardInput()
	{
		// "-" is not a path, so it goes through the console seam rather than the
		// filesystem --- otherwise a test could not supply it.
		RecordingConsole console = new RecordingConsole();
		console.inputLines.add("corpus/a.docx");
		console.inputLines.add("corpus/b.pdf");

		InputEnumerator enumerator = new InputEnumerator(tree(), console);
		InputOptions options = new InputOptions();
		options.filesFrom = "-";

		List<String> found = new ArrayList<>();

		for (InputSource source : enumerator.enumerate(options))
			found.add(source.path);

		assertEquals(Arrays.asList("corpus/a.docx", "corpus/b.pdf"), found);
	}

	@Test
	public void filesFromCombinesWithPositionalPaths()
	{
		InMemoryFileSystem fs = tree().addFile("list.txt", "corpus/b.pdf\n");
		InputOptions options = from("corpus/a.docx");
		options.filesFrom = "list.txt";

		List<String> found = walk(fs, options);

		assertTrue(found.contains("corpus/a.docx"));
		assertTrue(found.contains("corpus/b.pdf"));
	}

	// ------------------------------------------------------------------ relative paths

	@Test
	public void aWalkedFileRecordsItsPathRelativeToTheRoot()
	{
		// This is what lets --out-dir replicate the source tree.
		InputEnumerator enumerator = new InputEnumerator(tree(), new RecordingConsole());
		List<InputSource> matches = new ArrayList<>();

		for (InputSource source : enumerator.enumerate(from("corpus")))
			if (source.path.equals("corpus/reports/q1.docx"))
				matches.add(source);

		assertEquals(1, matches.size());
		InputSource found = matches.get(0);

		assertEquals("reports/q1.docx", found.relativePath);
		assertEquals("corpus", found.root);
	}

	@Test
	public void anExplicitFileHasNoRootAndIsRelativeToItself()
	{
		InputEnumerator enumerator = new InputEnumerator(tree(), new RecordingConsole());

		List<InputSource> found = enumerator.enumerate(from("corpus/a.docx"));
		assertEquals(1, found.size());

		assertEquals("a.docx", found.get(0).relativePath);
		assertNull(found.get(0).root);
	}

	@Test
	public void aBackslashPathStillReducesToABareFilename()
	{
		// A directory walk always yields forward-slash paths, built from
		// IFileSystem.listDirectory, but an explicit argument is whatever the user typed
		// on their own shell --- backslashes on Windows. Reproduces a real bug:
		// "C:\tmp\a.pdf" was kept whole as the relative path because only '/' was
		// recognised, so --out-dir nested the source drive underneath itself instead
		// of flattening to "a.pdf".
		InputEnumerator enumerator = new InputEnumerator(tree(), new RecordingConsole());

		List<InputSource> found = enumerator.enumerate(from("corpus\\a.docx"));
		assertEquals(1, found.size());

		assertEquals("a.docx", found.get(0).relativePath);
		assertNull(found.get(0).root);
	}

	// ------------------------------------------------------------------------- nothing

	@Test
	public void matchingNothingIsItsOwnError()
	{
		InputOptions options = from("corpus");
		options.matcher = NameMatcher.fromGlobs(Arrays.asList("**/*.nonexistent"));

		NoInputsError error = assertThrows(NoInputsError.class, () -> walk(tree(), options));

		assertEquals(ExitCode.NO_INPUTS_MATCHED, error.getExitCode());
	}

	@Test
	public void anEmptyDirectoryMatchesNothing()
	{
		InMemoryFileSystem fs = new InMemoryFileSystem().addDirectory("empty");

		assertThrows(NoInputsError.class, () -> walk(fs, from("empty")));
	}

	@Test
	public void aPathThatDoesNotExistIsAnError()
	{
		// Any CliError will do, as in the C#: which subclass is a detail, failing is not.
		try
		{
			walk(tree(), from("corpus/missing.docx"));
			fail("expected a CliError");
		}
		catch (CliError expected)
		{
			// The assertion is that this is reached.
		}
	}

	@Test
	public void aDirectoryThatCannotBeReadIsReportedRatherThanSkipped()
	{
		InMemoryFileSystem fs = new InMemoryFileSystem().addFile("corpus/a.docx").addFile("corpus/locked/b.docx");
		fs.unreadable.add("corpus/locked");

		NoInputsError error = assertThrows(NoInputsError.class, () -> walk(fs, from("corpus")));

		assertEquals("corpus/locked: could not be read", error.getMessage());
		assertEquals(ExitCode.NO_INPUTS_MATCHED, error.getExitCode());
	}

	private static List<String> sorted(List<String> paths)
	{
		List<String> copy = new ArrayList<>(paths);
		Collections.sort(copy);
		return copy;
	}
}
