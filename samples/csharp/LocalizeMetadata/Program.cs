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

        [Option("-l|--lang", "The language to use for metadata", CommandOptionType.SingleValue)]
        public LocalizedStrings.Language Language { get; set; } = LocalizedStrings.Language.English;

        [Option("-o|--output", "the folder to save the output files, defaults to current directory", CommandOptionType.SingleValue)]
        public string OutputFolder { get; set; } = ".";

        [Argument(0)]
        public List<string> Files { get; set; } = new();

        private void ProcessFile(string filename)
        {
            IDictionary<LocalizedStrings.StringID, string>? strings = LocalizedStrings.Get(Language);
            Func<OpenCallback, int> openCallback = (OpenCallback req) =>
            {
                switch (req.Action)
                {
                    case ISYS11dfConstants.IGR_OPEN_CALLBACK_ACTION_LOCALIZE:
                        if (strings != null && strings.TryGetValue((LocalizedStrings.StringID)req.Localize.string_id, out string? res))
                        {
                            req.Localize.replacement = res;
                            return ISYS11dfConstants.IGR_OK;
                        }
                        else
                            return ISYS11dfConstants.IGR_E_NOT_FOUND;
                    default:
                        return ISYS11dfConstants.IGR_OK;
                }
            };

            string destination = Path.Combine(OutputFolder, Path.GetFileNameWithoutExtension(filename) + ".pdf");

            Console.Error.WriteLine("Processing " + filename);
            try
            {
                using Extractor doc = m_docfilters.OpenExtractor(filename, OpenMode.Paginated, OpenType.BodyAndMeta, "", openCallback);
                using Canvas canvas = m_docfilters.MakeOutputCanvas(destination, isys_docfilters.IGR_DEVICE_IMAGE_PDF, "");

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
