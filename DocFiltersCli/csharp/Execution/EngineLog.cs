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

using Hyland.DocumentFilters.Cli.Configuration;
using Hyland.DocumentFilters.Cli.Runtime;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// <c>--log-level</c> and <c>--log-modules</c>: what the engine's own readers log,
	/// and where it goes.
	/// </summary>
	/// <remarks>
	/// The engine asks, per module, how much it should log, then hands back each
	/// message. Both arrive as actions on the open callback, so this only reaches
	/// documents being opened — there is no logging of a page loop, for the same
	/// reason there is no cancelling one.
	///
	/// The filtering happens here rather than being asked of the engine, exactly as
	/// doc2text does it: the engine has no notion of which modules the user named,
	/// so the answer to "what level for module X" is where the <c>--log-modules</c>
	/// set is applied.
	///
	/// Output goes to <b>stderr</b>, which doc2text also does. It shares that stream
	/// with the two contract error shapes, which is safe because nothing is logged
	/// unless <c>--log-level</c> was passed: a run that says nothing about logging
	/// produces the same stderr it always did.
	/// </remarks>
	public sealed class EngineLog
	{
		private readonly int _level;
		private readonly HashSet<string> _modules;
		private readonly IConsole _console;

		private EngineLog(int level, HashSet<string> modules, IConsole console)
		{
			_level = level;
			_modules = modules;
			_console = console;
		}

		/// <summary>
		/// The log for these options, or null when nothing was asked for.
		/// </summary>
		/// <remarks>
		/// Null rather than a disabled instance, so a run with no logging hands the
		/// engine no callback at all and the engine can skip composing messages
		/// nobody will print.
		/// </remarks>
		public static EngineLog For(DiagnosticOptions options, IConsole console)
		{
			string requested = options.LogLevel ?? options.DebugLogLevel;
			int level = ParseLevel(requested);
			if (level == LevelNone)
				return null;

			string modules = options.LogModules ?? options.DebugLogModules;
			var named = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
			if (!string.IsNullOrEmpty(modules) && modules != "*")
			{
				foreach (string module in modules.Split(new[] { ',', ';' }, StringSplitOptions.RemoveEmptyEntries))
					named.Add(module.Trim());
			}

			return new EngineLog(level, named, console);
		}

		public const int LevelNone = 0;
		public const int LevelCritical = 1;
		public const int LevelError = 2;
		public const int LevelWarning = 3;
		public const int LevelInformation = 4;
		public const int LevelDebug = 5;
		public const int LevelTrace = 6;

		/// <summary>
		/// Answers the engine's "how much should <paramref name="module"/> log".
		/// </summary>
		/// <remarks>
		/// <see cref="LevelNone"/> for a module the user did not name, which is how
		/// <c>--log-modules</c> is enforced — the engine then composes nothing for it
		/// rather than composing messages this side throws away.
		/// </remarks>
		public int LevelFor(string module)
		{
			return _modules.Count == 0 || _modules.Contains(module ?? string.Empty) ? _level : LevelNone;
		}

		/// <summary>Writes one message the engine produced.</summary>
		public void Write(int level, string module, string message)
		{
			_console.WriteError($"{Timestamp()} {Name(level)} [{module}] {message}");
		}

		/// <summary>
		/// The fixed-width level name, so the module and message columns line up.
		/// </summary>
		/// <remarks>
		/// Same five names and the same padding doc2text uses, minus the colour: this
		/// tool writes no escape codes of its own anywhere else.
		/// </remarks>
		public static string Name(int level)
		{
			switch (level)
			{
				case LevelCritical:
				case LevelError: return "ERROR";
				case LevelWarning: return "WARN ";
				case LevelInformation: return "INFO ";
				case LevelDebug: return "DEBUG";
				case LevelTrace: return "TRACE";
				default: return "     ";
			}
		}

		/// <summary>
		/// <c>--log-level</c>'s vocabulary, folded and mapped onto the engine's scale.
		/// </summary>
		/// <remarks>
		/// Anything unrecognised is <see cref="LevelNone"/> rather than an error,
		/// matching doc2text, whose own parse falls through to none.
		/// </remarks>
		public static int ParseLevel(string value)
		{
			switch (value)
			{
				case "critical": return LevelCritical;
				case "error": return LevelError;
				case "warning": return LevelWarning;
				case "information": return LevelInformation;
				case "debug": return LevelDebug;
				case "trace": return LevelTrace;
				default: return LevelNone;
			}
		}

		private static string Timestamp()
		{
			return DateTime.Now.ToString("yyyy-MM-dd'T'HH:mm:ss.fff");
		}
	}
}
