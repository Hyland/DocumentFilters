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

package com.hyland.documentfilters.cli.engine;

import com.hyland.documentfilters.cli.planning.CanvasType;
import com.perceptive.documentfilters.Annotation;
import com.perceptive.documentfilters.Bookmark;
import com.perceptive.documentfilters.Canvas;
import com.perceptive.documentfilters.Color;
import com.perceptive.documentfilters.DocumentFilters;
import com.perceptive.documentfilters.Extractor;
import com.perceptive.documentfilters.FormElement;
import com.perceptive.documentfilters.Hyperlink;
import com.perceptive.documentfilters.IGRException;
import com.perceptive.documentfilters.IGRFormat;
import com.perceptive.documentfilters.IGRFormatWhat;
import com.perceptive.documentfilters.IGROption;
import com.perceptive.documentfilters.IGRStream;
import com.perceptive.documentfilters.IGRStream_Data;
import com.perceptive.documentfilters.IGR_FRect;
import com.perceptive.documentfilters.OpenCallback;
import com.perceptive.documentfilters.OpenCallbackRequest;
import com.perceptive.documentfilters.Page;
import com.perceptive.documentfilters.PageElement;
import com.perceptive.documentfilters.RenderPageProperties;
import com.perceptive.documentfilters.SubFile;
import com.perceptive.documentfilters.VectIGRFormat;
import com.perceptive.documentfilters.VectIGROption;
import com.perceptive.documentfilters.isys_docfiltersConstants;
import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.Files;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.function.BooleanSupplier;
import java.util.function.Supplier;
import java.util.function.ToIntFunction;
import java.util.regex.Pattern;

// -------------------------------------------------------------------------
//  THE REAL ENGINE
//
//  The only file in the project that touches com.perceptive.documentfilters
//  types. Every method here is one binding call plus, where the binding hands
//  back a type a test cannot construct, a copy into the plain values from
//  IDocumentFiltersEngine.
//
//  Nothing here decides anything. If a method in this file grows an 'if' that
//  is not null-handling or error translation, it belongs above the seam.
// -------------------------------------------------------------------------

/**
 * {@link IDocumentFiltersEngine} over the real library.
 *
 * <p>The binding is created in {@link #initialize}, not when this object is: its
 * class loads the native libraries as it initialises, and a library that cannot be
 * found has to surface as an {@link EngineException} from {@code initialize} like
 * any other start-up failure, not as a {@code LinkageError} from a constructor.
 */
public final class DocumentFiltersEngine implements IDocumentFiltersEngine
{
	private DocumentFilters _api;

	/** Remembered from {@link #initialize}, for {@link #describeVersion}. */
	private String _libraryPath;

	@Override
	public void initialize(String licenseKey, String libraryPath)
	{
		EngineCall.run(() ->
		{
			DocumentFilters api = new DocumentFilters();
			api.Initialize(licenseKey != null ? licenseKey : "", libraryPath);
			_api = api;
		});
		_libraryPath = libraryPath;
	}

	@Override
	public IDocument openDocument(String path, int flags, String options, Supplier<String> password,
		BooleanSupplier shouldCancel, ToIntFunction<String> logLevelFor, LogSink onLogMessage)
	{
		return EngineCall.run(() -> open(_api.GetExtractor(path), null, flags, options, password, shouldCancel,
			logLevelFor, onLogMessage));
	}

	@Override
	public IDocument openDocument(SeekableByteChannel source, String displayName, int flags, String options,
		Supplier<String> password, BooleanSupplier shouldCancel, ToIntFunction<String> logLevelFor, LogSink onLogMessage)
	{
		return EngineCall.run(() ->
		{
			ChannelStream stream = new ChannelStream(source, true);
			return open(_api.GetExtractor(stream), stream, flags, options, password, shouldCancel, logLevelFor, onLogMessage);
		});
	}

	/**
	 * Opens an extractor the caller has already obtained, whether from a path or a
	 * stream.
	 *
	 * <p>Shared so the callback exists once: a second copy would be a second place
	 * for the password refusal and the cancellation answer to drift apart.
	 *
	 * @param stream the bridge the extractor reads through, or null; held by the
	 *        document so it outlives every call that may still read from it.
	 */
	private static IDocument open(Extractor extractor, ChannelStream stream, int flags, String options,
		Supplier<String> password, BooleanSupplier shouldCancel, ToIntFunction<String> logLevelFor,
		LogSink onLogMessage) throws IGRException
	{
		Callback callback = new Callback(password, shouldCancel, logLevelFor, onLogMessage);
		try
		{
			// The flags-only overload, so the caller's format bit survives.
			extractor.Open(flags, callback, options != null ? options : "");
		}
		catch (IGRException error)
		{
			extractor.close();
			callback.rethrowParked();
			throw error;
		}

		// A failure the callback swallowed to keep it out of native code, on an
		// open that went on to succeed anyway: still this tool's failure.
		if (callback.hasParked())
		{
			extractor.close();
			callback.rethrowParked();
		}

		return new DocumentAdapter(extractor, true, stream, callback);
	}

