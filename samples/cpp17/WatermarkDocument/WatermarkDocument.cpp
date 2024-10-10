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
* Document Filters Example - Watermark a document
****************************************************************************/

#include <DocumentFiltersObjects.h>
#include <DocumentFiltersSamples.h>
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>

namespace DF = Hyland::DocFilters;

struct options_t
{
	std::vector<std::string> filenames;
	std::string output_dir = ".";
	std::string license_key;
	std::string watermark_text;
	std::string watermark_image;
};


void process_file(DF::Api& api, const options_t& options, const std::filesystem::path& filename)
{
	std::cerr << "Processing " << filename.string() << std::endl;
	auto&& out_filename = (std::filesystem::path(options.output_dir) / (filename.filename().stem().string() + ".pdf")).string();

	// Get the extractor for the file...
	auto&& doc = api.GetExtractor(filename);

	// Setup a password prompt handler...
	DocumentFiltersSamples::handle_password_prompt(doc);

	// Open the document...
	doc.Open(DF::OpenMode::Paginated, IGR_BODY_AND_META);

	// Create the output canvas...
	auto&& canvas = api.MakeOutputCanvas(out_filename, DF::CanvasType::PDF, L"WATERMARK=" + DF::u8_to_w(options.watermark_text));

	std::vector<uint8_t> imageData;
	if (!options.watermark_image.empty())
	{
		// Load watermark image
		std::ifstream strm(options.watermark_image.c_str(), std::ios::in | std::ios::binary);
		if (strm.good())
		{
			strm.seekg(0, std::ios::end);

			size_t image_size = static_cast<size_t>(strm.tellg());
			strm.seekg(0, std::ios::beg);

			imageData.resize(image_size);
			strm.read((char*)&imageData[0], image_size);
		}
	}

	for (auto&& page : doc.pages())
	{
		auto page_num = page.getIndex() + 1;

		std::cerr << " - Rendering page " << page_num << " to " << out_filename << std::endl;

		canvas.RenderPage(page);
		if (imageData.size() > 0)
		{
			canvas.SetOpacity(70);
			canvas.DrawImage(40, 40, &imageData[0], imageData.size(), L"image/png");
		}
	}
}

int main(int argc, char* argv[])
{
	CLI::App app("Hyland Document Filters: JSON");

	try {
		options_t options;

		app.add_option("filename", options.filenames, "Files to convert")->required();
		app.add_option("-o,--output", options.output_dir, "Output directory");
		app.add_option("-l,--license", options.license_key, "License key for Document Filters");
		app.add_option("-w,--watermark-text", options.watermark_text, "Text to apply to document");
		app.add_option("--watermark-image", options.watermark_image, "Image to apply as watermark");
		app.parse(argc, argv);

		DF::Api api(DocumentFiltersSamples::get_license_key(options.license_key), ".");

		for (auto&& filename : options.filenames)
			process_file(api, options, filename);
	}
	catch (const CLI::ParseError& e) {
		return app.exit(e);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
