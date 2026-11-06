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
import com.hyland.documentfilters.cli.engine.EnginePageDecorations;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.IDocumentPage;
import com.hyland.documentfilters.cli.engine.IOutputCanvas;
import com.hyland.documentfilters.cli.planning.CanvasType;
import java.util.ArrayList;
import java.util.List;

/** A canvas that records what was drawn on it instead of writing bytes. */
public final class FakeCanvas implements IOutputCanvas
{
	/** The path it was opened on, or null for a stream canvas. */
	public final String target;
	public final CanvasType type;
	public final String options;

	/** Pages rendered onto it, in order. */
	public final List<IDocumentPage> renderedPages = new ArrayList<>();

	/** Decorations passed with each rendered page; null where none were given. */
	public final List<EnginePageDecorations> decorations = new ArrayList<>();

	/** Documents rendered whole, through the one-call path. */
	public final List<IDocument> renderedDocuments = new ArrayList<>();

	public int imagesDrawn;
	public boolean isDisposed;

	/** Set to fail rendering, the way a full disk or a missing font does. */
	public EngineError renderFailure;

	public FakeCanvas(String target, CanvasType type, String options)
	{
		this.target = target;
		this.type = type;
		this.options = options;
	}

	@Override
	public void renderPage(IDocumentPage page, String options, EnginePageDecorations decorations)
	{
		if (renderFailure != null)
			throw FakeDocumentFiltersEngine.failure(renderFailure, "render failed");

		renderedPages.add(page);
		this.decorations.add(decorations);
	}

	@Override
	public void renderPages(IDocument document)
	{
		if (renderFailure != null)
			throw FakeDocumentFiltersEngine.failure(renderFailure, "render failed");

		renderedDocuments.add(document);
	}

	@Override
	public void drawImage(int x, int y, byte[] image, String mimeType)
	{
		++imagesDrawn;
	}

	@Override
	public void close()
	{
		isDisposed = true;
	}
}
