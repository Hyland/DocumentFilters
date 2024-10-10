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
* Document Filters Example - Convert a document to JSON
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
	bool format_json = false;
	bool include_bookmarks = true;
	bool include_bounds = true;
	bool include_forms = true;
	bool include_image_data = true;
	bool include_images = true;
	bool include_metadata = true;
	bool include_styles = true;
	bool include_whitespace = false;
	bool include_words = false;
};


void process_file(DF::Api& api, const options_t& options, const std::filesystem::path& filename)
{
	std::cerr << "Processing " << filename.string() << std::endl;
	auto&& out_filename = (std::filesystem::path(options.output_dir) / (filename.filename().stem().string() + ".json")).string();

	std::string optstring;
	optstring += ";JSON_FORMAT_OUTPUT=" + std::string(options.format_json ? "on" : "off");
	optstring += ";JSON_INCLUDE_BOOKMARKS=" + std::string(options.include_bookmarks ? "on" : "off");
	optstring += ";JSON_INCLUDE_BOUNDS=" + std::string(options.include_bounds ? "on" : "off");
	optstring += ";JSON_INCLUDE_FORMS=" + std::string(options.include_forms ? "on" : "off");
	optstring += ";JSON_INCLUDE_IMAGEDATA=" + std::string(options.include_image_data ? "on" : "off");
	optstring += ";JSON_INCLUDE_IMAGES=" + std::string(options.include_images ? "on" : "off");
	optstring += ";JSON_INCLUDE_METADATA=" + std::string(options.include_metadata ? "on" : "off");
	optstring += ";JSON_INCLUDE_STYLES=" + std::string(options.include_styles ? "on" : "off");
	optstring += ";JSON_INCLUDE_WHITESPACE=" + std::string(options.include_whitespace ? "on" : "off");
	optstring += ";JSON_INCLUDE_WORDS=" + std::string(options.include_words ? "on" : "off");

	auto&& doc = api.GetExtractor(filename);

	// Setup a password prompt handler...
	DocumentFiltersSamples::handle_password_prompt(doc);

	// Open the document...
	doc.Open(DF::OpenMode::Paginated, IGR_BODY_AND_META, DF::u8_to_w(optstring));

	// Create the output canvas...
	auto&& canvas = api.MakeOutputCanvas(out_filename, DF::CanvasType::JSON);

	for (auto&& page : doc.pages())
	{
		auto page_num = page.getIndex() + 1;

		std::cerr << " - Rendering page " << page_num << " to " << out_filename << std::endl;

		canvas.RenderPage(page);
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
		app.add_flag("-p,--pretty", options.format_json, "Format the JSON output with newlines and indentation");
		app.add_flag("--bounds,!--no-bounds", options.include_bounds, "Include bounds in JSON output");
		app.add_flag("--forms,!--no-forms", options.include_forms, "Include forms in JSON output");
		app.add_flag("--image-data,!--no-image-data", options.include_image_data, "Include image data in JSON output");
		app.add_flag("--images,!--no-images", options.include_images, "Include images in markdown output");
		app.add_flag("--metadata,!--no-metadata", options.include_metadata, "Include metadata in JSON output");
		app.add_flag("--styles,!--no-styles", options.include_styles, "Include styles in JSON output");
		app.add_flag("--whitespace,!--no-whitespace", options.include_whitespace, "Include whitespace in JSON output");
		app.add_flag("--words,!--no-words", options.include_words, "Include words in JSON output");
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
