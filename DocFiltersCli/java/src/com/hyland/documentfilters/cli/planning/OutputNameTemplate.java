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
import java.util.ArrayList;
import java.util.List;

/**
 * An {@code --out-name} filename template, such as {@code {stem}-{page:04}.{ext}}.
 *
 * <p>Placeholders:
 * <ul>
 * <li>{@code {stem}}: input filename without its extension</li>
 * <li>{@code {page}}: one-based page number; {@code {page:04}} zero-pads to four digits</li>
 * <li>{@code {srcext}}: the input's original extension, without a dot</li>
 * <li>{@code {ext}}: the output format's extension, without a dot</li>
 * </ul>
 *
 * <p>The template is compiled when the argument is parsed, not on first use, so a typo
 * is reported before any work starts.
 */
public final class OutputNameTemplate
{
	private enum SegmentType
	{
		LITERAL,
		STEM,
		PAGE,
		SRC_EXT,
		EXT
	}

	private static final class Segment
	{
		final SegmentType type;
		final String literalText;
		final int width;

		Segment(SegmentType type, String literalText, int width)
		{
			this.type = type;
			this.literalText = literalText;
			this.width = width;
		}
	}

	private final String _originalText;
	private final List<Segment> _segments = new ArrayList<>();

	private OutputNameTemplate(String originalText)
	{
		_originalText = originalText;
	}

	private static UsageError error(String input)
	{
		return new UsageError(ErrorCodes.BAD_TEMPLATE, "--out-name does not accept the value '" + input + "'");
	}

	/**
	 * The template used when {@code --out-name} is not given: {@code {stem}.{ext}}
	 * for single-file formats, {@code {stem}-{page:04}.{ext}} for one-per-page formats.
	 *
	 * <p>{@code Default} in the reference; {@code default} is a Java keyword.
	 */
	public static OutputNameTemplate defaultFor(boolean isSingleFile)
	{
		if (isSingleFile)
			return parse("{stem}.{ext}");
		else
			return parse("{stem}-{page:04}.{ext}");
	}

	/**
	 * Compiles a template.
	 *
	 * <p>The template names a file <em>inside</em> {@code --out-dir}, so path
	 * separators are rejected here rather than sanitised at render time. Checking
	 * only the rendered output would miss them, because they can come from the
	 * template's own literal text as easily as from the input path.
	 *
	 * @throws UsageError an unknown placeholder, an unclosed brace, a malformed width
	 *         such as {@code {page:xx}}, or a path separator anywhere in the template.
	 */
	public static OutputNameTemplate parse(String text)
	{
		OutputNameTemplate result = new OutputNameTemplate(text);

		int startIndex = 0;
		while (startIndex < text.length())
		{
			int endIndex = indexOfSpecial(text, startIndex); // only an opening brace is supported at this point
			if (endIndex == -1)
			{
				result._segments.add(new Segment(SegmentType.LITERAL, text.substring(startIndex), 0));
				break;
			}

			if (text.charAt(endIndex) != '{')
				throw error(text); // invalid character

			if (endIndex > startIndex)
				result._segments.add(new Segment(SegmentType.LITERAL, text.substring(startIndex, endIndex), 0));

			startIndex = endIndex;
			endIndex = text.indexOf('}', startIndex + 1);
			if (endIndex == -1)
				throw error(text); // no closing brace
			// The name between the braces, e.g. "stem" or "page:04".
			String placeholder = text.substring(startIndex + 1, endIndex);
			switch (placeholder)
			{
				case "stem":
					result._segments.add(new Segment(SegmentType.STEM, null, 0));
					break;
				case "page":
					result._segments.add(new Segment(SegmentType.PAGE, null, 0));
					break;
				case "srcext":
					result._segments.add(new Segment(SegmentType.SRC_EXT, null, 0));
					break;
				case "ext":
					result._segments.add(new Segment(SegmentType.EXT, null, 0));
					break;
				default:
					if (placeholder.startsWith("page:"))
					{
						Long width = ValueParsers.tryParseDigits(placeholder.substring("page:".length()), Integer.MAX_VALUE);
						if (width == null || width <= 0)
							throw error(text); // malformed width

						result._segments.add(new Segment(SegmentType.PAGE, null, width.intValue()));
					}
					else
					{
						throw error(text); // unknown placeholder
					}
					break;
			}
			startIndex = endIndex + 1;
		}
		return result;
	}

