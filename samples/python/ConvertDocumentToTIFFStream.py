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
#* Document Filters Example - Convert a document to a multi-page TIFF
#****************************************************************************/

import os, sys, argparse, DocumentFiltersSample
import shutil
from DocumentFilters import *

api = DocumentFilters()

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

def ProcessFile(filename, outFilename, console, options = ""):
	console.write("Processing (FILE): " + filename + "\n")

	# Read the whole file a memory buffer
	with open(filename, 'rb') as file:
		memBuffer = file.read()

	# Get an Extractor based on the memory buffer
	with api.GetExtractor(memBuffer) as file:
		docType = file.getFileType()
		console.write("docType: " + str(docType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME) 
              + ", SupportsText: " + str(file.getSupportsText()) 
              + ", SupportsSubFiles: " + str(file.getSupportsSubFiles()) + "\n")
		file.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, options)
		
		memBuffer = io.BytesIO()
		memStream = CustomStream(memBuffer)
		with api.MakeOutputCanvas(memStream, IGR_DEVICE_IMAGE_TIF, "") as canvas:
			pageIndex = 0
			for page in file.Pages:
				with page:
					console.write(f"Rendering Page {pageIndex + 1}\n")
					canvas.RenderPage(page)
					pageIndex += 1

		memBuffer.seek(0)
		shutil.copyfileobj(memBuffer, outFilename)

		
try:
	parser = argparse.ArgumentParser(description='Convert Document to TIF.')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='Filename of file to convert')
	parser.add_argument('-o', '--output', dest='output', action='store', help='Output file to create')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')
	
	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".tif"

	# Ensure the console is in binary mode (for 2.6 and 2.7 under Windows)
	if sys.platform == "win32":
		import os, msvcrt
		msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)    

	# Prepare and Initialize Engine
	DocumentFiltersSample.InitializeAPI(api, args)

	# Get Extractor and Convert Document
	ProcessFile(args.file, args.output, sys.stderr, "")
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)
