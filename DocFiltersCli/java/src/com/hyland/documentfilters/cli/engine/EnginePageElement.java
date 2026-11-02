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

/** One node of a page's element tree. */
public final class EnginePageElement
{
	/** The kind of node. */
	public PageElementType type = PageElementType.PAGE;

	/** Depth in the tree, as the binding reports it. */
	public int depth;

	/** The binding reports element geometry as a float rectangle, not four ints. */
	public EngineRectangle bounds = new EngineRectangle();

	/** The nodes beneath this one, in document order. */
	public final List<EnginePageElement> children = new ArrayList<>();

	/** Every node beneath this one, depth first. */
	public List<EnginePageElement> allChildren()
	{
		List<EnginePageElement> result = new ArrayList<>();
		collect(this, result);
		return result;
	}

	private static void collect(EnginePageElement node, List<EnginePageElement> result)
	{
		for (EnginePageElement child : node.children)
		{
			result.add(child);
			collect(child, result);
		}
	}
}
