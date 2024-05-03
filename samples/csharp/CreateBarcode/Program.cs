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
    public enum BarCodeType { Qr, Dm, Pdf417, Code39, Code93, Code128, GS1 };

    [Argument(0, Description = "Barcode Text")]
    public string? Text { get; set; }

    [Option("-t|--type", Description = "Caption to include on barcode")]
    public BarCodeType Type { get; set; } = BarCodeType.Qr;

    [Option("-c|--caption", Description = "Caption to include on barcode")]
    public string? Caption { get; set; }

    [Option("-w|--width", Description = "Width of barcode")]
    public int Width { get; set; } = 350;

    [Option("-h|--height", Description = "Height of barcode")]
    public int Height { get; set; } = 350;

    [Option("-o|--output", Description = "Output filename")]
    public string OutputFileName { get; set; } = "output.png";

    [Option("-m|--margin", Description = "Margin around barcode")]
    public int Margin { get; set; } = 8;

    private readonly Hyland.DocumentFilters.Api _api = new();
    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");

        using Canvas canvas = _api.MakeOutputCanvas(OutputFileName, ExtToCanvasType(Path.GetExtension(OutputFileName)));
        canvas.BlankPage(Width, Height);

        canvas.Annotate(new Hyland.DocumentFilters.Annotations.Barcode
        {
            SubType = Type.ToString().ToLower(),
            Caption = Caption,
            Text = Text,
            Rect = System.Drawing.Rectangle.FromLTRB(Margin, Margin, Width - Margin, Height - Margin)
        });

        return 0;
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

