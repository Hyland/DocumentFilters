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

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Tests.Fakes
{
	/// <summary>
	/// A Document Filters engine that answers from a script instead of a library.
	/// </summary>
	/// <remarks>
	/// The engine is the one dependency the execution paths cannot be tested around:
	/// it needs a license, a native library, and a real document that actually
	/// exhibits whatever is being tested. Every failure this thing can produce is a
	/// failure the real engine produces too — the codes come from
	/// <see cref="EngineError"/>, which mirrors <c>ISYS11dfConstants</c> — so a test
	/// here is a statement about how the CLI handles a real outcome.
	///
	/// Documents are added by path. Anything not added fails the way the engine fails
	/// for a file it cannot read, so "the document is not there" needs no setup.
	/// </remarks>
	public sealed class FakeDocumentFiltersEngine : IDocumentFiltersEngine
	{
		private readonly Dictionary<string, FakeDocument> _documents = new Dictionary<string, FakeDocument>(StringComparer.Ordinal);
		private readonly Dictionary<string, EngineError> _openFailures = new Dictionary<string, EngineError>(StringComparer.Ordinal);

		/// <summary>Every call made, in order, for asserting on what the CLI asked the engine to do.</summary>
		public List<string> Calls { get; } = new List<string>();

		/// <summary>Canvases handed out, in order of creation.</summary>
		public List<FakeCanvas> Canvases { get; } = new List<FakeCanvas>();

		/// <summary>Set to fail <see cref="Initialize"/>, the way a bad license or a missing library does.</summary>
		public EngineError? InitialisationFailure { get; set; }

		/// <summary>Set to fail every <see cref="MakeCanvas(string, CanvasType, string)"/>.</summary>
		public EngineError? CanvasFailure { get; set; }

		/// <summary>
		/// Set to fail <see cref="GetSupportedFormats"/> and <see cref="GetAvailableOptions"/>.
		/// </summary>
		/// <remarks>
		/// The catalogues are the two commands that read no document, so there is no
		/// per-input failure to script — the whole call either answers or does not.
		/// </remarks>
		public EngineError? CatalogFailure { get; set; }

		public List<EngineFormat> Formats { get; } = new List<EngineFormat>();

		public List<EngineOption> Options { get; } = new List<EngineOption>();

		public bool IsDisposed { get; private set; }

		// ------------------------------------------------------------- building

		/// <summary>Adds a document the engine can open.</summary>
		public FakeDocument AddDocument(string path, string text = "")
		{
			var document = new FakeDocument(path) { Text = text };
			_documents[path] = document;
			return document;
		}

		/// <summary>
		/// Makes opening <paramref name="path"/> fail with a given engine error.
		/// </summary>
		/// <remarks>
		/// <see cref="EngineError.Password"/> is the one worth calling out: the real
		/// engine reports a wrong password and a refused prompt identically, so a test
		/// using it covers both.
		/// </remarks>
		public FakeDocumentFiltersEngine FailToOpen(string path, EngineError error)
		{
			_openFailures[path] = error;
			return this;
		}

		/// <summary>Adds a document that opens only when the right password is supplied.</summary>
		public FakeDocument AddProtectedDocument(string path, string password, string text = "")
		{
			FakeDocument document = AddDocument(path, text);
			document.Password = password;
			return document;
		}

		// -------------------------------------------------------------- engine

		public void Initialize(string licenseKey, string libraryPath)
		{
			Calls.Add($"Initialize({licenseKey ?? "null"},{libraryPath ?? "null"})");

			if (InitialisationFailure.HasValue)
				throw Failure(InitialisationFailure.Value, "initialisation failed");
		}

		/// <summary>Run at the start of an open, before the cancellation poll.</summary>
		public Action DuringOpen { get; set; }

		public IDocument OpenDocument(string path, int flags, string options, Func<string> password,
									  Func<bool> shouldCancel = null, Func<string, int> logLevelFor = null,
									  Action<int, string, string> onLogMessage = null)
		{
			Calls.Add($"OpenDocument({path},{flags},{options ?? string.Empty}{(shouldCancel != null ? ",cancellable" : string.Empty)})");

			// Runs before the cancellation poll, so a test can make something true
			// *during* the open rather than before it. Ctrl+C arriving mid-open is the
			// only way to reach the cancellation path at all — BatchRunner checks for an
			// interrupt between documents, so one requested earlier stops the run before
			// anything opens.
			DuringOpen?.Invoke();

			// The engine polls this while it parses; the fake polls it once, which is
			// enough to prove a caller wired it up and translates the refusal.
			if (shouldCancel != null && shouldCancel())
				throw Failure(EngineError.Cancelled, "cancelled while opening " + path);

			if (_openFailures.TryGetValue(path, out EngineError scripted))
				throw Failure(scripted, "could not open " + path);

			if (!_documents.TryGetValue(path, out FakeDocument document))
				throw Failure(EngineError.OpenError, "no such document: " + path);

			if (document.Password != null)
			{
				// The real engine asks through the callback and turns a refusal into
				// IGR_E_PASSWORD out of Open, so a wrong answer and no answer land here
				// the same way.
				string offered = password?.Invoke();
				if (offered != document.Password)
					throw Failure(EngineError.Password, "password required for " + path);
			}

			// The engine asks the level per module, then hands back messages. Answering
			// once with a known module is enough to show a caller wired both halves.
			if (logLevelFor != null && onLogMessage != null)
			{
				int level = logLevelFor("pdf");
				if (level > 0)
					onLogMessage(level, "pdf", "opened " + path);
			}

			document.OpenFlags = flags;
			document.OpenOptions = options;
			document.WasOpened = true;
			return document;
		}

		/// <summary>
		/// The stream form. Records the stream's length, which is what the debug
		/// switches that use it are actually about.
		/// </summary>
		public IDocument OpenDocument(Stream source, string displayName, int flags, string options,
									  Func<string> password, Func<bool> shouldCancel = null,
									  Func<string, int> logLevelFor = null, Action<int, string, string> onLogMessage = null)
		{
			Calls.Add($"OpenDocument(<stream:{source.Length}>,{displayName},{flags},{options ?? string.Empty}"
					  + (shouldCancel != null ? ",cancellable)" : ")"));

			return OpenDocument(displayName, flags, options, password, shouldCancel, logLevelFor, onLogMessage);
		}

		public void SetTempPath(string path)
		{
			Calls.Add($"SetTempPath({path})");
			TempPath = path;
		}

		/// <summary>The last path <see cref="SetTempPath"/> was given, or null.</summary>
		public string TempPath { get; private set; }

		public string GetMemoryStatus()
		{
			Calls.Add("GetMemoryStatus()");
			return MemoryStatus;
		}

		/// <summary>What <see cref="GetMemoryStatus"/> answers.</summary>
		public string MemoryStatus { get; set; } = "heap: 0 bytes";

		/// <summary>The stream form, for the debug switches that reach identify too.</summary>
		public EngineFileType IdentifyFile(Stream source, string displayName)
		{
			Calls.Add($"IdentifyFile(<stream:{source.Length}>,{displayName})");
			return IdentifyFile(displayName);
		}

		public EngineFileType IdentifyFile(string path)
		{
			Calls.Add($"IdentifyFile({path})");

			if (_openFailures.TryGetValue(path, out EngineError scripted))
				throw Failure(scripted, "could not identify " + path);

			if (!_documents.TryGetValue(path, out FakeDocument document))
				throw Failure(EngineError.OpenError, "no such document: " + path);

			return new EngineFileType
			{
				Id = document.FileType,
				MimeType = document.MimeType,
				Name = document.FormatName,
				Capabilities = document.Capabilities
			};
		}

		public IOutputCanvas MakeCanvas(string target, CanvasType type, string options)
		{
			Calls.Add($"MakeCanvas({target},{type},{options ?? string.Empty})");

			if (CanvasFailure.HasValue)
				throw Failure(CanvasFailure.Value, "could not create canvas at " + target);

			var canvas = new FakeCanvas(target, type, options);
			Canvases.Add(canvas);
			return canvas;
		}

		public IOutputCanvas MakeCanvas(Stream target, CanvasType type, string options)
		{
			Calls.Add($"MakeCanvas(<stream>,{type},{options ?? string.Empty})");

			if (CanvasFailure.HasValue)
				throw Failure(CanvasFailure.Value, "could not create canvas on a stream");

			var canvas = new FakeCanvas(null, type, options);
			Canvases.Add(canvas);
			return canvas;
		}

		public IReadOnlyList<EngineFormat> GetSupportedFormats()
		{
			Calls.Add("GetSupportedFormats()");

			if (CatalogFailure.HasValue)
				throw Failure(CatalogFailure.Value, "catalogue unavailable");

			return Formats;
		}

		public IReadOnlyList<EngineOption> GetAvailableOptions()
		{
			Calls.Add("GetAvailableOptions()");

			if (CatalogFailure.HasValue)
				throw Failure(CatalogFailure.Value, "catalogue unavailable");

			return Options;
		}

		public string DescribeVersion() => "0.0.0-fake";

		public void Dispose()
		{
			IsDisposed = true;
		}

		/// <summary>An engine failure with the code the real library would report.</summary>
		internal static EngineException Failure(EngineError error, string message)
		{
			return new EngineException(error, (int)error, message);
		}
	}

	/// <summary>A document the fake engine can hand out.</summary>
	public sealed class FakeDocument : IDocument
	{
		private int _textOffset;

		public FakeDocument(string path)
		{
			Path = path;
		}

		public string Path { get; }

		/// <summary>What <c>GetText</c> streams out, in 4k-ish chunks as the real one does.</summary>
		public string Text { get; set; } = string.Empty;

		/// <summary>When set, <c>OpenDocument</c> demands exactly this password.</summary>
		public string Password { get; set; }

		/// <summary>Scripted answers. The interface members below are the calls over them.</summary>
		public int FileType { get; set; } = 254;

		public string MimeType { get; set; } = "application/octet-stream";

		public string FormatName { get; set; } = "Fake Format";

		public int Capabilities { get; set; }

		public int GetFileType()
		{
			Check("GetFileType");
			return FileType;
		}

		public string GetMimeType()
		{
			Check("GetMimeType");
			return MimeType;
		}

		public string GetFormatName()
		{
			Check("GetFormatName");
			return FormatName;
		}

		public int GetFileCapabilities()
		{
			Check("GetFileCapabilities");
			return Capabilities;
		}

		public int PageCount { get; set; }

		public List<FakePage> Pages { get; } = new List<FakePage>();

		public List<FakeSubFile> SubFileEntries { get; } = new List<FakeSubFile>();

		public List<FakeSubFile> ImageEntries { get; } = new List<FakeSubFile>();

		public EngineBookmark Bookmarks { get; set; }

		/// <summary>Set to make a call on the open document fail.</summary>
		public EngineError? ReadFailure { get; set; }

		/// <summary>Which call <see cref="ReadFailure"/> applies to; null means all of them.</summary>
		public string ReadFailureOn { get; set; }

		public bool WasOpened { get; set; }

		public bool IsDisposed { get; private set; }

		public int OpenFlags { get; set; }

		public string OpenOptions { get; set; }

		public List<string> DocPacksWritten { get; } = new List<string>();

		public bool GetEOF()
		{
			Check("GetEOF");
			return _textOffset >= Text.Length;
		}

		public string GetText(int maxLength)
		{
			Check("GetText");

			int take = Math.Min(maxLength, Text.Length - _textOffset);
			if (take <= 0)
				return string.Empty;

			string chunk = Text.Substring(_textOffset, take);
			_textOffset += take;
			return chunk;
		}

		public int GetPageCount()
		{
			Check("GetPageCount");
			return Pages.Count > 0 ? Pages.Count : PageCount;
		}

		public IDocumentPage GetPage(int pageIndex)
		{
			Check("GetPage");

			if (pageIndex < 0 || pageIndex >= GetPageCount())
				throw FakeDocumentFiltersEngine.Failure(EngineError.NotFound, $"no page {pageIndex} in {Path}");

			// A document given only a page count, not page contents, still renders.
			while (Pages.Count <= pageIndex)
				Pages.Add(new FakePage());

			return Pages[pageIndex];
		}

		public IReadOnlyList<ISubFileEntry> SubFiles
		{
			get
			{
				Check("SubFiles");
				return SubFileEntries.Cast<ISubFileEntry>().ToList();
			}
		}

		public IReadOnlyList<ISubFileEntry> Images
		{
			get
			{
				Check("Images");
				return ImageEntries.Cast<ISubFileEntry>().ToList();
			}
		}

		public EngineBookmark RootBookmark
		{
			get
			{
				Check("RootBookmark");
				return Bookmarks;
			}
		}

		public void MakeDocPack(string filename, string options)
		{
			Check("MakeDocPack");
			DocPacksWritten.Add(filename);
		}

		public void MakeDocPack(Stream destination, string options)
		{
			Check("MakeDocPack");
			DocPacksWritten.Add("<stream>");
		}

		public void Dispose()
		{
			IsDisposed = true;
		}

		private void Check(string call)
		{
			if (ReadFailure.HasValue && (ReadFailureOn == null || ReadFailureOn == call))
				throw FakeDocumentFiltersEngine.Failure(ReadFailure.Value, $"{call} failed on {Path}");
		}
	}

	/// <summary>One page of a <see cref="FakeDocument"/>.</summary>
	public sealed class FakePage : IDocumentPage
	{
		/// <summary>Scripted answers, read back through <see cref="GetWidth"/> and <see cref="GetHeight"/>.</summary>
		public int Width { get; set; } = 612;

		public int Height { get; set; } = 792;

		public int GetWidth() => Width;

		public int GetHeight() => Height;

		public Dictionary<string, string> Attributes { get; } = new Dictionary<string, string>(StringComparer.Ordinal);

		public List<EngineHyperlink> HyperlinkList { get; } = new List<EngineHyperlink>();

		public List<EngineFormElement> FormElementList { get; } = new List<EngineFormElement>();

		public List<EngineAnnotation> AnnotationList { get; } = new List<EngineAnnotation>();

		public EnginePageElement RootElement { get; set; }

		public bool IsDisposed { get; private set; }

		public string GetAttribute(string name)
		{
			return Attributes.TryGetValue(name, out string value) ? value : string.Empty;
		}

		public IReadOnlyList<EngineHyperlink> Hyperlinks => HyperlinkList;

		public IReadOnlyList<EngineFormElement> FormElements => FormElementList;

		public IReadOnlyList<EngineAnnotation> Annotations => AnnotationList;

		public EnginePageElement GetRootPageElement() => RootElement;

		public void Dispose() => IsDisposed = true;
	}

	/// <summary>A canvas that records what was drawn on it instead of writing bytes.</summary>
	public sealed class FakeCanvas : IOutputCanvas
	{
		public FakeCanvas(string target, CanvasType type, string options)
		{
			Target = target;
			Type = type;
			Options = options;
		}

		/// <summary>The path it was opened on, or null for a stream canvas.</summary>
		public string Target { get; }

		public CanvasType Type { get; }

		public string Options { get; }

		/// <summary>Pages rendered onto it, in order.</summary>
		public List<IDocumentPage> RenderedPages { get; } = new List<IDocumentPage>();

		/// <summary>Decorations passed with each rendered page; null where none were given.</summary>
		public List<EnginePageDecorations> Decorations { get; } = new List<EnginePageDecorations>();

		/// <summary>Documents rendered whole, through the one-call path.</summary>
		public List<IDocument> RenderedDocuments { get; } = new List<IDocument>();

		public int ImagesDrawn { get; private set; }

		public bool IsDisposed { get; private set; }

		/// <summary>Set to fail rendering, the way a full disk or a missing font does.</summary>
		public EngineError? RenderFailure { get; set; }

		public void RenderPage(IDocumentPage page, string options, EnginePageDecorations decorations)
		{
			if (RenderFailure.HasValue)
				throw FakeDocumentFiltersEngine.Failure(RenderFailure.Value, "render failed");

			RenderedPages.Add(page);
			Decorations.Add(decorations);
		}

		public void RenderPages(IDocument document)
		{
			if (RenderFailure.HasValue)
				throw FakeDocumentFiltersEngine.Failure(RenderFailure.Value, "render failed");

			RenderedDocuments.Add(document);
		}

		public void DrawImage(int x, int y, byte[] image, string options) => ImagesDrawn++;

		public void Dispose() => IsDisposed = true;
	}

	/// <summary>One entry inside a fake container.</summary>
	public sealed class FakeSubFile : ISubFileEntry
	{
		public FakeSubFile(string name, string id = "0001")
		{
			Name = name;
			Id = id;
		}

		public string Id { get; set; }

		public string Name { get; set; }

		public long Size { get; set; }

		public DateTime? Modified { get; set; }

		/// <summary>What this entry is when opened, for nested containers.</summary>
		public FakeDocument Content { get; set; }

		/// <summary>Set to fail <see cref="Open"/>.</summary>
		public EngineError? OpenFailure { get; set; }

		/// <summary>Set to fail <see cref="CopyTo"/>, the way an unreadable entry does.</summary>
		public EngineError? CopyFailure { get; set; }

		/// <summary>Paths this entry was copied to.</summary>
		public List<string> CopiedTo { get; } = new List<string>();

		public bool IsDisposed { get; private set; }

		public IDocument Open(int flags, string options)
		{
			if (OpenFailure.HasValue)
				throw FakeDocumentFiltersEngine.Failure(OpenFailure.Value, "could not open subfile " + Name);

			return Content ?? new FakeDocument(Name);
		}

		public void CopyTo(string path)
		{
			if (CopyFailure.HasValue)
				throw FakeDocumentFiltersEngine.Failure(CopyFailure.Value, "could not copy subfile " + Name);

			CopiedTo.Add(path);
		}

		public void Dispose() => IsDisposed = true;
	}
}
