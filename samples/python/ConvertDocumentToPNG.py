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

import os, sys, argparse, DocumentFiltersSample
from DocumentFilters import *

api = DocumentFilters()

def GenerateFilename(base, pageNum):
	filename = os.path.basename(os.path.splitext(base)[0])
	Directory = os.path.dirname(base)
	if Directory and Directory[-1:] != os.sep:
		Directory += os.sep
	filename = Directory + filename + "_page" + str(pageNum) + ".png"
	return filename

def ProcessFile(filename, outFilename, console):
	console.write("Processing (FILE): " + filename + "\n")

	# try:
	with api.GetExtractor(filename) as file:
		docType = file.getFileType()
		console.write("DocType: " + str(docType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME)
              + ", SupportsText: " + str(file.getSupportsText())
              + ", SupportsSubFiles: " + str(file.getSupportsSubFiles()) + "\n")
		file.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

		pageIndex = 0
		for page in file.Pages:
			with page:
				pageFilename = GenerateFilename(outFilename, pageIndex + 1)
				console.write(f"Rendering Page {pageIndex+1} -> {pageFilename}\n")

				with api.MakeOutputCanvas(pageFilename, IGR_DEVICE_IMAGE_PNG, "") as canvas:
					canvas.RenderPage(page)
					pageIndex += 1

try:
	parser = argparse.ArgumentParser(description='Convert Document to PNGs.')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='filename of file to convert')
	parser.add_argument('-o', '--output', dest='output', action='store', help='output file to create')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='license key for Document Filters')

	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".png"
	
	# Prepare and Initialize Engine
	DocumentFiltersSample.InitializeAPI(api, args)

	# Get Extractor and Convert Document
	ProcessFile(args.file, args.output, sys.stderr)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)

