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

package com.hyland.documentfilters.cli.tests.cli;

import com.hyland.documentfilters.cli.Program;
import com.hyland.documentfilters.cli.engine.EngineError;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.Interruption;
import com.hyland.documentfilters.cli.tests.fakes.FakeDocumentFiltersEngine;
import com.hyland.documentfilters.cli.tests.fakes.InMemoryFileSystem;
import com.hyland.documentfilters.cli.tests.fakes.RecordingConsole;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;

/**
 * Runs a whole command line the way the shell does, in process.
 *
 * <p>{@code argv} in; exit code, stdout and stderr out. Everything between is the
 * real thing --- the real parser, the real validator, the real walk, the real
 * planner, the real commands, the real error-to-exit-code mapping --- with only the
 * three edges swapped: the console, the filesystem and the engine.
 *
 * <p>This covers the one seam the conformance corpus cannot reach. The corpus stops
 * at {@code bind} and {@code describePlan}; everything above that ---
 * {@code Program.run}, {@code CommandLineApp.run}'s dispatch, {@code --print-contract},
 * the dry-run branch, the session's lifetime, and {@code reportError}'s stderr format
 * --- is only exercised here.
 *
 * <p>No license and no native library: the engine is a fake, so these run wherever
 * the unit tests run. What they deliberately do not prove is that the real engine
 * behaves as the fake claims.
 */
public final class CliHarness
{
	/** The fake disk, so a test can assert what the run left on it. */
	public final InMemoryFileSystem fileSystem = new InMemoryFileSystem();

	/** The engine the run sees. Script documents and failures on it before running. */
	public final FakeDocumentFiltersEngine engine = new FakeDocumentFiltersEngine();

	/** Everything the run wrote, kept after {@link #run} returns. */
	public final RecordingConsole console = new RecordingConsole();

	/**
	 * A clock the test moves by hand, so a run can outlast {@code --timeout} without
	 * the suite waiting.
	 */
	public final TestClock clock = new TestClock();

	/**
	 * What {@code --view} was handed, and what was in it at that moment.
	 *
	 * <p>The content is read when the viewer is called rather than afterwards,
	 * because the question {@code --view} raises is one of timing: a combined output
	 * is not a finished file until its canvas or its writer is closed, and reading it
	 * after the run would look the same either way.
	 */
	public final List<String> viewed = new ArrayList<>();

	/** The content of each {@link #viewed} file when the viewer saw it, or null. */
	public final List<String> viewedContent = new ArrayList<>();

	private final Interruption _interruption;

	private int _exitCode;

	/** The default: nothing is watching for an interrupt. */
	public CliHarness()
	{
		this(null);
	}

	/** Builds a harness whose runs watch {@code interruption}. */
	public CliHarness(Interruption interruption)
	{
		_interruption = interruption;
	}

	/** The exit code of the last {@link #run}. */
	public int exitCode()
	{
		return _exitCode;
	}

	/** Every line written to standard output. */
	public List<String> out()
	{
		return console.outLines;
	}

	/**
	 * Everything written to stdout, verbatim.
	 *
	 * <p>{@link #out} only records whole lines, so it misses output written without
	 * one --- the JSON reports build a document and write it in a single call, which
	 * is exactly the output whose exact shape is worth asserting.
	 */
	public String output()
	{
		return console.output();
	}

	/** Every line written to standard error. */
	public List<String> errors()
	{
		return console.errorLines;
	}

	/** The files the run created, for asserting on what it wrote. */
	public List<String> created()
	{
		return fileSystem.created;
	}

	/** Adds an empty input that exists on disk and opens in the engine. */
	public CliHarness withDocument(String path)
	{
		return withDocument(path, "");
	}

	/**
	 * Adds an input that exists on disk and opens in the engine.
	 *
	 * <p>Both halves, always. A path in only one of the two is the setup mistake that
	 * otherwise shows up as a walk selecting nothing, or an open failing for a file
	 * the walk did find --- neither of which is what the test meant to say.
	 */
	public CliHarness withDocument(String path, String text)
	{
		fileSystem.addFile(path);
		engine.addDocument(path, text);
		return this;
	}

	/** Adds a file on disk that the engine will refuse to open. */
	public CliHarness withUnreadableDocument(String path, EngineError error)
	{
		fileSystem.addFile(path);
		engine.failToOpen(path, error);
		return this;
	}

	/** Adds an empty file the engine opens only with the right password. */
	public CliHarness withProtectedDocument(String path, String password)
	{
		return withProtectedDocument(path, password, "");
	}

	/** Adds a file the engine opens only with the right password. */
	public CliHarness withProtectedDocument(String path, String password, String text)
	{
		fileSystem.addFile(path);
		engine.addProtectedDocument(path, password, text);
		return this;
	}

	/**
	 * Adds a file on disk that the engine has never heard of.
	 *
	 * <p>The walk finds it and the open fails --- which is what a file of a format
	 * this build cannot read looks like from the outside.
	 */
	public CliHarness withUnopenableFile(String path)
	{
		fileSystem.addFile(path);
		return this;
	}

	/** Puts a file where an output would go, for the overwrite rules. */
	public CliHarness withExistingOutput(String path)
	{
		fileSystem.addFile(path, "already here");
		return this;
	}

	/** Runs the command line and returns the exit code. */
	public int run(String... args)
	{
		_exitCode = Program.run(args, console, fileSystem, engine, _interruption, clock::read, this::recordView);
		return _exitCode;
	}

	private void recordView(String path, IConsole viewerConsole)
	{
		viewed.add(path);
		viewedContent.add(fileSystem.fileExists(path) ? fileSystem.readText(path) : null);
	}

	/**
	 * A monotonic reading a test advances itself.
	 *
	 * <p>Starts at zero and only moves when a test says so, so a run takes no real
	 * time however long it claims to have taken. Every harness run uses one, which
	 * also means timings printed by {@code --time} are reproducible rather than
	 * whatever the machine happened to do.
	 */
	public static final class TestClock
	{
		private Duration _now = Duration.ZERO;

		/** The current reading. */
		public Duration read()
		{
			return _now;
		}

		/** Moves the clock forward. */
		public void advance(Duration by)
		{
			_now = _now.plus(by);
		}
	}
}
