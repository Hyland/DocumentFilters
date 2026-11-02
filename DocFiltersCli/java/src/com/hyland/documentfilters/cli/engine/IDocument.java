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

import java.nio.channels.SeekableByteChannel;
import java.util.List;

/** One open document: {@code Extractor}. */
public interface IDocument extends AutoCloseable
{
	/** {@code getFileType()}: the numeric format id. */
	int getFileType();

	/** The format's MIME type. */
	String getMimeType();

	/** The format's long name. */
	String getFormatName();

	/** {@code getFileCapabilities()}: the text/subfiles/hd flag word. */
	int getFileCapabilities();

	/** {@code getEOF()}. */
	boolean getEOF();

	/** {@code getText(maxLength)}. */
	String getText(int maxLength);

	/** {@code getPageCount()}. */
	int getPageCount();

	/** {@code getPage(index)}, zero-based as the binding has it. */
	IDocumentPage getPage(int pageIndex);

	/**
	 * The subfile cursor, drained into a list.
	 *
	 * <p>Drained rather than streamed because the binding's cursor is positional state
	 * on the extractor: interleaving a walk with anything else that touches the same
	 * handle silently loses entries. A list makes that impossible.
	 */
	List<ISubFileEntry> getSubFiles();

	/** The image cursor, drained into a list. */
	List<ISubFileEntry> getImages();

	/** The outline's root, or null when the document has none. */
	EngineBookmark getRootBookmark();

	/** {@code MakeDocPack} onto a file. */
	void makeDocPack(String fileName, String options);

	/** {@code MakeDocPack} onto a stream. */
	void makeDocPack(SeekableByteChannel destination, String options);

	/** Releases the document handle. */
	@Override
	void close();
}
