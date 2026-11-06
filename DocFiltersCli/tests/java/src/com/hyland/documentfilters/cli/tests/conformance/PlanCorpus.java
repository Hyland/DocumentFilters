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

import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import java.util.ArrayList;
import java.util.List;

/**
 * The plan corpus: {@link Fixture} plus the one thing specific to it, splitting a
 * subject line into command line arguments.
 */
public final class PlanCorpus
{
	public static final String FILE_NAME = "plan-corpus.txt";

	private PlanCorpus()
	{
	}

	/** Every case in the corpus. */
	public static List<FixtureCase> load()
	{
		return Fixture.load(FILE_NAME);
	}

	/** The expected exit code for a case, defaulting to success. */
	public static int expectedExit(FixtureCase testCase)
	{
		String value = testCase.get("exit");
		return value == null ? 0 : Integer.parseInt(value);
	}

	/**
	 * Builds the filesystem a case is given by its {@code <} lines.
	 *
	 * <p>Never the real one. A real directory listing has no defined order and differs
	 * between ext4, NTFS and APFS, so a corpus backed by real files could not be
	 * byte-identical across languages and platforms. The fake also models symlinks
	 * and hidden entries as plain sets, which is what lets --follow and
	 * --include-hidden be contract at all: creating a symlink on Windows needs a
	 * privilege most checkouts do not have, and "hidden" is a dot-prefix on POSIX and
	 * a file attribute on Windows.
	 */
	public static InMemoryFileSystem buildFileSystem(FixtureCase testCase)
	{
		InMemoryFileSystem fileSystem = new InMemoryFileSystem();
		for (String row : testCase.givenAll("tree"))
		{
			for (String path : splitPaths(row))
			{
				if (path.endsWith("/"))
					fileSystem.addDirectory(path.substring(0, path.length() - 1));
				else
					fileSystem.addFile(path);
			}
		}

		// Links after the tree, so a link may point at a directory the tree created.
		for (String row : testCase.givenAll("link"))
		{
			List<String> parts = splitPaths(row);
			if (parts.size() != 2)
				throw new FixtureFormatException(testCase.describe("'< link' needs '<path> <target>': " + row));
			fileSystem.markSymlink(parts.get(0), parts.get(1));
		}

		// Hidden last: it marks an entry the two above have already created.
		for (String row : testCase.givenAll("hidden"))
		{
			for (String path : splitPaths(row))
				fileSystem.markHidden(path);
		}
		return fileSystem;
	}

	/** Splits a {@code <} row on spaces and tabs, dropping empty entries. */
	static List<String> splitPaths(String row)
	{
		List<String> result = new ArrayList<>();
		for (String part : row.split("[ \t]+"))
		{
			if (!part.isEmpty())
				result.add(part);
		}
		return result;
	}

	/**
	 * Splits a corpus subject into arguments on whitespace, honouring double quotes so
	 * a value may contain spaces.
	 */
	public static String[] splitArguments(String commandLine)
	{
		List<String> args = new ArrayList<>();
		StringBuilder current = new StringBuilder();
		boolean quoted = false;
		for (int i = 0; i < commandLine.length(); i++)
		{
			char c = commandLine.charAt(i);
			if (c == '"')
			{
				quoted = !quoted;
			}
			else if (Character.isWhitespace(c) && !quoted)
			{
				if (current.length() > 0)
				{
					args.add(current.toString());
					current.setLength(0);
				}
			}
			else
			{
				current.append(c);
			}
		}
		if (current.length() > 0)
			args.add(current.toString());
		return args.toArray(new String[0]);
	}
}
