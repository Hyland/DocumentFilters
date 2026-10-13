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

// 'Type' is the binding's own name for the annotation kind enum, which collides
// with System.Type wherever it is not qualified.
using System.Drawing;
using AnnotationKind = Hyland.DocumentFilters.Annotations.Type;

namespace Hyland.DocumentFilters.Cli.Engine
{
	// -------------------------------------------------------------------------
	//  THE ENGINE SEAM
	//
	//  One interface method per Document Filters call, with no logic of its own.
	//  Everything above it — the conversion paths, the reports, the batch runner —
	//  talks to these interfaces, so all of it can be tested without the native
	//  library, a license, or a real document.
	//
	//  WHY THE RETURN TYPES ARE OURS AND NOT THE BINDING'S
	//
	//  A thinner wrapper would hand back Extractor, Canvas, Page and SubFile
	//  directly. That cannot be tested: all four have internal-only constructors,
	//  so no fake outside Hyland.DocumentFilters.dll can produce one, and their
	//  members are not virtual, so none of them can be subclassed either. The
	//  binding is built to be used, not stood in for.
	//
	//  So each of those four gets an interface here and a one-line adapter in
	//  DocumentFiltersEngine, and the small value types they expose — a format
	//  row, a bookmark, a hyperlink — get a plain record. Enums are reused from
	//  the binding as they are: an enum is public and constructible, so it costs
	//  a fake nothing.
	//
	//  The rule for anything added here: it maps one call, and it decides nothing.
	//  A method that chooses, filters or formats belongs above the seam, where it
	//  can be tested as a pure function.
	//
	//  An engine call stays a call. Where the binding exposes something as a method
	//  — GetWidth, getEOF, getFileType — so does this, even where the binding also
	//  offers a property wrapping it. A reader tracing a conversion should be able to
	//  read each of these as "this is where the engine is asked", and a property
	//  reads as a field that is simply there.
	// -------------------------------------------------------------------------

	/// <summary>The engine itself: what <c>DocumentFilters</c> offers before a document exists.</summary>
	public interface IDocumentFiltersEngine : IDisposable
	{
		/// <summary><c>DocumentFilters.Initialize</c>.</summary>
		/// <exception cref="EngineException">The library was not found, or the license was rejected.</exception>
		void Initialize(string licenseKey, string libraryPath);

		/// <summary>
		/// <c>GetExtractor</c> followed by <c>Extractor.Open</c>, which the engine
		/// applies as one step.
		/// </summary>
		/// <remarks>
		/// The two are not separable here on purpose: the open flags take effect
		/// <em>during</em> <c>Open</c>, so handing back an unopened extractor would
		/// invite exactly the mistake D18 exists to prevent.
		///
		/// <paramref name="flags"/> is the complete <c>IGR_</c> flag set, format bit
		/// included, and reaches the engine unaltered. Deliberately not an
		/// <c>OpenMode</c> plus flags: the binding's <c>Open(OpenMode, OpenType, ...)</c>
		/// overload masks out bits 16 to 31 and substitutes the format bit its mode
		/// implies, and every <c>IGR_FORMAT_*</c> value lives in that range, so a format
		/// passed that way is discarded rather than honoured. Whoever decides the format
		/// has to be the one that names it — see
		/// <see cref="Execution.ExtractorOpenSettings"/>.
		///
		/// <paramref name="password"/> is asked for the document's password when the
		/// engine requests one, and returns null to give up — which surfaces as an
		/// <see cref="EngineException"/> carrying <see cref="EngineError.Password"/>.
		///
		/// <paramref name="shouldCancel"/> is polled while the engine works, and
		/// returning true stops the open with <see cref="EngineError.Cancelled"/>. It
		/// covers <em>this call only</em>: the engine raises it as
		/// <c>IGR_OPEN_CALLBACK_ACTION_HEARTBEAT</c> during <c>Open</c>, and there is no
		/// equivalent anywhere else — <c>IGR_Render_Page</c> and
		/// <c>IGR_Make_Output_Canvas</c> take no callback, so a page loop cannot be
		/// interrupted once the document is open.
		/// </remarks>
		/// <exception cref="EngineException">The document could not be opened.</exception>
		IDocument OpenDocument(string path, int flags, string options, Func<string> password,
							   Func<bool> shouldCancel = null, Func<string, int> logLevelFor = null,
							   Action<int, string, string> onLogMessage = null);

