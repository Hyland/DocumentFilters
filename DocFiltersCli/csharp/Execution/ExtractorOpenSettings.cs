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
using Hyland.DocumentFilters.Cli.Planning;

using Hyland.DocumentFilters.Cli.Engine;

namespace Hyland.DocumentFilters.Cli.Execution
{
	/// <summary>
	/// How a document should be opened: the engine flags and the options string
	/// passed to <c>IDocument.Open</c>.
	/// </summary>
	/// <remarks>
	/// These have to be decided <em>before</em> the document is opened, because the
	/// engine applies them during <c>Open</c> — a flag worked out afterwards has no
	/// effect. That is why <see cref="DocumentFiltersSession.OpenDocument"/> takes
	/// this rather than each conversion path computing flags for a document that is
	/// already open.
	/// </remarks>
	public sealed class ExtractorOpenSettings
	{
		/// <summary>
		/// The complete <c>IGR_</c> open flags: the content selector, plus exactly one
		/// <c>Format*</c> bit — or none, where the engine's default is what is wanted.
		/// </summary>
		/// <remarks>
		/// Complete because it has to be: these reach the engine exactly as built here,
		/// and nothing downstream supplies a format bit. Pairing a partial set with an
		/// <c>OpenMode</c> does not work — that binding overload masks out bits 16 to 31
		/// and substitutes the format its mode implies — see
		/// <see cref="Engine.IDocumentFiltersEngine.OpenDocument"/>.
		///
		/// The <c>Format*</c> values are an enumeration packed into bits 16 to 19, not
		/// independent bits (<c>Text</c> 0x1, <c>HTML</c> 0x2, <c>Image</c> 0x5,
		/// <c>Markdown</c> 0x8, each shifted by 16), so at most one may ever be set.
		/// </remarks>
		public int Flags { get; set; }

		/// <summary>The engine options string, from <c>--options</c> and <c>--codepage</c>.</summary>
		public string Options { get; set; }

		/// <summary>The settings for converting to a given target format.</summary>
		/// <remarks>
		/// <c>--no-images</c> reaches a text-mode format here rather than through the
		/// canvas options, because <c>md-text</c> never opens a canvas. <c>md</c>, which
		/// does, is handled by <see cref="CanvasConversion.ComposeCanvasOptions"/>; both
		/// read the same rule from <see cref="CanvasConversion.ImageSuppression"/> so it
		/// is stated once.
		/// </remarks>
		public static ExtractorOpenSettings ForFormat(FormatSpec format, ConvertOptions options, SharedOptions shared)
		{
			string engineOptions = BuildOptionsString(shared);
			if (options.NoImages && format.Kind == FormatKind.Text)
			{
				string suppressed = CanvasConversion.ImageSuppression(format);
				if (suppressed.Length > 0)
					engineOptions = Append(engineOptions, suppressed);
			}

			return new ExtractorOpenSettings
			{
				Flags = (int)OpenType.BodyAndMeta | FormatBit(format, options),
				Options = engineOptions
			};
		}

		/// <summary>Joins two engine option strings, inserting a separator where needed.</summary>
		private static string Append(string existing, string addition)
		{
			if (existing.Length > 0 && !existing.EndsWith(";", StringComparison.Ordinal))
				existing += ";";
			return existing + addition + ";";
		}

