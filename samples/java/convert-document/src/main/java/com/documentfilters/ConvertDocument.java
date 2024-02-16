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
import java.text.SimpleDateFormat;

public class ConvertDocument {

	private DocumentFilters docFilters;
	private boolean subFiles;
	private boolean html;

	protected void ProcessFileImpl(String filename, Extractor item, PrintStream OutStream, PrintStream ErrStream)
			throws IGRException {
		try {
			int flags = isys_docfilters.IGR_BODY_AND_META;
			if (html) 
				flags |= isys_docfilters.IGR_FORMAT_HTML;
			item.Open(flags);

			// Extract the text and return it to stdout
			while (!item.getEOF()) {
				String t = item.GetText(4096);
				t = t.replace('\u000E', '\n');
				t = t.replace('\r', '\n');
				OutStream.print(t);
			}
			OutStream.println("");

			// Extract the HTML generated images into the current folder
			if (html) {
				SubFile image = item.GetFirstImage();
				while (image != null) {
					ErrStream.println("Extracting image " + image.getName());
					image.CopyTo(image.getName());
					image.Close();
					// Move on to the next image
					image = item.GetNextImage();
				}
			}

			// Extract the sub files (if any) and process recursively
			if (subFiles) {
				SubFile child = item.GetFirstSubFile();
				while (child != null) {
					SimpleDateFormat dateFormatter = new SimpleDateFormat("EEE, d MMM yyyy HH:mm:ss");
					OutStream.println("Filename: " + filename + ">" + child.getName());
					OutStream.println("Date: " + dateFormatter.format(child.getFileTime()) + "\n");

					ProcessFile(filename + ">" + child.getName(), child, OutStream, ErrStream);
					// Move on to the next sub file
					child = item.GetNextSubFile();
				}
			}
		} finally {
			item.Close();
		}
	}

	protected void ProcessFile(String filename, Extractor item, PrintStream OutStream, PrintStream ErrStream) {
		ErrStream.println(filename);
		try {
			ProcessFileImpl(filename, item, OutStream, ErrStream);
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
		docFilters.Initialize(DocumentFiltersLicense.LICENSE_KEY, ".");

		for (int i = 0; i < args.length; i++) {
			String arg = args[i];
			if (arg.compareToIgnoreCase("--html") == 0) {
				html = true;
			} else if (arg.compareToIgnoreCase("--text") == 0 || arg.compareToIgnoreCase("-t") == 0) {
				html = false;
			} else if (arg.compareToIgnoreCase("--subfiles") == 0 || arg.compareToIgnoreCase("-s") == 0) {
				subFiles = true;
			} else if (arg.compareToIgnoreCase("-h") == 0 || arg.compareToIgnoreCase("--help") == 0) {
				ShowHelp();
				return;
			} else {
				PrintStream out = new PrintStream(System.out, true, "UTF-8");
				PrintStream err = new PrintStream(System.err, true, "UTF-8");
				ProcessFile(args[i], docFilters.GetExtractor(args[i]), out, err);
			}
		}
	}

	protected void ShowHelp() {
		System.out.println("Document Filters 11: ConvertDocument Java Example");
		System.out.println("(c) 2023 Hyland Software, Inc.");
		System.out.println("");
		System.out.println("Usage: ConvertDocument [options] filename");
		System.out.println("");
		System.out.println("Options:");
		System.out.println(" -h, --help        this help");
		System.out.println(" -t, --text        output as plain text");
		System.out.println("     --html        output as HTML text, images stored in current directory");
		System.out.println(" -s, --subfiles    process subfiles");
	}

	public static void main(String[] args) throws Exception {
		ConvertDocument app = new ConvertDocument();
		app.Run(args);
	}
}
