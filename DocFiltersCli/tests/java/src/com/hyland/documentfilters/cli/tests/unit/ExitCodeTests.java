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

package com.hyland.documentfilters.cli.tests.unit;

import static com.hyland.documentfilters.cli.tests.Assert.assertEquals;

import com.hyland.documentfilters.cli.runtime.DocumentError;
import com.hyland.documentfilters.cli.runtime.ErrorCodes;
import com.hyland.documentfilters.cli.runtime.ExitCode;
import com.hyland.documentfilters.cli.runtime.InitialisationError;
import com.hyland.documentfilters.cli.runtime.InterruptedError;
import com.hyland.documentfilters.cli.runtime.LimitExceededError;
import com.hyland.documentfilters.cli.runtime.NoInputsError;
import com.hyland.documentfilters.cli.runtime.OutputConflictError;
import com.hyland.documentfilters.cli.runtime.UsageError;
import com.hyland.documentfilters.cli.tests.Test;

/**
 * The exit code table, which every implementation shares.
 *
 * <p>{@code isys_doc2text} returns only 0 or 1, and returns 0 even on a usage
 * error, so there was no meaningful parity to preserve beyond "non-zero means
 * trouble". These values are the agreed replacement.
 */
public final class ExitCodeTests
{
	@Test
	public void theAgreedValuesAreStable()
	{
		assertEquals(0, ExitCode.SUCCESS);
		assertEquals(1, ExitCode.PARTIAL_FAILURE);
		assertEquals(2, ExitCode.USAGE_ERROR);
		assertEquals(3, ExitCode.NO_INPUTS_MATCHED);
		assertEquals(4, ExitCode.OUTPUT_CONFLICT);
		assertEquals(5, ExitCode.INITIALISATION_FAILURE);
		assertEquals(6, ExitCode.LIMIT_EXCEEDED);
		assertEquals(130, ExitCode.INTERRUPTED);
	}

	@Test
	public void aUsageErrorExitsTwo()
	{
		UsageError error = new UsageError(ErrorCodes.UNKNOWN_ARGUMENT, "unknown option --nonsense");

		assertEquals(ExitCode.USAGE_ERROR, error.getExitCode());
	}

	@Test
	public void noInputsExitsThree()
	{
		assertEquals(ExitCode.NO_INPUTS_MATCHED, new NoInputsError("nothing matched").getExitCode());
	}

	@Test
	public void anOutputConflictExitsFour()
	{
		OutputConflictError error = new OutputConflictError(ErrorCodes.OUT_FILE_MULTI_INPUT, "several inputs, one --out-file");

		assertEquals(ExitCode.OUTPUT_CONFLICT, error.getExitCode());
	}

	@Test
	public void anInitialisationFailureExitsFive()
	{
		assertEquals(ExitCode.INITIALISATION_FAILURE, new InitialisationError("no license").getExitCode());
	}

	@Test
	public void aLimitBeingExceededExitsSix()
	{
		assertEquals(ExitCode.LIMIT_EXCEEDED, new LimitExceededError("--max-time passed").getExitCode());
	}

	@Test
	public void anInterruptionExitsOneHundredAndThirty()
	{
		assertEquals(ExitCode.INTERRUPTED, new InterruptedError().getExitCode());
	}

	@Test
	public void aSingleDocumentFailureMapsToPartialFailure()
	{
		DocumentError error = new DocumentError("a.docx", "could not open");

		assertEquals(ExitCode.PARTIAL_FAILURE, error.getExitCode());
	}

	@Test
	public void everyErrorCarriesAMachineReadableCode()
	{
		// The corpus matches on the code, not the prose, so the message can be
		// improved without rewriting every fixture.
		assertEquals(ErrorCodes.UNKNOWN_ARGUMENT, new UsageError(ErrorCodes.UNKNOWN_ARGUMENT, "x").getCode());
		assertEquals(ErrorCodes.NO_INPUTS, new NoInputsError("x").getCode());
		assertEquals(ErrorCodes.DOCUMENT_FAILED, new DocumentError("a", "x").getCode());
	}

	@Test
	public void aDocumentErrorRemembersWhichFileFailed()
	{
		DocumentError error = new DocumentError("corpus/a.docx", "could not open");

		assertEquals("corpus/a.docx", error.getPath());
	}
}
