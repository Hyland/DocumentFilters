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
* Document Filters Example - Convert a document (and sub-documents) to XML
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"
#include "DocumentFiltersSamples.h"

#include <iostream>
#include <exception>
#include <memory>

Perceptive::DocumentFilters DocumentFilters;

void process_document(Perceptive::Extractor *Extractor)
{
	const int MaxCharsPerGetText = 4096;

	int DocType = Extractor->getFileType();
	std::cerr << "DocType: " << DocType << ", " << Perceptive::GetFileTypeName(DocType)
		<< ", SupportsText: " << Extractor->getSupportsText()
		<< ", SupportsSubFiles: " << Extractor->getSupportsSubFiles() << std::endl;

	if (Extractor->getSupportsText())
	{
		// We start by gathering up the metadata (if any)...
		std::string MetaStr = "";
		size_t StartMetadata = std::string::npos;
		size_t EndMetadata = std::string::npos;
		while (!Extractor->getEOF() && (EndMetadata == std::string::npos))
		{
			MetaStr = MetaStr + Extractor->GetText(MaxCharsPerGetText);
			if (StartMetadata == std::string::npos)
				StartMetadata = MetaStr.find(IGR_CHAR_START_META);
			// If we dont see a "start of metadata" marker in the first GetText(), document has no metadata
			if (StartMetadata == std::string::npos)
				break;
			// Keep going until we see the "end of metadata" marker (or end of document)
			EndMetadata = MetaStr.find(IGR_CHAR_END_META);
		}
		std::string BodyStr = MetaStr;
		if (EndMetadata != std::string::npos)
		{
			// We have metadata, now parse the paragraphs into field/value pairs...
			BodyStr = MetaStr.substr(EndMetadata + 1);
			MetaStr = MetaStr.substr(StartMetadata + 1, EndMetadata);
			Perceptive::Metadata Metadata = Extractor->ParseMetadata(MetaStr);
			// Output the metadata as a set of meta tags with name/value attributes...
			for (Perceptive::Metadata::iterator i = Metadata.begin(); i != Metadata.end(); i++)
				std::cout << "<meta name=\"" << Perceptive::HTMLEncode(i->first) << "\" value=\"" << Perceptive::HTMLEncode(i->second) << "\" />" << std::endl;
		}
		// Output the body as a CDATA section...
		std::cout << "<body>" << std::endl << "<![CDATA[" << std::endl;
		std::cout << Extractor->PrepareText(BodyStr);
		while (!Extractor->getEOF())
		{
			std::string S = Extractor->GetText(MaxCharsPerGetText);
			// Make sure the body content does not inadvertently close off the CDATA section
			S = Perceptive::ReplaceAll(S, "]]>", "]]><![CDATA[");
			std::cout << Extractor->PrepareText(S);
		}
		std::cout << std::endl << "]]>" << std::endl << "</body>" << std::endl;
	}
	if (Extractor->getSupportsSubFiles())
	{
		// Recursively process any sub-documents / attachments...
		std::unique_ptr<Perceptive::SubFile> SubExtractor(Extractor->GetFirstSubFile());
		while (SubExtractor.get())
		{
			std::cerr << "Processing (SUBFILE): " << SubExtractor->getID() << " -> " << SubExtractor->getName() << std::endl;
			std::cout << "<subdocument>" << std::endl;
			process_document(SubExtractor.get());
			std::cout << "</subdocument>" << std::endl;
			SubExtractor = std::unique_ptr<Perceptive::SubFile>(Extractor->GetNextSubFile());
		}
	}
}

void process_file(const std::string &filename)
{
	std::cerr << "Processing (FILE): " << filename << std::endl;
	std::unique_ptr<Perceptive::Extractor> extractor(DocumentFilters.GetExtractor(new Perceptive::FileStream(filename)));

	std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
	std::cout << "<document>" << std::endl;
	process_document(extractor.get());
	std::cout << "</document>" << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cerr << "Document Filters: ConvertDocumentToXML C++ Example" << std::endl;
		std::cerr << "(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename" << std::endl;
		return 1;
	}

	try
	{
		DocumentFilters.Initialize(DocumentFiltersSamples::get_license_key(), ".");
		process_file(argv[1]);
		return 0;
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
