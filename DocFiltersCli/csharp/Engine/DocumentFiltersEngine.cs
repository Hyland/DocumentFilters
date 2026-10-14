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

using System.Diagnostics;
using System.Drawing;
using System.Runtime.InteropServices;

namespace Hyland.DocumentFilters.Cli.Engine
{
	// -------------------------------------------------------------------------
	//  THE REAL ENGINE
	//
	//  The only file in the project that touches Hyland.DocumentFilters types.
	//  Every method here is one binding call plus, where the binding hands back a
	//  type a test cannot construct, a copy into the plain values from
	//  IDocumentFiltersEngine.
	//
	//  Nothing here decides anything. If a method in this file grows an 'if' that
	//  is not null-handling or error translation, it belongs above the seam.
	// -------------------------------------------------------------------------

	/// <summary>
	/// <see cref="IDocumentFiltersEngine"/> over the real library.
	/// </summary>
	public sealed class DocumentFiltersEngine : IDocumentFiltersEngine
	{
		private readonly DocumentFilters _api = new DocumentFilters();

		/// <summary>Remembered from <see cref="Initialize"/>, for <see cref="DescribeVersion"/>.</summary>
		private string _libraryPath;

		public void Initialize(string licenseKey, string libraryPath)
		{
			EngineCall.Run(() => _api.Initialize(licenseKey, libraryPath));
			_libraryPath = libraryPath;
		}

		public IDocument OpenDocument(string path, int flags, string options, Func<string> password,
									  Func<bool> shouldCancel = null, Func<string, int> logLevelFor = null,
									  Action<int, string, string> onLogMessage = null)
		{
			return EngineCall.Run(() => Open(_api.GetExtractor(path), flags, options, password, shouldCancel,
											 logLevelFor, onLogMessage));
		}

		public IDocument OpenDocument(Stream source, string displayName, int flags, string options,
									  Func<string> password, Func<bool> shouldCancel = null,
									  Func<string, int> logLevelFor = null, Action<int, string, string> onLogMessage = null)
		{
			return EngineCall.Run(() => Open(_api.GetExtractor(source), flags, options, password, shouldCancel,
											 logLevelFor, onLogMessage));
		}

		/// <summary>
		/// Opens an extractor the caller has already obtained, whether from a path or a
		/// stream.
		/// </summary>
		/// <remarks>
		/// Shared so the callback exists once: a second copy would be a second place
		/// for the password refusal and the cancellation answer to drift apart.
		/// </remarks>
		private static IDocument Open(Extractor extractor, int flags, string options,
									  Func<string> password, Func<bool> shouldCancel,
									  Func<string, int> logLevelFor, Action<int, string, string> onLogMessage)
		{
			// The engine asks for a password through this callback rather than by
			// returning: refusing it here is what turns into IGR_E_PASSWORD out of
			// Open, which EngineCall then translates like any other failure.
			Func<OpenCallback, int> callback = request =>
			{
				if (request.Action == ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_PASSWORD)
				{
					string candidate = password?.Invoke();
					if (candidate == null)
						return ISYS11dfConstants.IGR_E_PASSWORD;
					request.Password.password = candidate;
				}
				else if (request.Action == ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_HEARTBEAT
						 && shouldCancel != null && shouldCancel())
				{
					// The engine's one interruption point, raised periodically while
					// it parses. doc2text answers the same action the same way.
					return ISYS11dfConstants.IGR_CANCELLED;
				}
				else if (request.Action == ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOG_LEVEL && logLevelFor != null)
				{
					// The engine asks per module how much to log, so the --log-modules
					// filter is applied by answering none for a module nobody named.
					request.LogLevel.result = (uint)logLevelFor(request.LogLevel.module);
				}
				else if (request.Action == ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOG_MESSAGE && onLogMessage != null)
				{
					onLogMessage((int)request.LogMessage.log_level, request.LogMessage.module, request.LogMessage.message);
				}
				return ISYS11dfConstants.IGR_OK;
			};

			// The flags-only overload, so the caller's format bit survives. The
			// OpenMode overload would clear bits 16 to 31 and substitute its own.
			extractor.Open((OpenType)flags, options ?? string.Empty, callback);
			return new DocumentAdapter(extractor);
		}

