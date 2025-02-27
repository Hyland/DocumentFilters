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

package com.documentfilters;

import com.perceptive.documentfilters.*;
import java.io.*;
import java.io.File;

public class ConvertDocumentToTIFFFromMemory {

	private DocumentFilters docFilters;
	private String outputFolder;

	ConvertDocumentToTIFFFromMemory() {
		outputFolder = ".";
	}

	protected String makeFilename(String path, String base, String newExtension) {
		String retVal = path;
		if (!retVal.endsWith(File.separator)) {
			retVal += File.separator;
		}

		int pos = base.lastIndexOf(File.separator);
		if (pos >= 0) {
			base = base.substring(pos + 1);
		}

		pos = base.lastIndexOf(".");
		if (pos >= 0) {
			base = base.substring(0, pos);
		}

		return retVal + base + newExtension;
	}

	private byte[] readAllBytes(String filename) throws Exception {
		RandomAccessFile f = new RandomAccessFile(filename, "r");
		byte[] fileArray = new byte[(int) f.length()];
		f.readFully(fileArray);
		f.close();
		return fileArray;
	}

	protected void ProcessFileImpl(String filename, Extractor item, PrintStream ErrStream) throws IGRException {
		String destination = makeFilename(outputFolder, filename, ".tif");

		try {
			item.Open(isys_docfilters.IGR_BODY_AND_META | isys_docfilters.IGR_FORMAT_IMAGE);

			Canvas canvas = docFilters.MakeOutputCanvas(destination, isys_docfilters.IGR_DEVICE_IMAGE_TIF, "");
			try {
				for (int pageIndex = 0; pageIndex < item.GetPageCount(); pageIndex++) {
					Page page = item.GetPage(pageIndex);
					try {
						canvas.RenderPage(page);
					} finally {
						page.Close();
					}
				}
			} finally {
				canvas.Close();
			}
		} finally {
			item.Close();
		}
	}

	protected void ProcessFile(String filename, Extractor item, PrintStream ErrStream) {
		ErrStream.println(filename);
		try {
			ProcessFileImpl(filename, item, ErrStream);
		} catch (IGRException err) {
			ErrStream.println("IGRException caught while processing " + filename);
			ErrStream.println("Error code: " + Integer.toString(err.getErrorCode()));
			ErrStream.println("Error message: " + err.getMessage());
		}
	}

	public void Run(String[] args) throws Exception {
		if (args.length == 0) {
			ShowHelp();
			return;
		}

		docFilters = new DocumentFilters();
		docFilters.Initialize(DocumentFiltersSample.GetLicense(), ".");

		for (int i = 0; i < args.length; i++) {
			String arg = args[i];

			if (arg.compareToIgnoreCase("-h") == 0 || arg.compareToIgnoreCase("--help") == 0) {
				ShowHelp();
				return;
			} else if (arg.compareToIgnoreCase("-o") == 0 || arg.compareToIgnoreCase("--output") == 0) {
				outputFolder = args[++i];
			} else {
				PrintStream err = new PrintStream(System.err, true, "UTF-8");
				ProcessFile(args[i], docFilters.GetExtractor(readAllBytes(args[i])), err);
			}
		}
	}

	protected void ShowHelp() {
		System.out.println("Document Filters 11: ConvertDocumentToTIFFFromMemory Java Example");
		System.out.println("(c) 2023 Hyland Software, Inc.");
		System.out.println("");
		System.out.println("Usage: ConvertDocumentToTIFFFromMemory [options] filename");
		System.out.println("");
		System.out.println("Options:");
		System.out.println(" -h, --help                this help");
		System.out.println(
				" -o, --output [folder]     the folder to save the output files, defaults to current directory");
	}

	public static void main(String[] args) throws Exception {
		ConvertDocumentToTIFFFromMemory app = new ConvertDocumentToTIFFFromMemory();
		app.Run(args);
	}
}
