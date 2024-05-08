/*
   (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.

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
using Hyland.DocumentFilters;
using McMaster.Extensions.CommandLineUtils;
using System.Text.Json;

class Program
{
    [Option("--ignore-case", Description = "Case insensitive comparison")]
    public bool IgnoreCase { get; set; }

    [Option("--ignore-fields", Description = "Exclude fields from comparison")]
    public bool IgnoreFields { get; set; }

    [Option("--ignore-footers", Description = "Exclude marked footers from comparison")]
    public bool IgnoreFooters { get; set; }

    [Option("--ignore-headers", Description = "Exclude marked headers from comparison")]
    public bool IgnoreHeaders { get; set; }

    [Option("--ignore-punctuation", Description = "Exclude punctuation from comparison")]
    public bool IgnorePunctuation { get; set; }

    [Option("--ignore-tables", Description = "Exclude tables from comparison")]
    public bool IgnoreTables { get; set; }

    [Option("--ignore-textboxes", Description = "Exclude textboxes from comparison")]
    public bool IgnoreTextBoxes { get; set; }

    [Option("--include-formatting", Description = "Include formatting in comparison")]
    public bool IncludeFormatting { get; set; }

    [Option("--left-options", Description = "Option string that applies to the first document")]
    public string? LeftOptions { get; set; }

    [Option("--left-output", Description = "Filename where marked up left output should be saved")]
    public string? LeftOutput { get; set; }

    [Option("--left-top-margin", Description = "The size of the top margin that should be excluded")]
    public string? LeftTopMargin { get; set; }

    [Option("--left-bottom-margin", Description = "The size of the bottom margin that should be excluded")]
    public string? LeftBottomMargin { get; set; }

    [Option("--options", Description = "Option string that applies to both documents")]
    public string? Options { get; set; }

    [Option("--pages", Description = "Pages to compare in the form of FIRST-LAST")]
    public string? Pages { get; set; }

    [Option("--right-options", Description = "Option string that applies to the second document")]
    public string? RightOptions { get; set; }

    [Option("--right-output", Description = "Filename where marked up right output should be saved")]
    public string? RightOutput { get; set; }

    [Option("--right-top-margin", Description = "The size of the top margin that should be excluded")]
    public string? RightTopMargin { get; set; }

    [Option("--right-bottom-margin", Description = "The size of the bottom margin that should be excluded")]
    public string? RightBottomMargin { get; set; }

    [Option("--silent", Description = "Disable writing to console")]
    public bool Silent { get; set; }

    [Argument(0, "Left file")]
    public string? LeftFile { get; set; }

    [Argument(1, "Right file")]
    public string? RightFile { get; set; }

    private readonly Hyland.DocumentFilters.Api _api = new();

    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");
        if (string.IsNullOrWhiteSpace(LeftFile))
        {
            Console.Error.WriteLine("Left file is required");
            return 1;
        }

        if (string.IsNullOrWhiteSpace(RightFile))
        {
            Console.Error.WriteLine("Right file is required");
            return 1;
        }

        var compareSettings = new CompareSettings
        {
            CompareType = CompareType.Word,
            Flags = 0
        };
        if (IgnoreCase)
            compareSettings.Flags |= CompareFlags.NoCase;
        if (IgnoreFields)
            compareSettings.Flags |= CompareFlags.NoFields;
        if (IgnoreFooters)
            compareSettings.Flags |= CompareFlags.NoFooters;
        if (IgnoreHeaders)
            compareSettings.Flags |= CompareFlags.NoHeaders;
        if (IgnorePunctuation)
            compareSettings.Flags |= CompareFlags.NoPunctuation;
        if (IgnoreTables)
            compareSettings.Flags |= CompareFlags.NoTables;
        if (IgnoreTextBoxes)
            compareSettings.Flags |= CompareFlags.NoTextBoxes;
        if (IncludeFormatting)
            compareSettings.Flags |= CompareFlags.Formatting;

        var pages = ParsePages(Pages);
        var leftPageSettings = new CompareDocumentSettings
        {
            FirstPage = pages.first,
            PageCount = pages.last - pages.first + 1,
            Margins = System.Drawing.RectangleF.FromLTRB(0, UnpackMeasurement(LeftTopMargin), 0, UnpackMeasurement(RightBottomMargin))
        };
        var rightPageSettings = new CompareDocumentSettings
        {
            FirstPage = pages.first,
            PageCount = pages.last - pages.first + 1,
            Margins = System.Drawing.RectangleF.FromLTRB(0, UnpackMeasurement(RightTopMargin), 0, UnpackMeasurement(RightBottomMargin))
        };
        var writer = Silent ? null : new Utf8JsonWriter(Console.OpenStandardOutput(), new JsonWriterOptions { Indented = true });

        using var left = _api.OpenExtractor(LeftFile, OpenMode.Paginated, OpenType.BodyAndMeta, (Options ?? "") + ";" + (LeftOptions ?? ""));
        using var right = _api.OpenExtractor(RightFile, OpenMode.Paginated, OpenType.BodyAndMeta, (Options ?? "") + ";" + (RightOptions ?? ""));
        using var compareResult = left.Compare(leftPageSettings, right, rightPageSettings, compareSettings);

        using Canvas? leftOutputCanvas = !string.IsNullOrWhiteSpace(LeftOutput)
            ? _api.MakeOutputCanvas(LeftOutput, CanvasType.PDF)
            : null;
        using Canvas? rightOutputCanvas = !string.IsNullOrWhiteSpace(RightOutput)
            ? _api.MakeOutputCanvas(RightOutput, CanvasType.PDF)
            : null;

        writer?.WriteStartObject();
        writer?.WriteString("original", LeftFile);
        writer?.WriteString("revised", RightFile);
        writer?.WriteStartArray("diffs");

        int currentLeft = -1, currentRight = -1;
        while (compareResult.MoveNext())
        {
            if (compareResult.Current.Type == DifferenceType.Equal || compareResult.Current.Type == DifferenceType.NextBatch)
                continue;

            writer?.WriteStartObject();
            writer?.WriteString("type", $"{compareResult.Current.Type}");
            writer?.WriteNumber("originalPage", compareResult.Current.OriginalPageIndex+1);
            writer?.WriteNumber("revisedPage", compareResult.Current.RevisedPageIndex + 1);
            writer?.WriteStartArray("parts");

            foreach (var hit in compareResult.Current.Details)
            {
                if (compareResult.Current.Type == DifferenceType.Delete && leftOutputCanvas != null)
                {
                    ensurePage(hit.PageIndex, left, leftOutputCanvas, ref currentLeft, true);
                    leftOutputCanvas.Rect(FromRectF(hit.Bounds));
                }

                if (compareResult.Current.Type == DifferenceType.Insert && rightOutputCanvas != null)
                {
                    ensurePage(hit.PageIndex, left, rightOutputCanvas, ref currentRight, false);
                    rightOutputCanvas.Rect(FromRectF(hit.Bounds));
                }

                writer?.WriteStartObject();
                writer?.WriteString("text", hit.Text);
                writer?.WriteNumber("page", hit.PageIndex+1);
                writer?.WriteStartObject("bounds");
                writer?.WriteNumber("l", hit.Bounds.Left);
                writer?.WriteNumber("t", hit.Bounds.Top);
                writer?.WriteNumber("r", hit.Bounds.Right);
                writer?.WriteNumber("b", hit.Bounds.Bottom);
                writer?.WriteEndObject();
                writer?.WriteEndObject();
            }

            writer?.WriteEndArray();
            writer?.WriteEndObject();
        }

        writer?.WriteEndArray();
        writer?.WriteEndObject();
        writer?.Flush();

        return 0;
    }

    private (int first, int last) ParsePages(string? pages)
    {
        if (string.IsNullOrWhiteSpace(pages))
            return (0, 0xffff);

        var parts = pages.Split('-');
        if (parts.Length == 1 && int.TryParse(parts[0], out var page))
            return (page, page);

        if (parts.Length != 2)
            throw new ArgumentException("Pages must be in the form of FIRST-LAST");

        if (!int.TryParse(parts[0], out var first))
            throw new ArgumentException("First page must be an integer");

        if (!int.TryParse(parts[1], out var last))
            last = 0xffff;

        return (first, last);
    }

    private float UnpackMeasurement(string? measurement)
    {
        if (string.IsNullOrWhiteSpace(measurement))
            return 0;

        if (measurement.EndsWith("in"))
            return float.Parse(measurement.Substring(0, measurement.Length - 2)) * 72;

        if (measurement.EndsWith("cm"))
            return float.Parse(measurement.Substring(0, measurement.Length - 2)) * 28.35f;

        if (measurement.EndsWith("mm"))
            return float.Parse(measurement.Substring(0, measurement.Length - 2)) * 2.835f;

        if (measurement.EndsWith("pt"))
            return float.Parse(measurement.Substring(0, measurement.Length - 2));

        if (measurement.EndsWith("px"))
            return float.Parse(measurement.Substring(0, measurement!.Length - 2)) * 96 / 72;

        return float.Parse(measurement);
    }

    private void ensurePage(int pageIndex, Extractor doc, Canvas canvas, ref int currentPage, bool isLeft)
    {
        while (currentPage < pageIndex)
        {
            using var page = doc.GetPage(++currentPage);
            canvas.RenderPage(page);
        }
        canvas.SetBrush(isLeft ? 0x50ff0000 : 0x5000ff00, 1);
        canvas.SetPen(0, 0, 0);
    }

    private System.Drawing.Rectangle FromRectF(System.Drawing.RectangleF rect)
    {
        return System.Drawing.Rectangle.FromLTRB((int)rect.Left, (int)rect.Top, (int)Math.Ceiling(rect.Right), (int)Math.Ceiling(rect.Bottom));
    }

    static int Main(string[] args) => CommandLineApplication.Execute<Program>(args);
}

