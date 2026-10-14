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

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Planning;
using Hyland.DocumentFilters.Cli.Runtime;
using System.Drawing;

using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// The high-definition path: open a canvas, walk the pages, render each one.
	/// Covers PDF, TIFF, the image formats, HTML5, structured XML, JSON and
	/// high-definition markdown.
	/// </summary>
	public sealed class CanvasConversion
	{
		private readonly DocumentFiltersSession _session;
		private readonly IConsole _console;

		/// <summary>
		/// Takes the session because canvases are made by
		/// <c>DocumentFilters.MakeOutputCanvas</c> — an <c>IDocument</c> cannot produce
		/// one, so a document alone is not enough to render anything.
		/// </summary>
		/// <param name="shared">
		/// The output policy, for checking each per-page target as it is resolved.
		/// </param>
		/// <param name="fileSystem">Null in tests that render nowhere and verify nothing.</param>
		public CanvasConversion(DocumentFiltersSession session, IConsole console,
								SharedOptions shared = null, IFileSystem fileSystem = null)
		{
			_session = session;
			_console = console;
			_shared = shared;
			_fileSystem = fileSystem;
		}

		private readonly SharedOptions _shared;
		private readonly IFileSystem _fileSystem;

		/// <summary>
		/// Renders one document to its own output. Used for both single-file formats
		/// (one canvas, many pages) and one-per-page formats (one canvas per page,
		/// named from <see cref="PlannedItem.TargetPattern"/> as the pages are found).
		/// </summary>
		public void Convert(IDocument document, PlannedItem item, FormatSpec format, ConvertOptions options)
		{
			string canvasOptions = ComposeCanvasOptions(format, options.CanvasOptions, options.NoImages);

			if (item.Target != null)
			{
				using IOutputCanvas canvas = MakeCanvas(item.Target, format.CanvasType.Value, canvasOptions, options);
				AppendTo(canvas, document, item.Pages, options, item.DisplayName);
			}
			else
			{
				foreach (int pageNumber in RenderablePages(item.Pages, document.GetPageCount()))
				{
					string target = item.TargetPattern.Resolve(item.SourcePath, pageNumber);

					// Each page, not just the first. Planning can only check the first target
					// of a per-page item — how many pages a document has is not known until it
					// is open — so the rest have to be checked here, as they are resolved.
					// Without this, an existing page 2 was overwritten without --overwrite
					// whenever page 1 happened to be absent: exit 0, nothing reported.
					VerifyPageTarget(target, options);

					using (IOutputCanvas canvas = MakeCanvas(target, format.CanvasType.Value, canvasOptions, options))
						RenderPage(canvas, document, pageNumber, options, item.DisplayName);

					// After the canvas is closed, so the bytes are on disk to be identified.
					NameByRenderedType(target, format, options);
                }
			}
		}

		/// <summary>
		/// Renders one document straight to <paramref name="destination"/> — for
		/// <c>--stdout</c>, where a single-file canvas format owns no path at all.
		/// </summary>
		/// <remarks>
		/// <see cref="Convert"/> only ever branches on <see cref="PlannedItem.Target"/>
		/// versus <see cref="PlannedItem.TargetPattern"/>, because those are the two
		/// shapes <see cref="Planning.OutputPlanner"/> produces for a real file target —
		/// but <c>--stdout</c> resolves to <see cref="Planning.OutputCardinality.Stream"/>,
		/// which leaves both null. Convert's own <c>else</c> branch used to be reached
		/// with a null <see cref="PlannedItem.TargetPattern"/> in that case and threw a
		/// bare <see cref="NullReferenceException"/> — reported as a document failure
		/// with no useful message. <see cref="Commands.ConvertCommand"/> now calls this
		/// instead whenever <c>--stdout</c> is set, the same way <c>TextConversion</c>
		/// already had its own stdout branch for text formats; canvas formats never did.
		///
		/// Renders into a seekable buffer first rather than handing
		/// <paramref name="destination"/> to the canvas directly. Every published
		/// sample that streams a canvas (<c>ConvertDocumentToTIFFStream</c>,
		/// <c>ConvertDocumentToInteractiveHDHTML</c>) writes to a <see cref="MemoryStream"/>
		/// for the same reason: finishing a binary format means seeking back to patch
		/// a header or table computed only once the whole document has rendered, and
		/// <see cref="IConsole.OpenStandardOutput"/> is forward-only. Handing the
		/// console stream straight to the canvas produced no error and no output —
		/// the render itself succeeded, but whatever the canvas needed to seek back
		/// and write never reached anywhere.
		/// </remarks>
		public void ConvertToStream(IDocument document, Stream destination, PlannedItem item, FormatSpec format, ConvertOptions options)
		{
			string canvasOptions = ComposeCanvasOptions(format, options.CanvasOptions, options.NoImages);

			using var buffer = new MemoryStream();
			using (IOutputCanvas canvas = _session.Api.MakeCanvas(buffer, format.CanvasType.Value, canvasOptions))
			{
				AppendTo(canvas, document, item.Pages, options, item.DisplayName);
			}

			buffer.Position = 0;
			buffer.CopyTo(destination);
			destination.Flush();
		}

		/// <summary>
		/// The option string a canvas for <paramref name="format"/> is opened with:
		/// <c>--canvas-options</c> first, then the format's own.
		/// </summary>
		/// <remarks>
		/// The order is the whole of the rule. The engine resolves a repeated key to
		/// its first occurrence — <c>CLayoutOptions::parse</c> appends rather than
		/// replaces, and <c>find</c> returns the first match — so putting the user in
		/// front means they win every key they name, and
		/// <see cref="FormatSpec.CanvasOptions"/> only fills in what they left alone.
		///
		/// Used by both canvas paths, here and <c>--combine</c>'s shared canvas in
		/// <see cref="Commands.ConvertCommand"/>, because a combined run produces the
		/// same format and needs the same options. Static and free of the engine so
		/// the ordering can be tested directly.
		/// </remarks>
		public static string ComposeCanvasOptions(FormatSpec format, string userOptions, bool noImages = false)
		{
			string user = (userOptions ?? string.Empty).Trim(';');
			string own = (format.CanvasOptions ?? string.Empty).Trim(';');

			string suppressed = noImages ? ImageSuppression(format) : string.Empty;
			if (suppressed.Length > 0)
				own = own.Length == 0 ? suppressed : own + ";" + suppressed;

			if (user.Length == 0)
				return own;
			if (own.Length == 0)
				return user;

			return user + ";" + own;
		}

		/// <summary>
		/// What <c>--no-images</c> means for <paramref name="format"/>, or nothing when
		/// it means nothing.
		/// </summary>
		/// <remarks>
		/// Only the markdown formats have an engine switch for this —
		/// <c>MARKDOWN_INCLUDE_IMAGES</c>. Plain text has no way to carry an image in
		/// the first place, and the flag is deliberately ignored for <c>html5</c> and
		/// <c>svg</c>: those inline the page raster itself (D35), so suppressing
		/// images there would not drop embedded pictures, it would blank the page.
		/// Ignored rather than refused, so the flag can be set once for a batch of
		/// mixed formats without a run failing on the ones it does not touch.
		/// </remarks>
		public static string ImageSuppression(FormatSpec format)
		{
			return format != null && (format.Token == "md" || format.Token == "md-text")
				? "MARKDOWN_INCLUDE_IMAGES=OFF"
				: string.Empty;
		}

		/// <summary>
		/// Renders one document onto a canvas someone else owns, for <c>--combine</c>.
		/// The caller opens the canvas once and passes every document through in turn.
		/// </summary>
		/// <remarks>
		/// Takes the fast, single-call <c>RenderPages</c> path — the one the published
		/// samples use — whenever nothing needs a per-page hook; an explicit
		/// <c>--pages</c> selection or any decoration (including <c>--fingerprint</c>,
		/// which needs the page open after it renders) falls back to rendering one page
		/// at a time.
		/// </remarks>
		public void AppendTo(IOutputCanvas canvas, IDocument document, PageRange pages, ConvertOptions options, string displayName)
		{
			if (pages.IsAll && !HasPageDecorations(options) && !options.Fingerprint)
			{
				canvas.RenderPages(document);
			}
			else
			{
				foreach (int pageNumber in RenderablePages(pages, document.GetPageCount()))
					RenderPage(canvas, document, pageNumber, options, displayName);
			}
        }

		/// <summary>
		/// The pages of <paramref name="pages"/> that a document of
		/// <paramref name="pageCount"/> pages actually has.
		/// </summary>
		/// <remarks>
		/// <see cref="PageRange.Enumerate"/> deliberately does not filter an explicit
		/// selection by count — it has no way to know whether a count it is handed is
		/// genuine — so that filtering happens here instead, at the one point that
		/// does have a real, just-verified page count: after the document is open.
		/// Kept as a pure function of the range and the count, rather than folded
		/// inline, so it is testable without the engine. <see cref="PageRange.All"/>
		/// never needs the filter — <c>Enumerate</c> already stops at
		/// <paramref name="pageCount"/> for it — but applying it uniformly is simpler
		/// than special-casing which branch needs it.
		///
		/// <c>TakeWhile</c> and not <c>Where</c>: the pages arrive ascending, so the first
		/// one past the end means every one after it is too, and <c>Where</c> would still
		/// walk all two billion of <c>--pages 1-2147483647</c> to find that out.
		/// </remarks>
		public static IEnumerable<int> RenderablePages(PageRange pages, int pageCount)
		{
			return pages.Enumerate(pageCount).TakeWhile(pageNumber => pageNumber <= pageCount);
		}

		/// <summary>
		/// Opens the canvas a page would normally render onto — or, under
		/// <c>--fingerprint</c>, a canvas whose bytes are thrown away instead.
		/// </summary>
		/// <remarks>
		/// The render still has to happen, because a fingerprint is an attribute of
		/// the rendered page, not of the document — but the bytes it produces are
		/// never <paramref name="target"/>'s real content, the same way doc2text's
		/// own <c>--fingerprint</c> renders into a temporary stream it never keeps
		/// (<c>main.cpp</c>, <c>hidecontent</c>). <see cref="Configuration.OptionsValidator"/>
		/// already rejects every argument that would let someone rely on
		/// <paramref name="target"/> instead — <c>--out-dir</c>, <c>--out-file</c>,
		/// <c>--stdout</c>, <c>--view</c>, <c>--combine</c> — so it is only ever a
		/// plan's default-directory guess here, never a path the user asked for by
		/// name.
		/// </remarks>
		/// <summary>
		/// Renames a <c>web-safe</c> page to match whatever the engine actually wrote.
		/// </summary>
		/// <remarks>
		/// <c>web-safe</c> is the one format whose extension cannot be planned: the
		/// engine chooses PNG, GIF, JPEG or WebP per page, from the page's own content —
		/// one document here renders page 1 as JPEG, pages 2 and 3 as PNG and page 4 as
		/// GIF. The planner therefore names the file <c>.unknown</c>, and this puts the
		/// real name on it afterwards. Until now nothing did, so every page was left as
		/// <c>.unknown</c> and the format's whole point — the engine picking the best
		/// encoding per page — produced files nothing would open by extension.
		///
		/// The type comes from the engine identifying the file it just wrote, which is
		/// what doc2text does (<c>IGR_Get_File_Type</c> on the rendered page, then a
		/// rename). Mapped by MIME type rather than by the numeric format id doc2text
		/// switches on: the ids are internal, the MIME strings are public API, and
		/// <c>IdentifyFile</c> already returns them.
		///
		/// The destination goes through the same output policy as any other target.
		/// doc2text instead deletes whatever is in the way, which is exactly the
		/// overwrite bypass this round of review was about.
		/// </remarks>
		private void NameByRenderedType(string target, FormatSpec format, ConvertOptions options)
		{
			if (format.Token != "web-safe" || options.Fingerprint)
				return;

			string extension = ExtensionForMimeType(_session.Api.IdentifyFile(target).MimeType);
			if (extension == null)
				return;

			string renamed = Path.ChangeExtension(target, extension);
			if (string.Equals(renamed, target, StringComparison.Ordinal))
				return;

			VerifyPageTarget(renamed, options);
			_fileSystem.MoveFile(target, renamed);
		}

		/// <summary>The extension for an image the engine identified, or null if unknown.</summary>
		private static string ExtensionForMimeType(string mimeType)
		{
			switch (mimeType)
			{
				case "image/png": return "png";
				case "image/gif": return "gif";
				case "image/jpeg": return "jpg";
				case "image/webp": return "webp";
				default: return null;
			}
		}

		/// <summary>
		/// Applies the run's output policy to one resolved per-page target.
		/// </summary>
		/// <remarks>
		/// Skipped under <c>--fingerprint</c>, which renders to a null stream and writes
		/// no file at all, so an existing path of that name is not in anyone's way —
		/// the same exemption <c>ConvertCommand</c> makes for <c>VerifyTargets</c>.
		/// </remarks>
		private void VerifyPageTarget(string target, ConvertOptions options)
		{
			if (options.Fingerprint || _shared == null || _fileSystem == null)
				return;

			OutputPlanner.VerifyPageTarget(target, _shared, _fileSystem);
		}

		private IOutputCanvas MakeCanvas(string target, CanvasType canvasType, string canvasOptions, ConvertOptions options)
		{
			// The directory is already there: ConvertCommand creates it once per item,
			// because the engine opens this file itself and will not create one.
			return options.Fingerprint
				? _session.Api.MakeCanvas(Stream.Null, canvasType, canvasOptions)
				: _session.Api.MakeCanvas(target, canvasType, canvasOptions);
		}

		private void RenderPage(IOutputCanvas canvas, IDocument document, int pageNumber, ConvertOptions options, string displayName)
        {
            using IDocumentPage page = document.GetPage(pageNumber - 1);
            var renderOpts = options.RenderOptions ?? string.Empty;
            if (!string.IsNullOrEmpty(options.Watermark) && !File.Exists(options.Watermark.Replace('\\', '/')))
                renderOpts += (renderOpts.Length == 0 ? "" : ";") + "WATERMARK=" + options.Watermark;
            canvas.RenderPage(page, renderOpts, ApplyPageDecorations(pageNumber, options));
            DrawWatermark(canvas, page, options);

            if (options.Fingerprint)
                WriteFingerprint(page, pageNumber, displayName);
        }

		/// <summary>
		/// Writes <c>--fingerprint</c>'s one line for a page that has just been
		/// rendered.
		/// </summary>
		/// <remarks>
		/// Confirmed against doc2text's own <c>fingerPrintPage</c> (<c>main.cpp</c>):
		/// try the <c>IMAGE_FINGERPRINT_V2</c> page attribute first, fall back to the
		/// older <c>IMAGE_FINGERPRINT</c>, and print nothing when both are empty —
		/// doc2text only calls this after the page has rendered onto a canvas, which
		/// is why it lives here rather than in <see cref="StructureReport"/>: the
		/// fingerprint is a property of the rendered image, not of the document's
		/// layout.
		/// </remarks>
		internal void WriteFingerprint(IDocumentPage page, int pageNumber, string displayName)
		{
			string line = FormatFingerprint(displayName, pageNumber, page.GetAttribute("IMAGE_FINGERPRINT_V2"), page.GetAttribute("IMAGE_FINGERPRINT"));
			if (line != null)
				_console.WriteLine(line);
		}

		/// <summary>
		/// The formatting half of <see cref="WriteFingerprint"/>, kept as a pure
		/// function of the two attribute strings so it is testable without the engine.
		/// </summary>
		public static string FormatFingerprint(string displayName, int pageNumber, string fingerprintV2, string fingerprintV1)
		{
			string fingerprint = string.IsNullOrEmpty(fingerprintV2) ? fingerprintV1 : fingerprintV2;
			if (string.IsNullOrEmpty(fingerprint))
				return null;

			return $"{displayName}: page={pageNumber} fingerprint={fingerprint}";
		}

        private static bool HasPageDecorations(ConvertOptions options)
		{
			return !string.IsNullOrEmpty(options.Watermark) || options.Redactions.Count > 0 || options.FormValues.Count > 0;
		}

		/// <summary>Applies <c>--redact</c> and <c>--form</c> to one page before it is rendered.</summary>
		/// <remarks>
		/// Null when the page needs neither, so a run using none of them never builds a
		/// <see cref="RenderPageProperties"/> at all. <c>--watermark</c> is handled
		/// separately, by <see cref="DrawWatermark"/>.
		/// </remarks>
		internal static EnginePageDecorations ApplyPageDecorations(int pageNumber, ConvertOptions options)
		{
			var decorations = new EnginePageDecorations();

			foreach (RedactSpec redaction in options.Redactions.Where(r => r.Page == pageNumber))
			{
				decorations.Redactions.Add(new EngineRedaction
				{
					X = redaction.X,
					Y = redaction.Y,
					Width = redaction.Width,
					Height = redaction.Height
				});
			}

			foreach (FormValue form in options.FormValues.Where(f => f.Page == pageNumber))
				decorations.FormValues.Add(new EngineFormValue { Name = form.Name, Value = form.Value });

			return decorations.IsEmpty ? null : decorations;
		}

		internal void DrawWatermark(IOutputCanvas canvas, IDocumentPage page, ConvertOptions options)
        {
			if (string.IsNullOrEmpty(options.Watermark) || !File.Exists(options.Watermark.Replace('\\', '/')))
				return;

			var bytes = File.ReadAllBytes(options.Watermark.Replace('\\', '/'));
			canvas.DrawImage(0, 0, bytes, null);
        }
	}
}
