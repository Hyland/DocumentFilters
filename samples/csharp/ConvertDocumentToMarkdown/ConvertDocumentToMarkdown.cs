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

namespace DocFilters
{

    class Program
    {
        private readonly DocumentFilters m_docfilters = new();

        [Option("-o|--output", "the folder to save the output files, defaults to current directory", CommandOptionType.SingleValue)]
        public string OutputFolder { get; set; } = ".";

        [Option("--include-headers", "include headers in the output", CommandOptionType.NoValue)]
        public bool IncludeHeaders { get; set; } = false;

        [Option("--include-footers", "include footers in the output", CommandOptionType.NoValue)]
        public bool IncludeFooters { get; set; } = false;

        [Option("--include-images", "include images in the output", CommandOptionType.NoValue)]
        public bool IncludeImages { get; set; } = false;

        [Option("--metadata-format", "metadata format to use in the output", CommandOptionType.SingleValue)]
        public MetadataFormat Metadata { get; set; } = MetadataFormat.None;

        [Option("--heading-style", "heading style to use in the output", CommandOptionType.SingleValue)]
        public HeadingStyle Heading { get; set; } = HeadingStyle.ATX;

        [Option("--simple-table-style", "table style to use in the output", CommandOptionType.SingleValue)]
        public TableStyle SimpleTable { get; set; } = TableStyle.PIPE;

        [Option("--complex-table-style", "table style to use in the output", CommandOptionType.SingleValue)]
        public TableStyle ComplexTable { get; set; } = TableStyle.HTML;


        [Argument(0)]
        public List<string> Files { get; set; } = new();

        public enum MetadataFormat
        {
            None, JSON, YAML, TOML, Comments
        }

        public enum HeadingStyle
        {
            ATX, SETEXT
        }

        public enum TableStyle
        {
            PIPE, GRID, HTML, PIPE_WITH_HTML
        }   

        private void ProcessFile(string filename)
        {
            string destination = Path.Combine(OutputFolder, Path.GetFileNameWithoutExtension(filename) + ".md");

            Console.Error.WriteLine("Processing " + filename);
            try
            {
                string canvasOptions = 
                    $"MARKDOWN_HEADERS_STYLE={Heading};" +
                    $"MARKDOWN_SIMPLE_TABLE_STYLE={SimpleTable};" +
                    $"MARKDOWN_COMPLEX_TABLE_STYLE={ComplexTable};" +
                    $"MARKDOWN_INCLUDE_HEADERS={IncludeHeaders};" +
                    $"MARKDOWN_INCLUDE_FOOTERS={IncludeFooters};" +
                    $"MARKDOWN_INCLUDE_IMAGES={IncludeImages};";
                if (Metadata == MetadataFormat.None)
                    canvasOptions += $"MARKDOWN_METADATA_FORMAT={Metadata};MARKDOWN_INCLUDE_METADATA=ON;";

                using Extractor doc = m_docfilters.OpenExtractor(filename, OpenMode.Paginated);
                using Canvas canvas = m_docfilters.MakeOutputCanvas(destination, CanvasType.MARKDOWN, canvasOptions);

                canvas.RenderPages(doc);
            }
            catch (Exception e)
            {
                Console.Error.WriteLine("Error Processing " + filename);
                Console.Error.WriteLine("   - " + e.ToString());
            }
        }
        public void OnExecute()
        {
            m_docfilters.Initialize(DocumentFiltersLicense.Get(), ".");

            foreach (string file in Files)
                ProcessFile(file);
        }
        public static int Main(string[] args)
                => CommandLineApplication.Execute<Program>(args);
    }
}