	/**
	 * Not supported: the Java binding has no way to set the engine's temporary
	 * directory.
	 *
	 * <p>The reference reaches it through {@code IGR_Multiplex}, which this binding
	 * does not expose. Silently ignored rather than failing the run, because a
	 * temporary directory changes where the engine's scratch files go, never what
	 * any command prints.
	 */
	@Override
	public void setTempPath(String path)
	{
	}

	@Override
	public String getMemoryStatus()
	{
		return EngineCall.run(() ->
		{
			String status = _api.GetMemoryStatus();
			return status != null ? status : "";
		});
	}

	@Override
	public EngineFileType identifyFile(String path)
	{
		return EngineCall.run(() -> identify(_api.GetExtractor(path)));
	}

	@Override
	public EngineFileType identifyFile(SeekableByteChannel source, String displayName)
	{
		return EngineCall.run(() ->
		{
			ChannelStream stream = new ChannelStream(source, true);
			EngineFileType type = identify(_api.GetExtractor(stream));
			// Read after the extractor has gone, so the bridge is still reachable
			// for as long as the engine may call it.
			stream.keepAlive();
			return type;
		});
	}

	/** Reads what a file is, without opening it. */
	private static EngineFileType identify(Extractor extractor) throws IGRException
	{
		try (Extractor owned = extractor)
		{
			EngineFileType type = new EngineFileType();
			type.id = owned.getFileType();
			type.mimeType = owned.getFileType(IGRFormatWhat.IGR_FORMAT_MIME_TYPE);
			type.name = owned.getFileType(IGRFormatWhat.IGR_FORMAT_LONG_NAME);
			type.capabilities = capabilities(owned);
			return type;
		}
	}

	/**
	 * The engine's capability bits, put back together from the binding's three
	 * booleans.
	 *
	 * <p>The Java binding exposes no {@code getFileCapabilities}. These three are the
	 * only bits anything above the seam reads, so the composed value answers every
	 * question the native one would.
	 */
	static int capabilities(Extractor extractor) throws IGRException
	{
		return (extractor.getSupportsText() ? isys_docfiltersConstants.IGR_FILE_SUPPORTS_TEXT : 0)
			| (extractor.getSupportsSubFiles() ? isys_docfiltersConstants.IGR_FILE_SUPPORTS_SUBFILES : 0)
			| (extractor.getSupportsHTML() ? isys_docfiltersConstants.IGR_FILE_SUPPORTS_HDHTML : 0);
	}

	@Override
	public IOutputCanvas makeCanvas(String target, CanvasType type, String options)
	{
		return EngineCall.run(() -> new CanvasAdapter(_api.MakeOutputCanvas(target, type.getDevice(), options != null ? options : ""), null));
	}

	@Override
	public IOutputCanvas makeCanvas(SeekableByteChannel target, CanvasType type, String options)
	{
		return EngineCall.run(() ->
		{
			ChannelStream stream = new ChannelStream(target, false);
			return new CanvasAdapter(_api.MakeOutputCanvas(stream, type.getDevice(), options != null ? options : ""), stream);
		});
	}

	@Override
	public List<EngineFormat> getSupportedFormats()
	{
		return EngineCall.run(() ->
		{
			List<EngineFormat> formats = new ArrayList<>();
			VectIGRFormat all = _api.GetSupportedFormats();
			for (int i = 0; i < all.size(); i++)
			{
				IGRFormat format = all.get(i);
				EngineFormat copy = new EngineFormat();
				copy.id = format.getId();
				copy.shortName = format.getShortName();
				copy.displayName = format.getDisplayName();
				formats.add(copy);
			}
			return formats;
		});
	}

