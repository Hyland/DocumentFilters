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

import com.hyland.documentfilters.cli.configuration.ExtractOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.ISubFileEntry;
import com.hyland.documentfilters.cli.engine.OpenType;
import com.hyland.documentfilters.cli.planning.OutputPlanner;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.OutputConflictError;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Walking into documents that contain other documents: archives, mail stores,
 * attachments, embedded images.
 *
 * <p>The depth limit here is {@code --subfile-depth}, which is entirely separate
 * from the directory walk's {@code --max-depth}. A .docx inside a .zip inside an
 * .msg is three subfile levels regardless of where the .msg sits on disk.
 */
public final class SubfileExtraction
{
	private static final DateTimeFormatter TIMESTAMP = DateTimeFormatter.ofPattern("uuuu-MM-dd'T'HH:mm:ss");

	/** What {@link #walk} does with each subfile it reaches. */
	@FunctionalInterface
	public interface SubfileVisitor
	{
		void visit(ISubFileEntry subfile, String displayName, int depth);
	}

	private final IFileSystem _fileSystem;

	/**
	 * Every output path this extractor has already resolved, for the whole run.
	 *
	 * <p>Extraction resolves its own paths --- the planner never sees them, because
	 * what a container holds is not known until it is open --- so the output policy
	 * has to be applied here or not at all.
	 *
	 * <p>On the extractor rather than on the call, because {@code extract} builds one
	 * of these and runs every input through it: two archives each holding a
	 * {@code readme.txt} flattened into one output directory must collide, with or
	 * without {@code --overwrite}, as the planner's own collision check does for
	 * ordinary outputs.
	 */
	private final Set<String> _claimed = new HashSet<>();

	/**
	 * Takes the filesystem because extraction creates the directories a subfile's
	 * name implies. The directory is ours to create; the file itself is written by
	 * {@link ISubFileEntry#copyTo} inside the engine.
	 */
	public SubfileExtraction(IFileSystem fileSystem)
	{
		_fileSystem = fileSystem;
	}

	/**
	 * Extracts every subfile to disk, for the {@code extract} command.
	 *
	 * <p><b>Every written path must be inside the output directory.</b> A subfile's
	 * name comes from the container, not from the user, so an archive can carry a
	 * name like {@code ../../outside.txt} or an absolute path and walk out of the
	 * destination. Names are put through {@link #resolveSafeOutputPath} before
	 * anything is created; no path reaches the filesystem any other way.
	 */
	public List<String> extractAll(IDocument container, String containerPath, SharedOptions shared, ExtractOptions options)
	{
		List<String> written = new ArrayList<>();
		String outputRoot = shared.output.outDir != null ? shared.output.outDir : ".";

		SubfileVisitor extractOne = (subfile, displayName, depth) ->
		{
			if (!passesSubfileFilters(subfile, options))
				return;

			String outputPath = resolveSafeOutputPath(outputRoot, subfile.getName(), options.replicateTree());
			if (options.appendSubfileId)
				outputPath = insertSubfileId(outputPath, subfile.getId());

			if (!_claimed.add(OutputPlanner.collisionKeyFor(outputPath, _fileSystem)))
			{
				throw new OutputConflictError(ErrorCodes.OUTPUT_COLLISION,
					"two subfiles would be extracted to the same output; use --sub-id to distinguish them");
			}

			OutputPlanner.verifyPageTarget(outputPath, shared, _fileSystem);
			verifyNoLinkedComponent(outputRoot, outputPath);

			String directory = OutputPlanner.directoryOf(outputPath);
			if (!directory.isEmpty())
			{
				try
				{
					_fileSystem.createDirectory(directory);
				}
				catch (IOException e)
				{
					throw new UncheckedIOException(e.getMessage(), e);
				}
			}

			subfile.copyTo(outputPath);
			written.add(outputPath);
		};

		walk(container, containerPath, 1, shared, extractOne);

		if (options.images)
		{
			for (ISubFileEntry image : container.getImages())
			{
				try (ISubFileEntry owned = image)
				{
					extractOne.visit(owned, composeDisplayName(containerPath, owned.getName()), 1);
				}
			}
		}

		return written;
	}