		public void SetTempPath(string path)
		{
			EngineCall.Run(() =>
			{
				var error = new Error_Control_Block();
				uint unused = 0;
				var buffer = System.Runtime.InteropServices.Marshal.StringToHGlobalAnsi(path);
				try
				{
					uint address = (uint)(ulong)buffer;
					ISYS11df.IGR_Multiplex(ISYS11dfConstants.IGR_Multi_Set_Temp_Path, ref address, ref unused, ref error);
				}
				finally
				{
					System.Runtime.InteropServices.Marshal.FreeHGlobal(buffer);
				}
			});
		}

		/// <summary>
		/// <c>IGR_Get_Memory_Status</c>, declared here rather than called through the
		/// binding.
		/// </summary>
		/// <remarks>
		/// <b>The binding's own wrapper cannot be called at all.</b> It declares the
		/// length parameter as <c>[MarshalAs(UnmanagedType.SysUInt)] uint</c>, which is
		/// not a legal pairing — <c>SysUInt</c> means a pointer-sized integer and has to
		/// be <c>UIntPtr</c> — so <c>DocumentFilters.GetMemoryStatus()</c> throws
		/// <c>MarshalDirectiveException</c> on every call, before reaching the engine.
		/// Redeclaring the entry point with the default marshalling for <c>uint</c> is
		/// the only way to reach it from managed code, and is the one place in this
		/// project that goes around the binding rather than through it.
		/// </remarks>
		[System.Runtime.InteropServices.DllImport("ISYS11df", EntryPoint = "IGR_Get_Memory_Status",
												  CharSet = System.Runtime.InteropServices.CharSet.Ansi)]
		private static extern uint IGR_Get_Memory_Status(System.Text.StringBuilder buffer, uint length);

		public string GetMemoryStatus()
		{
			return EngineCall.Run(() =>
			{
				var buffer = new System.Text.StringBuilder(4096);
				uint written = IGR_Get_Memory_Status(buffer, (uint)buffer.Capacity);

				// The engine reports how much it wrote; a zero means it had nothing to
				// say rather than that the call failed.
				return written == 0 ? string.Empty : buffer.ToString();
			});
		}

		public EngineFileType IdentifyFile(string path)
		{
			return EngineCall.Run(() => Identify(_api.GetExtractor(path)));
		}

		public EngineFileType IdentifyFile(Stream source, string displayName)
		{
			return EngineCall.Run(() => Identify(_api.GetExtractor(source)));
		}

		/// <summary>Reads what a file is, without opening it.</summary>
		private static EngineFileType Identify(Extractor extractor)
		{
			using (extractor)
			{
				return new EngineFileType
				{
					Id = extractor.getFileType(),
					MimeType = extractor.getFileType(IGRFormatWhat.IGR_FORMAT_MIMETYPE),
					Name = extractor.getFileType(IGRFormatWhat.IGR_FORMAT_LONG_NAME),
					Capabilities = extractor.getFileCapabilities()
				};
			}
		}

		public IOutputCanvas MakeCanvas(string target, CanvasType type, string options)
		{
			return EngineCall.Run(() => (IOutputCanvas)new CanvasAdapter(_api.MakeOutputCanvas(target, type, options ?? string.Empty)));
		}

		public IOutputCanvas MakeCanvas(Stream target, CanvasType type, string options)
		{
			return EngineCall.Run(() => (IOutputCanvas)new CanvasAdapter(_api.MakeOutputCanvas(target, type, options ?? string.Empty)));
		}

		public IReadOnlyList<EngineFormat> GetSupportedFormats()
		{
			return EngineCall.Run(() =>
			{
				var formats = new List<EngineFormat>();
				foreach (Format format in _api.GetSupportedFormats())
					formats.Add(new EngineFormat { Id = format.Id, ShortName = format.ShortName, DisplayName = format.DisplayName });

				return (IReadOnlyList<EngineFormat>)formats;
			});
		}

