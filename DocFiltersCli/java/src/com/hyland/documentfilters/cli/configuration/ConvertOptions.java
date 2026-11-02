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

package com.hyland.documentfilters.cli.configuration;

import com.hyland.documentfilters.cli.planning.FormatSpec;
import com.hyland.documentfilters.cli.planning.PageRange;
import java.util.ArrayList;
import java.util.List;

/** Options that only {@code convert} accepts. */
public final class ConvertOptions
{
	/** {@code --to}: the resolved target format. Required. */
	public FormatSpec format;

	/** {@code --combine}: merge every input into one output document. */
	public boolean combine;

	/** {@code --pages}. Null means every page. */
	public PageRange pages;

	/** {@code --canvas-options}, passed to the engine untouched. */
	public String canvasOptions;

	/** {@code --render-options}, passed to the engine untouched. */
	public String renderOptions;

	/** {@code --no-images}. */
	public boolean noImages;

	/** {@code --include-control-codes}: keep ISYS control codes in text output. */
	public boolean includeControlCodes;

	/** {@code --fingerprint}. */
	public boolean fingerprint;

	/** {@code --watermark}: a file path or a literal string. */
	public String watermark;

	/** {@code --redact}, repeatable. */
	public final List<RedactSpec> redactions = new ArrayList<>();

	/** {@code --form}, repeatable. */
	public final List<FormValue> formValues = new ArrayList<>();

	/** {@code --view}: open the result when done. Best effort, platform specific. */
	public boolean view;

	/** The structure reports emitted during conversion. */
	public final StructureReportOptions reports = new StructureReportOptions();
}
