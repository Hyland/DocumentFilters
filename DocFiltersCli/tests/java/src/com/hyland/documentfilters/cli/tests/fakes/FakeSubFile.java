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
import com.hyland.documentfilters.cli.engine.IDocument;
import com.hyland.documentfilters.cli.engine.ISubFileEntry;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;

/** One entry inside a fake container. */
public final class FakeSubFile implements ISubFileEntry
{
	public String name;
	public String id;
	public long size;
	public LocalDateTime modified;

	/** What this entry is when opened, for nested containers. */
	public FakeDocument content;

	/** Set to fail {@link #open}. */
	public EngineError openFailure;

	/** Set to fail {@link #copyTo}, the way an unreadable entry does. */
	public EngineError copyFailure;

	/** Paths this entry was copied to. */
	public final List<String> copiedTo = new ArrayList<>();

	public boolean isDisposed;

	public FakeSubFile(String name)
	{
		this(name, "0001");
	}

	public FakeSubFile(String name, String id)
	{
		this.name = name;
		this.id = id;
	}

	@Override
	public String getId()
	{
		return id;
	}

	@Override
	public String getName()
	{
		return name;
	}

	@Override
	public long getSize()
	{
		return size;
	}

	@Override
	public LocalDateTime getModified()
	{
		return modified;
	}

	@Override
	public IDocument open(int flags, String options)
	{
		if (openFailure != null)
			throw FakeDocumentFiltersEngine.failure(openFailure, "could not open subfile " + name);

		return content != null ? content : new FakeDocument(name);
	}

	@Override
	public void copyTo(String path)
	{
		if (copyFailure != null)
			throw FakeDocumentFiltersEngine.failure(copyFailure, "could not copy subfile " + name);

		copiedTo.add(path);
	}

	@Override
	public void close()
	{
		isDisposed = true;
	}
}
