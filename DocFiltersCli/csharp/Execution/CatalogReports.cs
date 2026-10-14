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

using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// The two catalogue commands: <c>formats</c> and <c>options</c>. Both ask the
	/// engine what this build supports and print it; neither opens a document.
	/// </summary>
	public sealed class CatalogReports
	{
		private readonly DocumentFiltersSession _session;
		private readonly IConsole _console;

		public CatalogReports(DocumentFiltersSession session, IConsole console)
		{
			_session = session;
			_console = console;
		}

		/// <summary>
		/// <c>formats</c> — every readable file type, from
		/// <c>DocumentFilters.GetSupportedFormats</c>.
		/// </summary>
		/// <remarks>
		/// There is no capability column here, unlike <c>identify</c>: capability flags
		/// (text/subfiles/hd) exist only on an opened <see cref="IDocument"/>
		/// (<c>getFileCapabilities</c>), and neither <see cref="Format"/> nor doc2text's
		/// own catalogue dump (<c>print_file_types</c> in <c>main.cpp</c>) exposes them
		/// for a format nothing has been opened for.
		/// </remarks>
		public void WriteFormats(FormatsOptions options)
		{
			IEnumerable<EngineFormat> formats = _session.Api.GetSupportedFormats()
				.Where(format => MatchesFilter(options.Filter, format.DisplayName, format.ShortName));

			if (options.Json)
			{
				var json = new JsonWriter().BeginArray();
				foreach (EngineFormat format in formats)
				{
					json.BeginObject()
						.Property("id", format.Id)
						.Property("ext", format.ShortName)
						.Property("name", format.DisplayName)
						.EndObject();
				}
				_console.Write(json.EndArray().ToString());
			}
			else
			{
				foreach (EngineFormat format in formats)
					_console.WriteLine($"id={format.Id} ext={format.ShortName} name={format.DisplayName}");
			}
		}

		/// <summary>
		/// <c>options</c> — every engine tunable, from
		/// <c>DocumentFilters.GetAvailableOptions</c>. These are the keys accepted by
		/// <c>--options</c>.
		/// </summary>
		/// <remarks>
		/// <c>applies-to</c> comes from <see cref="Option.Flags"/>: bit <c>0x1</c> means
		/// document, <c>0x2</c> means canvas, confirmed against doc2text's own
		/// <c>--options --json</c> dump (<c>main.cpp</c>, the <c>appliesTo</c> array
		/// built from <c>f &amp; 0x1</c> / <c>f &amp; 0x2</c>).
		/// </remarks>
		public void WriteOptions(OptionCatalogOptions options)
		{
			var matches = new List<(EngineOption Option, string AppliesTo)>();
			foreach (EngineOption option in _session.Api.GetAvailableOptions())
			{
				if (!MatchesFilter(options.Filter, option.DisplayName, option.Description))
					continue;

				var appliesTo = AppliesTo(option.Flags);
				if (options.AppliesTo != null && !appliesTo.Contains(options.AppliesTo, StringComparer.OrdinalIgnoreCase))
					continue;

				matches.Add((option, appliesTo.Any() ? string.Join(",", appliesTo) : "none"));
			}

			if (options.Json)
			{
				var json = new JsonWriter().BeginArray();
				foreach (var match in matches)
				{
					json.BeginObject()
						.Property("name", match.Option.DisplayName)
						.Property("type", match.Option.Type)
						.Property("default", match.Option.DefaultValue)
						.Property("appliesTo", match.AppliesTo)
						.Property("description", match.Option.Description)
						.EndObject();
				}
				_console.Write(json.EndArray().ToString());
			}
			else
			{
				foreach (var match in matches)
				{
					_console.WriteLine(
						$"name={match.Option.DisplayName} type={match.Option.Type} default={match.Option.DefaultValue} " +
						$"applies-to={match.AppliesTo} description={match.Option.Description}");
				}
			}
		}

		private static List<string> AppliesTo(int flags)
		{
			var scopes = new List<string>();
			if ((flags & 0x1) != 0)
				scopes.Add("document");
			if ((flags & 0x2) != 0)
				scopes.Add("canvas");
			return scopes;
		}

		private static bool MatchesFilter(string filter, params string[] fields)
		{
			if (string.IsNullOrEmpty(filter))
				return true;

			return fields.Any(field => field != null && field.IndexOf(filter, StringComparison.OrdinalIgnoreCase) >= 0);
		}
	}
}
