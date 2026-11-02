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
import com.hyland.documentfilters.cli.configuration.FormValue;
import com.hyland.documentfilters.cli.configuration.RedactSpec;
import com.hyland.documentfilters.cli.configuration.SharedOptions;
import com.hyland.documentfilters.cli.engine.EngineFormValue;
import com.hyland.documentfilters.cli.engine.EnginePageDecorations;
import com.hyland.documentfilters.cli.engine.EngineRedaction;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.IDocumentPage;
import com.hyland.documentfilters.cli.engine.IOutputCanvas;
import com.hyland.documentfilters.cli.planning.CanvasType;
import com.hyland.documentfilters.cli.planning.FormatSpec;
import com.hyland.documentfilters.cli.planning.OutputPlanner;
import com.hyland.documentfilters.cli.planning.PageRange;
import com.hyland.documentfilters.cli.planning.PlannedItem;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.MemoryChannel;
import com.hyland.documentfilters.cli.runtime.NullChannel;
import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * The high-definition path: open a canvas, walk the pages, render each one.
 * Covers PDF, TIFF, the image formats, HTML5, structured XML, JSON and
 * high-definition markdown.
 */
public final class CanvasConversion
{
	private final DocumentFiltersSession _session;
	private final IConsole _console;
	private final SharedOptions _shared;
	private final IFileSystem _fileSystem;

	public CanvasConversion(DocumentFiltersSession session, IConsole console)
	{
		this(session, console, null, null);
	}

	/**
	 * Takes the session because canvases are made by
	 * {@code DocumentFilters.MakeOutputCanvas} --- an {@code IDocument} cannot
	 * produce one.
	 *
	 * @param shared the output policy, for checking each per-page target as it is resolved.
	 * @param fileSystem null in tests that render nowhere and verify nothing.
	 */
	public CanvasConversion(DocumentFiltersSession session, IConsole console, SharedOptions shared, IFileSystem fileSystem)
	{
		_session = session;
		_console = console;
		_shared = shared;
		_fileSystem = fileSystem;
	}

	/**
	 * Renders one document to its own output. Used for both single-file formats
	 * (one canvas, many pages) and one-per-page formats (one canvas per page, named
	 * from {@link PlannedItem#targetPattern} as the pages are found).
	 */
	public void convert(IDocument document, PlannedItem item, FormatSpec format, ConvertOptions options) throws IOException
	{
		String canvasOptions = composeCanvasOptions(format, options.canvasOptions, options.noImages);

		if (item.target != null)
		{
			try (IOutputCanvas canvas = makeCanvas(item.target, format.getCanvasType(), canvasOptions, options))
			{
				appendTo(canvas, document, item.pages, options, item.displayName);
			}
		}
		else
		{
			for (int pageNumber : renderablePages(item.pages, document.getPageCount()))
			{
				String target = item.targetPattern.resolve(item.sourcePath, pageNumber);

				// Each page, not just the first. Planning can only check the first target
				// of a per-page item --- how many pages a document has is not known until
				// it is open --- so the rest have to be checked here, as they are resolved.
				verifyPageTarget(target, options);

				try (IOutputCanvas canvas = makeCanvas(target, format.getCanvasType(), canvasOptions, options))
				{
					renderPage(canvas, document, pageNumber, options, item.displayName);
				}

				// After the canvas is closed, so the bytes are on disk to be identified.
				nameByRenderedType(target, format, options);
			}
		}
	}

	/**
	 * Renders one document straight to {@code destination} --- for
	 * {@code --stdout}, where a single-file canvas format owns no path at all.
	 *
	 * <p>Renders into a seekable buffer first rather than handing
	 * {@code destination} to the canvas directly: finishing a binary format means
	 * seeking back to patch a header or table computed only once the whole
	 * document has rendered, and standard output is forward-only.
	 */
	public void convertToStream(IDocument document, OutputStream destination, PlannedItem item, FormatSpec format,
		ConvertOptions options) throws IOException
	{
		String canvasOptions = composeCanvasOptions(format, options.canvasOptions, options.noImages);

		MemoryChannel buffer = new MemoryChannel();
		try (IOutputCanvas canvas = _session.getApi().makeCanvas(buffer, format.getCanvasType(), canvasOptions))
		{
			appendTo(canvas, document, item.pages, options, item.displayName);
		}

		buffer.writeTo(destination);
		destination.flush();
	}

	/** {@link #composeCanvasOptions(FormatSpec, String, boolean)} without {@code --no-images}. */
	public static String composeCanvasOptions(FormatSpec format, String userOptions)
	{
		return composeCanvasOptions(format, userOptions, false);
	}

