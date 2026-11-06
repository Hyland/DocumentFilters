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
import java.nio.channels.SeekableByteChannel;
import java.util.List;
import java.util.function.BooleanSupplier;
import java.util.function.Supplier;
import java.util.function.ToIntFunction;

/**
 * The engine itself: what Document Filters offers before a document exists.
 *
 * <p><b>The engine seam.</b> One interface method per Document Filters call, with no
 * logic of its own. Everything above it --- the conversion paths, the reports, the
 * batch runner --- talks to these interfaces, so all of it can be tested without the
 * native library, a license, or a real document.
 *
 * <p>The return types are ours rather than the binding's. Every binding class is a
 * live native handle, so a fake could not stand in for one, and the four ports of
 * this tool have to agree byte for byte: a value type declared here is the same in
 * all four, where a binding type is whatever each language's binding happens to
 * expose.
 *
 * <p>The four handle-owning interfaces are {@link AutoCloseable}, so each is held in
 * try-with-resources and never left to the garbage collector:
 * {@code IGR_E_TOO_MANY_HANDLES} is what running out looks like.
 *
 * <p>The rule for anything added here: it maps one call, and it decides nothing.
 * A method that chooses, filters or formats belongs above the seam, where it can be
 * tested as a pure function.
 *
 * <p>Every method may throw {@link EngineException}.
 */
public interface IDocumentFiltersEngine extends AutoCloseable
{
	/**
	 * {@code DocumentFilters.Initialize}. {@code libraryPath} names the directory
	 * holding the engine binaries.
	 *
	 * @throws EngineException the library was not found, or the license was rejected.
	 */
	void initialize(String licenseKey, String libraryPath);

	/**
	 * {@code GetExtractor} followed by {@code Open}, which the engine applies as one
	 * step.
	 *
	 * <p>The two are not separable here on purpose: the open flags take effect
	 * <em>during</em> open, so handing back an unopened extractor would invite exactly
	 * the mistake of opening with the wrong ones.
	 *
	 * <p>{@code flags} is the complete flag set, format bit included, and reaches the
	 * engine unaltered. Whoever decides the format has to be the one that names it ---
	 * see {@code ExtractorOpenSettings}.
	 *
	 * <p>{@code password} is asked for the document's password when the engine
	 * requests one, and returns null to give up --- which surfaces as an
	 * {@link EngineException} carrying {@link EngineError#PASSWORD}.
	 *
	 * <p>{@code shouldCancel} is polled while the engine works, and returning true
	 * stops the open with {@link EngineError#CANCELLED}. It covers <em>this call
	 * only</em>: nothing else in the API takes a callback, so a page loop cannot be
	 * interrupted once the document is open.
	 *
	 * <p>{@code shouldCancel}, {@code logLevelFor} and {@code onLogMessage} may be null.
	 *
	 * @throws EngineException the document could not be opened.
	 */
	IDocument openDocument(String path, int flags, String options, Supplier<String> password,
		BooleanSupplier shouldCancel, ToIntFunction<String> logLevelFor, LogSink onLogMessage);

	/**
	 * The same, reading the document from {@code source} rather than from a path.
	 *
	 * <p>For the debug switches that have to control how bytes reach the engine:
	 * {@code --debug-use-memory-stream}, {@code --debug-truncate} and
	 * {@code --debug-measure-io}. {@code displayName} is what failures are reported
	 * against, since a stream has no name of its own.
	 *
	 * <p>A channel rather than an {@code InputStream} because the engine seeks: the
	 * binding reads a stream through {@code IGRStream}, whose {@code Seek} has to be
	 * answered.
	 *
	 * @throws EngineException the document could not be opened.
	 */
	IDocument openDocument(SeekableByteChannel source, String displayName, int flags, String options, Supplier<String> password,
		BooleanSupplier shouldCancel, ToIntFunction<String> logLevelFor, LogSink onLogMessage);

	/**
	 * Points the engine at a directory for its scratch files --- {@code --temp-dir}.
	 * A library-wide setting, so it is applied once, immediately after
	 * {@link #initialize}.
	 */
	void setTempPath(String path);

	/**
	 * The engine's own memory report --- {@code -x} and {@code --xx}. Free-form text
	 * the engine composes, printed verbatim: its content is not contract.
	 */
	String getMemoryStatus();

	/**
	 * {@code GetExtractor} and {@code GetFileType}, <em>without</em> opening: what the
	 * file is, without reading it.
	 *
	 * @throws EngineException the file could not be reached or recognised.
	 */
	EngineFileType identifyFile(String path);

	/**
	 * The same, from a stream --- so the debug switches that change how bytes arrive
	 * reach identify too.
	 *
	 * @throws EngineException the file could not be reached or recognised.
	 */
	EngineFileType identifyFile(SeekableByteChannel source, String displayName);

	/** {@code MakeOutputCanvas} onto a file. */
	IOutputCanvas makeCanvas(String target, CanvasType type, String options);

	/**
	 * {@code MakeOutputCanvas} onto a stream, for {@code --stdout} and {@code --fingerprint}.
	 * Seekable for the same reason as the input form: finishing a binary format means
	 * seeking back to patch a header.
	 */
	IOutputCanvas makeCanvas(SeekableByteChannel target, CanvasType type, String options);

	/** {@code GetSupportedFormats}, for the formats command. */
	List<EngineFormat> getSupportedFormats();

	/** {@code GetAvailableOptions}, for the options command. */
	List<EngineOption> getAvailableOptions();

	/** The binding's version, for the version command. */
	String describeVersion();

	/** Releases the engine instance. */
	@Override
	void close();
}
