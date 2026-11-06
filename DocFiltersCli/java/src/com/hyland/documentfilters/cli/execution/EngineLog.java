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

package com.hyland.documentfilters.cli.execution;

import com.hyland.documentfilters.cli.configuration.DiagnosticOptions;
import com.hyland.documentfilters.cli.runtime.IConsole;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Set;
import java.util.TreeSet;

/**
 * {@code --log-level} and {@code --log-modules}: what the engine's own readers log,
 * and where it goes.
 *
 * <p>The engine asks, per module, how much it should log, then hands back each
 * message. Both arrive on the open callback, so this only reaches documents being
 * opened.
 *
 * <p>The filtering happens here rather than being asked of the engine, exactly as
 * doc2text does it: the answer to "what level for module X" is where the
 * {@code --log-modules} set is applied.
 *
 * <p>Output goes to <b>stderr</b>, which doc2text also does. Nothing is logged
 * unless {@code --log-level} was passed, so a run that says nothing about logging
 * produces the same stderr it always did.
 */
public final class EngineLog
{
	public static final int LEVEL_NONE = 0;
	public static final int LEVEL_CRITICAL = 1;
	public static final int LEVEL_ERROR = 2;
	public static final int LEVEL_WARNING = 3;
	public static final int LEVEL_INFORMATION = 4;
	public static final int LEVEL_DEBUG = 5;
	public static final int LEVEL_TRACE = 6;

	private static final DateTimeFormatter TIMESTAMP = DateTimeFormatter.ofPattern("uuuu-MM-dd'T'HH:mm:ss.SSS");

	private final int _level;
	private final Set<String> _modules;
	private final IConsole _console;

	private EngineLog(int level, Set<String> modules, IConsole console)
	{
		_level = level;
		_modules = modules;
		_console = console;
	}

	/**
	 * The log for these options, or null when nothing was asked for.
	 *
	 * <p>Null rather than a disabled instance, so a run with no logging hands the
	 * engine no callback at all.
	 */
	public static EngineLog forOptions(DiagnosticOptions options, IConsole console)
	{
		String requested = options.logLevel != null ? options.logLevel : options.debugLogLevel;
		int level = parseLevel(requested);
		if (level == LEVEL_NONE)
			return null;

		String modules = options.logModules != null ? options.logModules : options.debugLogModules;
		Set<String> named = new TreeSet<>(String.CASE_INSENSITIVE_ORDER);
		if (modules != null && !modules.isEmpty() && !modules.equals("*"))
		{
			for (String module : modules.split("[,;]"))
			{
				if (!module.isEmpty())
					named.add(module.trim());
			}
		}

		return new EngineLog(level, named, console);
	}

	/**
	 * Answers the engine's "how much should {@code module} log".
	 *
	 * <p>{@link #LEVEL_NONE} for a module the user did not name, which is how
	 * {@code --log-modules} is enforced.
	 */
	public int levelFor(String module)
	{
		return _modules.isEmpty() || _modules.contains(module != null ? module : "") ? _level : LEVEL_NONE;
	}

	/** Writes one message the engine produced. */
	public void write(int level, String module, String message)
	{
		_console.writeError(LocalDateTime.now().format(TIMESTAMP) + " " + name(level) + " [" + module + "] " + message);
	}

	/** The fixed-width level name, so the module and message columns line up. */
	public static String name(int level)
	{
		switch (level)
		{
			case LEVEL_CRITICAL:
			case LEVEL_ERROR:
				return "ERROR";
			case LEVEL_WARNING:
				return "WARN ";
			case LEVEL_INFORMATION:
				return "INFO ";
			case LEVEL_DEBUG:
				return "DEBUG";
			case LEVEL_TRACE:
				return "TRACE";
			default:
				return "     ";
		}
	}

	/**
	 * {@code --log-level}'s vocabulary, mapped onto the engine's scale. Anything
	 * unrecognised is {@link #LEVEL_NONE} rather than an error, matching doc2text.
	 */
	public static int parseLevel(String value)
	{
		if (value == null)
			return LEVEL_NONE;

		switch (value)
		{
			case "critical":
				return LEVEL_CRITICAL;
			case "error":
				return LEVEL_ERROR;
			case "warning":
				return LEVEL_WARNING;
			case "information":
				return LEVEL_INFORMATION;
			case "debug":
				return LEVEL_DEBUG;
			case "trace":
				return LEVEL_TRACE;
			default:
				return LEVEL_NONE;
		}
	}
}
