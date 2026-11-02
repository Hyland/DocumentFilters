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

package com.hyland.documentfilters.cli.planning;

/**
 * One input and everything decided about it.
 *
 * <p>Exactly one of {@link #target} and {@link #targetPattern} is set --- or
 * neither, when the item contributes to {@link RunPlan#combinedTarget}.
 *
 * <p><b>Every per-page output is deferred</b>, including when {@code --pages} was
 * given. An explicit range does not make the filenames knowable: nothing at this
 * stage has opened the document, so whether every requested page actually exists
 * is still unknown --- {@code --pages 1-2} against a document that turns out to
 * have only one page is discovered during execution, once the real page count can
 * be trusted, not guessed at here. Resolving both up front would also let the
 * planner reject a run because a page-2 output already exists when page 2 was
 * never going to be written.
 *
 * <p>Execution is the only point at which the pages that actually exist are known,
 * so that is where the names are produced.
 */
public final class PlannedItem
{
	/** The path as it will be opened. */
	public String sourcePath;

	/** How the input is named in messages; for a subfile this includes the container. */
	public String displayName;

	/**
	 * The one output path, for a single-file format. Null for a per-page format,
	 * and null when this item feeds {@link RunPlan#combinedTarget}.
	 */
	public String target;

	/** The unresolved output naming, for a per-page format. Null when {@link #target} is set. */
	public TargetPattern targetPattern;

	/** The pages to render, or {@link PageRange#ALL}. */
	public PageRange pages;

	/** True when this item's output names are produced during execution. */
	public boolean isPageCountDependent()
	{
		return targetPattern != null;
	}

	/**
	 * The directory this item's output lands in, or null when it has no file
	 * target --- {@code --stdout}, or an item feeding {@link RunPlan#combinedTarget}.
	 *
	 * <p>The plan already knows this for both target shapes, which is what lets the
	 * directory be created once per item instead of at each of the three places
	 * that open an output. That matters because only one of those three is ours:
	 * the text path writes through {@code IFileSystem}, but a canvas and a docpack
	 * are opened by the engine, which will not create a directory and reports a
	 * missing one as a per-document failure that reads like the input was missing.
	 *
	 * <p>The directory is everything before the last separator, either slash, or
	 * empty when there is none --- spelled out because {@code java.io.File.getParent}
	 * answers null for a bare name and knows only the host's separators.
	 */
	public String outputDirectory()
	{
		if (targetPattern != null)
			return targetPattern.directory;
		if (target == null || target.isEmpty())
			return null;
		String normalised = target.replace('\\', '/');
		int slash = normalised.lastIndexOf('/');
		return slash < 0 ? "" : normalised.substring(0, slash);
	}
}
