/*
   (c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved.

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

namespace DocFilters
{

	class Program
	{
		private DocumentFilters m_filters;
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

				if (String.Compare(arg, "-h", true) == 0 || String.Compare(arg, "--help", true) == 0)
				{
					ShowHelp();
					return;
				}
				else
				{
					fileList.Add(arg);
				}
			}

			foreach (string filename in fileList)
			{
				BinaryWriter outFile = new BinaryWriter(Console.OpenStandardOutput());
				ProcessFile(filename, outFile, m_filters.GetExtractor(filename));
			}

			m_stdout.Close();
		}

		private void ProcessFile(string filename, BinaryWriter outFile, Extractor item)
		{
			m_stderr.WriteLine("Processing " + filename);
			try
			{
				item.Open(isys_docfilters.IGR_BODY_AND_META | isys_docfilters.IGR_FORMAT_IMAGE);
				CustomStream outStream = new CustomStream(new MemoryStream());
				Hyland.DocumentFilters.Canvas canvas = m_filters.MakeOutputCanvas(outStream, isys_docfilters.IGR_DEVICE_IMAGE_TIF, "");
				try
				{
					for (int pageIndex = 0; pageIndex < item.GetPageCount(); pageIndex++)
					{
						Hyland.DocumentFilters.Page page = item.GetPage(pageIndex);
						try
						{
							canvas.RenderPage(page);
						}
						finally
						{
							page.Close();
						}
					}
				}
				finally
				{
					canvas.Close();
					// Write in-memory stream contents to existing open file (e.g. stdout)
					long BytesWritten = outStream.writeTo(outFile);
					m_stderr.WriteLine(BytesWritten + " bytes written to outFile");
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
			System.Console.WriteLine("Document Filters 11: ConvertDocumentToTIFFStream C# Example");
			System.Console.WriteLine("(c) 2023 Hyland Software, Inc.");
			System.Console.WriteLine("");
			System.Console.WriteLine("ConvertDocumentToTIFFStream filename [> OutFilename]");
			System.Console.WriteLine("");
			System.Console.WriteLine("options");
			System.Console.WriteLine(" -h, --help                this help");
		}

		static void Main(string[] args)
		{
			Program prog = new Program();
			prog.Run(args);
		}
	}

}
