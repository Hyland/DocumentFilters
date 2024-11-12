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
* Document Filters Example - Convert a document to TIFF
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
	bool multi_page = false;
};

void process_file(DF::Api& api, const options_t& options, const std::filesystem::path& filename)
{
	std::cerr << "Processing " << filename.string() << std::endl;

	auto&& doc = api.GetExtractor(filename);

	// Setup a password prompt handler...
	DocumentFiltersSamples::handle_password_prompt(doc);

	// Open the document...
	doc.Open(DF::OpenMode::Paginated);

	DF::Canvas canvas;
	std::string out_filename;
	
	if (options.multi_page)
	{
		out_filename = (std::filesystem::path(options.output_dir) / (filename.filename().stem().string() + ".tiff")).string();
		canvas = api.MakeOutputCanvas(out_filename, DF::CanvasType::TIF);
	}

	for (auto&& page : doc.pages())
	{
		auto page_num = page.getIndex() + 1;
		
		if (!options.multi_page)
		{
			out_filename = (std::filesystem::path(options.output_dir) / (filename.filename().stem().string() + "_page_" + std::to_string(page_num) + ".tiff")).string();
			canvas = api.MakeOutputCanvas(out_filename, DF::CanvasType::TIF);
		}
		std::cerr << " - Rendering page " << page_num << " to " << out_filename << std::endl;

		canvas.RenderPage(page);
	}
}

int main(int argc, char* argv[])
{
	CLI::App app("Hyland Document Filters: ConvertDocumentToTIFF");

	try {
		options_t options;

		app.add_option("filename", options.filenames, "Files to convert")->required();
		app.add_option("-o,--output", options.output_dir, "Output directory");
		app.add_option("-l,--license", options.license_key, "License key for Document Filters");
		app.add_flag("-m,--multi-page", options.multi_page, "Render all pages to a single file");
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
