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
#* Document Filters Example - Combine one or more documents
#****************************************************************************/

import os, sys, argparse, DocumentFiltersLicense
import shutil
from DocumentFilters import *

api = DocumentFilters()

def GetCanvasType(filename):
	ext = os.path.splitext(filename)[1].lower()
	if (ext == ".pdf"): return IGR_DEVICE_IMAGE_PDF
	if (ext == ".tif"): return IGR_DEVICE_IMAGE_TIF
	if (ext == ".xps"): return IGR_DEVICE_IMAGE_XPS
	if (ext == ".html"): return IGR_DEVICE_HTML
	if (ext == ".xml"): return IGR_DEVICE_XML
	raise Exception(f"I don't know how to create {ext} files")

def Process(args, console):

	with api.MakeOutputCanvas(args.output, GetCanvasType(args.output), "PDF_PRESERVE_ORIGINAL=on") as canvas:

		for filename in args.files:
			try:
				console.write(f"Loading {filename}\n")
				with api.GetExtractor(filename) as file:
					file.Open(IGR_FORMAT_IMAGE)

					pageIndex = 0
					for page in file.Pages:
						with page:
							console.write(f" Adding page {pageIndex+1}\n")
							canvas.RenderPage(page)
							pageIndex += 1

			except Exception as err:
				console.write("Error: {0}\n".format(err))

try:
	parser = argparse.ArgumentParser(description='Get Document Type')
	parser.add_argument('files', metavar='files', type=str, nargs='+', help='Filenames of file to combine')
	parser.add_argument('-o', '--output', dest='output', action='store', default="output.pdf", help='Output file name to create')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')
	
	args = parser.parse_args()
	if args.license_key is None: args.license_key = DocumentFiltersLicense.LICENSE_KEY
	if args.library_path is None: args.library_path = os.environ.get("DF_PATH")

	# Prepare and Initialize Engine
	api.Initialize(args.license_key, ".", args.library_path)

	# Get Extractor and Convert Document
	Process(args, sys.stdout)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)
