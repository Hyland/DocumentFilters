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

package com.hyland.documentfilters.cli.configuration;

import com.hyland.documentfilters.cli.planning.NameMatcher;
import java.util.ArrayList;
import java.util.List;

/** Options that only {@code extract} accepts. */
public final class ExtractOptions
{
	/** {@code -i, --images}: pull embedded images too, not just subfiles. */
	public boolean images;

	/**
	 * {@code --include}, repeatable: keep subfiles whose name matches.
	 *
	 * <p>Globs, the same pattern language as {@code -g/--glob} --- and repeatable for the
	 * same reason, so that one glob per idea reads better than one glob doing
	 * several jobs.
	 *
	 * <p><b>A different axis from {@code -g/--glob}.</b> That one chooses which files
	 * on disk to process; this one chooses which subfiles inside a container to
	 * extract --- the same distinction {@code --max-depth} and {@code --subfile-depth}
	 * draw between outside the document and inside it.
	 */
	public final List<String> includePatterns = new ArrayList<>();

	/** {@code --exclude}, repeatable: drop subfiles whose name matches. */
	public final List<String> excludePatterns = new ArrayList<>();

	/**
	 * {@link #includePatterns} compiled, or null when none were given.
	 *
	 * <p>Built by {@link OptionsValidator}, so a malformed pattern is reported
	 * before any archive is opened rather than with one half-extracted.
	 */
	public NameMatcher subfileInclude;

	/** {@link #excludePatterns} compiled, or null when none were given. */
	public NameMatcher subfileExclude;

	/**
	 * {@code --make-dirs}: replicate the source tree under the output directory.
	 *
	 * <p>Recorded separately from {@link #flatWasGiven} for the same reason
	 * {@code --max-depth} and {@code --no-recurse} are kept apart: collapsing two
	 * opposing switches into one boolean would make {@code --make-dirs --flat}
	 * indistinguishable from {@code --flat} alone, so the contradiction could not
	 * be reported.
	 */
	public boolean makeDirsWasGiven;

	/** {@code --flat}: put every extracted file straight into the output directory. */
	public boolean flatWasGiven;

	/** {@code --sub-id}: append the subfile id to extracted filenames. */
	public boolean appendSubfileId;

	/**
	 * Whether the source tree is replicated. Defaults to true, so {@code extract}
	 * lays out its output the way {@code convert} does.
	 */
	public boolean replicateTree()
	{
		return !flatWasGiven;
	}
}
