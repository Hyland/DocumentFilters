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

/**
 * Timing, limits and debugging aids. Their output is not part of the cross-language
 * contract, because timings and memory figures vary per run.
 */
public final class DiagnosticOptions
{
	public boolean time;

	public boolean logTime;

	public boolean logMemory;

	public boolean progress;

	public boolean pause;

	/** {@code --max-memory}, in megabytes. Null means no limit. */
	public Long maxMemory;

	/** {@code --max-time}, in seconds, for the whole run. */
	public Integer maxTime;

	/** {@code --timeout}, in seconds, per document. */
	public Integer timeout;

	/**
	 * How often to report the engine's memory status, from
	 * {@code IGR_Get_Memory_Status}.
	 *
	 * <ul>
	 * <li>0: never --- the default</li>
	 * <li>1: {@code -x}: once, at the end of the run</li>
	 * <li>2: {@code --xx}: after every document, and at the end of the run</li>
	 * </ul>
	 *
	 * <p>A level rather than two switches, so {@code --xx} implies {@code -x} without
	 * the two needing to be reconciled.
	 *
	 * <p>Distinct from {@code --logmem}, which samples the process's own memory use
	 * while a document is being processed and reports the peak. This reports what
	 * the engine says about its allocations at a moment in time.
	 */
	public int memoryStatusLevel;

	/**
	 * {@code --log-level} (or {@code --debug-log-level}): how much the low-level
	 * readers log. One of none, crit, error, warn, info, debug, trace.
	 *
	 * <p>Wired to the engine's log callbacks, which makes this the one diagnostic
	 * here a customer is likely to reach for --- it is the fastest way to find out
	 * why a particular document will not parse.
	 */
	public String logLevel;

	/**
	 * {@code --debug-log-level}: the same setting under its original name.
	 *
	 * <p>Kept in its own field rather than both aliases writing
	 * {@link #logLevel}: they carry a value, so two arguments sharing one
	 * field would let {@code --log-level trace --debug-log-level info} silently
	 * drop one of the two. {@link #effectiveLogLevel} derives the answer, and
	 * the validator rejects the pair when they disagree.
	 */
	public String debugLogLevel;

	/** {@code --log-modules}: which modules log. */
	public String logModules;

	/** {@code --debug-log-modules}: the same setting under its original name. */
	public String debugLogModules;

	/** {@code --debug-use-memory-stream}: read the document through a memory stream. */
	public boolean useMemoryStream;

	/** {@code --debug-measure-io}: report I/O counts and volumes per document. */
	public boolean measureIo;

	/**
	 * {@code --debug-truncate <bytes>}: read only the first N bytes of each input,
	 * for testing how gracefully a format handles a damaged or partial file.
	 */
	public Long truncateInputBytes;

	/** The log level actually used, from whichever alias was given. */
	public String effectiveLogLevel()
	{
		return logLevel != null ? logLevel : debugLogLevel;
	}

	/** The module list actually used, from whichever alias was given. */
	public String effectiveLogModules()
	{
		return logModules != null ? logModules : debugLogModules;
	}
}
