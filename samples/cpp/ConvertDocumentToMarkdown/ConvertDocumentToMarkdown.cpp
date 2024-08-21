/*
(c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.

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
* Document Filters Example - Convert a document to Markdown
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"
#include "DocumentFiltersSamples.h"

#include <iostream>
#include <exception>
#include <memory>
#include <fstream>

Perceptive::DocumentFilters DocumentFilters;

void process_file(const std::string &filename, const std::string& outfile)
{
	std::cerr << "Processing (FILE): " << filename << std::endl;

	std::unique_ptr<Perceptive::Extractor> extractor(DocumentFilters.GetExtractor(filename));

	int DocType = extractor->getFileType();
	std::cerr << "DocType: " << DocType << ", " << Perceptive::GetFileTypeName(DocType)
		<< ", SupportsText: " << extractor->getSupportsText()
		<< ", SupportsSubFiles: " << extractor->getSupportsSubFiles() << std::endl;
	extractor->Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE);

	std::unique_ptr<Perceptive::Canvas> canvas(DocumentFilters.MakeOutputCanvas(outfile, IGR_DEVICE_MARKDOWN, ""));

	int pageNum = 1, pageCount = extractor->getPageCount();
	for (std::unique_ptr<Perceptive::Page> page(extractor->GetFirstPage()); page.get(); page.reset(extractor->GetNextPage()))
	{
		std::cerr << "Rendering Page " << pageNum++ << " of " << pageCount << std::endl;
		canvas->RenderPage(page.get());
	}
	canvas->Close();
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		std::cerr << "Document Filters: ConvertDocumentToMarkdown C++ Example" << std::endl;
		std::cerr << "(c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename OutFilename" << std::endl;
		return 1;
	}

	try
	{
		DocumentFilters.Initialize(DocumentFiltersSamples::get_license_key(), ".");
		process_file(argv[1], argv[2]);
		return 0;
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
