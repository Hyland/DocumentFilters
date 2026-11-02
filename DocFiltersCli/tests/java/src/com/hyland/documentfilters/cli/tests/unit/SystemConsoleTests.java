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

import com.hyland.documentfilters.cli.configuration.ConsoleOptions;
import com.hyland.documentfilters.cli.runtime.SystemConsole;
import com.hyland.documentfilters.cli.tests.Test;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

/**
 * {@code --color}, asserted against the real console rather than a fake.
 *
 * <p>{@code RecordingConsole} deliberately knows nothing about colour: its
 * {@code errorLines} are the cross-language contract and are asserted character for
 * character, so letting escape sequences into them would mean every one of those
 * assertions had to strip ANSI first --- and a wrong sequence would then pass.
 * Colour is presentation, the fake captures content. That leaves
 * {@link SystemConsole} as the only thing that can be asserted on here, which is the
 * point: a flag tested only through "the option got set" is what let nine others
 * sit unimplemented.
 *
 * <p>The reference redirects {@code Console.Out} and {@code Console.Error} in
 * process. {@code System.setOut} would not see anything here, because
 * {@code SystemConsole} writes to the process's descriptors directly rather than
 * through {@code System.out}; so each case runs {@link Child} in a JVM of its own,
 * with its streams redirected, and asserts on the bytes it wrote.
 */
public final class SystemConsoleTests
{
	private static final String ESCAPE = String.valueOf((char) 27);

	private static final String NEWLINE = System.lineSeparator();

	/** What one child run left on its two streams. */
	private static final class Streams
	{
		final byte[] out;
		final byte[] err;

		Streams(byte[] out, byte[] err)
		{
			this.out = out;
			this.err = err;
		}

		String out()
		{
			return new String(out, StandardCharsets.UTF_8);
		}

		String err()
		{
			return new String(err, StandardCharsets.UTF_8);
		}
	}

	/** Runs {@link Child} with {@code mode} and returns what it wrote. */
	private static Streams run(String mode) throws IOException, InterruptedException
	{
		String java = System.getProperty("java.home") + File.separator + "bin" + File.separator + "java";
		Process process = new ProcessBuilder(java, "-cp", System.getProperty("java.class.path"), Child.class.getName(), mode).start();
		process.getOutputStream().close();

		// Both are a few bytes, well inside a pipe's buffer, so reading one after the
		// other cannot deadlock.
		byte[] out = readAll(process.getInputStream());
		byte[] err = readAll(process.getErrorStream());
		int exitCode = process.waitFor();

		assertEquals(0, exitCode, "child " + mode + " failed: " + new String(err, StandardCharsets.UTF_8));
		return new Streams(out, err);
	}

	private static byte[] readAll(InputStream stream) throws IOException
	{
		ByteArrayOutputStream buffer = new ByteArrayOutputStream();
		byte[] chunk = new byte[4096];
		int read;
		while ((read = stream.read(chunk)) != -1)
			buffer.write(chunk, 0, read);
		return buffer.toByteArray();
	}

	// ------------------------------------------------------------------------ the rule

	@Test
	public void colourIsUsedOnlyWhenAskedForAndTheStreamCanShowIt()
	{
		// { requested, redirected, expected }: asked for at a terminal, asked for but
		// redirected, and not asked for either way.
		boolean[][] rows = { { true, false, true }, { true, true, false }, { false, false, false }, { false, true, false } };

		for (boolean[] row : rows)
			assertEquals(row[2], SystemConsole.shouldColor(row[0], row[1]), row[0] + "/" + row[1]);
	}

	// ----------------------------------------------------------------- what is written

	@Test
	public void anErrorIsWrappedInRed() throws Exception
	{
		Streams written = run("red");

		assertEquals(ESCAPE + "[31m" + "E_DOCUMENT_FAILED: boom" + ESCAPE + "[0m" + NEWLINE, written.err());
		assertEquals("", written.out());
	}

	@Test
	public void anErrorCarriesNoEscapesWhenColourIsOff() throws Exception
	{
		// Asserted as an exact match rather than "contains no escape", which would also
		// pass for output that had gone missing altogether.
		assertEquals("E_DOCUMENT_FAILED: boom" + NEWLINE, run("plain").err());
	}

