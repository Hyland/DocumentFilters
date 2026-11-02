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

package com.hyland.documentfilters.cli.discovery;

import com.hyland.documentfilters.cli.configuration.InputOptions;
import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.NoInputsError;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Turns the positional paths and {@code --files-from} list into the concrete set
 * of files to process.
 *
 * <p>Directories are walked recursively by default, the way {@code fd} and
 * {@code ripgrep} behave; {@code --max-depth} and {@code --no-recurse} limit that.
 * Directory depth is entirely separate from subfile depth, which controls
 * recursion into documents nested inside other documents.
 *
 * <p>{@code -g/--glob} and {@code --regex} match against a walked entry's path
 * relative to the root it was found under, not the root-qualified path: an
 * anchored pattern such as {@code ^report-\d{4}\.pdf$} would otherwise never
 * match anything once the walked root itself has more than one path segment.
 * An explicit file argument is never checked against either --- naming a file
 * is an instruction, not a suggestion.
 */
public final class InputEnumerator
{
	private final IFileSystem _fileSystem;
	private final IConsole _console;

	/**
	 * Directory identities already entered by the in-progress {@link #walkDirectory} call.
	 *
	 * <p>Reset per root rather than per {@link InputEnumerator} instance: two different
	 * explicit roots are free to share a directory, but a link cycle within one root
	 * must not loop forever. Keyed by identity, not path, because a link to an
	 * ancestor produces an endless supply of distinct path strings for what is
	 * really one directory.
	 */
	private Set<String> _visited;

	public InputEnumerator(IFileSystem fileSystem, IConsole console)
	{
		_fileSystem = fileSystem;
		_console = console;
	}

	/**
	 * Produces the input list, in processing order. Selection by detected format
	 * ({@code --include-types} / {@code --exclude-types}) is not applied here,
	 * because it needs the engine to identify each file; that happens during
	 * execution.
	 *
	 * @throws NoInputsError nothing matched.
	 */
	public List<InputSource> enumerate(InputOptions options)
	{
		List<String> rawPaths = new ArrayList<>(options.paths);
		if (options.filesFrom != null)
			rawPaths.addAll(readFileList(options.filesFrom));
		if (rawPaths.isEmpty())
			throw new NoInputsError("no inputs given; name a file or a directory to process");

		List<InputSource> results = new ArrayList<>();
		for (String path : rawPaths)
		{
			if (_fileSystem.directoryExists(path))
				results.addAll(walkDirectory(path, options));
			else if (_fileSystem.fileExists(path))
				results.add(new InputSource(path, fileNameOf(path), null));
			else
				throw new NoInputsError(path + ": no such file or directory");
		}

		// String.compareTo is ordinal: it compares UTF-16 code units, as the
		// reference's string.CompareOrdinal does.
		if (options.sort)
			Collections.sort(results, (a, b) -> a.path.compareTo(b.path));

		if (results.isEmpty())
			throw new NoInputsError("no inputs matched; the walk and filters selected nothing");

		return results;
	}

	/**
	 * Reads a {@code --files-from} list. A path of "-" reads from stdin.
	 *
	 * <p>A list that cannot be read is not a {@link CliError}, in any port: the
	 * platform's own failure escapes, wrapped here only because Java will not let
	 * a checked exception through a method that does not declare it.
	 */
	public List<String> readFileList(String path)
	{
		List<String> lines;
		try
		{
			lines = path.equals("-") ? _console.readAllInputLines() : _fileSystem.readAllLines(path);
		}
		catch (IOException e)
		{
			throw new UncheckedIOException(e);
		}

		List<String> result = new ArrayList<>();
		for (String line : lines)
		{
			if (!ValueParsers.isNullOrWhiteSpace(line))
				result.add(line);
		}
		return result;
	}

	/** Walks one directory tree, honouring depth, hidden entries and symlinks. */
	public List<InputSource> walkDirectory(String root, InputOptions options)
	{
		List<InputSource> results = new ArrayList<>();
		_visited = new HashSet<>();
		walkInto(root, root, 1, options, results);
		return results;
	}

	/**
	 * The recursive worker behind {@link #walkDirectory}, accumulating into a
	 * caller-owned list.
	 *
	 * <p>Accumulating rather than returning a list per level is deliberate: the
	 * portability rules rule out {@code yield return}, because Java has no clean
	 * equivalent and C++17 has none at all, and merging a list at every level of
	 * the recursion is four different idioms in four languages. Passing one list
	 * down ports unchanged.
	 */
	private void walkInto(String root, String current, int depth, InputOptions options, List<InputSource> results)
	{
		// Outside the try below, as in the reference: a directory whose identity
		// cannot be read is not reported as unreadable, the platform's failure escapes.
		String identity;
		try
		{
			identity = _fileSystem.getDirectoryIdentity(current);
		}
		catch (IOException e)
		{
			throw new UncheckedIOException(e);
		}
		if (!_visited.add(identity))
			return;

		List<String> entries;
		try
		{
			entries = _fileSystem.listDirectory(current);
		}
		catch (CliError e)
		{
			throw e;
		}
		catch (IOException | RuntimeException e)
		{
			// A directory the walk cannot read is reported, not skipped. Skipping it
			// silently produces a shorter input list that looks like a complete one.
			//
			// The message is fixed rather than the platform's own: error text is part
			// of the cross-language contract, and every platform words it differently.
			throw new NoInputsError(current + ": could not be read");
		}

		for (String entry : entries)
		{
			if (!options.includeHidden && _fileSystem.isHidden(entry))
				continue;

			if (_fileSystem.directoryExists(entry))
			{
				if (_fileSystem.isSymbolicLink(entry) && !options.followSymlinks)
					continue;

				int nextDepth = depth + 1;
				if (!options.noRecurseWasGiven && (options.maxDepth == null || nextDepth <= options.maxDepth))
					walkInto(root, entry, nextDepth, options, results);
			}
			else
			{
				String relativePath = trimLeadingSlashes(entry.substring(root.length()));
				if (options.matcher != null && !options.matcher.matches(relativePath))
					continue;

				results.add(new InputSource(entry, relativePath, root));
			}
		}
	}

	private static String trimLeadingSlashes(String text)
	{
		int start = 0;
		while (start < text.length() && text.charAt(start) == '/')
			++start;
		return text.substring(start);
	}

	/**
	 * The filename portion of an explicitly named file argument.
	 *
	 * <p>Unlike a walked entry's {@link InputSource#relativePath} --- already
	 * forward-slash because {@link #walkInto} built it from
	 * {@link IFileSystem#listDirectory} output --- an explicit argument is whatever
	 * the user typed on their own shell, backslashes included on Windows.
	 * {@code OutputPlanner.directoryOf} normalises before splitting for exactly this
	 * reason; skipping it here would leave a bare {@code C:\tmp\a.pdf} unrecognised
	 * as having a directory at all, so {@code --out-dir} would nest the source drive
	 * underneath itself instead of flattening to the filename.
	 */
	private static String fileNameOf(String path)
	{
		String normalised = path.replace('\\', '/');
		int slash = normalised.lastIndexOf('/');
		return slash < 0 ? normalised : normalised.substring(slash + 1);
	}
}