	@Override
	public List<EngineOption> getAvailableOptions()
	{
		return EngineCall.run(() ->
		{
			List<EngineOption> options = new ArrayList<>();
			VectIGROption all = _api.GetAvailableOptions();
			for (int i = 0; i < all.size(); i++)
			{
				IGROption option = all.get(i);
				EngineOption copy = new EngineOption();
				copy.displayName = option.getDisplayName();
				copy.type = option.getType();
				copy.description = option.getDescription();
				copy.defaultValue = option.getDefaultValue();
				copy.flags = option.getFlags();
				options.add(copy);
			}
			return options;
		});
	}

	/**
	 * The native library's own file version, read from disk the way doc2text's
	 * {@code showVersion} does.
	 *
	 * <p>Java has no reader for a PE version resource, so the fixed version block
	 * is found by its signature and read directly. A Unix build has no such block
	 * to read and reports "unknown", which the other ports do there too.
	 */
	@Override
	public String describeVersion()
	{
		for (File candidate : libraryPathCandidates())
		{
			try
			{
				if (!candidate.isFile())
					continue;

				String version = readFileVersion(Files.readAllBytes(candidate.toPath()));
				if (version != null)
					return version;
			}
			catch (IOException | RuntimeException e)
			{
				// An unreadable candidate is not an error worth reporting from
				// `version`; try the next one and fall through to "unknown".
			}
		}

		return "unknown";
	}

	/**
	 * Where the native library might be, most authoritative first: the directory
	 * the engine was initialised with, then beside the binding's jar, then
	 * {@code java.library.path}, which is where the JVM actually loads it from.
	 */
	private List<File> libraryPathCandidates()
	{
		List<String> directories = new ArrayList<>();
		if (_libraryPath != null)
			directories.add(_libraryPath);

		try
		{
			File jar = new File(DocumentFilters.class.getProtectionDomain().getCodeSource().getLocation().toURI());
			directories.add(jar.isFile() ? jar.getParent() : jar.getPath());
		}
		catch (Exception | LinkageError e)
		{
			// No binding on the class path, or no location for it: the remaining
			// probes still answer the ordinary case.
		}

		String searchPath = System.getProperty("java.library.path");
		if (searchPath != null)
		{
			for (String entry : searchPath.split(Pattern.quote(File.pathSeparator)))
			{
				if (!entry.isEmpty())
					directories.add(entry);
			}
		}

		List<File> candidates = new ArrayList<>();
		for (String name : new String[] { "ISYS11df.dll", "libISYS11df.so", "libISYS11df.dylib" })
		{
			for (String directory : directories)
			{
				if (directory != null)
					candidates.add(new File(directory, name));
			}
		}
		return candidates;
	}

	/**
	 * The {@code VS_FIXEDFILEINFO} file version in {@code image}, as
	 * {@code major.minor.build.revision}, or null when there is none.
	 *
	 * <p>The block opens with the signature {@code 0xFEEF04BD} and the structure
	 * version {@code 0x00010000}; both are checked, so a stray match in code or
	 * data is not mistaken for it.
	 */
	static String readFileVersion(byte[] image)
	{
		for (int i = 0; i + 16 <= image.length; i++)
		{
			if (readUInt32(image, i) != 0xFEEF04BDL || readUInt32(image, i + 4) != 0x00010000L)
				continue;

			long most = readUInt32(image, i + 8);
			long least = readUInt32(image, i + 12);
			return (most >>> 16) + "." + (most & 0xFFFF) + "." + (least >>> 16) + "." + (least & 0xFFFF);
		}
		return null;
	}

	private static long readUInt32(byte[] data, int offset)
	{
		return (data[offset] & 0xFFL)
			| (data[offset + 1] & 0xFFL) << 8
			| (data[offset + 2] & 0xFFL) << 16
			| (data[offset + 3] & 0xFFL) << 24;
	}

	@Override
	public void close()
	{
		if (_api != null)
			_api.delete();
		_api = null;
	}

	// ---------------------------------------------------------------------
	//  Error translation
	// ---------------------------------------------------------------------

	/** A binding call, which may throw anything. */
	@FunctionalInterface
	interface Call<T>
	{
		T call() throws Exception;
	}

	/** A binding call with nothing to return. */
	@FunctionalInterface
	interface Action
	{
		void run() throws Exception;
	}

	/**
	 * Turns anything the binding throws into an {@link EngineException}.
	 *
	 * <p>One place, so no caller above the seam ever catches {@code IGRException} and
	 * no binding type reaches the conversion paths. A failure that is not an
	 * {@code IGRException} --- a missing native library surfaces as
	 * {@code UnsatisfiedLinkError}, a bad path as an I/O exception --- is wrapped
	 * too, under {@link EngineError#ERROR}, because the caller's question is the same
	 * either way: the call did not work, and here is what to tell the user.
	 */
	static final class EngineCall
	{
		private EngineCall()
		{
		}

