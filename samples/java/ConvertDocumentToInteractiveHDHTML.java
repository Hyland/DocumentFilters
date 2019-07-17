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

import com.perceptive.documentfilters.*;
import java.io.*;
import java.io.File;
import java.util.Date;
import java.text.SimpleDateFormat;

public class ConvertDocumentToInteractiveHDHTML
{

	private DocumentFilters isys;
	private String outputFolder;
	private boolean async;
	private boolean inlineImages;


	ConvertDocumentToInteractiveHDHTML()
	{
		outputFolder = ".";
		async = false;
		inlineImages = false;
	}

	protected String makeFilename(String path, String base, String newExtension)
	{
		String retVal = path;
		if (retVal != "" && !retVal.endsWith(File.separator))
		{
			retVal += File.separator;
		}

		int pos = base.lastIndexOf(File.separator);
		if (pos >= 0)
		{
			base = base.substring(pos + 1);
		}

		if (newExtension != "")
		{
			pos = base.lastIndexOf(".");
			if (pos >= 0)
			{
				base = base.substring(0, pos);
			}
		}

		return retVal + base + newExtension;
	}
	protected String getFilename(String filename)
	{
		int pos = filename.lastIndexOf(File.separator);
		if (pos >= 0)
		{
			return filename.substring(pos + 1);
		}
		return filename;
	}

