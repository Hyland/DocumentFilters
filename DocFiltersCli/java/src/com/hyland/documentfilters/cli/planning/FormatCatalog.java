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

import com.hyland.documentfilters.cli.configuration.ValueParsers;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.UsageError;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * The {@code --to} token table: what each format name means, and the two static
 * properties the output planner needs from it.
 *
 * <p>This table is the single source of truth for the format surface in all four
 * language implementations, and several entries are not a straight mapping onto
 * a single engine enum value:
 * <ul>
 * <li>{@code md} is high-definition markdown via a canvas; {@code md-text} is
 * text-mode markdown straight from the extractor.</li>
 * <li>{@code html} is flow HTML from the extractor; {@code html5} is paginated
 * high-definition HTML via a canvas.</li>
 * <li>{@code docpack} is neither --- it goes through the engine's document pack.</li>
 * <li>{@code mdast} has no device of its own: it is the JSON canvas under a
 * different schema, told apart only by the {@code JSON_OUTPUT_SCHEMA=MDAST;} it
 * contributes through {@link FormatSpec#getCanvasOptions}. doc2text spells it the
 * same way.</li>
 * </ul>
 */
public final class FormatCatalog
{
	private static final boolean SINGLE = true;
	private static final boolean PER_PAGE = false;
	private static final boolean COMBINE = true;
	private static final boolean NO_COMBINE = false;
	private static final boolean HIDEF = true;
	private static final boolean NO_HIDEF = false;

	private static final List<FormatSpec> ALL = Collections.unmodifiableList(Arrays.asList(
		// === text mode: straight from the extractor ================================

		new FormatSpec("text", FormatKind.TEXT, null, "", SINGLE, COMBINE, NO_HIDEF, "txt", "Plain text"),
		new FormatSpec("md-text", FormatKind.TEXT, null, "", SINGLE, COMBINE, NO_HIDEF, "md", "Markdown (text mode)"),
		new FormatSpec("html", FormatKind.TEXT, null, "", SINGLE, COMBINE, NO_HIDEF, "html", "HTML (flow)"),

		// === canvas, single file ===================================================

		new FormatSpec("md", FormatKind.CANVAS, CanvasType.MARKDOWN, "", SINGLE, COMBINE, HIDEF, "md", "Markdown (high definition)"),
		new FormatSpec("html5", FormatKind.CANVAS, CanvasType.HTML, "HTML_INLINE_IMAGES=ON;", SINGLE, COMBINE, HIDEF, "html",
			"HTML5 (paginated)"),
		new FormatSpec("json", FormatKind.CANVAS, CanvasType.JSON, "", SINGLE, COMBINE, HIDEF, "json", "JSON"),
		new FormatSpec("xml", FormatKind.CANVAS, CanvasType.XML, "", SINGLE, COMBINE, HIDEF, "xml", "XML"),
		new FormatSpec("pdf", FormatKind.CANVAS, CanvasType.PDF, "", SINGLE, COMBINE, HIDEF, "pdf", "PDF"),
		new FormatSpec("tiff", FormatKind.CANVAS, CanvasType.TIF, "", SINGLE, COMBINE, HIDEF, "tif", "TIFF (multi-page)"),
		new FormatSpec("xps", FormatKind.CANVAS, CanvasType.XPS, "", SINGLE, COMBINE, HIDEF, "xps", "XPS"),
		new FormatSpec("ps", FormatKind.CANVAS, CanvasType.PS, "", SINGLE, COMBINE, HIDEF, "ps", "PostScript"),
		new FormatSpec("dcx", FormatKind.CANVAS, CanvasType.DCX, "", SINGLE, COMBINE, HIDEF, "dcx", "DCX (multi-page PCX)"),

		// === canvas, one file per page =============================================

		new FormatSpec("png", FormatKind.CANVAS, CanvasType.PNG, "", PER_PAGE, NO_COMBINE, HIDEF, "png", "PNG"),
		new FormatSpec("jpg", FormatKind.CANVAS, CanvasType.JPG, "", PER_PAGE, NO_COMBINE, HIDEF, "jpg", "JPEG"),
		new FormatSpec("svg", FormatKind.CANVAS, CanvasType.SVG, "SVG_INLINE_IMAGES=ON;", PER_PAGE, NO_COMBINE, HIDEF, "svg", "SVG"),
		new FormatSpec("gif", FormatKind.CANVAS, CanvasType.GIF, "", PER_PAGE, NO_COMBINE, HIDEF, "gif", "GIF"),
		new FormatSpec("bmp", FormatKind.CANVAS, CanvasType.BMP, "", PER_PAGE, NO_COMBINE, HIDEF, "bmp", "Windows bitmap"),
		new FormatSpec("eps", FormatKind.CANVAS, CanvasType.EPS, "", PER_PAGE, NO_COMBINE, HIDEF, "eps", "Encapsulated PostScript"),
		new FormatSpec("webp", FormatKind.CANVAS, CanvasType.WEBP, "", PER_PAGE, NO_COMBINE, HIDEF, "webp", "WebP"),
		new FormatSpec("jpeg2000", FormatKind.CANVAS, CanvasType.JPEG2000, "", PER_PAGE, NO_COMBINE, HIDEF, "jp2", "JPEG 2000"),
		new FormatSpec("pcx", FormatKind.CANVAS, CanvasType.PCX, "", PER_PAGE, NO_COMBINE, HIDEF, "pcx", "PCX"),
		new FormatSpec("tga", FormatKind.CANVAS, CanvasType.TGA, "", PER_PAGE, NO_COMBINE, HIDEF, "tga", "Targa"),
		new FormatSpec("brk", FormatKind.CANVAS, CanvasType.BRK, "", PER_PAGE, NO_COMBINE, HIDEF, "brk", "Brooktrout fax"),
		new FormatSpec("pbm", FormatKind.CANVAS, CanvasType.PBM, "", PER_PAGE, NO_COMBINE, HIDEF, "pbm", "Portable bitmap (monochrome)"),
		new FormatSpec("pgm", FormatKind.CANVAS, CanvasType.PGM, "", PER_PAGE, NO_COMBINE, HIDEF, "pgm", "Portable greymap"),
		new FormatSpec("ppm", FormatKind.CANVAS, CanvasType.PPM, "", PER_PAGE, NO_COMBINE, HIDEF, "ppm", "Portable pixmap"),
		new FormatSpec("mdast", FormatKind.CANVAS, CanvasType.JSON, "JSON_OUTPUT_SCHEMA=MDAST;", SINGLE, COMBINE, HIDEF, "json",
			"Markdown AST (JSON)"),
		new FormatSpec("web-safe", FormatKind.CANVAS, CanvasType.WEB_SAFE, "", PER_PAGE, NO_COMBINE, HIDEF, "unknown", "Web-safe image"),

		// === the third path ========================================================

		new FormatSpec("docpack", FormatKind.DOCPACK, null, "", SINGLE, NO_COMBINE, HIDEF, "docpack", "Document Pack")));

	private static final Map<String, FormatSpec> BY_TOKEN = byToken();

	private FormatCatalog()
	{
	}

	private static Map<String, FormatSpec> byToken()
	{
		Map<String, FormatSpec> map = new HashMap<>();
		for (FormatSpec spec : ALL)
			map.put(spec.getToken(), spec);
		return map;
	}

	/** Every format token this build accepts, in help-display order. */
	public static List<FormatSpec> all()
	{
		return ALL;
	}

	/**
	 * Resolves a {@code --to} token.
	 *
	 * @throws UsageError the token is not in the catalogue. The message says how to
	 *         list the valid tokens, so that a typo is self-correcting.
	 */
	public static FormatSpec resolve(String token)
	{
		FormatSpec res = tryResolve(token);
		if (res != null)
			return res;

		throw new UsageError(ErrorCodes.UNKNOWN_FORMAT,
			"unknown format '" + token + "'; run 'docfilters formats' to list what this build can write");
	}

	/** Resolves a token, returning null instead of throwing when it is unknown. */
	public static FormatSpec tryResolve(String token)
	{
		return BY_TOKEN.get(ValueParsers.foldToken(token));
	}
}
