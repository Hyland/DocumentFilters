#
# (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.
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
#* Document Filters Example - Convert a document to a Markdown file.
#****************************************************************************/

import os, sys, argparse, DocumentFiltersSample
from DocumentFilters import *

api = DocumentFilters()

def ProcessFile(filename, outFilename, console, options, canvasOptions):
	console.write("Processing (FILE): " + filename + "\n")

	with api.GetExtractor(filename) as file:
		docType = file.getFileType()
		console.write("docType: " + str(docType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(file.getSupportsText()) 
              + ", SupportsSubFiles: " + str(file.getSupportsSubFiles()) + "\n")
		file.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, options)
		
		with api.MakeOutputCanvas(outFilename, IGR_DEVICE_MARKDOWN, canvasOptions) as canvas:
			pageIndex = 0
			for page in file.Pages:
				with page:
					console.write(f"Rendering Page {pageIndex + 1}\n")
					canvas.RenderPage(page)
					pageIndex += 1

try:
	parser = argparse.ArgumentParser(description='Convert Document to Markdown.')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='Filename of file to convert')
	parser.add_argument('-o', '--output', dest='output', action='store', help='Output file to create')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')
	
	parser.add_argument('--include-headers', dest='include_headers', action='store_true', help='include headers in the output', default=False)
	parser.add_argument('--include-footers', dest='include_footers', action='store_true', help='include footers in the output', default=False)
	parser.add_argument('--include-images', dest='include_images', action='store_true', help='include images in the output', default=False)
	parser.add_argument('--metadata-format', dest='metadata_format', action='store', type=str, help='metadata format to use in the output [JSON, YAML, COMMENTS]', default="COMMENTS")
	parser.add_argument('--heading-style', dest='heading_style', action='store', type=str, help='heading style to use in the output [ATX, SETEXT]', default="ATX")
	parser.add_argument('--simple-table-style', dest='simple_table_style', action='store', type=str, help='table style to use in the output [PIPE, GRID, HTML, PIPE_WITH_HTML]', default="PIPE")
	parser.add_argument('--complex-table-style', dest='complex_table_style', action='store', type=str, help='table style to use in the output  [PIPE, GRID, HTML, PIPE_WITH_HTML]', default="HTML")
	
	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".md"

	# Prepare and Initialize Engine
	DocumentFiltersSample.InitializeAPI(api, args)

	canvasOptions = (
		f"MARKDOWN_HEADERS_STYLE={args.heading_style};"
		f"MARKDOWN_SIMPLE_TABLE_STYLE={args.simple_table_style};"
		f"MARKDOWN_COMPLEX_TABLE_STYLE={args.complex_table_style};"
		f"MARKDOWN_INCLUDE_HEADERS={args.include_headers};"
		f"MARKDOWN_INCLUDE_FOOTERS={args.include_footers};"
		f"MARKDOWN_INCLUDE_IMAGES={args.include_images};"
	)

	if args.metadata_format == "None":
		canvasOptions += "MARKDOWN_METADATA_FORMAT=None;MARKDOWN_INCLUDE_METADATA=ON;"

	# Get Extractor and Convert Document
	ProcessFile(args.file, args.output, sys.stderr, "", canvasOptions)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)
