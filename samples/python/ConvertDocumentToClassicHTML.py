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
#* Document Filters Example - Convert a document (and sub-documents) to classic HTML
#****************************************************************************/

from __future__ import unicode_literals
import os
import sys
import DocumentFiltersLicense
import DocumentFiltersUtils
from ISYS11dfpython import *

global DocumentFilters

def ProcessDocument(Extractor, OutFolder, ErrStream):
	MaxCharsPerGetText = 4096
	try:
		DocType = Extractor.getFileType()
		ErrStream.write("DocType: " + str(DocType) + ", " + Extractor.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(Extractor.getSupportsText()) 
              + ", SupportsSubFiles: " + str(Extractor.getSupportsSubFiles()) + "\n")

		if Extractor.getSupportsText():
			Extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_HTML, "")
			if not os.path.exists(OutFolder):
				os.makedirs(OutFolder)

			OutStream = open(OutFolder + os.sep + "index.html", "wb")
			while not(Extractor.getEOF()):
				S = Extractor.GetText(MaxCharsPerGetText).encode('utf8')
				OutStream.write(S)
			OutStream.close()
			ImgExtractor = Extractor.GetFirstImage()
			while not(ImgExtractor is None):
				ErrStream.write("Processing (IMAGE): " + ImgExtractor.getID() + " -> " + ImgExtractor.getName() + "\n")
				ImgExtractor.CopyTo(OutFolder + os.sep + ImgExtractor.getName())
				ImgExtractor.Close()
				ImgExtractor = Extractor.GetNextImage()

		if Extractor.getSupportsSubFiles():
			SubExtractor = Extractor.GetFirstSubFile()
			while not(SubExtractor is None):
				ErrStream.write("Processing (SUBFILE): " + SubExtractor.getID() + " -> " + SubExtractor.getName() + "\n")
				ProcessDocument(SubExtractor, OutFolder + os.sep + SubExtractor.getName(), ErrStream)
				SubExtractor = Extractor.GetNextSubFile()
	except Exception as e:
		ErrStream.write("ProcessDocument: " + str(e) + "\n")
	Extractor.Close()

def ProcessFile(Filename, OutFolder, ErrStream):
	ErrStream.write("Processing (FILE): " + Filename + "\n")
	try:
		Extractor = DocumentFilters.GetExtractor(open(Filename, 'rb'))
		ProcessDocument(Extractor, OutFolder, ErrStream)
	except Exception as e:
		ErrStream.write("ProcessFile: " + str(e) + "\n")
		
def ShowHelp():
	sys.stdout.write("Document Filters 11: ConvertDocumentToClassicHTML Python Example\n")
	sys.stdout.write("(c) 2019 Hyland Software, Inc.\n")
	sys.stdout.write("ALL RIGHTS RESERVED\n")
	sys.stdout.write("\n")
	sys.stdout.write("Usage: " + sys.argv[0] + " DocFileName OutFolder\n")
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
OutFolder = sys.argv[2]
ProcessFile(Filename, OutFolder, sys.stderr)