		/// <summary>
		/// The same, reading the document from <paramref name="source"/> rather than
		/// from a path.
		/// </summary>
		/// <remarks>
		/// For the debug switches that have to control how bytes reach the engine:
		/// <c>--debug-use-memory-stream</c>, <c>--debug-truncate</c> and
		/// <c>--debug-measure-io</c>. Each of those is the stream itself doing something
		/// different, so none of them can be expressed as an option on the path form.
		/// <paramref name="displayName"/> is what failures are reported against, since a
		/// stream has no name of its own.
		/// </remarks>
		/// <exception cref="EngineException">The document could not be opened.</exception>
		IDocument OpenDocument(Stream source, string displayName, int flags, string options, Func<string> password,
							   Func<bool> shouldCancel = null, Func<string, int> logLevelFor = null,
							   Action<int, string, string> onLogMessage = null);

		/// <summary>
		/// Points the engine at a directory for its scratch files — <c>--temp-dir</c>.
		/// </summary>
		/// <remarks>
		/// <c>IGR_Multiplex(IGR_Multi_Set_Temp_Path, …)</c>, which is how doc2text does
		/// it. A library-wide setting rather than a per-document one, so it is applied
		/// once, immediately after <see cref="Initialize"/>.
		/// </remarks>
		void SetTempPath(string path);

		/// <summary>
		/// The engine's own memory report — <c>-x</c> and <c>--xx</c>.
		/// </summary>
		/// <remarks>
		/// <c>IGR_Get_Memory_Status</c>. Free-form text the engine composes, printed
		/// verbatim: its content is the engine's to change and is not contract.
		/// </remarks>
		string GetMemoryStatus();

		/// <summary>
		/// <c>GetExtractor</c> and <c>getFileType</c>, <em>without</em> opening: what the
		/// file is, without reading it.
		/// </summary>
		/// <remarks>
		/// Its own call rather than a mode of <see cref="OpenDocument"/> because it is a
		/// different operation on the engine — <c>identify</c> never opens a document, so
		/// it never needs a password, never applies open flags, and reports a file it
		/// could not have read. Returning plain data rather than an unopened document
		/// also stops a caller asking one for text it cannot give.
		/// </remarks>
		/// <exception cref="EngineException">The file could not be reached or recognised.</exception>
		EngineFileType IdentifyFile(string path);

		/// <summary>
		/// The same, from a stream — so the debug switches that change how bytes arrive
		/// reach <c>identify</c> too.
		/// </summary>
		/// <remarks>
		/// <c>--debug-truncate</c> is the one that earns this: what the engine makes of
		/// a prefix is exactly the question that switch exists to ask, and without a
		/// stream form it would be silently inert for this command.
		/// </remarks>
		/// <exception cref="EngineException">The file could not be reached or recognised.</exception>
		EngineFileType IdentifyFile(Stream source, string displayName);

		/// <summary><c>MakeOutputCanvas</c> onto a file.</summary>
		IOutputCanvas MakeCanvas(string target, CanvasType type, string options);

		/// <summary><c>MakeOutputCanvas</c> onto a stream, for <c>--stdout</c> and <c>--fingerprint</c>.</summary>
		IOutputCanvas MakeCanvas(Stream target, CanvasType type, string options);

		/// <summary><c>GetSupportedFormats</c>, for the <c>formats</c> command.</summary>
		IReadOnlyList<EngineFormat> GetSupportedFormats();

		/// <summary><c>GetAvailableOptions</c>, for the <c>options</c> command.</summary>
		IReadOnlyList<EngineOption> GetAvailableOptions();

		/// <summary>The binding's version, for the <c>version</c> command.</summary>
		string DescribeVersion();
	}

	/// <summary>One open document: <c>Extractor</c>.</summary>
	public interface IDocument : IDisposable
	{
		/// <summary><c>getFileType()</c>: the numeric format id.</summary>
		int GetFileType();

