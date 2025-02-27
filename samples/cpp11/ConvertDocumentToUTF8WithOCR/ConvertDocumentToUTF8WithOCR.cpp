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
* Document Filters Example - Convert a document (and sub-documents) to UTF8 text
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"
#include "DocumentFiltersSamples.h"

#include <iostream>
#include <exception>
#include <fstream>

Perceptive::DocumentFilters DocumentFilters;

void process_document(Perceptive::Extractor *Extractor, std::ostream& dest)
{
	static const int MaxCharsPerGetText = 4096;
	try
	{
		// Extractor closes stream when out of scope
		Extractor->Open(IGR_BODY_ONLY, "OCR=ON;OCR_REORIENT_PAGES=ON");
		int DocType = Extractor->getFileType();
		std::cerr << "DocType: " << DocType << ", " << Perceptive::GetFileTypeName(DocType)
			<< ", SupportsText: " << Extractor->getSupportsText() << std::endl;
		if (Extractor->getSupportsText())
		{
			while (!Extractor->getEOF())
			{
				std::string text = Extractor->PrepareText(Extractor->GetText(MaxCharsPerGetText));
				dest.write(text.c_str(), text.size());
			}
			std::cout << std::endl;
		}
	}
	catch(std::exception &e)
	{
		std::cerr << "ProcessDocument: " << e.what() << std::endl;
	}
}

void process_file(const std::string &filename, const std::string& dest_file)
{
	static const uint8_t byte_order_marker[] = { 0xEF, 0xBB, 0xBF };

	std::cerr << "Processing (FILE): " << filename << std::endl;

	std::ofstream dest;
	dest.open(dest_file.c_str(), std::ios::binary | std::ios::out);
	dest.write((const char*) &byte_order_marker[0], sizeof(byte_order_marker));

	std::unique_ptr<Perceptive::Extractor> Extractor(DocumentFilters.GetExtractor(new Perceptive::FileStream(filename)));
	process_document(Extractor.get(), dest);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
	SetConsoleOutputCP(65001);
#endif

	if (argc < 3)
	{
		std::cerr << "Document Filters: ConvertDocumentToUTF8WithOCR C++ Example" << std::endl;
		std::cerr << "(c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename OutputFile" << std::endl;
		return 1;
	}

	try
	{
		DocumentFilters.Initialize(DocumentFiltersSamples::get_license_key(), ".");
		process_file(argv[1], argv[2]);
		return 0;
	}
	catch(std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
