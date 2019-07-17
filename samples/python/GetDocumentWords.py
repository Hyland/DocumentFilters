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
#* Document Filters Example - List a document's Pages and Words
#****************************************************************************/

from __future__ import unicode_literals
import sys
import DocumentFiltersLicense
import DocumentFiltersUtils
from ISYS11dfpython import *

global DocumentFilters

def ProcessFile(Filename, ErrStream):
	ErrStream.write("Processing (FILE): " + Filename + "\n")

	try:
		Extractor = DocumentFilters.GetExtractor(DocumentFiltersUtils.ToUnicodeString(Filename))
		DocType = Extractor.getFileType()
		ErrStream.write("DocType: " + str(DocType) + ", " + Extractor.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(Extractor.getSupportsText()) 
              + ", SupportsSubFiles: " + str(Extractor.getSupportsSubFiles()) + "\n")
		Extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)
		
		for pageIndex in range(Extractor.GetPageCount()):
			try:
				Page = Extractor.GetPage(pageIndex)
				progressString = ("Page " + str(pageIndex + 1) + 
								"[width" + str(Page.GetWidth()) + 
								"; height: " + str(Page.GetHeight()) + "]\n")
				DocumentFiltersUtils.OutputStringToConsole(progressString)

				wordIndex = 1
				Word = Page.GetFirstWord()
				while not (Word is None):
					outputString = ( 
						str(wordIndex) + ". " +
						Word.GetText() +
						" [x: " + str(Word.GetX()) +
						"; y: " + str(Word.GetY()) + 
						"; width: " + str(Word.GetWidth()) + 
						"; height: " + str(Word.GetHeight()) +
						"; character: " + str(Word.GetCharacterOffset()) + "]\n")
					DocumentFiltersUtils.OutputStringToConsole(outputString)
					wordIndex = wordIndex + 1
					Word = Page.GetNextWord()
				sys.stdout.write("\n")
			finally:
				Page.Close()
	except Exception as e:
		ErrStream.write("ProcessFile: " + str(e) + "\n")
	finally:
		Extractor.Close()

def ShowHelp():
	sys.stdout.write("Document Filters 11: GetDocumentWords Python Example\n")
	sys.stdout.write("(c) 2019 Hyland Software, Inc.\n")
	sys.stdout.write("ALL RIGHTS RESERVED\n")
	sys.stdout.write("\n")
	sys.stdout.write("Usage: " + sys.argv[0] + " DocFileName\n")
	sys.exit(0)

if len(sys.argv) < 2:
	ShowHelp()

# Prepare and Initialize Engine
DocumentFilters = DocumentFilters()
try:
	DocumentFilters.Initialize(DocumentFiltersLicense.LICENSE_KEY, ".")
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)

# Get Extractor and Process Document
Filename = sys.argv[1]
ProcessFile(Filename, sys.stderr)
