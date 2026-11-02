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

/** Which inputs to process, and how to find them. */
public final class InputOptions
{
	/** Positional paths: files or directories, in the order given. */
	public final List<String> paths = new ArrayList<>();

	/** {@code --files-from <file>}; "-" means read from stdin. */
	public String filesFrom;

	/**
	 * {@code --max-depth}. Null means it was not given; directories are walked
	 * recursively by default.
	 *
	 * <p>{@code --no-recurse} deliberately does <em>not</em> write this field. Two
	 * arguments sharing one field destroys the information validation needs:
	 * if {@code --no-recurse} wrote 1 here, then {@code --max-depth 5 --no-recurse}
	 * would be indistinguishable from a plain {@code --max-depth 1}, and the
	 * contradiction could never be reported. Each argument records its own
	 * presence; {@link #effectiveMaxDepth} derives the answer.
	 */
	public Integer maxDepth;

	/** {@code --no-recurse}, which means a depth of 1. */
	public boolean noRecurseWasGiven;

	/** {@code -L, --follow}: enter symlinked directories. */
	public boolean followSymlinks;

	/** {@code --sort}: order entries before processing, for reproducible runs. */
	public boolean sort;

	/** {@code --include-hidden}. */
	public boolean includeHidden;

	/**
	 * {@code -g/--glob}, repeatable. A leading {@code !} excludes.
	 *
	 * <p>Kept apart from {@link #regexPatterns} rather than both compiling into
	 * {@link #matcher} directly: two arguments writing one field would let
	 * {@code -g "**}{@code /*.pdf" --regex ".*\.docx"} silently drop one of the two, and
	 * the validator could not see that both were given.
	 */
	public final List<String> globPatterns = new ArrayList<>();

	/** {@code --regex}, repeatable. */
	public final List<String> regexPatterns = new ArrayList<>();

	/**
	 * The compiled path selection, built from whichever of the two pattern lists
	 * was given. Null means everything matches.
	 *
	 * <p>Populated by {@code CommandLineApp.bind}. {@link OptionsValidator}
	 * is where the two pattern languages are rejected if both appear, but it does
	 * not build this itself. Reading it before binding gives null.
	 */
	public NameMatcher matcher;

	/** {@code --include-types}: format ids to keep. Empty means no restriction. */
	public final List<String> includeTypes = new ArrayList<>();

	/** {@code --exclude-types}: format ids to drop. */
	public final List<String> excludeTypes = new ArrayList<>();

	/**
	 * The depth actually used by the walk, once {@link #maxDepth} and
	 * {@link #noRecurseWasGiven} are reconciled. Null means unlimited.
	 */
	public Integer effectiveMaxDepth()
	{
		return noRecurseWasGiven ? Integer.valueOf(1) : maxDepth;
	}
}
