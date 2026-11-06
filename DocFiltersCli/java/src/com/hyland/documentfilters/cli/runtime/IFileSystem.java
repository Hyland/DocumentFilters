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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;

/**
 * The filesystem operations this tool needs, behind one seam so the directory
 * walk and the output planner can be unit tested entirely in memory.
 *
 * <p>The questions --- does it exist, is it hidden, is it a link --- answer false
 * for a path they cannot read, as {@code File.Exists} does in the reference. The
 * operations that read or write throw {@link IOException}.
 */
public interface IFileSystem
{
	boolean fileExists(String path);

	boolean directoryExists(String path);

	/** Immediate children of a directory, files and directories alike, unsorted. */
	List<String> listDirectory(String path) throws IOException;

	/** True for entries the walk skips unless {@code --include-hidden} is given. */
	boolean isHidden(String path);

	/** True for symlinked directories, which the walk enters only under {@code -L/--follow}. */
	boolean isSymbolicLink(String path);

	/**
	 * A stable identity for a directory, with links resolved --- two paths that
	 * reach the same directory return the same value.
	 *
	 * <p>Needed to make {@code -L/--follow} terminate. Knowing that an entry is a
	 * link is not enough: a link pointing at one of its own ancestors produces an
	 * endless supply of distinct path strings, so a set of visited <em>paths</em>
	 * never detects the cycle. A set of visited identities does.
	 */
	String getDirectoryIdentity(String path) throws IOException;

	/**
	 * Whether the filesystem holding {@code path} tells two names apart by case
	 * --- true where {@code Report.txt} and {@code report.txt} are two files.
	 *
	 * <p>Asked of the filesystem rather than derived from the host, for the same
	 * reason {@link #getDirectoryIdentity} is: the planner cannot work it out and
	 * guessing is wrong often enough to matter. An {@code --out-dir} may sit on a
	 * mounted NTFS or exFAT volume under Linux, or on a case-sensitive volume under
	 * macOS, and the answer decides whether two planned outputs are one file.
	 *
	 * <p>{@code path} need not exist: the question is about the volume it would
	 * live on, which is why the collision check can ask before anything is written
	 * and {@code --dry-run} can ask without touching the disk at all.
	 */
	boolean isCaseSensitive(String path);

	InputStream openRead(String path) throws IOException;

	/** Creates or truncates a file, creating parent directories as needed. */
	OutputStream createFile(String path) throws IOException;

	void createDirectory(String path) throws IOException;

	/**
	 * Renames {@code from} to {@code to}, replacing an existing file at the
	 * destination.
	 *
	 * <p>For {@code web-safe}, which is the one format whose extension is not
	 * knowable until the page has been rendered: the engine picks PNG, GIF, JPEG or
	 * WebP per page, and the file is named for what it turned out to be. Whether
	 * replacing is allowed at all is settled before this is called, by the same
	 * output policy every other target goes through --- so by the time it runs, the
	 * destination is one the run is entitled to write.
	 */
	void moveFile(String from, String to) throws IOException;

	/** Reads a {@code --files-from} list, one path per line. */
	List<String> readAllLines(String path) throws IOException;
}
