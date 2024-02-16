#
# (c) 2022 Hyland Software, Inc. and its affiliates. All rights reserved.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

import os, sys, argparse, DocumentFiltersSample
import shutil
from DocumentFilters import *

api = DocumentFilters()

def ProcessFile(filename, outFilename, console, is_async):
	console.write("Processing (FILE): " + filename + "\n")

	with api.GetExtractor(filename) as file:
		optionsMain = optionsPage = "HTML_INLINE_IMAGES=on;" + "HDHTML_INCLUDE_WORD_INDEXES=on;"
		# HTML_INLINE_IMAGES forces any images to be inlined directly into the HTML using url(data:image/png;base64,
		# rather than having external files. 
		#
		# HDHTML_INCLUDE_WORD_INDEXES will include information about each word in the run data for a page. This is 
		# used by the javascript to map between the web browsers co-ordinate system and the word co-ordinate system
		# used by Document Filters. In this example, it's used by the redaction function.

		optionsMain += "HDHTML_OUTPUT_INJECT_HEAD=@html-redist/perceptive-viewer-inject.html;"
		# HDHTML_OUTPUT_INJECT_HEAD will load the contents of the perceptive-viewer-inject.html file and place it
		# at the bottom of the <HEAD> section of the main HTML page. This allows us to inject stylesheets, javascript
		# and extra metadata that will be loaded when the page is viewed.

		optionsPage += "HDHTML_OUTPUT_BOILERPLATE=off"
		# HDHTML_OUTPUT_BOILERPLATE disables the surrounding <html>...<body> tags that typically get generated into
		# the HTML output.  This is used when generateing is_async pages, which must only contain the actual page data
		# and not the surrounding html.
	
		docType = file.getFileType()
		console.write("docType: " + str(docType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(file.getSupportsText()) 
              + ", SupportsSubFiles: " + str(file.getSupportsSubFiles()) + "\n")
		
		file.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

		with open(outFilename, 'w+b') as canvasStream:
			with api.MakeOutputCanvas(canvasStream, IGR_DEVICE_HTML, optionsMain) as canvas:
				pageIndex = 0
				for page in file.Pages:
					with page:
						console.write("Rendering Page " + str(pageIndex + 1) + "\n")
						if not is_async or pageIndex == 0:
							canvas.RenderPage(page)
						else:
							w = page.GetWidth()
							h = page.GetHeight()
							pageFilename = os.path.splitext(outFilename)[0] + "_page_" + str(pageIndex + 1) + ".html"
							html = "<div class=\"idf-page\" style=\"width: " + str(w) + "px; height: " + str(h) + "px; border: 1px solid black; overflow:hidden;\" title=\"" + os.path.basename(pageFilename) + "\">"
							html += "</div>\n" 
							canvasStream.write(html)

							with DocumentFilters.MakeOutputCanvas(pageFilename, IGR_DEVICE_HTML, optionsPage) as pageCanvas:
								pageCanvas.RenderPage(page)
						pageIndex += 1

				outDir = os.path.dirname(outFilename);
				if not outDir == "" and not outDir == ".":
					shutil.copy("html-redist/perceptive-viewer-utils.js", outDir)
					shutil.copy("html-redist/perceptive-viewer-utils.css", outDir)

try:
	parser = argparse.ArgumentParser(description='Convert Document to Interactive HTML.')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='filename of file to convert')
	parser.add_argument('-a', '--async', dest='is_async', action='store', help='Generate HTML that loads page data on demand. The generated output MUST be loaded into a web server.')
	parser.add_argument('-o', '--output', dest='output', action='store', help='Output file to create')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')

	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".html"

	# Prepare and Initialize Engine
	DocumentFiltersSample.InitializeAPI(api, args)

	# Ensure that the required viewer redistributables are findable
	if not (os.path.isfile("html-redist/perceptive-viewer-utils.js") and os.path.isfile("html-redist/perceptive-viewer-utils.css") and os.path.isfile("html-redist/perceptive-viewer-inject.html")):
		sys.stderr.write("Error: redistributable files are missing from application directory\n")
		sys.stderr.write(" - perceptive-viewer-utils.js\n")
		sys.stderr.write(" - perceptive-viewer-utils.css\n")
		sys.stderr.write(" - perceptive-viewer-inject.html\n")
		sys.exit(1)

	# Get Extractor and Convert Document
	ProcessFile(args.file, args.output, sys.stderr, args.is_async)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)
