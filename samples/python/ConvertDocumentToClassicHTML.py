#
#(c) 2022 Hyland Software, Inc. and its affiliates. All rights reserved.
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
#* Document Filters Example - Convert a document (and sub-documents) to classic HTML
#****************************************************************************/

import os, sys, argparse, DocumentFiltersSample
from DocumentFilters import *

api = DocumentFilters()


def ProcessFile(filename, outputFile, console):
	outputDir = os.path.dirname(outputFile)

	with api.GetExtractor(filename) as file:
		MaxCharsPerGetText = 4096
		try:
			DocType = file.getFileType()
			console.write("DocType: " + str(DocType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME) 
				+ ", SupportsText: " + str(file.getSupportsText()) 
				+ ", SupportsSubFiles: " + str(file.getSupportsSubFiles()) + "\n")

			if file.getSupportsText():
				file.Open(IGR_BODY_AND_META | IGR_FORMAT_HTML, "")

				# Extract the HTML body of the document
				htmlStream = open(outputFile, "wb")
				while not file.getEOF():
					htmlStream.write(file.GetText(MaxCharsPerGetText, stripControlCodes=True).encode('utf8'))
				htmlStream.close()

				# Extract any embedded images
				for image in file.Images:
					with image:
						console.write("Processing (IMAGE): " + image.getID() + " -> " + image.getName() + "\n")
						image.CopyTo(os.path.join(outputDir, image.getName()))

		except Exception as e:
			console.write("ProcessDocument: " + str(e) + "\n")

try:
	parser = argparse.ArgumentParser(description='Convert Document to Classic HTML.')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='filename of file to convert')
	parser.add_argument('-o', '--output', dest='output', action='store', help='output file to create')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='license key for Document Filters')

	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".html"

	# Prepare and Initialize Engine
	DocumentFiltersSample.InitializeAPI(api, args)

	# Get Extractor and Convert Document
	ProcessFile(args.file, args.output, sys.stderr)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)