		public IReadOnlyList<EngineOption> GetAvailableOptions()
		{
			return EngineCall.Run(() =>
			{
				var options = new List<EngineOption>();
				foreach (Option option in _api.GetAvailableOptions())
				{
					options.Add(new EngineOption
					{
						DisplayName = option.DisplayName,
						Type = option.Type,
						Description = option.Description,
						DefaultValue = option.DefaultValue,
						Flags = option.Flags
					});
				}

				return (IReadOnlyList<EngineOption>)options;
			});
		}

		/// <summary>
		/// The native library's own file version, read from disk the way doc2text's
		/// <c>showVersion</c> does.
		/// </summary>
		/// <remarks>
		/// The managed binding exposes no native version call of its own, so there is
		/// nothing to be faithful to by calling one; reading <c>ISYS11df.dll</c>'s
		/// version resource directly is the closest thing this tool has to "the
		/// engine's own version", and it is what doc2text's own version report does
		/// for the same library.
		/// </remarks>
		/// <summary>
		/// The version of the native library this process is actually running.
		/// </summary>
		/// <remarks>
		/// Asked of the <em>loaded module</em> rather than of a path this class guesses.
		/// Guessing does not work: the engine is normally restored by NuGet into
		/// <c>runtimes/&lt;rid&gt;/native/</c>, not next to the executable, so looking
		/// beside the executable finds nothing on an ordinary build — and finding
		/// nothing used to fall back to the <em>binding assembly's</em> version, which
		/// reported a plausible-looking <c>1.0.0.0</c> as though it were the engine's.
		/// A wrong answer in the right shape is worse than no answer, so the fallback is
		/// now "unknown". Reading the loaded module is also the only answer that stays
		/// right when <c>--library</c> points somewhere else, or when the loader found
		/// the library on <c>PATH</c>.
		///
		/// Windows only in practice: <c>FileVersionInfo</c> reads the PE version
		/// resource, and there is no ELF or Mach-O equivalent to read, so a Unix build
		/// reports "unknown" — which is what the C++ port does there too.
		/// </remarks>
		public string DescribeVersion()
		{
			foreach (string candidate in LibraryPathCandidates())
			{
				try
				{
					if (!File.Exists(candidate))
						continue;

					string version = FileVersionInfo.GetVersionInfo(candidate).FileVersion;
					if (!string.IsNullOrEmpty(version))
						return version;
				}
				catch
				{
					// An unreadable candidate is not an error worth reporting from
					// `version`; try the next one and fall through to "unknown".
				}
			}

			return "unknown";
		}

		/// <summary>
		/// Where the native library might be, most authoritative first.
		/// </summary>
		/// <remarks>
		/// The loaded module comes first because it is the only entry that is a fact
		/// rather than a guess. The probes after it exist for the case where
		/// <c>version</c> runs before anything opened a document, so nothing is loaded
		/// yet.
		/// </remarks>
		private IEnumerable<string> LibraryPathCandidates()
		{
			foreach (string loaded in LoadedEngineModules())
				yield return loaded;

			foreach (string name in new[] { "ISYS11df.dll", "libISYS11df.so", "libISYS11df.dylib" })
			{
				if (_libraryPath != null)
					yield return Path.Combine(_libraryPath, name);

				yield return Path.Combine(AppContext.BaseDirectory, name);
				yield return Path.Combine(AppContext.BaseDirectory, "runtimes",
										  RuntimeInformation.RuntimeIdentifier, "native", name);
			}
		}

		/// <summary>The engine's own module, as the loader resolved it.</summary>
		private static IEnumerable<string> LoadedEngineModules()
		{
			var paths = new List<string>();
			try
			{
				foreach (ProcessModule module in Process.GetCurrentProcess().Modules)
				{
					if (module.ModuleName != null
						&& module.ModuleName.IndexOf("ISYS11df", StringComparison.OrdinalIgnoreCase) >= 0
						&& !string.IsNullOrEmpty(module.FileName))
					{
						paths.Add(module.FileName);
					}
				}
			}
			catch
			{
				// Enumerating modules is not permitted everywhere; the probes below
				// still answer the ordinary case.
			}

			return paths;
		}