	protected void ProcessFileImpl(String filename, Extractor item, PrintStream OutStream, PrintStream ErrStream) throws IGRException, IOException
	{
		String destination = makeFilename(outputFolder, filename, ".html");

		try
		{
			String OptionsMain;
			String OptionsPage;

			OptionsMain = OptionsPage =
			                      "HDHTML_INCLUDE_WORD_INDEXES=on;";
			// HDHTML_INCLUDE_WORD_INDEXES will include information about each word in the run data for a page. This is
			// used by the javascript to map between the web browsers co-ordinate system and the word co-ordinate system
			// used by Document Filters. In this example, it's used by the redaction function.

			if (inlineImages)
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

			item.Open(isys_docfilters.IGR_BODY_AND_META | isys_docfilters.IGR_FORMAT_IMAGE);

			java.io.RandomAccessFile outputFile = new java.io.RandomAccessFile(destination, "rw");
			outputFile.setLength(0);

			Canvas canvas = isys.MakeOutputCanvas(new RandomAccessStream(outputFile), isys_docfilters.IGR_DEVICE_HTML, OptionsMain);
			try
			{
				for (int pageIndex = 0; pageIndex < item.GetPageCount(); pageIndex++)
				{
					Page page = item.GetPage(pageIndex);
					try
					{
						ErrStream.print(" - Page " + (pageIndex + 1) + " => ");

						// When in Async mode, always render the first page directly into the main output
						if (!async || pageIndex == 0)
						{
							ErrStream.println(destination);
							canvas.RenderPage(page);
						}
						else
						{
							String pageFilename = makeFilename(outputFolder, filename, String.format("_page_%d.html", pageIndex + 1));

							ErrStream.println(pageFilename);
							// Render page stubs instead of the actual page content
							int w = page.getWidth();
							int h = page.getHeight();

							String html = "<div class=\"idf-page\" style=\"width: " + w + "px; height: "
							              + h + "px; border: 1px solid black; overflow:hidden;\" title=\"" + getFilename(pageFilename) + "\"></div>";

							outputFile.write(html.getBytes("UTF-8"));

							// Render page to it's own file
							Canvas pageCanvas = isys.MakeOutputCanvas(pageFilename, isys_docfilters.IGR_DEVICE_HTML, OptionsPage);
							pageCanvas.RenderPage(page);
							pageCanvas.Close();
						}

						if (!inlineImages)
						{
							SubFile image = page.GetFirstImage();
							while (image != null)
							{
								ErrStream.println("    - Image " + image.getName() + " => " + makeFilename(outputFolder, image.getName(), ""));
								image.CopyTo(makeFilename(outputFolder, image.getName(), ""));
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
			CopyFile("perceptive-viewer-utils.js", outputFolder + File.separator + "perceptive-viewer-utils.js", false);
			CopyFile("perceptive-viewer-utils.css", outputFolder + File.separator + "perceptive-viewer-utils.css", false);
		}
		finally {
			item.Close();
		}
	}

	protected void ProcessFile(String filename, Extractor item, PrintStream OutStream, PrintStream ErrStream)
	{
		ErrStream.println(filename);
		try
		{
			ProcessFileImpl(filename, item, OutStream, ErrStream);
		}
		catch (IGRException err)
		{
			ErrStream.println("IGRException caught while processing " + filename);
			ErrStream.println("Error code: " + Integer.toString(err.getErrorCode()));
			ErrStream.println("Error message: " + err.getMessage());
		}
		catch (Exception err)
		{
			ErrStream.println("Exception caught while processing " + filename);
			ErrStream.println(err.toString());
		}
	}

	public void Run(String[] args) throws Exception
	{
		if (args.length == 0)
		{
			ShowHelp();
			return;
		}

		isys = new DocumentFilters();
		isys.Initialize(DocumentFiltersLicense.LICENSE_KEY, ".");

		for (int i = 0; i < args.length; i++)
		{
			String arg = args[i];

			if (arg.compareToIgnoreCase("-h") == 0 || arg.compareToIgnoreCase("--help") == 0)
			{
				ShowHelp();
				return;
			}
			else if (arg.compareToIgnoreCase("-o") == 0 || arg.compareToIgnoreCase("--output") == 0)
			{
				outputFolder = args[++i];
			}
			else if (arg.compareToIgnoreCase("-a") == 0 || arg.compareToIgnoreCase("--async") == 0)
			{
				async = true;
			}
			else if (arg.compareToIgnoreCase("--inline-images") == 0)
			{
				inlineImages = true;
			}
			else
			{
				PrintStream out = new PrintStream(System.out, true, "UTF-8");
				PrintStream err = new PrintStream(System.err, true, "UTF-8");
				ProcessFile(args[i], isys.GetExtractor(args[i]), out, err);
			}
		}
	}

	protected void ShowHelp()
	{
		System.out.println("Document Filters 11: ConvertDocumentToInteractiveHDHTML Java Example");
		System.out.println("(c) 2019 Hyland Software, Inc.");
		System.out.println("");
		System.out.println("Usage: ConvertDocumentToInteractiveHDHTML [options] filename");
		System.out.println("");
		System.out.println("Options:");
		System.out.println(" -h, --help                this help");
		System.out.println("     --inline-images       indicates that image data should be stored directly into the generated html");
		System.out.println(" -o, --output [folder]     the folder to save the output files, defaults to current directory");
		System.out.println(" -a, --async               generate HTML that loads page data on demand. The generated output ");
		System.out.println("                           MUST be loaded into a web server.");
		System.out.println("");
		System.out.println("                           The page will fail to load using the file:// protocol");
	}
	private void CopyFile(String src, String dst, boolean overwrite) throws IOException
	{
		CopyFile(new java.io.File(src), new java.io.File(dst), overwrite);
	}
	private void CopyFile(java.io.File src, java.io.File dst, boolean overwrite) throws IOException
	{
		if (dst.exists() && !overwrite)
		{
			return;
		}

		dst.createNewFile();
		InputStream in = new FileInputStream(src);
		OutputStream out = new FileOutputStream(dst);

		// Transfer bytes from in to out
		byte[] buf = new byte[1024];
		int len;
		while ((len = in.read(buf)) > 0)
		{
			out.write(buf, 0, len);
		}
		in.close();
		out.close();
	}
	public static void main(String[] args) throws Exception
	{
		ConvertDocumentToInteractiveHDHTML app = new ConvertDocumentToInteractiveHDHTML();
		app.Run(args);
	}

	class RandomAccessStream extends IGRStream
	{
		java.io.RandomAccessFile m_file;

		RandomAccessStream(java.io.RandomAccessFile file)
		{
			m_file = file;
		}

		public long Seek(long Offset, int Origin)
		{
			try
			{
				if (Origin == 0)
				{ //-->Seek Set
					m_file.seek(Offset);
				}
				else if (Origin == 1)
				{ //-->Seek Current
					m_file.seek(m_file.getFilePointer() + Offset);
				}
				else if (Origin == 2)
				{ //-->Seek End
					m_file.seek(m_file.length() + Offset);
				}
				return m_file.getFilePointer();
			}
			catch (IOException e)
			{
				return -1;
			}
		}

		public long Write(byte[] bytes)
		{
			try
			{
				m_file.write(bytes);
				return bytes.length;
			}
			catch (IOException e)
			{
				return 0;
			}
		}

		public long Read(long Size, IGRStream_Data Dest)
		{
			try
			{
				byte[] d = new byte[(int) Size];
				long ret = m_file.read(d, 0, (int)Size);

				// If the amount read is different that what was asked for, make a copy
				if (ret != Size)
				{
					byte[] arrDest = new byte[(int) ret];
					System.arraycopy(d, 0, arrDest, 0, (int) ret);
					Dest.write(arrDest);
				}
				else
				{
					Dest.write(d);
				}
				return ret;
			}
			catch (IOException e)
			{
				return 0;
			}
		}
	}
}
