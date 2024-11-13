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

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"
#include "DocumentFiltersSamples.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <memory>
#include <vector>

Perceptive::DocumentFilters DocumentFilters;

void process_file(const std::string &filename, const std::string &OutFilename, const std::string& ImageName)
{
	std::cerr << "Processing (FILE): " << filename << std::endl;
	try
	{
		std::unique_ptr<Perceptive::Extractor> Extractor(DocumentFilters.GetExtractor(filename));

		int doc_type = Extractor->getFileType();
		std::cerr << "DocType: " << doc_type << ", " << Perceptive::GetFileTypeName(doc_type)
			<< ", SupportsText: " << Extractor->getSupportsText()
			<< ", SupportsSubFiles: " << Extractor->getSupportsSubFiles() << std::endl;
		Extractor->Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE);

		std::unique_ptr<Perceptive::Canvas> Canvas(DocumentFilters.MakeOutputCanvas(OutFilename, IGR_DEVICE_IMAGE_PDF, ""));
		std::unique_ptr<Perceptive::Page> Page;

		size_t imageSize = 0;
		std::vector<uint8_t> imageData;
		if (!ImageName.empty())
		{
			// Load watermark image
			std::ifstream strm(ImageName.c_str(), std::ios::in | std::ios::binary);
			if (strm.good())
			{
				strm.seekg(0, std::ios::end);

				imageSize = static_cast<size_t>(strm.tellg());
				strm.seekg(0, std::ios::beg);
				
				imageData.resize(imageSize);
				strm.read((char*) &imageData[0], imageSize);
			}
		}

		int pageNum = 1;
		for (Page.reset(Extractor->GetFirstPage()); Page.get(); Page.reset(Extractor->GetNextPage()))
		{
			std::cerr << "Rendering Page " << pageNum++ << std::endl;
			Canvas->RenderPage(Page.get());
			int width = Page->getWidth();
			int height = Page->getHeight();

			Canvas->SetFont("Arial", 12, 0);
			int textHeight = Canvas->TextHeight("A") + 10;
			Canvas->SetOpacity(120);
			Canvas->SetBrush(0x101010, 0);
			Canvas->Rect(40, height - 40 - textHeight, width - 40, height - 40);
			Canvas->Ellipse(width - 80, height - 80, width - 20, height - 20);
			Canvas->SetBrush(0xFFFFFF, 0);
			Canvas->TextOut(50, height - 40 - textHeight + 5, "This document was converted using Document Filters 11");
			if (imageSize)
			{
				Canvas->SetOpacity(70);
				Canvas->DrawImage(40, 40, imageSize?&imageData[0]:NULL, imageSize, "image/png");
			}
		}
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
		std::cerr << "Document Filters: WatermarkDocument C++ Example" << std::endl;
		std::cerr << "(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
		std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
		std::cerr << "Usage: " << argv[0] << " DocFilename OutFilename [Image]" << std::endl;
		return 1;
	}

	try
	{
		DocumentFilters.Initialize(DocumentFiltersSamples::get_license_key(), ".");
		process_file(argv[1], argv[2], (argc > 3?argv[3]:""));
		return 0;
	}
	catch(std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
