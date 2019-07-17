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
* Document Filters Example - Convert a document to a series of PNG images
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"

#include <iostream>
#include <exception>
#include <memory>
#include <sstream>

Perceptive::DocumentFilters DocumentFilters;

std::string generate_filename(const std::string& base, int page_num, const char* new_extension = ".png")
{
	std::string filename = base;

	std::string::size_type pos = filename.find_last_of(".");
	if (pos != std::string::npos && filename[pos] == '.')
		filename = filename.substr(0, pos);

	std::stringstream ss;
	ss << filename << "_page" << page_num << new_extension;
	return ss.str();
}

void process_file(const std::string &filename, std::string output_filename)
{
	if (output_filename.empty())
	{
		std::string::size_type p = filename.find_last_of("/\\");
		if (p != std::string::npos)
			output_filename = filename.substr(p + 1);
		else
			output_filename = filename;
	}


	std::cerr << "Processing (FILE): " << filename << std::endl;
	std::auto_ptr<Perceptive::Extractor> extractor(DocumentFilters.GetExtractor(filename));

	int DocType = extractor->getFileType();
	std::cerr << "DocType: " << DocType << ", " << Perceptive::GetFileTypeName(DocType)
		<< ", SupportsText: " << extractor->getSupportsText()
		<< ", SupportsSubFiles: " << extractor->getSupportsSubFiles() << std::endl;
	extractor->Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE);

	int pageNum = 1, pageCount = extractor->getPageCount();
	for (std::auto_ptr<Perceptive::Page> page(extractor->GetFirstPage()); page.get(); page.reset(extractor->GetNextPage()))
	{
		std::string filename = generate_filename(output_filename, pageNum);
		std::auto_ptr<Perceptive::Canvas> Canvas(DocumentFilters.MakeOutputCanvas(filename, IGR_DEVICE_IMAGE_PNG, ""));

		std::cerr << "Rendering Page " << pageNum++ << " of " << pageCount << " to " << filename << std::endl;
		Canvas->RenderPage(page.get());
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cerr << "Document Filters 11: ConvertDocumentToPNG C++ Example" << std::endl;
		std::cerr << "(c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename [OutFilename]" << std::endl;
		return 1;
	}

	try
	{
		DocumentFilters.Initialize("{ENTER LICENSE KEY}", ".");
		process_file(argv[1], argc > 2 ? argv[2] : "");
		return 0;
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