	@Test
	public void verboseChatterIsDimmed() throws Exception
	{
		assertEquals(ESCAPE + "[2m" + "opening a.pdf" + ESCAPE + "[0m" + NEWLINE, run("dim").out());
	}

	/**
	 * The deliberate half of the decision: {@code --color} must not reach the answer
	 * the user asked for, which is routinely piped into something that would choke on
	 * escapes. A change that colours reports uniformly would pass every test above and
	 * fail this one.
	 */
	@Test
	public void reportOutputIsNeverColoured() throws Exception
	{
		// writeLine, then write: the line has the terminator and the chunk does not.
		assertEquals("a.pdf: format=51" + NEWLINE + "a.pdf: format=51", run("report").out());
	}

	/**
	 * {@code --silent} still wins over {@code --color}: an error is the one thing that
	 * survives it, and it survives coloured.
	 */
	@Test
	public void silentSuppressesColouredReportOutputButNotColouredErrors() throws Exception
	{
		Streams written = run("silent");

		assertEquals("", written.out());
		assertEquals(ESCAPE + "[31m" + "E_DOCUMENT_FAILED: boom" + ESCAPE + "[0m" + NEWLINE, written.err());
	}

	/**
	 * Not in the reference, where a redirected stdout is UTF-8 on every platform. A
	 * {@code PrintStream} would use the platform charset, which on a default Windows
	 * install cannot spell an em dash; the console encodes as UTF-8 itself.
	 */
	@Test
	public void aRedirectedStreamIsWrittenAsUtf8() throws Exception
	{
		byte[] expected = ("em dash \u2014 CJK \u6587\u5b57" + NEWLINE).getBytes(StandardCharsets.UTF_8);

		assertEquals(Arrays.toString(expected), Arrays.toString(run("utf8").out));
	}

	/**
	 * Not in the reference. A line break <em>inside</em> the text is written as the
	 * caller wrote it, and only the terminator is the platform's: an option
	 * description is several lines of Markdown table, and translating its breaks would
	 * make {@code docfilters options} differ byte for byte between the ports.
	 */
	@Test
	public void aBreakInsideTheTextIsWrittenAsIs() throws Exception
	{
		assertEquals("| a |\n| b |" + NEWLINE, run("newline").out());
	}

	/**
	 * Not in the reference's tests, but its behaviour: an unpaired surrogate cannot be
	 * encoded, and is written as U+FFFD rather than as '?', which is what the JDK's
	 * own replacement for UTF-8 would have been.
	 */
	@Test
	public void anUnpairedSurrogateIsWrittenAsTheReplacementCharacter() throws Exception
	{
		byte[] expected = { 'a', (byte) 0xEF, (byte) 0xBF, (byte) 0xBD, 'b' };

		assertEquals(Arrays.toString(expected), Arrays.toString(run("surrogate").out));
	}

	/** The process each case runs in: one write, chosen by its single argument. */
	public static final class Child
	{
		private Child()
		{
		}

		public static void main(String[] args)
		{
			ConsoleOptions options = new ConsoleOptions();
			options.color = true;

			switch (args[0])
			{
				case "red":
					new SystemConsole(options, true, true).writeError("E_DOCUMENT_FAILED: boom");
					break;
				case "plain":
					new SystemConsole(options, false, false).writeError("E_DOCUMENT_FAILED: boom");
					break;
				case "dim":
					options.verbose = true;
					new SystemConsole(options, true, true).writeVerbose("opening a.pdf");
					break;
				case "report":
				{
					SystemConsole console = new SystemConsole(options, true, true);
					console.writeLine("a.pdf: format=51");
					console.write("a.pdf: format=51");
					break;
				}
				case "silent":
				{
					options.silent = true;
					options.verbose = true;
					SystemConsole console = new SystemConsole(options, true, true);
					console.writeLine("a.pdf: format=51");
					console.writeVerbose("opening a.pdf");
					console.writeError("E_DOCUMENT_FAILED: boom");
					break;
				}
				case "utf8":
					new SystemConsole(options, false, false).writeLine("em dash \u2014 CJK \u6587\u5b57");
					break;
				case "newline":
					new SystemConsole(options, false, false).writeLine("| a |\n| b |");
					break;
				case "surrogate":
					new SystemConsole(options, false, false).write("a\ud800b");
					break;
				default:
					throw new IllegalArgumentException(args[0]);
			}
		}
	}
}