		static void run(Action call)
		{
			run(() ->
			{
				call.run();
				return null;
			});
		}

		static <T> T run(Call<T> call)
		{
			try
			{
				return call.call();
			}
			catch (IGRException error)
			{
				throw new EngineException(EngineException.classify(error.getErrorCode()), error.getErrorCode(),
					messageOf(error), error);
			}
			catch (EngineException error)
			{
				// Already translated --- a nested call inside this one.
				throw error;
			}
			catch (Exception | LinkageError error)
			{
				throw new EngineException(EngineError.ERROR, EngineError.ERROR.getValue(), error.getMessage(), error);
			}
		}

		/**
		 * The engine's own message, without the {@code ", error code N"} the JNI layer
		 * appends to it.
		 *
		 * <p>The other bindings hand the engine's text over as it is, and every port
		 * prints it in {@code path: code: message}; left on, the suffix would be the
		 * one difference in an otherwise identical line. Removed only when the number
		 * is this exception's own code, so a message that merely ends in similar words
		 * is not touched.
		 */
		static String messageOf(IGRException error)
		{
			String message = error.getMessage();
			String suffix = ", error code " + error.getErrorCode();
			if (message != null && message.endsWith(suffix) && message.length() > suffix.length())
				return message.substring(0, message.length() - suffix.length());

			return message;
		}
	}

	// ---------------------------------------------------------------------
	//  The open callback
	// ---------------------------------------------------------------------

	/**
	 * Answers the engine's questions during {@code Open}.
	 *
	 * <p>Nothing may be thrown back into native code: the binding's own wrapper
	 * turns any exception into a generic failure and the cause is lost. So a
	 * failure here --- a password file that cannot be read, say --- is parked, the
	 * engine is told to stop, and {@link #rethrowParked} raises the original once
	 * {@code Open} has returned.
	 */
	static final class Callback extends OpenCallback
	{
		private final Supplier<String> _password;
		private final BooleanSupplier _shouldCancel;
		private final ToIntFunction<String> _logLevelFor;
		private final LogSink _onLogMessage;
		private Throwable _parked;

		Callback(Supplier<String> password, BooleanSupplier shouldCancel, ToIntFunction<String> logLevelFor,
			LogSink onLogMessage)
		{
			_password = password;
			_shouldCancel = shouldCancel;
			_logLevelFor = logLevelFor;
			_onLogMessage = onLogMessage;
		}

		@Override
		public int OnCallback(OpenCallbackRequest request)
		{
			if (_parked != null)
				return isys_docfiltersConstants.IGR_E_ERROR;

			try
			{
				return answer(request);
			}
			catch (Throwable error)
			{
				_parked = error;
				return isys_docfiltersConstants.IGR_E_ERROR;
			}
		}

		private int answer(OpenCallbackRequest request) throws IGRException
		{
			int action = request.GetAction();
			if (action == isys_docfiltersConstants.IGR_OPEN_CALLBACK_ACTION_PASSWORD)
			{
				// Refusing here is what turns into IGR_E_PASSWORD out of Open, which
				// EngineCall then translates like any other failure.
				String candidate = _password != null ? _password.get() : null;
				if (candidate == null)
					return isys_docfiltersConstants.IGR_E_PASSWORD;
				request.GetPassword().SetPassword(candidate);
			}
			else if (action == isys_docfiltersConstants.IGR_OPEN_CALLBACK_ACTION_HEARTBEAT
				&& _shouldCancel != null && _shouldCancel.getAsBoolean())
			{
				// The engine's one interruption point, raised periodically while it
				// parses. doc2text answers the same action the same way.
				return isys_docfiltersConstants.IGR_CANCELLED;
			}
			else if (action == isys_docfiltersConstants.IGR_OPEN_CALLBACK_ACTION_LOG_LEVEL && _logLevelFor != null)
			{
				// The engine asks per module how much to log, so the --log-modules
				// filter is applied by answering none for a module nobody named.
				request.GetLogLevel().SetResult(_logLevelFor.applyAsInt(request.GetLogLevel().GetModule()));
			}
			else if (action == isys_docfiltersConstants.IGR_OPEN_CALLBACK_ACTION_LOG_MESSAGE && _onLogMessage != null)
			{
				_onLogMessage.accept((int) request.GetLogMessage().GetLevel(), request.GetLogMessage().GetModule(),
					request.GetLogMessage().GetMessage());
			}
			return isys_docfiltersConstants.IGR_OK;
		}