	/**
	 * Refuses an output path that reaches its destination through a symbolic link.
	 *
	 * <p>{@link #resolveSafeOutputPath} guarantees the path spells out as a plain
	 * name under the root, which is a statement about the string. Where it lands is
	 * a statement about the disk: if {@code out/latest} is a link to a web root, a
	 * container entry named {@code latest/index.html} is lexically contained and
	 * still overwrites a file outside the output directory entirely.
	 *
	 * <p>Only components below the root are examined. The root itself may well be a
	 * link --- {@code --out-dir} is the user's own instruction --- and so may
	 * anything above it.
	 *
	 * <p>This closes the hole rather than sealing it: a link created between this
	 * check and {@code copyTo} would still be followed.
	 */
	private void verifyNoLinkedComponent(String outputRoot, String outputPath)
	{
		String root = trimEndSlashes(outputRoot.replace('\\', '/'));
		String path = outputPath.replace('\\', '/');

		if (!path.startsWith(root + "/"))
			return;

		// The file itself is a component like any other: a link named report.txt is
		// written through just as a link named reports/ is.
		int cursor = root.length();
		while (cursor < path.length())
		{
			int slash = path.indexOf('/', cursor + 1);
			String component = slash < 0 ? path : path.substring(0, slash);

			boolean present = _fileSystem.directoryExists(component) || _fileSystem.fileExists(component);
			if (present && _fileSystem.isSymbolicLink(component))
			{
				throw new OutputConflictError(ErrorCodes.UNSAFE_OUTPUT,
					"a subfile would be written through '" + component + "', which is a link out of the output directory");
			}

			if (slash < 0)
				break;

			cursor = slash;
		}
	}

	/** Inserts a subfile's id before its extension, for {@code --sub-id}. */
	static String insertSubfileId(String path, String id)
	{
		int slash = path.lastIndexOf('/');
		String directory = slash < 0 ? "" : path.substring(0, slash + 1);
		String fileName = slash < 0 ? path : path.substring(slash + 1);

		int dot = fileName.lastIndexOf('.');
		String stem = dot < 0 ? fileName : fileName.substring(0, dot);
		String extension = dot < 0 ? "" : fileName.substring(dot);

		return directory + stem + "-" + (id != null ? id : "") + extension;
	}

	/**
	 * Turns a container-supplied subfile name into a path that is guaranteed to sit
	 * inside {@code outputRoot}.
	 *
	 * <p>The name is untrusted input. Traversal segments, absolute paths and drive
	 * letters are all neutralised; when {@code replicateTree} is false the name
	 * collapses to its final component.
	 *
	 * <p>This is the lexical half of the guarantee; the other half is
	 * {@link #verifyNoLinkedComponent}, because a name can be lexically contained and
	 * still land outside through a symlink already in the output directory.
	 */
	public static String resolveSafeOutputPath(String outputRoot, String subfileName, boolean replicateTree)
	{
		List<String> segments = safeSegments(subfileName, replicateTree);
		return trimEndSlashes(outputRoot) + "/" + String.join("/", segments);
	}

	/**
	 * Splits a subfile name into path components that are guaranteed not to traverse
	 * upward or escape via a drive letter or UNC share, never leaving the result
	 * empty.
	 *
	 * <p>Every {@code .} and {@code ..} component is dropped outright, so a name that
	 * is nothing but traversal falls back to a single placeholder segment. Only an
	 * exact match is dropped, so {@code ..data}, a legitimate filename, survives.
	 */
	private static List<String> safeSegments(String subfileName, boolean replicateTree)
	{
		String normalised = (subfileName != null ? subfileName : "").replace('\\', '/');

		// A drive letter ("C:...") names something outside any output directory;
		// dropping just the colon is enough, because the leading "C" and the rest of
		// the path pass through the same segment filter as everything else.
		if (normalised.length() >= 2 && normalised.charAt(1) == ':')
			normalised = normalised.substring(2);

		List<String> segments = new ArrayList<>();
		for (String segment : normalised.split("/", -1))
		{
			if (segment.isEmpty() || segment.equals(".") || segment.equals(".."))
				continue;

			segments.add(segment);
		}

		if (!replicateTree)
		{
			String last = segments.isEmpty() ? null : segments.get(segments.size() - 1);
			segments.clear();
			if (last != null)
				segments.add(last);
		}

		if (segments.isEmpty())
			segments.add("_");

		return segments;
	}