		public void Dispose()
		{
			_api.Dispose();
		}
	}

	/// <summary>
	/// Turns anything the binding throws into an <see cref="EngineException"/>.
	/// </summary>
	/// <remarks>
	/// One place, so no caller above the seam ever catches <c>IGRException</c> and no
	/// binding type reaches the conversion paths. A failure that is not an
	/// <c>IGRException</c> — a missing native DLL surfaces as
	/// <c>DllNotFoundException</c>, a bad path as an IO exception — is wrapped too,
	/// under <see cref="EngineError.Error"/>, because the caller's question is the
	/// same either way: the call did not work, and here is what to tell the user.
	/// </remarks>
	internal static class EngineCall
	{
		public static void Run(Action call)
		{
			Run<object>(() => { call(); return null; });
		}

		public static T Run<T>(Func<T> call)
		{
			try
			{
				return call();
			}
			catch (IGRException error)
			{
				throw new EngineException(EngineException.Classify(error.ErrorCode), error.ErrorCode, error.Message, error);
			}
			catch (EngineException)
			{
				// Already translated — a nested call inside this one.
				throw;
			}
			catch (Exception error)
			{
				throw new EngineException(EngineError.Error, (int)EngineError.Error, error.Message, error);
			}
		}
	}

	/// <summary><see cref="IDocument"/> over <c>Extractor</c>.</summary>
	internal sealed class DocumentAdapter : IDocument
	{
		private readonly Extractor _extractor;
		private readonly bool _ownsHandle;

		/// <param name="extractor">The handle to wrap.</param>
		/// <param name="ownsHandle">
		/// False when the handle belongs to something else — a subfile, which the
		/// binding models as an Extractor in its own right, so opening it yields a
		/// document that shares the entry's handle and must not close it.
		/// </param>
		public DocumentAdapter(Extractor extractor, bool ownsHandle = true)
		{
			_extractor = extractor;
			_ownsHandle = ownsHandle;
		}

		public int GetFileType() => EngineCall.Run(() => _extractor.getFileType());

		public string GetMimeType() => EngineCall.Run(() => _extractor.getFileType(IGRFormatWhat.IGR_FORMAT_MIMETYPE));

		public string GetFormatName() => EngineCall.Run(() => _extractor.getFileType(IGRFormatWhat.IGR_FORMAT_LONG_NAME));

		public int GetFileCapabilities() => EngineCall.Run(() => _extractor.getFileCapabilities());

		public bool GetEOF() => EngineCall.Run(() => _extractor.getEOF());

		public string GetText(int maxLength) => EngineCall.Run(() => _extractor.GetText(maxLength));

		public int GetPageCount() => EngineCall.Run(() => _extractor.GetPageCount());

		public IDocumentPage GetPage(int pageIndex) => EngineCall.Run(() => (IDocumentPage)new PageAdapter(_extractor.GetPage(pageIndex)));

		/// <remarks>
		/// Drained here rather than handed back as a cursor: <c>GetFirstSubFile</c> and
		/// <c>GetNextSubFile</c> advance state on the extractor itself, so anything else
		/// touching the same handle mid-walk loses entries.
		/// </remarks>
		public IReadOnlyList<ISubFileEntry> SubFiles => EngineCall.Run(() => Drain(_extractor.GetFirstSubFile, _extractor.GetNextSubFile));

		public IReadOnlyList<ISubFileEntry> Images => EngineCall.Run(() => Drain(_extractor.GetFirstImage, _extractor.GetNextImage));

		public EngineBookmark RootBookmark => EngineCall.Run(() => Convert(_extractor.RootBookmark));

		public void MakeDocPack(string filename, string options) => EngineCall.Run(() => _extractor.MakeDocPack(filename, options ?? string.Empty));

