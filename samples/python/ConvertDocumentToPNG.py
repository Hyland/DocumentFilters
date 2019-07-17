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
#* Document Filters Example - Convert a document to a series of PNG images
#****************************************************************************/

from __future__ import unicode_literals
import os
import sys
import DocumentFiltersLicense
import DocumentFiltersUtils
from ISYS11dfpython import *

global DocumentFilters

def generateFilename(base, pageNum):
	Filename = os.path.basename(os.path.splitext(base)[0])
	Directory = os.path.dirname(base)
	if Directory and Directory[-1:] != os.sep:
		Directory += os.sep
	Filename = Directory + Filename + "_page" + str(pageNum) + ".png"
	return Filename

def ProcessFile(Filename, OutFilename, ErrStream):
	ErrStream.write("Processing (FILE): " + Filename + "\n")

	try:
		Extractor = DocumentFilters.GetExtractor(DocumentFiltersUtils.ToUnicodeString(Filename))
		DocType = Extractor.getFileType()		
		ErrStream.write("DocType: " + str(DocType) + ", " + Extractor.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(Extractor.getSupportsText()) 
              + ", SupportsSubFiles: " + str(Extractor.getSupportsSubFiles()) + "\n")
		Extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

		for pageIndex in range(Extractor.GetPageCount()):
			OutFilenameGenerated = generateFilename(OutFilename, pageIndex + 1)
			try:
				Page = Extractor.GetPage(pageIndex)
				Canvas = DocumentFilters.MakeOutputCanvas(OutFilenameGenerated, IGR_DEVICE_IMAGE_PNG, "")
				ErrStream.write("Rendering Page " + str(pageIndex + 1) + "\n")
				Canvas.RenderPage(Page)
			finally:
				Page.Close()
				Canvas.Close()

	except Exception as e:
		ErrStream.write("ProcessFile: " + str(e) + "\n")
	finally:
		Extractor.Close()

def ShowHelp():
	sys.stdout.write("Document Filters 11: ConvertDocumentToPNG Python Example\n")
	sys.stdout.write("(c) 2019 Hyland Software, Inc.\n")
	sys.stdout.write("ALL RIGHTS RESERVED\n")
	sys.stdout.write("\n")
	sys.stdout.write("Usage: " + sys.argv[0] + " DocFileName OutFilename\n")
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
ProcessFile(Filename, OutFilename, sys.stderr)
