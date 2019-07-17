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
#* Document Filters Example - Convert a document to a watermarked PDF
#****************************************************************************/

from __future__ import unicode_literals
import sys
import DocumentFiltersLicense
import DocumentFiltersUtils
from ISYS11dfpython import *

global DocumentFilters

def ProcessFile(Filename, OutFilename, ImageName, ErrStream):
	ErrStream.write("Processing (FILE): " + Filename + "\n")
	try:
		Extractor = DocumentFilters.GetExtractor(DocumentFiltersUtils.ToUnicodeString(Filename))
		DocType = Extractor.getFileType()
		ErrStream.write("DocType: " + str(DocType) + ", " + Extractor.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(Extractor.getSupportsText()) 
              + ", SupportsSubFiles: " + str(Extractor.getSupportsSubFiles()) + "\n")
		Extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)
		Canvas = DocumentFilters.MakeOutputCanvas(OutFilename, IGR_DEVICE_IMAGE_PDF, "")

		ImageSize = None
		ImageData = None
		if not (ImageName is None):
			ImageStream = open(ImageName,"rb")
			if not (ImageStream is None):
				ImageStream.seek(0, 2)
				ImageSize = ImageStream.tell()
				ImageStream.seek(0, 0)
				ImageData = ImageStream.read(ImageSize)

		try:
			for pageIndex in range(Extractor.GetPageCount()):
				ErrStream.write("Rendering Page " + str(pageIndex) + "\n")
				try:
					Page = Extractor.GetPage(pageIndex)
					Canvas.RenderPage(Page)
					
					Width = Page.GetWidth()
					Height = Page.GetHeight()
					Canvas.SetFont("Arial", 12, 0)
					TextHeight = Canvas.TextHeight("A") + 10
					Canvas.SetOpacity(120)
					Canvas.SetBrush(0x101010, 0)
					Canvas.Rect(40, Height - 40 - TextHeight, Width - 40, Height - 40)
					Canvas.Ellipse(Width - 80, Height - 80, Width - 20, Height - 20)
					Canvas.SetBrush(0xFFFFFF, 0)
					Canvas.TextOut(50, Height - 40 - TextHeight + 5, "This document was converted using Document Filters 11")

					if not (ImageSize is None):
						Canvas.SetOpacity(70)
						Canvas.DrawImage(40, 40, ImageData, "image/png")

				finally:
					Page.Close()	
		finally:
			Canvas.Close()
	except Exception as e:
		ErrStream.write("ProcessFile: " + str(e) + "\n")
	finally:
		Extractor.Close()

def ShowHelp():
	sys.stdout.write("Document Filters 11: WatermarkDocument Python Example\n")
	sys.stdout.write("(c) 2019 Hyland Software, Inc.\n")
	sys.stdout.write("ALL RIGHTS RESERVED\n")
	sys.stdout.write("\n")
	sys.stdout.write("Usage: " + sys.argv[0] + " DocFilename OutFilename [Image]\n")
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

# Get Extractor and Convert Document
Filename = sys.argv[1]
OutFilename = sys.argv[2]
ImageName = None
if len(sys.argv) >= 4:
	ImageName = sys.argv[3]
ProcessFile(Filename, OutFilename, ImageName, sys.stderr)
