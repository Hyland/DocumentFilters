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

class Program
{
    [Argument(0, Description = "File to process")]
    public string? FileName { get; set; }

    [Option("-o|--output", Description = "Output filename")]
    public string? OutputFilename { get; set; } = "output.pdf";

    [Option("-l|--list", Description = "List forms")]
    public bool List { get; set; }

    [Option("-s|--set", Description = "Set a form value [Name=Value]")]
    public List<string> FormValues { get; set; } = new();

    private readonly Hyland.DocumentFilters.Api _api = new();

    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");

        if (string.IsNullOrEmpty(FileName))
            throw new ArgumentNullException(nameof(FileName));
        if (string.IsNullOrEmpty(OutputFilename))
            throw new ArgumentNullException(nameof(OutputFilename));

        using Extractor file = _api.OpenExtractor(FileName, OpenMode.Paginated);

        if (List)
        {
            ListFormElements(file);
        }
        else
        {

            using Canvas canvas = _api.MakeOutputCanvas(OutputFilename, ExtToCanvasType(Path.GetExtension(OutputFilename)), "PDF_PRESERVE_ORIGINAL=on");

            RenderPageProperties pageProps = new();
            foreach ((string Name, string Value) formValue in FormValues.Select(x => Split(x)))
                pageProps.AddFormValue(formValue.Name, formValue.Value, false);

            foreach ((Page page, int pageIndex) in file.Pages.Select((page, pageIndex) => (page, pageIndex)))
            {
                using (page)
                {
                    Console.Out.WriteLine($"Page {pageIndex + 1}");

                    canvas.RenderPage(page, "", pageProps);
                }
            }
        }

        return 0;
    }

    private void ListFormElements(Extractor extractor)
    {
        foreach ((Page page, int pageIndex) in extractor.Pages.Select((page, pageIndex) => (page, pageIndex)))
        {
            Console.Out.WriteLine($"Page {pageIndex + 1}");
            foreach ((FormElement formElement, int formElementIndex) in page.FormElements.Select((formElement, formElementIndex) => (formElement, formElementIndex)))
            {
                Console.Out.WriteLine($"  {formElementIndex + 1}. {formElement.Name}");
            }
        }
    }
    private (string Name, string Value) Split(string line)
    {
        int i = line.IndexOf('=');
        if (i >= 0)
            return (line.Substring(0, i), line.Substring(i + 1));
        else
            return (line, "");
    }

    private CanvasType ExtToCanvasType(string ext)
    {
        CanvasType res;
        if (!Enum.TryParse<CanvasType>(ext.Trim('.'), true, out res))
            throw new ArgumentException($"Unknown format {ext}");
        return res;
    }

    static int Main(string[] args) => CommandLineApplication.Execute<Program>(args);
}
