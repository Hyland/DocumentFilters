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

import java.util.List;

/** One page of an open document: {@code Page}. */
public interface IDocumentPage extends AutoCloseable
{
	int getWidth();

	int getHeight();

	/** {@code getAttribute(name)}; empty or null when the page has no such attribute. */
	String getAttribute(String name);

	/** Every hyperlink on the page, drained into a list. */
	List<EngineHyperlink> getHyperlinks();

	/** Every interactive form field on the page, drained into a list. */
	List<EngineFormElement> getFormElements();

	/** Every annotation on the page, drained into a list. */
	List<EngineAnnotation> getAnnotations();

	/** The root of the page's element tree, or null when the page exposes none. */
	EnginePageElement getRootPageElement();

	/** Releases the page handle. */
	@Override
	void close();
}
