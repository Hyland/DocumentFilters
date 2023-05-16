/*
(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved.

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
* Document Filters Example - List a document's Pages and Words
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"
#include "DocumentFiltersSamples.h"

#include <iostream>
#include <exception>
#include <iomanip>

Perceptive::DocumentFilters DocumentFilters;

void process_file(const std::string &filename)
{
	std::cerr << "Processing (FILE): " << filename << std::endl;

	std::auto_ptr<Perceptive::Extractor> Extractor(DocumentFilters.GetExtractor(filename));

	int doc_type = Extractor->getFileType();
	std::cerr << "DocType: " << doc_type << ", " << Perceptive::GetFileTypeName(doc_type)
		<< ", SupportsText: " << Extractor->getSupportsText()
		<< ", SupportsSubFiles: " << Extractor->getSupportsSubFiles() << std::endl;
	Extractor->Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE);

	std::auto_ptr<Perceptive::Page> Page;
	std::auto_ptr<Perceptive::Word> Word;

	int pageNum = 1;
	for (Page.reset(Extractor->GetFirstPage()); Page.get(); Page.reset(Extractor->GetNextPage()))
	{
		std::cout << "Page "
			<< std::setw(16) << std::left << pageNum++
			<< "[width: " << std::setw(3) << Page->getWidth()
			<< "; height: " << std::setw(3) << Page->getHeight()
			<< "]" << std::endl;

		int wordNum = 1;
		for (Word.reset(Page->GetFirstWord()); Word.get(); Word.reset(Page->GetNextWord()))
		{
			std::cout
				<< std::setw(3) << std::right << wordNum++ << ". "
				<< std::setw(15) << std::left << Word->GetText()
				<< std::right
				<< " [x: " << std::setw(4) << Word->getX()
				<< "; y: " << std::setw(4) << Word->getY()
				<< "; width: " << std::setw(3) << Word->getWidth()
				<< "; height: " << std::setw(3) << Word->getHeight()
				<< "; character: " << std::setw(3) << Word->getCharacterOffset() << "]"
				<< std::endl;
		}
		std::cout << std::endl;
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cerr << "Document Filters: GetDocumentWords C++ Example" << std::endl;
		std::cerr << "(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename " << std::endl;
		return 1;
	}

	try
	{
		DocumentFilters.Initialize(DocumentFiltersSamples::get_license_key(), ".");
		process_file(argv[1]);
		return 0;
	}
	catch(std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
