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

/****************************************************************************
 * Document Filters Example - a command line tool.
 *
 * Convert, extract, list, identify and inspect documents from the shell.
 *
 *   docfilters convert --to pdf --out-dir out/ ./corpus
 *   docfilters extract -i --out-dir dump/ bundle.pst
 *   docfilters identify --json *.bin
 *
 * The same command line is implemented in C#, Python, Java and C++17. See
 * ../README.md for how the pieces fit together and why they are arranged this
 * way; if you are here to learn the API, read the execution package and ignore
 * the rest.
 ****************************************************************************/

package com.hyland.documentfilters.cli;

import com.hyland.documentfilters.cli.cli.CommandLineApp;
import com.hyland.documentfilters.cli.configuration.ConsoleOptions;
import com.hyland.documentfilters.cli.engine.IDocumentFiltersEngine;
import com.hyland.documentfilters.cli.runtime.CliError;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.Interruption;
import com.hyland.documentfilters.cli.runtime.SystemConsole;
import com.hyland.documentfilters.cli.runtime.SystemFileSystem;
import java.time.Duration;
import java.util.Arrays;
import java.util.List;
import java.util.function.BiConsumer;
import java.util.function.Supplier;

/** The entry point: argv in, exit code out. */
public final class Program
{
	private Program()
	{
	}

	public static void main(String[] args)
	{
		// Ctrl+C is recorded rather than allowed to kill the process, so the run
		// reports and exits 130 through the same path as every other exit code.
		int exitCode = run(args, new SystemConsole(preScanConsoleOptions(args)), new SystemFileSystem(), null,
			Interruption.install(), null, null);
		System.exit(exitCode);
	}

	public static int run(String[] args, IConsole console, IFileSystem fileSystem, IDocumentFiltersEngine engine)
	{
		return run(args, console, fileSystem, engine, null, null, null);
	}

	/**
	 * Runs one command line. Split out so a test can drive a whole command line ---
	 * argv in, exit code and console text out --- against a fake engine.
	 */
	public static int run(String[] args, IConsole console, IFileSystem fileSystem, IDocumentFiltersEngine engine,
		Interruption interruption, Supplier<Duration> clock, BiConsumer<String, IConsole> openViewer)
	{
		CommandLineApp app = new CommandLineApp(console, fileSystem, engine, interruption, clock, openViewer);

		try
		{
			return app.run(args);
		}
		catch (CliError error)
		{
			return reportError(error, console);
		}
	}

	/**
	 * The console switches, read before the command line is bound so that an error
	 * in binding is itself reported under them.
	 */
	public static ConsoleOptions preScanConsoleOptions(String[] args)
	{
		List<String> tokens = Arrays.asList(args);
		ConsoleOptions options = new ConsoleOptions();
		options.silent = tokens.contains("--silent");
		options.verbose = tokens.contains("--verbose");
		options.color = tokens.contains("--color");
		return options;
	}

	public static int reportError(CliError error, IConsole console)
	{
		console.writeError(error.getPath() == null
			? error.getCode() + ": " + error.getMessage()
			: error.getPath() + ": " + error.getCode() + ": " + error.getMessage());
		return error.getExitCode();
	}
}