		public void MakeDocPack(Stream destination, string options) => EngineCall.Run(() => _extractor.MakeDocPack(destination, options ?? string.Empty));

		public void Dispose()
		{
			if (_ownsHandle)
				_extractor.Dispose();
		}

		/// <summary>The handle behind this document, for the calls that need it by identity.</summary>
		/// <remarks>
		/// <c>Canvas.RenderPages</c> takes an <c>Extractor</c>, not anything the seam can
		/// describe, so the canvas adapter has to reach back through here. Internal, so
		/// the binding type still does not escape this file.
		/// </remarks>
		internal Extractor Inner => _extractor;

		private static IReadOnlyList<ISubFileEntry> Drain(Func<SubFile> first, Func<SubFile> next)
		{
			var entries = new List<ISubFileEntry>();

			for (SubFile entry = first(); entry != null; entry = next())
				entries.Add(new SubFileAdapter(entry));

			return entries;
		}

		internal static EngineBookmark Convert(Bookmark bookmark)
		{
			if (bookmark == null)
				return null;

			var children = new List<EngineBookmark>();
			foreach (Bookmark child in bookmark.Children ?? Enumerable.Empty<Bookmark>())
				children.Add(Convert(child));

			return new EngineBookmark
			{
				Title = bookmark.Title,
				Level = bookmark.Level,
				PageIndex = bookmark.PageIndex,
				Children = children
			};
		}
	}

	/// <summary><see cref="IDocumentPage"/> over <c>Page</c>.</summary>
	internal sealed class PageAdapter : IDocumentPage
	{
		private readonly Page _page;

		public PageAdapter(Page page)
		{
			_page = page;
		}

		public int GetWidth() => EngineCall.Run(() => _page.GetWidth());

		public int GetHeight() => EngineCall.Run(() => _page.GetHeight());

		public string GetAttribute(string name) => EngineCall.Run(() => _page.GetAttribute(name));

		public IReadOnlyList<EngineHyperlink> Hyperlinks => EngineCall.Run(() =>
		{
			var links = new List<EngineHyperlink>();
			foreach (Hyperlink link in _page.Hyperlinks)
			{
				links.Add(new EngineHyperlink
				{
					Type = link.Type,
					Uri = link.Uri,
					Ref = link.Ref,
					X = link.X,
					Y = link.Y,
					Width = link.Width,
					Height = link.Height
				});
			}

			return (IReadOnlyList<EngineHyperlink>)links;
		});

		public IReadOnlyList<EngineFormElement> FormElements => EngineCall.Run(() =>
		{
			var elements = new List<EngineFormElement>();
			foreach (FormElement element in _page.FormElements)
			{
				elements.Add(new EngineFormElement
				{
					Type = element.Type,
					Name = element.Name,
					X = element.X,
					Y = element.Y,
					Width = element.Width,
					Height = element.Height
				});
			}

			return (IReadOnlyList<EngineFormElement>)elements;
		});

		public IReadOnlyList<EngineAnnotation> Annotations => EngineCall.Run(() =>
		{
			var annotations = new List<EngineAnnotation>();
			foreach (Annotations.Annotation annotation in _page.Annotations)
				annotations.Add(new EngineAnnotation { Type = annotation.Type, Title = annotation.Title, Text = annotation.Text });

			return (IReadOnlyList<EngineAnnotation>)annotations;
		});

		public EnginePageElement GetRootPageElement() => EngineCall.Run(() => Convert(_page.GetRootPageElement()));

		public void Dispose() => _page.Dispose();

		/// <summary>The handle behind this page, for <c>Canvas.RenderPage</c>.</summary>
		internal Page Inner => _page;

		/// <remarks>
		/// The whole tree is copied in one pass rather than lazily, for the same reason
		/// the subfile cursor is drained: the binding's element walk reads through the
		/// page handle, which the caller is free to close as soon as this returns.
		/// </remarks>
		internal static EnginePageElement Convert(PageElement element)
		{
			if (element == null)
				return null;

			var children = new List<EnginePageElement>();
			foreach (PageElement child in element.Children())
				children.Add(Convert(child));

			return new EnginePageElement
			{
				Type = element.Type,
				Depth = element.Depth,
				Bounds = element.Bounds,
				Children = children
			};
		}
	}

