/*
   (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.

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

/****************************************************************************
* Perceptive Document Filters Example - Convert a document to Interactive HD HTML
*
* This example creates a html view of a document which can be manipulated by
* javascript to provide more functionality than a standard static page.
*
* There are two parts to the sample, the page generate logic use standard
* Document Filters HDHTML exports to create the page data. The sample then
* injects CSS and Javascript to provide extra viewing capabilities, such as
* Zoom, Rotate, Highlight, Redact and Metadata Preview.
*
****************************************************************************/

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
		private string m_outputFolder;
		private TextWriter m_stdout;
		private TextWriter m_stderr;
		private bool m_async;
		private bool m_inlineImages;
		private string m_options;

		public Program()
		{
			m_stdout = System.Console.Out;
			m_stderr = System.Console.Error;
			m_outputFolder = ".";
			m_async = m_inlineImages = false;
		}

		public void Run(string[] args)
		{
			if (args.Length == 0)
			{
				ShowHelp();
				return;
			}

			m_filters = new DocumentFilters();
			m_filters.Initialize("{ENTER LICENSE KEY}", ".");

			List< string > fileList = new List< string >();
			for (int i = 0; i < args.Length; i++)
			{
				String arg = args[i];

				if (String.Compare(arg, "--output", true) == 0 || String.Compare(arg, "-o", true) == 0)
				{
					m_outputFolder = args[++i];
				}
				else if (String.Compare(arg, "--async", true) == 0 || String.Compare(arg, "-a", true) == 0)
				{
					m_async = true;
				}
				else if (String.Compare(arg, "--inline-images", true) == 0)
				{
					m_inlineImages = true;
				}
				else if (String.Compare(arg, "--options", true) == 0)
				{
					m_options = args[++i];
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

			foreach (string filename in fileList)
			{
				ProcessFile(filename, m_filters.GetExtractor(filename));
			}

			m_stdout.Close();
		}

		private void ProcessFile(string filename, Extractor item)
		{
			string destination = System.IO.Path.Combine(m_outputFolder, System.IO.Path.GetFileNameWithoutExtension(filename) + ".html");

			m_stderr.WriteLine("Processing " + filename + " to " + destination);
			try
			{
				String OptionsMain;
				String OptionsPage;

				OptionsMain = OptionsPage = m_options + ";" +
					"HDHTML_INCLUDE_WORD_INDEXES=on;";
				// HDHTML_INCLUDE_WORD_INDEXES will include information about each word in the run data for a page. This is
				// used by the javascript to map between the web browsers co-ordinate system and the word co-ordinate system
				// used by Document Filters. In this example, it's used by the redaction function.

				if (m_inlineImages)
				{
					OptionsMain += "HTML_INLINE_IMAGES=on;";
					OptionsPage += "HTML_INLINE_IMAGES=on;";
					// HTML_INLINE_IMAGES forces any images to be inlined directly into the HTML using url(data:image/png;base64,
					// rather than having external files.
				}


				OptionsMain += "HDHTML_OUTPUT_INJECT_HEAD=@perceptive-viewer-inject.html;";
				// HDHTML_OUTPUT_INJECT_HEAD will load the contents of the perceptive-viewer-inject.html file and place it
				// at the bottom of the <HEAD> section of the main HTML page. This allows us to inject stylesheets, javascript
				// and extra metadata that will be loaded when the page is viewed.

				OptionsPage += "HDHTML_OUTPUT_BOILERPLATE=off";
				// HDHTML_OUTPUT_BOILERPLATE disables the surrounding <html>...<body> tags that typically get generated into
				// the HTML output.  This is used when generateing ASYNC pages, which must only contain the actual page data
				// and not the surrounding html.

				item.Open(isys_docfilters.IGR_BODY_AND_META | isys_docfilters.IGR_FORMAT_IMAGE, OptionsMain);

				System.IO.FileStream OutputStream = new System.IO.FileStream(destination, FileMode.Create, FileAccess.ReadWrite);

				Hyland.DocumentFilters.Canvas canvas = m_filters.MakeOutputCanvas(new Hyland.DocumentFilters.IGRStreamBridge(OutputStream), isys_docfilters.IGR_DEVICE_HTML, OptionsMain);
				try
				{
					for (int pageIndex = 0; pageIndex < item.GetPageCount(); pageIndex++)
					{
						Hyland.DocumentFilters.Page page = item.GetPage(pageIndex);
						try
						{
							m_stderr.Write(" - Page " + (pageIndex + 1) + " => ");

							// When in Async mode, always render the first page directly into the main output
							if (!m_async || pageIndex == 0)
							{
								m_stderr.WriteLine(destination);
								canvas.RenderPage(page);
							}
							else
							{
								String pageFilename = System.IO.Path.ChangeExtension(destination, null) + String.Format("_page_{0}.html", pageIndex + 1);

								m_stderr.WriteLine(pageFilename);

								// Render page stubs instead of the actual page content
								int w = page.Width;
								int h = page.Height;


								String html = "<div class=\"idf-page\" style=\"width: " + w + "px; height: "
								              + h + "px; border: 1px solid black; overflow:hidden;\" title=\"" + System.IO.Path.GetFileName(pageFilename) + "\"></div>";

								byte[] chars = UTF8Encoding.Default.GetBytes(html);
								OutputStream.Write(chars, 0, chars.Length);

								// Render page to it's own file
								Hyland.DocumentFilters.Canvas pageCanvas = m_filters.MakeOutputCanvas(pageFilename, isys_docfilters.IGR_DEVICE_HTML, OptionsPage);
								pageCanvas.RenderPage(page);
								pageCanvas.Close();
							}

							// Embedded Images
							if (!m_inlineImages)
							{
								SubFile image = page.GetFirstImage();
								while (image != null)
								{
									image.CopyTo(System.IO.Path.Combine(m_outputFolder, image.getName()));
									image = page.GetNextImage();
								}
							}
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
				}
				String outDir = System.IO.Path.GetDirectoryName(destination);
				if (!System.IO.File.Exists(outDir + "\\perceptive-viewer-utils.js"))
					System.IO.File.Copy("perceptive-viewer-utils.js", outDir + "\\perceptive-viewer-utils.js", false);
				if (!System.IO.File.Exists(outDir + "\\perceptive-viewer-utils.css"))
					System.IO.File.Copy("perceptive-viewer-utils.css", outDir + "\\perceptive-viewer-utils.css", false);
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
			System.Console.WriteLine("Document Filters 11: ConvertDocumentToInteractiveHDHTML C# Example");
			System.Console.WriteLine("(c) 1988-2017 Perceptive Software");
			System.Console.WriteLine("");
			System.Console.WriteLine("ConvertDocumentToInteractiveHDHTML [options] filename");
			System.Console.WriteLine("");
			System.Console.WriteLine("options");
			System.Console.WriteLine(" -h, --help                  this help");
			System.Console.WriteLine("     --inline-images         indicates that image data should be stored directly into the generated html");
			System.Console.WriteLine("     --options [name=value;] indicates extra options that are to be passed when converting documents");
			System.Console.WriteLine(" -o, --output [folder]       the folder to save the output files, defaults to current directory");
			System.Console.WriteLine(" -a, --async                 generate HTML that loads page data on demand. The generated output ");
			System.Console.WriteLine("                             MUST be loaded into a web server.");
			System.Console.WriteLine("");
			System.Console.WriteLine("                           The page will fail to load using the file:// protocol");

		}

		static void Main(string[] args)
		{
			Program prog = new Program();
			prog.Run(args);
		}
	}

}
