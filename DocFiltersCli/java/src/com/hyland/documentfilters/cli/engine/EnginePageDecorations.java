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

import java.util.ArrayList;
import java.util.List;

/**
 * What {@code --redact} and {@code --form} add to one page before it renders.
 *
 * <p>Carried as data rather than as the binding's {@code RenderPageProperties}: the
 * adapter builds that object, and a fake can assert on these values without one.
 */
public final class EnginePageDecorations
{
	/** The regions to black out, in page coordinates. */
	public final List<EngineRedaction> redactions = new ArrayList<>();

	/** The field assignments to apply before rendering. */
	public final List<EngineFormValue> formValues = new ArrayList<>();

	/** True when this page needs no decoration at all, which is the common case. */
	public boolean isEmpty()
	{
		return redactions.isEmpty() && formValues.isEmpty();
	}
}
