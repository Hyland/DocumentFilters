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

public class ConvertDocumentToTIFFStream {

	private DocumentFilters docFilters;

	ConvertDocumentToTIFFStream() {
	}

	protected void ProcessFileImpl(OutputStream outFile, Extractor item, PrintStream ErrStream) throws IGRException {
		try {
			item.Open(isys_docfilters.IGR_BODY_AND_META | isys_docfilters.IGR_FORMAT_IMAGE);
			CustomStream outStream = new CustomStream();
			Canvas canvas = docFilters.MakeOutputCanvas(outStream, isys_docfilters.IGR_DEVICE_IMAGE_TIF, "");
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
				// Write in-memory stream contents to existing open file (e.g. stdout)
				long BytesWritten = outStream.writeTo(outFile);
				ErrStream.println(BytesWritten + " bytes written to outFile");
			}
		} catch (IOException ioe) {
			ErrStream.println("IOException: " + ioe.getMessage());
		} finally {
			item.Close();
		}
	}

	protected void ProcessFile(String filename, OutputStream outFile, Extractor item, PrintStream ErrStream) {
		try {
			ProcessFileImpl(outFile, item, ErrStream);
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
			} else {
				PrintStream out = new PrintStream(System.out, true);
				PrintStream err = new PrintStream(System.err, true, "UTF-8");
				ProcessFile(args[i], out, docFilters.GetExtractor(args[i]), err);
			}
		}
	}

	protected void ShowHelp() {
		System.out.println("Document Filters 11: ConvertDocumentToTIFFStream Java Example");
		System.out.println("(c) 2023 Hyland Software, Inc.");
		System.out.println("");
		System.out.println("Usage: ConvertDocumentToTIFFStream filename [> OutFilename]");
		System.out.println("");
		System.out.println("Options:");
		System.out.println(" -h, --help                this help");
	}

	public static void main(String[] args) throws Exception {
		ConvertDocumentToTIFFStream app = new ConvertDocumentToTIFFStream();
		app.Run(args);
	}
}
