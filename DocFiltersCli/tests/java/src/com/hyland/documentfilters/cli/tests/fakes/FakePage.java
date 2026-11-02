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

import com.hyland.documentfilters.cli.engine.EngineAnnotation;
import com.hyland.documentfilters.cli.engine.EngineFormElement;
import com.hyland.documentfilters.cli.engine.EngineHyperlink;
import com.hyland.documentfilters.cli.engine.EnginePageElement;
import com.hyland.documentfilters.cli.engine.IDocumentPage;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** One page of a {@link FakeDocument}. */
public final class FakePage implements IDocumentPage
{
	/** Scripted answers, read back through {@link #getWidth} and {@link #getHeight}. */
	public int width = 612;
	public int height = 792;

	public final Map<String, String> attributes = new HashMap<>();
	public final List<EngineHyperlink> hyperlinkList = new ArrayList<>();
	public final List<EngineFormElement> formElementList = new ArrayList<>();
	public final List<EngineAnnotation> annotationList = new ArrayList<>();
	public EnginePageElement rootElement;
	public boolean isDisposed;

	@Override
	public int getWidth()
	{
		return width;
	}

	@Override
	public int getHeight()
	{
		return height;
	}

	@Override
	public String getAttribute(String name)
	{
		String value = attributes.get(name);
		return value != null ? value : "";
	}

	@Override
	public List<EngineHyperlink> getHyperlinks()
	{
		return hyperlinkList;
	}

	@Override
	public List<EngineFormElement> getFormElements()
	{
		return formElementList;
	}

	@Override
	public List<EngineAnnotation> getAnnotations()
	{
		return annotationList;
	}

	@Override
	public EnginePageElement getRootPageElement()
	{
		return rootElement;
	}

	@Override
	public void close()
	{
		isDisposed = true;
	}
}
