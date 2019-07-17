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
* Document Filters Example - Convert a document (and sub-documents) to classic HTML
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"

#include <fstream>
#include <iostream>
#include <exception>
#include <memory>

Perceptive::DocumentFilters DocumentFilters;

void ProcessDocument(Perceptive::Extractor *Extractor, const std::string &OutFolder)
{
	const int MaxCharsPerGetText = 4096;
	
	int DocType = Extractor->getFileType();
	std::cerr << "DocType: " << DocType << ", " << Perceptive::GetFileTypeName(DocType)
		<< ", SupportsText: " << Extractor->getSupportsText()
		<< ", SupportsSubFiles: " << Extractor->getSupportsSubFiles() << std::endl;
	if (Extractor->getSupportsText())
	{
		Extractor->Open(IGR_BODY_AND_META | IGR_FORMAT_HTML, "");
		if (!Perceptive::CreateFolder(OutFolder))
			throw Perceptive::Exception("Cannot create output folder: " + OutFolder);
		std::ofstream OutStream;
		OutStream.open(Perceptive::JoinFilename(OutFolder, "index.html").c_str());
		while (!Extractor->getEOF())
		{
			std::string S = Extractor->GetText(MaxCharsPerGetText);
			OutStream << Extractor->PrepareText(S);
		}
		OutStream << std::endl;
		OutStream.close();
		
		std::auto_ptr<Perceptive::SubFile> ImgExtractor(Extractor->GetFirstImage());
		while (ImgExtractor.get())
		{
			std::cerr << "Processing (IMAGE): " << ImgExtractor->getID() << " -> " << ImgExtractor->getName() << std::endl;
			ImgExtractor->CopyTo(Perceptive::JoinFilename(OutFolder, Perceptive::MakeSafeFilename(ImgExtractor->getName())));
			ImgExtractor = std::auto_ptr<Perceptive::SubFile>(Extractor->GetNextImage());
		}
	}
	if (Extractor->getSupportsSubFiles())
	{
		std::auto_ptr<Perceptive::SubFile> SubExtractor(Extractor->GetFirstSubFile());
		while (SubExtractor.get())
		{
			std::cerr << "Processing (SUBFILE): " << SubExtractor->getID() << " -> " << SubExtractor->getName() << std::endl;
			ProcessDocument(SubExtractor.get(), Perceptive::JoinFilename(OutFolder, Perceptive::MakeSafeFilename(SubExtractor->getName())));
			SubExtractor = std::auto_ptr<Perceptive::SubFile>(Extractor->GetNextSubFile());
		}
	}
}

void process_file(const std::string &filename, const std::string &OutFolder)
{
	std::cerr << "Processing (FILE): " << filename << std::endl;
	try
	{
		std::auto_ptr<Perceptive::Extractor> Extractor(DocumentFilters.GetExtractor(new Perceptive::FileStream(filename)));
		ProcessDocument(Extractor.get(), OutFolder);
	}
	catch(std::exception &e)
	{
		std::cerr << "process_file: " << e.what() << std::endl;
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		std::cerr << "Document Filters 11: ConvertDocumentToClassicHTML C++ Example" << std::endl;
		std::cerr << "(c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename OutFolder" << std::endl;
		return 1;
	}

	try
	{
		DocumentFilters.Initialize("{ENTER LICENSE KEY}", ".");
		process_file(argv[1], argv[2]);
		return 0;
	}
	catch(std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
