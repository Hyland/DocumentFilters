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

/** An output canvas: {@code Canvas}. */
public interface IOutputCanvas extends AutoCloseable
{
	/** {@code renderPage}. {@code decorations} is null when the page needs none. */
	void renderPage(IDocumentPage page, String options, EnginePageDecorations decorations);

	/** {@code renderPages(extractor)}: the whole document in one call. */
	void renderPages(IDocument document);

	/**
	 * {@code drawImage}, for {@code --watermark}. The last argument is the image's
	 * MIME type, which the engine uses to pick a decoder, not an options string.
	 */
	void drawImage(int x, int y, byte[] image, String mimeType);

	/** Closes the canvas, which is what flushes and finishes the output. */
	@Override
	void close();
}
