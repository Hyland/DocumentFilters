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
 * A directory plus a filename template, for outputs whose count depends on the
 * document's page count.
 */
public final class TargetPattern
{
	/** The directory the files land in, already including any replicated source tree. */
	public String directory;

	/** The template applied per page. */
	public OutputNameTemplate template;

	/** The output format's extension, for {@code {ext}}. */
	public String extension;

	public TargetPattern()
	{
	}

	public TargetPattern(String directory, OutputNameTemplate template, String extension)
	{
		this.directory = directory;
		this.template = template;
		this.extension = extension;
	}

	/** Produces the concrete path for one page, during execution. */
	public String resolve(String sourcePath, int pageNumber)
	{
		String fileName = template.render(sourcePath, extension, pageNumber);
		return directory == null || directory.isEmpty() ? fileName : directory + "/" + fileName;
	}

	/**
	 * The part of the path that does not vary with the page number.
	 *
	 * <p>Two inputs whose keys match collide on <em>every</em> page, so this catches
	 * the case that actually bites --- a flattened walk containing two files with
	 * the same stem --- without knowing either page count.
	 */
	public String collisionKey(String sourcePath)
	{
		// Any fixed page number identifies the non-page part of the name: if two
		// items differ only by page, they render identically here and correctly
		// collide; if they differ elsewhere, they differ here too.
		return resolve(sourcePath, 0);
	}

	/**
	 * The path of the first page that would be written, for the fail-fast check
	 * before any work starts.
	 *
	 * <p>The first <em>selected</em> page, not page 1: with {@code --pages 5-6} the
	 * first output is page 5, read straight off the explicit list --- no document
	 * is open yet at this point, so there is no page count to check it against.
	 * If the document turns out to have fewer pages than that, nothing is
	 * written and this check was moot --- a rare false alarm that costs nothing,
	 * because execution re-checks every page before writing it. This is a
	 * fail-fast convenience for the common case of re-running into a directory
	 * that already holds output, not the guarantee.
	 */
	public String firstTarget(String sourcePath, PageRange pages)
	{
		return resolve(sourcePath, pages.firstPage());
	}
}
