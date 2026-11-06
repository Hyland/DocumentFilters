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

import com.hyland.documentfilters.cli.configuration.ConsoleOptions;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.Console;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.FilterOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.lang.reflect.Method;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * The real console: stdout for reports, stderr for errors.
 *
 * <p>Both streams are UTF-8 whatever the platform's default, so the bytes a report
 * produces are the same on every host; a character UTF-8 cannot carry (an
 * unpaired surrogate) is written as U+FFFD, as the reference's {@code Encoding.UTF8}
 * writes it --- not as the '?' a {@code PrintStream} would substitute. Line ends are the host's, written
 * explicitly, and a "\n" inside the text is written as it is --- the same as the
 * reference, whose {@code WriteLine} appends the host's terminator and leaves the
 * text alone.
 */
public final class SystemConsole implements IConsole
{
	// Built from the code point rather than written as an escape sequence, so no
	// source file in any port carries a literal control byte.
	private static final String ESCAPE = String.valueOf((char) 27);
	private static final String RED = ESCAPE + "[31m";
	private static final String DIM = ESCAPE + "[2m";
	private static final String RESET = ESCAPE + "[0m";

	private static final String NEWLINE = System.lineSeparator();

	private final ConsoleOptions _options;
	private final boolean _colorErrors;
	private final boolean _colorVerbose;

	private final OutputStream _rawOut;
	private final Writer _out;
	private final Writer _err;
	private BufferedReader _in;

	/**
	 * The console for these options, colouring only what goes to a terminal.
	 *
	 * <p>Java can only ask whether stdin and stdout <em>together</em> are a terminal,
	 * so stderr is taken to be one exactly when they are. A run with only stderr
	 * redirected therefore still gets colour codes in its error file; the reverse
	 * case loses colour it could have had. Neither changes a byte of stdout.
	 */
	public SystemConsole(ConsoleOptions options)
	{
		this(options, shouldColor(options.color, !isTerminal()), shouldColor(options.color, !isTerminal()));
	}

	public SystemConsole(ConsoleOptions options, boolean colorErrors, boolean colorVerbose)
	{
		_options = options;
		_colorErrors = colorErrors;
		_colorVerbose = colorVerbose;
		_rawOut = new FileOutputStream(FileDescriptor.out);
		_out = utf8(new BufferedOutputStream(_rawOut, 64 * 1024));
		_err = utf8(new FileOutputStream(FileDescriptor.err));
	}

	/** Colour is used only when asked for and when the stream is a terminal, so a redirected log stays plain text. */
	public static boolean shouldColor(boolean requested, boolean streamRedirected)
	{
		return requested && !streamRedirected;
	}

	@Override
	public void write(String text)
	{
		if (_options.silent)
			return;

		print(_out, text);
		flush(_out);
	}

	@Override
	public void writeLine(String text)
	{
		if (_options.silent)
			return;

		print(_out, text);
		print(_out, NEWLINE);
		flush(_out);
	}

	@Override
	public void writeError(String text)
	{
		// Whatever stdout has buffered goes first, so the two read in order on a terminal.
		flush(_out);
		print(_err, _colorErrors ? RED + text + RESET : text);
		print(_err, NEWLINE);
		flush(_err);
	}

	@Override
	public void writeVerbose(String text)
	{
		if (!_options.verbose || _options.silent)
			return;

		writeLine(_colorVerbose ? DIM + text + RESET : text);
	}

	/**
	 * The raw stdout, behind whatever text is still buffered for it.
	 *
	 * <p>Closing what is returned only flushes it: closing the real descriptor would
	 * take stdout away from everything that runs afterwards.
	 */
	@Override
	public OutputStream openStandardOutput()
	{
		flush(_out);
		return new FilterOutputStream(_rawOut)
		{
			@Override
			public void write(byte[] buffer, int offset, int length) throws IOException
			{
				out.write(buffer, offset, length);
			}

			@Override
			public void close() throws IOException
			{
				flush();
			}
		};
	}

	@Override
	public boolean isInputInteractive()
	{
		return isTerminal();
	}

	@Override
	public List<String> readAllInputLines() throws IOException
	{
		List<String> lines = new ArrayList<>();
		BufferedReader in = input();
		String line;
		while ((line = in.readLine()) != null)
			lines.add(line);
		return lines;
	}

	/**
	 * Reads a password without echoing it.
	 *
	 * <p>Through {@link Console#readPassword}, which hides the input but has no
	 * cancel key: Escape is just another character, and only end of input (Ctrl+Z
	 * or Ctrl+D) cancels. The reference also echoes a '*' per character, which the
	 * JDK cannot do.
	 */
	@Override
	public String readPassword(String prompt)
	{
		flush(_out);
		Console console = System.console();
		if (console == null)
			return null;

		char[] password = console.readPassword("%s", prompt);
		return password != null ? new String(password) : null;
	}

	/**
	 * Waits for the user, for {@code --pause}.
	 *
	 * <p>The JDK reads a terminal a line at a time, so this waits for Enter rather
	 * than for any key.
	 */
	@Override
	public void waitForKey()
	{
		flush(_out);
		try
		{
			int read;
			do
			{
				read = System.in.read();
			}
			while (read != -1 && read != '\n');
		}
		catch (IOException e)
		{
			// Nothing to wait on.
		}
	}

	private BufferedReader input()
	{
		if (_in == null)
			_in = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
		return _in;
	}

	/**
	 * True when stdin and stdout are a terminal.
	 *
	 * <p>Before Java 22 that is {@code System.console() != null}. From 22 a console
	 * exists even when redirected and {@code Console.isTerminal()} gives the answer
	 * instead, so that is asked for first, reflectively, as it is newer than the
	 * Java 8 this compiles against.
	 */
	private static boolean isTerminal()
	{
		Console console = System.console();
		if (console == null)
			return false;

		try
		{
			Method isTerminal = Console.class.getMethod("isTerminal");
			return (Boolean) isTerminal.invoke(console);
		}
		catch (ReflectiveOperationException | RuntimeException e)
		{
			return true;
		}
	}

	private static Writer utf8(OutputStream stream)
	{
		byte[] replacement = { (byte) 0xEF, (byte) 0xBF, (byte) 0xBD };
		return new OutputStreamWriter(stream, StandardCharsets.UTF_8.newEncoder()
			.onMalformedInput(CodingErrorAction.REPLACE)
			.onUnmappableCharacter(CodingErrorAction.REPLACE)
			.replaceWith(replacement));
	}

	/**
	 * Writes to a console stream. A console that has gone away is not worth failing
	 * the run over, as a {@code PrintStream} would also have decided.
	 */
	private static void print(Writer stream, String text)
	{
		try
		{
			stream.write(text != null ? text : "");
		}
		catch (IOException e)
		{
			// Nowhere left to report it.
		}
	}

	private static void flush(Writer stream)
	{
		try
		{
			stream.flush();
		}
		catch (IOException e)
		{
			// As print.
		}
	}
}