	/// <summary><see cref="IOutputCanvas"/> over <c>Canvas</c>.</summary>
	internal sealed class CanvasAdapter : IOutputCanvas
	{
		private readonly Canvas _canvas;

		public CanvasAdapter(Canvas canvas)
		{
			_canvas = canvas;
		}

		/// <remarks>
		/// The binding has two overloads, and passing a properties object built from
		/// nothing is not the same as passing none — so the null check picks the
		/// overload rather than defaulting the argument.
		/// </remarks>
		public void RenderPage(IDocumentPage page, string options, EnginePageDecorations decorations)
		{
			Page inner = InnerPage(page);

			EngineCall.Run(() =>
			{
				if (decorations == null || decorations.IsEmpty)
				{
					_canvas.RenderPage(inner, options ?? string.Empty);
					return;
				}

				var properties = new RenderPageProperties();
				foreach (EngineRedaction redaction in decorations.Redactions)
					properties.AddRedaction(new Rectangle(redaction.X, redaction.Y, redaction.Width, redaction.Height), Color.Black);

				foreach (EngineFormValue value in decorations.FormValues)
					properties.AddFormValue(value.Name, value.Value, false);

				_canvas.RenderPage(inner, options ?? string.Empty, properties);
			});
		}

		public void RenderPages(IDocument document)
		{
			if (!(document is DocumentAdapter adapter))
				throw Mismatch("document");

			EngineCall.Run(() => _canvas.RenderPages(adapter.Inner));
		}

		public void DrawImage(int x, int y, byte[] image, string options)
		{
			EngineCall.Run(() => _canvas.DrawImage(x, y, image, options));
		}

		public void Dispose() => _canvas.Dispose();

		private static Page InnerPage(IDocumentPage page)
		{
			if (page is PageAdapter adapter)
				return adapter.Inner;

			throw Mismatch("page");
		}

		/// <summary>
		/// A real canvas was handed something that did not come from the real engine.
		/// </summary>
		/// <remarks>
		/// Only reachable by mixing a fake engine's objects with a real canvas, which no
		/// run does — the engine is chosen once per process. Reported rather than left as
		/// an InvalidCastException so that, if it ever does happen, the message says what
		/// went wrong instead of naming two adapter classes.
		/// </remarks>
		private static EngineException Mismatch(string what)
		{
			return new EngineException(
				EngineError.InvalidParameter,
				(int)EngineError.InvalidParameter,
				"this canvas can only render a " + what + " that came from the same engine");
		}
	}

	/// <summary><see cref="ISubFileEntry"/> over <c>SubFile</c>.</summary>
	internal sealed class SubFileAdapter : ISubFileEntry
	{
		private readonly SubFile _subfile;

		public SubFileAdapter(SubFile subfile)
		{
			_subfile = subfile;
		}

		public string Id => _subfile.ID;

		public string Name => _subfile.Name;

		public long Size => _subfile.FileSize;

		/// <remarks>
		/// The binding returns <c>DateTime.MinValue</c> for an entry whose container
		/// records no timestamp; that is reported as absent rather than as a date in
		/// year one.
		/// </remarks>
		public DateTime? Modified => EngineCall.Run(() =>
		{
			DateTime value = _subfile.FileDate;
			return value == DateTime.MinValue ? (DateTime?)null : value;
		});

		public IDocument Open(int flags, string options)
		{
			return EngineCall.Run(() =>
			{
				_subfile.Open(flags, options ?? string.Empty);
				return (IDocument)new DocumentAdapter(_subfile, ownsHandle: false);
			});
		}

		public void CopyTo(string path) => EngineCall.Run(() => _subfile.CopyTo(path));

		public void Dispose() => _subfile.Dispose();
	}
}
