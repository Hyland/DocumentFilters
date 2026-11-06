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

package com.hyland.documentfilters.cli.commands;

import com.hyland.documentfilters.cli.configuration.DiagnosticOptions;
import com.hyland.documentfilters.cli.execution.Diagnostics;
import com.hyland.documentfilters.cli.execution.DocumentFiltersSession;
import com.hyland.documentfilters.cli.execution.StopSignal;
import com.hyland.documentfilters.cli.runtime.IConsole;
import com.hyland.documentfilters.cli.runtime.IFileSystem;
import com.hyland.documentfilters.cli.runtime.Interruption;
import com.hyland.documentfilters.cli.execution.DebugInput;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.channels.SeekableByteChannel;
import java.util.function.IntSupplier;

/**
 * The one piece every {@code execute} needs and none of them owns:
 * {@link DocumentFiltersSession}'s lifetime.
 *
 * <p>A CLI process runs exactly one command, so creating the session at the start
 * of that one {@code execute} call and closing it at the end already matches its
 * documented lifetime --- "once for the lifetime of the process" --- without
 * {@code CommandLineApp} or {@code Program} needing to know about it. A context
 * hand-wired with a session already --- as tests are free to do --- is left alone
 * and not closed here, since this did not create it.
 */
public final class CommandExecution
{
	private CommandExecution()
	{
	}

	/**
	 * A fresh {@link StopSignal} for one document.
	 *
	 * <p>Per document, not per run: the signal latches the first reason it sees, and
	 * a deadline belongs to the document it was measured against.
	 */
	public static StopSignal watchForStop(Diagnostics diagnostics, Interruption interruption)
	{
		return new StopSignal(diagnostics, interruption);
	}

	public static int withSession(CommandContext context, IntSupplier action)
	{
		boolean ownsSession = context.session == null;
		if (ownsSession)
			context.session = DocumentFiltersSession.create(context.sharedOptions.library,
				context.sharedOptions.processing.tempDir);

		try
		{
			return action.getAsInt();
		}
		finally
		{
			if (ownsSession)
			{
				context.session.close();
				context.session = null;
			}
		}
	}

	/**
	 * {@code DebugInput.forInput}, for a lambda that cannot throw a checked
	 * exception.
	 *
	 * <p>Not turned into a {@code DocumentError}: the reference calls it outside the
	 * per-document handler, so a file that cannot be read here ends the run rather
	 * than counting as one failed document.
	 */
	static SeekableByteChannel debugInput(String path, DiagnosticOptions options, IFileSystem fileSystem, IConsole console,
		String displayName)
	{
		try
		{
			return DebugInput.forInput(path, options, fileSystem, console, displayName);
		}
		catch (IOException e)
		{
			throw new UncheckedIOException(e.getMessage(), e);
		}
	}

	/**
	 * The message for a failure that becomes a {@code DocumentError}.
	 *
	 * <p>Java leaves a message null where the reference's never is, so the class
	 * name stands in rather than the report reading "null".
	 */
	static String messageOf(Throwable error)
	{
		String message = error.getMessage();
		return message != null ? message : error.getClass().getSimpleName();
	}
}