	/** The first index at or after {@code start} holding a brace or a path separator, or -1. */
	private static int indexOfSpecial(String text, int start)
	{
		for (int i = start; i < text.length(); i++)
		{
			char c = text.charAt(i);
			if (c == '{' || c == '}' || c == '/' || c == '\\')
				return i;
		}
		return -1;
	}

	/** True when the template refers to {@code {page}}, which a single-file format cannot satisfy. */
	public boolean usesPage()
	{
		for (Segment segment : _segments)
		{
			if (segment.type == SegmentType.PAGE)
				return true;
		}
		return false;
	}

	/** Produces a filename. {@code pageNumber} is ignored when the template has no {@code {page}}. */
	public String render(String sourcePath, String outputExtension, int pageNumber)
	{
		StringBuilder sb = new StringBuilder();
		for (Segment segment : _segments)
		{
			switch (segment.type)
			{
				case LITERAL:
					sb.append(segment.literalText);
					break;
				case SRC_EXT:
					String extension = extension(normalizeSeparators(sourcePath));
					sb.append(extension.isEmpty() ? "" : extension.substring(1));
					break;
				case EXT:
					sb.append(outputExtension);
					break;
				case PAGE:
					String pageText = Integer.toString(pageNumber);
					for (int pad = pageText.length(); pad < segment.width; pad++)
						sb.append('0');
					sb.append(pageText);
					break;
				default:
					sb.append(fileNameWithoutExtension(normalizeSeparators(sourcePath)));
					break;
			}
		}
		return sb.toString();
	}

	/**
	 * {@code path} with every backslash turned into a forward slash.
	 *
	 * <p>A source path is not guaranteed to match the platform running this tool: a
	 * walk finds real paths in the platform's own convention, but an explicit
	 * {@code --files-from} list or a subfile name from a container built elsewhere
	 * can carry either. Without this, {@code {stem}} on
	 * {@code ..\..\windows\system32\config} would return the whole string unchanged
	 * on Linux, backslashes included, which is exactly the path separator this
	 * template exists to keep out of the rendered name. Forward slash alone is
	 * unambiguous on every platform this tool targets, so normalising onto it is
	 * safe in both directions.
	 */
	private static String normalizeSeparators(String path)
	{
		return path.replace('\\', '/');
	}

	/** Everything after the last forward slash. */
	private static String fileName(String path)
	{
		int slash = path.lastIndexOf('/');
		return slash < 0 ? path : path.substring(slash + 1);
	}

	/**
	 * The filename up to its last dot, as the reference's
	 * {@code Path.GetFileNameWithoutExtension}: {@code .gitignore} has an empty stem.
	 * Spelled out because {@code java.nio.file.Path} has no such method, and the
	 * platform's separator is the thing {@link #normalizeSeparators} has just taken out.
	 */
	private static String fileNameWithoutExtension(String path)
	{
		String name = fileName(path);
		int lastPeriod = name.lastIndexOf('.');
		return lastPeriod < 0 ? name : name.substring(0, lastPeriod);
	}

	/**
	 * The last dot and everything after it, as the reference's
	 * {@code Path.GetExtension}: empty when the filename has no dot at all, and
	 * empty when it ends in one.
	 */
	private static String extension(String path)
	{
		for (int i = path.length() - 1; i >= 0; i--)
		{
			char c = path.charAt(i);
			if (c == '.')
				return i == path.length() - 1 ? "" : path.substring(i);
			if (c == '/')
				break;
		}
		return "";
	}

	/** The original template text. */
	@Override
	public String toString()
	{
		return _originalText;
	}
}
