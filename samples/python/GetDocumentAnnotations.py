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
#* Document Filters Example - Extract annotations for a document
#****************************************************************************/

import os, sys, argparse, DocumentFiltersLicense
import shutil
from DocumentFilters import *

api = DocumentFilters()

def ProcessFile(filename, console):
	console.write("Processing (FILE): " + filename + "\n")
	with api.GetExtractor(filename) as file:
		file.Open(IGR_FORMAT_IMAGE)

		result = {}

		page: DocumentFilters.Page
		for page in file.Pages:
			with page:
				# console.write(f"page {page.PageIndex + 1}\n")

				result[page.PageIndex + 1] = []

				anno: DocumentFilters.Annotation
				for anno in page.Annotations:
					result[page.PageIndex + 1].append(anno)

		console.write(json.dumps(result, indent=2, cls=DocumentFilters.Annotations.Encoder) + "\n")

try:
	parser = argparse.ArgumentParser(description='Get Document Type')
	parser.add_argument('file', metavar='file', type=str, nargs='?', help='filename of file to convert')
	parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
	parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')
	
	args = parser.parse_args()
	if args.file is None: raise Exception("filename cannot be empty")
	if args.license_key is None: args.license_key = DocumentFiltersLicense.LICENSE_KEY
	if args.library_path is None: args.library_path = os.environ.get("DF_PATH")

	# Prepare and Initialize Engine
	api.Initialize(args.license_key, ".", args.library_path)

	# Get Extractor and Convert Document
	ProcessFile(args.file, sys.stdout)
except Exception as e:
	sys.stderr.write(str(e) + "\n")
	exit(1)
