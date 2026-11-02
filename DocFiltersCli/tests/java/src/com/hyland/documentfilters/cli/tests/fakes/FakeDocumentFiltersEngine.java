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

package com.hyland.documentfilters.cli.tests.fakes;

import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.engine.EngineException;
import com.hyland.documentfilters.cli.engine.EngineFileType;
import com.hyland.documentfilters.cli.engine.EngineFormat;
import com.hyland.documentfilters.cli.engine.EngineOption;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.IDocumentFiltersEngine;
import com.hyland.documentfilters.cli.engine.IOutputCanvas;
import com.hyland.documentfilters.cli.engine.LogSink;
import com.hyland.documentfilters.cli.planning.CanvasType;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.channels.SeekableByteChannel;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BooleanSupplier;
import java.util.function.Supplier;
import java.util.function.ToIntFunction;

/**
 * A Document Filters engine that answers from a script instead of a library.
 *
 * <p>The engine is the one dependency the execution paths cannot be tested around:
 * it needs a license, a native library, and a real document that actually exhibits
 * whatever is being tested. Every failure this thing can produce is a failure the
 * real engine produces too --- the codes come from {@link EngineError} --- so a test
 * here is a statement about how the CLI handles a real outcome.
 *
 * <p>Documents are added by path. Anything not added fails the way the engine fails
 * for a file it cannot read, so "the document is not there" needs no setup.
 *
 * <p>The strings appended to {@link #calls} keep the reference's PascalCase
 * spellings --- {@code OpenDocument(a.pdf,2,)} --- because they are assertion text
 * shared with the other ports' suites, not API names.
 */
public final class FakeDocumentFiltersEngine implements IDocumentFiltersEngine
{
	private final Map<String, FakeDocument> _documents = new HashMap<>();
	private final Map<String, EngineError> _openFailures = new HashMap<>();

	/** Every call made, in order, for asserting on what the CLI asked the engine to do. */
	public final List<String> calls = new ArrayList<>();

	/** Canvases handed out, in order of creation. */
	public final List<FakeCanvas> canvases = new ArrayList<>();

	/** Set to fail {@link #initialize}, the way a bad license or a missing library does. */
	public EngineError initialisationFailure;

	/** Set to fail every {@code makeCanvas}. */
	public EngineError canvasFailure;

	/** Run at the start of an open, before the cancellation poll. */
	public Runnable duringOpen;

	/**
	 * Set to fail {@link #getSupportedFormats} and {@link #getAvailableOptions}. The
	 * catalogues are the two commands that read no document, so the whole call
	 * either answers or does not.
	 */
	public EngineError catalogFailure;

	public final List<EngineFormat> formats = new ArrayList<>();
	public final List<EngineOption> options = new ArrayList<>();

	public boolean isDisposed;

	/** The last path {@link #setTempPath} was given, or null. */
	public String tempPath;

	/** What {@link #getMemoryStatus} answers. */
	public String memoryStatus = "heap: 0 bytes";

	// ------------------------------------------------------------- building

	public FakeDocument addDocument(String path)
	{
		return addDocument(path, "");
	}

	/** Adds a document the engine can open. */
	public FakeDocument addDocument(String path, String text)
	{
		FakeDocument document = new FakeDocument(path);
		document.text = text;
		_documents.put(path, document);
		return document;
	}

	/**
	 * Makes opening {@code path} fail with a given engine error.
	 *
	 * <p>{@link EngineError#PASSWORD} is the one worth calling out: the real engine
	 * reports a wrong password and a refused prompt identically, so a test using it
	 * covers both.
	 */
	public FakeDocumentFiltersEngine failToOpen(String path, EngineError error)
	{
		_openFailures.put(path, error);
		return this;
	}

	public FakeDocument addProtectedDocument(String path, String password)
	{
		return addProtectedDocument(path, password, "");
	}

	/** Adds a document that opens only when the right password is supplied. */
	public FakeDocument addProtectedDocument(String path, String password, String text)
	{
		FakeDocument document = addDocument(path, text);
		document.password = password;
		return document;
	}

	// -------------------------------------------------------------- engine

	@Override
	public void initialize(String licenseKey, String libraryPath)
	{
		calls.add("Initialize(" + (licenseKey != null ? licenseKey : "null") + "," + (libraryPath != null ? libraryPath : "null") + ")");

		if (initialisationFailure != null)
			throw failure(initialisationFailure, "initialisation failed");
	}

