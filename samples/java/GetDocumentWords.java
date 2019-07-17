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

import com.perceptive.documentfilters.*;
import java.io.*;
import java.io.File;
import java.util.Date;
import java.text.SimpleDateFormat;

public class GetDocumentWords
{

	private DocumentFilters m_filters;

	GetDocumentWords()
	{
	}

	protected String makeFilename(String path, String base, String newExtension)
	{
		String retVal = path;
		if (!retVal.endsWith(File.separator))
		{
			retVal += File.separator;
		}

		int pos = base.lastIndexOf(File.separator);
		if (pos >= 0)
		{
			base = base.substring(pos + 1);
		}

		pos = base.lastIndexOf(".");
		if (pos >= 0)
		{
			base = base.substring(0, pos);
		}

		return retVal + base + newExtension;
	}

	protected void ProcessFileImpl(String filename, Extractor item, PrintStream OutStream, PrintStream ErrStream) throws IGRException
	{
		try
		{
			item.Open(isys_docfilters.IGR_BODY_AND_META | isys_docfilters.IGR_FORMAT_IMAGE);

			for (int pageIndex = 0; pageIndex < item.GetPageCount(); pageIndex++)
			{
				Page page = item.GetPage(pageIndex);
				try
				{
					System.out.println(String.format("Page %-16d[width: %3d; height: %3d; words: %3d]", pageIndex + 1, page.getWidth(), page.getHeight(), page.getWordCount()));

					for (Word word = page.getFirstWord(); word != null; word = page.getNextWord())
					{
						System.out.println(String.format("%3d. %-15s [x: %4d; y: %4d; width: %3d; height: %3d; character: %3d]",
						                                 word.getWordIndex(), word.getText(), word.getX(), word.getY(), word.getWidth(), word.getHeight(), word.getCharacterOffset()));
					}
				}
				finally
				{
					page.Close();
				}
			}
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
	}

	public void Run(String[] args) throws Exception
	{
		if (args.length == 0)
		{
			ShowHelp();
			return;
		}

		m_filters = new DocumentFilters();
		m_filters.Initialize(DocumentFiltersLicense.LICENSE_KEY, ".");

		for (int i = 0; i < args.length; i++)
		{
			String arg = args[i];

			if (arg.compareToIgnoreCase("-h") == 0 || arg.compareToIgnoreCase("--help") == 0)
			{
				ShowHelp();
				return;
			}
			else
			{
				PrintStream out = new PrintStream(System.out, true, "UTF-8");
				PrintStream err = new PrintStream(System.err, true, "UTF-8");
				ProcessFile(args[i], m_filters.GetExtractor(args[i]), out, err);
			}
		}
	}

	protected void ShowHelp()
	{
		System.out.println("Document Filters 11: GetDocumentWords Java Example");
		System.out.println("(c) 2019 Hyland Software, Inc.");
		System.out.println("");
		System.out.println("Usage: GetDocumentWords [options] filename");
		System.out.println("");
		System.out.println("Options:");
		System.out.println(" -h, --help                this help");
	}

	public static void main(String[] args) throws Exception
	{
		GetDocumentWords app = new GetDocumentWords();
		app.Run(args);
	}
}
