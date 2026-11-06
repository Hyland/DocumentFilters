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

import java.io.IOException;
import java.io.OutputStream;
import java.util.List;

/**
 * Everything the tool writes to the terminal, behind one seam so tests can
 * capture output without a console.
 *
 * <p>{@code --silent}, {@code --verbose} and {@code --color} are honoured by the
 * implementation, not by callers: call sites always say what they mean and let
 * this decide whether it is shown.
 */
public interface IConsole
{
	/** Report output --- the answer the user asked for. Goes to stdout. */
	void write(String text);

	/** Report output followed by a newline. */
	void writeLine(String text);

	/** An error message. Goes to stderr, and is part of the cross-language contract. */
	void writeError(String text);

	/** Progress or diagnostic chatter. Suppressed unless {@code --verbose}. */
	void writeVerbose(String text);

	/**
	 * The raw stdout stream, for {@code --stdout} with a binary format, so that PDF
	 * and image bytes reach it unchanged.
	 */
	OutputStream openStandardOutput();

	/** True when stdin is an interactive terminal, which {@code --password-prompt} requires. */
	boolean isInputInteractive();

	/**
	 * Every line of standard input, for {@code --files-from -}.
	 *
	 * <p>Reading stdin belongs here rather than on {@link IFileSystem}: "-" is not
	 * a path, and routing it through the filesystem would leave tests unable to
	 * supply it.
	 */
	List<String> readAllInputLines() throws IOException;

	/**
	 * Prompts for a password and reads it without echoing, for
	 * {@code --password-prompt}. Returns null when the user cancels.
	 */
	String readPassword(String prompt);

	/** Waits for a keypress between documents, for {@code --pause}. */
	void waitForKey();
}
