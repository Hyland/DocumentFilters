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

package com.hyland.documentfilters.cli.runtime;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.LinkOption;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.nio.file.attribute.DosFileAttributes;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * The real filesystem.
 *
 * <p>Every path handed back uses forward slashes, whatever the host, so the
 * planner and the reports see one spelling.
 */
public final class SystemFileSystem implements IFileSystem
{
	private static final boolean HOST_IS_CASE_SENSITIVE = hostIsCaseSensitive();

	private final Map<String, Boolean> _caseSensitivity = new HashMap<>();

	@Override
	public boolean fileExists(String path)
	{
		Path resolved = toPath(path);
		return resolved != null && Files.isRegularFile(resolved);
	}

	@Override
	public boolean directoryExists(String path)
	{
		Path resolved = toPath(path);
		return resolved != null && Files.isDirectory(resolved);
	}

	@Override
	public List<String> listDirectory(String path) throws IOException
	{
		List<String> entries = new ArrayList<>();
		try (DirectoryStream<Path> stream = Files.newDirectoryStream(Paths.get(path)))
		{
			for (Path entry : stream)
				entries.add(normalise(entry.toString()));
		}
		return entries;
	}

	@Override
	public boolean isHidden(String path)
	{
		String trimmed = path;
		while (trimmed.endsWith("/"))
			trimmed = trimmed.substring(0, trimmed.length() - 1);
		int slash = Math.max(trimmed.lastIndexOf('/'), trimmed.lastIndexOf('\\'));
		String name = slash < 0 ? trimmed : trimmed.substring(slash + 1);
		if (name.startsWith("."))
			return true;

		try
		{
			return Files.readAttributes(Paths.get(path), DosFileAttributes.class, LinkOption.NOFOLLOW_LINKS).isHidden();
		}
		catch (IOException | UnsupportedOperationException | InvalidPathException e)
		{
			// No DOS attributes on this filesystem: only the dot counts.
			return false;
		}
	}

	@Override
	public boolean isSymbolicLink(String path)
	{
		// False for a path that does not exist, because extraction asks about every
		// component of a file it is about to create. A dangling link still answers
		// true: its attributes are its own, and writing through it would create its
		// target.
		Path resolved = toPath(path);
		return resolved != null && Files.isSymbolicLink(resolved);
	}

	@Override
	public String getDirectoryIdentity(String path) throws IOException
	{
		Path resolved;
		try
		{
			resolved = Paths.get(path).toRealPath();
		}
		catch (NoSuchFileException e)
		{
			resolved = Paths.get(path).toAbsolutePath().normalize();
		}
		return trimTrailingSlashes(normalise(resolved.toString()));
	}

	@Override
	public boolean isCaseSensitive(String path)
	{
		Path directory = nearestExistingDirectory(path);
		if (directory == null)
			return HOST_IS_CASE_SENSITIVE;

		String key = directory.toString();
		Boolean cached = _caseSensitivity.get(key);
		if (cached != null)
			return cached;

		Boolean measured = probe(directory);
		boolean sensitive = measured != null ? measured : HOST_IS_CASE_SENSITIVE;
		_caseSensitivity.put(key, sensitive);
		return sensitive;
	}

	@Override
	public InputStream openRead(String path) throws IOException
	{
		return Files.newInputStream(Paths.get(path));
	}

	@Override
	public OutputStream createFile(String path) throws IOException
	{
		Path target = Paths.get(path);
		Path parent = target.getParent();
		if (parent != null)
			Files.createDirectories(parent);

		return Files.newOutputStream(target);
	}

	@Override
	public void createDirectory(String path) throws IOException
	{
		Files.createDirectories(Paths.get(path));
	}

	@Override
	public void moveFile(String from, String to) throws IOException
	{
		Files.move(Paths.get(from), Paths.get(to), StandardCopyOption.REPLACE_EXISTING);
	}

	/**
	 * Every line of a text file.
	 *
	 * <p>UTF-8, with a leading byte-order mark dropped and undecodable bytes read as
	 * U+FFFD rather than failing, as the reference's {@code File.ReadAllLines} does;
	 * lines end at "\r\n", "\n" or a lone "\r", as there.
	 */
	@Override
	public List<String> readAllLines(String path) throws IOException
	{
		List<String> lines = new ArrayList<>();
		InputStreamReader decoder = new InputStreamReader(Files.newInputStream(Paths.get(path)),
			StandardCharsets.UTF_8.newDecoder()
				.onMalformedInput(CodingErrorAction.REPLACE)
				.onUnmappableCharacter(CodingErrorAction.REPLACE));
		try (BufferedReader reader = new BufferedReader(decoder))
		{
			String line;
			while ((line = reader.readLine()) != null)
			{
				if (lines.isEmpty() && line.startsWith("\uFEFF"))
					line = line.substring(1);
				lines.add(line);
			}
		}
		return lines;
	}

	private static Path toPath(String path)
	{
		try
		{
			return Paths.get(path);
		}
		catch (InvalidPathException e)
		{
			// A name the host cannot spell is a name that is not there.
			return null;
		}
	}

	private static Path nearestExistingDirectory(String path)
	{
		Path candidate;
		try
		{
			candidate = Paths.get(path).toAbsolutePath().normalize().getParent();
		}
		catch (InvalidPathException e)
		{
			return null;
		}

		while (candidate != null)
		{
			if (Files.isDirectory(candidate))
				return candidate;

			candidate = candidate.getParent();
		}

		return null;
	}

	private static Boolean probe(Path directory)
	{
		try (DirectoryStream<Path> stream = Files.newDirectoryStream(directory))
		{
			Iterator<Path> entries = stream.iterator();
			Iterable<String> names = () -> new Iterator<String>()
			{
				@Override
				public boolean hasNext()
				{
					return entries.hasNext();
				}

				@Override
				public String next()
				{
					return entries.next().toString();
				}
			};

			return CaseProbe.measure(normalise(directory.toString()), names, candidate ->
			{
				Path probed = toPath(candidate);
				return probed != null && Files.exists(probed);
			});
		}
		catch (IOException | RuntimeException e)
		{
			// Unreadable: the host default answers instead.
			return null;
		}
	}

	private static boolean hostIsCaseSensitive()
	{
		String os = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
		return !os.startsWith("windows") && !os.startsWith("mac");
	}

	private static String normalise(String path)
	{
		return path.replace('\\', '/');
	}

	private static String trimTrailingSlashes(String path)
	{
		int end = path.length();
		while (end > 0 && path.charAt(end - 1) == '/')
			--end;
		return path.substring(0, end);
	}
}
