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
using Hyland.DocumentFilters.Cli.Engine;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// Owns the one <c>DocumentFilters</c> instance for the whole run.
	/// </summary>
	/// <remarks>
	/// The engine is expensive to create and is designed to be initialised once for
	/// the lifetime of the process, so this is created lazily — the first time a
	/// command actually needs to open something — and disposed at the end.
	/// <c>--print-plan</c> therefore never needs a license key.
	/// </remarks>
	public sealed class DocumentFiltersSession : IDisposable
	{
		private readonly IDocumentFiltersEngine _engine;

		private DocumentFiltersSession(IDocumentFiltersEngine engine)
		{
			_engine = engine;
		}

		/// <summary>
		/// Wraps an engine that is already initialised.
		/// </summary>
		/// <remarks>
		/// How the tests put a fake engine behind the execution paths. The production
		/// route is <see cref="Create"/>, which builds the real one.
		/// </remarks>
		public static DocumentFiltersSession For(IDocumentFiltersEngine engine)
		{
			return new DocumentFiltersSession(engine);
		}

		/// <summary>
		/// Initialises the engine.
		/// </summary>
		/// <exception cref="Runtime.InitialisationError">
		/// The native library could not be found, or the license was rejected.
		/// </exception>
		public static DocumentFiltersSession Create(LibraryOptions options, string tempDir = null)
		{
			string licenseKey = options.LicenseKey ?? Environment.GetEnvironmentVariable("DF_LICENSE_KEY");
			string libraryPath = ResolveLibraryPath(options, Environment.GetEnvironmentVariable("DF_PATH"), AppContext.BaseDirectory);

			var engine = new DocumentFiltersEngine();
			try
			{
				engine.Initialize(licenseKey, libraryPath);
			}
			catch (EngineException error)
			{
				throw new InitialisationError("could not initialise Document Filters: " + error.Message);
			}

			// --temp-dir, after Initialize because it is a setting on an initialised
			// library rather than an argument to starting one.
			if (!string.IsNullOrEmpty(tempDir))
				engine.SetTempPath(tempDir);

			return new DocumentFiltersSession(engine);
		}

		/// <summary>
		/// <c>--library</c>, then <c>DF_PATH</c>, then the directory this tool's own
		/// executable lives in — never the process's current directory.
		/// </summary>
		/// <remarks>
		/// This is also where <c>DocumentFilters.Initialize</c> looks for fonts
		/// (<c>ISYS_INIT_PATH/fonts</c>, per the binding's own doc comment on the
		/// parameter), which is where the <c>Hyland.DocumentFilters.Fonts</c> package
		/// copies its bundled <c>fonts.ini</c> and TTFs at build time — next to the
		/// executable, not next to whatever the caller's shell happens to be sitting
		/// in. doc2text and the other published samples default this to <c>"."</c>
		/// and get away with it only because they are always run from their own build
		/// output directory; this tool is invoked from wherever the user's documents
		/// are, so <c>"."</c> there resolves to a directory with no fonts at all. The
		/// engine still renders in that case — it does not fail to initialise — but
		/// falls back to a font it cannot rasterise glyphs from, so text comes out
		/// blank instead of missing. <paramref name="baseDirectory"/> is threaded in
		/// rather than read here so this stays a pure function of its inputs.
		/// </remarks>
		public static string ResolveLibraryPath(LibraryOptions options, string environmentValue, string baseDirectory)
		{
			return options.LibraryPath ?? environmentValue ?? baseDirectory;
		}

		/// <summary>The initialised engine, for the execution helpers.</summary>
		public IDocumentFiltersEngine Api => _engine;

		/// <summary>
		/// Opens one document with the given flags and options, wiring up the password
		/// callback.
		/// </summary>
		/// <remarks>
		/// The open settings are passed in rather than worked out here, because the
		/// engine applies them during <c>Open</c>: the format-specific bits for flow
		/// HTML or text-mode markdown have no effect once the document is already
		/// open. Each conversion path resolves its
		/// <see cref="ExtractorOpenSettings"/> first, and the document is opened
		/// exactly once.
		/// </remarks>
		/// <param name="shouldCancel">
		/// Polled while the engine parses; true stops the open. Null for a caller with
		/// no deadline to enforce.
		/// </param>
		public IDocument OpenDocument(string path, ExtractorOpenSettings open, PasswordProvider passwords,
									  Func<bool> shouldCancel = null, EngineLog log = null)
		{
			return _engine.OpenDocument(path, open.Flags, open.Options, () => passwords.GetPassword(path), shouldCancel,
										log == null ? null : (Func<string, int>)log.LevelFor,
										log == null ? null : (Action<int, string, string>)log.Write);
		}

		/// <summary>
		/// The same, from a stream — for the debug switches that change how bytes
		/// reach the engine.
		/// </summary>
		public IDocument OpenDocument(Stream source, string displayName, ExtractorOpenSettings open,
									  PasswordProvider passwords, Func<bool> shouldCancel = null, EngineLog log = null)
		{
			return _engine.OpenDocument(source, displayName, open.Flags, open.Options,
										() => passwords.GetPassword(displayName), shouldCancel,
										log == null ? null : (Func<string, int>)log.LevelFor,
										log == null ? null : (Action<int, string, string>)log.Write);
		}

		/// <summary>Version and build information, for the <c>version</c> command.</summary>
		/// <remarks>
		/// The native build number doc2text prints comes from reading each engine DLL's
		/// own file version directly, which the managed binding does not expose; this
		/// reports the .NET binding's own package version instead.
		/// </remarks>
		public string DescribeVersion()
		{
			return _engine.DescribeVersion();
		}

		public void Dispose()
		{
			_engine.Dispose();
		}
	}
}
