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

package com.hyland.documentfilters.cli.tests.fakes;

import com.hyland.documentfilters.cli.runtime.IFileSystem;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.AccessDeniedException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * A filesystem that lives entirely in a map, so the directory walk and the output
 * planner can be tested without touching disk.
 *
 * <p>Paths are normalised to forward slashes throughout, which is also how the
 * real implementation matches globs, so a test written here means the same thing
 * on Windows and Linux.
 *
 * <p>This is test infrastructure, so unlike the project under test it is fully
 * implemented.
 */
public final class InMemoryFileSystem implements IFileSystem
{
	private final Map<String, byte[]> _files = new HashMap<>();
	private final Set<String> _directories = new HashSet<>();
	private final Set<String> _hidden = new HashSet<>();
	private final Set<String> _symlinks = new HashSet<>();
	private final Map<String, String> _linkTargets = new HashMap<>();

	/**
	 * Names this filesystem matches to one file, beyond matching them exactly.
	 *
	 * <p>Stated case by case rather than folded by a rule, because the rule is the
	 * thing under test. A real volume decides for itself which spellings are one
	 * file, and the tool's own collision key cannot always predict it.
	 */
	private final Map<String, String> _sameFile = new HashMap<>();

	/**
	 * What this filesystem claims about case --- true unless a test says otherwise,
	 * because this fake stores paths in a plain map and so behaves that way.
	 */
	public boolean caseSensitive = true;

	/** Directories this filesystem refuses to list, as a real one may. */
	public final Set<String> unreadable = new HashSet<>();

	/** Everything written through {@link #createFile}, for assertions. */
	public final List<String> created = new ArrayList<>();

	/** Declares that two names reach one file. */
	public InMemoryFileSystem treatAsSameFile(String path, String sameAs)
	{
		_sameFile.put(normalise(path), normalise(sameAs));
		return this;
	}

	/** The one name a file is stored under, whatever spelling was asked for. */
	private String key(String path)
	{
		String normalised = normalise(path);
		String same = _sameFile.get(normalised);
		return same != null ? same : normalised;
	}

	public InMemoryFileSystem addFile(String path)
	{
		return addFile(path, "");
	}

	/** Adds a file, creating its parent directories. */
	public InMemoryFileSystem addFile(String path, String contents)
	{
		String key = key(path);
		_files.put(key, contents.getBytes(StandardCharsets.UTF_8));
		addParents(key);
		return this;
	}

	/** Adds an empty directory. */
	public InMemoryFileSystem addDirectory(String path)
	{
		String key = normalise(path);
		_directories.add(key);
		addParents(key);
		return this;
	}

	/** Marks an existing entry hidden, so the walk skips it without {@code --include-hidden}. */
	public InMemoryFileSystem markHidden(String path)
	{
		_hidden.add(normalise(path));
		return this;
	}

	public InMemoryFileSystem markSymlink(String path)
	{
		return markSymlink(path, null);
	}

	/**
	 * Marks a directory as a symlink, so the walk enters it only under {@code -L}.
	 *
	 * @param target what it resolves to. Point this at an ancestor to build the
	 *        cycle that {@code --follow} has to survive; leave it null when the
	 *        link's destination does not matter to the test.
	 */
	public InMemoryFileSystem markSymlink(String path, String target)
	{
		String key = normalise(path);
		_symlinks.add(key);

		// The link is a real directory entry, so a walk of its parent finds it.
		// Without this the walk never sees the link at all and any test about
		// following one passes without exercising anything.
		_directories.add(key);
		addParents(key);

		if (target != null)
			_linkTargets.put(key, normalise(target));

		return this;
	}

	@Override
	public boolean fileExists(String path)
	{
		return _files.containsKey(key(path));
	}

	@Override
	public boolean directoryExists(String path)
	{
		return _directories.contains(normalise(path));
	}

	@Override
	public List<String> listDirectory(String path) throws IOException
	{
		String requested = normalise(path);

		// The same shape of failure a real filesystem raises for a directory the
		// process may not read: an exception, not an empty listing.
		if (unreadable.contains(requested))
			throw new AccessDeniedException(requested);

		// Listing through a link yields the target's children, re-rooted under the
		// path that was asked for --- which is what makes a link to an ancestor
		// generate the endless corpus/loop/loop/... that --follow has to survive.
		String real = resolveThroughLinks(requested);
		if (!real.equals(requested))
		{
			List<String> rerooted = new ArrayList<>();
			for (String child : listDirectory(real))
				rerooted.add(requested + child.substring(real.length()));
			return rerooted;
		}

		String prefix = requested;
		if (!prefix.isEmpty() && !prefix.endsWith("/"))
			prefix += "/";

		Set<String> children = new TreeSet<>(Collections.reverseOrder());
		List<String> candidates = new ArrayList<>(_files.keySet());
		candidates.addAll(_directories);
		for (String candidate : candidates)
		{
			if (!candidate.startsWith(prefix) || candidate.length() == prefix.length())
				continue;

			String rest = candidate.substring(prefix.length());
			int slash = rest.indexOf('/');
			children.add(prefix + (slash < 0 ? rest : rest.substring(0, slash)));
		}

		// Deliberately reverse-sorted: IFileSystem promises no order and a real
		// directory listing gives none. A fake that sorted would let a walk ignoring
		// --sort pass the sorting tests.
		return new ArrayList<>(children);
	}

