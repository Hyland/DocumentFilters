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
#* Document Filters Example - Convert a document to PDF with a watermark.
#****************************************************************************/

import os, sys, argparse, DocumentFiltersSample
from DocumentFilters import *

api = DocumentFilters()

def ProcessFile(filename, outFilename, watermark, imageFile, console):
	console.write("Processing (FILE): " + filename + "\n")

	with api.GetExtractor(filename) as file:
		docType = file.getFileType()
		console.write("DocType: " + str(docType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(file.getSupportsText()) 
              + ", SupportsSubFiles: " + str(file.getSupportsSubFiles()) + "\n")
		file.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

		# Load the watermark image into memory
		imageBytes = None
		imageWidth = None
		imageHeight = None
		if imageFile is not None:
			imageBytes = io.BytesIO()
			with api.GetExtractor(imageFile) as image:
				image.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)
				with api.MakeOutputCanvas(imageBytes, IGR_DEVICE_IMAGE_PNG) as canvas:
					with image.GetPage(0) as page:
						canvas.RenderPage(page)
						imageWidth = page.Width
						imageHeight = page.Height
			imageBytes.seek(0, 0)
			imageBytes = imageBytes.read()

		with api.MakeOutputCanvas(outFilename, IGR_DEVICE_IMAGE_PDF) as canvas:
			pageIndex = 0
			for page in file.Pages:
				with page:
					console.write(f"Rendering Page {pageIndex + 1}\n")
					canvas.RenderPage(page)

					if watermark is not None:
						width = page.Width
						height = page.Height
						canvas.SetFont("Arial", 12, 0)
						textHeight = canvas.TextHeight("A") + 10
						canvas.SetOpacity(120)
						canvas.SetBrush(0x101010, 0)
						canvas.Rect(40, height - 40 - textHeight, width - 40, height - 40)
						canvas.Ellipse(width - 80, height - 80, width - 20, height - 20)
						canvas.SetBrush(0xFFFFFF, 0)
						canvas.TextOut(50, height - 40 - textHeight + 5, watermark)

					if imageBytes is not None:
						canvas.DrawImage(10, 10, imageBytes, "")

					pageIndex += 1

try:
	parser = argparse.ArgumentParser(description='Get Document Words')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='filename of file to convert')
	parser.add_argument('-o', '--output', dest='output', action='store', help='Output file to create')
	parser.add_argument('-w', '--watermark', metavar='watermark', type=str, nargs='?', default="WATERMARK", help='Text to apply to pages')
	parser.add_argument('-i', '--image', metavar='image', type=str, nargs='?', help='Image to apply to pages')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')

	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".pdf"

	# Prepare and Initialize Engine
	DocumentFiltersSample.InitializeAPI(api, args)

	# Get Extractor and Convert Document
	ProcessFile(args.file, args.output, args.watermark, args.image, sys.stdout)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	raise e
	exit(1)
