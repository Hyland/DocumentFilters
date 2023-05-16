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
#* Document Filters Example - Convert a document to a series of PNG images
#****************************************************************************/

import os, sys, argparse, DocumentFiltersLicense
from DocumentFilters import *

api = DocumentFilters()

def ProcessFile(filename, outFilename, console, page, options):
	console.write("Processing (FILE): " + filename + "\n")

	with api.GetExtractor(filename) as file:
		docType = file.getFileType()
		console.write("docType: " + str(docType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(file.getSupportsText()) 
              + ", SupportsSubFiles: " + str(file.getSupportsSubFiles()) + "\n")
		file.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, options)
		
		with api.MakeOutputCanvas(outFilename, IGR_DEVICE_IMAGE_PNG, "") as canvas:
			with file.GetPage(int(page)) as page:
				console.write(f"Rendering Page\n")
				canvas.RenderPage(page)

try:
	parser = argparse.ArgumentParser(description='Convert Document to PDF.')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='Filename of file to convert')
	parser.add_argument('-o', '--output', dest='output', action='store', help='Output file to create')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')
	parser.add_argument('-p', '--page', dest='page', action='store', type=int, default=1, help='Page number of page to create thumbnail from.')
	parser.add_argument('-w', '--width', dest='width', action='store', type=int, default=100, help='The width of the thumbnail to generate.')
	
	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".png"
	if args.license_key is None: args.license_key = DocumentFiltersLicense.LICENSE_KEY
	if args.library_path is None: args.library_path = os.environ.get("DF_PATH")

	# Prepare and Initialize Engine
	api.Initialize(args.license_key, ".", args.library_path)

	# Get Extractor and Convert Document
	ProcessFile(args.file, args.output, sys.stderr, args.page, f"GRAPHIC_WIDTH={args.width}")
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)
