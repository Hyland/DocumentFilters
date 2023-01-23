/*
   (c) 2022 Hyland Software, Inc. and its affiliates. All rights reserved.

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

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Hyland.DocumentFilters;

namespace ConvertDocument
{

	class Program
	{
		private DocumentFilters m_filters;
		private bool m_html;
		private bool m_subFiles;
		private string m_outputFile;
		private TextWriter m_stdout;
		private TextWriter m_stderr;

		public Program()
		{
			m_stdout = System.Console.Out;
			m_stderr = System.Console.Error;
		}

		public void Run(string[] args)
		{
			if (args.Length == 0)
			{
				ShowHelp();
				return;
			}

			m_filters = new DocumentFilters();
			m_filters.Initialize(DocumentFiltersLicense.LICENSE_KEY, ".");

			List< string > fileList = new List< string >();

			for (int i = 0; i < args.Length; i++)
			{
				String arg = args[i];

				if (String.Compare(arg, "--html", true) == 0)
				{
					m_html = true;
				}
				else if (String.Compare(arg, "--text", true) == 0 || String.Compare(arg, "-t", true) == 0)
				{
					m_html = false;
				}
				else if (String.Compare(arg, "--subfiles", true) == 0 || String.Compare(arg, "-s", true) == 0)
				{
					m_subFiles = true;
				}
				else if (String.Compare(arg, "--output", true) == 0 || String.Compare(arg, "-o", true) == 0)
				{
					m_outputFile = args[++i];
				}
				else if (String.Compare(arg, "-h", true) == 0 || String.Compare(arg, "--help", true) == 0)
				{
					ShowHelp();
					return;
				}
				else
				{
					fileList.Add(arg);
				}
			}

			if (m_outputFile != null)
			{
				m_stdout = new StreamWriter(File.Open(m_outputFile, FileMode.Create), Encoding.UTF8);
			}

			foreach (string filename in fileList)
			{
				ProcessFile(filename, m_filters.GetExtractor(filename));
			}

			m_stdout.Close();
		}

		private void ProcessFile(string filename, Extractor item)
		{
			m_stderr.WriteLine("Processing " + filename);
			try
			{
				int flags = isys_docfilters.IGR_BODY_AND_META;
				if (m_html)
				{
					flags |= isys_docfilters.IGR_FORMAT_HTML;
				}

				item.Open(flags);

				// Extract the text and return it to stdout
				while (!item.getEOF())
				{
					String t = item.GetText(4096);
					// Cleanup the text
					t = t.Replace('\u000E', '\n');
					t = t.Replace('\r', '\n');
					m_stdout.Write(t);
				}
				m_stdout.WriteLine("");

				// Extract the HTML generated images into the current folder
				if (m_html)
				{
					SubFile image = item.GetFirstImage();
					while (image != null)
					{
						m_stderr.WriteLine("Extracting image " + image.getName());
						image.CopyTo(image.getName());
						image.Close();
						// Move onto the next image
						image = item.GetNextImage();
					}
				}

				// Extract the sub files (if any) and process recursively
				if (m_subFiles)
				{
					SubFile child = item.GetFirstSubFile();
					while (child != null)
					{
						ProcessFile(filename + ">" + child.getName(), child);
						// Move onto the next sub file
						child = item.GetNextSubFile();
					}
				}
			}
			catch (Exception e)
			{
				m_stderr.WriteLine("Error Processing " + filename);
				m_stderr.WriteLine("   - " + e.ToString());
			}
			finally
			{
				item.Close();
			}
		}

		private void ShowHelp()
		{
			System.Console.WriteLine("Document Filters 11: ConvertDocument C# Example");
			System.Console.WriteLine("(c) 2022 Hyland Software, Inc.");
			System.Console.WriteLine("");
			System.Console.WriteLine("ConvertDocument [options] filename");
			System.Console.WriteLine("");
			System.Console.WriteLine("options");
			System.Console.WriteLine(" -h, --help                this help");
			System.Console.WriteLine(" -t, --text                output as plain text");
			System.Console.WriteLine("     --html                output as HTML text, images stored in current directory");
			System.Console.WriteLine(" -o, --output [filename]   write the output to the specified file");
			System.Console.WriteLine(" -s, --subfiles            process subfiles");
		}

		static void Main(string[] args)
		{
			Program prog = new Program();
			prog.Run(args);
		}
	}

}
