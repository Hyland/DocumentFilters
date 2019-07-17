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
#* Document Filters Example - Convert a document to a multi-page TIFF
#****************************************************************************/

from __future__ import unicode_literals
from io import BytesIO
import shutil
import sys
import DocumentFiltersLicense
import DocumentFiltersUtils
from ISYS11dfpython import *

global DocumentFilters

# Custom Stream class
class CustomStream(IGRStream): 
	def __init__(self, py_stream): 
		super(CustomStream, self).__init__() 
		self.__py_stream = py_stream 
		
	def Read(self, size, igr_stream_data_dest): 
		data = self.__py_stream.read(size) 
		igr_stream_data_dest.write(data) 
		return len(data) 
		
	def Seek(self, offset, origin): 
		self.__py_stream.seek(offset, origin) 
		return self.__py_stream.tell() 
		
	def Write(self, bytes): 
		self.__py_stream.write(bytes) 
		return len(bytes)


		
def ProcessFile(Filename, OutStream, ErrStream):
	ErrStream.write("Processing (FILE): " + Filename + "\n")

	try:
		Extractor = DocumentFilters.GetExtractor(DocumentFiltersUtils.ToUnicodeString(Filename))
		DocType = Extractor.getFileType()
		ErrStream.write("DocType: " + str(DocType) + ", " + Extractor.getFileType(IGR_FORMAT_LONG_NAME) 
			  + ", SupportsText: " + str(Extractor.getSupportsText()) 
			  + ", SupportsSubFiles: " + str(Extractor.getSupportsSubFiles()) + "\n")
		Extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)
		
		# Make the Canvas object write to a memory buffer
		MemBuffer = io.BytesIO()
		MemStream = CustomStream(MemBuffer)
		Canvas = DocumentFilters.MakeOutputCanvas(MemStream, IGR_DEVICE_IMAGE_TIF, "")

		try:
			for pageIndex in range(Extractor.GetPageCount()):
				ErrStream.write("Rendering Page " + str(pageIndex + 1) + "\n")
				try:
					Page = Extractor.GetPage(pageIndex)
					Canvas.RenderPage(Page)
				finally:
					Page.Close()
		finally:
			Canvas.Close()
			
		# Copy the memory buffer to the Output Stream
		MemBuffer.seek(0)
		shutil.copyfileobj(MemBuffer, OutStream)
			
			
	except Exception as e:
		raise
		ErrStream.write("ProcessFile: " + str(e) + "\n")
	finally:
		Extractor.Close()

def ShowHelp():
	sys.stdout.write("Document Filters 11: ConvertDocumentToTIFFStream Python Example\n")
	sys.stdout.write("(c) 2019 Hyland Software, Inc.\n")
	sys.stdout.write("ALL RIGHTS RESERVED\n")
	sys.stdout.write("\n")
	sys.stdout.write("Usage: " + sys.argv[0] + " DocFileName [> OutFilename]\n")
	sys.exit(0)

if len(sys.argv) != 2:
	ShowHelp()

# Prepare and Initialize Engine
DocumentFilters = DocumentFilters()
try:
	DocumentFilters.Initialize(DocumentFiltersLicense.LICENSE_KEY, ".")
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)

# Ensure the console is in binary mode (for 2.6 and 2.7 under Windows)
if sys.platform == "win32":
	import os, msvcrt
	msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)    
	
# Get Extractor and Convert Document
Filename = sys.argv[1]
OutStream = sys.stdout.buffer if sys.version_info[0] >= 3 else sys.stdout
ProcessFile(Filename, OutStream, sys.stderr)
