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
using System.Text;

namespace ConvertDocument
{

    class Program
    {
        private readonly DocumentFilters m_docfilters = new();

        [Option("--html", "process file as html with images saved to current directory", CommandOptionType.NoValue)]
        public bool Html { get; set; }

        [Option("-s|--subfiles", "process subfiles of file", CommandOptionType.NoValue)]
        public bool SubFiles { get; set; }

        [Option("-o|--output", "output filename", CommandOptionType.SingleValue)]
        public string OutputFile { get; set; }

        [Argument(0)]
        public List<string> Files { get; set; } = new();

        private void ProcessFile(string filename, TextWriter output)
        {
            using (Extractor doc = m_docfilters.GetExtractor(filename))
                ProcessFile(filename, doc, output);
        }

        private void ProcessFile(string filename, Extractor doc, TextWriter output)
        {
            Console.Error.WriteLine("Processing " + filename);
            try
            {
                int flags = isys_docfilters.IGR_BODY_AND_META;
                if (Html)
                    flags |= isys_docfilters.IGR_FORMAT_HTML;

                doc.Open(flags);

                // Extract the text and return it to stdout
                while (!doc.getEOF())
                {
                    string t = doc.GetText(4096);
                    // Cleanup the text
                    t = t.Replace('\u000E', '\n');
                    t = t.Replace('\r', '\n');
                    output.Write(t);
                }
                output.WriteLine("");

                // Extract the HTML generated images into the current folder
                if (Html)
                {
                    foreach (SubFile image in doc.Images)
                    {
                        Console.Error.WriteLine("Extracting image " + image.getName());
                        image.CopyTo(image.getName());
                        image.Close();
                    }
                }

                // Extract the sub files (if any) and process recursively
                if (SubFiles)
                {
                    foreach (SubFile subfile in doc.SubFiles)
                    {
                        using (subfile)
                            ProcessFile(filename + ">" + subfile.getName(), subfile, output);
                    }
                }
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

            TextWriter output = Console.Out;

            if (!string.IsNullOrWhiteSpace(OutputFile))
                output = new StreamWriter(File.Open(OutputFile, FileMode.Create), Encoding.UTF8);

            foreach (string file in Files)
                ProcessFile(file, output);

            if (output != Console.Out)
                output.Close();
        }

        public static int Main(string[] args)
                => CommandLineApplication.Execute<Program>(args);
    }

}