	@Override
	public IDocument openDocument(String path, int flags, String options, Supplier<String> password,
		BooleanSupplier shouldCancel, ToIntFunction<String> logLevelFor, LogSink onLogMessage)
	{
		calls.add("OpenDocument(" + path + "," + flags + "," + (options != null ? options : "")
			+ (shouldCancel != null ? ",cancellable" : "") + ")");

		// Runs before the cancellation poll, so a test can make something true
		// *during* the open rather than before it.
		if (duringOpen != null)
			duringOpen.run();

		// The engine polls this while it parses; the fake polls it once, which is
		// enough to prove a caller wired it up and translates the refusal.
		if (shouldCancel != null && shouldCancel.getAsBoolean())
			throw failure(EngineError.CANCELLED, "cancelled while opening " + path);

		EngineError openFailure = _openFailures.get(path);
		if (openFailure != null)
			throw failure(openFailure, "could not open " + path);

		FakeDocument document = _documents.get(path);
		if (document == null)
			throw failure(EngineError.OPEN_ERROR, "no such document: " + path);

		if (document.password != null)
		{
			// The real engine asks through the callback and turns a refusal into
			// IGR_E_PASSWORD out of Open, so a wrong answer and no answer land here
			// the same way.
			String offered = password != null ? password.get() : null;
			if (!document.password.equals(offered))
				throw failure(EngineError.PASSWORD, "password required for " + path);
		}

		// The engine asks the level per module, then hands back messages. Answering
		// once with a known module is enough to show a caller wired both halves.
		if (logLevelFor != null && onLogMessage != null)
		{
			int level = logLevelFor.applyAsInt("pdf");
			if (level > 0)
				onLogMessage.accept(level, "pdf", "opened " + path);
		}

		document.openFlags = flags;
		document.openOptions = options;
		document.wasOpened = true;
		return document;
	}

	/**
	 * The stream form. Records the stream's length, which is what the debug
	 * switches that use it are actually about.
	 */
	@Override
	public IDocument openDocument(SeekableByteChannel source, String displayName, int flags, String options,
		Supplier<String> password, BooleanSupplier shouldCancel, ToIntFunction<String> logLevelFor, LogSink onLogMessage)
	{
		calls.add("OpenDocument(<stream:" + lengthOf(source) + ">," + displayName + "," + flags + ","
			+ (options != null ? options : "") + (shouldCancel != null ? ",cancellable)" : ")"));

		return openDocument(displayName, flags, options, password, shouldCancel, logLevelFor, onLogMessage);
	}

	@Override
	public void setTempPath(String path)
	{
		calls.add("SetTempPath(" + path + ")");
		tempPath = path;
	}

	@Override
	public String getMemoryStatus()
	{
		calls.add("GetMemoryStatus()");
		return memoryStatus;
	}

	/** The stream form, for the debug switches that reach identify too. */
	@Override
	public EngineFileType identifyFile(SeekableByteChannel source, String displayName)
	{
		calls.add("IdentifyFile(<stream:" + lengthOf(source) + ">," + displayName + ")");
		return identifyFile(displayName);
	}

	@Override
	public EngineFileType identifyFile(String path)
	{
		calls.add("IdentifyFile(" + path + ")");

		EngineError openFailure = _openFailures.get(path);
		if (openFailure != null)
			throw failure(openFailure, "could not identify " + path);

		FakeDocument document = _documents.get(path);
		if (document == null)
			throw failure(EngineError.OPEN_ERROR, "no such document: " + path);

		EngineFileType type = new EngineFileType();
		type.id = document.fileType;
		type.mimeType = document.mimeType;
		type.name = document.formatName;
		type.capabilities = document.capabilities;
		return type;
	}

	@Override
	public IOutputCanvas makeCanvas(String target, CanvasType type, String options)
	{
		calls.add("MakeCanvas(" + target + "," + type.name() + "," + (options != null ? options : "") + ")");

		if (canvasFailure != null)
			throw failure(canvasFailure, "could not create canvas at " + target);

		FakeCanvas canvas = new FakeCanvas(target, type, options);
		canvases.add(canvas);
		return canvas;
	}

	@Override
	public IOutputCanvas makeCanvas(SeekableByteChannel target, CanvasType type, String options)
	{
		calls.add("MakeCanvas(<stream>," + type.name() + "," + (options != null ? options : "") + ")");

		if (canvasFailure != null)
			throw failure(canvasFailure, "could not create canvas on a stream");

		FakeCanvas canvas = new FakeCanvas(null, type, options);
		canvases.add(canvas);
		return canvas;
	}

	@Override
	public List<EngineFormat> getSupportedFormats()
	{
		calls.add("GetSupportedFormats()");

		if (catalogFailure != null)
			throw failure(catalogFailure, "catalogue unavailable");

		return formats;
	}

	@Override
	public List<EngineOption> getAvailableOptions()
	{
		calls.add("GetAvailableOptions()");

		if (catalogFailure != null)
			throw failure(catalogFailure, "catalogue unavailable");

		return options;
	}

	@Override
	public String describeVersion()
	{
		return "0.0.0-fake";
	}

	@Override
	public void close()
	{
		isDisposed = true;
	}

	/** An engine failure with the code the real library would report. */
	public static EngineException failure(EngineError error, String message)
	{
		return new EngineException(error, error.getValue(), message);
	}

	/** How many bytes {@code source} holds, without disturbing where it is reading from. */
	private static long lengthOf(SeekableByteChannel source)
	{
		try
		{
			return source.size();
		}
		catch (IOException e)
		{
			throw new UncheckedIOException(e.getMessage(), e);
		}
	}
}
