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

package com.hyland.documentfilters.cli.execution;

import com.hyland.documentfilters.cli.configuration.ConvertOptions;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.engine.OpenType;
import com.hyland.documentfilters.cli.planning.FormatKind;
import com.hyland.documentfilters.cli.planning.FormatSpec;

/**
 * How a document should be opened: the engine flags and the options string
 * passed to {@code IDocument.Open}.
 *
 * <p>These have to be decided <em>before</em> the document is opened, because the
 * engine applies them during {@code Open} --- a flag worked out afterwards has no
 * effect. That is why {@link DocumentFiltersSession#openDocument} takes this
 * rather than each conversion path computing flags for a document that is
 * already open.
 */
public final class ExtractorOpenSettings
{
	/**
	 * The complete {@code IGR_} open flags: the content selector, plus exactly one
	 * {@code FORMAT_*} bit --- or none, where the engine's default is what is wanted.
	 *
	 * <p>The {@code FORMAT_*} values are an enumeration packed into bits 16 to 19,
	 * not independent bits, so at most one may ever be set.
	 */
	public int flags;

	/** The engine options string, from {@code --options} and {@code --codepage}. */
	public String options;

	/**
	 * The settings for converting to a given target format.
	 *
	 * <p>{@code --no-images} reaches a text-mode format here rather than through the
	 * canvas options, because {@code md-text} never opens a canvas. {@code md}, which
	 * does, is handled by {@link CanvasConversion#composeCanvasOptions}; both read
	 * the same rule from {@link CanvasConversion#imageSuppression}.
	 */
	public static ExtractorOpenSettings forFormat(FormatSpec format, ConvertOptions options, SharedOptions shared)
	{
		String engineOptions = buildOptionsString(shared);
		if (options.noImages && format.getKind() == FormatKind.TEXT)
		{
			String suppressed = CanvasConversion.imageSuppression(format);
			if (!suppressed.isEmpty())
				engineOptions = append(engineOptions, suppressed);
		}

		ExtractorOpenSettings settings = new ExtractorOpenSettings();
		settings.flags = OpenType.BODY_AND_META | formatBit(format, options);
		settings.options = engineOptions;
		return settings;
	}

	/** Joins two engine option strings, inserting a separator where needed. */
	private static String append(String existing, String addition)
	{
		if (!existing.isEmpty() && !existing.endsWith(";"))
			existing += ";";
		return existing + addition + ";";
	}

	/**
	 * The one {@code IGR_FORMAT_*} bit a conversion opens with, or zero for none.
	 *
	 * <p>A canvas format needs {@code FORMAT_IMAGE}: that is what makes pages exist.
	 * A text format names the reader it wants, and {@code text} asks for none at
	 * all under {@code --include-control-codes} --- omitting {@code IGR_FORMAT_TEXT}
	 * is what leaves the ISYS control codes in the stream. {@code docpack} takes
	 * none either, matching doc2text.
	 *
	 * <p>Pure, and separate from {@link #forFormat}, so the whole table can be
	 * checked without an engine.
	 */
	public static int formatBit(FormatSpec format, ConvertOptions options)
	{
		if (format.getKind() == FormatKind.CANVAS)
			return OpenType.FORMAT_IMAGE;

		if (format.getKind() == FormatKind.DOCPACK)
			return 0;

		switch (format.getToken())
		{
			case "text":
				return options.includeControlCodes ? 0 : OpenType.FORMAT_TEXT;
			case "md-text":
				return OpenType.FORMAT_MARKDOWN;
			case "html":
				return OpenType.FORMAT_HTML;
			default:
				return OpenType.FORMAT_TEXT;
		}
	}

	/** {@link #forInspection(SharedOptions, boolean)} without {@code ENUMERATE_IMAGES}. */
	public static ExtractorOpenSettings forInspection(SharedOptions shared)
	{
		return forInspection(shared, false);
	}

	/**
	 * The settings for a command that reads a document without converting it ---
	 * {@code identify}, {@code meta}, {@code list} and {@code extract}.
	 *
	 * <p>{@code ENUMERATE_IMAGES} has to be set here, at open, because that is the
	 * only place the engine reads it. Appended after the user's {@code --options} so
	 * an explicit {@code ENUMERATE_IMAGES=OFF} still wins, the engine resolving a
	 * repeated key to its first occurrence.
	 *
	 * @param enumerateImages {@code extract --images}: ask the engine to surface a
	 *        document's embedded images as well as its subfiles.
	 */
	public static ExtractorOpenSettings forInspection(SharedOptions shared, boolean enumerateImages)
	{
		String options = buildOptionsString(shared);
		if (enumerateImages)
		{
			if (!options.isEmpty() && !options.endsWith(";"))
				options += ";";
			options += "ENUMERATE_IMAGES=ON;";
		}

		ExtractorOpenSettings settings = new ExtractorOpenSettings();
		settings.flags = OpenType.BODY_AND_META | OpenType.FORMAT_TEXT;
		settings.options = options;
		return settings;
	}

	/**
	 * The settings for {@code meta} specifically: {@code META_ONLY} rather than the
	 * {@code BODY_AND_META} every other inspection command uses, as doc2text's own
	 * {@code --meta} does. Body text would otherwise mix document content into the
	 * one {@code Label: value} line per field {@link MetadataReport} parses.
	 */
	public static ExtractorOpenSettings forMetadataOnly(SharedOptions shared)
	{
		ExtractorOpenSettings settings = new ExtractorOpenSettings();
		settings.flags = OpenType.META_ONLY | OpenType.FORMAT_TEXT;
		settings.options = buildOptionsString(shared);
		return settings;
	}

	/**
	 * Builds the semicolon-separated engine options string from {@code --options}
	 * and {@code --codepage}, in the form doc2text itself uses:
	 * {@code ...;CODEPAGE=1252;}.
	 */
	private static String buildOptionsString(SharedOptions shared)
	{
		String options = shared.processing.engineOptions != null ? shared.processing.engineOptions : "";
		String codepage = shared.processing.codepage;
		if (codepage != null && !codepage.isEmpty())
		{
			if (!options.isEmpty() && !options.endsWith(";"))
				options += ";";
			options += "CODEPAGE=" + codepage + ";";
		}
		return options;
	}
}