		boolean hasParked()
		{
			return _parked != null;
		}

		/** Raises what the callback swallowed, if anything; returns otherwise. */
		void rethrowParked() throws IGRException
		{
			Throwable parked = _parked;
			if (parked == null)
				return;

			if (parked instanceof IGRException)
				throw (IGRException) parked;
			if (parked instanceof RuntimeException)
				throw (RuntimeException) parked;
			if (parked instanceof Error)
				throw (Error) parked;
			throw new IllegalStateException(parked.getMessage(), parked);
		}
	}

	// ---------------------------------------------------------------------
	//  The stream bridge
	// ---------------------------------------------------------------------

	/**
	 * An {@code IGRStream} over a channel, so the engine can read a document from,
	 * or write a canvas to, something other than a named file.
	 *
	 * <p>Seek follows the reference binding's own bridge: a read-only stream clamps
	 * to its end, and every seek is exactly one {@code position} call, which is what
	 * {@code --debug-measure-io} counts. Nothing may be thrown into native code, so
	 * an I/O failure reads as end of stream and a failed seek as staying put.
	 */
	static final class ChannelStream extends IGRStream
	{
		private static final int SEEK_SET = 0;
		private static final int SEEK_CUR = 1;
		private static final int SEEK_END = 2;

		private final SeekableByteChannel _channel;
		private final boolean _readOnly;

		ChannelStream(SeekableByteChannel channel, boolean readOnly)
		{
			_channel = channel;
			_readOnly = readOnly;
		}

		@Override
		public long Read(long size, IGRStream_Data destination)
		{
			try
			{
				ByteBuffer buffer = ByteBuffer.allocate((int) Math.max(0, Math.min(size, Integer.MAX_VALUE)));
				int read = _channel.read(buffer);
				if (read <= 0)
					return 0;

				destination.write(Arrays.copyOf(buffer.array(), read));
				return read;
			}
			catch (IOException e)
			{
				return 0;
			}
		}

		@Override
		public long Seek(long offset, int origin)
		{
			try
			{
				long target;
				switch (origin)
				{
					case SEEK_CUR:
						target = _channel.position() + offset;
						break;
					case SEEK_END:
						target = _channel.size() + offset;
						break;
					case SEEK_SET:
					default:
						target = offset;
						break;
				}

				if (_readOnly)
					target = Math.min(target, _channel.size());
				target = Math.max(target, 0);

				_channel.position(target);
				return _channel.position();
			}
			catch (IOException e)
			{
				try
				{
					return _channel.position();
				}
				catch (IOException ignored)
				{
					return 0;
				}
			}
		}

		@Override
		public long Write(byte[] data)
		{
			try
			{
				ByteBuffer buffer = ByteBuffer.wrap(data);
				while (buffer.hasRemaining())
					_channel.write(buffer);
				return data.length;
			}
			catch (IOException e)
			{
				return 0;
			}
		}

		/** A no-op the caller makes after the engine is done, so the bridge is not collected early. */
		void keepAlive()
		{
		}
	}

	// ---------------------------------------------------------------------
	//  Adapters
	// ---------------------------------------------------------------------

	/** {@link IDocument} over {@code Extractor}. */
	static final class DocumentAdapter implements IDocument
	{
		/** {@code int.TryParse}'s accepted shape, which the reference's page index is read with. */
		private static final Pattern INTEGER = Pattern.compile("^-?[0-9]+$");

		private final Extractor _extractor;
		private final boolean _ownsHandle;

		/** Held, not read: the engine calls back into these for as long as the document is open. */
		@SuppressWarnings("unused")
		private final ChannelStream _stream;
		@SuppressWarnings("unused")
		private final Callback _callback;

		/**
		 * @param ownsHandle false when the handle belongs to something else --- a
		 *        subfile, which the binding models as an Extractor in its own right,
		 *        so opening it yields a document that shares the entry's handle and
		 *        must not close it.
		 */
		DocumentAdapter(Extractor extractor, boolean ownsHandle, ChannelStream stream, Callback callback)
		{
			_extractor = extractor;
			_ownsHandle = ownsHandle;
			_stream = stream;
			_callback = callback;
		}

		@Override
		public int getFileType()
		{
			return EngineCall.run(() -> _extractor.getFileType());
		}

		@Override
		public String getMimeType()
		{
			return EngineCall.run(() -> _extractor.getFileType(IGRFormatWhat.IGR_FORMAT_MIME_TYPE));
		}

