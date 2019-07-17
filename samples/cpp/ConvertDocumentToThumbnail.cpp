/*
(c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/****************************************************************************
* Document Filters Example - Convert first page of a document to a PNG thumbnail
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"

#include <iostream>
#include <exception>
#include <memory>

Perceptive::DocumentFilters DocumentFilters;

void process_file(const std::string &filename, const std::string &out_filename)
{
	std::cerr << "Processing (FILE): " << filename << std::endl;
	std::auto_ptr<Perceptive::Extractor> extractor(DocumentFilters.GetExtractor(filename));

	int DocType = extractor->getFileType();
	std::cerr << "DocType: " << DocType << ", " << Perceptive::GetFileTypeName(DocType)
		<< ", SupportsText: " << extractor->getSupportsText()
		<< ", SupportsSubFiles: " << extractor->getSupportsSubFiles() << std::endl;

	// Open the document, but only require the 1st page the be layed out
	extractor->Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "LIMIT_PAGES=1");

	std::auto_ptr<Perceptive::Page> Page(extractor->GetFirstPage());

	// Create a PNG canvas, scaling the output to be 100 pixels wide
	std::auto_ptr<Perceptive::Canvas> Canvas(DocumentFilters.MakeOutputCanvas(out_filename, IGR_DEVICE_IMAGE_PNG, "GRAPHIC_WIDTH=100"));

	Canvas->RenderPage(Page.get());
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		std::cerr << "Document Filters 11: ConvertDocumentToThumbnail C++ Example" << std::endl;
		std::cerr << "(c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename OutFilename" << std::endl;
		return 1;
	}

	try
	{
		DocumentFilters.Initialize("{ENTER LICENSE KEY}", ".");
		process_file(argv[1], argv[2]);
		return 0;
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
