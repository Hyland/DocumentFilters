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

using Hyland.DocumentFilters.Cli.Runtime;
using System.Text;

namespace Hyland.DocumentFilters.Cli.Planning
{
	/// <summary>
	/// An <c>--out-name</c> filename template, such as
	/// <c>{stem}-{page:04}.{ext}</c>.
	/// </summary>
	/// <remarks>
	/// Placeholders:
	/// <list type="table">
	/// <item><term>{stem}</term><description>input filename without its extension</description></item>
	/// <item><term>{page}</term><description>one-based page number; <c>{page:04}</c> zero-pads to four digits</description></item>
	/// <item><term>{srcext}</term><description>the input's original extension, without a dot</description></item>
	/// <item><term>{ext}</term><description>the output format's extension, without a dot</description></item>
	/// </list>
	/// The template is compiled when the argument is parsed, not on first use, so a typo
	/// is reported before any work starts.
	/// </remarks>
	public sealed class OutputNameTemplate
	{
		private struct Segment
		{
			public enum Type
			{
				Literal,
				Stem,
				Page,
				SrcExt,
				Ext
			}
			public Type SegmentType { get; }
			public string LiteralText { get; }
			public int Width { get; set; }
			public Segment(Type type, string text = null)
			{
				SegmentType = type;
				LiteralText = text;
            }
        }

		private string originalText;
        private readonly List<Segment> segments = new List<Segment>();

		private static void Throw(string input)
        {
            throw new UsageError(ErrorCodes.BadTemplate, $"--out-name does not accept the value '{input}'");
        }

        /// <summary>The template used when <c>--out-name</c> is not given: <c>{stem}.{ext}</c> for single-file formats, 
        /// <c>{stem}-{page:04}.{ext}</c> for one-per-page formats.</summary>
        public static OutputNameTemplate Default(bool isSingleFile)
		{
			if (isSingleFile)
				return Parse("{stem}.{ext}");
			else
				return Parse("{stem}-{page:04}.{ext}");
        }

		/// <summary>Compiles a template.</summary>
		/// <remarks>
		/// The template names a file <em>inside</em> <c>--out-dir</c>, so path
		/// separators and <c>..</c> are rejected here rather than sanitised at render
		/// time. Checking only the rendered output would miss them, because they can
		/// come from the template's own literal text as easily as from the input path.
		/// </remarks>
		/// <exception cref="Runtime.UsageError">
		/// An unknown placeholder, an unclosed brace, a malformed width such as
		/// <c>{page:xx}</c>, or a path separator or <c>..</c> anywhere in the template.
		/// </exception>
		public static OutputNameTemplate Parse(string text)
		{
            OutputNameTemplate result = new OutputNameTemplate { originalText = text };

            var chars = new[] { '{', '}', '/', '\\' };
            int startIndex = 0;
			string substr;
            while (startIndex < text.Length)
            {
				int endIndex = text.IndexOfAny(chars, startIndex); // only opening brace supported at this point
				if (endIndex == -1)
				{
					result.segments.Add(new Segment(Segment.Type.Literal, text.Substring(startIndex)));
					break;
				}

				if (text[endIndex] != '{')
					Throw(text); // invalid character

				if (endIndex > startIndex)
                {
                    substr = text.Substring(startIndex, endIndex - startIndex);
                    result.segments.Add(new Segment(Segment.Type.Literal, substr));
                }

                startIndex = endIndex;
                endIndex = text.IndexOf('}', startIndex + 1);
                if (endIndex == -1)
                    Throw(text); // no closing brace
                // The name between the braces, e.g. "stem" or "page:04".
                string placeholder = text.Substring(startIndex + 1, endIndex - startIndex - 1);
                switch (placeholder)
                {
                    case "stem":
                        result.segments.Add(new Segment(Segment.Type.Stem));
                        break;
                    case "page":
                        result.segments.Add(new Segment(Segment.Type.Page));
                        break;
                    case "srcext":
                        result.segments.Add(new Segment(Segment.Type.SrcExt));
                        break;
                    case "ext":
                        result.segments.Add(new Segment(Segment.Type.Ext));
                        break;
                    default:
                        if (placeholder.StartsWith("page:", StringComparison.Ordinal))
                        {
                            string widthText = placeholder.Substring("page:".Length);
                            if (!int.TryParse(widthText, out int width) || width <= 0)
                                Throw(text); // malformed width

                            result.segments.Add(new Segment(Segment.Type.Page) { Width = width });
                        }
                        else
                        {
                            Throw(text); // unknown placeholder
                        }
                        break;
                }
                startIndex = endIndex + 1;
            }
			return result;
		}

		/// <summary>True when the template refers to <c>{page}</c>, which a single-file format cannot satisfy.</summary>
		public bool UsesPage => segments.Any(s => s.SegmentType == Segment.Type.Page);

		/// <summary>Produces a filename. <paramref name="pageNumber"/> is ignored when the template has no <c>{page}</c>.</summary>
		public string Render(string sourcePath, string outputExtension, int pageNumber)
		{
			var sb = new StringBuilder();
            foreach (var segment in segments)
			{
				switch (segment.SegmentType)
				{
					case Segment.Type.Literal:
						sb.Append(segment.LiteralText);
						break;
					case Segment.Type.SrcExt:
						sb.Append(Path.GetExtension(NormalizeSeparators(sourcePath)).TrimStart('.'));
						break;
					case Segment.Type.Ext:
						sb.Append(outputExtension);
						break;
					case Segment.Type.Page:
						var pageStr = pageNumber.ToString();
						if (segment.Width > 0)
                            pageStr = pageStr.PadLeft(segment.Width, '0');
						sb.Append(pageStr);
                        break;
					case Segment.Type.Stem:
						sb.Append(Path.GetFileNameWithoutExtension(NormalizeSeparators(sourcePath)));
						break;
                }
			}
			return sb.ToString();
		}

		/// <summary>
		/// <paramref name="path"/> with every backslash turned into a forward slash.
		/// </summary>
		/// <remarks>
		/// <see cref="Path.GetFileNameWithoutExtension(string)"/> and <see cref="Path.GetExtension(string)"/>
		/// only recognise the separators the running platform does — backslash is not
		/// one of them on Linux — but a source path is not guaranteed to match the
		/// platform running this tool: a walk finds real paths in the platform's own
		/// convention, but an explicit <c>--files-from</c> list or a subfile name from
		/// a container built elsewhere can carry either. Without this, <c>{stem}</c>
		/// on <c>..\..\windows\system32\config</c> returned the whole string
		/// unchanged on Linux, backslashes included, which is exactly the path
		/// separator this template exists to keep out of the rendered name. Forward
		/// slash alone is unambiguous on every platform this tool targets, so
		/// normalising onto it is safe in both directions.
		/// </remarks>
		private static string NormalizeSeparators(string path) => path.Replace('\\', '/');

		/// <summary>The original template text.</summary>
		public override string ToString()
		{
			return originalText;
        }
	}
}
