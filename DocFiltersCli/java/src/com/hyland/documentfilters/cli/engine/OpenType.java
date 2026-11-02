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

package com.hyland.documentfilters.cli.engine;

/**
 * The flags a document is opened with: one content selector, plus at most one
 * {@code FORMAT_*} value.
 *
 * <p>Declared here rather than taken from the binding, which publishes these only
 * as {@code int} constants. Naming them at the seam keeps everything above it
 * compilable and testable with no binding on the class path.
 *
 * <p>The {@code FORMAT_*} values are an enumeration packed into bits 16 to 19, not
 * independent bits, so at most one may ever be set. Only the members this tool uses
 * are declared: a partial copy of a constant table is worse than a minimal one.
 * Constants rather than an enum because the values are combined with {@code |}.
 */
public final class OpenType
{
	private OpenType()
	{
	}

	/** Document text without metadata. */
	public static final int BODY_ONLY = 0;

	/** Metadata alone, which is what the meta command opens with. */
	public static final int META_ONLY = 1;

	/** Both, and the default every other command wants. */
	public static final int BODY_AND_META = 2;

	/** The plain-text reader, which strips the ISYS control codes. */
	public static final int FORMAT_TEXT = 0x00010000;

	/** The HTML reader. */
	public static final int FORMAT_HTML = 0x00020000;

	/** The paginated reader. This is what makes pages exist, so every canvas format needs it. */
	public static final int FORMAT_IMAGE = 0x00050000;

	/** The Markdown reader, for md-text. */
	public static final int FORMAT_MARKDOWN = 0x00080000;
}
