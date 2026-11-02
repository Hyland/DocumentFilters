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
 * Everything the planner needs to know about one {@code --to} value.
 *
 * <p>The two properties that drive the output model are {@link #isSingleFile}
 * and {@link #isHiDefCapable}. Both are static properties of the format, so
 * {@code -o} never has to guess and never depends on the document.
 */
public final class FormatSpec
{
	private final String _token;
	private final FormatKind _kind;
	private final CanvasType _canvasType;
	private final String _canvasOptions;
	private final boolean _isSingleFile;
	private final boolean _supportsCombine;
	private final boolean _isHiDefCapable;
	private final String _extension;
	private final String _displayName;

	public FormatSpec(String token, FormatKind kind, CanvasType canvasType, String canvasOptions, boolean isSingleFile,
		boolean supportsCombine, boolean isHiDefCapable, String extension, String displayName)
	{
		_token = token;
		_kind = kind;
		_canvasType = canvasType;
		_canvasOptions = canvasOptions == null ? "" : canvasOptions;
		_isSingleFile = isSingleFile;
		_supportsCombine = supportsCombine;
		_isHiDefCapable = isHiDefCapable;
		_extension = extension;
		_displayName = displayName;
	}

	/** The token the user writes, e.g. {@code pdf}. Always lower case. */
	public String getToken()
	{
		return _token;
	}

	/** Which engine path renders it. */
	public FormatKind getKind()
	{
		return _kind;
	}

	/**
	 * The canvas device, when {@link #getKind} is {@link FormatKind#CANVAS}. Null
	 * for text and docpack, which use no canvas --- not {@link CanvasType#PNG},
	 * which is device zero and a real one.
	 */
	public CanvasType getCanvasType()
	{
		return _canvasType;
	}

	/**
	 * Canvas options the format itself contributes, appended after whatever
	 * {@code --canvas-options} supplies. Empty for most.
	 *
	 * <p>These are defaults, not identity. The engine resolves a repeated key to its
	 * first occurrence, so appending them leaves {@code --canvas-options} in front
	 * and the user wins every key they name --- which is why there is one property
	 * here and not a second one for overridable settings.
	 *
	 * <p>{@code JSON_OUTPUT_SCHEMA=MDAST;} is what tells {@code mdast} apart from
	 * {@code json}: both are the JSON canvas, and an implementation that drops it
	 * writes plain JSON and looks correct.
	 *
	 * <p>{@code HTML_INLINE_IMAGES} and {@code SVG_INLINE_IMAGES} both default to off
	 * in the engine, which for {@code html5} and {@code svg} is never what this tool
	 * wants: those canvases rasterise each page into a sibling image and lay a
	 * transparent text layer over it, but only the markup is written to
	 * {@code --out-dir}, so the sibling never lands and every page renders blank.
	 * Turning them on inlines the raster as a {@code data:} URI, which also gives
	 * {@code --out-file} and {@code --stdout} the single self-contained file they
	 * promise. doc2text forces the same two, for the same reason.
	 */
	public String getCanvasOptions()
	{
		return _canvasOptions;
	}

	/**
	 * True when the format writes exactly one file however many pages the
	 * document has (pdf, tiff, text). False when it writes one file per page
	 * (png, jpg, svg), which is what makes {@code --out-file} and {@code --stdout}
	 * illegal for it.
	 */
	public boolean isSingleFile()
	{
		return _isSingleFile;
	}

	/**
	 * True when several inputs may merge into one output under {@code --combine}.
	 *
	 * <p>A separate question from {@link #isSingleFile}, and the two only look
	 * alike. {@code docpack} writes one file per document, like pdf, but the
	 * engine's document pack takes one extractor, so a pack cannot hold two
	 * documents. Deriving {@code --combine} from {@link #isSingleFile} lets
	 * {@code --to docpack --combine} pass validation and fail at execution.
	 */
	public boolean supportsCombine()
	{
		return _supportsCombine;
	}

	/** True when the format can carry high-definition, paginated output. */
	public boolean isHiDefCapable()
	{
		return _isHiDefCapable;
	}

	/**
	 * The filename extension, without a dot, used by {@code {ext}} in an output template.
	 *
	 * <p>{@code web-safe} is the one entry whose extension is a placeholder,
	 * {@code unknown}: the engine picks png, gif, jpg or webp from the bit depth it
	 * settles on, which is not knowable until the page has been rendered, so
	 * execution renames the file once the real type is detected. That rename is an
	 * execution concern and needs nothing here --- the renamed path goes through
	 * {@code OutputPlanner.verifyPageTarget} like every other per-page output.
	 */
	public String getExtension()
	{
		return _extension;
	}

	/** Human-readable name for help and error text. */
	public String getDisplayName()
	{
		return _displayName;
	}

	/** The canonical text form, used by the conformance corpus. */
	@Override
	public String toString()
	{
		String kind;
		switch (_kind)
		{
			case TEXT:
				kind = "text";
				break;
			case CANVAS:
				kind = "canvas";
				break;
			default:
				kind = "docpack";
				break;
		}
		return _token + " " + kind + " " + (_isSingleFile ? "single-file" : "per-page") + (_isHiDefCapable ? " hidef" : "");
	}
}
