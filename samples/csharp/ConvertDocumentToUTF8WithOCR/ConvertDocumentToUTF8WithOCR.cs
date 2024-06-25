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
using System;
using System.Text;
using System.Text.RegularExpressions;

namespace DocFilters
{
    class Program
    {
        const int MaxCharsPerGetText = 4096;
        private readonly DocumentFilters m_docfilters = new();

        [Argument(0)]
        public string file { get; set; } = "";

        [Option("-o|--output", "the file to save the output, defaults to .\\{filename}.txt", CommandOptionType.SingleValue)]
        public string destination { get; set; } = "";

        string stripControlChars(string input) => 
            Regex.Replace(input.Replace('\x0e', '\n'), "[\x01-\x08\x0b-\x10\v]", "");

        private void ProcessFile(string filename)
        {
            if (string.IsNullOrEmpty(destination))
                destination = Path.ChangeExtension(filename, ".txt");

            Console.Error.WriteLine("Processing " + filename + " to " + destination);
            try
            {
                using StreamWriter outputFile = new StreamWriter(File.Open(destination, FileMode.Create), Encoding.UTF8);
                using Extractor doc = m_docfilters.OpenExtractor(filename, OpenMode.Text, OpenType.BodyOnly, "OCR=ON;OCR_REORIENT_PAGES=ON");

                if (doc.getSupportsText())
                    while (!doc.getEOF())
                        outputFile.WriteLine(stripControlChars(doc.GetText(MaxCharsPerGetText)));
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
            if (!string.IsNullOrEmpty(file))
                ProcessFile(file);
        }
        public static int Main(string[] args)
                => CommandLineApplication.Execute<Program>(args);
    }
}