		@Override
		public String getFormatName()
		{
			return EngineCall.run(() -> _extractor.getFileType(IGRFormatWhat.IGR_FORMAT_LONG_NAME));
		}

		@Override
		public int getFileCapabilities()
		{
			return EngineCall.run(() -> capabilities(_extractor));
		}

		@Override
		public boolean getEOF()
		{
			return EngineCall.run(() -> _extractor.getEOF());
		}

		@Override
		public String getText(int maxLength)
		{
			return EngineCall.run(() -> _extractor.GetText(maxLength));
		}

		@Override
		public int getPageCount()
		{
			return EngineCall.run(() -> _extractor.GetPageCount());
		}

		@Override
		public IDocumentPage getPage(int pageIndex)
		{
			return EngineCall.run(() -> new PageAdapter(_extractor.GetPage(pageIndex)));
		}

		/**
		 * Drained here rather than handed back as a cursor: {@code GetFirstSubFile}
		 * and {@code GetNextSubFile} advance state on the extractor itself, so
		 * anything else touching the same handle mid-walk loses entries.
		 */
		@Override
		public List<ISubFileEntry> getSubFiles()
		{
			return EngineCall.run(() ->
			{
				List<ISubFileEntry> entries = new ArrayList<>();
				for (SubFile entry = _extractor.GetFirstSubFile(); entry != null; entry = _extractor.GetNextSubFile())
					entries.add(new SubFileAdapter(entry));
				return entries;
			});
		}

		@Override
		public List<ISubFileEntry> getImages()
		{
			return EngineCall.run(() ->
			{
				List<ISubFileEntry> entries = new ArrayList<>();
				for (SubFile entry = _extractor.GetFirstImage(); entry != null; entry = _extractor.GetNextImage())
					entries.add(new SubFileAdapter(entry));
				return entries;
			});
		}

		@Override
		public EngineBookmark getRootBookmark()
		{
			return EngineCall.run(() -> convert(_extractor.GetRootBookmark()));
		}

		@Override
		public void makeDocPack(String fileName, String options)
		{
			EngineCall.run(() -> docPack(() -> _extractor.MakeDocPack(fileName, options != null ? options : "")));
		}

		@Override
		public void makeDocPack(SeekableByteChannel destination, String options)
		{
			EngineCall.run(() ->
			{
				ChannelStream stream = new ChannelStream(destination, false);
				docPack(() -> _extractor.MakeDocPack(stream, options != null ? options : ""));
				// The engine writes the whole pack inside the call; held until it returns.
				stream.keepAlive();
			});
		}

		/**
		 * Runs a {@code MakeDocPack} call. The jar declares it, but a native library
		 * from a release older than the jar has no entry point for it, which the JVM
		 * only finds out on the first call; that is reported as this binding being
		 * unable to write a docpack, not as a generic failure.
		 */
		private static void docPack(Action call) throws Exception
		{
			try
			{
				call.run();
			}
			catch (UnsatisfiedLinkError e)
			{
				throw new EngineException(EngineError.ACTION_NOT_SUPPORTED, EngineError.ACTION_NOT_SUPPORTED.getValue(),
					"this binding's native library cannot write a docpack");
			}
		}

		@Override
		public void close()
		{
			if (_ownsHandle)
				_extractor.close();
		}

		/**
		 * The handle behind this document, for {@code Canvas.RenderPages}, which takes
		 * an {@code Extractor} rather than anything the seam can describe.
		 */
		Extractor inner()
		{
			return _extractor;
		}

		static EngineBookmark convert(Bookmark bookmark) throws IGRException
		{
			if (bookmark == null)
				return null;

			EngineBookmark copy = new EngineBookmark();
			copy.title = bookmark.GetTitle();
			copy.level = bookmark.GetLevel();
			copy.pageIndex = pageIndex(bookmark.GetDestination());

			for (Bookmark child = bookmark.GetFirstChild(); child != null; child = child.GetNextSibling())
				copy.children.add(convert(child));

			return copy;
		}

		/**
		 * The page a bookmark points at, read from its destination the way the
		 * reference binding reads it: a whole decimal integer, or -1.
		 */
		static int pageIndex(String destination)
		{
			if (destination == null || !INTEGER.matcher(destination).matches())
				return -1;

			try
			{
				return Integer.parseInt(destination);
			}
			catch (NumberFormatException e)
			{
				return -1;
			}
		}
	}

	/** {@link IDocumentPage} over {@code Page}. */
	static final class PageAdapter implements IDocumentPage
	{
		private final Page _page;