	@Override
	public boolean isHidden(String path)
	{
		return _hidden.contains(normalise(path));
	}

	@Override
	public boolean isSymbolicLink(String path)
	{
		return _symlinks.contains(normalise(path));
	}

	/**
	 * The directory a path resolves to, following any link. Two paths reaching the
	 * same directory return the same identity, which is what lets the walk detect a
	 * cycle.
	 */
	@Override
	public String getDirectoryIdentity(String path)
	{
		return resolveThroughLinks(normalise(path));
	}

	@Override
	public boolean isCaseSensitive(String path)
	{
		return caseSensitive;
	}

	/**
	 * Rewrites a path through however many links stand in it, so that every route
	 * to one directory produces the same answer. Bounded, because a link to its own
	 * ancestor never settles --- stopping it is the walk's job, not the filesystem's.
	 */
	private String resolveThroughLinks(String path)
	{
		for (int i = 0; i < 64; i++)
		{
			String rewritten = rewriteLongestLinkPrefix(path);
			if (rewritten.equals(path))
				return path;

			path = rewritten;
		}
		return path;
	}

	private String rewriteLongestLinkPrefix(String path)
	{
		String bestLink = null;
		for (String link : _linkTargets.keySet())
		{
			boolean matches = path.equals(link) || path.startsWith(link + "/");
			if (matches && (bestLink == null || link.length() > bestLink.length()))
				bestLink = link;
		}

		if (bestLink == null)
			return path;

		return _linkTargets.get(bestLink) + path.substring(bestLink.length());
	}

	@Override
	public InputStream openRead(String path) throws IOException
	{
		String key = key(path);
		byte[] contents = _files.get(key);
		if (contents == null)
			throw new NoSuchFileException(key, null, "No such file in the fake filesystem");

		return new ByteArrayInputStream(contents);
	}

	@Override
	public OutputStream createFile(String path)
	{
		String key = key(path);
		created.add(key);
		addParents(key);
		_files.put(key, new byte[0]);
		return new CommittingStream(key);
	}

	/** Reads back what was written to a created file. */
	public String readText(String path)
	{
		return new String(readBytes(path), StandardCharsets.UTF_8);
	}

	/** Reads back the bytes of a file, or null when there is none. */
	public byte[] readBytes(String path)
	{
		byte[] contents = _files.get(key(path));
		return contents == null ? null : contents.clone();
	}

	@Override
	public void createDirectory(String path)
	{
		addDirectory(path);
	}

	@Override
	public void moveFile(String source, String destination) throws IOException
	{
		String from = key(source);
		String to = key(destination);

		byte[] contents = _files.remove(from);
		if (contents == null)
			throw new NoSuchFileException(from);

		_files.put(to, contents);
		created.add(to);
	}

	@Override
	public List<String> readAllLines(String path) throws IOException
	{
		String text;
		try (InputStream stream = openRead(path))
		{
			ByteArrayOutputStream buffer = new ByteArrayOutputStream();
			byte[] chunk = new byte[4096];
			for (int read; (read = stream.read(chunk)) > 0;)
				buffer.write(chunk, 0, read);
			text = new String(buffer.toByteArray(), StandardCharsets.UTF_8);
		}

		List<String> lines = new ArrayList<>(Arrays.asList(text.replace("\r\n", "\n").replace('\r', '\n').split("\n", -1)));
		if (!lines.isEmpty() && lines.get(lines.size() - 1).isEmpty())
			lines.remove(lines.size() - 1);
		return lines;
	}

	private void addParents(String path)
	{
		int slash = path.lastIndexOf('/');
		while (slash > 0)
		{
			path = path.substring(0, slash);
			_directories.add(path);
			slash = path.lastIndexOf('/');
		}
	}

	private static String normalise(String path)
	{
		String normalised = path.replace('\\', '/');
		int end = normalised.length();
		while (end > 0 && normalised.charAt(end - 1) == '/')
			--end;
		return normalised.substring(0, end);
	}

	/**
	 * A stream that writes its contents back into the fake when closed. Without it
	 * everything written would be discarded, and conversion output could never be
	 * asserted on.
	 */
	private final class CommittingStream extends ByteArrayOutputStream
	{
		private final String _path;
		private boolean _closed;

		CommittingStream(String path)
		{
			_path = path;
		}

		@Override
		public void close()
		{
			if (_closed)
				return;

			_closed = true;
			_files.put(_path, toByteArray());
		}
	}
}
