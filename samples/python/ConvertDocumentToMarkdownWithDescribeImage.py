#
# (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.
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
#* Document Filters Example - Convert a document to UTF-8 text using a custom
#* DescribeImage callback. Instead of using a real AI model, this sample
#* demonstrates how to hook into the DescribeImage pipeline by providing a
#* fake description for every image encountered.
#****************************************************************************/

import os, sys, argparse, DocumentFiltersSample
from DocumentFilters import *

api = DocumentFilters()

def describe_image_callback(image):
    if image.SourceType != IGR_DESCRIBE_IMAGE_TYPE_PICTURE:
        return False
    image.AddText("FAKE SAMPLE DESCRIBE IMAGE RESULT", IGR_DESCRIBE_IMAGE_ADDTEXT_FLAGS_REPLACE)
    return True

def ProcessFile(filename, output, console):
    MaxCharsPerGetText = 4096

    with api.GetExtractor(filename) as file:
        docType = file.getFileType()
        console.write("DocType: " + str(docType) + ", " + file.getFileType(IGR_FORMAT_LONG_NAME)
            + ", SupportsText: " + str(file.getSupportsText()) + "\n")

        file.DescribeImageCallback = describe_image_callback

        if file.getSupportsText():
            file.Open(IGR_BODY_AND_META, "DESCRIBE_IMAGE=ON")

            while not file.getEOF():
                output.write(file.GetText(MaxCharsPerGetText, stripControlCodes=True))

try:
    parser = argparse.ArgumentParser(description='Convert Document to UTF8 with DescribeImage callback.')
    parser.add_argument('file', metavar='file', type=str, nargs='?', help='Filename of file to convert')
    parser.add_argument('-o', '--output', dest='output', action='store', help='Output file to create')
    parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
    parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')

    args = parser.parse_args()
    if args.file is None: raise Exception("filename cannot be empty")

    # Prepare and Initialize Engine
    DocumentFiltersSample.InitializeAPI(api, args)

    # Create or open existing file with proper encoding
    if args.output is None: args.output = os.path.basename(os.path.splitext(args.file)[0]) + ".txt"
    output = open(args.output, "w", encoding="utf-8")

    # Get Extractor and Convert Document
    ProcessFile(args.file, output, sys.stderr)
except Exception as e:
    sys.stderr.write(str(e) + "\n")
    exit(1)