		PageAdapter(Page page)
		{
			_page = page;
		}

		@Override
		public int getWidth()
		{
			return EngineCall.run(() -> _page.GetWidth());
		}

		@Override
		public int getHeight()
		{
			return EngineCall.run(() -> _page.GetHeight());
		}

		@Override
		public String getAttribute(String name)
		{
			return EngineCall.run(() -> _page.GetAttribute(name));
		}

		@Override
		public List<EngineHyperlink> getHyperlinks()
		{
			return EngineCall.run(() ->
			{
				List<EngineHyperlink> links = new ArrayList<>();
				for (Hyperlink link = _page.GetFirstHyperlink(); link != null; link = _page.GetNextHyperlink())
				{
					EngineHyperlink copy = new EngineHyperlink();
					copy.type = hyperlinkType(link.getType());
					copy.uri = link.getUri();
					copy.ref = link.getRef();
					copy.x = link.getX();
					copy.y = link.getY();
					copy.width = link.getWidth();
					copy.height = link.getHeight();
					links.add(copy);
				}
				return links;
			});
		}

		@Override
		public List<EngineFormElement> getFormElements()
		{
			return EngineCall.run(() ->
			{
				List<EngineFormElement> elements = new ArrayList<>();
				for (FormElement element = _page.GetFirstFormElement(); element != null; element = _page.GetNextFormElement())
				{
					EngineFormElement copy = new EngineFormElement();
					copy.type = named(FormElementType.values(), element.getElementType(), FormElementType.BUTTON);
					copy.name = element.getName();
					copy.x = element.getX();
					copy.y = element.getY();
					copy.width = element.getWidth();
					copy.height = element.getHeight();
					elements.add(copy);
				}
				return elements;
			});
		}

		@Override
		public List<EngineAnnotation> getAnnotations()
		{
			return EngineCall.run(() ->
			{
				List<EngineAnnotation> annotations = new ArrayList<>();
				for (Annotation annotation = _page.GetFirstAnnotation(); annotation != null; annotation = _page.GetNextAnnotation())
				{
					EngineAnnotation copy = new EngineAnnotation();
					copy.type = named(AnnotationKind.values(), annotation.GetAnnotationType().swigValue(), AnnotationKind.UNKNOWN);
					copy.title = annotation.getTitle();
					copy.text = annotation.getText();
					annotations.add(copy);
				}
				return annotations;
			});
		}

		@Override
		public EnginePageElement getRootPageElement()
		{
			return EngineCall.run(() -> convert(_page.GetRootPageElement()));
		}

		@Override
		public void close()
		{
			_page.close();
		}

		/** The handle behind this page, for {@code Canvas.RenderPage}. */
		Page inner()
		{
			return _page;
		}

		/**
		 * The whole tree, copied in one pass rather than lazily: the element walk
		 * reads through the page handle, which the caller is free to close as soon as
		 * this returns.
		 *
		 * <p>Walked with {@code GetFirstChild} and {@code GetNextSibling} rather than
		 * the binding's {@code Children()} iterable, which the Python port found does
		 * not visit every child.
		 */
		static EnginePageElement convert(PageElement element) throws IGRException
		{
			if (element == null)
				return null;

			EnginePageElement copy = new EnginePageElement();
			copy.type = pageElementType(element.GetType());
			copy.depth = element.GetDepth();

			IGR_FRect bounds = element.GetBounds();
			copy.bounds = new EngineRectangle(bounds.getLeft(), bounds.getTop(),
				bounds.getRight() - bounds.getLeft(), bounds.getBottom() - bounds.getTop());

			for (PageElement child = element.GetFirstChild(); child != null; child = child.GetNextSibling())
				copy.children.add(convert(child));

			return copy;
		}

		/**
		 * The engine's number as one of this tool's values.
		 *
		 * <p>A value newer than this tool falls back to a fixed member rather than
		 * failing the page, as the reference's enum cast would keep the number.
		 */
		private static PageElementType pageElementType(int value)
		{
			for (PageElementType type : PageElementType.values())
			{
				if (type.getValue() == value)
					return type;
			}
			return PageElementType.PAGE;
		}

		private static HyperlinkActionType hyperlinkType(int value)
		{
			for (HyperlinkActionType type : HyperlinkActionType.values())
			{
				if (type.getValue() == value)
					return type;
			}
			return HyperlinkActionType.GOTO;
		}

		/** The member whose position is {@code value}, for the enums numbered from zero. */
		private static <T> T named(T[] members, int value, T fallback)
		{
			return value >= 0 && value < members.length ? members[value] : fallback;
		}
	}

