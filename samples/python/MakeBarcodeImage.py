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
from xml.dom.minidom import Document
import shutil
from DocumentFilters import *

api = DocumentFilters()

BarcodeMap = {
	"qr": DocumentFilters.Annotations.QrCode,
	"dm": DocumentFilters.Annotations.DataMatrix,
	"pdf417": DocumentFilters.Annotations.PDF417,
	"code39": DocumentFilters.Annotations.Code39,
	"code93": DocumentFilters.Annotations.Code93,
	"code128": DocumentFilters.Annotations.Code128,
	"gs1-128": DocumentFilters.Annotations.GS1_128,
	"aztec": DocumentFilters.Annotations.Aztec,
}

def GetCanvasType(filename):
	ext = os.path.splitext(filename)[1].lower()
	if (ext == ".pdf"): return IGR_DEVICE_IMAGE_PDF
	if (ext == ".tif"): return IGR_DEVICE_IMAGE_TIF
	if (ext == ".xps"): return IGR_DEVICE_IMAGE_XPS
	if (ext == ".html"): return IGR_DEVICE_HTML
	if (ext == ".xml"): return IGR_DEVICE_XML
	if (ext == ".png"): return IGR_DEVICE_IMAGE_PNG
	if (ext == ".jpg"): return IGR_DEVICE_IMAGE_JPG
	if (ext == ".bmp"): return IGR_DEVICE_IMAGE_BMP
	raise Exception(f"I don't know how to create {ext} files")

def Process(args, console):
	barcode = BarcodeMap[args.type]()
	barcode.Content = args.text
	barcode.Caption = args.caption
	barcode.Rect = DocumentFilters.AnnotationRect.FromLTRB(10, 10, args.width - 10, args.height - 10)

	with api.MakeOutputCanvas(args.output, GetCanvasType(args.output)) as canvas:
		canvas.BlankPage(args.width, args.height)
		canvas.Annotate(barcode)

	console.write(f"Saved {args.output}")

try:
	parser = argparse.ArgumentParser(description='Get Document Type')
	parser.add_argument('text', metavar='text', type=str, nargs='?', help='Text to encode into barcode')
	parser.add_argument('-o', '--output', dest='output', action='store', default="output.png", help='Output file name to create')
	parser.add_argument('-t', '--type', dest='type', action='store', default="qr", help=f'Type of barcode to create ({(", ".join(BarcodeMap.keys()))})')
	parser.add_argument('--width',  dest='width', action='store', type=int, default=250, help='Width of barcode to create')
	parser.add_argument('--height', dest='height', action='store', type=int, default=250, help='Height of barcode to create')
	parser.add_argument('--caption', dest='caption', action='store', default=None, help='Caption to apply to barcode')
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