		/// <summary>
		/// The one <c>IGR_FORMAT_*</c> bit a conversion opens with, or zero for none.
		/// </summary>
		/// <remarks>
		/// A canvas format needs <c>FormatImage</c>: that is what makes pages exist.
		/// A text format names the reader it wants, and <c>text</c> asks for none at all
		/// under <c>--include-control-codes</c> — omitting <c>IGR_FORMAT_TEXT</c> is what
		/// leaves the ISYS control codes in the stream instead of having the engine clean
		/// them out. <c>docpack</c> takes none either, matching doc2text, which opens it
		/// with a bare <c>IGR_BODY_AND_META</c>.
		///
		/// Structure reports need no case of their own. They require a paginated
		/// handle, and <see cref="Configuration.OptionsValidator.CheckReportsNeedCanvasFormat"/>
		/// refuses them against anything but a canvas format, which already opens
		/// paginated — so by the time this runs, a run with reports has a canvas
		/// format. Forcing the bit here instead was tried and does not work: it
		/// leaves a text conversion with no text reader.
		///
		/// Pure, and separate from <see cref="ForFormat"/>, so the whole table can be
		/// checked without an engine.
		/// </remarks>
		public static int FormatBit(FormatSpec format, ConvertOptions options)
		{
			if (format.Kind == FormatKind.Canvas)
				return (int)OpenType.FormatImage;

			if (format.Kind == FormatKind.DocPack)
				return 0;

			switch (format.Token)
			{
				case "text":
					return options.IncludeControlCodes ? 0 : (int)OpenType.FormatText;
				case "md-text":
					return (int)OpenType.FormatMarkdown;
				case "html":
					return (int)OpenType.FormatHTML;
				default:
					return (int)OpenType.FormatText;
			}
		}

		/// <summary>
		/// The settings for a command that reads a document without converting it —
		/// <c>identify</c>, <c>meta</c>, <c>list</c> and <c>extract</c>.
		/// </summary>
		/// <param name="enumerateImages">
		/// <c>extract --images</c>: ask the engine to surface a document's embedded
		/// images as well as its subfiles.
		/// </param>
		/// <remarks>
		/// <c>FormatText</c>: these commands enumerate subfiles and read metadata rather
		/// than rendering, so they want the text reader and no pages.
		///
		/// <c>ENUMERATE_IMAGES</c> has to be set here, at open, because that is the
		/// only place the engine reads it — <c>IDocument.Images</c> is empty without
		/// it, and so is the subfile list for a document whose only subfiles are
		/// images. Setting it afterwards, or not at all and iterating
		/// <c>container.Images</c> anyway, finds nothing. Appended after the user's
		/// <c>--options</c> so an explicit <c>ENUMERATE_IMAGES=OFF</c> still wins, the
		/// engine resolving a repeated key to its first occurrence.
		/// </remarks>
		public static ExtractorOpenSettings ForInspection(SharedOptions shared, bool enumerateImages = false)
		{
			string options = BuildOptionsString(shared);
			if (enumerateImages)
			{
				if (options.Length > 0 && !options.EndsWith(";", StringComparison.Ordinal))
					options += ";";
				options += "ENUMERATE_IMAGES=ON;";
			}

			return new ExtractorOpenSettings
			{
				Flags = (int)OpenType.BodyAndMeta | (int)OpenType.FormatText,
				Options = options
			};
		}

		/// <summary>
		/// The settings for <c>meta</c> specifically: <c>OpenType.MetaOnly</c> rather
		/// than the <c>BodyAndMeta</c> every other inspection command uses.
		/// </summary>
		/// <remarks>
		/// Confirmed against doc2text's own <c>--meta</c> flag (<c>main.cpp</c>:
		/// <c>flags = IGR_META_ONLY;</c> — an assignment that replaces the default
		/// entirely, not an addition to it). <see cref="MetadataReport"/> parses
		/// <c>GetText</c>'s output as one <c>Label: value</c> line per field; opening
		/// with body text included as well would mix document content into that same
		/// stream.
		/// </remarks>
		public static ExtractorOpenSettings ForMetadataOnly(SharedOptions shared)
		{
			return new ExtractorOpenSettings
			{
				Flags = (int)OpenType.MetaOnly | (int)OpenType.FormatText,
				Options = BuildOptionsString(shared)
			};
		}

		/// <summary>
		/// Builds the semicolon-separated engine options string from <c>--options</c> and
		/// <c>--codepage</c>, in the form doc2text itself uses: <c>...;CODEPAGE=1252;</c>.
		/// </summary>
		private static string BuildOptionsString(SharedOptions shared)
		{
			string options = shared.Processing.EngineOptions ?? string.Empty;
			if (!string.IsNullOrEmpty(shared.Processing.Codepage))
			{
				if (options.Length > 0 && !options.EndsWith(";", StringComparison.Ordinal))
					options += ";";
				options += "CODEPAGE=" + shared.Processing.Codepage + ";";
			}
			return options;
		}
	}
}
