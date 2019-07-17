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
import sys
import DocumentFiltersLicense
import DocumentFiltersUtils
from ISYS11dfpython import *

global DocumentFilters

def ProcessDocument(Extractor, OutStream, ErrStream):
	MaxCharsPerGetText = 4096
	try:
		DocType = Extractor.getFileType()
		ErrStream.write("DocType: " + str(DocType) + ", " + Extractor.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(Extractor.getSupportsText()) 
              + ", SupportsSubFiles: " + str(Extractor.getSupportsSubFiles()) + "\n")

		if Extractor.getSupportsText():
			while not(Extractor.getEOF()):
				S = Extractor.GetText(MaxCharsPerGetText)
				DocumentFiltersUtils.OutputStringToConsole(S)

		if Extractor.getSupportsSubFiles():
			SubExtractor = Extractor.GetFirstSubFile()
			while not(SubExtractor is None):
				ErrStream.write("Processing (SUBFILE): " + SubExtractor.getID() + " -> " + SubExtractor.getName() + "\n")
				ProcessDocument(SubExtractor, OutStream, ErrStream)
				SubExtractor = Extractor.GetNextSubFile()

	except Exception as e:
		ErrStream.write("ProcessDocument: " + str(e) + "\n")
	Extractor.Close()

def ProcessFile(Filename, OutStream, ErrStream):
	ErrStream.write("Processing (FILE): " + Filename + "\n")
	try:
		input_file = open(Filename, "rb")
		input_stream = IGRStreamBridge(input_file)
		Extractor = DocumentFilters.GetExtractor(input_stream)
		ProcessDocument(Extractor, OutStream, ErrStream)
	except Exception as e:
		ErrStream.write("ProcessFile: " + str(e) + "\n")

def ShowHelp():
	sys.stdout.write("Document Filters 11: ConvertDocumentToUTF8 Python Example\n")
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

# Get Extractor and Convert Document
Filename = sys.argv[1]
# write BOM
sys.stdout.write(chr(int("0xEF",16))+chr(int("0xBB",16))+chr(int("0xBF",16)))
# process file
ProcessFile(Filename, sys.stdout, sys.stderr)
