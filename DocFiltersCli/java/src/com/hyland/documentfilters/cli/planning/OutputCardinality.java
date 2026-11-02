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

package com.hyland.documentfilters.cli.planning;

/**
 * Who owns the output target.
 *
 * <p>This is one axis only: whether each input writes its own output or they all
 * share one. How many <em>files</em> a single input produces is a separate
 * question, answered by {@link FormatSpec#isSingleFile} --- a .docx converted to
 * PNG writes one file per page but is still {@link #PER_INPUT}. Keeping the two
 * apart is what stops {@code -o} from ever depending on {@code --to}.
 */
public enum OutputCardinality
{
	/** Each input owns its output, inside {@code --out-dir}. Printed as {@code per-input}. */
	PER_INPUT,

	/** Every input merges into one document: {@code --combine --out-file}. Printed as {@code combined}. */
	COMBINED,

	/** A single input to a single {@code --out-file}. Printed as {@code single-target}. */
	SINGLE_TARGET,

	/** A single-file format streamed to stdout. Printed as {@code stream}. */
	STREAM
}
