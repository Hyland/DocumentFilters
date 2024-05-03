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

        [Argument(0)]
        public List<string> Files { get; set; } = new();

        private void ProcessFile(string filename)
        {
            string destination = Path.Combine(OutputFolder, Path.GetFileNameWithoutExtension(filename) + ".tif");

            byte[] bytes = File.ReadAllBytes(filename);

            Console.Error.WriteLine("Processing " + filename);
            try
            {
                using Extractor doc = m_docfilters.OpenExtractor(bytes, OpenMode.Paginated);
                using Canvas canvas = m_docfilters.MakeOutputCanvas(destination, isys_docfilters.IGR_DEVICE_IMAGE_TIF, "");

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
