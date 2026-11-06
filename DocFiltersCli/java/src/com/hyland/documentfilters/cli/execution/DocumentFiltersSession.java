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

import com.hyland.documentfilters.cli.configuration.LibraryOptions;
import com.hyland.documentfilters.cli.engine.DocumentFiltersEngine;
import com.hyland.documentfilters.cli.engine.EngineException;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.IDocumentFiltersEngine;
import com.hyland.documentfilters.cli.runtime.InitialisationError;
import java.io.File;
import java.nio.channels.SeekableByteChannel;
import java.util.function.BooleanSupplier;

/**
 * The initialised engine for one run, and the one place documents are opened.
 */
public final class DocumentFiltersSession implements AutoCloseable
{
	private final IDocumentFiltersEngine _engine;

	private DocumentFiltersSession(IDocumentFiltersEngine engine)
	{
		_engine = engine;
	}

	/**
	 * Wraps an engine that is already initialised.
	 *
	 * <p>How the tests put a fake engine behind the execution paths. The production
	 * route is {@link #create}, which builds the real one.
	 */
	public static DocumentFiltersSession forEngine(IDocumentFiltersEngine engine)
	{
		return new DocumentFiltersSession(engine);
	}

	/**
	 * Initialises the engine.
	 *
	 * @throws InitialisationError the native library could not be found, or the
	 *         license was rejected.
	 */
	public static DocumentFiltersSession create(LibraryOptions options, String tempDir)
	{
		String licenseKey = options.licenseKey != null ? options.licenseKey : System.getenv("DF_LICENSE_KEY");
		String libraryPath = resolveLibraryPath(options, System.getenv("DF_PATH"), applicationDirectory());

		DocumentFiltersEngine engine = new DocumentFiltersEngine();
		try
		{
			engine.initialize(licenseKey, libraryPath);
		}
		catch (EngineException error)
		{
			throw new InitialisationError("could not initialise Document Filters: " + error.getMessage());
		}

		// --temp-dir, after initialize because it is a setting on an initialised
		// library rather than an argument to starting one.
		if (tempDir != null && !tempDir.isEmpty())
			engine.setTempPath(tempDir);

		return new DocumentFiltersSession(engine);
	}

	/**
	 * {@code --library}, then {@code DF_PATH}, then the directory this tool's own
	 * jar lives in --- never the process's current directory.
	 *
	 * <p>This is also where the engine looks for fonts, which is why the current
	 * directory is wrong: this tool is invoked from wherever the user's documents
	 * are, and a directory with no fonts renders text blank rather than failing.
	 * {@code baseDirectory} is threaded in rather than read here so this stays a
	 * pure function of its inputs.
	 *
	 * <p>It is <em>not</em> where the JVM loads the native library from; that is
	 * {@code java.library.path}, fixed when the JVM starts.
	 */
	public static String resolveLibraryPath(LibraryOptions options, String environmentValue, String baseDirectory)
	{
		if (options.libraryPath != null)
			return options.libraryPath;
		return environmentValue != null ? environmentValue : baseDirectory;
	}

	/** The directory holding this tool's jar, or its class directory when run unpackaged. */
	static String applicationDirectory()
	{
		try
		{
			File location = new File(DocumentFiltersSession.class.getProtectionDomain().getCodeSource().getLocation().toURI());
			return location.isFile() ? location.getParent() : location.getPath();
		}
		catch (Exception e)
		{
			return System.getProperty("user.dir");
		}
	}

	/** The initialised engine, for the execution helpers. */
	public IDocumentFiltersEngine getApi()
	{
		return _engine;
	}

	/**
	 * Opens one document with the given flags and options, wiring up the password
	 * callback.
	 *
	 * <p>The open settings are passed in rather than worked out here, because the
	 * engine applies them during {@code Open}: the format-specific bits for flow HTML
	 * or text-mode markdown have no effect once the document is already open.
	 *
	 * @param shouldCancel polled while the engine parses; true stops the open. Null
	 *        for a caller with no deadline to enforce.
	 * @param log null for no engine logging.
	 */
	public IDocument openDocument(String path, ExtractorOpenSettings open, PasswordProvider passwords,
		BooleanSupplier shouldCancel, EngineLog log)
	{
		return _engine.openDocument(path, open.flags, open.options, () -> passwords.getPassword(path), shouldCancel,
			log == null ? null : log::levelFor,
			log == null ? null : log::write);
	}

	/**
	 * The same, from a channel --- for the debug switches that change how bytes
	 * reach the engine.
	 */
	public IDocument openDocument(SeekableByteChannel source, String displayName, ExtractorOpenSettings open,
		PasswordProvider passwords, BooleanSupplier shouldCancel, EngineLog log)
	{
		return _engine.openDocument(source, displayName, open.flags, open.options,
			() -> passwords.getPassword(displayName), shouldCancel,
			log == null ? null : log::levelFor,
			log == null ? null : log::write);
	}

	/** Version and build information, for the {@code version} command. */
	public String describeVersion()
	{
		return _engine.describeVersion();
	}

	@Override
	public void close()
	{
		_engine.close();
	}
}