	/** {@link IOutputCanvas} over {@code Canvas}. */
	static final class CanvasAdapter implements IOutputCanvas
	{
		private final Canvas _canvas;

		/** Held, not read: the engine writes through it until the canvas is closed. */
		@SuppressWarnings("unused")
		private final ChannelStream _stream;

		CanvasAdapter(Canvas canvas, ChannelStream stream)
		{
			_canvas = canvas;
			_stream = stream;
		}

		/**
		 * The binding has two overloads, and passing a properties object built from
		 * nothing is not the same as passing none --- so the null check picks the
		 * overload rather than defaulting the argument.
		 */
		@Override
		public void renderPage(IDocumentPage page, String options, EnginePageDecorations decorations)
		{
			Page inner = innerPage(page);
			String resolved = options != null ? options : "";

			EngineCall.run(() ->
			{
				if (decorations == null || decorations.isEmpty())
				{
					_canvas.RenderPage(inner, resolved);
					return;
				}

				RenderPageProperties properties = new RenderPageProperties();
				for (EngineRedaction redaction : decorations.redactions)
				{
					// This overload takes edges, not a size.
					properties.AddRedaction(redaction.x, redaction.y, redaction.x + redaction.width,
						redaction.y + redaction.height, new Color(0, 0, 0));
				}

				for (EngineFormValue value : decorations.formValues)
					properties.AddFormValue(value.name, value.value, false);

				_canvas.RenderPage(inner, resolved, properties);
			});
		}

		@Override
		public void renderPages(IDocument document)
		{
			if (!(document instanceof DocumentAdapter))
				throw mismatch("document");

			Extractor inner = ((DocumentAdapter) document).inner();
			EngineCall.run(() -> _canvas.RenderPages(inner));
		}

		@Override
		public void drawImage(int x, int y, byte[] image, String mimeType)
		{
			EngineCall.run(() -> _canvas.DrawImage(x, y, image, mimeType));
		}

		@Override
		public void close()
		{
			_canvas.close();
		}

		private static Page innerPage(IDocumentPage page)
		{
			if (page instanceof PageAdapter)
				return ((PageAdapter) page).inner();

			throw mismatch("page");
		}

		/**
		 * A real canvas was handed something that did not come from the real engine.
		 *
		 * <p>Only reachable by mixing a fake engine's objects with a real canvas, which
		 * no run does. Reported rather than left as a {@code ClassCastException} so the
		 * message says what went wrong.
		 */
		private static EngineException mismatch(String what)
		{
			return new EngineException(EngineError.INVALID_PARAMETER, EngineError.INVALID_PARAMETER.getValue(),
				"this canvas can only render a " + what + " that came from the same engine");
		}
	}

	/** {@link ISubFileEntry} over {@code SubFile}. */
	static final class SubFileAdapter implements ISubFileEntry
	{
		private final SubFile _subfile;

		SubFileAdapter(SubFile subfile)
		{
			_subfile = subfile;
		}

		@Override
		public String getId()
		{
			return EngineCall.run(() -> _subfile.getID());
		}

		@Override
		public String getName()
		{
			return EngineCall.run(() -> _subfile.getName());
		}

		@Override
		public long getSize()
		{
			return EngineCall.run(() -> _subfile.getFileSize());
		}

		/**
		 * The entry's timestamp in local time, or null when its container records
		 * none.
		 *
		 * <p>The binding builds its {@code Date} from the engine's broken-down local
		 * time, so reading it back in the default zone recovers the same fields. An
		 * entry with no timestamp comes back in the year 1900 or before, the
		 * counterpart of the reference's {@code DateTime.MinValue}.
		 */
		@Override
		public LocalDateTime getModified()
		{
			return EngineCall.run(() ->
			{
				Date value = _subfile.getFileTime();
				if (value == null)
					return null;

				LocalDateTime local = LocalDateTime.ofInstant(value.toInstant(), ZoneId.systemDefault());
				return local.getYear() <= 1900 ? null : local;
			});
		}

		@Override
		public IDocument open(int flags, String options)
		{
			return EngineCall.run(() ->
			{
				_subfile.Open(flags, options != null ? options : "");
				return new DocumentAdapter(_subfile, false, null, null);
			});
		}

		@Override
		public void copyTo(String path)
		{
			EngineCall.run(() -> _subfile.CopyTo(path));
		}

		@Override
		public void close()
		{
			_subfile.close();
		}
	}
}