	/** Lists subfiles without extracting, for the {@code list} command. */
	public List<SubfileEntry> listAll(IDocument container, String containerPath, SharedOptions shared)
	{
		List<SubfileEntry> entries = new ArrayList<>();
		walk(container, containerPath, 1, shared, (subfile, displayName, depth) ->
		{
			SubfileEntry entry = new SubfileEntry();
			entry.id = subfile.getId();
			entry.name = subfile.getName();
			entry.displayName = displayName;
			entry.size = subfile.getSize();
			entry.modified = formatTimestamp(subfile.getModified());
			entry.depth = depth;
			entries.add(entry);
		});
		return entries;
	}

	/**
	 * Invokes {@code action} for every subfile, depth-first, honouring
	 * {@code --subfile-depth}. Shared by {@code convert}, {@code extract},
	 * {@code list}, {@code identify} and {@code meta} when {@code -s} is given.
	 *
	 * <p>{@code container} must already be open --- recursion opens each subfile
	 * itself, since a subfile's own subfiles are unavailable until it is.
	 */
	public void walk(IDocument container, String containerPath, int depth, SharedOptions shared, SubfileVisitor action)
	{
		for (ISubFileEntry subfile : container.getSubFiles())
		{
			try (ISubFileEntry owned = subfile)
			{
				String displayName = composeDisplayName(containerPath, owned.getName());
				action.visit(owned, displayName, depth);

				boolean canRecurse = shared.subfiles.enabled
					&& (shared.subfiles.maxDepth == null || depth < shared.subfiles.maxDepth);

				if (canRecurse)
				{
					// Opening the entry yields a document sharing its handle, so the
					// entry's own try block is what closes it.
					IDocument opened = owned.open(OpenType.BODY_ONLY, "");
					walk(opened, displayName, depth + 1, shared, action);
				}
			}
		}
	}

	/**
	 * Renders a subfile timestamp in the ISO 8601 form the console contract uses, or
	 * null when the container records none.
	 *
	 * <p>Kept as a pure function of the nullable value so the boundary --- a
	 * container with no timestamp versus one dated at the epoch --- is testable
	 * without an archive.
	 */
	public static String formatTimestamp(LocalDateTime modified)
	{
		if (modified == null || modified.getYear() <= 1970)
			return null;

		return modified.format(TIMESTAMP) + "Z";
	}

	/** Builds the display name for a subfile: {@code container.zip>inner.docx}. */
	public static String composeDisplayName(String containerPath, String subfileName)
	{
		return containerPath + ">" + (subfileName != null ? subfileName : "");
	}

	/**
	 * Applies {@code extract}'s {@code --include} and {@code --exclude} globs to a
	 * subfile's name.
	 *
	 * <p>Both are already compiled; a malformed pattern fails when the argument is
	 * read, not here with an archive half-extracted. Matching on the name lets a
	 * subfile be rejected without being opened, which matters for a large archive.
	 */
	static boolean passesSubfileFilters(ISubFileEntry subfile, ExtractOptions options)
	{
		if (options.subfileInclude != null && !options.subfileInclude.matches(subfile.getName()))
			return false;

		if (options.subfileExclude != null && options.subfileExclude.matches(subfile.getName()))
			return false;

		return true;
	}

	private static String trimEndSlashes(String text)
	{
		int end = text.length();
		while (end > 0 && text.charAt(end - 1) == '/')
			--end;
		return text.substring(0, end);
	}
}
