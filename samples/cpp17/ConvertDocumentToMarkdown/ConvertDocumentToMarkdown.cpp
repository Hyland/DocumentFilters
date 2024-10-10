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
* Document Filters Example - Convert a document to Markdown
****************************************************************************/

#include <DocumentFiltersObjects.h>
#include <DocumentFiltersSamples.h>
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>

namespace DF = Hyland::DocFilters;

struct options_t
{
	enum class metadata_format_t
	{
		none,
		comments,
		json,
		yaml,
		toml,
	};
	enum class heading_style_t
	{
		atx,
		setext,
	};
	enum class table_style_t
	{
		pipe,
		grid,
		html,
		pipe_with_html
	};
	std::vector<std::string> filenames;
	std::string output_dir = ".";
	std::string license_key;
	std::string markdown_flavor;
	metadata_format_t metadata_format = metadata_format_t::comments;
	heading_style_t heading_style = heading_style_t::atx;
	table_style_t simple_table_style = table_style_t::pipe;
	table_style_t complex_table_style = table_style_t::html;
	bool include_fields = true;
	bool include_images = true;
	bool include_headers = false;
	bool include_footers = false;
	bool include_bookmarks = true;
};
static const std::map<std::string, options_t::metadata_format_t> metadata_format_map{
	{"none", options_t::metadata_format_t::none},
	{"comments", options_t::metadata_format_t::comments},
	{"json", options_t::metadata_format_t::json},
	{"yaml", options_t::metadata_format_t::yaml},
	{"toml", options_t::metadata_format_t::toml},
};
static const std::map<std::string, options_t::heading_style_t> heading_style_map{
	{"atx", options_t::heading_style_t::atx},
	{"setext", options_t::heading_style_t::setext},
};
static const std::map<std::string, options_t::table_style_t> table_style_map{
	{"pipe", options_t::table_style_t::pipe},
	{"grid", options_t::table_style_t::grid},
	{"html", options_t::table_style_t::html},
	{"pipe_with_html", options_t::table_style_t::pipe_with_html},
};

void process_file(DF::Api& api, const options_t& options, const std::filesystem::path& filename)
{
	std::cerr << "Processing " << filename.string() << std::endl;
	auto&& out_filename = (std::filesystem::path(options.output_dir) / (filename.filename().stem().string() + ".md")).string();

	std::string optstring;
	if (options.metadata_format != options_t::metadata_format_t::none)
		optstring += ";MARKDOWN_INCLUDE_METADATA=on";
	optstring += ";MARKDOWN_METADATA_FORMAT=" + std::find_if(metadata_format_map.begin(), metadata_format_map.end(), [&](const auto& p) { return p.second == options.metadata_format; })->first;
	optstring += ";MARKDOWN_HEADERS_STYLE=" + std::find_if(heading_style_map.begin(), heading_style_map.end(), [&](const auto& p) { return p.second == options.heading_style; })->first;
	optstring += ";MARKDOWN_SIMPLE_TABLE_STYLE=" + std::find_if(table_style_map.begin(), table_style_map.end(), [&](const auto& p) { return p.second == options.simple_table_style; })->first;
	optstring += ";MARKDOWN_COMPLEX_TABLE_STYLE=" + std::find_if(table_style_map.begin(), table_style_map.end(), [&](const auto& p) { return p.second == options.complex_table_style; })->first;
	optstring += options.include_images ? ";MARKDOWN_INCLUDE_IMAGES=on" : ";MARKDOWN_INCLUDE_IMAGES=off";
	optstring += options.include_fields ? ";MARKDOWN_INCLUDE_FIELDS=on" : ";MARKDOWN_INCLUDE_FIELDS=off";
	optstring += options.include_headers ? ";MARKDOWN_INCLUDE_HEADERS=on" : ";MARKDOWN_INCLUDE_HEADERS=off";
	optstring += options.include_footers ? ";MARKDOWN_INCLUDE_FOOTERS=on" : ";MARKDOWN_INCLUDE_FOOTERS=off";
	optstring += options.include_bookmarks ? ";MARKDOWN_INCLUDE_BOOKMARKS=on" : ";MARKDOWN_INCLUDE_BOOKMARKS=off";

	auto&& doc = api.GetExtractor(filename);

	// Setup a password prompt handler...
	DocumentFiltersSamples::handle_password_prompt(doc);

	// Open the document...
	doc.Open(DF::OpenMode::Paginated, IGR_BODY_AND_META, DF::u8_to_w(optstring));

	// Create the output canvas...
	auto&& canvas = api.MakeOutputCanvas(out_filename, DF::CanvasType::MARKDOWN);

	for (auto&& page : doc.pages())
	{
		auto page_num = page.getIndex() + 1;

		std::cerr << " - Rendering page " << page_num << " to " << out_filename << std::endl;

		canvas.RenderPage(page);

		if (options.include_images)
		{
			for (auto&& image : page.images())
			{
				auto image_filename = (std::filesystem::path(options.output_dir) / image.getName()).string();
				std::cerr << "   - Extracting image " << DF::w_to_u8(image.getName()) << " to " << image_filename << std::endl;
				image.CopyTo(DF::u8_to_w(image_filename));
			}
		};
	}
}

int main(int argc, char* argv[])
{
	CLI::App app("Hyland Document Filters: ConvertDocumentToMarkdown");

	try {
		options_t options;

		app.add_option("filename", options.filenames, "Files to convert")->required();
		app.add_option("-o,--output", options.output_dir, "Output directory");
		app.add_option("-l,--license", options.license_key, "License key for Document Filters");
		app.add_option("--metadata", options.metadata_format, "Include metadata in the markdown output")
			->required()
			->transform(CLI::CheckedTransformer(metadata_format_map, CLI::ignore_case));
		app.add_option("--heading-style", options.heading_style, "Heading style to use in the markdown output")
			->transform(CLI::CheckedTransformer(heading_style_map, CLI::ignore_case));
		app.add_option("--simple-table-style", options.simple_table_style, "Table style to use for simple tables in the markdown output")
			->transform(CLI::CheckedTransformer(table_style_map, CLI::ignore_case));
		app.add_option("--complex-table-style", options.complex_table_style, "Table style to use for complex tables in the markdown output")
			->transform(CLI::CheckedTransformer(table_style_map, CLI::ignore_case));
		app.add_flag("--images,!--no-images", options.include_images, "Include images in markdown output");
		app.add_flag("--fields,!--no-fields", options.include_fields, "Include fields in markdown output");
		app.add_flag("--headers,!--no-headers", options.include_headers, "Include headers in markdown output");
		app.add_flag("--footers,!--no-footers", options.include_footers, "Include footers in markdown output");
		app.add_flag("--bookmarks,!--no-bookmarks", options.include_bookmarks, "Include bookmarks in markdown output");
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
