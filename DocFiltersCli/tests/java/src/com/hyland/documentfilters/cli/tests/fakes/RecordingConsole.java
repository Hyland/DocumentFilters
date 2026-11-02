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

package com.hyland.documentfilters.cli.tests.fakes;

import com.hyland.documentfilters.cli.runtime.IConsole;
import java.io.ByteArrayOutputStream;
import java.io.OutputStream;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;

/**
 * Captures everything the tool would have printed, so that output can be asserted
 * on without a terminal.
 *
 * <p>Standard output and standard error are kept apart deliberately: error text is
 * part of the cross-language contract and is asserted precisely, while progress
 * chatter is not.
 */
public final class RecordingConsole implements IConsole
{
	private final StringBuilder _out = new StringBuilder();
	private final ByteArrayOutputStream _binary = new ByteArrayOutputStream();

	/** Every line written to standard output, in order. */
	public final List<String> outLines = new ArrayList<>();

	/** Every line written to standard error, in order. Part of the contract. */
	public final List<String> errorLines = new ArrayList<>();

	/** Every verbose line. Not part of the contract. */
	public final List<String> verboseLines = new ArrayList<>();

	/** Standard input, for {@code --files-from -}. */
	public final List<String> inputLines = new ArrayList<>();

	/** Answers handed to successive {@link #readPassword} calls, in order. */
	public final Deque<String> passwordResponses = new ArrayDeque<>();

	/** The prompts {@link #readPassword} was called with. */
	public final List<String> passwordPrompts = new ArrayList<>();

	/** Set this to exercise the {@code --password-prompt} paths. */
	public boolean inputInteractive;

	/** How many times {@link #waitForKey} was called, for {@code --pause}. */
	public int keyWaits;

	@Override
	public boolean isInputInteractive()
	{
		return inputInteractive;
	}

	@Override
	public List<String> readAllInputLines()
	{
		return inputLines;
	}

	@Override
	public String readPassword(String prompt)
	{
		passwordPrompts.add(prompt);
		return passwordResponses.pollFirst();
	}

	@Override
	public void waitForKey()
	{
		++keyWaits;
	}

	/** Everything written to the binary stdout stream. */
	public byte[] binaryOutput()
	{
		return _binary.toByteArray();
	}

	/** Standard output as one string. */
	public String output()
	{
		return _out.toString();
	}

	@Override
	public void write(String text)
	{
		_out.append(text);
	}

	@Override
	public void writeLine(String text)
	{
		_out.append(text).append('\n');
		outLines.add(text);
	}

	@Override
	public void writeError(String text)
	{
		errorLines.add(text);
	}

	@Override
	public void writeVerbose(String text)
	{
		verboseLines.add(text);
	}

	@Override
	public OutputStream openStandardOutput()
	{
		return _binary;
	}
}
