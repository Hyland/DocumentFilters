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
    [Argument(0, Description = "file to process")]
    public string? FileName { get; set; }

    [Option("--json", Description = "Output as JSON")]
    public bool Json { get; set; }

    private readonly Hyland.DocumentFilters.Api _api = new();

    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");

        if (string.IsNullOrEmpty(FileName))
            throw new ArgumentNullException(nameof(FileName));

        using Extractor file = _api.GetExtractor(FileName);

        ProcessFile(file, FileName);

        return 0;
    }

    private void ProcessFile(Extractor extractor, string fileName, int depth = 0)
    {
        Console.Error.WriteLine($"File: {fileName}");
        try
        {
            extractor.Open(OpenMode.Paginated, OpenType.BodyOnly);

            if (Json)
            {
                var payload = extractor.Pages.Select((page, pageIndex) =>
                {
                    using (page)
                    {
                        return new
                        {
                            Page = pageIndex,
                            Items = page.FormElements
                        };
                    }
                });
                Console.Out.WriteLine(System.Text.Json.JsonSerializer.Serialize(payload, new System.Text.Json.JsonSerializerOptions { WriteIndented = true }));
            }
            else
            {
                foreach ((Page page, int pageIndex) in extractor.Pages.Select((page, idx) => (page, idx)))
                {
                    using (page)
                    {
                        Console.Out.WriteLine($"Page {pageIndex + 1}");
                        foreach ((FormElement formElement, int formElementIndex) in page.FormElements.Select((e, i) => (e, i)))
                        {
                            Console.Out.WriteLine($" {formElementIndex + 1,4}. {formElement.Name}");
                            Console.Out.WriteLine($"       type: {formElement.Type}, x: {formElement.X}, y: {formElement.Y}, width: {formElement.Width}, height: {formElement.Height}");
                            Console.Out.WriteLine($"       value: {formElement.Value}");

                        }
                    }
                }
            }
        }
        catch (Exception e)
        {
            Console.Error.WriteLine($"Error: {e.Message}");
        }
    }

    static int Main(string[] args) => CommandLineApplication.Execute<Program>(args);
}