		/// <summary><c>getFileType(IGR_FORMAT_MIMETYPE)</c>.</summary>
		string GetMimeType();

		/// <summary><c>getFileType(IGR_FORMAT_LONG_NAME)</c>.</summary>
		string GetFormatName();

		/// <summary><c>getFileCapabilities()</c>: the text/subfiles/hd flag word.</summary>
		int GetFileCapabilities();

		/// <summary><c>getEOF()</c>.</summary>
		bool GetEOF();

		/// <summary><c>GetText(maxLength)</c>.</summary>
		string GetText(int maxLength);

		/// <summary><c>GetPageCount()</c>.</summary>
		int GetPageCount();

		/// <summary><c>GetPage(index)</c>, zero-based as the binding has it.</summary>
		IDocumentPage GetPage(int pageIndex);

		/// <summary><c>GetFirstSubFile</c>/<c>GetNextSubFile</c>, drained into a list.</summary>
		/// <remarks>
		/// Drained rather than streamed because the binding's cursor is positional
		/// state on the extractor: interleaving a walk with anything else that touches
		/// the same handle silently loses entries. A list makes that impossible.
		/// </remarks>
		IReadOnlyList<ISubFileEntry> SubFiles { get; }

		/// <summary><c>GetFirstImage</c>/<c>GetNextImage</c>, drained into a list.</summary>
		IReadOnlyList<ISubFileEntry> Images { get; }

		/// <summary><c>RootBookmark</c>, or null when the document has no outline.</summary>
		EngineBookmark RootBookmark { get; }

		/// <summary><c>MakeDocPack</c> onto a file.</summary>
		void MakeDocPack(string filename, string options);

		/// <summary><c>MakeDocPack</c> onto a stream.</summary>
		void MakeDocPack(Stream destination, string options);
	}

	/// <summary>One page of an open document: <c>Page</c>.</summary>
	public interface IDocumentPage : IDisposable
	{
		/// <summary><c>GetWidth()</c>.</summary>
		int GetWidth();

		/// <summary><c>GetHeight()</c>.</summary>
		int GetHeight();

		/// <summary><c>GetAttribute(name)</c>; empty or null when the page has no such attribute.</summary>
		string GetAttribute(string name);

		IReadOnlyList<EngineHyperlink> Hyperlinks { get; }

		IReadOnlyList<EngineFormElement> FormElements { get; }

		IReadOnlyList<EngineAnnotation> Annotations { get; }

		/// <summary><c>GetRootPageElement()</c>, or null when the page exposes no element tree.</summary>
		EnginePageElement GetRootPageElement();
	}

	/// <summary>An output canvas: <c>Canvas</c>.</summary>
	public interface IOutputCanvas : IDisposable
	{
		/// <summary><c>RenderPage</c>. <paramref name="decorations"/> is null when the page needs none.</summary>
		void RenderPage(IDocumentPage page, string options, EnginePageDecorations decorations);

		/// <summary><c>RenderPages(extractor)</c>: the whole document in one call.</summary>
		void RenderPages(IDocument document);

		/// <summary><c>DrawImage</c>, for <c>--watermark</c>.</summary>
		void DrawImage(int x, int y, byte[] image, string options);
	}

	/// <summary>One entry inside a container: <c>SubFile</c>.</summary>
	/// <remarks>
	/// Disposable because the binding's <c>SubFile</c> is a handle, not a row of data.
	/// The entry owns that handle for its whole life, including the <see cref="IDocument"/>
	/// that <see cref="Open"/> returns — in the binding those are the same object, so
	/// the document must not be disposed separately.
	/// </remarks>
	public interface ISubFileEntry : IDisposable
	{
		string Id { get; }

		/// <summary>The name as the container gives it — untrusted, and never used as a path unchecked.</summary>
		string Name { get; }

		long Size { get; }

		DateTime? Modified { get; }

		/// <summary><c>SubFile.Open</c>: the entry as a document in its own right.</summary>
		/// <exception cref="EngineException">The entry could not be opened.</exception>
		IDocument Open(int flags, string options);

		/// <summary><c>CopyTo(filename)</c>: the entry's bytes, untouched.</summary>
		void CopyTo(string path);
	}

	// ------------------------------------------------------------------ values