	/**
	 * The option string a canvas for {@code format} is opened with:
	 * {@code --canvas-options} first, then the format's own.
	 *
	 * <p>The order is the whole of the rule. The engine resolves a repeated key to
	 * its first occurrence, so putting the user in front means they win every key
	 * they name, and {@link FormatSpec#getCanvasOptions} only fills in what they
	 * left alone.
	 */
	public static String composeCanvasOptions(FormatSpec format, String userOptions, boolean noImages)
	{
		String user = trimSemicolons(userOptions != null ? userOptions : "");
		String own = trimSemicolons(format.getCanvasOptions() != null ? format.getCanvasOptions() : "");

		String suppressed = noImages ? imageSuppression(format) : "";
		if (!suppressed.isEmpty())
			own = own.isEmpty() ? suppressed : own + ";" + suppressed;

		if (user.isEmpty())
			return own;
		if (own.isEmpty())
			return user;

		return user + ";" + own;
	}

	/**
	 * What {@code --no-images} means for {@code format}, or nothing when it means
	 * nothing.
	 *
	 * <p>Only the markdown formats have an engine switch for this ---
	 * {@code MARKDOWN_INCLUDE_IMAGES}. The flag is deliberately ignored for
	 * {@code html5} and {@code svg}: those inline the page raster itself, so
	 * suppressing images there would blank the page.
	 */
	public static String imageSuppression(FormatSpec format)
	{
		return format != null && (format.getToken().equals("md") || format.getToken().equals("md-text"))
			? "MARKDOWN_INCLUDE_IMAGES=OFF"
			: "";
	}

	/**
	 * Renders one document onto a canvas someone else owns, for {@code --combine}.
	 *
	 * <p>Takes the fast, single-call {@code RenderPages} path whenever nothing needs a
	 * per-page hook; an explicit {@code --pages} selection or any decoration
	 * (including {@code --fingerprint}) falls back to rendering one page at a time.
	 */
	public void appendTo(IOutputCanvas canvas, IDocument document, PageRange pages, ConvertOptions options, String displayName)
		throws IOException
	{
		if (pages.isAll() && !hasPageDecorations(options) && !options.fingerprint)
		{
			canvas.renderPages(document);
		}
		else
		{
			for (int pageNumber : renderablePages(pages, document.getPageCount()))
				renderPage(canvas, document, pageNumber, options, displayName);
		}
	}

	/**
	 * The pages of {@code pages} that a document of {@code pageCount} pages actually
	 * has.
	 *
	 * <p>Stops at the first page past the end rather than filtering: the pages
	 * arrive ascending, and a filter would still walk all two billion of
	 * {@code --pages 1-2147483647} to find that out.
	 */
	public static Iterable<Integer> renderablePages(PageRange pages, int pageCount)
	{
		return () -> new Iterator<Integer>()
		{
			private final Iterator<Integer> _inner = pages.enumerate(pageCount).iterator();
			private Integer _next = advance();

			private Integer advance()
			{
				if (!_inner.hasNext())
					return null;
				Integer candidate = _inner.next();
				return candidate <= pageCount ? candidate : null;
			}

			@Override
			public boolean hasNext()
			{
				return _next != null;
			}

			@Override
			public Integer next()
			{
				if (_next == null)
					throw new NoSuchElementException();
				Integer current = _next;
				_next = advance();
				return current;
			}
		};
	}

	/**
	 * Renames a {@code web-safe} page to match whatever the engine actually wrote.
	 *
	 * <p>{@code web-safe} is the one format whose extension cannot be planned: the
	 * engine chooses PNG, GIF, JPEG or WebP per page. The planner names the file
	 * {@code .unknown}, and this puts the real name on it afterwards, from the
	 * engine identifying the file it just wrote. The destination goes through the
	 * same output policy as any other target.
	 */
	private void nameByRenderedType(String target, FormatSpec format, ConvertOptions options) throws IOException
	{
		if (!format.getToken().equals("web-safe") || options.fingerprint)
			return;

		String extension = extensionForMimeType(_session.getApi().identifyFile(target).mimeType);
		if (extension == null)
			return;

		String renamed = changeExtension(target, extension);
		if (renamed.equals(target))
			return;

		verifyPageTarget(renamed, options);
		_fileSystem.moveFile(target, renamed);
	}

	/** The extension for an image the engine identified, or null if unknown. */
	private static String extensionForMimeType(String mimeType)
	{
		if (mimeType == null)
			return null;

		switch (mimeType)
		{
			case "image/png":
				return "png";
			case "image/gif":
				return "gif";
			case "image/jpeg":
				return "jpg";
			case "image/webp":
				return "webp";
			default:
				return null;
		}
	}

	/**
	 * The reference's {@code Path.ChangeExtension}: the extension after the last dot
	 * of the final segment is replaced, or one is appended when there is none.
	 */
	static String changeExtension(String path, String extension)
	{
		int separator = Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
		int dot = path.lastIndexOf('.');
		String stem = dot > separator ? path.substring(0, dot) : path;
		return stem + "." + extension;
	}

