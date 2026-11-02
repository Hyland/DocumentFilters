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

/** What happens when one file in a batch fails. */
public final class BatchOptions
{
	/**
	 * {@code --on-error}, also written by {@code --fail-fast}. Skipping still exits
	 * non-zero if anything failed.
	 */
	public OnErrorBehaviour onError = OnErrorBehaviour.SKIP;

	/**
	 * True when {@code --on-error} was given explicitly.
	 *
	 * <p>Recorded so that a contradictory pair --- {@code --fail-fast --on-error skip} ---
	 * can be rejected as a usage error rather than silently resolved. None of the
	 * four parsing libraries reliably records which of two different options came
	 * first on the command line, so a precedence rule could not have been made
	 * identical across the ports.
	 */
	public boolean onErrorWasExplicit;

	/** True when {@code --fail-fast} was given. */
	public boolean failFastWasGiven;
}
