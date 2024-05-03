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
#* Document Filters Example - Convert a document to UTF-8 text.
#****************************************************************************/

import os, sys, argparse, DocumentFiltersSample
import shutil
from DocumentFilters import *

api = DocumentFilters()

def ProcessFile(filename, output, console, subFiles):
	def Walk(file, console, subFiles):
		MaxCharsPerGetText = 4096
		docType = file.getFileType()
		console.write("DocType: " + str(docType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME) 
			+ ", SupportsText: " + str(file.getSupportsText()) 
			+ ", SupportsSubFiles: " + str(file.getSupportsSubFiles()) + "\n")

		if file.getSupportsText():
			file.Open(IGR_BODY_AND_META, "")

			while not file.getEOF():
				output.write(file.GetText(MaxCharsPerGetText, stripControlCodes=True))

		if subFiles and file.getSupportsSubFiles():
			for child in file.SubFiles:
				with child:
					Walk(child, console.subFiles)

	with api.GetExtractor(filename) as file:
		Walk(file, console, subFiles)


try:
	parser = argparse.ArgumentParser(description='Convert Document to UTF8.')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='Filename of file to convert')
	parser.add_argument('-s', '--subfiles', dest='subfiles', action='store_const', const="True", help='Output subfiles')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')
	
	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")

	# Ensure the console is in binary mode (for 2.6 and 2.7 under Windows)
	if sys.platform == "win32":
		import os, msvcrt
		msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)    

	# Prepare and Initialize Engine
	DocumentFiltersSample.InitializeAPI(api, args)

	# Get Extractor and Convert Document
	ProcessFile(args.file, sys.stdout, sys.stderr, args.subfiles != None)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)
