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

import com.hyland.documentfilters.cli.engine.EngineBookmark;
import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.IDocumentPage;
import com.hyland.documentfilters.cli.engine.ISubFileEntry;
import java.nio.channels.SeekableByteChannel;
import java.util.ArrayList;
import java.util.List;

/** A document the fake engine can hand out. */
public final class FakeDocument implements IDocument
{
	private int _textOffset;

	public final String path;

	/** What {@link #getText} streams out, in chunks as the real one does. */
	public String text = "";

	/** When set, {@code openDocument} demands exactly this password. */
	public String password;

	/** Scripted answers. The interface members below are the calls over them. */
	public int fileType = 254;
	public String mimeType = "application/octet-stream";
	public String formatName = "Fake Format";
	public int capabilities;

	public int pageCount;
	public final List<FakePage> pages = new ArrayList<>();
	public final List<FakeSubFile> subFileEntries = new ArrayList<>();
	public final List<FakeSubFile> imageEntries = new ArrayList<>();
	public EngineBookmark bookmarks;

	/** Set to make a call on the open document fail. */
	public EngineError readFailure;

	/** Which call {@link #readFailure} applies to; null means all of them. */
	public String readFailureOn;

	public boolean wasOpened;
	public boolean isDisposed;
	public int openFlags;
	public String openOptions;
	public final List<String> docPacksWritten = new ArrayList<>();

	public FakeDocument(String path)
	{
		this.path = path;
	}

	@Override
	public int getFileType()
	{
		check("GetFileType");
		return fileType;
	}

	@Override
	public String getMimeType()
	{
		check("GetMimeType");
		return mimeType;
	}

	@Override
	public String getFormatName()
	{
		check("GetFormatName");
		return formatName;
	}

	@Override
	public int getFileCapabilities()
	{
		check("GetFileCapabilities");
		return capabilities;
	}

	@Override
	public boolean getEOF()
	{
		check("GetEOF");
		return _textOffset >= text.length();
	}

	@Override
	public String getText(int maxLength)
	{
		check("GetText");

		int take = Math.min(maxLength, text.length() - _textOffset);
		if (take <= 0)
			return "";

		String chunk = text.substring(_textOffset, _textOffset + take);
		_textOffset += take;
		return chunk;
	}

	@Override
	public int getPageCount()
	{
		check("GetPageCount");
		return !pages.isEmpty() ? pages.size() : pageCount;
	}

	@Override
	public IDocumentPage getPage(int pageIndex)
	{
		check("GetPage");

		if (pageIndex < 0 || pageIndex >= getPageCount())
			throw FakeDocumentFiltersEngine.failure(EngineError.NOT_FOUND, "no page " + pageIndex + " in " + path);

		// A document given only a page count, not page contents, still renders.
		while (pages.size() <= pageIndex)
			pages.add(new FakePage());

		return pages.get(pageIndex);
	}

	@Override
	public List<ISubFileEntry> getSubFiles()
	{
		check("SubFiles");
		// A copy, so the caller cannot mutate the scripted list through the interface.
		return new ArrayList<ISubFileEntry>(subFileEntries);
	}

	@Override
	public List<ISubFileEntry> getImages()
	{
		check("Images");
		return new ArrayList<ISubFileEntry>(imageEntries);
	}

	@Override
	public EngineBookmark getRootBookmark()
	{
		check("RootBookmark");
		return bookmarks;
	}

	@Override
	public void makeDocPack(String fileName, String options)
	{
		check("MakeDocPack");
		docPacksWritten.add(fileName);
	}

	@Override
	public void makeDocPack(SeekableByteChannel destination, String options)
	{
		check("MakeDocPack");
		docPacksWritten.add("<stream>");
	}

	@Override
	public void close()
	{
		isDisposed = true;
	}

	private void check(String call)
	{
		if (readFailure != null && (readFailureOn == null || readFailureOn.equals(call)))
			throw FakeDocumentFiltersEngine.failure(readFailure, call + " failed on " + path);
	}
}
