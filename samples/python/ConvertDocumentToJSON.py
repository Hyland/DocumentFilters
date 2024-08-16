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
#* Document Filters Example - Convert a document to a JSON file.
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
		
		with api.MakeOutputCanvas(outFilename, IGR_DEVICE_JSON, canvasOptions) as canvas:
			pageIndex = 0
			for page in file.Pages:
				with page:
					console.write(f"Rendering Page {pageIndex + 1}\n")
					canvas.RenderPage(page)
					pageIndex += 1

try:
	parser = argparse.ArgumentParser(description='Convert Document to JSON.')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='Filename of file to convert')
	parser.add_argument('-o', '--output', dest='output', action='store', help='Output file to create')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')
	
	parser.add_argument('--format-output', dest='format_output', action='store_true', help='add new lines and indentation to the output', default=False)
	parser.add_argument('--include-whitespace', dest='include_whitespace', action='store_true', help='include whitespace words in the output', default=False)
	parser.add_argument('--include-words', dest='include_words', action='store_true', help='include word-level information in the output', default=False)
	parser.add_argument('--skip-empty-paragraphs', dest='skip_empty_paragraphs', action='store_true', help='exclude empty paragraphs in the output', default=False)
	
	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".md"

	# Prepare and Initialize Engine
	DocumentFiltersSample.InitializeAPI(api, args)

	canvasOptions = (
		f"JSON_FORMAT_OUTPUT={args.format_output};"
		f"JSON_INCLUDE_WHITESPACE={args.include_whitespace};"
		f"JSON_INCLUDE_WORDS={args.include_words};"
		f"JSON_SKIP_EMPTY_PARAGRAPHS={args.skip_empty_paragraphs};"
	)

	# Get Extractor and Convert Document
	ProcessFile(args.file, args.output, sys.stderr, "", canvasOptions)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)
