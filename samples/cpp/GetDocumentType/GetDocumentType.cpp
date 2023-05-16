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
* Document Filters Example - Identify a document's type (format)
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"
#include "DocumentFiltersSamples.h"

#include <iostream>
#include <exception>
#include <memory>

Perceptive::DocumentFilters DocumentFilters;

void process_file(const std::string &filename)
{
	std::cerr << "Processing: " << filename << std::endl;
	
	std::auto_ptr<Perceptive::Extractor> extractor(DocumentFilters.GetExtractor(filename));

	int doc_type = extractor->getFileType();
	std::cout << "DocType: " << doc_type << ", " << Perceptive::GetFileTypeName(doc_type)
		<< ", SupportsText: " << extractor->getSupportsText()
		<< ", SupportsSubFiles: " << extractor->getSupportsSubFiles() << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cerr << "Document Filters: GetDocumentType C++ Example" << std::endl;
		std::cerr << "(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename" << std::endl;
		return 1;
	}

	int exit_code = 0;
	try
	{
		DocumentFilters.Initialize(DocumentFiltersSamples::get_license_key(), ".");
		process_file(argv[1]);
	}
	catch(std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		exit_code = 1;
	}
	return exit_code;
}
