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

package com.hyland.documentfilters.cli.runtime;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Stable machine-readable identifiers for every error this tool can report.
 *
 * <p>These strings appear in the conformance corpus, so all four language
 * implementations must raise the same code for the same situation. The
 * human-readable message is also part of the contract; it is authored here
 * rather than left to a CLI parsing library, so that it can be identical
 * everywhere.
 */
public final class ErrorCodes
{
	public static final String UNKNOWN_COMMAND = "E_UNKNOWN_COMMAND";
	public static final String UNKNOWN_ARGUMENT = "E_UNKNOWN_ARGUMENT";
	public static final String MISSING_VALUE = "E_MISSING_VALUE";
	public static final String MISSING_ARGUMENT = "E_MISSING_ARGUMENT";
	public static final String BAD_VALUE = "E_BAD_VALUE";
	public static final String BAD_PAGE_RANGE = "E_BAD_PAGE_RANGE";
	public static final String BAD_TEMPLATE = "E_BAD_TEMPLATE";
	public static final String UNKNOWN_FORMAT = "E_UNKNOWN_FORMAT";
	public static final String MUTUALLY_EXCLUSIVE = "E_MUTUALLY_EXCLUSIVE";
	public static final String COMBINE_UNSUPPORTED = "E_COMBINE_UNSUPPORTED";
	public static final String STDOUT_NEEDS_SINGLE_FILE = "E_STDOUT_NEEDS_SINGLE_FILE";
	public static final String OUT_FILE_NEEDS_SINGLE_FILE = "E_OUTFILE_NEEDS_SINGLE_FILE";
	public static final String OUT_NAME_NEEDS_OUT_DIR = "E_OUTNAME_NEEDS_OUTDIR";
	public static final String TEMPLATE_NEEDS_PAGE = "E_TEMPLATE_NEEDS_PAGE";
	public static final String REPORT_NEEDS_HI_DEF = "E_REPORT_NEEDS_HIDEF";
	public static final String OUT_FILE_MULTI_INPUT = "E_OUTFILE_MULTI_INPUT";
	public static final String OUTPUT_EXISTS = "E_OUTPUT_EXISTS";
	public static final String OUTPUT_COLLISION = "E_OUTPUT_COLLISION";
	public static final String UNSAFE_OUTPUT = "E_UNSAFE_OUTPUT";
	public static final String OUTPUT_IS_INPUT = "E_OUTPUT_IS_INPUT";
	public static final String NO_INPUTS = "E_NO_INPUTS";
	public static final String INIT_FAILED = "E_INIT_FAILED";
	public static final String LIMIT_EXCEEDED = "E_LIMIT_EXCEEDED";
	public static final String DOCUMENT_FAILED = "E_DOCUMENT_FAILED";
	public static final String INTERRUPTED = "E_INTERRUPTED";

	/**
	 * Every code above, so that the tool's error vocabulary can be enumerated
	 * without reflection.
	 *
	 * <p>A hand-written list beside the declarations, in the same idiom as
	 * {@code ArgumentTable.COMMANDS}, and for the same reason: C++17 has no
	 * reflection, and the ports translate these declarations rather than
	 * rewriting them.
	 *
	 * <p>{@code ErrorCodeTests} checks it against the fixtures in both directions,
	 * so a code declared here that no case provokes fails, and so does a case
	 * naming a code that is not here.
	 */
	public static final List<String> ALL = Collections.unmodifiableList(Arrays.asList(
		UNKNOWN_COMMAND, UNKNOWN_ARGUMENT, MISSING_VALUE, MISSING_ARGUMENT, BAD_VALUE, BAD_PAGE_RANGE,
		BAD_TEMPLATE, UNKNOWN_FORMAT, MUTUALLY_EXCLUSIVE, COMBINE_UNSUPPORTED,
		STDOUT_NEEDS_SINGLE_FILE, OUT_FILE_NEEDS_SINGLE_FILE, OUT_NAME_NEEDS_OUT_DIR, TEMPLATE_NEEDS_PAGE, REPORT_NEEDS_HI_DEF,
		OUT_FILE_MULTI_INPUT, OUTPUT_EXISTS, OUTPUT_COLLISION, UNSAFE_OUTPUT, OUTPUT_IS_INPUT, NO_INPUTS, INIT_FAILED,
		LIMIT_EXCEEDED, DOCUMENT_FAILED, INTERRUPTED));

	private ErrorCodes()
	{
	}
}
