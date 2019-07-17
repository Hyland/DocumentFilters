#
#(c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
#ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#****************************************************************************
#* Document Filters Example - Convert a document to HDHTML
#*
#* This example creates a html view of a document which can be manipulated by
#* javascript to provide more functionality than a standard static page.
#*
#* There are two parts to the sample, the page generate logic use standard
#* Document Filters HDHTML exports to create the page data. The sample then
#* injects CSS and Javascript to provide extra viewing capabilities, such as
#* Zoom, Rotate, Highlight, Redact and Metadata Preview.
#*
#****************************************************************************/

from __future__ import unicode_literals
import io
import os
import shutil
import sys
import DocumentFiltersLicense
import DocumentFiltersUtils
from ISYS11dfpython import *

global DocumentFilters

def ProcessFile(Filename, OutFilename, ErrStream, Async):
	ErrStream.write("Processing (FILE): " + Filename + "\n")

	try:
		Extractor = DocumentFilters.GetExtractor(DocumentFiltersUtils.ToUnicodeString(Filename))

		OptionsMain = OptionsPage = "HTML_INLINE_IMAGES=on;" + "HDHTML_INCLUDE_WORD_INDEXES=on;"
		# HTML_INLINE_IMAGES forces any images to be inlined directly into the HTML using url(data:image/png;base64,
		# rather than having external files. 
		#
		# HDHTML_INCLUDE_WORD_INDEXES will include information about each word in the run data for a page. This is 
		# used by the javascript to map between the web browsers co-ordinate system and the word co-ordinate system
		# used by Document Filters. In this example, it's used by the redaction function.

		OptionsMain += "HDHTML_OUTPUT_INJECT_HEAD=@perceptive-viewer-inject.html;"
		# HDHTML_OUTPUT_INJECT_HEAD will load the contents of the perceptive-viewer-inject.html file and place it
		# at the bottom of the <HEAD> section of the main HTML page. This allows us to inject stylesheets, javascript
		# and extra metadata that will be loaded when the page is viewed.

		OptionsPage += "HDHTML_OUTPUT_BOILERPLATE=off"
		# HDHTML_OUTPUT_BOILERPLATE disables the surrounding <html>...<body> tags that typically get generated into
		# the HTML output.  This is used when generateing ASYNC pages, which must only contain the actual page data
		# and not the surrounding html.
	
		DocType = Extractor.getFileType()
		ErrStream.write("DocType: " + str(DocType) + ", " + Extractor.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(Extractor.getSupportsText()) 
              + ", SupportsSubFiles: " + str(Extractor.getSupportsSubFiles()) + "\n")
		Extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)
		OutputStream = open(OutFilename, 'w+b')
		Canvas = DocumentFilters.MakeOutputCanvas(OutputStream, IGR_DEVICE_HTML, OptionsMain)

		try:
			for pageIndex in range(Extractor.GetPageCount()):
				ErrStream.write("Rendering Page " + str(pageIndex + 1) + "\n")
				try:
					Page = Extractor.GetPage(pageIndex)	
					if not Async or pageIndex == 0:
						Canvas.RenderPage(Page)
					else:
						w = Page.GetWidth()
						h = Page.GetHeight()
						PageFilename = os.path.splitext(OutFilename)[0] + "_page_" + str(pageIndex + 1) + ".html"
						html = "<div class=\"idf-page\" style=\"width: " + str(w) + "px; height: " + str(h) + "px; border: 1px solid black; overflow:hidden;\" title=\"" + os.path.basename(PageFilename) + "\">"
						html += "</div>\n" 
						OutputStream.write(html)
						PageCanvas = DocumentFilters.MakeOutputCanvas(PageFilename, IGR_DEVICE_HTML, OptionsPage)
						PageCanvas.RenderPage(Page)
				finally:
					Page.Close()

			OutDir = os.path.dirname(OutFilename);
			if not OutDir == "" and not OutDir == ".":
				shutil.copy("perceptive-viewer-utils.js", OutDir)
				shutil.copy("perceptive-viewer-utils.css", OutDir)
		finally:
			Canvas.Close()
			
	except Exception as e:
		ErrStream.write("ProcessFile: " + str(e) + "\n")
	finally:
		Extractor.Close()

def ShowHelp():
	sys.stdout.write("Document Filters 11: ConvertDocumentToHDHTML Python Example\n")
	sys.stdout.write("(c) 2019 Hyland Software, Inc.\n")
	sys.stdout.write("ALL RIGHTS RESERVED\n")
	sys.stdout.write("\n")
	sys.stdout.write("Usage: " + sys.argv[0] + " [options] DocFileName OutFilename\n")
	sys.stdout.write("Options\n")
	sys.stdout.write("    -a, --async        Generate HTML that loads page data on demand. The generated output \n")
	sys.stdout.write("                       MUST be loaded into a web server.\n\n")
	sys.stdout.write("                       The page will fail to load using the file:// protocol\n")
	sys.exit(0)

if len(sys.argv) < 3:
	ShowHelp()

# Prepare and Initialize Engine
DocumentFilters = DocumentFilters()
try:
	DocumentFilters.Initialize(DocumentFiltersLicense.LICENSE_KEY, ".")
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)

# Ensure that the required viewer redistributables are findable
if not (os.path.isfile("perceptive-viewer-utils.js") and os.path.isfile("perceptive-viewer-utils.css") and os.path.isfile("perceptive-viewer-inject.html")):
	sys.stderr.write("Error: redistributable files are missing from application directory\n")
	sys.stderr.write(" - perceptive-viewer-utils.js\n")
	sys.stderr.write(" - perceptive-viewer-utils.css\n")
	sys.stderr.write(" - perceptive-viewer-inject.html\n")
	sys.exit(1)

# Process command line arguments
Async=False
Filename=""
OutFilename=""

for arg in sys.argv[1:]:
	if arg == "-a" or arg == "--async":
		Async=True
	elif Filename == "":
		Filename = arg
	elif OutFilename == "":
		OutFilename = arg
	else:
		sys.stderr.write("Unexected command line argument: " + arg + "\n")
		sys.exit(1)
	
# Get Extractor and Convert Document

ProcessFile(Filename, OutFilename, sys.stderr, Async)