	/**
	 * Applies the run's output policy to one resolved per-page target. Skipped under
	 * {@code --fingerprint}, which writes no file at all.
	 */
	private void verifyPageTarget(String target, ConvertOptions options)
	{
		if (options.fingerprint || _shared == null || _fileSystem == null)
			return;

		OutputPlanner.verifyPageTarget(target, _shared, _fileSystem);
	}

	/**
	 * Opens the canvas a page would normally render onto --- or, under
	 * {@code --fingerprint}, a canvas whose bytes are thrown away instead.
	 *
	 * <p>The render still has to happen, because a fingerprint is an attribute of
	 * the rendered page, but the bytes it produces are never {@code target}'s real
	 * content, the same way doc2text's own {@code --fingerprint} renders into a
	 * temporary stream it never keeps.
	 */
	private IOutputCanvas makeCanvas(String target, CanvasType canvasType, String canvasOptions, ConvertOptions options)
	{
		// The directory is already there: ConvertCommand creates it once per item,
		// because the engine opens this file itself and will not create one.
		return options.fingerprint
			? _session.getApi().makeCanvas(new NullChannel(), canvasType, canvasOptions)
			: _session.getApi().makeCanvas(target, canvasType, canvasOptions);
	}

	private void renderPage(IOutputCanvas canvas, IDocument document, int pageNumber, ConvertOptions options, String displayName)
		throws IOException
	{
		try (IDocumentPage page = document.getPage(pageNumber - 1))
		{
			String renderOptions = options.renderOptions != null ? options.renderOptions : "";
			if (options.watermark != null && !options.watermark.isEmpty() && !isWatermarkFile(options))
				renderOptions += (renderOptions.isEmpty() ? "" : ";") + "WATERMARK=" + options.watermark;
			canvas.renderPage(page, renderOptions, applyPageDecorations(pageNumber, options));
			drawWatermark(canvas, page, options);

			if (options.fingerprint)
				writeFingerprint(page, pageNumber, displayName);
		}
	}

	/**
	 * Writes {@code --fingerprint}'s one line for a page that has just been rendered:
	 * {@code IMAGE_FINGERPRINT_V2} first, falling back to {@code IMAGE_FINGERPRINT},
	 * nothing when both are empty --- doc2text's {@code fingerPrintPage}.
	 */
	public void writeFingerprint(IDocumentPage page, int pageNumber, String displayName)
	{
		String line = formatFingerprint(displayName, pageNumber, page.getAttribute("IMAGE_FINGERPRINT_V2"),
			page.getAttribute("IMAGE_FINGERPRINT"));
		if (line != null)
			_console.writeLine(line);
	}

	/** The formatting half of {@link #writeFingerprint}, testable without the engine. */
	public static String formatFingerprint(String displayName, int pageNumber, String fingerprintV2, String fingerprintV1)
	{
		String fingerprint = fingerprintV2 == null || fingerprintV2.isEmpty() ? fingerprintV1 : fingerprintV2;
		if (fingerprint == null || fingerprint.isEmpty())
			return null;

		return displayName + ": page=" + pageNumber + " fingerprint=" + fingerprint;
	}

	private static boolean hasPageDecorations(ConvertOptions options)
	{
		return (options.watermark != null && !options.watermark.isEmpty()) || !options.redactions.isEmpty()
			|| !options.formValues.isEmpty();
	}

	/**
	 * Applies {@code --redact} and {@code --form} to one page before it is rendered.
	 * Null when the page needs neither. {@code --watermark} is handled separately,
	 * by {@link #drawWatermark}.
	 */
	public static EnginePageDecorations applyPageDecorations(int pageNumber, ConvertOptions options)
	{
		EnginePageDecorations decorations = new EnginePageDecorations();

		for (RedactSpec redaction : options.redactions)
		{
			if (redaction.page == pageNumber)
				decorations.redactions.add(new EngineRedaction(redaction.x, redaction.y, redaction.width, redaction.height));
		}

		for (FormValue form : options.formValues)
		{
			if (form.page == pageNumber)
				decorations.formValues.add(new EngineFormValue(form.name, form.value));
		}

		return decorations.isEmpty() ? null : decorations;
	}

	public void drawWatermark(IOutputCanvas canvas, IDocumentPage page, ConvertOptions options) throws IOException
	{
		if (options.watermark == null || options.watermark.isEmpty() || !isWatermarkFile(options))
			return;

		byte[] bytes = Files.readAllBytes(new File(options.watermark.replace('\\', '/')).toPath());
		canvas.drawImage(0, 0, bytes, null);
	}

	/** Whether {@code --watermark} names an existing file rather than watermark text. */
	private static boolean isWatermarkFile(ConvertOptions options)
	{
		return new File(options.watermark.replace('\\', '/')).isFile();
	}

	private static String trimSemicolons(String text)
	{
		int start = 0;
		int end = text.length();
		while (start < end && text.charAt(start) == ';')
			++start;
		while (end > start && text.charAt(end - 1) == ';')
			--end;
		return text.substring(start, end);
	}
}
