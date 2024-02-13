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
using System.Data;
using System.Reflection;
using System.Text.Json;

class Program
{

    [Argument(0, Description = "file to process")]
    public List<string> Files { get; set; } = new();

    [Option("-o|--output", Description = "Output filename to create")]
    public string? OutputFilename { get; set; } = "output.pdf";

    [Option("-w|--word", Description = "One or more words to redact")]
    public List<string> Words { get; set; } = new();

    [Option("--regex", Description = "One or more regex expressions to redact")]
    public List<string> RegExes { get; set; } = new();

    [Option("-r|--rule", Description = "One or more defined rules to redact")]
    public List<string> Rules { get; set; } = new();

    [Option("--rect", Description = "One or more rectangles to redact, in the for of \"LTRB\"")]
    public List<string> Rects { get; set; } = new();

    [Option("-m|--match", Description = "Matching algorithm to use")]
    public Rule.MatchType WordMatch { get; set; } = Rule.MatchType.Contains;

    [Option("--color", Description = "Redaction color")]
    public string Color { get; set; } = "#000000";

    private readonly Hyland.DocumentFilters.Api _api = new();
    private Dictionary<string, Rule> _ruleDefinitions = new();

    /// <summary>
    /// OnExecute is called by the command line parser.
    /// </summary>
    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");

        string ruleDefinitionsFiles = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) ?? "", "Rules.json");
        if (File.Exists(ruleDefinitionsFiles))
        {
            using FileStream strm = File.OpenRead(ruleDefinitionsFiles);
            _ruleDefinitions = JsonSerializer.Deserialize<Dictionary<string, Rule>>(strm) ?? new Dictionary<string, Rule>();
        }

        IList<IRule> rules = PrepareRules();

        using Canvas canvas = _api.MakeOutputCanvas(OutputFilename, ExtToCanvasType(Path.GetExtension(OutputFilename)), "PDF_PRESERVE_ORIGINAL=on");

        foreach (string file in Files)
            ProcessFile(canvas, file, rules);

        return 0;
    }

    /// <summary>
    /// Converts the command line arguments into a list of IRule interfaces.
    /// </summary>
    private IList<IRule> PrepareRules()
    {
        List<IRule> rules = new();

        rules.AddRange(Words.Select(x => Rule.Make(x, this.WordMatch)).Where(x => x != null));
        rules.AddRange(RegExes.Select(x => new RegExRule(x)));

        foreach (string rule in Rules)
        {
            if (_ruleDefinitions.TryGetValue(rule, out Rule? value))
            {
                if (value.RegEx != null)
                    rules.Add(new RegExRule(value.RegEx));
            }
            else
            {
                foreach (KeyValuePair<string, Rule> ruleByCat in _ruleDefinitions.Where(x => x.Value.Category == rule))
                {
                    if (ruleByCat.Value.RegEx != null)
                        rules.Add(new RegExRule(ruleByCat.Value.RegEx));
                }
            }
        }
        return rules;
    }

    /// <summary>
    /// Processes a single file, applying redactions and rendering to the canvas.
    /// </summary>
    private void ProcessFile(Canvas canvas, string filename, IEnumerable<IRule> rules)
    {
        Console.Error.WriteLine($"File: {filename}");
        try
        {
            using Extractor doc = _api.OpenExtractor(filename, OpenMode.Paginated, options: "OCR=ON");

            System.Drawing.Color color = System.Drawing.ColorTranslator.FromHtml(Color);

            foreach (Page? page in doc.Pages)
            {
                using (page)
                {
                    RenderPageProperties properties = new();

                    foreach (System.Drawing.Rectangle rect in FindRedactionRectangles(rules, page))
                        properties.AddRedaction(rect, color);

                    canvas.RenderPage(page, "", properties);
                }
            }
        }
        catch (Exception e)
        {
            Console.Error.WriteLine($"Error: {e.Message}");
        }
    }

    /// <summary>
    /// Calculate and return the rectangles based on the rules for the given page.
    /// </summary>
    private IEnumerable<System.Drawing.Rectangle> FindRedactionRectangles(IEnumerable<IRule> rules, Page page)
    {
        foreach (string rect in Rects)
        {
            int[] coords = rect.Split(new char[] { ' ', ',' }, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
                .Select(x => int.TryParse(x, out int v) ? v : 0)
                .ToArray();

            if (coords.Length == 4)
                yield return System.Drawing.Rectangle.FromLTRB(coords[0], coords[1], coords[2], coords[3]);
        }

        if (rules.Count() > 0)
        {
            foreach ((System.Drawing.Rectangle Rect, Word Word) match in Redactor.Redact(page, rules))
            {
                yield return match.Rect;
            }
        }
    }

    /// <summary>
    /// Utility function that maps a file extension to a Canvas type.
    /// </summary>
    private CanvasType ExtToCanvasType(string? ext)
    {
        CanvasType res;
        if (!Enum.TryParse<CanvasType>(ext?.Trim('.'), true, out res))
            throw new ArgumentException($"Unknown format {ext}");
        return res;
    }

    static int Main(string[] args) => CommandLineApplication.Execute<Program>(args);
}