	/// <summary>What a file is, without opening it.</summary>
	public sealed class EngineFileType
	{
		/// <summary>The numeric format id; 0 when the engine cannot place the file.</summary>
		public int Id { get; set; }

		public string MimeType { get; set; }

		/// <summary>The format's long name, e.g. <c>Microsoft Word 2007</c>.</summary>
		public string Name { get; set; }

		/// <summary>The text/subfiles/hd flag word.</summary>
		public int Capabilities { get; set; }
	}

	/// <summary>One row of <c>GetSupportedFormats</c>.</summary>
	public sealed class EngineFormat
	{
		public int Id { get; set; }

		public string ShortName { get; set; }

		public string DisplayName { get; set; }
	}

	/// <summary>One row of <c>GetAvailableOptions</c>.</summary>
	public sealed class EngineOption
	{
		public string DisplayName { get; set; }

		/// <summary>The value's type, as the engine names it: <c>int</c>, <c>string</c>, <c>bool</c>.</summary>
		public string Type { get; set; }

		public string Description { get; set; }

		public int Flags { get; set; }

		public string DefaultValue { get; set; }
	}

	/// <summary>One node of the document outline.</summary>
	public sealed class EngineBookmark
	{
		public string Title { get; set; }

		public int Level { get; set; }

		/// <summary>Zero-based, as the binding reports it.</summary>
		public int PageIndex { get; set; }

		public IReadOnlyList<EngineBookmark> Children { get; set; } = new List<EngineBookmark>();
	}

	/// <summary>One hyperlink on a page.</summary>
	public sealed class EngineHyperlink
	{
		public Hyperlink.ActionType Type { get; set; }

		public string Uri { get; set; }

		public string Ref { get; set; }

		public int X { get; set; }

		public int Y { get; set; }

		public int Width { get; set; }

		public int Height { get; set; }
	}

	/// <summary>One form element on a page.</summary>
	public sealed class EngineFormElement
	{
		public FormElement.ElementType Type { get; set; }

		public string Name { get; set; }

		public int X { get; set; }

		public int Y { get; set; }

		public int Width { get; set; }

		public int Height { get; set; }
	}

	/// <summary>One annotation on a page.</summary>
	public sealed class EngineAnnotation
	{
		public AnnotationKind Type { get; set; }

		public string Title { get; set; }

		public string Text { get; set; }
	}

	/// <summary>One node of a page's element tree.</summary>
	public sealed class EnginePageElement
	{
		public PageElementType Type { get; set; }

		public int Depth { get; set; }

		/// <summary>The binding reports element geometry as a float rectangle, not four ints.</summary>
		public RectangleF Bounds { get; set; }

		public IReadOnlyList<EnginePageElement> Children { get; set; } = new List<EnginePageElement>();

		/// <summary>This node and every node beneath it, depth first.</summary>
		public IEnumerable<EnginePageElement> AllChildren()
		{
			foreach (EnginePageElement child in Children)
			{
				yield return child;

				foreach (EnginePageElement descendant in child.AllChildren())
					yield return descendant;
			}
		}
	}

	/// <summary>
	/// What <c>--redact</c> and <c>--form</c> add to one page before it renders.
	/// </summary>
	/// <remarks>
	/// Carried as data rather than as the binding's <c>RenderPageProperties</c> for
	/// the same reason as everything else here: the adapter builds that object, and
	/// a fake can assert on these values without one.
	/// </remarks>
	public sealed class EnginePageDecorations
	{
		public List<EngineRedaction> Redactions { get; } = new List<EngineRedaction>();

		public List<EngineFormValue> FormValues { get; } = new List<EngineFormValue>();

		public bool IsEmpty => Redactions.Count == 0 && FormValues.Count == 0;
	}

	/// <summary>One <c>--redact</c> region, in page coordinates.</summary>
	public sealed class EngineRedaction
	{
		public int X { get; set; }

		public int Y { get; set; }

		public int Width { get; set; }

		public int Height { get; set; }
	}

	/// <summary>One <c>--form</c> assignment.</summary>
	public sealed class EngineFormValue
	{
		public string Name { get; set; }

		public string Value { get; set; }
	}
}
